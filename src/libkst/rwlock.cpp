/***************************************************************************
                                 rwlock.cpp 
                             -------------------
    begin                : Feb 21, 2004
    copyright            : (C) 2004 The University of toronto
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

#include "rwlock.h"

#include <qdebug.h>

//#define LOCKTRACE

#ifdef ONE_LOCK_TO_RULE_THEM_ALL
QMutex KstRWLock::_mutex(true);
#endif

KstRWLock::KstRWLock()
: _readCount(0), _writeCount(0), _waitingReaders(0), _waitingWriters(0) {
}


KstRWLock::~KstRWLock() {
}


void KstRWLock::readLock() const {
#ifndef ONE_LOCK_TO_RULE_THEM_ALL
  QMutexLocker lock(&_mutex);
  
#ifdef LOCKTRACE
  qDebug() << (void*)this << " KstRWLock::readLock() by tid=" << (int)QThread::currentThreadId() << Qt::endl;
//  qDebug() << kstdBacktrace(6) << Qt::endl;
#endif

  Qt::HANDLE me = QThread::currentThreadId();

  if (_writeCount > 0 && _writeLocker == me) {
    // thread already has a write lock
#ifdef LOCKTRACE
    qDebug() << "Thread " << (int)QThread::currentThreadId() << " has a write lock on KstRWLock " << (void*)this << ", getting a read lock" << Qt::endl;
#endif
  } else {
    QMap<Qt::HANDLE, int>::Iterator it = _readLockers.find(me);
    if (it != _readLockers.end() && it.value() > 0) {
      // thread already has another read lock
    } else {
      while (_writeCount > 0 || _waitingWriters) {  // writer priority otherwise
        ++_waitingReaders;
        _readerWait.wait(&_mutex);
        --_waitingReaders;
      }
    }
  }

  _readLockers[me] = _readLockers[me] + 1;
  ++_readCount;

#ifdef LOCKTRACE
  qDebug() << (void*)this << " KstRWLock::readLock() done by tid=" << (int)QThread::currentThread() << Qt::endl;
#endif
#else
  _mutex.lock();
#endif
}


void KstRWLock::writeLock() const {
#ifndef ONE_LOCK_TO_RULE_THEM_ALL
  QMutexLocker lock(&_mutex);

#ifdef LOCKTRACE
  qDebug() << (void*)this << " KstRWLock::writeLock() by tid=" << (int)QThread::currentThreadId() << Qt::endl;
//  qDebug() << kstdBacktrace(6) << Qt::endl;
#endif

  Qt::HANDLE me = QThread::currentThreadId();

  if (_readCount > 0) {
    QMap<Qt::HANDLE, int>::Iterator it = _readLockers.find(me);
    if (it != _readLockers.end() && it.value() > 0) {
      // cannot acquire a write lock if I already have a read lock -- ERROR
      qDebug() << "Thread " << QThread::currentThread() << " tried to write lock KstRWLock " << (void*)this << " while holding a read lock" << Qt::endl;
      return;
    }
  }

  while (_readCount > 0 || (_writeCount > 0 && _writeLocker != me)) {
    ++_waitingWriters;
    _writerWait.wait(&_mutex);
    --_waitingWriters;
  }
  _writeLocker = me;
  ++_writeCount;

#ifdef LOCKTRACE
  qDebug() << (void*)this << " KstRWLock::writeLock() done by tid=" << (int)QThread::currentThread() << Qt::endl;
#endif
#else
  _mutex.lock();
#endif
}


void KstRWLock::unlock() const {
#ifndef ONE_LOCK_TO_RULE_THEM_ALL
  QMutexLocker lock(&_mutex);

#ifdef LOCKTRACE
  qDebug() << (void*)this << " KstRWLock::unlock() by tid=" << (int)QThread::currentThreadId() << Qt::endl;
#endif

  Qt::HANDLE me = QThread::currentThreadId();

  if (_readCount > 0) {
    QMap<Qt::HANDLE, int>::Iterator it = _readLockers.find(me);
    if (it == _readLockers.end()) {
      // read locked but not by me -- ERROR
      qDebug() << "Thread " << QThread::currentThread() << " tried to unlock KstRWLock " << (void*)this << " (read locked) without holding the lock" << Qt::endl;
      return;
    } else {
      --_readCount;
      if (it.value() == 1) {
        _readLockers.remove(it.key());
      } else {
        --(it.value());
      }
    }
  } else if (_writeCount > 0) {
    if (_writeLocker != me) {
      // write locked but not by me -- ERROR
      qDebug() << "Thread " << QThread::currentThread() << " tried to unlock KstRWLock " << (void*)this << " (write locked) without holding the lock" << Qt::endl;
      return;
    } else {
      --_writeCount;
    }
  } else if (_readCount == 0 && _writeCount == 0) {
    // not locked -- ERROR
    qDebug() << "Thread " << QThread::currentThread() << " tried to unlock KstRWLock " << (void*)this << " (unlocked) without holding the lock" << Qt::endl;
    return;
  }

  if (_readCount == 0 && _writeCount == 0) {
    // no locks remaining
    if (_waitingWriters) {
      _writerWait.wakeOne();
    } else if (_waitingReaders) {
      _readerWait.wakeAll();
    }
  }

#ifdef LOCKTRACE
  qDebug() << (void*)this << " KstRWLock::unlock() done by tid=" << (int)QThread::currentThread() << Qt::endl;
#endif
#else
  _mutex.unlock();
#endif
}


KstRWLock::LockStatus KstRWLock::lockStatus() const {
#ifndef ONE_LOCK_TO_RULE_THEM_ALL
  QMutexLocker lock(&_mutex);

  if (_writeCount > 0) {
    return WRITELOCKED;
  } else if (_readCount > 0) {
    return READLOCKED;
  } else {
    return UNLOCKED;
  }
#else
#error lockStatus() not supported using the single lock
  return UNLOCKED;
#endif
}


KstRWLock::LockStatus KstRWLock::myLockStatus() const {
#ifndef ONE_LOCK_TO_RULE_THEM_ALL
  QMutexLocker lock(&_mutex);

  Qt::HANDLE me = QThread::currentThreadId();

  if (_writeCount > 0 && _writeLocker == me) {
    return WRITELOCKED;
  } else if (_readCount > 0 && _readLockers.find(me) != _readLockers.end()) {
    return READLOCKED;
  } else {
    return UNLOCKED;
  }
#else
#error myLockStatus() not supported using the single lock
  return UNLOCKED;
#endif
}

// vim: ts=2 sw=2 et
