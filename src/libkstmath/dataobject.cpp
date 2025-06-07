/***************************************************************************
                  dataobject.cpp: base class for data objects
                             -------------------
    begin                : May 20, 2003
    copyright            : (C) 2003 by C. Barth Netterfield
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

#include "dataobject.h"

#include "datacollection.h"
#include "dataobjectplugin.h"
#include "debug.h"

#include "objectstore.h"
#include "relation.h"
#include "sharedptr.h"
#include "primitive.h"
#include "settings.h"

#include "dataobjectscriptinterface.h"

#include <QApplication>
#include <QDir>
#include <qdebug.h>
#include <qtimer.h>
#include <QPluginLoader>
#include <QLibraryInfo>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <limits.h>
#include <assert.h>

//#define LOCKTRACE

using namespace Kst;


QSettings& DataObject::settingsObject()
{
  static QSettings& settingsObject = createSettings("data");
  return settingsObject;
}

QMap<QString,QString> DataObject::url_map;




void DataObject::init() {
  initPlugins();
}


DataObject::DataObject(ObjectStore *store) : Object() {
  Q_UNUSED(store);
  _curveHints = new CurveHintList;
}


DataObject::~DataObject() {
  delete _curveHints;
}


static DataObjectPluginList _pluginList;
void DataObject::cleanupForExit() {
  _pluginList.clear(); //FIXME?
}


void DataObject::attach() {
}

void DataObject::replaceInput(PrimitivePtr p, PrimitivePtr new_p) {
  if (VectorPtr v = kst_cast<Vector>(p) ) {
    if (VectorPtr new_v = kst_cast<Vector>(new_p)) {
      for (VectorMap::Iterator j = _inputVectors.begin(); j != _inputVectors.end(); ++j) {
        if (j.value() == v) {
          _inputVectors[j.key()] = new_v;
        }
      }
    }
  } else if (MatrixPtr m = kst_cast<Matrix>(p) ) {
    if (MatrixPtr new_m = kst_cast<Matrix>(new_p)) {
      for (MatrixMap::Iterator j = _inputMatrices.begin(); j != _inputMatrices.end(); ++j) {
        if (j.value() == m) {
          _inputMatrices[j.key()] = new_m;
        }
      }
    }
  } else if (StringPtr s = kst_cast<String>(p) ) {
    if (StringPtr new_s = kst_cast<String>(new_p)) {
      for (StringMap::Iterator j = _inputStrings.begin(); j != _inputStrings.end(); ++j) {
        if (j.value() == s) {
          _inputStrings[j.key()] = new_s;
        }
      }
    }
  } else if (ScalarPtr s = kst_cast<Scalar>(p) ) {
    if (ScalarPtr new_s = kst_cast<Scalar>(new_p)) {
      for (ScalarMap::Iterator j = _inputScalars.begin(); j != _inputScalars.end(); ++j) {
        if (j.value() == s) {
          _inputScalars[j.key()] = new_s;
        }
      }
    }
  }
}


VectorPtr DataObject::outputVector(const QString& vector) const {
  VectorMap::ConstIterator i = _outputVectors.constFind(vector);
  if (i != _outputVectors.constEnd())
    return *i;
  else
    return 0;
}


ScalarPtr DataObject::outputScalar(const QString& scalar) const {
  ScalarMap::ConstIterator i = _outputScalars.constFind(scalar);
  if (i != _outputScalars.constEnd())
    return *i;
  else
    return 0;
}


StringPtr DataObject::outputString(const QString& string) const {
  StringMap::ConstIterator i = _outputStrings.constFind(string);
  if (i != _outputStrings.constEnd())
    return *i;
  else
    return 0;
}


void DataObject::setInputVector(const QString &type, VectorPtr ptr) {
  if (ptr) {
    _inputVectors[type] = ptr;
  } else {
    _inputVectors.remove(type);
  }
}


void DataObject::setInputScalar(const QString &type, ScalarPtr ptr) {
  if (ptr) {
    _inputScalars[type] = ptr;
  } else {
    _inputScalars.remove(type);
  }
}


void DataObject::setInputString(const QString &type, StringPtr ptr) {
  if (ptr) {
    _inputStrings[type] = ptr;
  } else {
    _inputStrings.remove(type);
  }
}

PrimitiveList DataObject::inputPrimitives() const {
  PrimitiveList primitive_list;

  int n = _inputMatrices.count();
  for (int i = 0; i< n; i++) {
      primitive_list.append(kst_cast<Primitive>(_inputMatrices.values().at(i)));
  }

  n = _inputStrings.count();
  for (int i = 0; i< n; i++) {
      primitive_list.append(kst_cast<Primitive>(_inputStrings.values().at(i)));
  }

  n = _inputScalars.count();
  for (int i = 0; i< n; i++) {
      primitive_list.append(kst_cast<Primitive>(_inputScalars.values().at(i)));
  }

  n = _inputVectors.count();
  for (int i = 0; i< n; i++) {
      primitive_list.append(kst_cast<Primitive>(_inputVectors.values().at(i)));
  }

  return primitive_list;
}


PrimitiveList DataObject::outputPrimitives(bool include_decendants)  const {
  PrimitiveList primitive_list;

  int n = _outputMatrices.count();
  for (int i = 0; i< n; i++) {
      primitive_list.append(kst_cast<Primitive>(_outputMatrices.values().at(i)));
      if (include_decendants) {
          primitive_list.append(_outputMatrices.values().at(i)->outputPrimitives());
      }
  }

  n = _outputStrings.count();
  for (int i = 0; i< n; i++) {
      primitive_list.append(kst_cast<Primitive>(_outputStrings.values().at(i)));
      if (include_decendants) {
          primitive_list.append(_outputStrings.values().at(i)->outputPrimitives());
      }
  }

  n = _outputScalars.count();
  for (int i = 0; i< n; i++) {
      primitive_list.append(kst_cast<Primitive>(_outputScalars.values().at(i)));
      if (include_decendants) {
          primitive_list.append(_outputScalars.values().at(i)->outputPrimitives());
      }
  }

  n = _outputVectors.count();
  for (int i = 0; i< n; i++) {
      primitive_list.append(kst_cast<Primitive>(_outputVectors.values().at(i)));
      if (include_decendants) {
          primitive_list.append(_outputVectors.values().at(i)->outputPrimitives());
      }
  }

  return primitive_list;
}


// set flags on all output primitives
// used for sorting dataobjects by Document::sortedDataObjectList()
void DataObject::setOutputFlags(bool flag) {
  PrimitiveList output_primitives = outputPrimitives();
  int n = output_primitives.count();
  for (int i=0; i<n; i++) {
    output_primitives[i]->setFlag(flag);
  }
}


bool DataObject::inputFlagsSet() const {
  PrimitiveList input_primitives = inputPrimitives();
  int n = input_primitives.count();
  bool all_set = true;
  for (int i=0; i<n; i++) {
    all_set &= input_primitives[i]->flagSet();
  }

  return all_set;
}

// Scans for plugins and stores the information for them
void DataObject::scanPlugins() {
  Debug::self()->log(tr("Scanning for data-object plugins."));

  _pluginList.clear(); //FIXME?

  DataObjectPluginList tmpList;

  Debug::self()->log(tr("Scanning for data-object plugins."));

  foreach (QObject *plugin, QPluginLoader::staticInstances()) {
    //try a cast
    if (DataObjectPluginInterface *basicPlugin = qobject_cast<DataObjectPluginInterface*>(plugin)) {
      tmpList.append(basicPlugin);
    }
  }

  QStringList pluginPaths = pluginSearchPaths();
  foreach (const QString &pluginPath, pluginPaths) {
    QDir d(pluginPath);
    foreach (const QString &fileName, d.entryList(QDir::Files)) {
        QPluginLoader loader(d.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        if (plugin) {
          if (DataObjectPluginInterface *dataObjectPlugin = qobject_cast<DataObjectPluginInterface*>(plugin)) {
            tmpList.append(dataObjectPlugin);
            Debug::self()->log(QString("Plugin loaded: %1").arg(fileName));
          }
        } else {
          Debug::self()->log(QString("Plugin failed to load: %1").arg(fileName));
        }
    }
  }

  // This cleans up plugins that have been uninstalled and adds in new ones.
  // Since it is a shared pointer it can't dangle anywhere.
  _pluginList.clear();
  _pluginList = tmpList;
}


void DataObject::initPlugins() {
  if (_pluginList.isEmpty()) {
      scanPlugins();
  }
}


QStringList DataObject::pluginList() {
  // Ensure state.  When using kstapp MainWindow calls init.
  init();

  QStringList plugins;

  for (DataObjectPluginList::ConstIterator it = _pluginList.constBegin(); it != _pluginList.constEnd(); ++it) {
    plugins += (*it)->pluginName();
  }

  return plugins;
}


QStringList DataObject::dataObjectPluginList() {
  // Ensure state.  When using kstapp MainWindow calls init.
  init();

  QStringList plugins;

  for (DataObjectPluginList::ConstIterator it = _pluginList.constBegin(); it != _pluginList.constEnd(); ++it) {
    if ((*it)->pluginType() == DataObjectPluginInterface::Generic) {
      plugins += (*it)->pluginName();
    }
  }

  plugins.sort();
  return plugins;
}


QStringList DataObject::filterPluginList() {
  // Ensure state.  When using kstapp MainWindow calls init.
  init();

  QStringList plugins;

  for (DataObjectPluginList::ConstIterator it = _pluginList.constBegin(); it != _pluginList.constEnd(); ++it) {
    if ((*it)->pluginType() == DataObjectPluginInterface::Filter) {
      plugins += (*it)->pluginName();
    }
  }

  plugins.sort();
  return plugins;
}


QStringList DataObject::fitsPluginList() {
  // Ensure state.  When using kstapp MainWindow calls init.
  init();

  QStringList plugins;

  for (DataObjectPluginList::ConstIterator it = _pluginList.constBegin(); it != _pluginList.constEnd(); ++it) {
    if ((*it)->pluginType() == DataObjectPluginInterface::Fit) {
      plugins += (*it)->pluginName();
    }
  }

  plugins.sort();
  return plugins;
}


DataObjectConfigWidget* DataObject::pluginWidget(const QString& name) {
  // Ensure state.  When using kstapp MainWindow calls init.
  init();

  for (DataObjectPluginList::ConstIterator it = _pluginList.constBegin(); it != _pluginList.constEnd(); ++it) {
    if ((*it)->pluginName() == name) {
      if ((*it)->hasConfigWidget()) {

        return (*it)->configWidget(&settingsObject());
      }
      break;
    }
  }
  return 0L;
}


QString DataObject::pluginDescription(const QString& name) {
  // Ensure state.  When using kstapp MainWindow calls init.
  init();

  for (DataObjectPluginList::ConstIterator it = _pluginList.constBegin(); it != _pluginList.constEnd(); ++it) {
    if ((*it)->pluginName() == name) {
      return (*it)->pluginDescription();
    }
  }
  return QString();
}


int DataObject::pluginType(const QString& name) {
  // Ensure state.  When using kstapp MainWindow calls init.
  init();

  for (DataObjectPluginList::ConstIterator it = _pluginList.constBegin(); it != _pluginList.constEnd(); ++it) {
    if ((*it)->pluginName() == name) {
      return (*it)->pluginType();
    }
  }
  return -1;
}


DataObjectPtr DataObject::createPlugin(const QString& name, ObjectStore *store, DataObjectConfigWidget *configWidget, bool setupInputsOutputs) {
  // Ensure state.  When using kstapp MainWindow calls init.
  init();

  for (DataObjectPluginList::ConstIterator it = _pluginList.constBegin(); it != _pluginList.constEnd(); ++it) {
    if ((*it)->pluginName() == name) {
      if (DataObjectPtr object = (*it)->create(store, configWidget, setupInputsOutputs)) {
        return object;
      }
    }
  }

#if 0
  KService::List sl = KServiceTypeTrader::self()->query("Kst Data Object");
  for (KService::List::ConstIterator it = sl.constBegin(); it != sl.constEnd(); ++it) {
    if ((*it)->name() != name) {
      continue;
    } else if (DataObjectPtr object = createPlugin(*it)) {
      return object;
    }
  }
#endif
  return 0L;
}


#if 0
double *DataObject::vectorRealloced(VectorPtr v, double *memptr, int newSize) const {
  if (!v) {
    return 0L;
  }

  // One would think this needs special locking, but it results in deadlock
  // in complicated object hierarchies such as filtered vectors.  Therefore if
  // you call vectorRealloced() and v is not locked by you already, you'd
  // better lock it!
  return v->realloced(memptr, newSize);
}
#endif

void DataObject::load(const QXmlStreamReader &e) {
  qDebug() << QString("FIXME! Loading of %1 is not implemented yet.").arg(typeString()) << Qt::endl;
  Q_UNUSED(e)
}


void DataObject::save(QXmlStreamWriter& ts) {
  qDebug() << QString("FIXME! Saving of %1 is not implemented yet.").arg(typeString()) << Qt::endl;
  Q_UNUSED(ts)
}

int DataObject::getUsage() const {
  int rc = 0;

  for (VectorMap::ConstIterator i = _outputVectors.constBegin(); i != _outputVectors.constEnd(); ++i) {
    if (i.value().data()) {
      rc += i.value()->getUsage() - 1;
    }
  }

  for (ScalarMap::ConstIterator i = _outputScalars.constBegin(); i != _outputScalars.constEnd(); ++i) {
    if (i.value().data()) {
      rc += i.value()->getUsage() - 1;
    }
  }

  for (StringMap::ConstIterator i = _outputStrings.constBegin(); i != _outputStrings.constEnd(); ++i) {
    if (i.value().data()) {
      rc += i.value()->getUsage() - 1;
    }
  }

  for (MatrixMap::ConstIterator i = _outputMatrices.constBegin(); i != _outputMatrices.constEnd(); ++i) {
    if (i.value().data()) {
      rc += i.value()->getUsage() - 1;
    }
  }

  return Object::getUsage() + rc;
}


void DataObject::showDialog(bool isNew) {
  if (isNew) {
    QTimer::singleShot(0, this, SLOT(showNewDialog()));
  } else {
    QTimer::singleShot(0, this, SLOT(showEditDialog()));
  }
}


void DataObject::readLock() const {
  #ifdef LOCKTRACE
  qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::readLock() by tid=" << (int)QThread::currentThread() << ": read locking myself" << Qt::endl;
  #endif

  Object::readLock();
}


void DataObject::writeLock() const {
  #ifdef LOCKTRACE
  qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::writeLock() by tid=" << (int)QThread::currentThread() << ": write locking myself" << Qt::endl;
  #endif

  Object::writeLock();
}


void DataObject::unlock() const {
  #ifdef LOCKTRACE
  qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlock() by tid=" << (int)QThread::currentThread() << ": unlocking myself" << Qt::endl;
  #endif

  Object::unlock();
}


void DataObject::writeLockInputsAndOutputs() const {
  Q_ASSERT(myLockStatus() == KstRWLock::WRITELOCKED);

  #ifdef LOCKTRACE
  qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::writeLockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << Qt::endl;
  #endif

  QList<PrimitivePtr> inputs;
  QList<PrimitivePtr> outputs;

  QList<StringPtr> sl = _inputStrings.values();
  for (QList<StringPtr>::Iterator i = sl.begin(); i != sl.end(); ++i) {
    inputs += (*i).data();
  }
  sl = _outputStrings.values();
  for (QList<StringPtr>::Iterator i = sl.begin(); i != sl.end(); ++i) {
    outputs += (*i).data();
  }

  QList<ScalarPtr> sc = _inputScalars.values();
  for (QList<ScalarPtr>::Iterator i = sc.begin(); i != sc.end(); ++i) {
    inputs += (*i).data();
  }
  sc = _outputScalars.values();
  for (QList<ScalarPtr>::Iterator i = sc.begin(); i != sc.end(); ++i) {
    outputs += (*i).data();
  }

  QList<VectorPtr> vl = _inputVectors.values();
  for (QList<VectorPtr>::Iterator i = vl.begin(); i != vl.end(); ++i) {
    inputs += (*i).data();
  }
  vl = _outputVectors.values();
  for (QList<VectorPtr>::Iterator i = vl.begin(); i != vl.end(); ++i) {
    outputs += (*i).data();
  }

  QList<MatrixPtr> ml = _inputMatrices.values();
  for (QList<MatrixPtr>::Iterator i = ml.begin(); i != ml.end(); ++i) {
    inputs += (*i).data();
  }
  ml = _outputMatrices.values();
  for (QList<MatrixPtr>::Iterator i = ml.begin(); i != ml.end(); ++i) {
    outputs += (*i).data();
  }

  std::sort(inputs.begin(), inputs.end());
  std::sort(outputs.begin(), outputs.end());

  QList<PrimitivePtr>::ConstIterator inputIt = inputs.constBegin();
  QList<PrimitivePtr>::ConstIterator outputIt = outputs.constBegin();

  while (inputIt != inputs.constEnd() || outputIt != outputs.constEnd()) {
    if (inputIt != inputs.constEnd() && (outputIt == outputs.constEnd() || (void*)(*inputIt) < (void*)(*outputIt))) {
      // do input
      if (!(*inputIt)) {
        qWarning() << "Input for data object " << this->Name() << " is invalid." << Qt::endl;
      }
#ifdef LOCKTRACE
      qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::writeLockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": write locking input \"" << (*inputIt)->Name() << "\" (" << (void*)((KstRWLock*)*inputIt) << ")" << Qt::endl;
#endif
      (*inputIt)->writeLock();
      ++inputIt;
    } else {
      // do output
      if (!(*outputIt)) {
        qWarning() << "Output for data object " << this->Name() << " is invalid." << Qt::endl;
      }
#ifdef LOCKTRACE
      qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::writeLockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": write locking output \"" << (*outputIt)->Name() << "\" (" << (void*)((KstRWLock*)*outputIt) << ")" << Qt::endl;
#endif
      if ((*outputIt)->provider() != this) {
        Debug::self()->log(tr("(%1) DataObject::writeLockInputsAndOutputs() by tid=%2: write locking output %3 (not provider) -- this is probably an error. Please email kst@kde.org with details.").arg(this->type()).arg(reinterpret_cast<qint64>(QThread::currentThread())).arg((*outputIt)->Name()), Debug::Error);
      }

      (*outputIt)->writeLock();
      ++outputIt;
    }
  }
}


void DataObject::unlockInputsAndOutputs() const {
  #ifdef LOCKTRACE
  qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << Qt::endl;
  #endif

  for (MatrixMap::ConstIterator i = _outputMatrices.constBegin(); i != _outputMatrices.constEnd(); ++i) {
    if (!(*i)) {
      qWarning() << "Output matrix for data object " << this->Name() << " is invalid." << Qt::endl;
    }
    #ifdef LOCKTRACE
    qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": unlocking output matrix \"" << (*i)->Name() << "\" (" << (void*)((KstRWLock*)*i) << ")" << Qt::endl;
    #endif
    (*i)->unlock();
  }

  for (MatrixMap::ConstIterator i = _inputMatrices.constBegin(); i != _inputMatrices.constEnd(); ++i) {
    if (!(*i)) {
      qWarning() << "Input matrix for data object " << this->Name() << " is invalid." << Qt::endl;
    }
    #ifdef LOCKTRACE
    qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": unlocking input matrix \"" << (*i)->Name() << "\" (" << (void*)((KstRWLock*)*i) << ")" << Qt::endl;
    #endif
    (*i)->unlock();
  }

  for (VectorMap::ConstIterator i = _outputVectors.constBegin(); i != _outputVectors.constEnd(); ++i) {
    if (!(*i)) {
      qWarning() << "Output vector for data object " << this->Name() << " is invalid." << Qt::endl;
    }
    #ifdef LOCKTRACE
    qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": unlocking output vector \"" << (*i)->Name() << "\" (" << (void*)((KstRWLock*)*i) << ")" << Qt::endl;
    #endif
    (*i)->unlock();
  }

  for (VectorMap::ConstIterator i = _inputVectors.constBegin(); i != _inputVectors.constEnd(); ++i) {
    if (!(*i)) {
      qWarning() << "Input vector for data object " << this->Name() << " is invalid." << Qt::endl;
    }
    #ifdef LOCKTRACE
    qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": unlocking input vector \"" << (*i)->Name() << "\" (" << (void*)((KstRWLock*)*i) << ")" << Qt::endl;
    #endif
    (*i)->unlock();
  }

  for (ScalarMap::ConstIterator i = _outputScalars.constBegin(); i != _outputScalars.constEnd(); ++i) {
    if (!(*i)) {
      qWarning() << "Output scalar for data object " << this->Name() << " is invalid." << Qt::endl;
    }
    #ifdef LOCKTRACE
    qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": unlocking output scalar \"" << (*i)->Name() << "\" (" << (void*)((KstRWLock*)*i) << ")" << Qt::endl;
    #endif
    (*i)->unlock();
  }

  for (ScalarMap::ConstIterator i = _inputScalars.constBegin(); i != _inputScalars.constEnd(); ++i) {
    if (!(*i)) {
      qWarning() << "Input scalar for data object " << this->Name() << " is invalid." << Qt::endl;
    }
    #ifdef LOCKTRACE
    qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": unlocking input scalar \"" << (*i)->Name() << "\" (" << (void*)((KstRWLock*)*i) << ")" << Qt::endl;
    #endif
    (*i)->unlock();
  }

  for (StringMap::ConstIterator i = _outputStrings.constBegin(); i != _outputStrings.constEnd(); ++i) {
    if (!(*i)) {
      qWarning() << "Output string for data object " << this->Name() << " is invalid." << Qt::endl;
    }
   #ifdef LOCKTRACE
    qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": unlocking output string \"" << (*i)->Name() << "\" (" << (void*)((KstRWLock*)*i) << ")" << Qt::endl;
    #endif
    (*i)->unlock();
  }

  for (StringMap::ConstIterator i = _inputStrings.constBegin(); i != _inputStrings.constEnd(); ++i) {
    if (!(*i)) {
      qWarning() << "Input string for data object " << this->Name() << " is invalid." << Qt::endl;
    }
    #ifdef LOCKTRACE
    qDebug() << (void*)this << " (" << this->type() << ": " << this->Name() << ") DataObject::unlockInputsAndOutputs() by tid=" << (int)QThread::currentThread() << ": unlocking input string \"" << (*i)->Name() << "\" (" << (void*)((KstRWLock*)*i) << ")" << Qt::endl;
    #endif
    (*i)->unlock();
  }
}


bool DataObject::isValid() {
  return true;
}


const CurveHintList* DataObject::curveHints() const {
  return _curveHints;
}


void DataObject::deleteDependents() {
  DataObjectList dataObjects = _store->getObjects<DataObject>();
  foreach (const DataObjectPtr &object, dataObjects) {
    bool usesObject = object->uses(this);
    if (!usesObject) {
      for (VectorMap::Iterator j = _outputVectors.begin(); !usesObject && j != _outputVectors.end(); ++j) {
        usesObject = object->uses(j.value().data());
      }
      for (ScalarMap::Iterator j = _outputScalars.begin(); !usesObject && j != _outputScalars.end(); ++j) {
        usesObject = object->uses(j.value().data());
      }
      for (StringMap::Iterator j = _outputStrings.begin(); !usesObject && j != _outputStrings.end(); ++j) {
        usesObject = object->uses(j.value().data());
      }
      for (MatrixMap::Iterator j = _outputMatrices.begin(); !usesObject && j != _outputMatrices.end(); ++j) {
        usesObject = object->uses(j.value().data());
      }
    }
    if (usesObject) {
      _store->removeObject(object);
    }
  }

  RelationList relations = _store->getObjects<Relation>();
  foreach (const RelationPtr &relation, relations) {
    bool usesRelation = relation->uses(this);
    if (!usesRelation) {
      for (VectorMap::Iterator j = _outputVectors.begin(); !usesRelation && j != _outputVectors.end(); ++j) {
        usesRelation = relation->uses(j.value().data());
      }
      for (ScalarMap::Iterator j = _outputScalars.begin(); !usesRelation && j != _outputScalars.end(); ++j) {
        usesRelation = relation->uses(j.value().data());
      }
      for (StringMap::Iterator j = _outputStrings.begin(); !usesRelation && j != _outputStrings.end(); ++j) {
        usesRelation = relation->uses(j.value().data());
      }
      for (MatrixMap::Iterator j = _outputMatrices.begin(); !usesRelation && j != _outputMatrices.end(); ++j) {
        usesRelation = relation->uses(j.value().data());
      }
    }
    if (usesRelation) {
      _store->removeObject(relation);
    }
  }

  foreach (const VectorPtr &vector, _outputVectors) {
    _store->removeObject(vector);
  }
  foreach (const MatrixPtr &matrix, _outputMatrices) {
    _store->removeObject(matrix);
  }
  foreach (const ScalarPtr &scalar, _outputScalars) {
    _store->removeObject(scalar);
  }
  foreach (const StringPtr &string, _outputStrings) {
    _store->removeObject(string);
  }
}


bool DataObject::uses(ObjectPtr p) const {
  PrimitiveList this_input_primitives;
  PrimitiveList p_output_primitives;

  this_input_primitives = inputPrimitives();

  PrimitivePtr p_prim = kst_cast<Primitive>(p);
  DataObjectPtr p_dobj = kst_cast<DataObject>(p);

  if (p_prim) {
    p_output_primitives = p_prim->outputPrimitives();
    p_output_primitives << p_prim; // include the object itself.
  } else if (p_dobj) {
    p_output_primitives = p_dobj->outputPrimitives();
  }  else {
    p_output_primitives.clear();
  }

  // now check if any of this's input primitives are one of p's output primitives...
  int n_in = this_input_primitives.count();
  for (int i_in = 0; i_in<n_in; i_in++) {
    PrimitivePtr p_in = this_input_primitives.at(i_in);
    if (p_output_primitives.contains(p_in)) {
      return true;
    }
  }
  return false;
}

qint64 DataObject::minInputSerial() const{
  qint64 minSerial = LLONG_MAX;

  foreach (const VectorPtr &P, _inputVectors) {
    minSerial = qMin(minSerial, P->serial());
  }
  foreach (const ScalarPtr &P, _inputScalars) {
    minSerial = qMin(minSerial, P->serial());
  }
  foreach (const MatrixPtr &P, _inputMatrices) {
    minSerial = qMin(minSerial, P->serial());
  }
  foreach (const StringPtr &P, _inputStrings) {
    minSerial = qMin(minSerial, P->serial());
  }
  return minSerial;
}

qint64 DataObject::maxInputSerialOfLastChange() const {
  qint64 maxSerial = NoInputs;

  foreach (const VectorPtr &P, _inputVectors) {
    maxSerial = qMax(maxSerial, P->serialOfLastChange());
  }
  foreach (const ScalarPtr &P, _inputScalars) {
    maxSerial = qMax(maxSerial, P->serialOfLastChange());
  }
  foreach (const MatrixPtr &P, _inputMatrices) {
    maxSerial = qMax(maxSerial, P->serialOfLastChange());
  }
  foreach (const StringPtr &P, _inputStrings) {
    maxSerial = qMax(maxSerial, P->serialOfLastChange());
  }
  return maxSerial;
}


/////////////////////////////////////////////////////////////////////////////
DataObjectConfigWidget::DataObjectConfigWidget(QSettings *cfg)
: QWidget(0L), _cfg(cfg) {
}


DataObjectConfigWidget::~DataObjectConfigWidget() {
}


void DataObjectConfigWidget::save() {
}


void DataObjectConfigWidget::load() {
}

void DataObjectConfigWidget::updateLabels() {
}

void DataObjectConfigWidget::setObjectStore(ObjectStore* store) {
  Q_UNUSED(store);
}


void DataObjectConfigWidget::setupFromObject(Object* dataObject) {
  Q_UNUSED(dataObject);
}


void DataObjectConfigWidget::setVectorX(VectorPtr vector) {
  Q_UNUSED(vector);
}


void DataObjectConfigWidget::setVectorY(VectorPtr vector) {
  Q_UNUSED(vector);
}


void DataObjectConfigWidget::setupSlots(QWidget* dialog) {
  Q_UNUSED(dialog);
}


void DataObjectConfigWidget::setVectorsLocked(bool locked) {
  Q_UNUSED(locked);
}


bool DataObjectConfigWidget::configurePropertiesFromXml(ObjectStore *store, QXmlStreamAttributes& attrs) {
  Q_UNUSED(store);
  Q_UNUSED(attrs);
  return true;
}

QByteArray DataObject::scriptInterface(QList<QByteArray> &c) {
  Q_ASSERT(c.size());
  if(c[0]=="outputVectorHandle") {
    if (c.size()==2) {
      QString c1 = QString(c[1]).trimmed();
      if (_outputVectors.contains(c1)) {
        return _outputVectors[c1]->Name().toLatin1();
      } else {
        return QByteArray("vector not found: ").append(c[1]);
      }
    } else {
      return "outputVectorHandle takes one arg";
    }
  } else if(c[0]=="outputScalarHandle") {
    if (c.size()==2) {
      QString c1 = QString(c[1]).trimmed();
      if (_outputScalars.contains(c1)) {
        return _outputScalars[c1]->Name().toLatin1();
      } else {
        return QByteArray("scalar not found: ").append(c[1]);
      }
    } else {
      return "outputScalarHandle takes one arg";
    }
  } else if(c[0]=="outputMatrixHandle") {
    if (c.size()==2) {
      QString c1 = QString(c[1]).trimmed();
      if (_outputMatrices.contains(c1)) {
        return _outputMatrices[c1]->Name().toLatin1();
      } else {
        return QByteArray("Matrix not found: ").append(c[1]);
      }
    } else {
      return "outputMatrixHandle takes one arg";
    }
  } else if(c[0]=="outputStringHandle") {
    if (c.size()==2) {
      QString c1 = QString(c[1]).trimmed();
      if (_outputStrings.contains(c1)) {
        return _outputStrings[c1]->Name().toLatin1();
      } else {
        return QByteArray("String not found: ").append(c[1]);
      }
    } else {
      return "outputStringHandle takes one arg";
    }
  }

  return "No such command...";
}

// vim: ts=2 sw=2 et
