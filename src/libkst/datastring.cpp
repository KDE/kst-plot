/***************************************************************************
                          dataststring.cpp  -  a string from a data source
                             -------------------
    begin                : September, 2008
    copyright            : (C) 2008 by cbn
    email                : netterfield@astro.utoronto.ca
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "datastring.h"
#include "stringscriptinterface.h"

#include <QDebug>
#include <QTextDocument>
#include <QXmlStreamWriter>

#include "debug.h"
#include "objectstore.h"

namespace Kst {

const QString DataString::staticTypeString = "Data String";
const QString DataString::staticTypeTag = "datastring";

/** Create a DataVector: raw data from a file */
DataString::DataString(ObjectStore *store)
: String(store), DataPrimitive(this) {

  setOrphan(true);
}


DataString::~DataString() {
}

ScriptInterface* DataString::createScriptInterface() {
  return new StringDataSI(this);
}

int DataString::fileLength() const {

  if (dataSource()) {
    const DataInfo info = dataSource()->string().dataInfo(_field);

    return info.frameCount;
  }

  return 0;
}


QString DataString::_automaticDescriptiveName() const {
  QString name = _field;

  // un-escape escaped special characters so they aren't escaped 2x.
  name.replace("\\_", "_").replace("\\^","^").replace("\\[", "[").replace("\\]", "]");
  // now escape the special characters.
  name.replace('_', "\\_").replace('^', "\\^").replace('[', "\\[").replace(']', "\\]");

  return name;
}


QString DataString::typeString() const {
  return staticTypeString;
}


/** return true if it has a valid file and field, or false otherwise */
bool DataString::isValid() const {
  if (dataSource()) {
    dataSource()->readLock();
    bool rc = dataSource()->string().isValid(_field);
    dataSource()->unlock();
    return rc;
  }
  return false;
}


bool DataString::checkValidity(const DataSourcePtr& ds) const {
  if (ds) {
    ds->readLock();
    bool rc = ds->string().isValid(_field);
    ds->unlock();
    return rc;
  }
  return false;
}


void DataString::change(DataSourcePtr in_file, const QString &in_field, int in_frame) {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  _field = in_field;
  _frame = in_frame;
  setDataSource(in_file);

  registerChange();
}

void DataString::changeFile(DataSourcePtr in_file) {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  if (!in_file) {
    Debug::self()->log(tr("Data file for string %1 was not opened.").arg(Name()), Debug::Warning);
  }
  setDataSource(in_file);

  registerChange();
}

bool DataString::isStream()
{
  return dataSource()->isStringStream(_field);
}


/** Save data string information */
void DataString::save(QXmlStreamWriter &s) {
  if (dataSource()) {
    s.writeStartElement("datastring");
    saveFilename(s);
    s.writeAttribute("field", _field);

    saveNameInfo(s, STRINGNUM);
    s.writeEndElement();
  }
}


/** Update a data String */
void DataString::internalUpdate() {  
  if (dataSource()) {
    int frame;
    const DataInfo info = dataSource()->string().dataInfo(_field);
    if ((_frame < 0) || (_frame >= info.frameCount)) {
      frame = info.frameCount-1;
    } else {
      frame = _frame;
    }

    dataSource()->writeLock();
    ReadInfo readInfo(&_value, frame);
    dataSource()->string().read(_field, readInfo);
    dataSource()->unlock();
  }
}

qint64 DataString::minInputSerial() const {
  if (dataSource()) {
    return (dataSource()->serial());
  }
  return LLONG_MAX;
}

qint64 DataString::maxInputSerialOfLastChange() const {
  if (dataSource()) {
    return (dataSource()->serialOfLastChange());
  }
  return NoInputs;
}



PrimitivePtr DataString::makeDuplicate() const {
  Q_ASSERT(store());
  DataStringPtr string = store()->createObject<DataString>();

  string->writeLock();
  string->change(dataSource(), _field, _frame);
  if (descriptiveNameIsManual()) {
    string->setDescriptiveName(descriptiveName());
  }

  string->registerChange();
  string->unlock();

  return kst_cast<Primitive>(string);
}


QString DataString::descriptionTip() const {
  QString IDstring;

  IDstring = tr(
      "Data String: %1 = %4\n"
      "  %2\n"
      "  Field: %3"
  ).arg(Name()).arg(dataSource()->fileName()).arg(_field).arg(value());
  return IDstring;
}


QString DataString::propertyString() const {
  return tr("%1 of %2").arg(_field).arg(dataSource()->fileName());
}

void DataString::reload() {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  if (dataSource()) {
    dataSource()->writeLock();
    dataSource()->reset();
    dataSource()->unlock();
    reset();
    registerChange();
  }
}

void DataString::reset() {
  ReadInfo readInfo(&_value, _frame);
  dataSource()->string().read(_field, readInfo);
}

DataString::DataInfo::DataInfo() :
  frameCount(-1)
{
}

}
// vim: ts=2 sw=2 et
