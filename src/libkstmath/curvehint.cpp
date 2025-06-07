/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2004 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
*                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "curvehint.h"
#include "datacollection.h"
#include "relation.h"
#include "curve.h"

#include <qdebug.h>

namespace Kst {

CurveHint::CurveHint(const QString& name, const QString& x, const QString& y)
: Shared(),  _curveName(name), _xVectorName(x), _yVectorName(y) {
}


CurveHint::~CurveHint() {
}


const QString& CurveHint::xVectorName() const {
  return _xVectorName;
}


const QString& CurveHint::yVectorName() const {
  return _yVectorName;
}


#if 0
VectorPtr CurveHint::xVector() const {
  return *vectorList.findTag(_xVectorName);
}


VectorPtr CurveHint::yVector() const {
  return *vectorList.findTag(_yVectorName);
}


RelationPtr CurveHint::makeCurve(const QString& tag, const QColor& color) const {
  VectorPtr x = xVector();
  VectorPtr y = yVector();
  if (!x || !y) {
    qDebug() << "Couldn't find either " << _xVectorName << " or " << _yVectorName << Qt::endl;
    return 0L;
  }

  return new Curve(tag, x, y, 0L, 0L, 0L, 0L, color);
}
#endif

}

// vim: ts=2 sw=2 et
