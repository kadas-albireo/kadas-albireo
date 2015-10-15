/***************************************************************************
    qgsredlining.cpp - Redlining support
     --------------------------------------
    Date                 : Sep 2015
    Copyright            : (C) 2015 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsredlining.h"
#include "qgisapp.h"
#include "qgscolorbuttonv2.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptooladdfeature.h"
#include "qgsmaptoolsredlining.h"
#include "qgsredlininglayer.h"
#include "qgsredliningtextdialog.h"
#include "qgsrubberband.h"
#include "qgssymbollayerv2utils.h"
#include "qgsproject.h"

#include <QSettings>

QgsRedlining::QgsRedlining( QgisApp* app )
    : QObject( app ), mApp( app ), mLayer( 0 ), mLayerRefCount( 0 )
{

  QToolBar* redliningToolbar = mApp->addToolBar( tr( "Redlining" ) );

  QAction* actionNewMarker = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Marker" ), this );

  QAction* actionNewPoint = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Point" ), this );
  actionNewPoint->setCheckable( true );
  connect( actionNewPoint, SIGNAL( triggered( bool ) ), this, SLOT( newPoint() ) );

  QAction* actionNewSquare = new QAction( QIcon( ":/images/themes/default/redlining_square.svg" ), tr( "Square" ), this );
  actionNewSquare->setCheckable( true );
  connect( actionNewSquare, SIGNAL( triggered( bool ) ), this, SLOT( newSquare() ) );

  QAction* actionNewTriangle = new QAction( QIcon( ":/images/themes/default/redlining_triangle.svg" ), tr( "Triangle" ), this );
  actionNewTriangle->setCheckable( true );
  connect( actionNewTriangle, SIGNAL( triggered( bool ) ), this, SLOT( newTriangle() ) );

  QAction* actionNewLine = new QAction( QIcon( ":/images/themes/default/redlining_line.svg" ), tr( "Line" ), this );
  actionNewLine->setCheckable( true );
  connect( actionNewLine, SIGNAL( triggered( bool ) ), this, SLOT( newLine() ) );
  QAction* actionNewRectangle = new QAction( QIcon( ":/images/themes/default/redlining_rectangle.svg" ), tr( "Rectangle" ), this );
  actionNewRectangle->setCheckable( true );
  connect( actionNewRectangle, SIGNAL( triggered( bool ) ), this, SLOT( newRectangle() ) );
  QAction* actionNewPolygon = new QAction( QIcon( ":/images/themes/default/redlining_polygon.svg" ), tr( "Polygon" ), this );
  actionNewPolygon->setCheckable( true );
  connect( actionNewPolygon, SIGNAL( triggered( bool ) ), this, SLOT( newPolygon() ) );
  QAction* actionNewCircle = new QAction( QIcon( ":/images/themes/default/redlining_circle.svg" ), tr( "Circle" ), this );
  actionNewCircle->setCheckable( true );
  connect( actionNewCircle, SIGNAL( triggered( bool ) ), this, SLOT( newCircle() ) );
  QAction* actionNewText = new QAction( QIcon( ":/images/themes/default/redlining_text.svg" ), tr( "Text" ), this );
  actionNewText->setCheckable( true );
  connect( actionNewText, SIGNAL( triggered( bool ) ), this, SLOT( newText() ) );

  mBtnNewObject = new QToolButton();
  mBtnNewObject->setToolTip( tr( "New Object" ) );
  QMenu* menuNewMarker = new QMenu();
  menuNewMarker->addAction( actionNewPoint );
  menuNewMarker->addAction( actionNewSquare );
  menuNewMarker->addAction( actionNewTriangle );
  actionNewMarker->setMenu( menuNewMarker );
  QMenu* menuNewObject = new QMenu();
  menuNewObject->addAction( actionNewMarker );
  menuNewObject->addAction( actionNewLine );
  menuNewObject->addAction( actionNewRectangle );
  menuNewObject->addAction( actionNewPolygon );
  menuNewObject->addAction( actionNewCircle );
  menuNewObject->addAction( actionNewText );
  mBtnNewObject->setMenu( menuNewObject );
  mBtnNewObject->setPopupMode( QToolButton::MenuButtonPopup );
  mBtnNewObject->setDefaultAction( actionNewPoint );
  connect( menuNewObject, SIGNAL( triggered( QAction* ) ), mBtnNewObject, SLOT( setDefaultAction( QAction* ) ) );
  redliningToolbar->addWidget( mBtnNewObject );

  mActionEditObject = new QAction( QIcon( ":/images/themes/default/mActionNodeTool.png" ), QString(), this );
  mActionEditObject->setToolTip( tr( "Edit Object" ) );
  mActionEditObject->setCheckable( true );
  redliningToolbar->addAction( mActionEditObject );
  connect( mActionEditObject, SIGNAL( triggered( bool ) ), this, SLOT( editObject() ) );

  redliningToolbar->addWidget( new QLabel( tr( "Border/Size:" ) ) );
  mSpinBorderSize = new QSpinBox();
  mSpinBorderSize->setRange( 1, 20 );
  mSpinBorderSize->setValue( QSettings().value( "/Redlining/size", 1 ).toInt() );
  connect( mSpinBorderSize, SIGNAL( valueChanged( int ) ), this, SLOT( saveOutlineWidth() ) );
  redliningToolbar->addWidget( mSpinBorderSize );

  redliningToolbar->addWidget( new QLabel( tr( "Outline:" ) ) );

  mBtnOutlineColor = new QgsColorButtonV2();
  mBtnOutlineColor->setAllowAlpha( true );
  mBtnOutlineColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/Redlining/outline_color", "0,0,0,255" ).toString() );
  mBtnOutlineColor->setColor( initialOutlineColor );
  connect( mBtnOutlineColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );
  redliningToolbar->addWidget( mBtnOutlineColor );

  mOutlineStyleCombo = new QComboBox();
  mOutlineStyleCombo->setProperty( "settings_key", "outline_style" );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::NoPen ), QString(), Qt::NoPen );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::SolidLine ), QString(), Qt::SolidLine );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::DashLine ), QString(), Qt::DashLine );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::DashDotLine ), QString(), Qt::DashDotLine );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::DotLine ), QString(), Qt::DotLine );
  mOutlineStyleCombo->setCurrentIndex( QSettings().value( "/Redlining/outline_style", "1" ).toInt() );
  connect( mOutlineStyleCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveStyle() ) );
  redliningToolbar->addWidget( mOutlineStyleCombo );

  redliningToolbar->addWidget( new QLabel( tr( "Fill:" ) ) );

  mBtnFillColor = new QgsColorButtonV2();
  mBtnFillColor->setAllowAlpha( true );
  mBtnFillColor->setProperty( "settings_key", "fill_color" );
  QColor initialFillColor = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/Redlining/fill_color", "255,0,0,255" ).toString() );
  mBtnFillColor->setColor( initialFillColor );
  connect( mBtnFillColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );
  redliningToolbar->addWidget( mBtnFillColor );

  mFillStyleCombo = new QComboBox();
  mFillStyleCombo->setProperty( "settings_key", "fill_style" );
  mFillStyleCombo->addItem( createFillStyleIcon( Qt::NoBrush ), QString(), Qt::NoBrush );
  mFillStyleCombo->addItem( createFillStyleIcon( Qt::SolidPattern ), QString(), Qt::SolidPattern );
  mFillStyleCombo->addItem( createFillStyleIcon( Qt::HorPattern ), QString(), Qt::HorPattern );
  mFillStyleCombo->addItem( createFillStyleIcon( Qt::VerPattern ), QString(), Qt::VerPattern );
  mFillStyleCombo->addItem( createFillStyleIcon( Qt::BDiagPattern ), QString(), Qt::BDiagPattern );
  mFillStyleCombo->addItem( createFillStyleIcon( Qt::DiagCrossPattern ), QString(), Qt::DiagCrossPattern );
  mFillStyleCombo->addItem( createFillStyleIcon( Qt::FDiagPattern ), QString(), Qt::FDiagPattern );
  mFillStyleCombo->addItem( createFillStyleIcon( Qt::CrossPattern ), QString(), Qt::CrossPattern );
  mFillStyleCombo->setCurrentIndex( QSettings().value( "/Redlining/fill_style", "1" ).toInt() );
  connect( mFillStyleCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveStyle() ) );
  redliningToolbar->addWidget( mFillStyleCombo );

  connect( mApp, SIGNAL( newProject() ), this, SLOT( clearLayer() ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProject( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProject( QDomDocument& ) ) );
}

QgsRedliningLayer* QgsRedlining::getOrCreateLayer()
{
  if ( mLayer )
  {
    return mLayer;
  }
  mLayer = new QgsRedliningLayer();
  QgsMapLayerRegistry::instance()->addMapLayer( mLayer, true, true );
  mLayerRefCount = 0;

  // QueuedConnection to delay execution of the slot until the signal-emitting function has exited,
  // since otherwise the undo stack becomes corrupted (featureChanged change inserted before featureAdded change)
  connect( mLayer, SIGNAL( featureAdded( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ), Qt::QueuedConnection );
  connect( mLayer.data(), SIGNAL( destroyed( QObject* ) ), this, SLOT( deactivateTool() ) );

  return mLayer;
}

void QgsRedlining::clearLayer()
{
  mLayer = 0;
  mLayerRefCount = 0;
}

void QgsRedlining::editObject()
{
  QgsRedliningEditTool* tool = new QgsRedliningEditTool( mApp->mapCanvas(), getOrCreateLayer() );
  connect( this, SIGNAL( featureStyleChanged() ), tool, SLOT( onStyleChanged() ) );
  connect( tool, SIGNAL( featureSelected( QgsFeatureId ) ), this, SLOT( syncStyleWidgets( QgsFeatureId ) ) );
  connect( tool, SIGNAL( updateFeatureStyle( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ) );
  activateTool( tool, qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newPoint()
{
  activateTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "circle" ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newSquare()
{
  activateTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "rectangle" ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newTriangle()
{
  activateTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "triangle" ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newLine()
{
  activateTool( new QgsMapToolAddFeature( mApp->mapCanvas(), QgsMapToolCapture::CaptureLine, QGis::WKBLineString ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newRectangle()
{
  activateTool( new QgsRedliningRectangleMapTool( mApp->mapCanvas(), getOrCreateLayer() ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newPolygon()
{
  activateTool( new QgsMapToolAddFeature( mApp->mapCanvas(), QgsMapToolCapture::CapturePolygon, QGis::WKBPolygon ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newCircle()
{
  activateTool( new QgsRedliningCircleMapTool( mApp->mapCanvas(), getOrCreateLayer() ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newText()
{
  activateTool( new QgsRedliningTextTool( mApp->mapCanvas(), getOrCreateLayer() ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::activateTool( QgsMapTool *tool, QAction* action )
{
  tool->setAction( action );
  connect( tool, SIGNAL( deactivated() ), this, SLOT( deactivateTool() ) );
  if ( mLayerRefCount == 0 )
  {
    mApp->mapCanvas()->setCurrentLayer( getOrCreateLayer() );
    mLayer->startEditing();
  }
  ++mLayerRefCount;
  mApp->mapCanvas()->setMapTool( tool );
  mRedliningTool = tool;
}

void QgsRedlining::deactivateTool()
{
  if ( mRedliningTool )
  {
    if ( mLayer )
    {
      --mLayerRefCount;
      if ( mLayerRefCount == 0 )
      {
        mLayer->commitChanges();
        mApp->mapCanvas()->setCurrentLayer( 0 );
      }
    }
    mRedliningTool->deleteLater();
  }
}

void QgsRedlining::syncStyleWidgets( const QgsFeatureId& fid )
{
  QTextStream( stdout ) << "syncStyleWidgets" << endl;
  if ( !mLayer )
  {
    return;
  }
  QgsFeature f;
  if ( !mLayer->getFeatures( QgsFeatureRequest( fid ) ).nextFeature( f ) )
  {
    return;
  }
  mSpinBorderSize->blockSignals( true );
  mSpinBorderSize->setValue( 2 * f.attribute( "size" ).toInt() );
  mSpinBorderSize->blockSignals( false );
  mBtnOutlineColor->blockSignals( true );
  mBtnOutlineColor->setColor( QgsSymbolLayerV2Utils::decodeColor( f.attribute( "outline" ).toString() ) );
  mBtnOutlineColor->blockSignals( false );
  mBtnFillColor->blockSignals( true );
  mBtnFillColor->setColor( QgsSymbolLayerV2Utils::decodeColor( f.attribute( "fill" ).toString() ) );
  mBtnFillColor->blockSignals( false );
  mOutlineStyleCombo->blockSignals( true );
  mOutlineStyleCombo->setCurrentIndex( mOutlineStyleCombo->findData( QgsSymbolLayerV2Utils::decodePenStyle( f.attribute( "outline_style" ).toString() ) ) );
  mOutlineStyleCombo->blockSignals( false );
  mFillStyleCombo->blockSignals( true );
  mFillStyleCombo->setCurrentIndex( mFillStyleCombo->findData( QgsSymbolLayerV2Utils::decodeBrushStyle( f.attribute( "fill_style" ).toString() ) ) );
  mFillStyleCombo->blockSignals( false );
}

void QgsRedlining::updateFeatureStyle( const QgsFeatureId &fid )
{
  if ( !mLayer )
  {
    return;
  }
  QgsFeature f;
  if ( !mLayer->getFeatures( QgsFeatureRequest( fid ) ).nextFeature( f ) )
  {
    return;
  }
  const QgsFields& fields = mLayer->pendingFields();
  mLayer->changeAttributeValue( fid, fields.indexFromName( "size" ), 0.5 * mSpinBorderSize->value() );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline" ), QgsSymbolLayerV2Utils::encodeColor( mBtnOutlineColor->color() ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill" ), QgsSymbolLayerV2Utils::encodeColor( mBtnFillColor->color() ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline_style" ), QgsSymbolLayerV2Utils::encodePenStyle( static_cast<Qt::PenStyle>( mOutlineStyleCombo->itemData( mOutlineStyleCombo->currentIndex() ).toInt() ) ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill_style" ), QgsSymbolLayerV2Utils::encodeBrushStyle( static_cast<Qt::BrushStyle>( mFillStyleCombo->itemData( mFillStyleCombo->currentIndex() ).toInt() ) ) );
  mApp->mapCanvas()->clearCache( mLayer->id() );
  mApp->mapCanvas()->refresh();
}

void QgsRedlining::saveColor()
{
  QgsColorButtonV2* btn = qobject_cast<QgsColorButtonV2*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + btn->property( "settings_key" ).toString();
  QSettings().setValue( key, QgsSymbolLayerV2Utils::encodeColor( btn->color() ) );
  emit featureStyleChanged();
}

void QgsRedlining::saveOutlineWidth()
{
  QSettings().setValue( "/Redlining/size", mSpinBorderSize->value() );
  emit featureStyleChanged();
}

void QgsRedlining::saveStyle()
{
  QComboBox* combo = qobject_cast<QComboBox*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + combo->property( "settings_key" ).toString();
  QSettings().setValue( key, combo->currentIndex() );
  emit featureStyleChanged();
}

void QgsRedlining::readProject( const QDomDocument& doc )
{
  clearLayer();
  QDomNodeList nl = doc.elementsByTagName( "Redlining" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  getOrCreateLayer()->read( nl.at( 0 ).toElement() );
}

void QgsRedlining::writeProject( QDomDocument& doc )
{
  if ( !mLayer )
  {
    return;
  }

  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement redliningElem = doc.createElement( "Redlining" );
  mLayer->write( redliningElem );
  qgisElem.appendChild( redliningElem );
}

QIcon QgsRedlining::createOutlineStyleIcon( Qt::PenStyle style )
{
  QPixmap pixmap( 16, 16 );
  pixmap.fill( Qt::transparent );
  QPainter painter( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  QPen pen;
  pen.setStyle( style );
  pen.setColor( Qt::black );
  pen.setWidth( 1 );
  painter.setPen( pen );
  painter.drawLine( 0, 8, 16, 8 );
  return pixmap;
}

QIcon QgsRedlining::createFillStyleIcon( Qt::BrushStyle style )
{
  QPixmap pixmap( 16, 16 );
  pixmap.fill( Qt::transparent );
  QPainter painter( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  QBrush brush;
  brush.setStyle( style );
  brush.setColor( Qt::black );
  painter.fillRect( 0, 0, 16, 16, brush );
  return pixmap;
}
