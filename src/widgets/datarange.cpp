/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2007 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "datarange.h"
#include "dialogdefaults.h"

#include <QtMath>
#include <QSignalBlocker>

namespace Kst {

DataRange::DataRange(QWidget *parent)
  : QWidget(parent) {
  setupUi(this);

  connect(_countFromEnd, SIGNAL(toggled(bool)), this, SLOT(countFromEndChanged()));
  connect(_readToEnd, SIGNAL(toggled(bool)), this, SLOT(readToEndChanged()));
  connect(_doSkip, SIGNAL(toggled(bool)), this, SLOT(doSkipChanged()));

  connect(_start, SIGNAL(textEdited(QString)), this, SLOT(startChanged()));
  connect(_range, SIGNAL(textEdited(QString)), this, SLOT(rangeChanged()));
  connect(_last, SIGNAL(textEdited(QString)), this, SLOT(lastChanged()));
  connect(_skip, SIGNAL(valueChanged(int)), this, SIGNAL(modified()));
  connect(_doFilter, SIGNAL(toggled(bool)), this, SIGNAL(modified()));
  connect(_countFromEnd, SIGNAL(toggled(bool)), this, SIGNAL(modified()));
  connect(_readToEnd, SIGNAL(toggled(bool)), this, SIGNAL(modified()));
  connect(_doSkip, SIGNAL(toggled(bool)), this, SIGNAL(modified()));
  connect(_startUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(unitsChanged()));
  connect(_rangeUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(unitsChanged()));

  _controlField0 = Range;
  _controlField1 = Start;
}


DataRange::~DataRange() {
}


void DataRange::clearValues() {
  _start->clear();
  _range->clear();
  _skip->clear();
  _doFilter->setCheckState(Qt::PartiallyChecked);
  _readToEnd->setCheckState(Qt::PartiallyChecked);
  _doSkip->setCheckState(Qt::PartiallyChecked);
}


qreal DataRange::start() const {
  return _start->text().toDouble();
}


bool DataRange::startDirty() const {
  return !_start->text().isEmpty();
}


void DataRange::setStart(qreal start, bool callUpdateFields) {
  _start->setText(QString::number(start, 'g', 12));
  if (callUpdateFields) {
    updateFields(None);
  }
}

qreal DataRange::last() const {
  return _last->text().toDouble();
}


bool DataRange::lastDirty() const {
  return !_last->text().isEmpty();
}


void DataRange::setLast(qreal last, bool callUpdateFields) {
  _last->setText(QString::number(last, 'g', 12));
  if (callUpdateFields) {
    updateFields(None);
  }
}

void DataRange::clearIndexList() {
  _startUnits->clear();
  _rangeUnits->clear();
  _indexFieldProps.clear();
  _dataSource = DataSourcePtr();
}


void DataRange::updateIndexList(const QList<Kst::IndexFieldProperties> &indexFields) {
  _indexFieldProps = indexFields;
  QStringList names;
  for (const Kst::IndexFieldProperties& ifp : indexFields) {
    names.append(ifp.name);
  }

  QSignalBlocker startUnitsBlocker(_startUnits);
  QSignalBlocker rangeUnitsBlocker(_rangeUnits);

  _startUnits->clear();
  _startUnits->addItems(names);
  setStartUnits(_requestedStartUnits);
  _rangeUnits->clear();
  _rangeUnits->addItems(names);
  setRangeUnits(_requestedRangeUnits);

  // These selections were set programmatically while wiring up the dialog.
  // Treat them as the displayed baseline so unitsChanged() does not convert
  // values before the dialog is fully initialized.
  _displayedStartUnits = startUnits();
  _displayedRangeUnits = rangeUnits();
}


QString DataRange::startUnits() const {
  return _startUnits->currentText();
}


// int DataRange::startUnitsIndex() const {
//   return _startUnits->currentIndex();
// }
 

void DataRange::setStartUnits(const QString &startUnits) {
  _requestedStartUnits = startUnits;
  int i = _startUnits->findText(startUnits);
  if (i>=0) {
    _startUnits->setCurrentIndex(i);
  }
}

qreal DataRange::range() const {
  return _range->text().toDouble();
}


// int DataRange::rangeUnitsIndex()  const {
//   return _rangeUnits->currentIndex();
// }


bool DataRange::rangeDirty() const {
  return !_range->text().isEmpty();
}


void DataRange::setRange(qreal range, bool callUpdateFields) {
  _range->setText(QString::number(range));
  if (callUpdateFields) {
    updateFields(None);
  }
}


QString DataRange::rangeUnits() const {
  return _rangeUnits->currentText();
}


void DataRange::setRangeUnits(const QString &rangeUnits) {
  _requestedRangeUnits = rangeUnits;
  int i = _rangeUnits->findText(rangeUnits);
  if (i>=0) {
    _rangeUnits->setCurrentIndex(i);
  } else {
    _rangeUnits->setCurrentIndex(0);
  }
}


// helper implementations ---------------------------------------------------
static const Kst::IndexFieldProperties* findField(const QList<Kst::IndexFieldProperties>& list, const QString &name) {
    for (const Kst::IndexFieldProperties &ifp : list) {
        if (ifp.name == name) return &ifp;
    }
    return nullptr;
}

bool DataRange::startIsFrame() const {
    const QString name = startUnits();
    const Kst::IndexFieldProperties *ifp = findField(_indexFieldProps, name);
    return ifp ? ifp->is_frame : false;
}

bool DataRange::startIsSeconds() const {
    const QString name = startUnits();
    const Kst::IndexFieldProperties *ifp = findField(_indexFieldProps, name);
    return ifp ? ifp->is_seconds : false;
}

bool DataRange::startIsCTime() const {
    const QString name = startUnits();
    const Kst::IndexFieldProperties *ifp = findField(_indexFieldProps, name);
    return ifp ? ifp->is_ctime : false;
}

bool DataRange::rangeIsFrame() const {
    const QString name = rangeUnits();
    const Kst::IndexFieldProperties *ifp = findField(_indexFieldProps, name);
    return ifp ? ifp->is_frame : false;
}

bool DataRange::rangeIsSeconds() const {
    const QString name = rangeUnits();
    const Kst::IndexFieldProperties *ifp = findField(_indexFieldProps, name);
    return ifp ? ifp->is_seconds : false;
}

bool DataRange::rangeIsCTime() const {
    const QString name = rangeUnits();
    const Kst::IndexFieldProperties *ifp = findField(_indexFieldProps, name);
    return ifp ? ifp->is_ctime : false;
}


void DataRange::setDataSource(const DataSourcePtr &dataSource) {
  _dataSource = dataSource;
  updateFields(None);
}


DataSourcePtr DataRange::dataSource() const {
  return _dataSource;
}


bool DataRange::isFrameUnits(const QString &units) {
  const Kst::IndexFieldProperties *ifp = findField(_indexFieldProps, units);
  if (ifp) {
    return ifp->is_frame;
  }
  return units.compare("frames", Qt::CaseInsensitive) == 0;
}


bool DataRange::canConvertUnits(const QString &units) {
  if (isFrameUnits(units)) {
    return true;
  }
  if (!_dataSource) {
    return false;
  }

  _dataSource->readLock();
  bool valid = _dataSource->vector().isValid(units);
  _dataSource->unlock();
  return valid;
}


int DataRange::maxFrameForClamp(const QString &preferredUnits) {
  if (!_dataSource) {
    return -1;
  }

  DataSource *ds = _dataSource.data();
  ds->readLock();

  QStringList candidates;
  if (!preferredUnits.isEmpty() && !isFrameUnits(preferredUnits) && ds->vector().isValid(preferredUnits)) {
    candidates.append(preferredUnits);
  }

  const QString su = startUnits();
  if (!isFrameUnits(su) && !candidates.contains(su) && ds->vector().isValid(su)) {
    candidates.append(su);
  }

  const QString ru = rangeUnits();
  if (!isFrameUnits(ru) && !candidates.contains(ru) && ds->vector().isValid(ru)) {
    candidates.append(ru);
  }

  for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
    if (!ifp.is_frame && !candidates.contains(ifp.name) && ds->vector().isValid(ifp.name)) {
      candidates.append(ifp.name);
      break;
    }
  }

