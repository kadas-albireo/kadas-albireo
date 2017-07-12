/***************************************************************************
    qgsfloatinginputwidget.h  -  A floating CAD-style input widget
    -------------------------------------------------------------
  begin                : Nov 26, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPTOOLDRAWSHAPE_P_H
#define QGSMAPTOOLDRAWSHAPE_P_H

#include <QDoubleValidator>
#include <QGridLayout>
#include <QLineEdit>

class QgsFloatingInputWidgetField : public QLineEdit
{
    Q_OBJECT
  public:
    QgsFloatingInputWidgetField( QValidator* validator = new QDoubleValidator(), QWidget* parent = 0 );
    void setText( const QString& text );

  signals:
    void inputChanged();
    void inputConfirmed();

  private:
    QString mPrevText;

  private slots:
    void focusOutEvent( QFocusEvent *ev ) override;
    void checkInputChanged();
};

class QgsFloatingInputWidget : public QWidget
{
  public:
    QgsFloatingInputWidget( QWidget* parent );
    void addInputField( const QString& label, QgsFloatingInputWidgetField* widget, bool initiallyfocused = false );
    void setFocusedInputField( QgsFloatingInputWidgetField* widget );
    QgsFloatingInputWidgetField* focusedInputField() const { return mFocusChild; }
    bool eventFilter( QObject *obj, QEvent *ev ) override;

  protected:
    bool focusNextPrevChild( bool next ) override;
    void keyPressEvent( QKeyEvent *ev ) override;

  private:
    QgsFloatingInputWidgetField* mFocusChild;
    QList<QgsFloatingInputWidgetField*> mFocusChildren;

};

#endif // QGSMAPTOOLDRAWSHAPE_P_H
