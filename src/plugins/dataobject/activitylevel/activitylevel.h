/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2013 The Kst Team                                     *
 *                   kst@kde.org                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ACTIVITYLEVELPLUGIN_H
#define ACTIVITYLEVELPLUGIN_H

#include <QFile>

#include <basicplugin.h>
#include <dataobjectplugin.h>

class ActivityLevelSource : public Kst::BasicPlugin {
  Q_OBJECT

  public:
  virtual QString _automaticDescriptiveName() const;
  virtual QString descriptionTip() const;

    Kst::VectorPtr vector() const;
    Kst::ScalarPtr samplingTime() const;
    Kst::ScalarPtr windowWidth() const;
    Kst::ScalarPtr noiseThreshold() const;

    virtual void change(Kst::DataObjectConfigWidget *configWidget);

    void setupOutputs();
    virtual bool algorithm();

    virtual QStringList inputVectorList() const;
    virtual QStringList inputScalarList() const;
    virtual QStringList inputStringList() const;
    virtual QStringList outputVectorList() const;
    virtual QStringList outputScalarList() const;
    virtual QStringList outputStringList() const;

    virtual void saveProperties(QXmlStreamWriter &s);

  protected:
    ActivityLevelSource(Kst::ObjectStore *store);
    ~ActivityLevelSource();

  friend class Kst::ObjectStore;


};


class ActivityLevelPlugin : public QObject, public Kst::DataObjectPluginInterface {
    Q_OBJECT
    Q_INTERFACES(Kst::DataObjectPluginInterface)
    Q_PLUGIN_METADATA(IID "com.kst.DataObjectPluginInterface/2.0")
  public:
    virtual ~ActivityLevelPlugin() {}

    virtual QString pluginName() const;
    virtual QString pluginDescription() const;

    virtual DataObjectPluginInterface::PluginTypeID pluginType() const { return Generic; }

    virtual bool hasConfigWidget() const { return true; }

    virtual Kst::DataObject *create(Kst::ObjectStore *store, Kst::DataObjectConfigWidget *configWidget, bool setupInputsOutputs = true) const;

    virtual Kst::DataObjectConfigWidget *configWidget(QSettings *settingsObject) const;
};

#endif
// vim: ts=2 sw=2 et