  if (candidates.isEmpty()) {
    const QStringList fields = ds->vector().list();
    if (!fields.isEmpty()) {
      candidates.append(fields.first());
    }
  }

  int maxFrame = -1;
  foreach (const QString &field, candidates) {
    if (ds->vector().isValid(field)) {
      const DataVector::DataInfo info = ds->vector().dataInfo(field);
      if (info.frameCount > 0) {
        maxFrame = int(info.frameCount) - 1;
        break;
      }
    }
  }

  ds->unlock();
  return maxFrame;
}


bool DataRange::frameFromUnits(double value, const QString &units, bool roundUp, int *frameOut) {
  if (!frameOut) {
    return false;
  }

  int frame = 0;
  if (isFrameUnits(units)) {
    frame = roundUp ? qCeil(value) : qFloor(value);
    frame = qMax(0, frame);
    *frameOut = frame;
    return true;
  }

  if (!_dataSource) {
    return false;
  }

  DataSource *ds = _dataSource.data();
  ds->readLock();
  if (!ds->vector().isValid(units)) {
    ds->unlock();
    return false;
  }

  frame = ds->indexToFrame(value, units);
  if (frame < 0) {
    ds->unlock();
    return false;
  }

  const DataVector::DataInfo info = ds->vector().dataInfo(units);
  const int maxFrame = qMax(0, int(info.frameCount) - 1);

  const double minValue = ds->frameToIndex(0, units);
  const double maxValue = ds->frameToIndex(maxFrame, units);
  if (value <= minValue) {
    ds->unlock();
    *frameOut = 0;
    return true;
  }

  if (value > maxValue) {
    const double fps = ds->framePerIndex(units);
    if (fps <= 0.0) {
      ds->unlock();
      return false;
    }

    const double extrapolated = double(maxFrame) + (value - maxValue) * fps;
    ds->unlock();
    *frameOut = qMax(0, roundUp ? int(qCeil(extrapolated)) : int(qFloor(extrapolated)));
    return true;
  }

  double atFrame = ds->frameToIndex(frame, units);

  if (roundUp) {
    if (atFrame < value && frame < maxFrame) {
      ++frame;
    }
  } else {
    if (atFrame > value && frame > 0) {
      --frame;
    }
  }

  frame = qBound(0, frame, maxFrame);
  ds->unlock();
  *frameOut = frame;
  return true;
}


