/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2008 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LEGENDITEMDIALOG_H
#define LEGENDITEMDIALOG_H

#include "viewitemdialog.h"

#include "kstcore_export.h"

namespace Kst {

class LegendTab;
class LegendItem;
class ObjectStore;

class LegendItemDialog : public ViewItemDialog
{
  Q_OBJECT
  public:
    explicit LegendItemDialog(LegendItem *item, QWidget *parent = 0);
    virtual ~LegendItemDialog();

  private Q_SLOTS:
    void legendChanged();
    void editMultiple();
    void editSingle();
    void slotApply();

  private:
    void setupLegend();
    void saveLegend(LegendItem *item, bool save_relations);

  private:
    LegendItem* _legendItem;

    LegendTab *_legendTab;

    ObjectStore* _store;
};

}

#endif

// vim: ts=2 sw=2 et
