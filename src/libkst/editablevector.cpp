/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2007 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
 *   copyright : (C) 2005  University of British Columbia                  *
 *                   dscott@phas.ubc.ca                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "editablevector.h"

#include "vectorscriptinterface.h"

// use KCodecs::base64Encode() in kmdcodecs.h
// Create QDataStream into a QByteArray
// qCompress the bytearray
#include <QXmlStreamWriter>
#include <QFile>
#include <QDataStream>

#include "debug.h"
namespace Kst {

const QString EditableVector::staticTypeString = "Editable Vector";
const QString EditableVector::staticTypeTag = "editablevector";

EditableVector::EditableVector(ObjectStore *store)
    : Vector(store), _sum(0.0) {
  _editable = true;
  _saveable = true;
  _saveData = true;
}


QString EditableVector::typeString() const {
  return staticTypeString;
}


ScriptInterface* EditableVector::createScriptInterface() {
  return new EditableVectorSI(this);
}

void EditableVector::setSaveData(bool save) {
  Q_UNUSED(save)
}

void EditableVector::setValue(const int &i, const double &val) { //sa Vector::change(...)
    writeLock();
    Q_ASSERT(i>=0);
    if(i>_size) {
        resize(i,1);
    }
    _scalars["sum"]->setValue(_sum+val-_v_out[i]);
    _scalars["sumsquared"]->setValue(_sum*_sum);
    _scalars["max"]->setValue(qMax(_max,val));
    _scalars["min"]->setValue(qMin(_min,val));
    if (val>=0.0) {
      _scalars["minpos"]->setValue(qMin(_minPos,val));
    }
    _scalars["last"]->setValue(_v_out[_size-1]);
    _scalars["first"]->setValue(_v_out[0]);
    _v_raw[i]=val;
    unlock();
}

/** Save vector information */
void EditableVector::save(QXmlStreamWriter &s) {
  s.writeStartElement("editablevector");
  saveNameInfo(s, VECTORNUM|SCALARNUM);

  if (_saveData) {
    QByteArray qba(length()*sizeof(double), '\0');
    QDataStream qds(&qba, QIODevice::WriteOnly);

    for (int i = 0; i < length(); i++) {
      qds << _v_raw[i];
    }

    s.writeTextElement("data", qCompress(qba).toBase64());
  }
  s.writeEndElement();
}

/**  used for scripting IPC.
     accepts an open readable file.
     fails silently */
void EditableVector::loadFromTmpFile(QFile &fp) {
  qint64 n_read;

  resize(fp.size()/sizeof(double));

  n_read = fp.read((char *)_v_raw, fp.size());

  if (n_read != fp.size()) {
    resize(n_read/sizeof(double));
  }
  internalUpdate(); // not sure if we need this here.
}


QString EditableVector::_automaticDescriptiveName() const {

  QString name("(");
  if (length()>=1) {
    name += QString::number(_v_out[0]);
  }
  if (length()>=2) {
    name += " " + QString::number(_v_out[1]);
  }

  if (length()>=3) {
    name += " ...";
  }

  name += ')';

  return name;
}

QString EditableVector::descriptionTip() const {
    return tr("Editable Vector: %1\n"
      "  %2 values").arg(Name()).arg(length());

}

}
// vim: ts=2 sw=2 et
