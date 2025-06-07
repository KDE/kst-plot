/***************************************************************************
                          vcurve.h: defines a curve for kst
                             -------------------
    begin                : Fri Oct 22 2000
    copyright            : (C) 2000 by C. Barth Netterfield
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

#ifndef CURVE_H
#define CURVE_H

#include "relation.h"
#include "painter.h"
#include "curvepointsymbol.h"
#include "kstmath_export.h"
#include "labelinfo.h"

#include <QStack>

/**A class for handling curves for kst
 *@author C. Barth Netterfield
 */

#define CURVE_DEFAULT_POINT_SIZE 12

namespace Kst {

class KSTMATH_EXPORT Curve: public Relation 
{
    Q_OBJECT

  public:
    static const QString staticTypeString;
    QString typeString() const { return staticTypeString; }
    static const QString staticTypeTag;

    virtual void internalUpdate();
    virtual QString propertyString() const;

    virtual int getIndexNearXY(double x, double dx, double y) const;

    virtual bool hasXError() const;
    virtual bool hasYError() const;
    virtual bool hasXMinusError() const;
    virtual bool hasYMinusError() const;

    // Note: these are -expensive-.  Don't use them on inner loops!
    virtual void point(int i, double &x1, double &y1) const;
    virtual void getEXPoint(int i, double &x1, double &y1, double &ex1);
    virtual void getEYPoint(int i, double &x1, double &y1, double &ey1);
    virtual void getEXMinusPoint(int i, double &x1, double &y1, double &ex1);
    virtual void getEYMinusPoint(int i, double &x1, double &y1, double &ey1);
    virtual void getEXPoints(int i, double &x, double &y, double &ex, double &exminus);
    virtual void getEYPoints(int i, double &x, double &y, double &ey, double &eyminus);

    void setXVector(VectorPtr new_vx);
    void setYVector(VectorPtr new_vy);
    void setXError(VectorPtr new_ex);
    void setYError(VectorPtr new_ey);
    void setXMinusError(VectorPtr new_ex);
    void setYMinusError(VectorPtr new_ey);

    /** Save curve information */
    void save(QXmlStreamWriter &s);

    virtual bool xIsRising() const;

    virtual double maxX() const;
    virtual double minX() const;

    virtual double meanX() const { return MeanX; }
    virtual double meanY() const { return MeanY; }
    virtual void yRange(double xFrom, double xTo, double* yMin, double* yMax);

    virtual double ns_maxX(int) const;
    virtual double ns_minX(int) const;
    virtual double ns_maxY(int) const;
    virtual double ns_minY(int) const;

    virtual int samplesPerFrame() const;

    virtual void showNewDialog();
    virtual void showEditDialog();

    VectorPtr xVector() const;
    VectorPtr yVector() const;
    VectorPtr xErrorVector() const;
    VectorPtr yErrorVector() const;
    VectorPtr xMinusErrorVector() const;
    VectorPtr yMinusErrorVector() const;

    virtual bool hasPoints()    const { return HasPoints; }
    virtual bool hasLines()     const { return HasLines; }
    virtual bool hasBars()      const { return HasBars; }
    virtual bool hasHead()      const { return HasHead; }
    virtual void setHasPoints(bool in_HasPoints);
    virtual void setHasLines(bool in_HasLines);
    virtual void setHasBars(bool in_HasBars);
    virtual void setHasHead(bool in_HasHead);
    virtual void setLineWidth(int in_LineWidth);
    virtual void setLineStyle(int in_LineStyle);
    virtual void setPointDensity(int in_PointDensity);
    virtual void setPointType(int in_PointType);
    virtual void setPointSize(double in_PointSize);
    virtual void setHeadType(int in_HeadType);

    virtual int lineWidth()     const { return LineWidth; }
    static double lineDim(const QRectF &R, double linewidth);
    virtual int lineStyle()     const { return LineStyle; }
    virtual int pointDensity()  const { return PointDensity; }
    virtual int pointType()  const { return PointType; }
    virtual int headType()  const { return HeadType; }
    virtual double pointSize() const { return PointSize; }
    virtual double pointDim(QRectF w) const;

