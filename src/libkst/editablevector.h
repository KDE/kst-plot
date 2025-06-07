/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2007 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
 *   copyright : (C) 2005  University of British Columbia                  *
 *                   dscott@phas.ubc.ca                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef EDITABLEVECTOR_H
#define EDITABLEVECTOR_H

#include "vector.h"
#include "kstcore_export.h"

//#include <QFile>

/**A vector  with n editable pts
 *@author cbn
 */

namespace Kst {

class KSTCORE_EXPORT EditableVector : public Vector {
  Q_OBJECT

  public:
    virtual QString typeString() const;
    static const QString staticTypeString;
    static const QString staticTypeTag;

    void save(QXmlStreamWriter &s);

    void setSaveData(bool save);

    /** If value exceeds length, vector is resized
      * @sa Vector::change()
      */
    void setValue(const int& i,const double&val);

    virtual QString descriptionTip() const;

    void loadFromTmpFile(QFile &fp);

    ScriptInterface* createScriptInterface();

  protected:
    long double _sum;
    EditableVector(ObjectStore *store);

    friend class ObjectStore; 

    virtual QString _automaticDescriptiveName() const;
};

typedef SharedPtr<EditableVector> EditableVectorPtr;
typedef ObjectList<EditableVector> EditableVectorList;

}

#endif
// vim: ts=2 sw=2 et
