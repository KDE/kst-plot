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

#ifndef IMAGEDIALOG_H
#define IMAGEDIALOG_H

#include "datadialog.h"
#include "datatab.h"

#include "image.h"

#include "ui_imagetab.h"

#include <QPointer>

#include "kstcore_export.h"

namespace Kst {

  class ObjectStore;

class ImageTab : public DataTab, Ui::ImageTab {
  Q_OBJECT
  public:
    explicit ImageTab(QWidget *parent = 0);
    virtual ~ImageTab();

    CurvePlacement* curvePlacement() const;
    ColorPalette* colorPalette() const;

    void setObjectStore(ObjectStore *store);

    bool realTimeAutoThreshold() const;
    bool realTimeAutoThresholdDirty() const;
    void setRealTimeAutoThreshold(const bool realTimeAutoThreshold);

    bool colorOnly() const;
    void setColorOnly(const bool colorOnly);

    bool contourOnly() const;
    void setContourOnly(const bool contourOnly);

    bool colorAndContour() const;
    void setColorAndContour(const bool colorAndContour);

    bool modeDirty() const;

    double lowerThreshold() const;
    bool lowerThresholdDirty() const;
    void setLowerThreshold(const double lowerThreshold);

    double upperThreshold() const;
    bool upperThresholdDirty() const;
    void setUpperThreshold(const double upperThreshold);

    int numberOfContourLines() const;
    bool numberOfContourLinesDirty() const;
    void setNumberOfContourLines(const int numberOfContourLines);

    int contourWeight() const;
    bool contourWeightDirty() const;
    void setContourWeight(const int contourWeight);

    QColor contourColor() const;
    bool contourColorDirty() const;
    void setContourColor(const QColor contourColor);

    MatrixPtr matrix() const;
    bool matrixDirty() const;
    void setMatrix(const MatrixPtr matrix);

    bool useVariableLineWeight() const;
    bool useVariableLineWeightDirty() const;
    void setUseVariableLineWeight(const bool useVariableLineWeight);

    void hidePlacementOptions();
    void clearTabValues();

    void resetModeDirty();

  private Q_SLOTS:
    void realTimeAutoThresholdToggled(const bool checked);
    void updateEnabled(const bool checked);
    void selectionChanged();
    void calculateAutoThreshold();
    void calculateSmartThreshold();
    void modeChanged();

  Q_SIGNALS:
    void optionsChanged();

  private:
    ObjectStore* _store;

    bool _modeDirty;
};

class ImageDialog : public DataDialog {
  Q_OBJECT
  public:
    explicit ImageDialog(ObjectPtr dataObject, QWidget *parent = 0);
    virtual ~ImageDialog();

    void setMatrix(MatrixPtr matrix);

    virtual bool dialogValid() const {return bool(_imageTab->matrix()) || (editMode() == EditMultiple);}

  protected:
    virtual ObjectPtr createNewDataObject();
    virtual ObjectPtr editExistingDataObject() const;

  private Q_SLOTS:
    void updateButtons();
    void editMultipleMode();
    void editSingleMode();

  private:
    void configureTab(ObjectPtr object);

    ImageTab *_imageTab;
};

}

#endif
