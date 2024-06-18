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

#include "document.h"
#include "mainwindow.h"
#include "sessionmodel.h"
#include "tabwidget.h"
#include <datasourcefactory.h>
#include <graphicsfactory.h>
#include <datacollection.h>
#include <objectfactory.h>
#include <primitivefactory.h>
#include <relationfactory.h>
#include <viewitem.h>
#include <commandlineparser.h>
#include "objectstore.h"
#include "updatemanager.h"
#include "updateserver.h"

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QXmlStreamReader>


namespace Kst {

Document::Document(MainWindow *window)
: CoreDocument(), _win(window), _dirty(false), _isOpen(false) {
  _session = new SessionModel(objectStore());

  _fileName.clear();

  UpdateManager::self()->setStore(objectStore());
}


Document::~Document() {
  delete _session;
  _session = 0;
}


SessionModel* Document::session() const {
  return _session;
}


QString Document::fileName() const {
  return _fileName;
}

/** return a list of data objects where all dependencies appear earlier in the list */
ObjectList<DataObject> Document::sortedDataObjectList() {
  ObjectList<DataObject> sorted;
  ObjectList<DataObject> raw = objectStore()->getObjects<DataObject>();

  sorted.clear();


  // set the flag for all primitives: not strictly necessary
  // since it should have been done in the constructor, but...
  PrimitiveList all_primitives = objectStore()->getObjects<Primitive>();
  int n = all_primitives.size();
  for (int i=0; i<n; i++) {
    all_primitives[i]->setFlag(true);
  }

  // now unset the flags of all output primitives to indicate their parents haven't been
  // put in the sorted list yet
  n = raw.size();
  for (int i=0; i<n; i++) {
    raw[i]->setOutputFlags(false);
  }

  // now place into the sorted list all data objects whose inputs haven't got parents
  // or whose inputs have parents which are already in the sorted list.
  // do this at most n^2 times, which is worse than worse case.
  int i=0;
  while (!raw.isEmpty() && (++i <= n*n)) {
    DataObjectPtr D = raw.takeFirst();
    if (D->inputFlagsSet()) {
      D->setOutputFlags(true);
      sorted.append(D);
    } else {
      raw.append(D); // try again later
    }
  }

  if ((i== n*n) && (n>1)) {
    qDebug() << "Warning: loop detected, File will not be able to be loaded correctly!";
    while (!raw.isEmpty()) {
      DataObjectPtr D = raw.takeFirst();
      sorted.append(D);
    }
  }

  return sorted;
}


bool Document::save(const QString& to) {

  QString file = !to.isEmpty() ? to : _fileName;
  if (!file.endsWith(".kst")) {
    file.append(".kst");
  }
  QFile f(file);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    _lastError = QObject::tr("File could not be opened for writing.");
    return false;
  }

  Q_ASSERT(objectStore());

  objectStore()->cleanUpDataSourceList();

  _fileName = file;

  QXmlStreamWriter xml;
  xml.setDevice(&f);
  xml.setAutoFormatting(true);
  xml.writeStartDocument();
  xml.writeStartElement("kst");
  if (objectStore()->sessionVersion >=9999999) {
    xml.writeAttribute("version", "2.0.9");
  } else {
    xml.writeAttribute("version", objectStore()->sessionVersionString);
  }
  if (_win->scriptServerNameSet()) {
    QString server_name = _win->scriptServerName();
    QString user_name = "--"+kstApp->userName();
    if (server_name.endsWith(user_name)) {
      server_name.remove(server_name.lastIndexOf(user_name),10000);
      xml.writeAttribute("scriptServerNameHasUserName", QVariant(true).toString());
    }

    xml.writeAttribute("scriptServerName", server_name);
  }

  xml.writeStartElement("data");
  foreach (DataSourcePtr s, objectStore()->dataSourceList()) {
    s->saveSource(xml);
  }
  xml.writeEndElement();

  xml.writeStartElement("variables");

  foreach (VectorPtr s, objectStore()->getObjects<Vector>()) {
    s->save(xml);
  }
  foreach (MatrixPtr s, objectStore()->getObjects<Matrix>()) {
    s->save(xml);
  }
  foreach (ScalarPtr s, objectStore()->getObjects<Scalar>()) {
    s->save(xml);
  }
  foreach (StringPtr s, objectStore()->getObjects<String>()) {
    s->save(xml);
  }
  xml.writeEndElement();

