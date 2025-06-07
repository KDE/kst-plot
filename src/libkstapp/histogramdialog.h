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

#ifndef HISTOGRAMDIALOG_H
#define HISTOGRAMDIALOG_H

#include "datadialog.h"
#include "datatab.h"

#include "histogram.h"

#include "ui_histogramtab.h"

#include <QPointer>

#include "kstcore_export.h"

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace Kst {

class HistogramTab : public DataTab, Ui::HistogramTab {
  Q_OBJECT
  public:
    explicit HistogramTab(QWidget *parent = 0);
    virtual ~HistogramTab();

    void setObjectStore(ObjectStore *store);

    VectorPtr vector() const;
    bool vectorDirty() const;
    void setVector(VectorPtr vector);
    bool vectorSelected() const {return _vector->vectorSelected();}

    double min() const;
    bool minDirty() const;
    void setMin(const double min);

    double max() const;
    bool maxDirty() const;
    void setMax(const double max);

    int bins() const;
    bool binsDirty() const;
    void setBins(const int bins);

    bool realTimeAutoBin() const;
    bool realTimeAutoBinDirty() const;
    void setRealTimeAutoBin(const bool autoBin);

    Histogram::NormalizationType normalizationType() const;
    bool normalizationTypeDirty() const;
    void setNormalizationType(const Histogram::NormalizationType normalizationType);

    CurveAppearance* curveAppearance() const;
    CurvePlacement* curvePlacement() const;

    void hideCurveOptions();
    void clearTabValues();

    void resetNormalizationDirty();

  private Q_SLOTS:
    void generateAutoBin();
    void updateButtons();
    void selectionChanged();
    void normalizationChanged();

  Q_SIGNALS:
    void vectorChanged();

  private:
    bool _normalizationDirty;
};

class HistogramDialog : public DataDialog {
  Q_OBJECT
  public:
    explicit HistogramDialog(ObjectPtr dataObject, QWidget *parent = 0);
    virtual ~HistogramDialog();

    void setVector(VectorPtr vector);

    virtual bool dialogValid() const;

  protected:
//     virtual QString tagString() const;
    virtual ObjectPtr createNewDataObject();
    virtual ObjectPtr editExistingDataObject() const;

  private Q_SLOTS:
    void updateButtons();
    void editMultipleMode();
    void editSingleMode();

  private:
    void configureTab(ObjectPtr object=0);

    HistogramTab *_histogramTab;
};

}

#endif

// vim: ts=2 sw=2 et
