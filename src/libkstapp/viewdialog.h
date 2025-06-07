/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2008 The University of Toronto                        *
 *                   netterfield@astro.utoronto.ca                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef VIEWDIALOG_H
#define VIEWDIALOG_H

#include "dialog.h"

#include <QPointer>

#include "kstcore_export.h"

namespace Kst {

class View;
class FillTab;

class ViewDialog : public Dialog
{
  Q_OBJECT
  public:
    explicit ViewDialog(View *view, QWidget *parent = 0);
    virtual ~ViewDialog();

  private Q_SLOTS:
    void fillChanged();

  private:
    void setupFill();

  private:
    QPointer<View> _view;
    FillTab *_fillTab;
};

}

#endif

// vim: ts=2 sw=2 et