bool DataRange::unitsFromFrame(int frame, const QString &units, double *valueOut) {
  if (!valueOut) {
    return false;
  }

  if (isFrameUnits(units)) {
    *valueOut = frame;
    return true;
  }

  if (!_dataSource) {
    return false;
  }

  DataSource *ds = _dataSource.data();
  ds->readLock();
  if (!ds->vector().isValid(units)) {
    ds->unlock();
    return false;
  }

  const DataVector::DataInfo info = ds->vector().dataInfo(units);
  const int maxFrame = qMax(0, int(info.frameCount) - 1);
  if (frame <= maxFrame) {
    *valueOut = ds->frameToIndex(frame, units);
  } else {
    // Extrapolate past end of file using average frames-per-index
    const double fps = ds->framePerIndex(units);
    const double valueAtMax = ds->frameToIndex(maxFrame, units);
    *valueOut = (fps > 0.0) ? valueAtMax + double(frame - maxFrame) / fps
                            : valueAtMax;
  }
  ds->unlock();
  return true;
}


int DataRange::skip() const {
  return _skip->value();
}


bool DataRange::skipDirty() const {
  return !_skip->text().isEmpty();
}


void DataRange::setSkip(int skip) {
  _skip->setValue(skip);
}


bool DataRange::countFromEnd() const {
  return _countFromEnd->isChecked();
}


