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
#include "geticon.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTimeZone>
#include <QtMath>

namespace Kst {

DataRange::DataRange(QWidget *parent) : QWidget(parent) {
  setupUi(this);

  _range->setToolTip(tr("Time durations can be specified in the format: [Xw] "
                        "[Xd] [Xh] [Xm] [X[s]]\ne.g. 1h30m or 6w5d4h3m2.5s"));

  connect(_countFromEnd, SIGNAL(toggled(bool)), this,
          SLOT(countFromEndChanged()));
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
  connect(_startUnits, SIGNAL(currentIndexChanged(int)), this,
          SLOT(unitsChanged()));
  connect(_rangeUnits, SIGNAL(currentIndexChanged(int)), this,
          SLOT(unitsChanged()));

  _rangeToDuration->setIcon(KstGetIcon("clock"));
  _rangeToDuration->setToolTip(
      tr("Convert current range to a time duration string (e.g. 1h30m3s)"));
  _rangeToNumber->setIcon(KstGetIcon("draw-number"));
  _rangeToNumber->setToolTip(
      tr("Convert current range to a plain number of frames or seconds"));
  connect(_rangeToNumber, &QPushButton::clicked, this,
          &DataRange::convertRangeToNumber);
  connect(_rangeToDuration, &QPushButton::clicked, this,
          &DataRange::convertRangeToDuration);

  _startToDuration->setIcon(KstGetIcon("clock"));
  _startToDuration->setToolTip(
      tr("Convert current start to a time duration string (e.g. 1h30m3s)"));
  _startToNumber->setIcon(KstGetIcon("draw-number"));
  _startToNumber->setToolTip(
      tr("Convert current start to a plain number of frames or seconds"));
  _startToISOTime->setIcon(KstGetIcon("calendar"));
  _startToISOTime->setToolTip(tr("Convert current start to an ISO time string "
                                 "(e.g. 2024-01-01T12:34:56.789)"));
  connect(_startToNumber, &QPushButton::clicked, this,
          &DataRange::convertStartToNumber);
  connect(_startToDuration, &QPushButton::clicked, this,
          &DataRange::convertStartToDuration);
  connect(_startToISOTime, &QPushButton::clicked, this,
          &DataRange::convertStartToISOTime);

  QFontMetrics fm(_start->font());
  _start->setMinimumWidth(
      fm.horizontalAdvance("0000-00-00T88:88:88.888+00:00000"));
  _last->setMinimumWidth(
      fm.horizontalAdvance("0000-00-00T88:88:88.888+00:00000"));

  _controlField0 = Range;
  _controlField1 = Start;
}

DataRange::~DataRange() {}

// Forward declarations for static helpers defined later in this file.
static double parseTimeDuration(const QString &text);
static double parseISOTime(const QString &text);
static QString secondsToDurationString(double totalSeconds);
static QString ctimeToISOString(double ctime);

void DataRange::clearValues() {
  _start->clear();
  _range->clear();
  _skip->clear();
  _doFilter->setCheckState(Qt::PartiallyChecked);
  _readToEnd->setCheckState(Qt::PartiallyChecked);
  _doSkip->setCheckState(Qt::PartiallyChecked);
}

qreal DataRange::start() const {
  if (startIsISOTime()) {
    return parseISOTime(_start->text());
  }
  if (startIsSeconds()) {
    return parseTimeDuration(_start->text());
  }
  return _start->text().toDouble();
}

bool DataRange::startDirty() const { return !_start->text().isEmpty(); }

void DataRange::setStart(qreal start, bool callUpdateFields) {
  // Match the display format already used by _last so both fields stay in sync.
  if (lastIsISOTime() && startIsCTime()) {
    _start->setText(ctimeToISOString(start));
  } else if (lastIsDuration() && startIsSeconds()) {
    _start->setText(secondsToDurationString(start));
  } else {
    _start->setText(QString::number(start, 'g', 12));
  }
  if (callUpdateFields) {
    updateFields(None);
  }
}

qreal DataRange::last() const {
  if (lastIsISOTime()) {
    return parseISOTime(_last->text());
  }
  if (startIsSeconds()) {
    return parseTimeDuration(_last->text());
  }
  return _last->text().toDouble();
}

bool DataRange::lastDirty() const { return !_last->text().isEmpty(); }

void DataRange::setLast(qreal last, bool callUpdateFields) {
  // Match the display format already used by _start so both fields stay in sync.
  if (startIsISOTime() && startIsCTime()) {
    _last->setText(ctimeToISOString(last));
  } else if (startIsDuration() && startIsSeconds()) {
    _last->setText(secondsToDurationString(last));
  } else {
    _last->setText(QString::number(last, 'g', 12));
  }
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

void DataRange::updateIndexList(
    const QList<Kst::IndexFieldProperties> &indexFields) {
  _indexFieldProps = indexFields;
  QStringList startLabelsList;
  QStringList rangeUnitsList;
  for (const Kst::IndexFieldProperties &ifp : indexFields) {
    startLabelsList.append(ifp.name);
    rangeUnitsList.append(ifp.name);
  }

  QSignalBlocker startUnitsBlocker(_startUnits);
  QSignalBlocker rangeUnitsBlocker(_rangeUnits);

  _startUnits->clear();
  _startUnits->addItems(startLabelsList);
  setStartUnits(_requestedStartUnits);
  _rangeUnits->clear();
  _rangeUnits->addItems(rangeUnitsList);
  setRangeUnits(_requestedRangeUnits);

  // These selections were set programmatically while wiring up the dialog.
  // Treat them as the displayed baseline so unitsChanged() does not convert
  // values before the dialog is fully initialized.
  _displayedStartUnits = startUnits();
  _displayedRangeUnits = rangeUnits();

  updateFields(None);
}

QString DataRange::startUnits() const { return _startUnits->currentText(); }

// int DataRange::startUnitsIndex() const {
//   return _startUnits->currentIndex();
// }

void DataRange::setStartUnits(const QString &startUnits) {
  _requestedStartUnits = startUnits;
  int i = _startUnits->findText(startUnits);
  if (i >= 0) {
    _startUnits->setCurrentIndex(i);
  }
}

// Format a seconds value as a compact KST duration string, e.g. 3661.5 ->
// "1h1m1.5s". Returns "0s" for zero or negative values.
static QString secondsToDurationString(double totalSeconds) {
  if (totalSeconds <= 0.0) {
    return QStringLiteral("0s");
  }
  const qint64 totalSecs = static_cast<qint64>(totalSeconds);
  const double fracSecs = totalSeconds - double(totalSecs);
  const qint64 secsPerWeek = 7LL * 24 * 3600;
  const qint64 secsPerDay = 24LL * 3600;
  const qint64 weeks = totalSecs / secsPerWeek;
  const qint64 rem1 = totalSecs % secsPerWeek;
  const qint64 days = rem1 / secsPerDay;
  const qint64 rem2 = rem1 % secsPerDay;
  const qint64 hours = rem2 / 3600;
  const qint64 rem3 = rem2 % 3600;
  const qint64 minutes = rem3 / 60;
  const qint64 secs = rem3 % 60;
  const double fullSecs = double(secs) + fracSecs;

  QString result;
  if (weeks)
    result += QString::number(weeks) + QLatin1Char('w');
  if (days)
    result += QString::number(days) + QLatin1Char('d');
  if (hours)
    result += QString::number(hours) + QLatin1Char('h');
  if (minutes)
    result += QString::number(minutes) + QLatin1Char('m');
  if (fullSecs != 0.0 || result.isEmpty()) {
    result += QString::number(fullSecs, 'g', 10) + QLatin1Char('s');
  }
  return result;
}

// Returns true if the text is a valid KST duration string (not a plain number).
static bool isDurationString(const QString &text) {
  const QString trimmed = text.trimmed();
  if (trimmed.isEmpty()) {
    return false;
  }
  bool ok = false;
  trimmed.toDouble(&ok);
  if (ok) {
    return false;
  }
  static const QRegularExpression durationRe(
      QStringLiteral(R"(^\s*)"
                     R"((?:[0-9]+(?:\.[0-9]+)?\s*[wW]\s*)?)"
                     R"((?:[0-9]+(?:\.[0-9]+)?\s*[dD]\s*)?)"
                     R"((?:[0-9]+(?:\.[0-9]+)?\s*[hH]\s*)?)"
                     R"((?:[0-9]+(?:\.[0-9]+)?\s*[mM]\s*)?)"
                     R"((?:[0-9]+(?:\.[0-9]+)?\s*[sS]?\s*)?)"
                     R"(\s*$)"));
  static const QRegularExpression hasUnit(QStringLiteral("[wWdDhHmMsS]"));
  return durationRe.match(trimmed).hasMatch() && trimmed.contains(hasUnit);
}

// Returns true if the text is a valid ISO 8601 date-time string.
static bool isISOTimeString(const QString &text) {
  return QDateTime::fromString(text.trimmed(), Qt::ISODateWithMs).isValid();
}

// Parse an ISO 8601 date-time string to a ctime double (seconds since epoch,
// UTC).
static double parseISOTime(const QString &text) {
  QDateTime dt = QDateTime::fromString(text.trimmed(), Qt::ISODateWithMs);
  if (!dt.isValid()) {
    return text.toDouble();
  }
  dt.setTimeZone(QTimeZone::utc());
  return double(dt.toMSecsSinceEpoch()) / 1000.0;
}

// Format a ctime double (seconds since epoch) as an ISO 8601 UTC string.
static QString ctimeToISOString(double ctime) {
  QDateTime dt =
      QDateTime::fromMSecsSinceEpoch(qint64(ctime * 1000.0), QTimeZone::utc());
  return dt.toString(Qt::ISODateWithMs);
}

// Parse an optional convenience syntax for a duration in seconds:
//   [<weeks>w][ ][<days>d][ ][<hours>h][ ][<minutes>m][ ][<seconds>[s]]
// Falls back to plain toDouble() for strings that contain no unit letters.
static double parseTimeDuration(const QString &text) {
  // Fast path: plain number (no unit letters present)
  bool ok = false;
  double plain = text.toDouble(&ok);
  if (ok) {
    return plain;
  }

  // [<w>w][ ][<d>d][ ][<h>h][ ][<m>m][ ][<s>[s]]
  static const QRegularExpression re(QStringLiteral(
      R"(^\s*)"
      R"((?:([0-9]+(?:\.[0-9]+)?)\s*[wW]\s*)?)"  // weeks
      R"((?:([0-9]+(?:\.[0-9]+)?)\s*[dD]\s*)?)"  // days
      R"((?:([0-9]+(?:\.[0-9]+)?)\s*[hH]\s*)?)"  // hours
      R"((?:([0-9]+(?:\.[0-9]+)?)\s*[mM]\s*)?)"  // minutes
      R"((?:([0-9]+(?:\.[0-9]+)?)\s*[sS]?\s*)?)" // seconds (suffix optional)
      R"(\s*$)"));

  QRegularExpressionMatch m = re.match(text.trimmed());
  if (!m.hasMatch()) {
    return text.toDouble(); // fallback
  }

  const double weeks = m.captured(1).toDouble();
  const double days = m.captured(2).toDouble();
  const double hours = m.captured(3).toDouble();
  const double minutes = m.captured(4).toDouble();
  const double seconds = m.captured(5).toDouble();

  return weeks * (7.0 * 24.0 * 3600.0) + days * (24.0 * 3600.0) +
         hours * 3600.0 + minutes * 60.0 + seconds;
}

bool DataRange::rangeIsDuration() const {
  return isDurationString(_range->text());
}

bool DataRange::startIsDuration() const {
  return isDurationString(_start->text());
}

bool DataRange::lastIsDuration() const {
  return isDurationString(_last->text());
}

bool DataRange::startIsISOTime() const {
  return isISOTimeString(_start->text());
}

bool DataRange::lastIsISOTime() const { return isISOTimeString(_last->text()); }

qreal DataRange::range() const {
  if (rangeIsSeconds()) {
    return parseTimeDuration(_range->text());
  }
  return _range->text().toDouble();
}

// int DataRange::rangeUnitsIndex()  const {
//   return _rangeUnits->currentIndex();
// }

bool DataRange::rangeDirty() const { return !_range->text().isEmpty(); }

void DataRange::setRange(qreal range, bool callUpdateFields) {
  _range->setText(QString::number(range));
  if (callUpdateFields) {
    updateFields(None);
  }
}

QString DataRange::rangeUnits() const { return _rangeUnits->currentText(); }

void DataRange::setRangeUnits(const QString &rangeUnits) {
  _requestedRangeUnits = rangeUnits;
  int i = _rangeUnits->findText(rangeUnits);
  if (i >= 0) {
    _rangeUnits->setCurrentIndex(i);
  } else {
    _rangeUnits->setCurrentIndex(0);
  }
}

// helper implementations ---------------------------------------------------
static const Kst::IndexFieldProperties *
findField(const QList<Kst::IndexFieldProperties> &list, const QString &name) {
  for (const Kst::IndexFieldProperties &ifp : list) {
    if (ifp.name == name)
      return &ifp;
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

DataSourcePtr DataRange::dataSource() const { return _dataSource; }

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

// int DataRange::maxFrameForClamp(const QString &preferredUnits) {
//   if (!_dataSource) {
//     return -1;
//   }

//   DataSource *ds = _dataSource.data();
//   ds->readLock();

//   QStringList candidates;
//   if (!preferredUnits.isEmpty() && !isFrameUnits(preferredUnits) &&
//   ds->vector().isValid(preferredUnits)) {
//     candidates.append(preferredUnits);
//   }

//   const QString su = startUnits();
//   if (!isFrameUnits(su) && !candidates.contains(su) &&
//   ds->vector().isValid(su)) {
//     candidates.append(su);
//   }

//   const QString ru = rangeUnits();
//   if (!isFrameUnits(ru) && !candidates.contains(ru) &&
//   ds->vector().isValid(ru)) {
//     candidates.append(ru);
//   }

//   for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
//     if (!ifp.is_frame && !candidates.contains(ifp.name) &&
//     ds->vector().isValid(ifp.name)) {
//       candidates.append(ifp.name);
//       break;
//     }
//   }

//   if (candidates.isEmpty()) {
//     const QStringList fields = ds->vector().list();
//     if (!fields.isEmpty()) {
//       candidates.append(fields.first());
//     }
//   }

//   int maxFrame = -1;
//   foreach (const QString &field, candidates) {
//     if (ds->vector().isValid(field)) {
//       const DataVector::DataInfo info = ds->vector().dataInfo(field);
//       if (info.frameCount > 0) {
//         maxFrame = int(info.frameCount) - 1;
//         break;
//       }
//     }
//   }

//   ds->unlock();
//   return maxFrame;
// }

bool DataRange::frameFromUnits(double value, const QString &units, bool roundUp,
                               int *frameOut) {
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
    *frameOut =
        qMax(0, roundUp ? int(qCeil(extrapolated)) : int(qFloor(extrapolated)));
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

bool DataRange::unitsFromFrame(int frame, const QString &units,
                               double *valueOut) {
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
    *valueOut =
        (fps > 0.0) ? valueAtMax + double(frame - maxFrame) / fps : valueAtMax;
  }
  ds->unlock();
  return true;
}

int DataRange::skip() const { return _skip->value(); }

bool DataRange::skipDirty() const { return !_skip->text().isEmpty(); }

void DataRange::setSkip(int skip) { _skip->setValue(skip); }

bool DataRange::countFromEnd() const { return _countFromEnd->isChecked(); }

void DataRange::setCountFromEnd(bool countFromEnd) {
  _countFromEnd->setChecked(countFromEnd);
  updateFields(None);
}

bool DataRange::countFromEndDirty() const {
  return _readToEnd->checkState() == Qt::PartiallyChecked;
}

bool DataRange::readToEnd() const { return _readToEnd->isChecked(); }

bool DataRange::readToEndDirty() const {
  return _readToEnd->checkState() == Qt::PartiallyChecked;
}

void DataRange::setReadToEnd(bool readToEnd) {
  _readToEnd->setChecked(readToEnd);
  updateFields(None);
}

bool DataRange::doSkip() const { return _doSkip->isChecked(); }

bool DataRange::doSkipDirty() const {
  return _doSkip->checkState() == Qt::PartiallyChecked;
}

void DataRange::setDoSkip(bool doSkip) { _doSkip->setChecked(doSkip); }

bool DataRange::doFilter() const { return _doFilter->isChecked(); }

bool DataRange::doFilterDirty() const {
  return _doFilter->checkState() == Qt::PartiallyChecked;
}

void DataRange::setDoFilter(bool doFilter) { _doFilter->setChecked(doFilter); }

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

  // When start/last hold duration or ISO strings and the start units change,
  // resolve them to plain numbers in the OLD units before the translation block
  // converts those numbers to the new units.
  if (startUnitsChanged && (_displayedStartUnits != newStartUnits)) {
    const Kst::IndexFieldProperties *oldIfp =
        findField(_indexFieldProps, _displayedStartUnits);
    if (oldIfp) {
      if (startIsDuration() && oldIfp->is_seconds) {
        _start->setText(
            QString::number(parseTimeDuration(_start->text()), 'g', 12));
      } else if (startIsISOTime() && oldIfp->is_ctime) {
        _start->setText(QString::number(parseISOTime(_start->text()), 'g', 12));
      }
      if (lastIsDuration() && oldIfp->is_seconds) {
        _last->setText(
            QString::number(parseTimeDuration(_last->text()), 'g', 12));
      } else if (lastIsISOTime() && oldIfp->is_ctime) {
        _last->setText(QString::number(parseISOTime(_last->text()), 'g', 12));
      }
    }
  }

  if (startUnitsChanged && (_displayedStartUnits != newStartUnits) &&
      !countFromEnd() && !readToEnd() && !_start->text().isEmpty() &&
      !_last->text().isEmpty() && canConvertUnits(_displayedStartUnits) &&
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

        // Keep translated start/last fixed and recompute range in the selected
        // range units.
        _controlField0 = Start;
        _controlField1 = Last;
      }
    }
  }

  if (rangeUnitsChanged && (_displayedRangeUnits != newRangeUnits) &&
      !rangeIsDuration() && !countFromEnd() && !readToEnd() &&
      !_start->text().isEmpty() && !_last->text().isEmpty() &&
      canConvertUnits(startUnits()) && canConvertUnits(_displayedRangeUnits) &&
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
          unitsFromFrame(lastFrame + 1, newRangeUnits,
                         &endExclusiveInRangeUnits)) {
        const double translatedRange =
            qMax(0.0, endExclusiveInRangeUnits - startInRangeUnits);
        _range->setText(QString::number(translatedRange, 'g', 12));

        // Keep translated start/last fixed and recompute any dependent fields.
        _controlField0 = Start;
        _controlField1 = Last;
      }
    }
  }

  // When range holds a duration string and the units are switched away from a
  // seconds-compatible field, convert the duration to an equivalent numeric
  // span in the new units (e.g. "1h30m" -> 5400 frames at 1 fps).
  if (rangeUnitsChanged && (_displayedRangeUnits != newRangeUnits) &&
      rangeIsDuration()) {
    const Kst::IndexFieldProperties *oldIfp =
        findField(_indexFieldProps, _displayedRangeUnits);
    if (oldIfp && oldIfp->is_seconds) {
      const double secs = parseTimeDuration(_range->text());
      // Determine span in frames by placing it immediately after frame 0
      // in the old units, then convert that frame count to the new units.
      double originInOldUnits = 0.0;
      unitsFromFrame(0, _displayedRangeUnits, &originInOldUnits);
      int endFrame = 0;
      if (frameFromUnits(originInOldUnits + secs, _displayedRangeUnits, false,
                         &endFrame)) {
        double originInNewUnits = 0.0;
        double endInNewUnits = 0.0;
        if (unitsFromFrame(0, newRangeUnits, &originInNewUnits) &&
            unitsFromFrame(endFrame, newRangeUnits, &endInNewUnits)) {
          _range->setText(QString::number(
              qMax(0.0, endInNewUnits - originInNewUnits), 'g', 12));
        }
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
  // If the user typed a duration string and the current start units are not
  // seconds-compatible, auto-switch to the first seconds-compatible unit.
  if (startIsDuration() && !startIsSeconds()) {
    for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
      if (ifp.is_seconds) {
        setStartUnits(ifp.name);
        break;
      }
    }
  }
  // If the user typed a valid ISO time (not a plain number), auto-switch to
  // the first ctime-compatible unit.
  {
    bool ok = false;
    _start->text().toDouble(&ok);
    if (!ok && startIsISOTime() && !startIsCTime()) {
      for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
        if (ifp.is_ctime) {
          QSignalBlocker blocker(_startUnits);
          _startUnits->setCurrentIndex(_startUnits->findText(ifp.name));
          _displayedStartUnits = ifp.name;
          break;
        }
      }
    }
  }
  updateFields(Start);
  // Keep _last's format in sync with _start.
  if (!_last->text().isEmpty()) {
    if (startIsDuration() && !lastIsDuration() && startIsSeconds()) {
      _last->setText(secondsToDurationString(last()));
    } else if (startIsISOTime() && !lastIsISOTime() && startIsCTime()) {
      _last->setText(ctimeToISOString(last()));
    } else if (!startIsDuration() && !startIsISOTime() &&
               (lastIsDuration() || lastIsISOTime())) {
      _last->setText(QString::number(last(), 'g', 12));
    }
  }
  emit modified();
}

