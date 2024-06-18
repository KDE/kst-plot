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

#include "matrixselector.h"

#include "objectstore.h"

#include "dialoglauncher.h"
#include "datacollection.h"
#include "geticon.h"

namespace Kst {

MatrixSelector::MatrixSelector(QWidget *parent, ObjectStore *store)
  : QWidget(parent), _store(store) {

  setupUi(this);

  int size = style()->pixelMetric(QStyle::PM_SmallIconSize);

  _newMatrix->setIcon(KstGetIcon("kst_matrixnew"));
  _editMatrix->setIcon(KstGetIcon("kst_matrixedit"));

  _newMatrix->setFixedSize(size + 8, size + 8);
  _editMatrix->setFixedSize(size + 8, size + 8);

  fillMatrices();

  connect(_newMatrix, SIGNAL(pressed()), this, SLOT(newMatrix()));
  connect(_editMatrix, SIGNAL(pressed()), this, SLOT(editMatrix()));

  connect(_matrix, SIGNAL(currentIndexChanged(int)), this, SLOT(matrixSelected(int)));
  connect(_matrix, SIGNAL(editTextChanged(QString)), this, SLOT(matrixSelected(QString)));
}


MatrixSelector::~MatrixSelector() {
}


void MatrixSelector::setObjectStore(ObjectStore *store) {
  _store = store;
  fillMatrices();;
}


MatrixPtr MatrixSelector::selectedMatrix() const {
  ObjectPtr p=_store->retrieveObject(_matrix->currentText());
  if(p.isPtrValid()) return kst_cast<Matrix>(p);
  else return NULL;
}


bool MatrixSelector::selectedMatrixDirty() const {
  return _matrix->currentIndex() != -1;
}


void MatrixSelector::matrixSelected(int index) {
  Q_UNUSED(index)
  if (index != -1)
    emit selectionChanged();
}

void MatrixSelector::matrixSelected(QString) {
  emit selectionChanged();
}

void MatrixSelector::setSelectedMatrix(MatrixPtr selectedMatrix) {
  int i = _matrix->findData(QVariant::fromValue(selectedMatrix.data()));
  if (i != -1) {
    _matrix->setCurrentIndex(i);
  }
}


void MatrixSelector::newMatrix() {
  QString matrixName;
  DialogLauncher::self()->showMatrixDialog(matrixName, 0, true);
  fillMatrices();
  MatrixPtr matrix = kst_cast<Matrix>(_store->retrieveObject(matrixName));

  if (matrix) {
    setSelectedMatrix(matrix);
  }
}


void MatrixSelector::editMatrix() {

  if (selectedMatrix()->provider()) {
    DialogLauncher::self()->showObjectDialog(selectedMatrix()->provider());
  } else {
    QString matrixName;
    DialogLauncher::self()->showMatrixDialog(matrixName, ObjectPtr(selectedMatrix()), true);
  }

}


void MatrixSelector::clearSelection() {
  _matrix->setCurrentIndex(-1);
}


void MatrixSelector::updateMatrices() {
  fillMatrices();;
}


void MatrixSelector::fillMatrices() {
  if (!_store) {
    return;
  }

  QHash<QString, MatrixPtr> matrices;

  MatrixList matrixList = _store->getObjects<Matrix>();

  MatrixList::ConstIterator it = matrixList.constBegin();
  for (; it != matrixList.constEnd(); ++it) {
    MatrixPtr matrix = (*it);

    matrix->readLock();
    matrices.insert(matrix->CleanedName(), matrix);
    matrix->unlock();
  }

  QStringList list = matrices.keys();

  std::sort(list.begin(), list.end());

  MatrixPtr current = selectedMatrix();

  _matrix->clear();
  foreach (const QString &string, list) {
    MatrixPtr m = matrices.value(string);
    _matrix->addItem(string, QVariant::fromValue(m.data()));
  }

  if (current)
    setSelectedMatrix(current);

  _editMatrix->setEnabled(_matrix->count() > 0);
}

}

// vim: ts=2 sw=2 et
