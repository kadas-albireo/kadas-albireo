/***************************************************************************
 *  qgsiamauth.h                                                           *
 *  ------------                                                           *
 *  begin                : March 2016                                      *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSIAMAUTH_H
#define QGSIAMAUTH_H

#include "qgisplugin.h"
#include <QDialog>
#include <QObject>

class QStackedLayout;
class QToolButton;
struct IDispatch;

class StackedDialog : public QDialog {
public:
  StackedDialog(QWidget* parent = 0, Qt::WindowFlags flags = 0);
  void pushWidget(QWidget* widget);
  void popWidget(QWidget *widget);
private:
  QStackedLayout* mLayout;
};

class QgsIAMAuth : public QObject, public QgisPlugin
{
    Q_OBJECT
  public:
    QgsIAMAuth( QgisInterface * theInterface );

    void initGui();
    void unload();

  private:
    QgisInterface* mQGisIface;
    QToolButton* mLoginButton;
    StackedDialog* mLoginDialog;

  private slots:
    void performLogin();
    void checkLoginComplete(QString);
    void handleNewWindow(IDispatch** ppDisp, bool& cancel, uint dwFlags, QString bstrUrlContext, QString bstrUrl);
    void handleWindowClose(bool isChild, bool& cancel);
};

#endif // QGSIAMAUTH_H
