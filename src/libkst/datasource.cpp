/***************************************************************************
                   datasource.cpp  -  abstract data source
                             -------------------
    begin                : Thu Oct 16 2003
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

#include "datasource.h"

#include <assert.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QPluginLoader>
#include <QTextDocument>
#include <QUrl>
#include <QXmlStreamWriter>
#include <QTimer>
#include <QRegularExpression>
#include <QFileSystemWatcher>


#include "datacollection.h"
#include "debug.h"
#include "objectstore.h"
#include "scalar.h"
#include "string.h"
#include "nextcolor.h"
#include "updatemanager.h"

#include "dataplugin.h"


// TODO DataSource should not need the plugin code
#include "datasourcepluginmanager.h"


using namespace Kst;

template<class T>
struct NotSupportedImp : public DataSource::DataInterface<T>
{
  // read one element
  int read(const QString&, typename T::ReadInfo&) { return -1; }

  // named elements
  QStringList list() const { return QStringList(); }
  bool isListComplete() const { return false; }
  bool isValid(const QString&) const { return false; }

  // T specific
  const typename T::DataInfo dataInfo(const QString&, int frame=0) const { Q_UNUSED(frame) return typename T::DataInfo(); }
  void setDataInfo(const QString&, const typename T::DataInfo&) {}

  // meta data
  QMap<QString, double> metaScalars(const QString&) { return QMap<QString, double>(); }
  QMap<QString, QString> metaStrings(const QString&) { return QMap<QString, QString>(); }
};


const QString DataSource::staticTypeString = "Data Source";
const QString DataSource::staticTypeTag = "source";

Object::UpdateType DataSource::objectUpdate(qint64 newSerial) {
  if (_serial==newSerial) {
    return NoChange;
  }

  UpdateType updated = NoChange;

  if (!UpdateManager::self()->paused()) {
    // update the datasource
    updated = internalDataSourceUpdate();
    if (updated == Updated) {
      _serialOfLastChange = newSerial; // tell data objects it is new
    }
  }

  _serial = newSerial;

  return updated;
}


void DataSource::_initializeShortName() {
  _shortName = QString("DS%1").arg(_datasourcenum);
  if (_datasourcenum>max_datasourcenum)
    max_datasourcenum = _datasourcenum;
  _datasourcenum++;
}

bool Kst::DataSource::isValid() const
{
  return _valid;
}


bool DataSource::hasConfigWidget() const {
    return DataSourcePluginManager::sourceHasConfigWidget(_filename, fileType());
}


DataSourceConfigWidget* DataSource::configWidget() {
  if (!hasConfigWidget())
    return 0;

  DataSourceConfigWidget *w = DataSourcePluginManager::configWidgetForSource(_filename, fileType());
  Q_ASSERT(w);

  //This is still ugly to me...
  w->_instance = this;

  // TODO check if not all plugins already have load() called
  w->load();

  return w;
}



DataSource::DataSource(ObjectStore *store, QSettings *cfg, const QString& filename, const QString& type) :
  Object(),
  _filename(filename),
  _alternateFilename(QString()),
  _cfg(cfg),
  interf_scalar(new NotSupportedImp<DataScalar>),
  interf_string(new NotSupportedImp<DataString>),
  interf_vector(new NotSupportedImp<DataVector>),
  interf_matrix(new NotSupportedImp<DataMatrix>),
  _watcher(0),
  _color(NextColor::self().current())
{
  Q_UNUSED(type)
  Q_UNUSED(store)

  _valid = false;
  _reusable = true;
  _writable = false;
  _watcher = 0L;

  _initializeShortName();

  // Timer needs to be the default: File sometimes fails.
  startUpdating(Timer);
}

DataSource::~DataSource() {
  resetFileWatcher();
  delete interf_scalar;
  delete interf_string;
  delete interf_vector;
  delete interf_matrix;
}

QString DataSource::cleanPath(QString abs_path) {
  QString name = QDir::cleanPath(abs_path);

  // qdir::cleanpath doesn't seem to remove leading /.. from paths (!)
  while (name.startsWith("/..")) {
    name.remove(QRegularExpression("^/.."));
  }

  return name;
}

QMap<QString, QString> DataSource::fileMetas() const
{
  QMap<QString, QString> map;
  QFileInfo info(_filename);
  map["File name"] = info.fileName();
  map["File path"] = info.path();
  map["File creation"] = info.birthTime().toString(Qt::ISODate).replace('T', ' ');
  map["File modification"] = info.lastModified().toString(Qt::ISODate).replace('T', ' ');
  return map;
}


void DataSource::resetFileWatcher() {
  if (_watcher) {
    disconnect(_watcher, SIGNAL(fileChanged(QString)), this, SLOT(checkUpdate()));
    disconnect(_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(checkUpdate()));
    delete _watcher;
    _watcher = 0L;
  }
}


void DataSource::setInterface(DataInterface<DataScalar>* i) {
  delete interf_scalar;
  interf_scalar = i;
}

void DataSource::setInterface(DataInterface<DataString>* i) {
  delete interf_string;
  interf_string = i;
}

void DataSource::setInterface(DataInterface<DataVector>* i) {
  delete interf_vector;
  interf_vector = i;
}

void DataSource::setInterface(DataInterface<DataMatrix>* i) {
  delete interf_matrix;
  interf_matrix = i;
}


DataSource::UpdateCheckType DataSource::updateType() const
{
  return _updateCheckType;
}

void DataSource::setUpdateType(UpdateCheckType updateType)
{
    _updateCheckType = updateType;
}

void DataSource::startUpdating(UpdateCheckType updateType, const QString& file)
{
  setUpdateType(updateType);
  resetFileWatcher();
  if (_updateCheckType == Timer) {
    QTimer::singleShot(UpdateManager::self()->minimumUpdatePeriod()-1, this, SLOT(checkUpdate()));
  } else if (_updateCheckType == File) {
    // TODO only works on local files:
    // http://bugreports.qt.nokia.com/browse/QTBUG-8351
    // http://bugreports.qt.nokia.com/browse/QTBUG-13248
    _watcher = new QFileSystemWatcher();
    const QString usedfile = (file.isEmpty() ? _filename : file);      
    _watcher->addPath(usedfile);
    connect(_watcher, SIGNAL(fileChanged(QString)), this, SLOT(checkUpdate()));
    connect(_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(checkUpdate()));
  }
}


void DataSource::checkUpdate() {
  if (!UpdateManager::self()->paused()) {
    UpdateManager::self()->doUpdates(false);
  }

  if (_updateCheckType == Timer) {
    QTimer::singleShot(UpdateManager::self()->minimumUpdatePeriod()-1, this, SLOT(checkUpdate()));
  }
}


void DataSource::deleteDependents() {

  ObjectList<Primitive> primitiveList = _store->getObjects<Primitive>();
  foreach (PrimitivePtr primitive, primitiveList) {
    DataPrimitive* dp = qobject_cast<DataPrimitive*>(primitive);
    if (dp && (dp->dataSource()->Name() == Name())) {
      primitive->deleteDependents();
      _store->removeObject(primitive);
    }
  }

  QList<ObjectPtr> Objects = _store->objectList();

  foreach (const PrimitivePtr &p, slavePrimitives) {
    ObjectPtr op = kst_cast<Object>(p);
    foreach (ObjectPtr object, Objects) {
      if (object->uses(op)) {
        _store->removeObject(object);
      }
    }
    store()->removeObject(p);
  }
}


QString DataSource::typeString() const {
  return staticTypeString;
}


QString DataSource::fileName() const {
  // Look to see if it was a URL and save the URL instead
  const QMap<QString,QString> urlMap = DataSourcePluginManager::urlMap();
  for (QMap<QString,QString>::ConstIterator i = urlMap.begin(); i != urlMap.end(); ++i) {
    if (i.value() == _filename) {
      return i.key();
    }
  }

  return DataSource::cleanPath(_filename);
}

QString DataSource::alternateFilename() const {
  return DataSource::cleanPath(_alternateFilename);
}

void DataSource::setAlternateFilename(const QString &file) {
  _alternateFilename = file;
}

QString DataSource::fileType() const {
  return QString();
}

void DataSource::save(QXmlStreamWriter &s) {
  Q_UNUSED(s)
}


void DataSource::saveSource(QXmlStreamWriter &s) {
  QString name = _filename;
  // Look to see if it was a URL and save the URL instead
  const QMap<QString,QString> urlMap = DataSourcePluginManager::urlMap();
  for (QMap<QString,QString>::ConstIterator i = urlMap.begin(); i != urlMap.end(); ++i) {
    if (i.value() == _filename) {
      name = i.key();
      break;
    }
  }
  s.writeStartElement("source");
  s.writeAttribute("reader", fileType());
  s.writeAttribute("updateType", QString::number(int(_updateCheckType)));
  DataPrimitive::saveFilename(name, s);
  save(s);

  saveNameInfo(s, DATASOURCENUM | SCALARNUM | STRINGNUM);
  s.writeEndElement();
}



void DataSource::parseProperties(QXmlStreamAttributes &properties) {
  Q_UNUSED(properties);
}


bool DataSource::isEmpty() const {
  return true;
}


void DataSource::reset() {
  Object::reset();
}


bool DataSource::supportsTimeConversions() const {
  return false;
}

QString DataSource::timeFormat() const {
  return QString();
}

int DataSource::sampleForTime(const QDateTime& time, bool *ok) {
  Q_UNUSED(time)
  if (ok) {
    *ok = false;
  }
  return 0;
}



int DataSource::sampleForTime(double ms, bool *ok) {
  Q_UNUSED(ms)
  if (ok) {
    *ok = false;
  }
  return 0;
}



QDateTime DataSource::timeForSample(int sample, bool *ok) {
  Q_UNUSED(sample)
  if (ok) {
    *ok = false;
  }
  return QDateTime::currentDateTime();
}


bool DataSource::isTime(const QString& field) const {
  return (_timeFields.contains(field));
}


double DataSource::relativeTimeForSample(int sample, bool *ok) {
  Q_UNUSED(sample)
  if (ok) {
    *ok = false;
  }
  return 0;
}


double DataSource::frameToIndex(int frame, const QString &field) {
  return readDespikedIndex(frame+1, field);
}


int DataSource::indexToFrame(double X, const QString &field) {
  const DataVector::DataInfo info = vector().dataInfo(field);
  int Fmin = 0;
  int Fmax = info.frameCount-1;
  double Xmin = readDespikedIndex(Fmin, field);
  double Xmax = readDespikedIndex(Fmax, field);
  int F0 = (Fmin + Fmax)/2;
  double X0;

  if (X>=Xmax) {
    return Fmax;
  }
  if (X<=Xmin) {
    return Fmin;
  }

  while (F0 != Fmin) {
    X0 = readDespikedIndex(F0, field);
    if ((X0>Xmax) || (X0<Xmin)) { // not monotoically rising!
      return (-1);
    }

    if (X <= X0) {
      Xmax = X0;
      Fmax = F0;
    } else {
      Xmin = X0;
      Fmin = F0;
    }
    F0 = (Fmin + Fmax)/2;
  }
  return F0;
}

/* generic determination of frames per index (eg, frames per unit "TIME")
 * This is a challenge because:
 *  i) the index may not change every frame (eg, ctime index w/ 5 samples/frame
 *  ii) the starting frame may not be frame 0
 * Here, as a hacky heuristic, we will see how much index changes in the last
 * 1000 frames... hoping that index changes enough to make this accurate, and that
 * there are at least 1000 valid frames in the file, or that the first frame is frame 0.
 */
