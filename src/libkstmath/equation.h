/***************************************************************************
                          equation.h: Equation for KST
                             -------------------
    begin                : Fri Feb 10 2002
    copyright            : (C) 2002 by C. Barth Netterfield
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

/** A class for handling equations for kst
 *@author C. Barth Netterfield
 */
#ifndef EQUATION_H
#define EQUATION_H

#include "dataobject.h"
#include "objectfactory.h"
#include "kstmath_export.h"

#define MAX_DIV_REG 100

class QXmlStreamWriter;

namespace Equations {
  class Node;
}

namespace Kst {

class ObjectStore;

class KSTMATH_EXPORT Equation : public DataObject {
  Q_OBJECT

  public:
    static const QString staticTypeString;
    QString typeString() const { return staticTypeString; }
    static const QString staticTypeTag;

    void attach();
    void save(QXmlStreamWriter &s);
    QString propertyString() const;

    /** equations used to edit the vector */
    void setEquation(const QString &Equation);
    void setExistingXVector(VectorPtr xvector, bool do_interp);

    const QString& equation() const { return _equation; }
    const QString reparsedEquation() const;
    void updateVectorLabels();

    VectorPtr vXIn() const { return _xInVector; }
    VectorPtr vX() const { return _xOutVector; }
    VectorPtr vY() const { return _yOutVector; }

    bool doInterp() const { return _doInterp; }
    void setDoInterp(bool interp) {_doInterp = interp;}

    bool isValid();

    void showNewDialog();
    void showEditDialog();

    const CurveHintList *curveHints() const;

    DataObjectPtr makeDuplicate() const;

    bool uses(ObjectPtr p) const;

    virtual QString descriptionTip() const;
    virtual void internalUpdate();

    virtual PrimitiveList inputPrimitives() const;
    virtual void replaceInput(PrimitivePtr p, PrimitivePtr new_p);

    virtual ScriptInterface* createScriptInterface();

  protected:
    Equation(ObjectStore *store);
    ~Equation();

    friend class ObjectStore;

    virtual QString _automaticDescriptiveName() const;
    virtual void _initializeShortName();

    virtual qint64 minInputSerial() const;
    virtual qint64 maxInputSerialOfLastChange() const;

    QString readableEquation(const QString &equation) const;
    QByteArray parseableEquation() const;

  private:
    QString _equation;

    VectorMap VectorsUsed;
    ScalarMap ScalarsUsed;

    bool FillY(bool force = false);
    bool _isValid : 1;
    bool _doInterp : 1;

    int _numNew, _numShifted, _interp, _ns;

    VectorPtr _xInVector, _xOutVector, _yOutVector;
    Equations::Node *_pe;
};

typedef SharedPtr<Equation> EquationPtr;
typedef ObjectList<Equation> EquationList;

}
#endif
// vim: ts=2 sw=2 et
