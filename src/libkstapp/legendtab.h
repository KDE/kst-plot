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

#ifndef LEGENDTAB_H
#define LEGENDTAB_H

#include "dialogtab.h"
#include "ui_legendtab.h"

#include "kstcore_export.h"

namespace Kst {

class ObjectStore;

class LegendTab : public DialogTab, Ui::LegendTab {
  Q_OBJECT
  public:
    explicit LegendTab(QWidget *parent = 0);
    virtual ~LegendTab();

    void setDisplayedRelations(const QStringList& displayedRelations, const QStringList& displayedRelationTips);
    void setAvailableRelations(const QStringList& availableRelations, const QStringList& availableRelationTips);

    QStringList displayedRelations();

    QString title() const;
    void setTitle(const QString& title);
    bool titleDirty() const;

    bool autoContents() const;
    void setAutoContents(const bool value);
    bool autoContentsDirty() const;

    QFont font(const QFont font) const;
    void setFont(const QFont &font);
    bool fontDirty() const;

    QColor legendColor() const {return _labelColor->color();}
    void setColor(const QColor c) {_labelColor->setColor(c);}

    qreal fontScale() const;
    void setFontScale(const qreal scale);
    bool fontScaleDirty() const;

    bool verticalDisplay() const;
    void setVerticalDisplay(const bool vertical);
    bool verticalDisplayDirty() const;

    void clearTabValues();
    void setSingle(bool single);
  private:
    ObjectStore* _store;
    bool _single;

  private Q_SLOTS:
    void updateActive();
    void updateButtons();
    void addButtonClicked();
    void removeButtonClicked();
    void upButtonClicked();
    void downButtonClicked();
};

}

#endif

// vim: ts=2 sw=2 et
