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

class QgsMapCanvas;
class QgsPoint;

class GUI_EXPORT QgsFloatingInputWidgetField : public QLineEdit
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

class GUI_EXPORT QgsFloatingInputWidget : public QWidget
{
  public:
    QgsFloatingInputWidget( QgsMapCanvas* canvas );
    int addInputField( const QString& label, QgsFloatingInputWidgetField* widget, bool initiallyfocused = false );
    void removeInputField( int idx );
    void setInputFieldVisible( int idx, bool visible );
    void setFocusedInputField( QgsFloatingInputWidgetField* widget );
    const QList<QgsFloatingInputWidgetField*>& inputFields() const { return mInputFields; }
    QgsFloatingInputWidgetField* focusedInputField() const { return mFocusedInput; }
    bool eventFilter( QObject *obj, QEvent *ev ) override;

    void adjustCursorAndExtent( const QgsPoint& geoPos );

  protected:
    bool focusNextPrevChild( bool next ) override;
    void keyPressEvent( QKeyEvent *ev ) override;

  private:
    QgsMapCanvas* mCanvas;
    QgsFloatingInputWidgetField* mFocusedInput;
    QList<QgsFloatingInputWidgetField*> mInputFields;

};

#endif // QGSMAPTOOLDRAWSHAPE_P_H