void DataRange::setCountFromEnd(bool countFromEnd) {
  _countFromEnd->setChecked(countFromEnd);
  updateFields(None);
}


bool DataRange::countFromEndDirty() const {
  return _readToEnd->checkState() == Qt::PartiallyChecked;
}


bool DataRange::readToEnd() const {
  return _readToEnd->isChecked();
}


bool DataRange::readToEndDirty() const {
  return _readToEnd->checkState() == Qt::PartiallyChecked;
}


void DataRange::setReadToEnd(bool readToEnd) {
  _readToEnd->setChecked(readToEnd);
  updateFields(None);
}


bool DataRange::doSkip() const {
  return _doSkip->isChecked();
}


bool DataRange::doSkipDirty() const {
  return _doSkip->checkState() == Qt::PartiallyChecked;
}


void DataRange::setDoSkip(bool doSkip) {
  _doSkip->setChecked(doSkip);
}


bool DataRange::doFilter() const {
  return _doFilter->isChecked();
}


bool DataRange::doFilterDirty() const {
  return _doFilter->checkState() == Qt::PartiallyChecked;
}


void DataRange::setDoFilter(bool doFilter) {
  _doFilter->setChecked(doFilter);
}


void DataRange::countFromEndChanged() {
  if (countFromEnd()) {
    setReadToEnd(false);
  }

  updateFields(None);

}


void DataRange::readToEndChanged() {
  if (readToEnd()) {
    setCountFromEnd(false);
  }

  updateFields(None);

}


void DataRange::unitsChanged() {
  const bool startUnitsChanged = (sender() == _startUnits);
  const bool rangeUnitsChanged = (sender() == _rangeUnits);
  const QString newStartUnits = startUnits();
  const QString newRangeUnits = rangeUnits();

  if (_displayedStartUnits.isEmpty()) {
    _displayedStartUnits = newStartUnits;
  }
  if (_displayedRangeUnits.isEmpty()) {
    _displayedRangeUnits = newRangeUnits;
  }

  if (startUnitsChanged &&
      (_displayedStartUnits != newStartUnits) &&
      !countFromEnd() &&
      !readToEnd() &&
      !_start->text().isEmpty() &&
      !_last->text().isEmpty() &&
      canConvertUnits(_displayedStartUnits) &&
      canConvertUnits(newStartUnits)) {

    int startFrame = 0;
    int lastFrame = 0;
    if (frameFromUnits(start(), _displayedStartUnits, false, &startFrame) &&
        frameFromUnits(last(), _displayedStartUnits, true, &lastFrame)) {
      if (lastFrame < startFrame) {
        lastFrame = startFrame;
      }

      double translatedStart = 0;
      double translatedLast = 0;
      if (unitsFromFrame(startFrame, newStartUnits, &translatedStart) &&
          unitsFromFrame(lastFrame, newStartUnits, &translatedLast)) {
        _start->setText(QString::number(translatedStart, 'g', 12));
        _last->setText(QString::number(translatedLast, 'g', 12));

        // Keep translated start/last fixed and recompute range in the selected range units.
        _controlField0 = Start;
        _controlField1 = Last;
      }
    }
  }

  if (rangeUnitsChanged &&
      (_displayedRangeUnits != newRangeUnits) &&
      !countFromEnd() &&
      !readToEnd() &&
      !_start->text().isEmpty() &&
      !_last->text().isEmpty() &&
      canConvertUnits(startUnits()) &&
      canConvertUnits(_displayedRangeUnits) &&
      canConvertUnits(newRangeUnits)) {

    int startFrame = 0;
    int lastFrame = 0;
    if (frameFromUnits(start(), startUnits(), false, &startFrame) &&
        frameFromUnits(last(), startUnits(), true, &lastFrame)) {
      if (lastFrame < startFrame) {
        lastFrame = startFrame;
      }

      double startInRangeUnits = 0;
      double endExclusiveInRangeUnits = 0;
      if (unitsFromFrame(startFrame, newRangeUnits, &startInRangeUnits) &&
          unitsFromFrame(lastFrame + 1, newRangeUnits, &endExclusiveInRangeUnits)) {
        const double translatedRange = qMax(0.0, endExclusiveInRangeUnits - startInRangeUnits);
        _range->setText(QString::number(translatedRange, 'g', 12));

        // Keep translated start/last fixed and recompute any dependent fields.
        _controlField0 = Start;
        _controlField1 = Last;
      }
    }
  }

  _displayedStartUnits = newStartUnits;
  _displayedRangeUnits = newRangeUnits;
  updateFields(None);
}


