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

#ifndef EXPORTGRAPHICSDIALOG_H
#define EXPORTGRAPHICSDIALOG_H

#include <QDialog>

#include "ui_exportgraphicsdialog.h"

#include "kstcore_export.h"

namespace Kst {

class MainWindow;

class ExportGraphicsDialog : public QDialog, Ui::ExportGraphicsDialog
{
  Q_OBJECT
  public:
    explicit ExportGraphicsDialog(MainWindow *win);
    virtual ~ExportGraphicsDialog();
    bool exportAll() { return _exportAll->isChecked();}

  public slots:
    void enableWidthHeight();
    void OKClicked();
    void apply();
    void updateButtons();
    void updateFormats();
    void updateFilenameLabel();

  Q_SIGNALS:
    void exportGraphics(const QString &filename, const QString &format, int w, int h, int display, bool export_all, int autosave_period);

  private:
};

}

#endif

// vim: ts=2 sw=2 et