void DataRange::lastChanged() {
  // If the user typed a duration string and the current start units are not
  // seconds-compatible, auto-switch to the first seconds-compatible unit.
  if (lastIsDuration() && !startIsSeconds()) {
    for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
      if (ifp.is_seconds) {
        setStartUnits(ifp.name);
        break;
      }
    }
  }
  // If the user typed a valid ISO time (not a plain number), auto-switch to
  // the first ctime-compatible unit.
  {
    bool ok = false;
    _last->text().toDouble(&ok);
    if (!ok && lastIsISOTime() && !startIsCTime()) {
      for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
        if (ifp.is_ctime) {
          QSignalBlocker blocker(_startUnits);
          _startUnits->setCurrentIndex(_startUnits->findText(ifp.name));
          _displayedStartUnits = ifp.name;
          break;
        }
      }
    }
  }
  updateFields(Last);
  // Keep _start's format in sync with _last.
  if (!_start->text().isEmpty()) {
    if (lastIsDuration() && !startIsDuration() && startIsSeconds()) {
      _start->setText(secondsToDurationString(start()));
    } else if (lastIsISOTime() && !startIsISOTime() && startIsCTime()) {
      _start->setText(ctimeToISOString(start()));
    } else if (!lastIsDuration() && !lastIsISOTime() &&
               (startIsDuration() || startIsISOTime())) {
      _start->setText(QString::number(start(), 'g', 12));
    }
  }
  emit modified();
}

