/***************************************************************************
                          datavector.cpp  -  a vector which gets its data from
                          a datasource.
                             -------------------
    begin                : Fri Sep 22 2000
    copyright            : (C) 2000-2015 by C. Barth Netterfield
    email                : netterfield@astro.utoronto.ca
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *   Permission is granted to link with any opensource library             *
 *                                                                         *
 ***************************************************************************/
#include "datavector.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <QDebug>
#include <QXmlStreamWriter>

#include "datacollection.h"
#include "debug.h"
#include "datasource.h"
#include "math_kst.h"
#include "objectstore.h"
#include "updatemanager.h"
#include "vectorscriptinterface.h"

// ReqNF <=0 means read from ReqF0 to end of File
// ReqF0 < means start at EndOfFile-ReqNF.
//
// ReqNF      ReqF0      Action
//  < 1        >=0       read from ReqF0 to end of file
//  < 1        < 0       illegal: fixed in checkIntegrity
//    1         ??       illegal: fixed in checkIntegrity
//  > 1        < 0       read the last ReqNF frames from the file
//  > 1        >=0       Read ReqNF frames starting at frame ReqF0

namespace Kst {

const QString DataVector::staticTypeString = "Data Vector";
const QString DataVector::staticTypeTag = "datavector";

//const int INVALIDS_PER_RESET = 5;

DataVector::DataInfo::DataInfo() :
    frameCount(-1),
    samplesPerFrame(-1)
{
}


DataVector::DataInfo::DataInfo(int fc, int spf) :
    frameCount(fc),
    samplesPerFrame(spf)
{
}

/** Create a DataVector: raw data from a file */
DataVector::DataVector(ObjectStore *store)
: Vector(store), DataPrimitive(this) {

  _saveable = true;
  _numSamples = 0;
  _scalars["sum"]->setValue(0.0);
  _scalars["sumsquared"]->setValue(0.0);
  F0 = NF = 0; // nothing read yet

  N_AveReadBuf = 0;
  AveReadBuf = 0L;

  ReqF0 = 0;
  ReqNF = -1;
  Skip = 1;
  DoSkip = false;
  DoAve = false;
  _invalidCount = 0;
}


const QString& DataVector::typeString() const {
  return staticTypeString;
}


/** return true if it has a valid file and field, or false otherwise */
bool DataVector::isValid() const {
  if (dataSource()) {
    dataSource()->readLock();
    bool rc = dataSource()->vector().isValid(_field);
    dataSource()->unlock();
    return rc;
  }
  return false;
}


ScriptInterface* DataVector::createScriptInterface() {
  return new DataVectorSI(this);
}


bool DataVector::checkValidity(const DataSourcePtr& ds) const {
  if (ds) {
    ds->readLock();
    bool rc = ds->vector().isValid(_field);
    ds->unlock();
    return rc;
  }
  return false;
}

void DataVector::change(DataSourcePtr in_file, const QString &in_field,
                        int in_f0, int in_n,
                        int in_skip, bool in_DoSkip,
                        bool in_DoAve) {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  Skip = in_skip;
  DoSkip = in_DoSkip;
  DoAve = in_DoAve;
  if (DoSkip && Skip < 1) {
    Skip = 1;
  }

  setDataSource(in_file);
  ReqF0 = in_f0;
  ReqNF = in_n;
  _field = in_field;

  if (dataSource()) {
    dataSource()->writeLock();
  }
  reset();
  if (dataSource()) {
    dataSource()->unlock();
  }

  if (ReqNF <= 0 && ReqF0 < 0) {
    ReqF0 = 0;
  }
  registerChange();
}

qint64 DataVector::minInputSerial() const {
  if (dataSource()) {
    return (dataSource()->serial());
  }
  return LLONG_MAX;
}

qint64 DataVector::maxInputSerialOfLastChange() const {
  if (dataSource()) {
    return (dataSource()->serialOfLastChange());
  }
  return NoInputs;
}


void DataVector::changeFile(DataSourcePtr in_file) {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  if (!in_file) {
    Debug::self()->log(tr("Data file for vector %1 was not opened.").arg(Name()), Debug::Warning);
  }
  setDataSource(in_file);
  if (dataSource()) {
    dataSource()->writeLock();
  }
  reset();
  if (dataSource()) {
    dataSource()->unlock();
  }
  registerChange();
}


void DataVector::changeFrames(int in_f0, int in_n,
                              int in_skip, bool in_DoSkip,
                              bool in_DoAve) {

  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  if (dataSource()) {
    dataSource()->writeLock();
  }
  reset();
  if (dataSource()) {
    dataSource()->unlock();
  }
  Skip = in_skip;
  DoSkip = in_DoSkip;
  DoAve = in_DoAve;
  if (DoSkip && Skip < 1) {
    Skip = 1;
  }

  ReqF0 = in_f0;
  ReqNF = in_n;

  if (ReqNF <= 0 && ReqF0 < 0) {
    ReqF0 = 0;
  }
  registerChange();
}


void DataVector::setFromEnd() {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  ReqF0 = -1;
  if (ReqNF < 2) {
    ReqNF = numFrames();
    if (ReqNF < 2) {
      ReqF0 = 0;
    }
  }
  registerChange();
}


DataVector::~DataVector() {
  if (AveReadBuf) {
    free(AveReadBuf);
    AveReadBuf = 0L;
  }
}


bool DataVector::readToEOF() const {
  return ReqNF <= 0;
}


bool DataVector::countFromEOF() const {
  return ReqF0 < 0;
}


/** Return Starting Frame of Vector */
int DataVector::startFrame() const {
  return F0;
}


/** Return frames per skip to read */
int DataVector::skip() const {
  return DoSkip ? Skip : 0;
}


bool DataVector::doSkip() const {
  return DoSkip;
}


bool DataVector::doAve() const {
  return DoAve;
}


/** Return frames held in Vector */
int DataVector::numFrames() const {
  return NF;
}


int DataVector::reqNumFrames() const {
  return ReqNF;
}


int DataVector::reqStartFrame() const {
  return ReqF0;
}


/** Save vector information */
void DataVector::save(QXmlStreamWriter &s) {
  if (dataSource()) {
    s.writeStartElement("datavector");
    saveFilename(s);
    s.writeAttribute("field", _field);

    s.writeAttribute("start", QString::number(ReqF0));
    s.writeAttribute("count", QString::number(ReqNF));

    if (doSkip()) {
      s.writeAttribute("skip", QString::number(Skip));
      if (doAve()) {
        s.writeAttribute("doAve", "true");
      }
    } else {
      s.writeAttribute("skip", QString::number(-1));
      s.writeAttribute("doAve", "false");
    }

    s.writeAttribute("startUnits", startUnits());
    s.writeAttribute("rangeUnits", rangeUnits());

    saveNameInfo(s, VECTORNUM|SCALARNUM);
    s.writeEndElement();
  }
}


/**
 * @brief Generate default label info for axis associated with this vector.
 * Use meta-scalars "units" or "quantity" if they are defined.
 * Escape special characters in the field name.
 *
 * @return LabelInfo
 **/

LabelInfo DataVector::labelInfo() const {
  LabelInfo label_info;

  if (_fieldStrings.contains("quantity")) {
    label_info.quantity = _fieldStrings.value("quantity")->value();
    label_info.quantity.replace('[', "\\[").replace(']', "\\]");
  } else {
    label_info.quantity.clear();
  }

  if (_fieldStrings.contains("units")) {
    label_info.units = _fieldStrings.value("units")->value();
    label_info.units.replace('[', "\\[").replace(']', "\\]");
  } else {
    label_info.units.clear();
  }

  label_info.name = descriptiveName();// _field;

  label_info.file = filename();

  return label_info;
}


void DataVector::reset() { // must be called with a lock
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  if (dataSource()) {
    SPF = dataInfo(_field).samplesPerFrame;
  }
  F0 = NF = 0;
  resize(0);
  _numSamples = 0;
  _dirty = true;
  _resetFieldMetadata();

  Object::reset();
}


bool DataVector::checkIntegrity() {
  if (DoSkip && Skip < 1) {
    Skip = 1;
  }

  if (_dirty) {
    reset();
  }

  // if the file seems to have shrunk/changed, return false.
  // if it has happened several times in a row, assume the
  // file has been over-written, and re-read it.
  // this is a hack to handle glitchy file system situations.
  // TODO: there has to be a better way.
  // const DataInfo info = dataInfo(_field);
  // if (dataSource() && (SPF != info.samplesPerFrame || info.frameCount < NF)) {
  //   _invalidCount++;
  //   if (_invalidCount>INVALIDS_PER_RESET) {

  //     reset();
  //     _invalidCount=0;
  //   }
  //   return false;
  // }

  // check for illegal NF and F0 values
  if (ReqNF < 1 && ReqF0 < 0) {
    ReqF0 = 0; // for this illegal case, read the whole file
  }

  if (ReqNF == 1) {
    ReqNF = 2;
  }

  _invalidCount = 0;
  return true;
}

// Some things to consider about the following routine...
// Frames:
//    Some data sources have data divided into frames.  Each field
//    has a fixed number of samples per frame.  For some (eg, ascii files)
//    each frame has 1 sample.  For others (eg, dirfiles) you may have more.
//    Different fields in the same data source may have different samples per frame.
//    Within a data source, it is assumed that the first sample of each frame is
//    simultaneous between fields.
// Last Frame Read:
//    Only read the first sample of the last frame read, in cases where there are more
//    than one sample per frame.   This allows for more sensible association of vectors
//    into curves, when the X and Y vectors have different numbers of samples per frame.
//    The rule is that we assume is that the first sample of each frame is simultaneous.
// Skip reading:  
//    -'Skip' means read 1 sample each 'Skip' frames (not read one sample,
//     then skip 'Skip' samples or something else).
//    -In order that the data are not re-drawn each time a new sample arrives, and to
//     ensure the re-usability (via shifting) of previously read data, and to make a
//     region of data look the same regardless of the chouse of f0, all samples
//     read with skip enabled are read on 'skip boundries'... ie, the first samples of
//     frame 0, Skip, 2*Skip... N*skip, and never M*Skip+1.

void DataVector::internalUpdate() {
  int i, k, shift, n_read=0;
  int ave_nread;
  int new_f0, new_nf;
  bool start_past_eof = false;

  if (dataSource()) {
    dataSource()->writeLock();
  } else {
    return;
  }

  const DataInfo info = dataInfo(_field);
  if (!checkIntegrity()) {
    if (dataSource()) {
      dataSource()->unlock();
    }
    return;
  }

  if (DoSkip && Skip < 2 && SPF == 1) {
    DoSkip = false;
  }

  // set new_nf and new_f0
  int fc = info.frameCount;
  if (ReqNF < 1) { // read to end of file
    new_f0 = ReqF0;
    new_nf = fc - new_f0;
  } else if (ReqF0 < 0) { // count back from end of file
    new_nf = fc;
    if (new_nf > ReqNF) {
      new_nf = ReqNF;
    }
    new_f0 = fc - new_nf;
  } else {
    new_f0 = ReqF0;
    new_nf = ReqNF;
    if (new_f0 + new_nf > fc) {
      new_nf = fc - new_f0;
    }
    if (new_nf <= 0) {
      // Tried to read starting past the end.
      new_f0 = 0;
      new_nf = 1;
      start_past_eof = true;
    }
  }

  if (DoSkip) {
    // in count from end mode, change new_f0 and new_nf so they both lie on skip boundaries
    if ((new_f0 != 0) && (ReqF0<0)) {
      new_f0 = ((new_f0-1)/Skip+1)*Skip;
    }
    new_nf = (new_nf/Skip)*Skip;
  }

  // shift vector if necessary
  if ((NF>new_nf) || new_f0 < F0 || new_f0 >= F0 + NF) { // No useful data around.
    reset();
  } else { // shift stuff rather than re-read
    if (DoSkip) {
      shift = (new_f0 - F0)/Skip;
      NF -= (new_f0 - F0);
      _numSamples = NF/Skip;
    } else {
      shift = SPF*(new_f0 - F0);
      NF -= (new_f0 - F0);
      //_numSamples = (NF-1)*SPF;
      if (shift > 0) {
        if (SPF == 1) {
          _numSamples = NF;
        } else {
          _numSamples = (NF-1)*SPF;
        }
      }
    }

    memmove(_v_raw, _v_raw+shift, _numSamples*sizeof(double));
  }

  if (DoSkip) {
    // reallocate V if necessary
    if (new_nf / Skip != _size) {
      if (! resize(new_nf/Skip)) {
        // TODO: Is aborting all we can do?
        fatalError("Not enough memory for vector data");
        return;
      }
    }
    n_read = 0;
    /** read each sample from the File */
    double *t = _v_raw + _numSamples;
    int new_nf_Skip = new_nf - Skip;
    if (DoAve) {
      for (i = NF; new_nf_Skip >= i; i += Skip) {
        /* enlarge AveReadBuf if necessary */
        if (N_AveReadBuf < Skip*SPF) {
          N_AveReadBuf = Skip*SPF;
          if (!kstrealloc(AveReadBuf, N_AveReadBuf*sizeof(double))) {
            qCritical() << "Vector resize failed";
          }
          if (!AveReadBuf) {
            // FIXME: handle failed resize
          }
        }
        ave_nread = readField(AveReadBuf, _field, new_f0+i, Skip);
        for (k = 1; k < ave_nread; ++k) {
          AveReadBuf[0] += AveReadBuf[k];
        }
        if (ave_nread > 0) {
          *t = AveReadBuf[0]/double(ave_nread);
          n_read++;
        }
        ++t;
      }
    } else {
      for (i = NF; new_nf_Skip >= i; i += Skip) {
        n_read += readField(t++, _field, new_f0 + i, -1);
      }
    }
  } else {
    // reallocate V if necessary
    if ((new_nf - 1)*SPF + 1 != _size) {
      if (!resize((new_nf - 1)*SPF + 1)) {
        // TODO: Is aborting all we can do?
        fatalError("Not enough memory for vector data");
        return;
      }
    }

    // read the new data from file
    if (start_past_eof) {
      _v_raw[0] = NOPOINT;
      n_read = 1;
    } else if (info.samplesPerFrame > 1) {
      if (NF>0) {
        NF--;  /* last frame read was only partially read... */
      }
      int safe_nf = (new_nf>0 ? new_nf : 0);

      assert(new_f0 + NF >= 0);
      //assert(new_f0 + safe_nf - 1 >= 0);
      if ((new_f0 + safe_nf - 1 >= 0) && (safe_nf - NF > 1)) {
        n_read = readField(_v_raw+NF*SPF, _field, new_f0 + NF, safe_nf - NF - 1);
        n_read += readField(_v_raw+(safe_nf-1)*SPF, _field, new_f0 + safe_nf - 1, -1);
      }
    } else {
      assert(new_f0 + NF >= 0);
      if (new_nf - NF > 0 || new_nf - NF == -1) {
        n_read = readField(_v_raw+NF*SPF, _field, new_f0 + NF, new_nf - NF);
      }
    }
  }
  _numNew = _size - _numSamples;
  NF = new_nf;
  F0 = new_f0;
  _numSamples += n_read;

  // if for some reason (eg, realtime reading an nfs mounted
  // dirfile) not all of the data was read, the data will never
  // be read; the samples will be filled in with the last data
  // point read, and a complete reset will be requested.
  // This is bad - I think it will be worthwhile
  // to add blocking w/ timeout to KstFile.
  // As a first fix, mount all nsf mounted dirfiles with "-o noac"
  _dirty = false;
  if (_numSamples != _size && !(_numSamples == 0 && _size == 1)) {
    _dirty = true;
    for (i = _numSamples; i < _size; ++i) {
      _v_raw[i] = _v_raw[0];
    }
  }

  if (_numNew > _size) {
    _numNew = _size;
  }
  if (_numShifted > _size) {
    _numShifted = _size;
  }

  if (dataSource()) {
    dataSource()->unlock();
  }

  if (n_read>0) {
    Vector::internalUpdate();
  }
}

QByteArray DataVector::scriptInterface(QList<QByteArray> &c)
{
  Q_ASSERT(c.size());
  if(c[0]=="changeFrames") {
    if(c.size()!=6) {
      return QByteArray("Bad parameter count (!=5).");
    }
    changeFrames(c[1].toInt(),c[2].toInt(),c[3].toInt(),(c[4]=="true")?1:0,(c[5]=="true")?1:0);
    return QByteArray("Done.");
  } else if(c[0]=="numFrames") {
    return QByteArray::number(numFrames());
  } else if(c[0]=="startFrame") {
    return QByteArray::number(startFrame());
  } else if(c[0]=="doSkip") {
    return QByteArray(doSkip()?"true":"false");
  } else if(c[0]=="doAve") {
    return QByteArray(doAve()?"true":"false");
  } else if(c[0]=="skip") {
    return QByteArray::number(skip());
  } else if(c[0]=="reload") {
    reload();
    return QByteArray("Done");
  } else if(c[0]=="samplesPerFrame") {
    return QByteArray::number(samplesPerFrame());
  } else if(c[0]=="fileLength") {
    return QByteArray::number(fileLength());
  } else if(c[0]=="readToEOF") {
    return QByteArray(readToEOF()?"true":"false");
  } else if(c[0]=="countFromEOF") {
    return QByteArray(countFromEOF()?"true":"false");
  } else if(c[0]=="descriptionTip") {
    return QByteArray(descriptionTip().toLatin1());
  } else if(c[0]=="isValid") {
    return isValid()?"true":"false";
  }

  return "No such command...";
}


/** Returns intrinsic samples per frame */
int DataVector::samplesPerFrame() const {
  return SPF;
}


int DataVector::fileLength() const {

  if (dataSource()) {
    int rc = dataInfo(_field).frameCount;

    return rc;
  }

  return 0;
}


void DataVector::reload() {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  if (dataSource()) {
    dataSource()->writeLock();
    dataSource()->reset();
    dataSource()->unlock();
    reset();
    _resetFieldMetadata();
    registerChange();
  }
}

void DataVector::_resetFieldMetadata() {
  _resetFieldScalars();
  _resetFieldStrings();
}

void DataVector::_resetFieldStrings() {
  const QMap<QString, QString> meta_strings = dataSource()->vector().metaStrings(_field);
  
  QStringList fieldStringKeys = _fieldStrings.keys();
  // remove field strings that no longer need to exist
  readLock();
  for (int i=0; i<fieldStringKeys.count(); ++i) {
    QString key = fieldStringKeys.at(i);
    if (!meta_strings.contains(key)) {
      StringPtr sp = _fieldStrings[key];
      _strings.remove(key);
      _fieldStrings.remove(key);
      sp = 0L;
    }
  }
  // find or insert strings, to set their value
  QMapIterator<QString, QString> it(meta_strings);
  while (it.hasNext()) {
    it.next();
    QString key = it.key();
    StringPtr sp;
    if (!_fieldStrings.contains(key)) { // insert a new one
      _strings.insert(key, sp = store()->createObject<String>());
      _fieldStrings.insert(key, sp);
      sp->setProvider(this);
      sp->setSlaveName(key);
    } else {  // find it
      sp = _fieldStrings[key];
    }
    sp->setValue(it.value());
  }
  unlock();
}


void DataVector::_resetFieldScalars() {
  const QMap<QString, double> meta_scalars = dataSource()->vector().metaScalars(_field);


  QStringList fieldScalarKeys = _fieldScalars.keys();
  // remove field scalars that no longer need to exist
  readLock();
  for (int i=0; i<fieldScalarKeys.count(); ++i) {
    QString key = fieldScalarKeys.at(i);
    if (!meta_scalars.contains(key)) {
      ScalarPtr sp = _fieldScalars[key];
      _scalars.remove(key);
      _fieldScalars.remove(key);
      sp = 0L;
    }
  }
  // find or insert scalars, to set their value
  QMapIterator<QString, double> it(meta_scalars);
  while (it.hasNext()) {
    it.next();
    QString key = it.key();
    ScalarPtr sp;
    if (!_fieldScalars.contains(key)) { // insert a new one
      _scalars.insert(key, sp = store()->createObject<Scalar>());
      _fieldScalars.insert(key, sp);
      sp->setProvider(this);
      sp->setSlaveName(key);
    } else {  // find it
      sp = _fieldScalars[key];
    }
    sp->setValue(it.value());
  }
  unlock();
}


PrimitivePtr DataVector::makeDuplicate() const {
  Q_ASSERT(store());
  DataVectorPtr vector = store()->createObject<DataVector>();

  vector->writeLock();
  vector->change(dataSource(), _field, ReqF0, ReqNF, Skip, DoSkip, DoAve);
  if (descriptiveNameIsManual()) {
    vector->setDescriptiveName(descriptiveName());
  }

  vector->registerChange();
  vector->unlock();

  return kst_cast<Primitive>(vector);
}

QString DataVector::_automaticDescriptiveName() const {
  QString name;
  name = _field;
  // un-escape escaped special characters so they aren't escaped 2x.
  name.replace("\\_", "_").replace("\\^","^").replace("\\[", "[").replace("\\]", "]");
  // now escape the special characters.
  name.replace('_', "\\_").replace('^', "\\^").replace('[', "\\[").replace(']', "\\]");
  return name;
}

QString DataVector::descriptionTip() const {
  QString IDstring;
  //QString range_string;

  if (dataSource()) {
    IDstring = tr(
                 "Data Vector: %1\n"
                 "  %2\n"
                 "  Field: %3"
                 ).arg(Name()).arg(dataSource()->fileName()).arg(_field);
    if (countFromEOF()) {
      IDstring += tr("\n  Last %1 frames.").arg(numFrames());
    } else if (readToEOF()) {
      IDstring += tr("\n  Frame %1 to end.").arg(startFrame());
    } else {
      IDstring += tr("\n  %1 Frames starting at %2.").arg(numFrames()).arg(startFrame());
    }
    if (skip()) {
      if (!doAve()) {
        IDstring+=tr("\n  Read 1 sample per %1 frames.").arg(skip());
      } else {
        IDstring+=tr("\n  Average each %1 frames.").arg(skip());
      }
    }
  } else{
    IDstring.clear();
  }

  return IDstring;
}

QString DataVector::propertyString() const {
  return tr("%2 F0: %3 N: %4 of %1", "%2 is a variable name.  F0 is short for the first element.  N is the number of elements").arg(dataSource()->fileName()).arg(_field).arg(startFrame()).arg(numFrames());
}


int DataVector::readField(double *v, const QString& field, int s, int n, int skip)
{
  ReadInfo par = {v, s, n, skip};
  return dataSource()->vector().read(field, par);
}

const DataVector::DataInfo DataVector::dataInfo(const QString& field) const
{
  dataSource()->readLock();
  const DataInfo info = dataSource()->vector().dataInfo(field);
  dataSource()->unlock();
  return info;
}

bool DataVector::isTime() const {
  return dataSource()->isTime(_field);
}

}
// vim: ts=2 sw=2 et
