/***************************************************************************
                              primitive.cpp
                             -------------------
    begin                : Tue Jun 20, 2006
    copyright            : Copyright (C) 2006, The University of Toronto
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

//#define UPDATEDEBUG
#include "primitive.h"
#include <QDebug>
#include <QMessageBox>

#include <limits.h>

#include "updatemanager.h"
#include "datasource.h"

namespace Kst {

const QString Primitive::staticTypeString = "Primitive";

Primitive::Primitive(ObjectStore *store, Object *provider)
  : Object(), _provider(provider) {
  Q_UNUSED(store);
  _slaveName = "fixme: set _slaveName";

  _hidden = false;
}


Primitive::~Primitive() {
}


QString Primitive::typeString() const {
  return staticTypeString;
}

void Primitive::setProvider(Object* obj) {
  _provider = obj;
}

void Primitive::setSlaveName(QString slaveName) {
  _slaveName=slaveName;
}

QString Primitive::_automaticDescriptiveName() const {
  QString name;
  if (_provider) {
    name = _provider->descriptiveName() + ':';
  }
  name += _slaveName;

  return name;
}

qint64 Primitive::minInputSerial() const {
  if (_provider) {
    return (_provider->serial());
  }
  return LLONG_MAX;
}

qint64 Primitive::maxInputSerialOfLastChange() const {
  if (_provider) {
    return (_provider->serialOfLastChange());
  }
  return NoInputs;
}


QString Primitive::propertyString() const {
  return QString("Base Class Property String");
}

QString Primitive::sizeString() const {
  return QString("Base Class Size String");
}

bool Primitive::used() const {
  if (_provider) {
    return true;
  } else {
    return Object::used();
  }
}

void Primitive::setUsed(bool used_in) {
  _used = used_in;
  if (_used && provider()) {
    provider()->setUsed(true);
  }
}

void Primitive::fatalError(const QString& msg)
{
  QString message = msg;
  message +="\nError could be ignored, but chances are high that Kst will crash.";
  message += "\nWhen reading ASCII data you could limit the size of the file buffer to save memory.";
  QMessageBox::StandardButton btn = QMessageBox::critical(0, "A fatal error occurred", message, QMessageBox::Abort | QMessageBox::Ignore);
  if (btn == QMessageBox::Abort) {
    exit(-2);
  }
}

bool Primitive::hidden() const
{
  return _hidden;
}

void Primitive::setHidden(bool hidden) {
  _hidden = hidden;
}


}

// vim: et sw=2 ts=2
