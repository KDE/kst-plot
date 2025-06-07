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

#ifndef CONTENTTAB_H
#define CONTENTTAB_H

#include "dialogtab.h"
#include "ui_contenttab.h"

#include "kstcore_export.h"

namespace Kst {

class ObjectStore;

class ContentTab : public DialogTab, Ui::ContentTab {
  Q_OBJECT
  public:
    ContentTab(QWidget *parent, ObjectStore* store);
    virtual ~ContentTab();

    void setDisplayedRelations(QStringList displayedRelations, QStringList displayedRelationTips);
    void setAvailableRelations(QStringList availableRelations, QStringList availableRelationTips);

    QStringList displayedRelations();

  private:
    ObjectStore* _store;

  private Q_SLOTS:
    void updateButtons();
    void addButtonClicked();
    void removeButtonClicked();
    void upButtonClicked();
    void downButtonClicked();
    void availableDoubleClicked(QListWidgetItem * item);
    void displayedDoubleClicked(QListWidgetItem * item);
    void editSelectedAvailable();
    void editSelectedDisplayed();
    void addObject(QString);
    void removeObject(QString);
    void filterCurves(const QString&);
    void searchCurves();

};

}

#endif

// vim: ts=2 sw=2 et