  xml.writeStartElement("objects");
  ObjectList<DataObject> dataObjects = sortedDataObjectList();
  foreach (DataObjectPtr s, dataObjects) {
    s->save(xml);
  }
  dataObjects.clear();
  xml.writeEndElement();

  xml.writeStartElement("relations");
  foreach (RelationPtr s, objectStore()->getObjects<Relation>()) {
    s->save(xml);
  }
  xml.writeEndElement();

  xml.writeStartElement("graphics");
  xml.writeAttribute("currentTab", QString::number(_win->tabWidget()->currentIndex()));
  for (int i = 0; i < _win->tabWidget()->count(); ++i) {
    View *v = qobject_cast<View*>(_win->tabWidget()->widget(i));
    xml.writeStartElement("view");
    xml.writeAttribute("name", _win->tabWidget()->tabText(i));

    v->save(xml);

    xml.writeEndElement();
  }
  xml.writeEndElement();

  xml.writeEndDocument();

  setChanged(false);
  _isOpen = true; // Set _isOpen when saving into a new file so that kst does not ask for the filename again
  return true;
}

void Document::updateRecentDataFiles(const QStringList &datafiles) {
  foreach(const QString &file, datafiles) {
    _win->updateRecentDataFiles(file);
  }
}

bool Document::initFromCommandLine(CommandLineParser *P) {

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  bool ok;
  bool dataPlotted = P->processCommandLine(&ok);

  if (!dataPlotted && ok) {
    QString kstfile = P->kstFileName();
    if (!kstfile.isEmpty()) {
      dataPlotted = open(kstfile);
      _win->updateRecentKstFiles(kstfile);

      if (dataPlotted) {
        UpdateManager::self()->doUpdates(true);
        setChanged(false);
      }
    }
  }
  objectStore()->override.fileName.clear();
  objectStore()->override.f0 = objectStore()->override.N =
                               objectStore()->override.skip =
                               objectStore()->override.doAve = -5;

  QApplication::restoreOverrideCursor();

  return ok;
}


#define malformed() \
  return false;


