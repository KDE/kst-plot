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

#ifndef OVERRIDELABELTAB_H
#define OVERRIDELABELTAB_H

#include "dialogtab.h"
#include "ui_overridelabeltab.h"

#include "kstcore_export.h"

namespace Kst {

class OverrideLabelTab : public DialogTab, Ui::OverrideLabelTab {
  Q_OBJECT
  public:
    explicit OverrideLabelTab(QString title, QWidget *parent = 0);
    virtual ~OverrideLabelTab();

    QFont labelFont(QFont ref_font) const;
    bool labelFontDirty() const;
    void setLabelFont(const QFont &font);

    qreal labelFontScale() const;
    bool labelFontScaleDirty() const;
    void setLabelFontScale(const qreal scale);

    QColor labelColor() const;
    bool labelColorDirty() const;
    void setLabelColor(const QColor &color);

    bool useDefault() const;
    bool useDefaultDirty() const;
    void setUseDefault(const bool useDefault);

    void setFontSpecsIfDefault(const QFont &font, const qreal scale, const QColor &color);

    void enableSingleEditOptions(bool enabled);
    void clearTabValues();

  Q_SIGNALS:
    void useDefaultChanged(bool);

  private Q_SLOTS:
    void buttonUpdate();

  private:
    bool _fontDirty;

};

}

#endif

// vim: ts=2 sw=2 et
