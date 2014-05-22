/***************************************************************************
    qgswebviewwidget.h
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

#ifndef QGSWEBVIEWWIDGET_H
#define QGSWEBVIEWWIDGET_H

#include "qgseditorwidgetwrapper.h"

#include <QWebView>
#include <QLineEdit>
#include <QPushButton>

class GUI_EXPORT QgsWebViewWidget : public QgsEditorWidgetWrapper
{
    Q_OBJECT
  public:
    explicit QgsWebViewWidget( QgsVectorLayer* vl, int fieldIdx, QWidget* editor = 0, QWidget* parent = 0 );


    // QgsEditorWidgetWrapper interface
  public:
    QVariant value();

  protected:
    QWidget* createWidget( QWidget* parent );
    void initWidget( QWidget* editor );

  public slots:
    void setValue( const QVariant& value );
    void setEnabled( bool enabled );

  private slots:
    void loadUrl( const QString &url );
    void selectFileName();

  private:
    //! This label is used as a container to display the picture
    QWebView* mWebView;
    //! The line edit containing the path to the picture
    QLineEdit* mLineEdit;
    //! The button to open the file chooser dialog
    QPushButton* mButton;
};

#endif // QGSWEBVIEWWIDGET_H