bool Document::open(const QString& file) {
  _isOpen = false;
  QFile f(file);
  if (!f.open(QIODevice::ReadOnly)) {
    _lastError = QObject::tr("File could not be opened for reading.");
    return false;
  }
  // Temporarily set the application dir to the current dir to be able to load data using the "fileRelative" attribute
  QString restorePath = QDir::currentPath();
  QDir::setCurrent(file.left(file.lastIndexOf('/')) + '/');
  _fileName = file;

  // If we move this into the <graphics> block then we could, if desired, open
  // .kst files that contained only data and basically "merge" that data into
  // the current session
  
  View *loadedView = 0;
  bool firstView = true;
  QRectF currentSceneRect;

  QXmlStreamReader xml;
  xml.setDevice(&f);

  enum State { Unknown=0, Data, Variables, Objects, Relations, Graphics, View };
  State state = Unknown;
  int currentTab = 0;

  while (!xml.atEnd()) {
    if (xml.isStartElement()) {
      QString n = xml.name().toString();
      if (n == "kst") {
        QXmlStreamAttributes attrs = xml.attributes();
        objectStore()->sessionVersionString = attrs.value("version").toString();
        QStringList version = objectStore()->sessionVersionString.split('.');
        objectStore()->sessionVersion = version[0].toInt() * 10000 + version[1].toInt();
        if (version.size()>2) {
          objectStore()->sessionVersion += version[2].toInt();
        }
        QString server_name = attrs.value("scriptServerName").toString();
        if (attrs.value("scriptServerNameHasUserName").toString() == "true") {
          server_name += "--"  + kstApp->userName();
        }

        if (!server_name.isEmpty()) {
          _win->setScriptServerName(server_name);
        }

        //qDebug() << "version" << version << objectStore()->sessionVersion;
      } else if (n == "data") {
        if (state != Unknown) {
          malformed();
        }
        state = Data;
      } else if (n == "variables") {
        if (state != Unknown) {
          malformed();
        }
        state = Variables;
      } else if (n == "objects") {
        if (state != Unknown) {
          malformed();
        }
        state = Objects;
      } else if (n == "relations") {
        if (state != Unknown) {
          malformed();
        }
        state = Relations;
      } else if (n == "graphics") {
        if (state != Unknown) {
          malformed();
        }
        state = Graphics;
        QXmlStreamAttributes attrs = xml.attributes();
        currentTab = attrs.value("currentTab").toString().toInt();
      } else {
        switch (state) {
          case Objects:
            {
              DataObjectPtr object = ObjectFactory::parse(objectStore(), xml);
              if (object) {
//                addDataObjectToList(object);
              } else {
                malformed();
              }
              break;
            }
          case Graphics:
            {
              if (n == "view") {
                QXmlStreamAttributes attrs = xml.attributes();
                loadedView = new Kst::View(0);

                QBrush brush;
                QStringRef av = attrs.value("gradient");
                if (!av.isNull()) {
                  QStringList stopInfo = av.toString().split(',', Qt::SkipEmptyParts);
                  QLinearGradient gradient(1,0,0,0);
                  gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
                  for (int i = 0; i < stopInfo.size(); i+=2) {
                    gradient.setColorAt(stopInfo.at(i).toDouble(), QColor(stopInfo.at(i+1)));
                  }
                  brush = QBrush(gradient);
                } else {
                  av = attrs.value("color");
                  if (!av.isNull()) {
                      brush.setColor(QColor(av.toString()));
                  }
                  av = attrs.value("style");
                  if (!av.isNull()) {
                    brush.setStyle((Qt::BrushStyle)av.toString().toInt());
                  }
                }
                loadedView->setBackgroundBrush(brush);

                if (firstView) {
                  _win->tabWidget()->clear();
                  firstView = false;
                }
                _win->tabWidget()->addView(loadedView);
                loadedView->setObjectName(attrs.value("name").toString());
                _win->tabWidget()->setCurrentViewName(attrs.value("name").toString());

                qreal width = 1.0, height = 1.0;
                av = attrs.value("width");
                if (!av.isNull()) {
                   width = av.toString().toDouble();
                }
                av = attrs.value("height");
                if (!av.isNull()) {
                   height = av.toString().toDouble();
                }
                currentSceneRect = QRectF(QPointF(0, 0), QSizeF(width, height));
                state = View;
              } else {
                malformed();
              }
            }
            break;
          case View:
            {

              ViewItem *i = GraphicsFactory::parse(xml, objectStore(), loadedView);
              if (i) {
                loadedView->scene()->addItem(i);
              }
            }
            break;
          case Data:
            DataSourceFactory::parse(objectStore(), xml);
            break;
          case Variables:
            PrimitiveFactory::parse(objectStore(), xml);
            break;
          case Relations:
            RelationFactory::parse(objectStore(), xml);
            break;
          case Unknown:
            malformed();
            break;
        }
      }
    } else if (xml.isEndElement()) {
      QString n = xml.name().toString();
      if (n == "kst") {
        if (state != Unknown) {
          malformed();
        }
        break;
      } else if (n == "view") {
        if (loadedView->sceneRect() != currentSceneRect) {
          loadedView->forceChildResize(currentSceneRect, loadedView->sceneRect());
        }
        state = Graphics;
      } else if (n == "data") {
        state = Unknown;
      } else if (n == "objects") {
        state = Unknown;
      } else if (n == "variables") {
        state = Unknown;
      } else if (n == "relations") {
        state = Unknown;
      } else if (n == "graphics") {
        state = Unknown;
      }
    }
    xml.readNext();
  }

  if (xml.hasError()) {
    _lastError = QObject::tr("File is malformed and encountered an error while reading.");
    return false;
  }

  _vectornum = max_vectornum+1;
  _scalarnum = max_scalarnum+1;
  _pluginnum = max_pluginnum+1;
  _csdnum = max_csdnum+1;
  _curvecnum = max_curvenum+1;
  _equationnum = max_equationnum+1;
  _histogramnum = max_histogramnum+1;
  _imagenum = max_imagenum+1;
  _psdnum = max_psdnum+1;
  _stringnum = max_stringnum+1;
  _matrixnum = max_matrixnum+1;


  if (_win->tabWidget()->count() > currentTab) {
    _win->tabWidget()->setCurrentIndex(currentTab);
  }

  UpdateManager::self()->doUpdates(true);
  setChanged(false);
  // Restore current app path
  QDir::setCurrent(restorePath);


  UpdateServer::self()->requestUpdateSignal();

  return _isOpen = true;
}

#undef malformed


void Document::createView() {
  _win->tabWidget()->createView();
}


QString Document::lastError() const {
  return _lastError;
}


bool Document::isChanged() const {
  return _dirty;
}


bool Document::isOpen() const {
  return _isOpen;
}


void Document::setChanged(bool dirty) {
  _dirty = dirty;
}

View* Document::currentView() const {
  return _win->tabWidget()->currentView();
}

}

// vim: ts=2 sw=2 et
