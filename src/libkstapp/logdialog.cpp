/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2010 C. Barth Netterfield                             *
 *                   netterfield@astro.utoronto.ca                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "logdialog.h"
#include "dialogdefaults.h"

#include "mainwindow.h"

#include <time.h>
#include <QImageWriter>

namespace Kst {

LogDialog::LogDialog(MainWindow *parent)
  : QDialog(parent), _logtime(0), _format(QString()), _logdir(QString()), _parent(parent), _closeIfFinished(false) {

  setupUi(this);

  QStringList formats;
  foreach(const QByteArray &array, QImageWriter::supportedImageFormats()) {
    formats.append(QString(array));
  }

  _formats->addItems(formats);
  _formats->setCurrentIndex(
        _formats->findText(dialogDefaults().value("log/format","png").toString()));

  _xSize->setValue(dialogDefaults().value("log/xsize","1024").toInt());
  _ySize->setValue(dialogDefaults().value("log/ysize","768").toInt());

  _sizeOption->setCurrentIndex(dialogDefaults().value("log/sizeOption","0").toInt());
  enableWidthHeight();

  _saveLocation->setFile(dialogDefaults().value("log/logdir",QDir::currentPath()).toString());

  _script->setText(dialogDefaults().value("log/script", QString()).toString());

  _tab->setCurrentIndex(0);

  _user->setText(dialogDefaults().value("log/user", QString()).toString());

  connect(_message, SIGNAL(textChanged()), this, SLOT(changed()));
  connect(_sizeOption, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()));
  connect(_formats, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()));
  connect(_sizeOption, SIGNAL(currentIndexChanged(int)), this, SLOT(changed()));
  connect(_sizeOption, SIGNAL(currentIndexChanged(int)), this, SLOT(enableWidthHeight()));
  connect(_xSize, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(_ySize, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(_saveLocation, SIGNAL(changed(QString)), this, SLOT(changed()));
  connect(_saveLocation, SIGNAL(changed(QString)), this, SLOT(enableApply()));
  connect(_script, SIGNAL(textChanged(QString)), this, SLOT(enableApply()));

  connect(_close, SIGNAL(clicked()), this, SLOT(close()));
  connect(_apply, SIGNAL(clicked()), this, SLOT(apply()));
  connect(_ok, SIGNAL(clicked()), this, SLOT(ok()));

  _proc = new QProcess(this);
  connect(_proc, SIGNAL(readyReadStandardError()), this, SLOT(scriptStdErr()));
  connect(_proc, SIGNAL(readyReadStandardOutput()), this, SLOT(scriptStdOut()));
  connect(_proc, SIGNAL(started()), this, SLOT(scriptStarted()));
  connect(_proc, SIGNAL(finished(int)), this, SLOT(scriptFinished(int)));
  connect(_rerunScript, SIGNAL(clicked()), this, SLOT(runScript()));
  connect(_proc, SIGNAL(error(QProcess::ProcessError)), this, SLOT(scriptError(QProcess::ProcessError)));

  _saveLocation->setMode(QFileDialog::Directory);

  QFileInfo info(_saveLocation->file());
  bool valid = info.isDir() && info.isWritable();
  _apply->setEnabled(valid);

  _scriptRunning->clear();

}

LogDialog::~LogDialog() {
}

void LogDialog::changed() {
  _rerunScript->setEnabled(false);
  _scriptRunning->clear();
}


void LogDialog::enableApply() {
  QFileInfo info(_saveLocation->file());
  bool file_valid = info.isDir() && info.isWritable();

  _apply->setEnabled(file_valid);
}


void LogDialog::doIt() {
  _logdir = _saveLocation->file();
  _format = _formats->currentText();
  _username = _user->text();
  int x_size = _xSize->value();
  int y_size = _ySize->value();
  int size_option_index = _sizeOption->currentIndex();

  dialogDefaults().setValue("log/logdir", _logdir);
  dialogDefaults().setValue("log/format", _format);
  dialogDefaults().setValue("log/xsize", x_size);
  dialogDefaults().setValue("log/ysize", y_size);
  dialogDefaults().setValue("log/sizeOption", size_option_index);
  dialogDefaults().setValue("log/script", _script->text());
  dialogDefaults().setValue("log/user", _username);

  _logtime = time(NULL);

  if (_logdir.endsWith('/')) {
    _imagename = _logdir;
  } else {
    _imagename = _logdir + '/';
  }
  _imagename += QString::number(_logtime)+'.';

  _msgfilename = _imagename + "txt";
  _imagename += _format;

  _parent->exportLog(_imagename, _msgfilename, _format, x_size, y_size, size_option_index, _message->toPlainText());
  runScript();

  _rerunScript->setEnabled(true);
}

void LogDialog::apply() {
  _closeIfFinished = false;
  doIt();
}

void LogDialog::ok() {
  _closeIfFinished = true;
  doIt();
}

void LogDialog::runScript() {
  QString script = _script->text().simplified().replace("$imagefile",_imagename).
                   replace("$messagefile", _msgfilename).replace("$user", _username);
  QStringList args = QProcess::splitCommand(script);
  if (!args.isEmpty()) {
    QString cmd = args.first();
    args.removeFirst();
    _proc->start(cmd, args);
  }
}


void LogDialog::scriptStdErr() {

}


void LogDialog::scriptStdOut() {
  _scriptOutput->appendPlainText(_proc->readAllStandardOutput());
}


void LogDialog::scriptStarted() {
  _scriptRunning->setText(tr("Script: Running"));
}


void LogDialog::scriptFinished(int code) {
  if (code == 0) {
    _scriptRunning->setText(tr("Script: Finished"));
    if (_closeIfFinished) {
      close();
    }
  } else {
    _scriptRunning->setText(tr("Script: return code %1").arg(code));
  }
}


void LogDialog::scriptError(QProcess::ProcessError p) {
  if (p==QProcess::FailedToStart) {
    _scriptRunning->setText(tr("Script error: Failed to start"));
  } else if (p==QProcess::Crashed) {
    _scriptRunning->setText(tr("Script error: Crashed"));
  } else {
    _scriptRunning->setText(tr("Script error:"));
  }
}


void LogDialog::enableWidthHeight() {
  int size_option_index = _sizeOption->currentIndex();

  switch (size_option_index) {
  case 0: // Width and Maintain Aspect Ratio
    _xSize->setEnabled(true);
    _ySize->setEnabled(false);
    _widthLabel->setEnabled(true);
    _heightLabel->setEnabled(false);
    break;
  case 1: // Height and Maintain Aspect Ratio
    _xSize->setEnabled(false);
    _ySize->setEnabled(true);
    _widthLabel->setEnabled(false);
    _heightLabel->setEnabled(true);
    break;
  case 2: // Width and Height
    _xSize->setEnabled(true);
    _ySize->setEnabled(true);
    _widthLabel->setEnabled(true);
    _heightLabel->setEnabled(true);
    break;
  case 3: // Size of Square
    _xSize->setEnabled(true);
    _ySize->setEnabled(false);
    _widthLabel->setEnabled(true);
    _heightLabel->setEnabled(false);
    break;
  }
}


}