void DataRange::doSkipChanged() {
  _skip->setEnabled(doSkip());
  _doFilter->setEnabled(doSkip());
}

// control field logic:
// the last one changed, other than this one, should be the control field
// do we need a history?
// F0 R -> L
// F0 L -> R
// R L -> F0
// R F0 -> L

void DataRange::startChanged() {
  updateFields(Start);
  emit modified();
}


void DataRange::lastChanged() {
  updateFields(Last);
  emit modified();
}


void DataRange::rangeChanged() {
  updateFields(Range);
  emit modified();
}


void DataRange::updateFields(ControlField cf) {
  const bool sameUnits = (_rangeUnits->currentIndex() == _startUnits->currentIndex());
  const bool mixedUnitsConvertible = canConvertUnits(startUnits()) && canConvertUnits(rangeUnits());
  bool enable_last = !readToEnd() && !countFromEnd() && (sameUnits || mixedUnitsConvertible);


  _last->setEnabled(enable_last);
  _lastLabel->setEnabled(enable_last);

  _start->setEnabled(!countFromEnd());
  _startLabel->setEnabled(!countFromEnd());
  _startUnits->setEnabled(!countFromEnd());
  _range->setEnabled(!readToEnd());
  _rangeLabel->setEnabled(!readToEnd());
  _rangeUnits->setEnabled(!readToEnd());

  if ((cf!=None) && (cf != _controlField1)) {
    _controlField0 = _controlField1;
    _controlField1 = cf;
  }

  // don't do anything if it wouldn't make sense to.
  if (readToEnd() || countFromEnd()) {
    return;
  } 

  if (sameUnits) {
    if ((_controlField0 != Start) && (_controlField1 != Start)) {
      // Determine the minimum valid start for these units (frame 0 in the file)
      double minStart = 0.0;
      if (!isFrameUnits(startUnits())) {
        double minVal = 0.0;
        if (unitsFromFrame(0, startUnits(), &minVal)) {
          minStart = minVal;
        }
      }
      double startVal = last() - range() + 1;
      if (startVal < minStart) {
        // Range pushes start before the beginning of the file.
        // Keep start at the minimum and push last forward so that
        // start, last, and range remain fully consistent.
        startVal = minStart;
        _last->setText(QString::number(minStart + range() - 1, 'g', 12));
      }
      _start->setText(QString::number(startVal, 'g', 12));
    } else if ((_controlField0 != Last) && (_controlField1 != Last)) {
      _last->setText(QString::number(start() + range() - 1, 'g', 12));
    } else if ((_controlField0 != Range) && (_controlField1 != Range)) {
      _range->setText(QString::number(last() - start() + 1, 'g', 12));
    }
    return;
  }

  if (!mixedUnitsConvertible) {
    return;
  }

  const bool solveStart = (_controlField0 != Start) && (_controlField1 != Start);
  const bool solveLast = (_controlField0 != Last) && (_controlField1 != Last);
  const bool solveRange = (_controlField0 != Range) && (_controlField1 != Range);

  if (solveStart) {
    int lastFrame = 0;
    if (!frameFromUnits(last(), startUnits(), true, &lastFrame)) {
      return;
    }

    int startFrame = 0;
    int adjustedEndFrame = -1; // >=0 when start was clamped and last must be updated
    if (isFrameUnits(rangeUnits())) {
      const int spanFrames = qMax(1, int(qRound64(range())));
      const int rawStart = lastFrame - spanFrames + 1;
      startFrame = qMax(0, rawStart);
      if (rawStart < 0) {
        // Start would be before the beginning; push last forward instead.
        adjustedEndFrame = startFrame + spanFrames - 1; // = spanFrames - 1
      }
    } else {
      double lastExclusiveInRangeUnits = 0;
      if (!unitsFromFrame(lastFrame + 1, rangeUnits(), &lastExclusiveInRangeUnits)) {
        return;
      }

      const double startInRangeUnits = lastExclusiveInRangeUnits - range();
      double minRangeUnit = 0.0;
      const bool haveMin = unitsFromFrame(0, rangeUnits(), &minRangeUnit);
      if (haveMin && startInRangeUnits < minRangeUnit) {
        // Start would be before the beginning of the file.
        startFrame = 0;
        int exclusiveEndFrame = 0;
        if (frameFromUnits(minRangeUnit + range(), rangeUnits(), true, &exclusiveEndFrame)) {
          adjustedEndFrame = qMax(startFrame, exclusiveEndFrame - 1);
        }
      } else {
        if (!frameFromUnits(startInRangeUnits, rangeUnits(), false, &startFrame)) {
          return;
        }
      }
    }

    double startValue = 0;
    if (!unitsFromFrame(startFrame, startUnits(), &startValue)) {
      return;
    }
    _start->setText(QString::number(startValue, 'g', 12));

    if (adjustedEndFrame >= 0) {
      // Update last to stay consistent: start is now at the beginning,
      // range is unchanged, so last moves forward (may be past end of file).
      double lastValue = 0;
      if (unitsFromFrame(adjustedEndFrame, startUnits(), &lastValue)) {
        _last->setText(QString::number(lastValue, 'g', 12));
      }
    }
    return;
  }

  if (solveLast) {
    int startFrame = 0;
    if (!frameFromUnits(start(), startUnits(), false, &startFrame)) {
      return;
    }

    int endFrame = startFrame;
    if (isFrameUnits(rangeUnits())) {
      const int spanFrames = qMax(1, int(qRound64(range())));
      // Do NOT clamp endFrame to maxFrame: it is valid and desirable for last
      // to be past the current end of file (file may still be growing).
      endFrame = qMax(0, startFrame + spanFrames - 1);
    } else {
      double startInRangeUnits = 0;
      if (!unitsFromFrame(startFrame, rangeUnits(), &startInRangeUnits)) {
        return;
      }

      int exclusiveEndFrame = 0;
      if (!frameFromUnits(startInRangeUnits + range(), rangeUnits(), true, &exclusiveEndFrame)) {
        return;
      }
      endFrame = qMax(startFrame, exclusiveEndFrame - 1);
    }

    double lastValue = 0;
    if (!unitsFromFrame(endFrame, startUnits(), &lastValue)) {
      return;
    }
    _last->setText(QString::number(lastValue, 'g', 12));
    return;
  }

  if (solveRange) {
    int startFrame = 0;
    int endFrame = 0;
    if (!frameFromUnits(start(), startUnits(), false, &startFrame)) {
      return;
    }
    if (!frameFromUnits(last(), startUnits(), true, &endFrame)) {
      return;
    }

    if (endFrame < startFrame) {
      endFrame = startFrame;
    }

    double rangeValue = 1.0;
    if (isFrameUnits(rangeUnits())) {
      rangeValue = endFrame - startFrame + 1.0;
    } else {
      double startInRangeUnits = 0;
      double endExclusiveInRangeUnits = 0;
      if (!unitsFromFrame(startFrame, rangeUnits(), &startInRangeUnits)) {
        return;
      }
      if (!unitsFromFrame(endFrame + 1, rangeUnits(), &endExclusiveInRangeUnits)) {
        return;
      }
      rangeValue = endExclusiveInRangeUnits - startInRangeUnits;
      if (rangeValue < 0.0) {
        rangeValue = 0.0;
      }
    }
    _range->setText(QString::number(rangeValue, 'g', 12));
  }
}


