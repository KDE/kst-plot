/***************************************************************************
                netcdf.cpp  -  netCDF file data source reader
                             -------------------
    begin                : 17/06/2004
    copyright            : (C) 2004 Nicolas Brisset <nicodev@users.sourceforge.net>
    email                : kst@kde.org
    modified             : 03/14/05 by K. Scott
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// TODO move
#include "sharedptr.h"

#include "netcdfsource.h"

#include "debug.h"

#include <QFile>
#include <QFileInfo>

#include <ctype.h>
#include <stdlib.h>

//#define NETCDF_DEBUG_SHARED
#ifdef NETCDF_DEBUG_SHARED
#define NETCDF_DBG if (true)
#else
#define NETCDF_DBG if (false)
#endif


using namespace Kst;

static const QString netCdfTypeString = "netCDF Files";


//
// Scalar interface
//

class DataInterfaceNetCdfScalar : public DataSource::DataInterface<DataScalar>
{
public:
  DataInterfaceNetCdfScalar(NetcdfSource& s) : netcdf(s) {}

  // read one element
  int read(const QString&, DataScalar::ReadInfo&);

  // named elements
  QStringList list() const { return netcdf._scalarList; }
  bool isListComplete() const { return true; }
  bool isValid(const QString&) const;

  // T specific
  const DataScalar::DataInfo dataInfo(const QString&, int frame = 0) const { Q_UNUSED(frame) return DataScalar::DataInfo(); }
  void setDataInfo(const QString&, const DataScalar::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&) { return QMap<QString, double>(); }
  QMap<QString, QString> metaStrings(const QString&) { return QMap<QString, QString>(); }


private:
  NetcdfSource& netcdf;
};


int DataInterfaceNetCdfScalar::read(const QString& scalar, DataScalar::ReadInfo& p)
{
  return netcdf.readScalar(p.value, scalar);
}


bool DataInterfaceNetCdfScalar::isValid(const QString& scalar) const
{
  return  netcdf._scalarList.contains( scalar );
}


//
// String interface
//

class DataInterfaceNetCdfString : public DataSource::DataInterface<DataString>
{
public:
  DataInterfaceNetCdfString(NetcdfSource& s) : netcdf(s) {}

  // read one element
  int read(const QString&, DataString::ReadInfo&);

  // named elements
  QStringList list() const { return netcdf._strings.keys(); }
  bool isListComplete() const { return true; }
  bool isValid(const QString&) const;

  // T specific
  const DataString::DataInfo dataInfo(const QString&, int frame=0) const { Q_UNUSED(frame) return DataString::DataInfo(); }
  void setDataInfo(const QString&, const DataString::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&) { return QMap<QString, double>(); }
  QMap<QString, QString> metaStrings(const QString&) { return QMap<QString, QString>(); }


private:
  NetcdfSource& netcdf;
};


//-------------------------------------------------------------------------------------------
int DataInterfaceNetCdfString::read(const QString& string, DataString::ReadInfo& p)
{
  //return netcdf.readString(p.value, string);
  if (isValid(string) && p.value) {
    *p.value = netcdf._strings[string];
    return 1;
  }
  return 0;
}


bool DataInterfaceNetCdfString::isValid(const QString& string) const
{
  return netcdf._strings.contains( string );
}





//
// Vector interface
//

class DataInterfaceNetCdfVector : public DataSource::DataInterface<DataVector>
{
public:
  DataInterfaceNetCdfVector(NetcdfSource& s) : netcdf(s) {}

  // read one element
  int read(const QString&, DataVector::ReadInfo&);

  // named elements
  QStringList list() const { return netcdf._fieldList; }
  bool isListComplete() const { return true; }
  bool isValid(const QString&) const;

  // T specific
  const DataVector::DataInfo dataInfo(const QString&, int frame=0) const;
  void setDataInfo(const QString&, const DataVector::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&);
  QMap<QString, QString> metaStrings(const QString&);


private:
  NetcdfSource& netcdf;
};


const DataVector::DataInfo DataInterfaceNetCdfVector::dataInfo(const QString &field, int frame) const
{
  Q_UNUSED(frame)
  if (!netcdf._fieldList.contains(field))
    return DataVector::DataInfo();

  return DataVector::DataInfo(netcdf.frameCount(field), netcdf.samplesPerFrame(field));
}



int DataInterfaceNetCdfVector::read(const QString& field, DataVector::ReadInfo& p)
{
  return netcdf.readField(p.data, field, p.startingFrame, p.numberOfFrames);
}


bool DataInterfaceNetCdfVector::isValid(const QString& field) const
{
  return  netcdf._fieldList.contains( field );
}

QMap<QString, double> DataInterfaceNetCdfVector::metaScalars(const QString& field)
{
  NcVar *var = 0;
  if (field != "INDEX") {
    var = netcdf._ncfile->get_var((NcToken) field.toLatin1().constData());
  }
  if (!var) {
    NETCDF_DBG qDebug() << "Queried field " << field << " which can't be read" << Qt::endl;
    return QMap<QString, double>();
  }
  QMap<QString, double> fieldScalars;
  fieldScalars["NbAttributes"] = var->num_atts();
  for (int i=0; i<var->num_atts(); ++i) {
    NcAtt *att = var->get_att(i);
    // Only handle char attributes as fieldStrings, the others as fieldScalars
    if (att->type() == ncByte || att->type() == ncShort || att->type() == ncInt
        || att->type() == ncLong || att->type() == ncFloat || att->type() == ncDouble) {
      // Some attributes may have multiple values => load the first as is, and for the others
      // add a -2, -3, etc... suffix as obviously we can have only one value per scalar.
      // Do it in two steps to avoid a test in the loop while keeping a "clean" name for the first one
      fieldScalars[QString(att->name())] = att->values()->as_double(0);
      for (int j=1; j<att->values()->num(); ++j) {
        fieldScalars[QString(att->name()) + QString("-") + QString::number(j+1)] = att->values()->as_double(j);
      }
    }
  }
  return fieldScalars;
}

QMap<QString, QString> DataInterfaceNetCdfVector::metaStrings(const QString& field)
{
  NcVar *var = 0;
  if (field != "INDEX") {
    var = netcdf._ncfile->get_var(field.toLatin1().constData());
  }
  if (!var) {
    NETCDF_DBG qDebug() << "Queried field " << field << " which can't be read" << Qt::endl;
    return QMap<QString, QString>();
  }
  QMap<QString, QString> fieldStrings;
  QString tmpString;
  for (int i=0; i<var->num_atts(); ++i) {
    NcAtt *att = var->get_att(i);
    // Only handle char/unspecified attributes as fieldStrings, the others as fieldScalars
    if (att->type() == ncChar || att->type() == ncNoType) {
      fieldStrings[att->name()] = QString(att->values()->as_string(0));
    }
    // qDebug() << att->name() << ": " << att->values()->num() << Qt::endl;
  }
  return fieldStrings;
}


//
// Matrix interface
//

class DataInterfaceNetCdfMatrix : public DataSource::DataInterface<DataMatrix>
{
public:

  DataInterfaceNetCdfMatrix(NetcdfSource& s) : netcdf(s) {}

  // read one element
  int read(const QString&, DataMatrix::ReadInfo&);

  // named elements
  QStringList list() const { return netcdf._matrixList; }
  bool isListComplete() const { return true; }
  bool isValid(const QString&) const;

  // T specific
  const DataMatrix::DataInfo dataInfo	(const QString&, int frame) const;
  void setDataInfo(const QString&, const DataMatrix::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&) { return QMap<QString, double>(); }
  QMap<QString, QString> metaStrings(const QString&) { return QMap<QString, QString>(); }


private:
  NetcdfSource& netcdf;
};


const DataMatrix::DataInfo DataInterfaceNetCdfMatrix::dataInfo(const QString& matrix, int frame) const
{
  Q_UNUSED(frame)
  if (!netcdf._matrixList.contains( matrix ) ) {
    return DataMatrix::DataInfo();
  }

  QByteArray bytes = matrix.toLatin1();
  NcVar *var = netcdf._ncfile->get_var(bytes.constData());  // var is owned by _ncfile
  if (!var) {
    return DataMatrix::DataInfo();
  }

  if (var->num_dims() != 2) {
    return DataMatrix::DataInfo();
  }

  DataMatrix::DataInfo info;
  // TODO is this right?
  info.xSize = var->get_dim(0)->size();
  info.ySize = var->get_dim(1)->size();

  return info;
}


int DataInterfaceNetCdfMatrix::read(const QString& field, DataMatrix::ReadInfo& p)
{
  int count = netcdf.readMatrix(p.data->z, field);

  p.data->xMin = 0;
  p.data->yMin = 0;
  p.data->xStepSize = 1;
  p.data->yStepSize = 1;

  return count;
}


bool DataInterfaceNetCdfMatrix::isValid(const QString& field) const {
  return  netcdf._matrixList.contains( field );
}


//
// NetcdfSource
//

NetcdfSource::NetcdfSource(Kst::ObjectStore *store, QSettings *cfg, const QString& filename, const QString& type, const QDomElement &element) :
  Kst::DataSource(store, cfg, filename, type),
  _ncfile(0L),
  _ncErr(NcError::silent_nonfatal),
  is(new DataInterfaceNetCdfScalar(*this)),
  it(new DataInterfaceNetCdfString(*this)),
  iv(new DataInterfaceNetCdfVector(*this)),
  im(new DataInterfaceNetCdfMatrix(*this))
  {
  setInterface(is);
  setInterface(it);
  setInterface(iv);
  setInterface(im);

  setUpdateType(None);

  if (!type.isEmpty() && type != "netCDF") {
    return;
  }

  _valid = false;
  _maxFrameCount = 0;

  _filename = filename;
  _strings = fileMetas();
  _valid = initFile();
}


NetcdfSource::~NetcdfSource() {
  delete _ncfile;
  _ncfile = 0L;
}


void NetcdfSource::reset() {
  delete _ncfile;
  _ncfile = 0L;
  _maxFrameCount = 0;
  _valid = initFile();
}


bool NetcdfSource::initFile() {
  _ncfile = new NcFile(_filename.toUtf8().data(), NcFile::ReadOnly);
  if (!_ncfile->is_valid()) {
      qDebug() << _filename << ": failed to open in initFile()" << Qt::endl;
      return false;
    }

  NETCDF_DBG qDebug() << _filename << ": building field list" << Qt::endl;
  _fieldList.clear();
  _fieldList += "INDEX";

  int nb_vars = _ncfile->num_vars();
  NETCDF_DBG qDebug() << nb_vars << " vars found in total" << Qt::endl;

  _maxFrameCount = 0;

  for (int i = 0; i < nb_vars; i++) {
    NcVar *var = _ncfile->get_var(i);
    if (!var) {
      continue;
    }
    if (var->num_dims() == 0) {
      _scalarList += var->name();
    } else if (var->num_dims() == 1) {
      _fieldList += var->name();
      int fc = var->num_vals() / var->rec_size();
      _maxFrameCount = qMax(_maxFrameCount, fc);
      _frameCounts[var->name()] = fc;
    } else if (var->num_dims() == 2) {
      _matrixList += var->name();
    }
  }

  // Get strings
  int globalAttributesNb = _ncfile->num_atts();
  for (int i = 0; i < globalAttributesNb; ++i) {
    // Get only first value, should be enough for a start especially as strings are complete
    NcAtt *att = _ncfile->get_att(i);
    if (att) {
      QString attrName = QString(att->name());
      char *attString = att->as_string(0);
      QString attrValue = QString(att->as_string(0));
      delete[] attString;
      //TODO port
      //KstString *ms = new KstString(KstObjectTag(attrName, tag()), this, attrValue);
      _strings[attrName] = attrValue;
    }
    delete att;
  }
  setUpdateType(Timer);

  NETCDF_DBG qDebug() << "netcdf file initialized";
  // TODO update(); // necessary?  slows down initial loading
  return true;
}



Kst::Object::UpdateType NetcdfSource::internalDataSourceUpdate() {
  //TODO port
  /*
  if (KstObject::checkUpdateCounter(u)) {
    return lastUpdateResult();
  }
  */

  _ncfile->sync();

  bool updated = false;
  /* Update member variables _ncfile, _maxFrameCount, and _frameCounts
     and indicate that an update is needed */
  int nb_vars = _ncfile->num_vars();
  for (int j = 0; j < nb_vars; j++) {
    NcVar *var = _ncfile->get_var(j);
    if (!var) {
      continue;
    }
    int fc = var->num_vals() / var->rec_size();
    _maxFrameCount = qMax(_maxFrameCount, fc);
    updated = updated || (_frameCounts[var->name()] != fc);
    _frameCounts[var->name()] = fc;
  }
  return updated ? Object::Updated : Object::NoChange;
}