void DataRange::rangeChanged() {
  // If the user typed a duration string and the current range units are not
  // seconds-compatible, auto-switch to the first seconds-compatible unit.
  if (rangeIsDuration() && !rangeIsSeconds()) {
    for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
      if (ifp.is_seconds) {
        setRangeUnits(ifp.name);
        break;
      }
    }
  }
  updateFields(Range);
  emit modified();
}

void DataRange::convertRangeToNumber() {
  if (!rangeIsDuration()) {
    return;
  }
  const double seconds = parseTimeDuration(_range->text());
  _range->setText(QString::number(seconds, 'g', 12));
  updateFields(Range);
  emit modified();
}

void DataRange::convertRangeToDuration() {
  if (rangeIsDuration() || _range->text().isEmpty()) {
    return;
  }
  // Mirror the auto-switch that happens when typing a duration string:
  // if not already on a seconds-compatible unit, switch to the first one.
  // setRangeUnits() triggers unitsChanged() which translates the range value
  // from the old units (e.g. frames) to seconds when start/last are available.
  if (!rangeIsSeconds()) {
    for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
      if (ifp.is_seconds) {
        setRangeUnits(ifp.name);
        break;
      }
    }
  }
  const double seconds = _range->text().toDouble();
  _range->setText(secondsToDurationString(seconds));
  updateFields(Range);
  emit modified();
}

