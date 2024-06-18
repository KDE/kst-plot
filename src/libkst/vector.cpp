/***************************************************************************
                          vector.cpp  -  description
                             -------------------
    begin                : Fri Sep 22 2000
    copyright            : (C) 2000-2011 by C. Barth Netterfield
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

#include "vector.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include <QDebug>
#include <QApplication>
#include <QXmlStreamWriter>



#include "datacollection.h"
#include "math_kst.h"
#include "debug.h"
#include "objectstore.h"
#include "updatemanager.h"
#include "vectorscriptinterface.h"

using namespace std;

namespace Kst {

#define INITSIZE 1

const QString Vector::staticTypeString = "Vector";
const QString Vector::staticTypeTag = "vector";

/** Create a vector */
Vector::Vector(ObjectStore *store)
    : Primitive(store, 0L), _nsum(0), _labelInfo(LabelInfo()), _titleInfo(LabelInfo()) {

  _initializeShortName();

  _editable = false;
  _numShifted = 0;
  _numNew = 0;
  _saveData = false;
  _isScalarList = false;

  _saveable = false;

  _labelInfo.name.clear();
  _labelInfo.quantity.clear();
  _labelInfo.units.clear();

  int size = INITSIZE;

  _v_raw = static_cast<double*>(malloc(size * sizeof(double)));
  _v_raw_managed = true;

  if (!_v_raw) { // Malloc failed
    _v_raw = static_cast<double*>(malloc(sizeof(double)));
    _size = 1;
  } else {
    _size = size;
  }
  _v_out = _v_raw;
  _is_rising = false;
  _has_nan = false;
  _v_no_nans_dirty = true;
  _v_no_nans_size = 0;

  _scalars.clear();
  _strings.clear();

  CreateScalars(store);
  setFlag(true);

  blank();

}

void Vector::_initializeShortName() {
  _shortName = 'V'+QString::number(_vectornum);
  if (_vectornum>max_vectornum)
    max_vectornum = _vectornum;
  _vectornum++;
}

Vector::~Vector() {
  if (_v_raw) {
    free(_v_raw);
    _v_raw = 0;
  }
}


ScriptInterface* Vector::createScriptInterface() {
  return new VectorSI(this);
}


void Vector::deleteDependents() {

  for (QHash<QString, ScalarPtr>::Iterator it = _scalars.begin(); it != _scalars.end(); ++it) {
    _store->removeObject(it.value());
  }
  for (QHash<QString, StringPtr>::Iterator it = _strings.begin(); it != _strings.end(); ++it) {
    _store->removeObject(it.value());
  }
  Object::deleteDependents();
}


const QString& Vector::typeString() const {
  return staticTypeString;
}


#define GENERATE_INTERPOLATION              \
  assert(_size > 0);                        \
  /** Limits checks - optional? **/         \
  if (in_i < 0 || _size == 1) {             \
    return _v_out[0];                           \
  }                                         \
                                            \
  if (in_i >= ns_i - 1) {                   \
    return _v_out[_size - 1];                   \
  }                                         \
                                            \
  /** speedup check **/                     \
  if (ns_i == _size) { /* no extrapolating or decimating needed */  \
    return _v_out[in_i];                        \
  }                                         \
                                            \
  double fj = in_i * double(_size - 1) / double(ns_i-1); /* scaled index */ \
                                            \
  int j = int(fj); /* index of sample one lower */ \
  /*assert(j==int(floor(fj)));*/ \
  assert(j+1 < _size && j >= 0);            \
  if (_v_out[j + 1] != _v_out[j + 1] || _v_out[j] != _v_out[j]) { \
    return NOPOINT;                    \
  }                                         \
                                            \
  double fdj = fj - float(j); /* fdj is fraction between _v_out[j] and _v_out[j+1] */ \
                                            \
  return _v_out[j + 1] * fdj + _v_out[j] * (1.0 - fdj);


/** Return v[i], i is sample number, interpolated to have ns_i total
    samples in vector */
double Vector::interpolate(int in_i, int ns_i) const {
  GENERATE_INTERPOLATION
}

/** same as above, but as a function for use in plugins, etc */
double kstInterpolate(double *_v_out, int _size, int in_i, int ns_i) {
  GENERATE_INTERPOLATION
}

#undef GENERATE_INTERPOLATION

