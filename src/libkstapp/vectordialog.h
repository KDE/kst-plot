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

#ifndef VECTORDIALOG_H
#define VECTORDIALOG_H

#include "datadialog.h"
#include "datatab.h"

#include "ui_vectortab.h"

#include <QPointer>

#include "kstcore_export.h"

#include "datasource.h"

namespace Kst {

class ObjectStore;

class VectorTab : public DataTab, Ui::VectorTab {
  Q_OBJECT

  public:
    friend class DialogSI;
    enum VectorMode { DataVector, GeneratedVector };

    explicit VectorTab(ObjectStore *store, QWidget *parent = 0);
    virtual ~VectorTab();

    VectorMode vectorMode() const { return _mode; }
    void setVectorMode(VectorMode mode);

    //DataVector mode methods...
    DataSourcePtr dataSource() const;
    void setDataSource(DataSourcePtr dataSource);

    QString file() const;
    void setFile(const QString &file);

    QString field() const;
    void setField(const QString &field);

    void setFieldList(const QStringList &fieldList);

    DataRange *dataRange() const;

    //GeneratedVector methods...
    qreal from() const;
    void setFrom(qreal from);
    bool fromDirty() const;

    qreal to() const;
    void setTo(qreal to);
    bool toDirty() const;

    int numberOfSamples() const;
    void setNumberOfSamples(int numberOfSamples);
    bool numberOfSamplesDirty() const;

    void hideGeneratedOptions();
    void hideDataOptions();
    void enableSingleEditOptions(bool enabled);
    void clearTabValues();

    bool validating;

    void updateIndexList(DataSourcePtr dataSource);

  Q_SIGNALS:
    void sourceChanged();
    void fieldChanged();

  private Q_SLOTS:
    void readFromSourceClicked();
    void generateClicked();
    void fileNameChanged(const QString &file);
    void showConfigWidget();
    void sourceValid(QString filename, int requestID);
    void updateTypeActivated(int);
    void clearIndexList();

  private:
    VectorMode _mode;
    ObjectStore *_store;
    DataSourcePtr _dataSource;
    QString _initField;
    int _requestID;
    void updateUpdateBox();
    bool _valid;
};

class VectorDialog : public DataDialog {
  Q_OBJECT
  public:
    friend class DialogSI;
    explicit VectorDialog(ObjectPtr dataObject, QWidget *parent = 0);
    virtual ~VectorDialog();

    void setField(QString field) {_vectorTab->setField(field);}
    virtual void waitForValidation();

  protected:
//     virtual QString tagString() const;
    virtual ObjectPtr createNewDataObject();
    virtual ObjectPtr editExistingDataObject() const;

  private:
    ObjectPtr createNewDataVector();
    ObjectPtr createNewGeneratedVector();
    void configureTab(ObjectPtr vector=0);

  private Q_SLOTS:
    void updateButtons();
    void editMultipleMode();
    void editSingleMode();

  private:
    VectorTab *_vectorTab;

};

}

#endif

// vim: ts=2 sw=2 et