void DataRange::convertStartToNumber() {
  bool changed = false;
  if (startIsDuration()) {
    _start->setText(
        QString::number(parseTimeDuration(_start->text()), 'g', 12));
    changed = true;
  } else if (startIsISOTime()) {
    _start->setText(QString::number(parseISOTime(_start->text()), 'g', 12));
    changed = true;
  }
  if (lastIsDuration()) {
    _last->setText(QString::number(parseTimeDuration(_last->text()), 'g', 12));
    changed = true;
  } else if (lastIsISOTime()) {
    _last->setText(QString::number(parseISOTime(_last->text()), 'g', 12));
    changed = true;
  }
  if (changed) {
    updateFields(Start);
    emit modified();
  }
}

void DataRange::convertStartToDuration() {
  if (_start->text().isEmpty()) {
    return;
  }
  // If not already on a seconds-compatible unit, switch to the first one.
  if (!startIsSeconds()) {
    for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
      if (ifp.is_seconds) {
        setStartUnits(ifp.name);
        break;
      }
    }
  }
  if (!startIsDuration()) {
    _start->setText(secondsToDurationString(_start->text().toDouble()));
  }
  if (!lastIsDuration() && !_last->text().isEmpty()) {
    _last->setText(secondsToDurationString(_last->text().toDouble()));
  }
  updateFields(Start);
  emit modified();
}

