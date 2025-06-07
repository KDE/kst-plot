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

#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QTextBrowser>
#include "debug.h"

#include "kstcore_export.h"

namespace Kst {

class LogWidget : public QTextBrowser
{
  Q_OBJECT
  public:
    explicit LogWidget(QWidget *parent);
    virtual ~LogWidget();

  public Q_SLOTS:
    void logAdded(const Debug::LogMessage&);

    void setShowError(bool show);
    void setShowWarning(bool show);
    void setShowNotice(bool show);
    void setShowTrace(bool show);

  private slots:
    void regenerate();

  private:
    int _show;
    void setShowLevel(Debug::LogLevel, bool show);
};

}

#endif

// vim: ts=2 sw=2 et
