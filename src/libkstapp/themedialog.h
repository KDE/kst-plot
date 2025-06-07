/*
    Copyright (C) 2011  Barth Netterfield <netterfield@astro.utoronto.ca>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef THEMEDIALOG_H
#define THEMEDIALOG_H

#include "dialog.h"
#include "editmultiplewidget.h"

#include "kstcore_export.h"

#include "ui_themedialog.h"

namespace Kst {
  
class FillTab;
class StrokeTab;
class LabelPropertiesTab;

class ThemeDialog: public QDialog, public Ui::ThemeDialog
{
Q_OBJECT
public:
  explicit ThemeDialog(QWidget *parent = 0);
  virtual ~ThemeDialog();

  void reset();
  void apply();
  QFont font() const;

private:
  void setFillTab();
  void setStrokeTab();
  void setFontTab();

protected:
  FillTab *_fillTab;
  StrokeTab *_strokeTab;
  LabelPropertiesTab *_fontTab;

Q_SIGNALS:
  void cancel();

private Q_SLOTS:
  void buttonClicked(QAbstractButton *button);

};

}

#endif // THEMEDIALOG_H