void DataRange::convertStartToISOTime() {
  if (_start->text().isEmpty()) {
    return;
  }
  // Save range text — this is a format-only conversion and must not alter
  // range.
  const QString savedRange = _range->text();
  // If not already on a ctime-compatible unit, switch to the first one.
  if (!startIsCTime()) {
    for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
      if (ifp.is_ctime) {
        setStartUnits(ifp.name);
        break;
      }
    }
  }
  if (!startIsISOTime()) {
    _start->setText(ctimeToISOString(start()));
  }
  if (!lastIsISOTime() && !_last->text().isEmpty()) {
    _last->setText(ctimeToISOString(last()));
  }
  _range->setText(savedRange);
  updateFields(None);
  _range->setText(savedRange);
  emit modified();
}

void DataRange::updateFields(ControlField cf) {
  // Capture format state before any text changes so solved fields can
  // preserve the display format of start/last and range.
  const bool startWasDuration = startIsDuration();
  const bool startWasISO = startIsISOTime();
  const bool lastWasDuration = lastIsDuration();
  const bool lastWasISO = lastIsISOTime();
  const bool rangeWasDuration = rangeIsDuration();

  const bool sameUnits =
      (_rangeUnits->currentIndex() == _startUnits->currentIndex());
  const bool mixedUnitsConvertible =
      canConvertUnits(startUnits()) && canConvertUnits(rangeUnits());
  bool enable_last =
      !readToEnd() && !countFromEnd() && (sameUnits || mixedUnitsConvertible);

  _last->setEnabled(enable_last);
  _lastLabel->setEnabled(enable_last);

  _start->setEnabled(!countFromEnd());
  _startLabel->setEnabled(!countFromEnd());
  _startUnits->setEnabled(!countFromEnd());
  _range->setEnabled(!readToEnd());
  _rangeLabel->setEnabled(!readToEnd());
  _rangeUnits->setEnabled(!readToEnd());

  {
    bool hasSeconds = false;
    bool hasCTime = false;
    for (const Kst::IndexFieldProperties &ifp : _indexFieldProps) {
      if (ifp.is_seconds) {
        hasSeconds = true;
      }
      if (ifp.is_ctime) {
        hasCTime = true;
      }
    }
    _rangeToDuration->setEnabled(!readToEnd() && !rangeIsDuration() &&
                                 !_range->text().isEmpty() && hasSeconds);
    _rangeToNumber->setEnabled(!readToEnd() && rangeIsDuration());

    const bool startHasText = !_start->text().isEmpty();
    const bool startIsSpecial = startIsDuration() || startIsISOTime();
    _startToDuration->setEnabled(!countFromEnd() && startHasText &&
                                 !startIsDuration() && hasSeconds);
    _startToNumber->setEnabled(!countFromEnd() && startIsSpecial);
    _startToISOTime->setEnabled(!countFromEnd() && startHasText &&
                                !startIsISOTime() && hasCTime);
  }

  if ((cf != None) && (cf != _controlField1)) {
    _controlField0 = _controlField1;
    _controlField1 = cf;
  }

  // Determine format for solved start/last values.
  // The edited field (cf) dictates the format; when cf is Range or None the
  // existing start/last format is preserved.
  const bool isSec = startIsSeconds();
  const bool isCT = startIsCTime();
  const bool rSec = rangeIsSeconds();
  enum SLFmt { SLPlain, SLDuration, SLISO };
  SLFmt slFmt = SLPlain;
  if (cf == Start) {
    if (startWasDuration)
      slFmt = SLDuration;
    else if (startWasISO)
      slFmt = SLISO;
  } else if (cf == Last) {
    if (lastWasDuration)
      slFmt = SLDuration;
    else if (lastWasISO)
      slFmt = SLISO;
  } else {
    if (startWasDuration || lastWasDuration)
      slFmt = SLDuration;
    else if (startWasISO || lastWasISO)
      slFmt = SLISO;
  }
  auto formatSL = [&](double value) -> QString {
    if (slFmt == SLDuration && isSec)
      return secondsToDurationString(value);
    if (slFmt == SLISO && isCT)
      return ctimeToISOString(value);
    return QString::number(value, 'g', 12);
  };
  auto formatR = [&](double value) -> QString {
    if (rangeWasDuration && rSec)
      return secondsToDurationString(value);
    return QString::number(value, 'g', 12);
  };

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
        _last->setText(formatSL(minStart + range() - 1));
      }
      _start->setText(formatSL(startVal));
    } else if ((_controlField0 != Last) && (_controlField1 != Last)) {
      _last->setText(formatSL(start() + range() - 1));
    } else if ((_controlField0 != Range) && (_controlField1 != Range)) {
      _range->setText(formatR(last() - start() + 1));
    }
    return;
  }

  if (!mixedUnitsConvertible) {
    return;
  }

  const bool solveStart =
      (_controlField0 != Start) && (_controlField1 != Start);
  const bool solveLast = (_controlField0 != Last) && (_controlField1 != Last);
  const bool solveRange =
      (_controlField0 != Range) && (_controlField1 != Range);

  if (solveStart) {
    int lastFrame = 0;
    if (!frameFromUnits(last(), startUnits(), true, &lastFrame)) {
      return;
    }

    int startFrame = 0;
    int adjustedEndFrame =
        -1; // >=0 when start was clamped and last must be updated
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
      if (!unitsFromFrame(lastFrame + 1, rangeUnits(),
                          &lastExclusiveInRangeUnits)) {
        return;
      }

      const double startInRangeUnits = lastExclusiveInRangeUnits - range();
      double minRangeUnit = 0.0;
      const bool haveMin = unitsFromFrame(0, rangeUnits(), &minRangeUnit);
      if (haveMin && startInRangeUnits < minRangeUnit) {
        // Start would be before the beginning of the file.
        startFrame = 0;
        int exclusiveEndFrame = 0;
        if (frameFromUnits(minRangeUnit + range(), rangeUnits(), true,
                           &exclusiveEndFrame)) {
          adjustedEndFrame = qMax(startFrame, exclusiveEndFrame - 1);
        }
      } else {
        if (!frameFromUnits(startInRangeUnits, rangeUnits(), false,
                            &startFrame)) {
          return;
        }
      }
    }

    double startValue = 0;
    if (!unitsFromFrame(startFrame, startUnits(), &startValue)) {
      return;
    }
    _start->setText(formatSL(startValue));

    if (adjustedEndFrame >= 0) {
      // Update last to stay consistent: start is now at the beginning,
      // range is unchanged, so last moves forward (may be past end of file).
      double lastValue = 0;
      if (unitsFromFrame(adjustedEndFrame, startUnits(), &lastValue)) {
        _last->setText(formatSL(lastValue));
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
      if (!frameFromUnits(startInRangeUnits + range(), rangeUnits(), true,
                          &exclusiveEndFrame)) {
        return;
      }
      endFrame = qMax(startFrame, exclusiveEndFrame - 1);
    }

    double lastValue = 0;
    if (!unitsFromFrame(endFrame, startUnits(), &lastValue)) {
      return;
    }
    _last->setText(formatSL(lastValue));
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
      if (!unitsFromFrame(endFrame + 1, rangeUnits(),
                          &endExclusiveInRangeUnits)) {
        return;
      }
      rangeValue = endExclusiveInRangeUnits - startInRangeUnits;
      if (rangeValue < 0.0) {
        rangeValue = 0.0;
      }
    }
    _range->setText(formatR(rangeValue));
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
  const qreal defaultRange =
      dialogDefaults().value("vector/range", 1).toDouble();
  const qreal defaultStart =
      dialogDefaults().value("vector/start", 0).toDouble();
  bool defaultCountFromEnd =
      dialogDefaults().value("vector/countFromEnd", false).toBool();
  const bool defaultReadToEnd =
      dialogDefaults().value("vector/readToEnd", true).toBool();
  const int defaultSkip = dialogDefaults().value("vector/skip", 0).toInt();
  const bool defaultDoSkip =
      dialogDefaults().value("vector/doSkip", false).toBool();
  const bool defaultDoFilter =
      dialogDefaults().value("vector/doAve", false).toBool();
  const QString defaultRangeUnits =
      dialogDefaults().value("vector/rangeUnits", tr("frames")).toString();
  const QString defaultStartUnits =
      dialogDefaults().value("vector/startUnits", tr("frames")).toString();

  // Preserve previous behavior where readToEnd wins if both flags were saved
  // true.
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
  printf("DataRange::rangeIsValid: readToEnd=%d range=%g\n", readToEnd(),
         range());
  if (readToEnd()) {
    return true;
  } else {
    return (range() > 1);
  }
}

} // namespace Kst

// vim: ts=2 sw=2 et
