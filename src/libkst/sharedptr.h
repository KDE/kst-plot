/* This file is derived from the KDE libraries
   Copyright (c) 1999 Waldo Bastian <bastian@kde.org>
   Copyright (c) 2004 The University of Toronto <netterfield@astro.utoronto.ca>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
*/
#ifndef SharedPTR_H
#define SharedPTR_H

#include <QSemaphore>
#include <QDebug>

// Supress GCC 13's (apparently) over-agressive warnings about use of this class.
// If you are a 22nd level C++ mage (or higher) feel free to remove this, and see
// if there really is a problem here.
#ifdef __GNUC__
  #ifndef __clang__
    #pragma GCC diagnostic ignored "-Wfree-nonheap-object"
  #endif
#endif

//#define KST_DEBUG_SHARED
#ifdef KST_DEBUG_SHARED
#define KST_DBG if (true)
#else
#define KST_DBG if (false)
#endif

// NOTE: In order to preserve binary compatibility with plugins, you must
//       not add, remove, or change member variables or virtual functions.
//       You must also not remove or change non-virtual functions.

// See KSharedPtr in KDE libraries for more information

namespace Kst {

#define SEMAPHORE_COUNT 999999

class Shared {
public:
   /**
    * Standard constructor.  This will initialize the reference count
    * on this object to 0.
    */
   Shared() : sem(SEMAPHORE_COUNT) { }

   /**
    * Copy constructor.  This will @em not actually copy the objects
    * but it will initialize the reference count on this object to 0.
    */
   Shared( const Shared & ) : sem(SEMAPHORE_COUNT) { }

   /**
    * Overloaded assignment operator.
    */
   Shared &operator=(const Shared & ) { return *this; }

   /**
    * Increases the reference count by one.
    */
   void _KShared_ref() const {
     sem.acquire(1);
     KST_DBG qDebug() << "KShared_ref: " << (void*)this << " -> " << _KShared_count() << endl;
   }

   /**
    * Releases a reference (decreases the reference count by one).  If
    * the count goes to 0, this object will delete itself.
    */
   void _KShared_unref() const {
     sem.release(1);
     KST_DBG qDebug() << "KShared_unref: " << (void*)this << " -> " << _KShared_count() << endl;
     if (SEMAPHORE_COUNT == sem.available()) delete this;
   }

   /**
    * Return the current number of references held.
    *
    * @return Number of references
    */
   int _KShared_count() const { return SEMAPHORE_COUNT - sem.available(); }

protected:
   virtual ~Shared() { }

private:
   mutable QSemaphore sem;
};


template< class T >
struct SharedPtr
{
public:
  /**
   * Creates a null pointer.
   */
  SharedPtr() : ptr(0) { isPtrValid(); }

  /**
   * Creates a new pointer.
   * @param t the pointer
   */
  SharedPtr( T* t ) : ptr(t) { if (isPtrValid()) ptr->_KShared_ref(); }

  /**
   * Copies a pointer.
   * @param p the pointer to copy
   */
  SharedPtr( const SharedPtr& p )
    : ptr(p.ptr) { if (isPtrValid()) ptr->_KShared_ref(); }

  template<class Y> SharedPtr(SharedPtr<Y>& p)
    : ptr(p.data()) { if (isPtrValid()) ptr->_KShared_ref(); }

  /**
   * Unreferences the object that this pointer points to. If it was
   * the last reference, the object will be deleted.
   */
  ~SharedPtr() { if (isPtrValid()) ptr->_KShared_unref(); }

  SharedPtr<T>& operator= ( const SharedPtr<T>& p ) {
    isPtrValid();
    if ( ptr == p.ptr ) return *this;
    if (isPtrValid()) ptr->_KShared_unref();
    ptr = p.ptr;
    if (isPtrValid()) ptr->_KShared_ref();
    return *this;
  }

  template<class Y>
  SharedPtr<T>& operator=(SharedPtr<Y>& p) {
    isPtrValid();
    if (ptr == p.data()) return *this;
    if (isPtrValid()) ptr->_KShared_unref();
    ptr = p.data();
    if (isPtrValid()) ptr->_KShared_ref();
    return *this;
  }

  SharedPtr<T>& operator= ( T* p ) {
    isPtrValid();
    if (ptr == p) return *this;
    if (isPtrValid()) ptr->_KShared_unref();
    ptr = p;
    if (isPtrValid()) ptr->_KShared_ref();
    return *this;
  }

  bool operator== ( const SharedPtr<T>& p ) const { isPtrValid(); return ( ptr == p.ptr ); }
  bool operator!= ( const SharedPtr<T>& p ) const { isPtrValid(); return ( ptr != p.ptr ); }
  bool operator== ( const T* p ) const { isPtrValid(); return ( ptr == p ); }
  bool operator!= ( const T* p ) const { isPtrValid(); return ( ptr != p ); }
  bool operator!() const { isPtrValid(); return ( ptr == 0 ); }
  operator T*() const { isPtrValid(); return ptr; }

  /**
   * Returns the pointer.
   * @return the pointer
   */
  T* data() { isPtrValid(); return ptr; }

  /**
   * Returns the pointer.
   * @return the pointer
   */
  const T* data() const { isPtrValid(); return ptr; }

  const T& operator*() const  { Q_ASSERT(isPtrValid()); return *ptr; }
  T& operator*()              { Q_ASSERT(isPtrValid()); return *ptr; }
  const T* operator->() const { Q_ASSERT(isPtrValid()); return ptr; }
  T* operator->()             { Q_ASSERT(isPtrValid()); return ptr; }

  /**
   * Returns the number of references.
   * @return the number of references
   */
  int count() const { Q_ASSERT(isPtrValid()); return ptr->_KShared_count(); } // for debugging purposes


  bool isPtrValid() const { return ptr != 0; }

private:
  T* ptr;
};


template <typename T, typename U>
inline SharedPtr<T> kst_cast(SharedPtr<U> object) {
  return qobject_cast<T*>(object.data());
}

// FIXME: make this safe
template <typename T>
inline SharedPtr<T> kst_cast(QObject *object) {
  return qobject_cast<T*>(object);
}

}
#endif