#define RETURN_FIRST_NON_HOLE               \
    for (int i = 0; i < _size; ++i) {       \
      if (_v_out[i] == _v_out[i]) {                 \
        return _v_out[i];                       \
      }                                     \
    }                                       \
    return 0.;

#define RETURN_LAST_NON_HOLE                \
    for (int i = _size - 1; i >= 0; --i) {  \
      if (_v_out[i] == _v_out[i]) {                 \
        return _v_out[i];                       \
      }                                     \
    }                                       \
    return 0.;

#define FIND_LEFT(val, idx)                 \
    for (; idx >= 0; --idx) {               \
      if (_v_out[idx] == _v_out[idx]) {             \
        val = _v_out[idx]; break;               \
      }                                     \
    }

#define FIND_RIGHT(val, idx)                \
    for (; idx < _size; ++idx) {            \
      if (_v_out[idx] == _v_out[idx]) {             \
        val = _v_out[idx]; break;               \
      }                                     \
    }

#define GENERATE_INTERPOLATION              \
  assert(_size > 0);                        \
  /** Limits checks - optional? **/         \
  if (in_i <= 0 || _size == 1) {            \
    RETURN_FIRST_NON_HOLE                   \
  }                                         \
                                            \
  if (in_i >= ns_i - 1) {                   \
    RETURN_LAST_NON_HOLE                    \
  }                                         \
                                            \
  /** speedup check **/                     \
  if (ns_i == _size) {                      \
    if (_v_out[in_i] == _v_out[in_i]) {             \
      return _v_out[in_i];                      \
    }                                       \
    double left = 0., right = 0.;           \
    int leftIndex = in_i, rightIndex = in_i;\
    FIND_LEFT(left, leftIndex)              \
    FIND_RIGHT(right, rightIndex)           \
    if (leftIndex == -1) {                  \
      return right;                         \
    }                                       \
    if (rightIndex == _size) {              \
      return left;                          \
    }                                       \
    return left + (right - left) * double(in_i - leftIndex) / double(rightIndex - leftIndex); \
  }                                         \
  abort(); /* FIXME; this is effectivly assert(ns_i == _size) */                      \
  double indexScaleFactor = double(_size - 1) / double(ns_i - 1); \
  double fj = in_i * indexScaleFactor; /* scaled index */ \
                                            \
  int j = int(floor(fj)); /* index of sample one lower */ \
  assert(j+1 < _size && j >= 0);            \
  if (_v_out[j + 1] != _v_out[j + 1] || _v_out[j] != _v_out[j]) { \
    return NOPOINT;                    \
  }                                         \
                                            \
  double fdj = fj - float(j); /* fdj is fraction between _v_out[j] and _v_out[j+1] */ \
                                            \
  return _v_out[j + 1] * fdj + _v_out[j] * (1.0 - fdj);


// FIXME: optimize me - possible that floor() (especially) and isnan() are
//        expensive here.
double Vector::interpolateNoHoles(int in_i, int ns_i) const {
  GENERATE_INTERPOLATION
}


#if 0
double kstInterpolateNoHoles(double *_v, int _size, int in_i, int ns_i) {
  GENERATE_INTERPOLATION
}
#endif


#undef FIND_LEFT
#undef FIND_RIGHT
#undef RETURN_LAST_NON_HOLE
#undef RETURN_FIRST_NON_HOLE
#undef GENERATE_INTERPOLATION

double Vector::value(int i) const {
  if (i < 0 || i >= _size) { // can't look before beginning or past end
    return 0.0;
  }
  return _v_out[i];
}

double Vector::noNanValue(int i) {
  if (i < 0 || i >= _size) { // can't look before beginning or past end
    return 0.0;
  }
  if (_has_nan) {
    if (_v_no_nans_dirty) {
      updateVNoNans();
    }
    return _v_no_nans[i];
  }
  return _v_out[i];
}

