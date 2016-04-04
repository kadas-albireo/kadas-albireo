/***************************************************************************
                              qgspinannotationitem.cpp
                              ------------------------
  begin                : August, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgspinannotationitem.h"
#include "qgsproject.h"
#include "qgsbillboardregistry.h"
#include "qgscoordinateformat.h"
#include "qgscrscache.h"
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QTextBrowser>


class RemarksEdit : public QTextBrowser
{
  public:
    RemarksEdit( QWidget* parent = 0 ) : QTextBrowser( parent )
    {
      setOpenLinks( true );
      setOpenExternalLinks( true );
      setReadOnly( false );
    }
  protected:
    void contextMenuEvent( QContextMenuEvent *e ) override
    {
      QString anchor = anchorAt( e->pos() );
      if ( !anchor.isEmpty() )
      {
        QMenu menu;
        QAction* openAction = menu.addAction( tr( "Open link..." ) );
        QAction* copyAction = menu.addAction( tr( "Copy link location" ) );
        QAction* clickedAction = menu.exec( e->globalPos() );
        if ( clickedAction == openAction )
        {
          QDesktopServices::openUrl( QUrl::fromUserInput( anchor ) );
        }
        else if ( clickedAction == copyAction )
        {
          QApplication::clipboard()->setText( anchor );
        }
        e->accept();
      }
      else
      {
        QTextBrowser::contextMenuEvent( e );
      }
    }
    void keyPressEvent( QKeyEvent* e ) override
    {
      if ( e->key() == Qt::Key_Control )
      {
        setReadOnly( true );
        e->accept();
        return;
      }
      setReadOnly( false );
      QTextBrowser::keyPressEvent( e );
    }
    void keyReleaseEvent( QKeyEvent* e ) override
    {
      setReadOnly( false );
      QTextBrowser::keyReleaseEvent( e );
    }
};

REGISTER_QGS_ANNOTATION_ITEM( QgsPinAnnotationItem )

QgsPinAnnotationItem::QgsPinAnnotationItem( QgsMapCanvas* canvas )
    : QgsSvgAnnotationItem( canvas )
{
  setItemFlags( QgsAnnotationItem::ItemIsNotResizeable |
                QgsAnnotationItem::ItemHasNoFrame |
                QgsAnnotationItem::ItemHasNoMarker );
  QSize imageSize = QImageReader( ":/images/themes/default/pin_red.svg" ).size();
  setFilePath( ":/images/themes/default/pin_red.svg" );
  setFrameSize( imageSize );
  setOffsetFromReferencePoint( QPointF( -imageSize.width() / 2., -imageSize.height() ) );
  connect( QgsCoordinateFormat::instance(), SIGNAL( coordinateDisplayFormatChanged( QgsCoordinateFormat::Format, QString ) ), this, SLOT( updateToolTip() ) );
  connect( QgsCoordinateFormat::instance(), SIGNAL( heightDisplayUnitChanged( QGis::UnitType ) ), this, SLOT( updateToolTip() ) );
}

QgsPinAnnotationItem::~QgsPinAnnotationItem()
{
  if ( !mIsClone )
  {
    QgsBillBoardRegistry::instance()->removeItem( this );
  }
}

QgsPinAnnotationItem::QgsPinAnnotationItem( QgsMapCanvas* canvas, QgsPinAnnotationItem* source )
    : QgsSvgAnnotationItem( canvas, source )
{
}

void QgsPinAnnotationItem::updateToolTip()
{
  QString posStr = QgsCoordinateFormat::instance()->getDisplayString( mGeoPos, mGeoPosCrs );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( mGeoPos.toString() ).arg( mGeoPosCrs.authid() );
  }
  QString toolTipText = QString( "<b>Position:</b> %1<br /><b>Height:</b> %2 %3<br /><b>Name:</b> %4<br /><b>Remarks:</b><br />%5" )
                        .arg( posStr )
                        .arg( QgsCoordinateFormat::instance()->getHeightAtPos( mGeoPos, mGeoPosCrs ) )
                        .arg( QgsCoordinateFormat::instance()->getHeightDisplayUnit() == QGis::Feet ? "ft" : "m" )
                        .arg( mName )
                        .arg( mRemarks );
  setToolTip( toolTipText );
}

void QgsPinAnnotationItem::setMapPosition( const QgsPoint& pos, const QgsCoordinateReferenceSystem &crs )
{
  QgsSvgAnnotationItem::setMapPosition( pos, crs );
  updateToolTip();
  if ( !mIsClone )
  {
    QgsPoint worldPos = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), "EPSG:4326" )->transform( mGeoPos );
    QgsBillBoardRegistry::instance()->addItem( this, getImage(), worldPos );
  }
}

void QgsPinAnnotationItem::showContextMenu( const QPoint& screenPos )
{
  QMenu menu;
  menu.addAction( tr( "Copy position" ), this, SLOT( copyPosition() ) );
  menu.addAction( tr( "Remove" ), this, SLOT( deleteLater() ) );
  menu.exec( screenPos );
}

void QgsPinAnnotationItem::writeXML( QDomDocument& doc ) const
{
  QDomElement documentElem = doc.documentElement();
  if ( !documentElem.isNull() )
  {
    QDomElement pinAnnotationElem = doc.createElement( "PinAnnotationItem" );
    pinAnnotationElem.setAttribute( "file", QgsProject::instance()->writePath( mFilePath ) );
    pinAnnotationElem.setAttribute( "pinName", mName );
    QDomElement remarksElem = doc.createElement(( "PinRemarks" ) );
    remarksElem.appendChild( doc.createTextNode( mRemarks ) );
    pinAnnotationElem.appendChild( remarksElem );
    _writeXML( doc, pinAnnotationElem );
    documentElem.appendChild( pinAnnotationElem );
  }
}

void QgsPinAnnotationItem::readXML( const QDomDocument& doc, const QDomElement& itemElem )
{
  QString filePath = QgsProject::instance()->readPath( itemElem.attribute( "file" ) );
  setFilePath( filePath );
  mName = itemElem.attribute( "pinName" );
  QDomElement remarksElem = itemElem.firstChildElement( "PinRemarks" );
  mRemarks = remarksElem.text();
  QDomElement annotationElem = itemElem.firstChildElement( "AnnotationItem" );
  if ( !annotationElem.isNull() )
  {
    _readXML( doc, annotationElem );
  }
  updateToolTip();
  QgsPoint worldPos = QgsCoordinateTransformCache::instance()->transform( mGeoPosCrs.authid(), "EPSG:4326" )->transform( mGeoPos );
  QgsBillBoardRegistry::instance()->addItem( this, getImage(), worldPos );
}


void QgsPinAnnotationItem::copyPosition()
{
  QString posStr = QgsCoordinateFormat::instance()->getDisplayString( mGeoPos, mGeoPosCrs );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( mGeoPos.toString() ).arg( mGeoPosCrs.authid() );
  }
  QString text = QString( "%1 %2" )
                 .arg( posStr )
                 .arg( QgsCoordinateFormat::instance()->getHeightAtPos( mGeoPos, mGeoPosCrs ) );
  QApplication::clipboard()->setText( text );
}

void QgsPinAnnotationItem::_showItemEditor()
{
  QDialog dialog;
  dialog.setWindowTitle( tr( "Pin attributes" ) );
  QGridLayout* layout = new QGridLayout();
  dialog.setLayout( layout );
  layout->addWidget( new QLabel( tr( "Name:" ) ), 0, 0, 1, 1 );
  QLineEdit* nameEdit = new QLineEdit( mName );
  layout->addWidget( nameEdit, 0, 1, 1, 1 );
  layout->addWidget( new QLabel( tr( "Remarks:" ) ), 1, 0, 1, 2 );
  RemarksEdit* remarksEdit = new RemarksEdit();
  remarksEdit->setHtml( mRemarks );
  layout->addWidget( remarksEdit, 2, 0, 1, 2 );
  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal );
  layout->addWidget( bbox, 3, 0, 1, 2 );
  connect( bbox, SIGNAL( accepted() ), &dialog, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), &dialog, SLOT( reject() ) );
  if ( dialog.exec() == QDialog::Accepted )
  {
    mName = nameEdit->text();
    mRemarks = remarksEdit->toHtml();
  }
  updateToolTip();
}
