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
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptooladdfeature.h"
#include "qgsmaptoolmovelabel.h"
#include "qgsredliningrendererv2.h"
#include "qgsrubberband.h"
#include "nodetool/qgsselectedfeature.h"
#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsproject.h"

#include <QInputDialog>
#include <QSettings>
#include <QUuid>

class QgsRedlining::RedliningLayer : public QgsVectorLayer
{
  public:
    RedliningLayer() : QgsVectorLayer(
          QString( "mixed?crs=EPSG:4326&memoryid=%1" ).arg( QUuid::createUuid().toString() ),
          "Redlining",
          "memory" )
    {
      mLayerType = QgsMapLayer::RedliningLayer;
    }
};


QgsRedlining::QgsRedlining( QgisApp* app )
    : QObject( app ), mApp( app ), mLayer( 0 ), mLayerRefCount( 0 )
{
  QAction* actionNewPoint = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Point" ), this );
  actionNewPoint->setCheckable( true );
  connect( actionNewPoint, SIGNAL( triggered( bool ) ), this, SLOT( newPoint() ) );
  QAction* actionNewLine = new QAction( QIcon( ":/images/themes/default/redlining_line.svg" ), tr( "Line" ), this );
  actionNewLine->setCheckable( true );
  connect( actionNewLine, SIGNAL( triggered( bool ) ), this, SLOT( newLine() ) );
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
  mBtnOutlineColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QColor( QSettings().value( "/Redlining/outline_color", QColor( Qt::black ).name() ).toString() );
  mBtnOutlineColor->setColor( initialOutlineColor );
  connect( mBtnOutlineColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );
  mApp->redliningToolBar()->addWidget( mBtnOutlineColor );

  mApp->redliningToolBar()->addWidget( new QLabel( tr( "Fill:" ) ) );

  mBtnFillColor = new QgsColorButtonV2();
  mBtnFillColor->setProperty( "settings_key", "fill_color" );
  QColor initialFillColor = QColor( QSettings().value( "/Redlining/fill_color", QColor( Qt::red ).name() ).toString() );
  mBtnFillColor->setColor( initialFillColor );
  connect( mBtnFillColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );
  mApp->redliningToolBar()->addWidget( mBtnFillColor );

  connect( mApp, SIGNAL( newProject() ), this, SLOT( clearLayer() ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProject( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProject( QDomDocument& ) ) );
}

QgsRedlining::RedliningLayer* QgsRedlining::getOrCreateLayer()
{
  if ( mLayer )
  {
    return mLayer;
  }
  mLayer = new RedliningLayer;
  mLayer->setFeatureFormSuppress( QgsVectorLayer::SuppressOn );
  mLayer->dataProvider()->addAttributes( QList<QgsField>()
                                         << QgsField( "size", QVariant::Int, "integer", 2 )
                                         << QgsField( "outline", QVariant::String, "string", 7 )
                                         << QgsField( "fill", QVariant::String, "string", 7 )
                                         << QgsField( "text", QVariant::String, "string", 128 )
                                         << QgsField( "text_x", QVariant::Double, "double", 20, 15 )
                                         << QgsField( "text_y", QVariant::Double, "double", 20, 15 ) );
  mLayer->setRendererV2( new QgsRedliningRendererV2 );

  QgsMapLayerRegistry::instance()->addMapLayer( mLayer, true, true );
  mLayerRefCount = 0;
  // QueuedConnection to delay execution of the slot until the signal-emitting function has exited,
  // since otherwise the undo stack becomes corrupted (featureChanged change inserted before featureAdded change)
  connect( mLayer, SIGNAL( featureAdded( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ), Qt::QueuedConnection );

  mLayer->setCustomProperty( "labeling", "pal" );
  mLayer->setCustomProperty( "labeling/enabled", true );
  mLayer->setCustomProperty( "labeling/fieldName", "text" );
  mLayer->setCustomProperty( "labeling/dataDefined/PositionX", "1~~0~~~~text_x" );
  mLayer->setCustomProperty( "labeling/dataDefined/PositionY", "1~~0~~~~text_y" );
  mLayer->setCustomProperty( "labeling/dataDefined/Color", "1~~0~~~~outline" );
  mLayer->setCustomProperty( "labeling/dataDefined/Size", "1~~1~~8 + \"size\"~~" );

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
  connect( this, SIGNAL( styleChanged() ), tool, SLOT( onStyleChanged() ) );
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
  mBtnOutlineColor->setColor( QColor( f.attribute( "outline" ).toString() ) );
  mBtnOutlineColor->blockSignals( false );
  mBtnFillColor->blockSignals( true );
  mBtnFillColor->setColor( QColor( f.attribute( "fill" ).toString() ) );
  mBtnFillColor->blockSignals( false );
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
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline" ), mBtnOutlineColor->color().name() );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill" ), mBtnFillColor->color().name() );
  mApp->mapCanvas()->refresh();
}