double DataSource::framePerIndex(const QString &field) {
  // FIXME: for now, calculate the sample rate, but later allow us to define it
  const DataVector::DataInfo info = vector().dataInfo(field);
  int fn = info.frameCount-2;
  int f0 = fn-1000.0; //FIXME: not general (but maybe nothing is...)
  if (f0<0) {
    f0 = 0;
  }
  if (f0 == fn) {
    return 1.0;
  }

  double x0 = readDespikedIndex(f0, field);
  double xn = readDespikedIndex(fn, field);

  if (xn == x0) {
    return 1.0;
  }

  return double(fn-f0)/(xn - x0);
}

double DataSource::readDespikedIndex(int frame_in, const QString &field) {

  // we want a despike buffer which is an integer number of frames and
  // at least 5 samples on each side of our desired sample
  int frame = frame_in;
  const int min_despike_margin = 5; // samples
  const DataVector::DataInfo info = vector().dataInfo(field);
  int margin_frames = qMax(1,min_despike_margin/info.samplesPerFrame);
  int margin_samp = margin_frames * info.samplesPerFrame;
  double *data = new double[2*margin_samp];
  double x;

  frame -= margin_frames;
  if (frame<0) {
    frame = 0;
  }
  if (frame + 2*margin_frames >= info.frameCount) {
    frame = info.frameCount - 2*margin_frames;
  }
  DataVector::ReadInfo par = {data, frame, 2*margin_frames, -1};

  vector().read(field, par);

  bool spike_found;
  int n = 2*margin_samp-1;
  do {
    int j=0;
    spike_found = false;
    for (int i = 0; i < n; i++) {
      if (data[i+1] >= data[i]) { // increasing.  This point is probably ok.
        data[j++] = data[i];
      } else {
        i++; // this point and the next one are now suspect - skip them
        spike_found = true;
      }
    }
    n  = j;
  } while(spike_found); // We found a spike - better check for wider ones.

  x = data[n/2]; // FIXME: we might be off by a couple samples if there were spikes.

  delete[] data;

  return(x);
}