void DataRange::setWidgetDefaults() {
  dialogDefaults().setValue("vector/range", range());
  dialogDefaults().setValue("vector/start", start());
  dialogDefaults().setValue("vector/countFromEnd", countFromEnd());
  dialogDefaults().setValue("vector/readToEnd", readToEnd());
  dialogDefaults().setValue("vector/skip", skip());
  dialogDefaults().setValue("vector/doSkip", doSkip());
  dialogDefaults().setValue("vector/doAve", doFilter());
  dialogDefaults().setValue("vector/rangeUnits", rangeUnits());
  dialogDefaults().setValue("vector/startUnits", startUnits());
}

void DataRange::loadWidgetDefaults() {
  const qreal defaultRange = dialogDefaults().value("vector/range", 1).toDouble();
  const qreal defaultStart = dialogDefaults().value("vector/start", 0).toDouble();
  bool defaultCountFromEnd = dialogDefaults().value("vector/countFromEnd", false).toBool();
  const bool defaultReadToEnd = dialogDefaults().value("vector/readToEnd", true).toBool();
  const int defaultSkip = dialogDefaults().value("vector/skip", 0).toInt();
  const bool defaultDoSkip = dialogDefaults().value("vector/doSkip", false).toBool();
  const bool defaultDoFilter = dialogDefaults().value("vector/doAve", false).toBool();
  const QString defaultRangeUnits = dialogDefaults().value("vector/rangeUnits", tr("frames")).toString();
  const QString defaultStartUnits = dialogDefaults().value("vector/startUnits", tr("frames")).toString();

  // Preserve previous behavior where readToEnd wins if both flags were saved true.
  if (defaultReadToEnd && defaultCountFromEnd) {
    defaultCountFromEnd = false;
  }

  {
    QSignalBlocker countFromEndBlocker(_countFromEnd);
    QSignalBlocker readToEndBlocker(_readToEnd);
    QSignalBlocker doSkipBlocker(_doSkip);
    QSignalBlocker doFilterBlocker(_doFilter);
    QSignalBlocker skipBlocker(_skip);
    QSignalBlocker startUnitsBlocker(_startUnits);
    QSignalBlocker rangeUnitsBlocker(_rangeUnits);

    setRange(defaultRange, false);
    setStart(defaultStart, false);
    _countFromEnd->setChecked(defaultCountFromEnd);
    _readToEnd->setChecked(defaultReadToEnd);
    setSkip(defaultSkip);
    _doSkip->setChecked(defaultDoSkip);
    _doFilter->setChecked(defaultDoFilter);
    setRangeUnits(defaultRangeUnits);
    setStartUnits(defaultStartUnits);

    // Mark restored units as already displayed so startup index changes do not
    // trigger conversions before the dialog is shown.
    _displayedStartUnits = startUnits();
    _displayedRangeUnits = rangeUnits();
  }

  doSkipChanged();
  updateFields(None);
}

bool DataRange::rangeIsValid() {
  if (readToEnd()) {
    return true;
  } else {
    return (range()>1);
  }
}

}

// vim: ts=2 sw=2 et