int NetcdfSource::readScalar(double *v, const QString& field)
{
  // TODO error handling
  QByteArray bytes = field.toLatin1();
  NcVar *var = _ncfile->get_var(bytes.constData());  // var is owned by _ncfile
  if (var) {
    var->get(v);
    return 1;
  }
  return 0;
}

int NetcdfSource::readString(QString *stringValue, const QString& stringName)
{
  // TODO more error handling?
  NcAtt *att = _ncfile->get_att((NcToken) stringName.toLatin1().data());
  if (att) {
    *stringValue = QString(att->as_string(0));
    delete att;
    return 1;
  }
  return 0;
}

int NetcdfSource::readField(double *v, const QString& field, int s, int n) {
  NcType dataType = ncNoType; /* netCDF data type */
  /* Values for one record */
  NcValues *record = 0;// = new NcValues(dataType,numFrameVals);

  NETCDF_DBG qDebug() << "Entering NetcdfSource::readField with params: " << field << ", from " << s << " for " << n << " frames" << Qt::endl;

  /* For INDEX field */
  if (field.toLower() == "index") {
    if (n < 0) {
      v[0] = double(s);
      return 1;
    }
    for (int i = 0; i < n; ++i) {
      v[i] = double(s + i);
    }
    return n;
  }

  /* For a variable from the netCDF file */
  QByteArray bytes = field.toLatin1();
  NcVar *var = _ncfile->get_var(bytes.constData());  // var is owned by _ncfile
  if (!var) {
    NETCDF_DBG qDebug() << "Queried field " << field << " which can't be read" << Qt::endl;
    return -1;
  }

  dataType = var->type();

  if (s >= var->num_vals() / var->rec_size()) {
    return 0;
  }

  bool oneSample = n < 0;
  int recSize = var->rec_size();
  double add_offset = 1.0, scale_factor = 1.0;
  switch (dataType) {
    case ncShort:
      {
        // Check for special attributes add_offset and scale_factor indicating the use of the convention described in
        // <http://www.unidata.ucar.edu/software/netcdf/docs/netcdf/Attribute-Conventions.html>
        bool packed = iv->metaScalars(field).contains("add_offset") && iv->metaScalars(field).contains("scale_factor");
        if (packed) {
          // Get the values into local vars
            add_offset = iv->metaScalars(field)["add_offset"];
            scale_factor = iv->metaScalars(field)["scale_factor"];
        }
        if (oneSample) {
          record = var->get_rec(s);
          v[0] = packed ? record->as_short(0)*scale_factor+add_offset : record->as_short(0);
          delete record;
        } else {
            for (int i = 0; i < n; i++) {
              record = var->get_rec(i+s);
              if (packed) {
                for (int j = 0; j < recSize; j++) {
                  v[i*recSize + j] = record->as_short(j)*scale_factor+add_offset;
                }
              }
              else {
                for (int j = 0; j < recSize; j++) {
                  v[i*recSize + j] = record->as_short(j);
                }
              }
            delete record;
          }
        }
      }
      break;

    case ncInt:
      {
        if (oneSample) {
          record = var->get_rec(s);
          v[0] = record->as_int(0);
          delete record;
        } else {
          for (int i = 0; i < n; i++) {
            record = var->get_rec(i+s);
            NETCDF_DBG qDebug() << "Read record " << i+s << Qt::endl;
            for (int j = 0; j < recSize; j++) {
              v[i*recSize + j] = record->as_int(j);
            }
            delete record;
          }
        }
      }
      break;

    case ncFloat:
      {
        if (oneSample) {
          record = var->get_rec(s);
          v[0] = record->as_float(0);
          delete record;
        } else {
          for (int i = 0; i < n; i++) {
            record = var->get_rec(i+s);
            for (int j = 0; j < recSize; j++) {
              v[i*recSize + j] = record->as_float(j);
            }
            delete record;
          }
        }
      }
      break;

    case ncDouble:
      {
        if (oneSample) {
          record = var->get_rec(s);
          v[0] = record->as_double(0);
          delete record;
        } else {
          for (int i = 0; i < n; i++) {
            record = var->get_rec(i+s);
            for (int j = 0; j < recSize; j++) {
              v[i*recSize + j] = record->as_double(j);
            }
            delete record;
          }
        }
      }
      break;

    default:
      NETCDF_DBG qDebug() << field << ": wrong datatype for kst, no values read" << Qt::endl;
      return -1;
      break;

  }

  NETCDF_DBG qDebug() << "Finished reading " << field << Qt::endl;

  return oneSample ? 1 : n * recSize;
}





