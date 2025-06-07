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

#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include "kstcore_export.h"

#include "ui_dialog.h"

template<class Key, class Value>
class QHash;

namespace Kst {

class DialogPage;

class Dialog : public QDialog, public Ui::Dialog
{
  Q_OBJECT
  public:
    friend class DialogSI;
    friend class ScriptServer;
    explicit Dialog(QWidget *parent = 0);
    virtual ~Dialog();

    void addDialogPage(DialogPage *page, bool addMinSize = true);
    void removeDialogPage(DialogPage *page);
    DialogPage* getDialogPage(const QString &pageName);
    void selectDialogPage(DialogPage *page);

    void setAlwaysAllowApply(const bool allow);

    // dialogs should implement the following if
    // they have validation threads that scripting
    // needs to wait for.
    virtual void waitForValidation() {return;}

  Q_SIGNALS:
    void ok();
    void apply();
    void cancel();

  protected:
    void setVisible(bool visible);

    QWidget *leftCustomWidget() const;
    QWidget *rightCustomWidget() const;
    QWidget *topCustomWidget() const;
    QWidget *bottomCustomWidget() const;
    QWidget *extensionWidget() const;
    QDialogButtonBox *buttonBox() const;

  private Q_SLOTS:
    void selectPageForItem(QListWidgetItem *item);
    void buttonClicked(QAbstractButton *button);
    void modified();

  private:
    void resetListWidget();

    QHash<QListWidgetItem*, DialogPage*> _itemHash;
    bool _allowApply;
};

}

#endif

// vim: ts=2 sw=2 et
