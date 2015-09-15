/***************************************************************************
 *  qgsvbscrashhandler.h                                                   *
 *  -------------------                                                    *
 *  begin                : Sep 15, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSVBSCRASHHANDLER_H
#define QGSVBSCRASHHANDLER_H

#include <QDialog>

class QPlainTextEdit;

class QgsVBSCrashHandler : public QDialog
{
    Q_OBJECT

  public:
    explicit QgsVBSCrashHandler( const QString& coredumpFile, const QString &coredumpError, QWidget *parent = 0 );

#ifdef __MSC_VER
    static LONG WINAPI crashHandler( struct _EXCEPTION_POINTERS *ExceptionInfo );
#else
    static void crashHandler( int sig );
#endif
    static void installCrashHandler();

  private:
    QPlainTextEdit* mTextEditUserMessage;
    QPushButton* mButtonSubmit;
    QPushButton* mButtonClose;
    QString mCoredumpFile;
    QString mCoredumpError;
    int m_pid;

  private slots:
    void submitCrashReport();
};

#endif // QGSVBSCRASHHANDLER_H