int NetcdfSource::readMatrix(double *v, const QString& field) 
{
  /* For a variable from the netCDF file */
  QByteArray bytes = field.toLatin1();
  NcVar *var = _ncfile->get_var(bytes.constData());  // var is owned by _ncfile
  if (!var) {
    NETCDF_DBG qDebug() << "Queried field " << field << " which can't be read" << Qt::endl;
    return -1;
  }

  int xSize = var->get_dim(0)->size();
  int ySize = var->get_dim(1)->size();

  var->get(v, xSize, ySize);

 
  return  xSize * ySize;
}








int NetcdfSource::samplesPerFrame(const QString& field) {
  if (field.toLower() == "index") {
    return 1;
  }
  QByteArray bytes = field.toLatin1();
  NcVar *var = _ncfile->get_var(bytes.constData());
  if (!var) {
    NETCDF_DBG qDebug() << "Queried field " << field << " which can't be read" << Qt::endl;
    return 0;
  }
  return var->rec_size();
}



int NetcdfSource::frameCount(const QString& field) const {
  if (field.isEmpty() || field.toLower() == "index") {
    return _maxFrameCount;
  } else {
    return _frameCounts[field];
  }
}



QString NetcdfSource::fileType() const {
  return "netCDF";
}



bool NetcdfSource::isEmpty() const {
  return frameCount() < 1;
}



QString NetcdfSource::typeString() const
{
  return netCdfTypeString;
}


const QString NetcdfSource::netcdfTypeKey()
{
  return ::netCdfTypeString;
}
