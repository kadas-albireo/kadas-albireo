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
#include "qgscircularstringv2.h"
#include "qgscurvepolygonv2.h"
#include "qgscolorbuttonv2.h"
#include "qgslinestringv2.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptooladdfeature.h"
#include "qgsmaptoolmovelabel.h"
#include "qgspolygonv2.h"
#include "qgsredlininglayer.h"
#include "qgsredliningrendererv2.h"
#include "qgsrubberband.h"
#include "qgssymbollayerv2utils.h"
#include "nodetool/qgsselectedfeature.h"
#include "nodetool/qgsvertexentry.h"
#include "qgsproject.h"

#include <QInputDialog>
#include <QSettings>

QgsRedlining::QgsRedlining( QgisApp* app )
    : QObject( app ), mApp( app ), mLayer( 0 ), mLayerRefCount( 0 )
{
  QAction* actionNewPoint = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Point" ), this );
  actionNewPoint->setCheckable( true );
  connect( actionNewPoint, SIGNAL( triggered( bool ) ), this, SLOT( newPoint() ) );
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
  QMenu* menuNewObject = new QMenu();
  menuNewObject->addAction( actionNewPoint );
  menuNewObject->addAction( actionNewLine );
  menuNewObject->addAction( actionNewRectangle );
  menuNewObject->addAction( actionNewPolygon );
  menuNewObject->addAction( actionNewCircle );
  menuNewObject->addAction( actionNewText );
  mBtnNewObject->setMenu( menuNewObject );
  mBtnNewObject->setPopupMode( QToolButton::MenuButtonPopup );
  mBtnNewObject->setDefaultAction( menuNewObject->actions().first() );
  connect( menuNewObject, SIGNAL( triggered( QAction* ) ), mBtnNewObject, SLOT( setDefaultAction( QAction* ) ) );
  mApp->redliningToolBar()->addWidget( mBtnNewObject );

  mActionEditObject = new QAction( QIcon( ":/images/themes/default/mActionNodeTool.png" ), QString(), this );
  mActionEditObject->setToolTip( tr( "Edit Object" ) );
  mActionEditObject->setCheckable( true );
  mApp->redliningToolBar()->addAction( mActionEditObject );
  connect( mActionEditObject, SIGNAL( triggered( bool ) ), this, SLOT( editObject() ) );

  mApp->redliningToolBar()->addWidget( new QLabel( tr( "Border/Size:" ) ) );
  mSpinBorderSize = new QSpinBox();
  mSpinBorderSize->setRange( 1, 20 );
  mSpinBorderSize->setValue( QSettings().value( "/Redlining/size", 1 ).toInt() );
  connect( mSpinBorderSize, SIGNAL( valueChanged( int ) ), this, SLOT( saveOutlineWidth() ) );
  mApp->redliningToolBar()->addWidget( mSpinBorderSize );

  mApp->redliningToolBar()->addWidget( new QLabel( tr( "Outline:" ) ) );

  mBtnOutlineColor = new QgsColorButtonV2();
  mBtnOutlineColor->setAllowAlpha( true );
  mBtnOutlineColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/Redlining/outline_color", "0,0,0,255" ).toString() );
  mBtnOutlineColor->setColor( initialOutlineColor );
  connect( mBtnOutlineColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );
  mApp->redliningToolBar()->addWidget( mBtnOutlineColor );

  mOutlineStyleCombo = new QComboBox();
  mOutlineStyleCombo->setProperty( "settings_key", "outline_style" );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::NoPen ), QString(), Qt::NoPen );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::SolidLine ), QString(), Qt::SolidLine );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::DashLine ), QString(), Qt::DashLine );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::DashDotLine ), QString(), Qt::DashDotLine );
  mOutlineStyleCombo->addItem( createOutlineStyleIcon( Qt::DotLine ), QString(), Qt::DotLine );
  mOutlineStyleCombo->setCurrentIndex( QSettings().value( "/Redlining/outline_style", "1" ).toInt() );
  connect( mOutlineStyleCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveStyle() ) );
  mApp->redliningToolBar()->addWidget( mOutlineStyleCombo );

  mApp->redliningToolBar()->addWidget( new QLabel( tr( "Fill:" ) ) );

  mBtnFillColor = new QgsColorButtonV2();
  mBtnFillColor->setAllowAlpha( true );
  mBtnFillColor->setProperty( "settings_key", "fill_color" );
  QColor initialFillColor = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/Redlining/fill_color", "255,0,0,255" ).toString() );
  mBtnFillColor->setColor( initialFillColor );
  connect( mBtnFillColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );
  mApp->redliningToolBar()->addWidget( mBtnFillColor );

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
  mApp->redliningToolBar()->addWidget( mFillStyleCombo );

  connect( mApp, SIGNAL( newProject() ), this, SLOT( clearLayer() ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProject( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProject( QDomDocument& ) ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL(layerWillBeRemoved(QString)), this, SLOT(checkLayerRemoved(QString)));
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
  activateTool( new QgsMapToolAddFeature( mApp->mapCanvas(), QgsMapToolCapture::CapturePoint, QGis::WKBPoint ), qobject_cast<QAction*>( QObject::sender() ) );
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
}

void QgsRedlining::deactivateTool()
{
  QgsMapTool* tool = qobject_cast<QgsMapTool*>( QObject::sender() );
  if ( mLayer )
  {
    --mLayerRefCount;
    if ( mLayerRefCount == 0 )
    {
      mLayer->commitChanges();
      mApp->mapCanvas()->setCurrentLayer( 0 );
    }
  }
  tool->deleteLater();
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
  mSpinBorderSize->setValue( f.attribute( "size" ).toInt() );
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
  mLayer->changeAttributeValue( fid, fields.indexFromName( "size" ), mSpinBorderSize->value() );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline" ), QgsSymbolLayerV2Utils::encodeColor( mBtnOutlineColor->color() ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill" ), QgsSymbolLayerV2Utils::encodeColor( mBtnFillColor->color() ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline_style" ), QgsSymbolLayerV2Utils::encodePenStyle( static_cast<Qt::PenStyle>( mOutlineStyleCombo->itemData( mOutlineStyleCombo->currentIndex() ).toInt() ) ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill_style" ), QgsSymbolLayerV2Utils::encodeBrushStyle( static_cast<Qt::BrushStyle>( mFillStyleCombo->itemData( mFillStyleCombo->currentIndex() ).toInt() ) ) );
  mApp->mapCanvas()->clearCache(mLayer->id());
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

