/***************************************************************************
                          datastscalar.h  -  a scalar from a data source
                             -------------------
    begin                : September, 2008
    copyright            : (C) 2008 by cbn
    email                : netterfield@astro.utoronto.ca
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef DATASCALAR_H
#define DATASCALAR_H

#include "kstcore_export.h"
#include "dataprimitive.h"
#include "scalar.h"

class QXmlStreamWriter;

namespace Kst {

/**A class for handling data scalars for kst.
 *@author cbn
 */

/** A scalar which gets its value from a data file. */
class KSTCORE_EXPORT DataScalar : public Scalar, public DataPrimitive
{
  Q_OBJECT

  protected:
    DataScalar(ObjectStore *store);
    friend class ObjectStore;

    virtual QString _automaticDescriptiveName() const;

    /** Update the scalar.*/
    virtual qint64 minInputSerial() const;
    virtual qint64 maxInputSerialOfLastChange() const;


  public:
    virtual ~DataScalar();

    struct ReadInfo {
      ReadInfo(double* d) : value(d) {}
      double* value;
    };

    struct DataInfo {
    };

    virtual void internalUpdate();
    virtual QString typeString() const;
    static const QString staticTypeString;
    static const QString staticTypeTag;

    /** change the properties of a DataScalar */
    void change(DataSourcePtr file, const QString &field);
    void changeFile(DataSourcePtr file);

    /** Save scalar information */
    virtual void save(QXmlStreamWriter &s);

    virtual QString descriptionTip() const;

    virtual QString propertyString() const;
    bool isValid() const;

    virtual void reset();
    void reload();

    virtual ScriptInterface* createScriptInterface();

  private:
    /** make a copy of the DataScalar */
    virtual PrimitivePtr makeDuplicate() const;
    virtual bool checkValidity(const DataSourcePtr& ds) const;
};

typedef SharedPtr<DataScalar> DataScalarPtr;
typedef ObjectList<DataScalar> DataScalarList;

}

#endif
// vim: ts=2 sw=2 et