    virtual QColor color() const { return Color; }
    virtual void setColor(const QColor& new_c);

    virtual QColor barFillColor() const { return BarFillColor; }
    virtual void setBarFillColor(const QColor& new_c);

    virtual QColor headColor() const { return HeadColor; }
    virtual void setHeadColor(const QColor& new_c);

#if 0
    void pushColor(const QColor& c) { _colorStack.push(color()); setColor(c); }
    void popColor() { setColor(_colorStack.pop()); }
    void pushLineWidth(int w) { _widthStack.push(lineWidth()); setLineWidth(w); }
    void popLineWidth() { setLineWidth(_widthStack.pop()); }
    void pushLineStyle(int s) { _lineStyleStack.push(lineStyle()); setLineStyle(s); }
    void popLineStyle() { setLineStyle(_lineStyleStack.pop()); }
    void pushPointStyle(int s) { _pointStyleStack.push(PointType); PointType = s; }
    void popPointStyle() { PointType = _pointStyleStack.pop(); }
    void pushHasPoints(bool h) { _hasPointsStack.push(hasPoints()); setHasPoints(h); }
    void popHasPoints() { setHasPoints(_hasPointsStack.pop()); }
    void pushHasLines(bool h) { _hasLinesStack.push(hasLines()); setHasLines(h); }
    void popHasLines() { setHasLines(_hasLinesStack.pop()); }
    void pushPointDensity(int d) { _pointDensityStack.push(pointDensity()); setPointDensity(d); }
    void popPointDensity() { setPointDensity(_pointDensityStack.pop()); }
#endif

    //virtual RelationPtr makeDuplicate(QMap<RelationPtr, RelationPtr> &duplicatedRelations);
    virtual RelationPtr makeDuplicate() const;

    // render this curve
    virtual void paintObjects(const CurveRenderContext& context);

    // Update the curve details.
    void updatePaintObjects(const CurveRenderContext& context);

    // render the legend symbol for this curve
    virtual QSize legendSymbolSize(QPainter *p);
    virtual void paintLegendSymbol(QPainter *p, const QSize &size);
    virtual bool symbolLabelOnTop() {return false;}

    // see KstRelation::distanceToPoint
    virtual double distanceToPoint(double xpos, double dx, double ypos) const;

    virtual QString descriptionTip() const;

    // labels for plots
    virtual LabelInfo xLabelInfo() const;
    virtual LabelInfo yLabelInfo() const;
    virtual LabelInfo titleInfo() const;

    virtual ScriptInterface* createScriptInterface();

  protected:
    Curve(ObjectStore *store);

    virtual ~Curve();

    friend class ObjectStore;

    virtual QString _automaticDescriptiveName() const;
    virtual void _initializeShortName();

  private:
    double MeanY;

    int LineWidth;
    int LineStyle;
    int PointDensity;
    int PointType;
    double PointSize;
    int HeadType;

    bool HasPoints;
    bool HasLines;
    bool HasBars;
    bool HasHead;

    QColor Color;
    QColor HeadColor;
    QColor BarFillColor;

#if 0
    QStack<int> _widthStack;
    QStack<QColor> _colorStack;
    QStack<int> _pointStyleStack;
    QStack<int> _lineStyleStack;
    QStack<bool> _hasPointsStack;
    QStack<bool> _hasLinesStack;
    QStack<int> _pointDensityStack;
#endif

    QVector<QPolygonF> _polygons;
    QVector<QLineF> _lines;
    QVector<QPointF> _points;
    QVector<QRectF> _filledRects;
    QVector<QRectF> _rects;
    QPointF _head;
    bool _head_valid;

    int _width;
};

typedef SharedPtr<Curve> CurvePtr;
typedef ObjectList<Curve> CurveList;

}

Q_DECLARE_METATYPE(Kst::Curve*)

#endif
// vim: ts=2 sw=2 et
