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
#ifndef EDITABLEMATRIX_H
#define EDITABLEMATRIX_H

#include "matrix.h"
#include "kstcore_export.h"

namespace Kst {

class KSTCORE_EXPORT EditableMatrix : public Matrix {
  Q_OBJECT

  public:
    virtual QString typeString() const;
    static const QString staticTypeString;
    static const QString staticTypeTag;

    virtual void save(QXmlStreamWriter &xml);

    virtual QString descriptionTip() const;

    void loadFromTmpFile(QFile &fp, int nx, int ny);

  protected:
    EditableMatrix(ObjectStore *store);

    friend class ObjectStore;

    virtual QString _automaticDescriptiveName() const;

    ScriptInterface* createScriptInterface();
};

typedef SharedPtr<EditableMatrix> EditableMatrixPtr;
typedef ObjectList<EditableMatrix> EditableMatrixList;

}

#endif
// vim: ts=2 sw=2 et