void QgsRedlining::checkLayerRemoved(const QString &layerId)
{
  if(layerId == mLayer->id())
  {
    mLayer = 0;
  }
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

///////////////////////////////////////////////////////////////////////////////


QgsRedliningNewShapeMapTool::QgsRedliningNewShapeMapTool( QgsMapCanvas* canvas , QgsVectorLayer *layer )
    : QgsMapTool( canvas ), mLayer( layer ), mGeometry( 0 ), mRubberBand( 0 )
{
  setCursor( Qt::CrossCursor );
}

QgsRedliningNewShapeMapTool::~QgsRedliningNewShapeMapTool()
{
  delete mRubberBand;
  delete mGeometry;
}

void QgsRedliningNewShapeMapTool::canvasPressEvent( QMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton )
  {
    mPressPos = e->pos();
    mRubberBand = new QgsRubberBand( mCanvas );
  }
}

void QgsRedliningNewShapeMapTool::canvasReleaseEvent( QMouseEvent */*e*/ )
{
  if ( mRubberBand )
  {
    QgsFeature f( mLayer->pendingFields() );
    f.setGeometry( new QgsGeometry( mGeometry ) );
    mLayer->addFeature( f );
    mCanvas->refresh();
    mGeometry = 0; // no delete since ownership taken by QgsGeometry above
    delete mRubberBand;
    mRubberBand = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////

void QgsRedliningRectangleMapTool::canvasMoveEvent( QMouseEvent *e )
{
  if ( mRubberBand )
  {
    QRect rect = QRect( mPressPos, e->pos() ).normalized();
    delete mGeometry;
    mGeometry = new QgsPolygonV2();
    QgsLineStringV2* exterior = new QgsLineStringV2();
    exterior->setPoints(
      QList<QgsPointV2>()
      << toLayerCoordinates( mLayer, QPoint( rect.x(), rect.y() ) )
      << toLayerCoordinates( mLayer, QPoint( rect.x() + rect.width(), rect.y() ) )
      << toLayerCoordinates( mLayer, QPoint( rect.x() + rect.width(), rect.y() + rect.height() ) )
      << toLayerCoordinates( mLayer, QPoint( rect.x(), rect.y() + rect.height() ) )
      << toLayerCoordinates( mLayer, QPoint( rect.x(), rect.y() ) ) );
    mGeometry->setExteriorRing( exterior );
    QgsGeometry geom( mGeometry->clone() );
    mRubberBand->setToGeometry( &geom, mLayer );
  }
}

void QgsRedliningRectangleMapTool::canvasReleaseEvent( QMouseEvent */*e*/ )
{
  if ( mRubberBand )
  {
    QgsFeature f( mLayer->pendingFields() );
    f.setAttribute( "flags", "rect:" + mCanvas->mapSettings().destinationCrs().authid() );
    f.setGeometry( new QgsGeometry( mGeometry ) );
    mLayer->addFeature( f );
    mCanvas->refresh();
    mGeometry = 0; // no delete since ownership taken by QgsGeometry above
    delete mRubberBand;
    mRubberBand = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////

void QgsRedliningCircleMapTool::canvasMoveEvent( QMouseEvent *e )
{
  if ( mRubberBand )
  {
    QPoint diff = mPressPos - e->pos();
    double r = qSqrt( diff.x() * diff.x() + diff.y() + diff.y() );
    delete mGeometry;
    mGeometry = new QgsCurvePolygonV2();
    QgsCircularStringV2* exterior = new QgsCircularStringV2();
    exterior->setPoints(
      QList<QgsPointV2>() << toLayerCoordinates( mLayer, QPoint( mPressPos.x() + r, mPressPos.y() ) )
      << toLayerCoordinates( mLayer, QPoint( mPressPos.x(), mPressPos.y() ) )
      << toLayerCoordinates( mLayer, QPoint( mPressPos.x() + r, mPressPos.y() ) ) );
    mGeometry->setExteriorRing( exterior );
    QgsGeometry geom( mGeometry->segmentize() );
    mRubberBand->setToGeometry( &geom, mLayer );
  }
}

///////////////////////////////////////////////////////////////////////////////

void QgsRedliningTextTool::canvasReleaseEvent( QMouseEvent *e )
{
  QString text = QInputDialog::getText( 0, tr( "Enter text" ), tr( "Label text:" ) );
  if ( !text.isEmpty() )
  {
    QgsPoint pos = toLayerCoordinates( mLayer, e->pos() );
    QgsFeature f( mLayer->pendingFields() );
    f.setAttribute( "text", text );
    f.setAttribute( "text_x", pos.x() );
    f.setAttribute( "text_y", pos.y() );
    f.setGeometry( new QgsGeometry( new QgsPointV2( pos.x(), pos.y() ) ) );
    mLayer->addFeature( f );
    mCanvas->refresh();
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningEditTool::QgsRedliningEditTool( QgsMapCanvas* canvas , QgsVectorLayer *layer )
    : QgsMapTool( canvas ), mLayer( layer ), mMode( NoSelection ), mRubberBand( 0 ), mCurrentFeature( 0 ), mCurrentVertex( -1 ), mIsRectangle( false )
{
  connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( updateLabelBoundingBox() ) );
}

QgsRedliningEditTool::~QgsRedliningEditTool()
{
  delete mRubberBand;
  delete mCurrentFeature;
}

void QgsRedliningEditTool::canvasPressEvent( QMouseEvent *e )
{
  clearCurrent( true );
  mPrevPos = mPressPos = toMapCoordinates( e->pos() );
  QgsPoint pressLayerPot = toLayerCoordinates( mLayer, mPressPos );

  // First, look for a label below the cursor
  const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
  if ( labelingResults )
  {
    foreach ( const QgsLabelPosition& labelPos, labelingResults->labelsAtPosition( mPressPos ) )
    {
      if ( labelPos.layerID == mLayer->id() )
      {
        mCurrentLabel = labelPos;
        mMode = TextSelected;
        mRubberBand = new QgsRubberBand( mCanvas, QGis::Line );
        const QgsRectangle& rect = mCurrentLabel.labelRect;
        mRubberBand->addPoint( QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
        mRubberBand->addPoint( QgsPoint( rect.xMinimum(), rect.yMaximum() ) );
        mRubberBand->addPoint( QgsPoint( rect.xMaximum(), rect.yMaximum() ) );
        mRubberBand->addPoint( QgsPoint( rect.xMaximum(), rect.yMinimum() ) );
        mRubberBand->addPoint( QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
        mRubberBand->setColor( QColor( 0, 255, 0, 150 ) );
        mRubberBand->setWidth( 3 );
        mRubberBand->setLineStyle( Qt::DotLine );
        emit featureSelected( mCurrentLabel.featureId );
        return;
      }
    }
  }

  // Then, look for a feature below the cursor
  double r = QgsTolerance::vertexSearchRadius( mCanvas->currentLayer(), mCanvas->mapSettings() );
  QgsRectangle selectRect( pressLayerPot.x() - r, pressLayerPot.y() - r, pressLayerPot.x() + r, pressLayerPot.y() + r );
  QgsFeature feature;
  QgsFeatureIterator fit = mLayer->getFeatures( QgsFeatureRequest().setFilterRect( selectRect ) );
  if ( fit.nextFeature( feature ) && feature.attribute("text").toString().isEmpty() )
  {
    mMode = FeatureSelected;
    emit featureSelected( feature.id() );
    mRubberBand = new QgsRubberBand( mCanvas, feature.geometry()->type() );
    mRubberBand->setBorderColor( QColor( 0, 255, 0, 170 ) );
    mRubberBand->setFillColor( QColor( 0, 255, 0, 15 ) );
    mRubberBand->setWidth( 3 );
    mRubberBand->setLineStyle( Qt::DotLine );
    mRubberBand->setToGeometry( feature.geometry(), mLayer );
    mCurrentFeature = new QgsSelectedFeature( feature.id(), mLayer, mCanvas );
    mIsRectangle = false;
    foreach ( const QString& flag, feature.attribute( "flags" ).toString().split( "," ) )
    {
      if ( flag.startsWith( "rect" ) )
      {
        mIsRectangle = true;
        mRectangleCRS = flag.mid( 5 );
      }
    }

    // Check if a vertex was clicked
    int beforeVertex = -1, afterVertex = -1;
    double dist2;
    QgsPoint layerPos = toLayerCoordinates( mLayer, mPressPos );
    mCurrentFeature->geometry()->closestVertex( layerPos, mCurrentVertex, beforeVertex, afterVertex, dist2 );
    if ( mCurrentVertex != -1 && qSqrt( dist2 ) < QgsTolerance::vertexSearchRadius( mLayer, mCanvas->mapSettings() ) )
    {
      mCurrentFeature->selectVertex( mCurrentVertex );
    }
    else
    {
      mCurrentVertex = -1;
    }
    return;
  }
}

void QgsRedliningEditTool::canvasMoveEvent( QMouseEvent *e )
{
  QgsPoint p = toMapCoordinates( e->pos() );
  if (( e->buttons() & Qt::LeftButton ) == 0 )
  {
    return;
  }
  if ( mMode == FeatureSelected )
  {
    QgsPoint layerPos = toLayerCoordinates( mCurrentFeature->vlayer(), p );
    QgsPoint prevLayerPos = toLayerCoordinates( mCurrentFeature->vlayer(), mPrevPos );
    if ( mCurrentVertex != -1 )
    {
      QgsPoint p = mCurrentFeature->vertexMap()[mCurrentVertex]->pointV1();
      mCurrentFeature->geometry()->moveVertex( layerPos, mCurrentVertex );
      if ( mIsRectangle )
      {
        QgsCoordinateTransform t( mCurrentFeature->vlayer()->crs(), QgsCoordinateReferenceSystem( mRectangleCRS ) );
        int n = mCurrentFeature->vertexMap().size() - 1;
        int iPrev = ( mCurrentVertex - 1 + n ) % n;
        int iNext = ( mCurrentVertex + 1 + n ) % n;
        QgsPoint pPrev = t.transform( mCurrentFeature->vertexMap()[iPrev]->pointV1() );
        QgsPoint pNext = t.transform( mCurrentFeature->vertexMap()[iNext]->pointV1() );
        QgsPoint pCurr = t.transform( layerPos );
        if (( mCurrentVertex % 2 ) == 0 )
        {
          mCurrentFeature->geometry()->moveVertex( t.transform( QgsPoint( pCurr.x(), pPrev.y() ), QgsCoordinateTransform::ReverseTransform ), iPrev );
          mCurrentFeature->geometry()->moveVertex( t.transform( QgsPoint( pNext.x(), pCurr.y() ), QgsCoordinateTransform::ReverseTransform ), iNext );
        }
        else
        {
          mCurrentFeature->geometry()->moveVertex( t.transform( QgsPoint( pPrev.x(), pCurr.y() ), QgsCoordinateTransform::ReverseTransform ), iPrev );
          mCurrentFeature->geometry()->moveVertex( t.transform( QgsPoint( pCurr.x(), pNext.y() ), QgsCoordinateTransform::ReverseTransform ), iNext );
        }
      }
    }
    else
    {
      mCurrentFeature->geometry()->translate( layerPos.x() - prevLayerPos.x(), layerPos.y() - prevLayerPos.y() );
    }
    mCurrentFeature->updateVertexMarkersPosition();
    mRubberBand->setToGeometry( mCurrentFeature->geometry(), mCurrentFeature->vlayer() );
  }
  else if ( mMode == TextSelected )
  {
    mRubberBand->setTranslationOffset( p.x() - mPressPos.x(), p.y() - mPressPos.y() );
    mRubberBand->updatePosition();
  }
  mPrevPos = p;
}

void QgsRedliningEditTool::canvasReleaseEvent( QMouseEvent */*e*/ )
{
  if ( mMode == TextSelected )
  {
    double dx, dy;
    mRubberBand->translationOffset( dx, dy );
    mCurrentLabel.labelRect.setXMinimum( mCurrentLabel.labelRect.xMinimum() + dx );
    mCurrentLabel.labelRect.setYMinimum( mCurrentLabel.labelRect.yMinimum() + dy );
    mCurrentLabel.labelRect.setXMaximum( mCurrentLabel.labelRect.xMaximum() + dx );
    mCurrentLabel.labelRect.setYMaximum( mCurrentLabel.labelRect.yMaximum() + dy );
    QgsPoint pos = toLayerCoordinates( mLayer, QgsPoint( mCurrentLabel.labelRect.xMinimum(), mCurrentLabel.labelRect.yMinimum() ) );
    mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().fieldNameIndex( "text_x" ), pos.x() );
    mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().fieldNameIndex( "text_y" ), pos.y() );
    mCanvas->refresh();
  }
  else if ( mMode == FeatureSelected )
  {
    mLayer->changeGeometry( mCurrentFeature->featureId(), mCurrentFeature->geometry() );
    mCurrentFeature->deselectVertex( mCurrentVertex );
    mCanvas->refresh();
  }
}

void QgsRedliningEditTool::canvasDoubleClickEvent( QMouseEvent */*e*/ )
{
  if ( mMode == TextSelected )
  {
    bool ok = false;
    QString text = QInputDialog::getText( 0, tr( "Enter text" ), tr( "Label text:" ), QLineEdit::Normal, mCurrentLabel.labelText, &ok );
    if ( ok )
    {
      mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().indexFromName( "text" ), text );
      mCanvas->refresh();
    }
    return;
  }
}

void QgsRedliningEditTool::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace )
  {
    if ( mMode == TextSelected )
    {
      mLayer->deleteFeature( mCurrentLabel.featureId );
      clearCurrent();
      e->ignore();
    }
    else if ( mMode == FeatureSelected )
    {
      mLayer->deleteFeature( mCurrentFeature->featureId() );
      clearCurrent();
      e->ignore();
    }
  }
}

void QgsRedliningEditTool::deactivate()
{
  QgsMapTool::deactivate();
  clearCurrent();
}

void QgsRedliningEditTool::onStyleChanged()
{
  QTextStream( stdout ) << "onStyleChanged" << endl;
  if ( mMode == TextSelected )
  {
    emit updateFeatureStyle( mCurrentLabel.featureId );
  }
  else if ( mMode == FeatureSelected )
  {
    emit updateFeatureStyle( mCurrentFeature->featureId() );
  }
}

void QgsRedliningEditTool::clearCurrent( bool refresh )
{
  mMode = NoSelection;
  delete mRubberBand;
  mRubberBand = 0;
  mIsRectangle = false;
  delete mCurrentFeature.data();
  mCurrentFeature = 0;
  mCurrentVertex = -1;
  if ( refresh )
  {
    mCanvas->refresh();
  }
}

void QgsRedliningEditTool::updateLabelBoundingBox()
{
  if ( mMode == TextSelected )
  {
    // Try to find the label again
    QgsPoint p( mCurrentLabel.labelRect.xMinimum(), mCurrentLabel.labelRect.yMinimum() );

    const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
    if ( labelingResults )
    {
      foreach ( const QgsLabelPosition& labelPos, labelingResults->labelsAtPosition( p ) )
      {
        if ( labelPos.layerID == mCurrentLabel.layerID &&
             labelPos.featureId == mCurrentLabel.featureId &&
             labelPos.labelRect != mCurrentLabel.labelRect )
        {
          mCurrentLabel = labelPos;
          mRubberBand->reset();
          const QgsRectangle& rect = mCurrentLabel.labelRect;
          mRubberBand->setTranslationOffset( 0, 0 );
          mRubberBand->addPoint( QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
          mRubberBand->addPoint( QgsPoint( rect.xMinimum(), rect.yMaximum() ) );
          mRubberBand->addPoint( QgsPoint( rect.xMaximum(), rect.yMaximum() ) );
          mRubberBand->addPoint( QgsPoint( rect.xMaximum(), rect.yMinimum() ) );
          mRubberBand->addPoint( QgsPoint( rect.xMinimum(), rect.yMinimum() ) );
          return;
        }
      }
    }
    // Label disappeared? Should not happen
  }
}
