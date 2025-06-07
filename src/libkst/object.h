/***************************************************************************
                 object.h: abstract base class for all Kst objects
                             -------------------
    begin                : May 22, 2003
    copyright            : (C) 2003 The University of Toronto
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

#ifndef OBJECT_H
#define OBJECT_H

#include <QPointer>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QDebug>
#include <QStringList>
#include <QMetaType>
#include <QXmlStreamWriter>

#include "namedobject.h"
#include "kstcore_export.h"
#include "sharedptr.h"
#include "rwlock.h"

namespace Kst {

class ObjectStore;
class Object;
class ScriptInterface;

typedef SharedPtr<Object> ObjectPtr;


class KSTCORE_EXPORT Object : public QObject, public Shared, public KstRWLock, public NamedObject 
{
    Q_OBJECT

  public:
    static QString type();
    static const qint64 Forced = -1;
    static const qint64 NoInputs = -2;

    enum UpdateType { NoChange = 0, Updated, Deferred };

    virtual UpdateType objectUpdate(qint64 newSerial);
    virtual void registerChange() {_serial = Forced; emit dirty();}

    virtual void reset();

    qint64 serial() const {return _serial;}
    qint64 serialOfLastChange() const {return _serialOfLastChange;}

    virtual QString typeString() const;
    static const QString staticTypeString;

    ObjectStore *store() const;

    // Returns count - 2 to account for "this" and the list pointer, therefore
    // you MUST have a reference-counted pointer to call this function
    virtual int getUsage() const;

    virtual void deleteDependents();

    virtual void internalUpdate() = 0;

    virtual bool used() const {return _used;}
    virtual void setUsed(bool used_in) {_used = used_in;}

    virtual bool uses(ObjectPtr p) const;

    virtual ScriptInterface* createScriptInterface();
    ScriptInterface *scriptInterface();

  protected:
    Object();
    virtual ~Object();

    friend class ObjectStore;
    ObjectStore *_store;  // set by ObjectStore

    virtual qint64 minInputSerial() const = 0;
    virtual qint64 maxInputSerialOfLastChange() const = 0;

    qint64 _serial;
    qint64 _serialOfLastChange;
    bool _used;
  private:
    ScriptInterface *_interface;

  signals:
    void dirty();
  };


}

Q_DECLARE_METATYPE(Kst::Object*)

#endif

// vim: ts=2 sw=2 et
