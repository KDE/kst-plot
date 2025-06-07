/***************************************************************************
              objectstore.cpp: store of Objects
                             -------------------
    begin                : November 22, 2006
    copyright            : (C) 2006 The University of Toronto
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
#include "objectstore.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QRegularExpression>

#include "object.h"
#include "datavector.h"
#include "datastring.h"
#include "datascalar.h"
#include "datamatrix.h"
#include "vscalar.h"

// NAMEDEBUG: 0 for no debug, 1 for some debug, 2 for more debug, 3 for all debug
#define NAMEDEBUG 0

namespace Kst {

ObjectStore::ObjectStore()
{
  override.fileName.clear();
  override.f0 = override.N = override.skip = override.doAve = -5;
  sessionVersion = 9999999;
}


ObjectStore::~ObjectStore()
{
}


bool ObjectStore::removeObject(Object *o) {
  // clang says, 'this' pointer cannot be null in well-defiened C++ code;
  // pointer may be assumed to always convert to true.
  //if (!this) {
  //  return false;
  //}

  if (!o) {
    return false;
  }

  KstWriteLocker l(&this->_lock);

  DataSourcePtr ds = kst_cast<DataSource>(o);
  if (ds) {
    if (!_dataSourceList.contains(ds)) {
      return false;
    }
  } else if (!_list.contains(o)) {
    return false;
  }

  if (ds) {
    ds->deleteDependents();
    _dataSourceList.removeAll(ds);
  } else {
    o->deleteDependents();
    _list.removeAll(o);
  }

  o->_store = 0;

  return true;
}

ObjectPtr ObjectStore::retrieveObject(const QString& name, bool enforceUnique) const {

  int match = -1;

  if (name.isEmpty()) {
    return NULL;
  }

  QString shortName;
  QRegularExpression rx("(\\(|^)([A-Z]\\d+)(\\)$|$)");
  QRegularExpressionMatch match2;
  name.indexOf(rx, 0, &match2);

  shortName = match2.captured(2);

  // 1) search for short names
  int size = _list.size();
  for (int i = 0; i < size; ++i) {
    if (_list.at(i)->shortName()==shortName)
      return _list.at(i);
  }
  // 3) search for descriptive names: must be unique
  for (int i = 0; i < size; ++i) {
    if (_list.at(i)->descriptiveName() == name) {
      if (enforceUnique && (match != -1)) {
        return NULL; // not unique, so... no match
      }
      match = i;
    }
  }

  if (match >-1) 
    return _list.at(match);

  return NULL;
}

void ObjectStore::resetDataSourceDependents(QString filename) {
  PrimitiveList allPrimitives = getObjects<Primitive>();

  foreach (PrimitivePtr P, allPrimitives) {
    DataPrimitive* dp = qobject_cast<DataPrimitive*>(P);
    if (dp) {
      if (filename == dp->filename()) {
        P->writeLock();
        P->reset();
        P->unlock();
      }
    }
  }
}

void ObjectStore::rebuildDataSourceList() {
  cleanUpDataSourceList();
  foreach (DataSourcePtr ds, _dataSourceList) {
    ds->writeLock();
    ds->reset();
    ds->unlock();
  }
  foreach (ObjectPtr object, _list) {
    object->writeLock();
    object->reset();
    object->unlock();
  }
}

void ObjectStore::cleanUpDataSourceList() {
  DataSourceList currentSourceList;

  currentSourceList.clear();
  currentSourceList.append(_dataSourceList);
  // clean up unused data sources
  for (DataSourceList::Iterator it = currentSourceList.begin(); it != currentSourceList.end(); ++it) {
    if ((*it)->getUsage() <= 1) {
      removeObject(*it);
      //qDebug() << "remove" << (*it)->fileName() << "usage" << (*it)->getUsage();
    } else {
      //qDebug() << "  keep" << (*it)->fileName() << "usage" << (*it)->getUsage();
    }
  }
  currentSourceList.clear();
}

bool ObjectStore::isEmpty() const {
  KstReadLocker l(&_lock);
  return _list.isEmpty();
}


void ObjectStore::clear() {
  KstWriteLocker l(&_lock);
#if NAMEDEBUG > 0
  qDebug () << "Clearing object store " << (void*) this;
#endif
  foreach(DataSource *ds, _dataSourceList) {
    removeObject(ds);
  }
  foreach(Object *o, _list) {
    removeObject(o);
  }

  // Reset the named objects id's.
  NamedObject::resetNameIndex();
}


DataSourceList ObjectStore::dataSourceList() const {
  KstReadLocker l(&_lock);
  return _dataSourceList;
}

QList<ObjectPtr> ObjectStore::objectList() {
  KstReadLocker l(&_lock);
  return _list;
}

void ObjectStore::clearUsedFlags() {
  foreach (ObjectPtr p, _list) {
    p->setUsed(false);
  }
}

bool ObjectStore::deleteUnsetUsedFlags() {
  QList<ObjectPtr> list = _list;
  bool some_deleted = false;
  foreach (ObjectPtr p, list) {
    if (!p->used()) {
      removeObject(p);
      some_deleted = true;
    }
  }
  return some_deleted;
}

const PrimitiveList ObjectStore::getFramePrimitives() const {
  PrimitiveList primitives;
  DataVectorList data_vectors = getObjects<DataVector>();
  VScalarList vscalars = getObjects<VScalar>();
  DataMatrixList data_matrixes = getObjects<DataMatrix>();
  DataStringList data_strings = getObjects<DataString>();


  foreach (DataVectorPtr dv, data_vectors) {
    primitives.append(dv);
  }
  foreach (VScalarPtr vs, vscalars) {
    primitives.append(vs);
  }
  foreach (DataStringPtr ds, data_strings) {
    if (ds->isStream()) {
      primitives.append(ds);
    }
  }
  foreach (DataMatrixPtr dm, data_matrixes) {
    if (dm->isStream()) {
      primitives.append(dm);
    }
  }

  return primitives;
}


}
// vim: ts=2 sw=2 et
