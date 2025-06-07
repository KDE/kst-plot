/***************************************************************************
                    string.cpp  -  the base string type
                             -------------------
    begin                : Sept 29, 2004
    copyright            : (C) 2004 by The University of Toronto
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

#include "string_kst.h"
#include "stringscriptinterface.h"

#include <QTextDocument>
#include <QXmlStreamWriter>

namespace Kst {

const QString String::staticTypeString = "String";
const QString String::staticTypeTag = "string";

String::String(ObjectStore *store)
    : Primitive(store, 0L), _orphan(false), _editable(false) {

  _value.clear();
  setFlag(true);
  _initializeShortName();

}


ScriptInterface* String::createScriptInterface() {
  return new StringGenSI(this);
}


void String::_initializeShortName() {
  _shortName = 'T'+QString::number(_stringnum);
  if (_stringnum>max_stringnum)
    max_stringnum = _stringnum;
  _stringnum++;
}


String::~String() {
}


QString String::typeString() const {
  return staticTypeString;
}


void String::save(QXmlStreamWriter &s) {
  if (provider()) { // Don't save datasource- or vector-derived strings
    return;
  }
  s.writeStartElement("string");
  if (_orphan) {
    s.writeAttribute("orphan", "true");
  }
  if (_editable) {
    s.writeAttribute("editable", "true");
  }
  s.writeAttribute("value", value());
  saveNameInfo(s, STRINGNUM);
  s.writeEndElement();
}


void String::internalUpdate() {
  // do nothing
}


String& String::operator=(const QString& v) {
  setValue(v);
  return *this;
}


String& String::operator=(const char *v) {
  setValue(v);
  return *this;
}


void String::setValue(const QString& inV) {
  _value = inV;
}


QString String::_automaticDescriptiveName() const {
  if (_orphan) {
    return value();
  } else {
    return Primitive::_automaticDescriptiveName();
  }
}


QString String::descriptionTip() const {
  return tr("String: %1").arg(Name());
}


QString String::sizeString() const {
  return QString::number(_value.size());
}


QString String::propertyString() const {
  return _value;
}

}

// vim: ts=2 sw=2 et
