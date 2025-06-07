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

#ifndef CHANGEFILEDIALOG_H
#define CHANGEFILEDIALOG_H

#include <QDialog>
#include "vector.h"
#include "matrix.h"
#include "relation.h"
#include "datasource.h"

#include "ui_changefiledialog.h"

#include "kstcore_export.h"

namespace Kst {

class ObjectStore;

class ChangeFileDialog : public QDialog, Ui::ChangeFileDialog
{
  Q_OBJECT
  public:
    explicit ChangeFileDialog(QWidget *parent);
    virtual ~ChangeFileDialog();

    void show();

  private Q_SLOTS:
    void fileNameChanged(const QString &file);
    void sourceValid(QString filename, int requestID);

    void addButtonClicked();
    void removeButtonClicked();
    void addAll();
    void removeAll();
    void selectAllFromFile();

    void availableDoubleClicked(QListWidgetItem * item);
    void selectedDoubleClicked(QListWidgetItem * item);
    void showConfigWidget();

    void updateButtons();
    void OKClicked();
    void apply();

    void updatePrimitiveList();

  private:
    ObjectStore *_store;
    DataSourcePtr _dataSource;
    int _requestID;
};

}

#endif

// vim: ts=2 sw=2 et
