/***************************************************************************
              objectstore.h: store of Objects
                             -------------------
    begin                : November 22, 2006
    copyright            : (C) 2006-2010 The University of Toronto
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

#ifndef OBJECTSTORE_H
#define OBJECTSTORE_H

#include <QDebug>

#include "kstcore_export.h"
#include "object.h"
#include "objectlist.h"
#include "rwlock.h"
#include "datasource.h"

namespace Kst {

class ObjectNameIndex;


// The ObjectStore is responsible for storing all the Objects in an
// application.

class KSTCORE_EXPORT ObjectStore
{
  public:
    ObjectStore();
    ~ObjectStore();

    template<class T> SharedPtr<T> createObject();

    template<class T> bool addObject(T *o);
    bool removeObject(Object *o);

    ObjectPtr retrieveObject(const QString& name, bool enforceUnique = true) const;

    bool isEmpty() const;
    void clear();

    /** get a list containing only objects of type T in the object store
     ** T must inherit from Kst::Object */
    template<class T> const ObjectList<T> getObjects() const;

    const PrimitiveList getFramePrimitives() const;

    /**  get just the data sources */
    DataSourceList dataSourceList() const;

    /** Close all data sources, and reopen ones that are needed */
    void rebuildDataSourceList();

    /** remove unused data sources from the list */
    void cleanUpDataSourceList();

    /** reset dependents of a data source */
    void resetDataSourceDependents(QString filename);

    /** get everything but the data sources */
    QList<ObjectPtr> objectList();

    /** locking */
    KstRWLock& lock() const { return _lock; }

    /** clear the 'used' flag on all objects in list */
    void clearUsedFlags();

    /** delete everything that doesn't have the used flag set.
      * Note: the caller is responsible to make sure that the
      * used flags are set properly.  It is not done here! */
    bool deleteUnsetUsedFlags();

//    void deleteDependentObjects(const Primitive &p);

    // some variables for overriding data source properties
    // from the command line when opening a .kst file
    struct {
      QString fileName;
      int f0;
      int N;
      int skip;
      int doAve;
    } override;

    unsigned sessionVersion; // keep track of older .kst file sessions.
    QString sessionVersionString;
  private:
    Q_DISABLE_COPY(ObjectStore)

    mutable KstRWLock _lock;

    // objects are stored in these lists
    DataSourceList _dataSourceList;
    QList<ObjectPtr> _list;

};


// this is an inefficient implementation for now
template<class T>
const ObjectList<T> ObjectStore::getObjects() const {
  KstReadLocker l(&(this->_lock));
  ObjectList<T> rc;

  for (QList<ObjectPtr>::ConstIterator it = _list.begin(); it != _list.end(); ++it) {
    SharedPtr<T> x = kst_cast<T>(*it);
    if (x != 0) {
      rc.append(x);
    }
  }

  return rc;
}


template<class T>
SharedPtr<T> ObjectStore::createObject() {
  KstWriteLocker l(&(this->_lock));
  T *object = new T(this);
  addObject(object);

  return SharedPtr<T>(object);
}

// Add an object to the store.
template<class T>
bool ObjectStore::addObject(T *o) {
  if (!o) {
    return false;
  }

  KstWriteLocker l(&this->_lock);

  o->_store = this;

  // put the object in the right place depending on its type
  if (DataSourcePtr ds = kst_cast<DataSource>(o)) {
    _dataSourceList.append(ds);
  } else {
    _list.append(o);
  }
  return true;
}


}
#endif

// vim: ts=2 sw=2 et
