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

#ifndef FILTERFITDIALOG_H
#define FILTERFITDIALOG_H

#include "datadialog.h"
#include "datatab.h"
#include "dataobjectplugin.h"
#include "plotitem.h"

#include "kstcore_export.h"

#include "ui_filterfittab.h"

namespace Kst {

class FilterFitTab : public DataTab, Ui::FilterFitTab {
  Q_OBJECT
  public:
    explicit FilterFitTab(QString& pluginName, QWidget *parent = 0);
    virtual ~FilterFitTab();

    void setObjectStore(ObjectStore *store);
    void setVectorX(VectorPtr vector);
    void setVectorY(VectorPtr vector);

    CurveAppearance* curveAppearance() const;
    CurvePlacement* curvePlacement() const;

    DataObjectConfigWidget* configWidget() { return _configWidget; }

    void loadSettings();
    void saveSettings();

    void setPlotMode(PlotItem* plot);
    void lockVectors();

    QString pluginName();

  public Q_SLOTS:
    void pluginChanged(const QString&);

  private:
    DataObjectConfigWidget* _configWidget;
    QGridLayout* _layout;
    DataObjectPluginInterface::PluginTypeID _type;
    ObjectStore *_store;
    VectorPtr _vectorX, _vectorY;
    bool _lockVectors;

};

class FilterFitDialog : public DataDialog {
  Q_OBJECT
  public:
    FilterFitDialog(QString& pluginName, ObjectPtr dataObject, QWidget *parent = 0);
    virtual ~FilterFitDialog();

    void setVectorX(VectorPtr vector);
    void setVectorY(VectorPtr vector);
    void setPlotMode(PlotItem* plot);

  protected:
    virtual QString tagString() const;
    virtual ObjectPtr createNewDataObject();
    virtual ObjectPtr editExistingDataObject() const;

  private:
    void configureTab();
    FilterFitTab *_filterFitTab;
    VectorPtr _vectorX;
    VectorPtr _vectorY;
};

}

#endif

// vim: ts=2 sw=2 et
