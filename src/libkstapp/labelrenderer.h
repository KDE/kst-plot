/***************************************************************************
                              labelrenderer.h
                             ------------------
    begin                : Jun 17 2005
    copyright            : (C) 2005 by The University of Toronto
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

#ifndef LABELRENDERER_H
#define LABELRENDERER_H

#include <qfont.h>
#include <qpainter.h>
#include <qpair.h>
#include <qstring.h>
#include <qvariant.h>
#ifndef KST_NO_PRINTER
#include <QPrinter>
#endif

#include "vector.h"
#include "scalar.h"
#include "string.h"
#include "kst_export.h"

namespace Label {

struct RenderedText {
  QPointF location;
  QString text;
  QFont font;
  QPen pen;
};

// inline for speed.
class RenderContext : public QObject {
  Q_OBJECT

  public:
  RenderContext(const QFont& font, QPainter *p)
  : QObject(), p(p), _fm(_font) {
    x = y = xMax = xStart = 0;
    precision = 8;
    setFont(font);
    lines = 0;
  }

  inline void addToCache(QPointF location, QString &text, QFont &font, QPen &pen) {
    RenderedText cacheEntry;
    cacheEntry.location = location;
    cacheEntry.text = text;
    cacheEntry.font = font;
    cacheEntry.pen = pen;
    cachedText.append(cacheEntry);
  }

  inline const QFont& font() const {
    if (p) {
      return p->font();
    } else {
      return _font;
    }
  }

  inline void setFont(const QFont& f_in) {
    QFont f = f_in;
    _fontSize = f.pointSize();

    if (p) {
      p->setFont(f);
      _ascent = p->fontMetrics().ascent();
      _descent = p->fontMetrics().descent();
      _height = p->fontMetrics().height();
      _lineSpacing = p->fontMetrics().lineSpacing();
    } else {
      _font = f;
      _fm = QFontMetrics(_font);
      _ascent = _fm.ascent();
      _lineSpacing = _fm.lineSpacing();
      _descent = _fm.descent();
      _height = _fm.height();
    }
  }

  inline void addObject(Kst::VectorPtr vp) {
    _refObjects.append(vp);
    connect(vp.data(), SIGNAL(dirty()), this, SIGNAL(labelDirty()));
  }

  inline void addObject(Kst::ScalarPtr scalar) {
    _refObjects.append(scalar);
    connect(scalar.data(), SIGNAL(dirty()), this, SIGNAL(labelDirty()));
  }

  inline void addObject(Kst::StringPtr string) {
    _refObjects.append(string);
    connect(string.data(), SIGNAL(dirty()), this, SIGNAL(labelDirty()));
  }

  inline int fontSize() const {
    return _fontSize;
  }

  inline int fontAscent() const {
    return _ascent;
  }

  inline int lineSpacing() const {
    return _lineSpacing;
  }

  inline int fontDescent() const {
    return _descent;
  }

  inline int fontHeight() const {
    return _height;
  }

  inline int fontWidth(const QString& txt) const {
    if (p) {
      return p->fontMetrics().horizontalAdvance(txt);
    } else {
      return _fm.horizontalAdvance(txt);
    }
  }

  int x, y; // Coordinates we're rendering at
  int xMax, xStart;
  QString fontName;
  int size;
  QPainter *p;
  int precision;
  QList<Kst::Primitive*> _refObjects;
  QPen pen;
  int lines;
  QVector<RenderedText> cachedText;

  Q_SIGNALS:
    void labelDirty();

  private:
    QFont _font;
    QFontMetrics _fm;
    int _ascent, _descent, _height, _lineSpacing; // caches to avoid performance problem with QFont*
    int _fontSize;
};

struct Chunk;
void renderLabel(RenderContext& rc, Chunk *fi, bool cache, bool draw);
void paintLabel(RenderContext& rc, QPainter *p);
}

#endif
// vim: ts=2 sw=2 et