void Vector::CreateScalars(ObjectStore *store) {
  if (!_isScalarList) {
    _min = _max = _mean = _minPos = 0.0;
    _imin = _imax = 0;

    Q_ASSERT(store);
    ScalarPtr sp;
    _scalars.insert("max", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("Max");

    _scalars.insert("min", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("Min");

    _scalars.insert("last", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("Last");

    _scalars.insert("first", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("First");

    _scalars.insert("mean", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("Mean");

    _scalars.insert("sigma", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("Sigma");

    _scalars.insert("rms", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("Rms");

    _scalars.insert("ns", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("NS");

    _scalars.insert("sum", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("Sum");

    _scalars.insert("sumsquared", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("SumSquared");

    _scalars.insert("minpos", sp = store->createObject<Scalar>());
    sp->setProvider(this);
    sp->setSlaveName("MinPos");

    if (store->sessionVersion>20000) {
      _scalars.insert("imax", sp = store->createObject<Scalar>());
      sp->setProvider(this);
      sp->setSlaveName("iMax");

      _scalars.insert("imin", sp = store->createObject<Scalar>());
      sp->setProvider(this);
      sp->setSlaveName("iMin");
    } else {
      //qDebug() << "old session version does not support iMin and iMax";
    }
  }
}


void Vector::updateScalars() {
  if (!_isScalarList) {
    _scalars["ns"]->setValue(_size);

    if (_nsum >= 2) {
      double sum = _scalars["sum"]->value();
      double sumsq = _scalars["sumsquared"]->value();
      _scalars["mean"]->setValue(_mean = sum/double(_nsum));
      _scalars["sigma"]->setValue(sqrt((sumsq - sum * sum / double(_nsum)) / double(_nsum-1)));
      _scalars["rms"]->setValue(sqrt(sumsq/double(_nsum)));
    } else {
      _scalars["sigma"]->setValue(_max - _min);
      _scalars["rms"]->setValue(sqrt(_scalars["sumsquared"]->value()));
      _scalars["mean"]->setValue(_mean = NOPOINT);
    }
  }
}


void Vector::setV(double *memptr, int newSize) {
  if (_v_raw_managed) {
    free(_v_raw);
    _v_raw_managed = false;
  }
  _v_raw = memptr;
  _v_out = _v_raw;

  _numNew = newSize;
  _size = newSize;
}

#define FIND_LEFT(val, idx)                 \
    for (; idx >= 0; --idx) {               \
      if (_v_out[idx] == _v_out[idx]) {             \
        val = _v_out[idx]; break;               \
      }                                     \
    }

#define FIND_RIGHT(val, idx)                \
    for (; idx < _size; ++idx) {            \
      if (_v_out[idx] == _v_out[idx]) {             \
        val = _v_out[idx]; break;               \
      }                                     \
    }

void Vector::updateVNoNans()  {

  if (_size != _v_no_nans_size) {
    if (_v_no_nans_size < 1) {
      _v_no_nans = NULL;
    }
    kstrealloc(_v_no_nans, _size*sizeof(double));
    _v_no_nans_size = _size;
  }

  for (int in_i = 0; in_i < _size; in_i++) {
    if (_v_out[in_i] == _v_out[in_i]) {
      _v_no_nans[in_i] = _v_out[in_i];
    } else {
      double left = 0., right = 0.;
      int leftIndex = in_i, rightIndex = in_i;
      FIND_LEFT(left, leftIndex);
      FIND_RIGHT(right, rightIndex);
      if (leftIndex == -1) {
        _v_no_nans[in_i] = right;
      } else if (rightIndex == _size) {
        _v_no_nans[in_i] = left;
      } else {
        _v_no_nans[in_i] = left + (right - left) * double(in_i - leftIndex) / double(rightIndex - leftIndex);
      }
    }
  }
  _v_no_nans_dirty = false;
}
#undef FIND_LEFT
#undef FIND_RIGHT


void Vector::zero() {
  _n_ns_stats = 0;
  _ns_stats_sorted = false;

  memset(_v_raw, 0, sizeof(double)*_size);
  updateScalars();
}


void Vector::blank() {
  _n_ns_stats = 0;
  _ns_stats_sorted = false;

  for (int i = 0; i < _size; ++i) {
    _v_raw[i] = NOPOINT;
  }
  updateScalars();
}


bool Vector::resize(int sz, bool init) {
  if (sz > 0) {
    if (!kstrealloc(_v_raw, sz*sizeof(double))){
       qCritical() << "Vector resize failed";
       return false;
    }
    if (init && _size < sz) {
      for (int i = _size; i < sz; ++i) {
        _v_raw[i] = NOPOINT;
      }
    }
    _size = sz;
    _v_out = _v_raw;
    updateScalars();
  }
  return true;
}


/* used for scripting IPC
    accepts an open writable file.
    returns false on failure */
bool Vector::saveToTmpFile(QFile &fp) {
  qint64 n_written;
  qint64 n_write;

  n_write = length()*sizeof(double);

  n_written = fp.write((char *)_v_raw, n_write);

  fp.flush();

  return (n_write == n_written);
}


double Vector::ns_max(int ns_zoom_level) {
  if (!_ns_stats_sorted) {
    if (_n_ns_stats>4) {
      std::sort(_v_ns_stats, _v_ns_stats+_n_ns_stats);
      _ns_stats_sorted = true;
    }
  }
  if (_n_ns_stats <= 4 ) {
    return max();
  }

  switch (ns_zoom_level) {
  case 0:
    return (_v_ns_stats[_n_ns_stats - 1]);
    break;
  case 1:
    return (_v_ns_stats[_n_ns_stats - _n_ns_stats/333 - 1]);
    break;
  case 2:
    return (_v_ns_stats[_n_ns_stats - _n_ns_stats/100 - 1]);
    break;
  case 3:
    return (_v_ns_stats[_n_ns_stats - _n_ns_stats/33 - 1]);
    break;
  default:
    return (_v_ns_stats[_n_ns_stats - _n_ns_stats/10 - 1]);
    break;
  }
}

double Vector::ns_min(int ns_zoom_level) {
  if (_n_ns_stats>2) {
    std::sort(_v_ns_stats, _v_ns_stats+_n_ns_stats);
    _ns_stats_sorted = true;
  }
  if (_n_ns_stats <= 4 ) {
    return min();
  }
  switch (ns_zoom_level) {
  case 0:
    return (_v_ns_stats[1]);
    break;
  case 1:
    return (_v_ns_stats[_n_ns_stats/333 + 1]);
    break;
  case 2:
    return (_v_ns_stats[_n_ns_stats/100 + 1]);
    break;
  case 3:
    return (_v_ns_stats[_n_ns_stats/33 + 1]);
    break;
  default:
    return (_v_ns_stats[_n_ns_stats/10 + 1]);
    break;
  }
}

void Vector::internalUpdate() {
  int i, i0;
  double sum, sum2, last, first, v;
  double last_v;
  const double epsilon=DBL_MIN; // FIXME: this is not the smallest positive subnormal

  _max = _min = sum = sum2 = _minPos = last = first = NOPOINT;
  _imax = _imin = 0;
  _nsum = 0;

  _has_nan = false;
  _v_no_nans_dirty = true;

  if (_size > 0) {
    _is_rising = true;

    // FIXME: update V_out here
    _v_out = _v_raw;

    // Look for a valid (finite) point...
    for (i = 0; i < _size && !isfinite(_v_out[i]); ++i) {
      // do nothing
    }

    if (i>0) {
      _has_nan = true;
    }

    if (i == _size) { // there were no finite points:
      if (!_isScalarList) {
        _scalars["sum"]->setValue(sum);
        _scalars["sumsquared"]->setValue(sum2);
        _scalars["max"]->setValue(_max);
        _scalars["min"]->setValue(_min);
        _scalars["minpos"]->setValue(_minPos);
        _scalars["last"]->setValue(last);
        _scalars["first"]->setValue(first);
        if (store()->sessionVersion>20000) {
          _scalars["imax"]->setValue(_imax);
          _scalars["imin"]->setValue(_imin);
        }
      }
      _n_ns_stats = 0;
      _ns_stats_sorted = false;

      updateScalars();
      return;
    }

    i0 = i;

    if (i0 > 0) {
      _is_rising = false;
    }

    _max = _min = _v_out[i0];
    _imax = _imin = i0;
    sum = sum2 = 0.0;

    if (_v_out[i0] > epsilon) {
      _minPos = _v_out[i0];
    }

    last_v = _v_out[i0];

    for (i = i0; i < _size; ++i) {
      v = _v_out[i]; // get rid of redirections

      if (isfinite(v)) {
        if (v <= last_v) {
          if (i != i0) {
            _is_rising = false;
          }
        }

        last_v = v;

        _nsum++;
        sum += v;
        sum2 += v*v;

        if (v > _max) {
          _max = v;
          _imax = i;
        } else if (v < _min) {
          _min = v;
          _imin = i;
        }
        if ((isnan(_minPos) || v < _minPos) && v > epsilon) {
          _minPos = v;
        }
      } else {
        _is_rising = false;
        _has_nan = true;
      }
    }

    //no_spike_max_dv = 7.0*sqrt(dv2/double(_nsum));

    last_v = _v_out[i0];

    last = _v_out[_size-1];
    first = _v_out[0];

    /* make vector for spike insensitive autoscale */
    _n_ns_stats = 0;
    _ns_stats_sorted = false;
    double step = qMax(double(_size)/double(MAX_N_DESPIKE_STAT), 1.0);
    for (int k = 0; (k < _size) && (k < MAX_N_DESPIKE_STAT); k++) {
      int m = int(double(k) * step); // FIXME: add random([0, step]) to m
      if (isfinite(_v_out[m])) {
        _v_ns_stats[_n_ns_stats] = _v_out[m];
        _n_ns_stats++;
      }
    }

    if (_isScalarList) {
      _max = _min = _minPos = 0.0;
      _imax =_imin = 0;
    } else {
      _scalars["sum"]->setValue(sum);
      _scalars["sumsquared"]->setValue(sum2);
      _scalars["max"]->setValue(_max);
      _scalars["min"]->setValue(_min);
      _scalars["minpos"]->setValue(_minPos);
      _scalars["last"]->setValue(last);
      _scalars["first"]->setValue(first);
      if (store()->sessionVersion>20000) {
        _scalars["imax"]->setValue(_imax);
        _scalars["imin"]->setValue(_imin);
      }
    }

    updateScalars();

  }
}

void Vector::save(QXmlStreamWriter &s) {
  if (provider()) {
    return;
  }
  s.writeStartElement("vector");
  if (_saveData) {
    QByteArray qba(length()*sizeof(double), '\0');
    QDataStream qds(&qba, QIODevice::WriteOnly);

    for (int i = 0; i < length(); ++i) {
      qds << _v_raw[i];
    }

    s.writeTextElement("data_v2", qCompress(qba).toBase64());
  }
  saveNameInfo(s, VECTORNUM|SCALARNUM);
  s.writeEndElement();
}

void Vector::setNewAndShift(int inNew, int inShift) {
  _numNew = inNew;
  _numShifted = inShift;
}

double const *Vector::noNanValue() {
  if (_has_nan) {
    if (_v_no_nans_dirty) {
      updateVNoNans();
    }
    return _v_no_nans;
  }
  return _v_out;
}


void Vector::newSync() {
  _numNew = _numShifted = 0;
}

int Vector::getUsage() const {
  int adj = 0;
  for (QHash<QString, ScalarPtr>::ConstIterator it = _scalars.begin(); it != _scalars.end(); ++it) {
    adj += it.value()->getUsage() - 1;
  }
  return Object::getUsage() + adj;
}


bool Vector::saveable() const {
  return _saveable;
}


bool Vector::editable() const {
  return _editable;
}


void Vector::setEditable(bool editable) {
  _editable = editable;
}


bool Vector::saveData() const {
  return _saveData;
}


void Vector::setSaveData(bool save) {
  _saveData = save;
}

void Vector::oldChange(QByteArray &data) {
  if (!data.isEmpty()) {
    _saveable = true;
    _saveData = true;

    QDataStream qds(data);

    int sz = qMax(qint64((size_t)(INITSIZE)), qint64(data.size()/sizeof(double)));
    resize(sz, true);

    double sum=0.0;
    for (int i = 0; i<sz; ++i) {
      qds >> _v_raw[i];
      if(!i) {
          _min=_max=_minPos=sum=_v_raw[i];
          _imin = _imax = i;
          _minPos=qMax(_minPos,double(0.0));
      } else {
          _min=qMin(_v_raw[i],_min);
          _max=qMax(_v_raw[i],_max);
          _minPos=qMin(qMax(_v_raw[i],double(0.0)),_minPos);
          sum+=_v_raw[i];
      }
    }
    _mean=sum/double(_size);
  }
  updateScalars();
  internalUpdate();
}

void Vector::change(QByteArray &data) {
  if (!data.isEmpty()) {
    _saveable = true;
    _saveData = true;

    qint64 count;
    QDataStream qds(data);
    qds>>count;

    int sz = qMax(qint64((size_t)(INITSIZE)), count);
    resize(sz, true);

    double sum=0.0;
    for (int i = 0; i<count; ++i) {
      qds >> _v_raw[i];
      if(!i) {
          _min=_max=_minPos=sum=_v_raw[i];
          _minPos=qMax(_minPos,double(0.0));
      } else {
          _min=qMin(_v_raw[i],_min);
          _max=qMax(_v_raw[i],_max);
          _minPos=qMin(qMax(_v_raw[i],double(0.0)),_minPos);
          sum+=_v_raw[i];
      }
    }
    _mean=sum/double(count);
  }
  updateScalars();
  internalUpdate();
}

QString Vector::propertyString() const {
  if(_provider) {
      return tr("Provider: %1").arg(_provider->Name());
  } else {
      return Name();
  }
}

QString Vector::descriptionTip() const {
  return tr("Vector: %1\n  %2 samples\n%3").arg(Name()).arg(length()).arg(_provider->descriptionTip());
}

QString Vector::sizeString() const {
  return QString::number(length());
}

ObjectList<Primitive> Vector::outputPrimitives() const {
  PrimitiveList primitive_list;

  int n = _scalars.count();
  for (int i = 0; i< n; ++i) {
      primitive_list.append(kst_cast<Primitive>(_scalars.values().at(i)));
  }

  n = _strings.count();
  for (int i = 0; i< n; ++i) {
      primitive_list.append(kst_cast<Primitive>(_strings.values().at(i)));
  }

  return primitive_list;
}

PrimitiveMap Vector::metas() const
{
  PrimitiveMap meta;
  for (QHash<QString, StringPtr>::ConstIterator it = _strings.begin(); it != _strings.end(); ++it) {
    meta[it.key()] = it.value();
  }
  for (QHash<QString, ScalarPtr>::ConstIterator it = _scalars.begin(); it != _scalars.end(); ++it) {
    meta[it.key()] = it.value();
  }
  return meta;
}


LabelInfo Vector::labelInfo() const {
  return _labelInfo;
}


LabelInfo Vector::titleInfo() const {
  return _titleInfo;
}


void Vector::setLabelInfo(const LabelInfo &label_info) {
  _labelInfo = label_info;
}


void Vector::setTitleInfo(const LabelInfo &label_info) {
  _titleInfo = label_info;
}

QByteArray Vector::scriptInterface(QList<QByteArray> &c) {
    Q_ASSERT(c.size());
    if(c[0]=="length") {
        return QByteArray::number(_size);
    } else if(c[0]=="interpolate") {
        if(c.size()!=3) {
            return "interpolate takes 2 args";
        }
        return QByteArray::number(interpolate(c[1].toInt(),c[2].toInt()));
    } else if(c[0]=="interpolateNoHoles") {
        if(c.size()!=3) {
            return "interpolateNoHoles takes 2 args";
        }
        return QByteArray::number(interpolateNoHoles(c[1].toInt(),c[2].toInt()));
    } else if(c[0]=="value") {
        if(c.size()!=2) {
            return "value takes 1 arg";
        }
        readLock();
        QByteArray ret = QByteArray::number(value(c[1].toDouble()));
        unlock();
        return ret;
    } else if(c[0]=="min") {
        return QByteArray::number(min());
    } else if(c[0]=="max") {
        return QByteArray::number(max());
    } else if(c[0]=="ns_max") {
        return QByteArray::number(ns_max(2));
    } else if(c[0]=="ns_min") {
        return QByteArray::number(ns_min(2));
    } else if(c[0]=="mean") {
        return QByteArray::number(mean());
    } else if(c[0]=="minPos") {
        return QByteArray::number(minPos());
    } else if(c[0]=="numNew") {
        return QByteArray::number(numNew());
    } else if(c[0]=="numShift") {
        return QByteArray::number(numShift());
    } else if(c[0]=="isRising") {
        return isRising()?"true":"false";
    } else if(c[0]=="newSync") {
        newSync();
        return "Ok";
    } else if(c[0]=="resize") {
        if(c.size()!=3) {
            return "takes 2 args";
        }
        return resize(c[1].toInt(),c[2].toInt())?"true":"false";
    } else if(c[0]=="setNewAndShift") {
        if(c.size()!=3) {
            return "takes 2 args";
        }
        setNewAndShift(c[0].toInt(),c[1].toInt());
        return "Ok";
    } else if(c[0]=="zero") {
        zero();
        return "Ok";
    } else if(c[0]=="blank") {
        blank();
        return "Ok";
    }

    return "No such command...";
}

QByteArray Vector::getBinaryArray() const {
    readLock();
    QByteArray ret;
    QDataStream ds(&ret,QIODevice::WriteOnly);
    ds<<(qint64)_size;
    for(int i=0; i<_size; ++i) {
        ds<<(double)_v_raw[i];
    }
    unlock();
    return ret;
}

#undef INITSIZE

}
// vim: et sw=2 ts=2