QStringList &DataSource::timeFields() {
  if (_timeFields.size() == 0) {
    // FIXME: this must be created by the UI somehow.
    // or by the datasource itself.  Or something
    // different than this!
    QStringList requestedFields;
    requestedFields.append("TIME");
    requestedFields.append("Time");
    requestedFields.append("time");
    requestedFields.append("Temps");
    requestedFields.append("TEMPS");
    requestedFields.append("temps");

    // Make sure the requested fields actually exist.
    foreach (const QString &field, requestedFields) {
      if (vector().list().contains(field)) {
        _timeFields.append(field);
      }
    }
  }

  return(_timeFields);
}

QStringList &DataSource::indexFields() {
  if (_frameFields.size() == 0) {
    _frameFields.append(tr("frames"));
    _frameFields.append(timeFields());
  }

  return(_frameFields);
}

bool DataSource::reusable() const {
  return _reusable;
}


void DataSource::disableReuse() {
  _reusable = false;
}

QString DataSource::_automaticDescriptiveName() const {
  return QFileInfo(_filename).fileName();
}

QString DataSource::descriptionTip() const {
  return fileName();
}

QColor DataSource::color() const {
  return _color;
}

void DataSource::setColor(const QColor& color) {
  _color = color;
}


/////////////////////////////////////////////////////////////////////////////
DataSourceConfigWidget::DataSourceConfigWidget(QSettings& settings)
: QWidget(0L), _cfg(settings) {
}


DataSourceConfigWidget::~DataSourceConfigWidget() {
}


void DataSourceConfigWidget::setInstance(DataSourcePtr inst) {
  _instance = inst;
}


DataSourcePtr DataSourceConfigWidget::instance() const {
  return _instance;
}


QSettings& DataSourceConfigWidget::settings() const {
  return _cfg;
}

bool DataSourceConfigWidget::isOkAcceptabe() const {
  return true;
}

bool DataSourceConfigWidget::hasInstance() const {
  return _instance != 0L;
}


ValidateDataSourceThread::ValidateDataSourceThread(const QString& file, const int requestID) : QRunnable(),
  _file(file),
  _requestID(requestID) {
}

static QMutex _mutex;
void ValidateDataSourceThread::run() {

  QFileInfo info(_file);
  if (!info.exists()) {
    emit dataSourceInvalid(_requestID);
    return;
  }

  // FIXME validSource(_file) is not thread safe, so wait
  // if there is another one running
  QMutexLocker locker(&_mutex);
  if (!DataSourcePluginManager::validSource(_file)) {
    emit dataSourceInvalid(_requestID);
    return;
  }

  emit dataSourceValid(_file, _requestID);
}


// vim: ts=2 sw=2 et


