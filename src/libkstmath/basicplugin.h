/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2006 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef BASICPLUGIN_H
#define BASICPLUGIN_H

#include "dataobject.h"
#include "kstmath_export.h"

namespace Kst {

class ObjectStore;

class KSTMATH_EXPORT BasicPlugin : public DataObject {
  Q_OBJECT

  public:
    static const QString staticTypeString;
    QString typeString() const { return staticTypeString; }
    static const QString staticTypeTag;

    //The implementation of the algorithm the plugin provides.
    //Operates on the inputVectors, inputScalars, and inputStrings
    //to produce the outputVectors, outputScalars, and outputStrings.
    virtual bool algorithm() = 0;

    //String lists of the names of the expected inputs.
    virtual QStringList inputVectorList() const = 0;
    virtual QStringList inputScalarList() const = 0;
    virtual QStringList inputStringList() const = 0;
    //String lists of the names of the expected outputs.
    virtual QStringList outputVectorList() const = 0;
    virtual QStringList outputScalarList() const = 0;
    virtual QStringList outputStringList() const = 0;

    //Pure virtual methods inherited from DataObject
    //This _must_ equal the 'Name' entry in the .desktop file of
    //the plugin
    QString propertyString() const { return name(); } //no longer virtual

    //Provide an impl...
    virtual DataObjectPtr makeDuplicate() const;

    virtual QString descriptionTip() const;

    // Validator of plugin data.  Expensive, only use to verify successful creation.
    virtual bool isValid() { return (inputsExist() && algorithm()); }
    QString errorMessage() { return _errorString; }

  public slots:
    //Pure virtual slots from DataObject
    //Each plugin can provide an implementation or use the default
    virtual void showNewDialog();
    virtual void showEditDialog();

  public:
    virtual void change(DataObjectConfigWidget *configWidget) = 0;

    //Returns the respective input object for name
    VectorPtr inputVector(const QString& name) const;
    ScalarPtr inputScalar(const QString& name) const;
    StringPtr inputString(const QString& name) const;

    void setOutputVector(const QString &type, const QString &name);
    void setOutputScalar(const QString &type, const QString &name);
    void setOutputString(const QString &type, const QString &name);

    // for setting non primitive properties
    virtual void setProperty(const QString &key, const QString &val) {Q_UNUSED(key) Q_UNUSED(val);}

    void setPluginName(const QString &pluginName);
    QString pluginName() { return _pluginName; }

    //Regular virtual methods from DataObject
    virtual void save(QXmlStreamWriter &s);
    virtual void saveProperties(QXmlStreamWriter &s);

    void createScalars();
    QString label(int precision) const;

    virtual void internalUpdate();
    virtual bool hasParameterVector() const { return _outputVectors.contains("Parameters Vector");}
    virtual QString parameterVectorToString() const { return label(9);}

    virtual ScriptInterface* createScriptInterface();

  protected:
    BasicPlugin(ObjectStore *store);
    virtual ~BasicPlugin();

    //Pure virtual methods inherited from DataObject
    //We do this one ourselves for benefit of all plugins...

    virtual QString parameterName(int index) const;
    QString _errorString;
    virtual void _initializeShortName();
  private:
    bool inputsExist() const;
    void updateOutput() const;

    QString _pluginName;
};

typedef SharedPtr<BasicPlugin> BasicPluginPtr;
typedef ObjectList<BasicPlugin> BasicPluginList;

}

#endif

// vim: ts=2 sw=2 et
