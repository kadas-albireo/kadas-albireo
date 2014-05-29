/***************************************************************************
    qgstexteditwidget.h
     --------------------------------------
    Date                 : 5.1.2014
    Copyright            : (C) 2014 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSTEXTEDITWIDGET_H
#define QGSTEXTEDITWIDGET_H

#include "qgseditorwidgetwrapper.h"

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>

class GUI_EXPORT QgsTextEditWidget : public QgsEditorWidgetWrapper
{
    Q_OBJECT
  public:
    explicit QgsTextEditWidget( QgsVectorLayer* vl, int fieldIdx, QWidget* editor = 0, QWidget* parent = 0 );

    // QgsEditorWidgetWrapper interface
  public:
    QVariant value();

  protected:
    QWidget*createWidget( QWidget* parent );
    void initWidget( QWidget* editor );

  public slots:
    void setValue( const QVariant& value );

  private:
    QTextEdit* mTextEdit;
    QPlainTextEdit* mPlainTextEdit;
    QLineEdit* mLineEdit;
};

#endif // QGSTEXTEDITWIDGET_H
