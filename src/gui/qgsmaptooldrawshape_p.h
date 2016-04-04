/***************************************************************************
    qgsmaptooldrawshape_p.h  -  Utility classes for QgsMapToolDrawShape
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
#include <QKeyEvent>
#include <QLineEdit>

class QgsMapToolDrawShapeInputField : public QLineEdit
{
    Q_OBJECT
  public:
    QgsMapToolDrawShapeInputField( QValidator* validator = new QDoubleValidator(), QWidget* parent = 0 ) : QLineEdit( parent )
    {
      setValidator( validator );
      connect( this, SIGNAL( returnPressed() ), this, SLOT( checkInputChanged() ) );
    }
    void setText( const QString& text )
    {
      QLineEdit::setText( text );
      mPrevText = text;
    }

  signals:
    void inputChanged();
    void inputConfirmed();

  private:
    QString mPrevText;

  private slots:
    void focusOutEvent( QFocusEvent *ev ) override
    {
      QString curText = text();
      if ( curText != mPrevText )
      {
        mPrevText = curText;
        emit inputChanged();
      }
      QLineEdit::focusOutEvent( ev );
    }
    void checkInputChanged()
    {
      QString curText = text();
      if ( curText != mPrevText )
      {
        mPrevText = curText;
        emit inputChanged();
      }
      else
      {
        emit inputConfirmed();
      }
    }
};

class QgsMapToolDrawShapeInputWidget : public QWidget
{
  public:
    QgsMapToolDrawShapeInputWidget( QWidget* parent ) : QWidget( parent ), mFocusChild( 0 )
    {
      setObjectName( "MapToolDrawShapeInputWidget" );
      QGridLayout* gridLayout = new QGridLayout();
      gridLayout->setContentsMargins( 2, 2, 2, 2 );
      gridLayout->setSpacing( 1 );
      setLayout( gridLayout );
    }
    void addFocusChild( QLineEdit* widget )
    {
      mFocusChildren.append( widget );
    }
    void setFocusChild( QLineEdit* widget )
    {
      if ( mFocusChild )
      {
        mFocusChild->removeEventFilter( this );
      }
      mFocusChild = widget;
      mFocusChild->setFocus();
      mFocusChild->selectAll();
      mFocusChild->installEventFilter( this );
    }
    QLineEdit* focusChild() const
    {
      return mFocusChild;
    }
    bool eventFilter( QObject *obj, QEvent *ev ) override
    {
      // If currently focused widget loses focus, make it receive focus again
      if ( obj == mFocusChild && ev->type() == QEvent::FocusOut )
      {
        mFocusChild->setFocus();
        mFocusChild->selectAll();
        return true;
      }
      return QWidget::eventFilter( obj, ev );
    }
  protected:
    bool focusNextPrevChild( bool /*next*/ ) override
    {
      // Disable automatic TAB event handling
      // http://stackoverflow.com/a/21351638/1338788
      return false;
    }
    void keyPressEvent( QKeyEvent *ev ) override
    {
      // Override tab handling to ensure only the input fields inside the widget receive focus
      if ( ev->key() == Qt::Key_Tab )
      {
        int n = mFocusChildren.size();
        int nextIdx = ( mFocusChildren.indexOf( mFocusChild ) + 1 ) % n;
        setFocusChild( mFocusChildren[nextIdx] );
        ev->accept();
      }
      else if ( ev->key() == Qt::Key_Backtab )
      {
        int n = mFocusChildren.size();
        int nextIdx = ( n + mFocusChildren.indexOf( mFocusChild ) - 1 ) % n;
        setFocusChild( mFocusChildren[nextIdx] );
        ev->accept();
      }
      else
      {
        QWidget::keyPressEvent(( ev ) );
      }
    }

  private:
    QLineEdit* mFocusChild;
    QList<QLineEdit*> mFocusChildren;
};

#endif // QGSMAPTOOLDRAWSHAPE_P_H
