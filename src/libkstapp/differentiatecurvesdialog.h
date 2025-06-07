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

#ifndef DIFFERENTIATECURVESDIALOG_H
#define DIFFERENTIATECURVESDIALOG_H

#include <QDialog>

#include "ui_differentiatecurvesdialog.h"

#include "kstcore_export.h"

namespace Kst {

class ObjectStore;

class DifferentiateCurvesDialog : public QDialog, Ui::DifferentiateCurvesDialog
{
  Q_OBJECT
  public:
    explicit DifferentiateCurvesDialog(QWidget *parent);
    virtual ~DifferentiateCurvesDialog();

    void show();

  private slots:
    void updateButtons();
    void addButtonClicked();
    void removeButtonClicked();
    void upButtonClicked();
    void downButtonClicked();
    void OKClicked();
    void apply();

  private:
    void resetLists();

    ObjectStore *_store;

};

}

#endif

// vim: ts=2 sw=2 et
