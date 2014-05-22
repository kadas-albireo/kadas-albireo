/***************************************************************************
    qgsuuidwidget.h
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

#ifndef QGSUUIDWIDGET_H
#define QGSUUIDWIDGET_H

#include "qgseditorwidgetwrapper.h"

#include <QLineEdit>
#include <QLabel>

class GUI_EXPORT QgsUuidWidget : public QgsEditorWidgetWrapper
{
    Q_OBJECT
  public:
    explicit QgsUuidWidget( QgsVectorLayer* vl, int fieldIdx, QWidget* editor = 0, QWidget* parent = 0 );

    // QgsEditorWidgetWrapper interface
  public:
    QVariant value();

  protected:
    QWidget* createWidget(QWidget* parent);
    void initWidget(QWidget* editor);

  public slots:
    void setValue(const QVariant& value);
    void setEnabled( bool enabled );

  private:
    QLabel* mLabel;
    QLineEdit* mLineEdit;
};

#endif // QGSUUIDWIDGET_H
