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

#include "filerequester.h"
#include "geticon.h"

#include <QStyle>
#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QCompleter>

#include <QDebug>

namespace Kst {

FileRequester::FileRequester(QWidget *parent)
  : QWidget(parent), _mode(QFileDialog::AnyFile) {
  setup();
}


FileRequester::~FileRequester() {
}


void FileRequester::setup() {
  _fileEdit = new QLineEdit(this);
  _fileButton = new QToolButton(this);

  QHBoxLayout * layout = new QHBoxLayout(this);
  layout->setContentsMargins({});
  layout->addWidget(_fileEdit);
  layout->addWidget(_fileButton);
  setLayout(layout);

  int size = style()->pixelMetric(QStyle::PM_SmallIconSize);
  _fileButton->setIcon(KstGetIcon("kst_changefile"));
  _fileButton->setFixedSize(size + 8, size + 8);

  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  connect (_fileEdit, SIGNAL(textChanged(QString)), this, SLOT(updateFile(QString)));
  connect (_fileButton, SIGNAL(clicked()), this, SLOT(chooseFile()));

  QFileSystemModel *dirModel = new QFileSystemModel(this);
  dirModel->setFilter(QDir::AllEntries);
  dirModel->setRootPath(QString('/'));

  QCompleter *completer = new QCompleter(this);
  completer->setModel(dirModel); 

  _fileEdit->setCompleter(completer);
}


QString FileRequester::file() const {
  return _file;
}


void FileRequester::setFile(const QString &file) {
  _file = file;
  //FIXME grrr QLineEdit doc *lies* to me... the textEdited signal is being triggered!!
  _fileEdit->blockSignals(true);
  _fileEdit->setText(_file);
  _fileEdit->blockSignals(false);
  emit changed(file);
}

void FileRequester::updateFile(const QString &file) {
  if (file.contains('~')) {
    QString home = qgetenv("HOME"); // linux
    if (!home.isEmpty()) {
      QString changed_file = file;
      changed_file.replace('~', home);
      setFile(changed_file);
      return;
    }
    home = qgetenv("USERPROFILE"); // windows, maybe (?)
    if (!home.isEmpty()) {
      QString changed_file = file;
      changed_file.replace('~', home);
      setFile(changed_file);
      return;
    }
  }

  _file = file;
  emit changed(file);
}


void FileRequester::chooseFile() {
  QString file;
  if (_mode == QFileDialog::ExistingFile) {
    file = QFileDialog::getOpenFileName(this, tr("Open File"), _file, tr("All Files (*)"));
  } else if (_mode == QFileDialog::Directory) {
    file = QFileDialog::getExistingDirectory(this, tr("Logfile Directory"), _file);
  } else {
    file = QFileDialog::getSaveFileName(this, tr("Save File"), _file, tr("All Files (*)"));
  }

  if (!file.isEmpty()) {
    setFile(file);
  }
}

}

// vim: ts=2 sw=2 et