void QgsRedlining::saveColor()
{
  QgsColorButtonV2* btn = qobject_cast<QgsColorButtonV2*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + btn->property( "settings_key" ).toString();
  QSettings().setValue( key, btn->color().name() );
  emit styleChanged();
}

void QgsRedlining::saveOutlineWidth()
{
  QSettings().setValue( "/Redlining/size", mSpinBorderSize->value() );
  emit styleChanged();
}

void QgsRedlining::readProject( const QDomDocument& doc )
{
  clearLayer();
  getOrCreateLayer();

  QDomNodeList nl = doc.elementsByTagName( "Redlining" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement redliningElement = nl.at( 0 ).toElement();
  QgsFeatureList features;
  QDomNodeList nodes = redliningElement.childNodes();
  for ( int iNode = 0, nNodes = nodes.size(); iNode < nNodes; ++iNode )
  {
    QDomElement redliningItemElem = nodes.at( iNode ).toElement();
    if ( redliningItemElem.nodeName() == "RedliningItem" )
    {
      QgsFeature feature( mLayer->dataProvider()->fields() );
      feature.setAttribute( "size", redliningItemElem.attribute( "size", "1" ) );
      feature.setAttribute( "outline", redliningItemElem.attribute( "outline", "#FF0000" ) );
      feature.setAttribute( "fill", redliningItemElem.attribute( "fill", "#0000FF" ) );
      feature.setAttribute( "text", redliningItemElem.attribute( "text", "" ) );
      feature.setAttribute( "text_x", redliningItemElem.attribute( "text_x", "" ) );
      feature.setAttribute( "text_y", redliningItemElem.attribute( "text_y", "" ) );
      feature.setGeometry( QgsGeometry::fromWkt( redliningItemElem.attribute( "geometry", "" ) ) );
      features.append( feature );
    }
  }
  mLayer->dataProvider()->addFeatures( features );
  mLayer->updateFields();
  mApp->mapCanvas()->refresh();
}

void QgsRedlining::writeProject( QDomDocument& doc )
{
  getOrCreateLayer();

  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement redliningElem = doc.createElement( "Redlining" );
  QgsFeatureIterator it = mLayer->getFeatures();
  QgsFeature feature;
  while ( it.nextFeature( feature ) )
  {
    QDomElement redliningItemElem = doc.createElement( "RedliningItem" );
    redliningItemElem.setAttribute( "size", feature.attribute( "size" ).toString() );
    redliningItemElem.setAttribute( "outline", feature.attribute( "outline" ).toString() );
    redliningItemElem.setAttribute( "fill", feature.attribute( "fill" ).toString() );
    redliningItemElem.setAttribute( "text", feature.attribute( "text" ).toString() );
    redliningItemElem.setAttribute( "text_x", feature.attribute( "text_x" ).toString() );
    redliningItemElem.setAttribute( "text_y", feature.attribute( "text_y" ).toString() );
    redliningItemElem.setAttribute( "geometry", feature.geometry()->exportToWkt() );
    redliningElem.appendChild( redliningItemElem );
  }
  qgisElem.appendChild( redliningElem );
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningCircleMapTool::QgsRedliningCircleMapTool( QgsMapCanvas* canvas , QgsVectorLayer *layer )
    : QgsMapTool( canvas ), mLayer( layer ), mGeometry( 0 ), mRubberBand( 0 )
{
  setCursor( Qt::CrossCursor );
}

QgsRedliningCircleMapTool::~QgsRedliningCircleMapTool()
{
  delete mRubberBand;
  delete mGeometry;
}

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

void QgsRedliningCircleMapTool::canvasPressEvent( QMouseEvent *e )
{
  if ( e->button() == Qt::LeftButton )
  {
    mPressPos = e->pos();
    mRubberBand = new QgsRubberBand( mCanvas );
  }
}

void QgsRedliningCircleMapTool::canvasReleaseEvent( QMouseEvent */*e*/ )
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
    : QgsMapTool( canvas ), mLayer( layer ), mMode( NoSelection ), mRubberBand( 0 ), mCurrentFeature( 0 ), mCurrentVertex( -1 )
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
  QgsRectangle selectRect( mPressPos.x() - r, mPressPos.y() - r, mPressPos.x() + r, mPressPos.y() + r );
  QgsFeature feature;
  QgsFeatureIterator fit = mLayer->getFeatures( QgsFeatureRequest().setFilterRect( selectRect ).setSubsetOfAttributes( QgsAttributeList() ) );
  if ( fit.nextFeature( feature ) )
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

  // Else, nothing is selected
  mMode = NoSelection;
  delete mCurrentFeature;
  mCurrentFeature = 0;
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
      mCurrentFeature->geometry()->moveVertex( layerPos, mCurrentVertex );
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
    mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().fieldNameIndex( "text_x" ), mCurrentLabel.labelRect.xMinimum() );
    mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().fieldNameIndex( "text_y" ), mCurrentLabel.labelRect.yMinimum() );
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

void QgsRedliningEditTool::onStyleChanged()
{
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
  delete mCurrentFeature;
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
