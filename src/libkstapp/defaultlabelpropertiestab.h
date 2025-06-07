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

#ifndef DEFAULTLABELPROPERTIESTAB_H
#define DEFAULTLABELPROPERTIESTAB_H

#include "dialogtab.h"
#include "ui_defaultlabelpropertiestab.h"

#include "kstcore_export.h"

namespace Kst {

class DefaultLabelPropertiesTab : public DialogTab, Ui_DefaultLabelPropertiesTab {
  Q_OBJECT
  public:
    explicit DefaultLabelPropertiesTab(QWidget *parent = 0);
    virtual ~DefaultLabelPropertiesTab();

    double referenceViewWidth() const;
    void setReferenceViewWidth(const double width);

    double referenceViewHeight() const;
    void setReferenceViewHeight(const double height);

    int minimumFontSize() const;
    void setMinimumFontSize(const int points);

    void checkSizeDefaults();

  public Q_SLOTS:
    void referenceViewSizeComboChanged(int i);
};

extern const double A4Width; //  A4 with a 1.5 cm margin;
extern const double A4Height; // A4 with a 1.5 cm margin;

}

#endif

// vim: ts=2 sw=2 et
