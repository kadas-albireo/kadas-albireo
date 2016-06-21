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

#include "qgsmaptoolsredlining.h"
#include "qgscircularstringv2.h"
#include "qgscrscache.h"
#include "qgscurvepolygonv2.h"
#include "qgsgeometryutils.h"
#include "qgslinestringv2.h"
#include "qgsmapcanvas.h"
#include "qgspallabeling.h"
#include "qgspolygonv2.h"
#include "qgsredliningtextdialog.h"
#include "qgsrubberband.h"
#include "qgsvectorlayer.h"
#include "nodetool/qgsselectedfeature.h"
#include "nodetool/qgsvertexentry.h"

#include <QSettings>
#include <QMenu>
#include <QMouseEvent>


QgsRedliningPointMapTool::QgsRedliningPointMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& shape , QgsRedliningAttributeEditor* editor )
    : QgsMapToolDrawPoint( canvas ), mLayer( layer ), mShape( shape ), mEditor( editor )
{
  setShowInputWidget( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  connect( this, SIGNAL( finished() ), this, SLOT( onFinished() ) );
}

QgsRedliningPointMapTool::~QgsRedliningPointMapTool()
{
  delete mEditor;
}

void QgsRedliningPointMapTool::onFinished()
{
  QgsFeature f( mLayer->pendingFields() );
  QString flags = QString( "symbol=%1,w=2*\"size\",h=2*\"size\",r=0" ).arg( mShape );
  f.setAttribute( "flags", flags );
  f.setGeometry( new QgsGeometry( createGeometry( mLayer->crs() ) ) );
  QStringList changedAttributes;
  if ( mEditor )
  {
    if ( !mEditor->exec( f, changedAttributes ) )
    {
      return;
    }
  }
  mLayer->addFeature( f );
  reset();
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningRectangleMapTool::QgsRedliningRectangleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer )
    : QgsMapToolDrawRectangle( canvas ), mLayer( layer )
{
  setShowInputWidget( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  setMeasurementMode( QgsGeometryRubberBand::MEASURE_RECTANGLE, QGis::Meters );
  connect( this, SIGNAL( finished() ), this, SLOT( onFinished() ) );
}

void QgsRedliningRectangleMapTool::onFinished()
{
  QgsFeature f( mLayer->pendingFields() );
  f.setAttribute( "flags", "rect:" + mCanvas->mapSettings().destinationCrs().authid() );
  f.setGeometry( new QgsGeometry( createGeometry( mLayer->crs() ) ) );
  mLayer->addFeature( f );
  reset();
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningPolylineMapTool::QgsRedliningPolylineMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, bool closed , QgsRedliningAttributeEditor *editor )
    : QgsMapToolDrawPolyLine( canvas, closed ), mLayer( layer ), mEditor( editor )
{
  setShowInputWidget( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  setMeasurementMode( closed ? QgsGeometryRubberBand::MEASURE_POLYGON : QgsGeometryRubberBand::MEASURE_LINE_AND_SEGMENTS, QGis::Meters );
  connect( this, SIGNAL( finished() ), this, SLOT( onFinished() ) );
}

QgsRedliningPolylineMapTool::~QgsRedliningPolylineMapTool()
{
  delete mEditor;
}

void QgsRedliningPolylineMapTool::onFinished()
{
  QgsFeature f( mLayer->pendingFields() );
  f.setGeometry( new QgsGeometry( createGeometry( mLayer->crs() ) ) );
  QStringList changedAttributes;
  if ( mEditor )
    mEditor->exec( f, changedAttributes );
  mLayer->addFeature( f );
  reset();
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningCircleMapTool::QgsRedliningCircleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer )
    : QgsMapToolDrawCircle( canvas ), mLayer( layer )
{
  setShowInputWidget( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  setMeasurementMode( QgsGeometryRubberBand::MEASURE_CIRCLE, QGis::Meters );
  connect( this, SIGNAL( finished() ), this, SLOT( onFinished() ) );
}

void QgsRedliningCircleMapTool::onFinished()
{
  QgsFeature f( mLayer->pendingFields() );
  f.setGeometry( new QgsGeometry( createGeometry( mLayer->crs() ) ) );
  mLayer->addFeature( f );
  reset();
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningEditTool::QgsRedliningEditTool( QgsMapCanvas* canvas , QgsVectorLayer *layer , QgsRedliningAttributeEditor *editor )
    : QgsMapToolPan( canvas ), mLayer( layer ), mEditor( editor ), mMode( NoSelection ), mRubberBand( 0 ), mCurrentFeature( 0 ), mCurrentVertex( -1 ), mIsRectangle( false ), mUnsetOnMiss( false )
{
  connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( updateLabelBoundingBox() ) );
  setCursor( Qt::CrossCursor );
}

QgsRedliningEditTool::~QgsRedliningEditTool()
{
  delete mRubberBand;
  delete mCurrentFeature;
}

void QgsRedliningEditTool::canvasPressEvent( QMouseEvent *e )
{
  mPrevPos = mPressPos = toMapCoordinates( e->pos() );
  QgsPoint pressLayerPos = toLayerCoordinates( mLayer, mPressPos );

  const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
  QList<QgsLabelPosition> labelPositions = labelingResults ? labelingResults->labelsAtPosition( mPressPos ) : QList<QgsLabelPosition>();

  double r = QgsTolerance::vertexSearchRadius( mCanvas->currentLayer(), mCanvas->mapSettings() );
  QgsRectangle selectRect( pressLayerPos.x() - r, pressLayerPos.y() - r, pressLayerPos.x() + r, pressLayerPos.y() + r );
  QgsFeature feature;

  // Check whether we can keep the same selection as before
  if ( mMode == TextSelected && labelingResults )
  {
    foreach ( const QgsLabelPosition& labelPos, labelPositions )
    {
      if ( labelPos.layerID == mLayer->id() && labelPos.featureId == mCurrentLabel.featureId )
      {
        if ( e->button() == Qt::RightButton )
        {
          showContextMenu( e );
        }
        return;
      }
    }
  }
  else if ( mMode == FeatureSelected )
  {
    QgsFeatureIterator fit = mLayer->getFeatures( QgsFeatureRequest( selectRect ).setFlags( QgsFeatureRequest::NoGeometry | QgsFeatureRequest::SubsetOfAttributes ) );
    while ( fit.nextFeature( feature ) )
    {
      if ( feature.id() == mCurrentFeature->featureId() )
      {
        checkVertexSelection();
        if ( e->button() == Qt::RightButton )
        {
          showContextMenu( e );
        }
        return;
      }
    }
  }

  // Else, quit
  if ( mUnsetOnMiss )
  {
    delete mRubberBand;
    mRubberBand = 0;
    delete mCurrentFeature;
    mCurrentFeature = 0;
    QgsMapToolPan::canvasPressEvent( e );
    canvas()->unsetMapTool( this );
    return;
  }
  clearCurrent( true );

  // First, look for a label below the cursor
  foreach ( const QgsLabelPosition& labelPos, labelPositions )
  {
    if ( labelPos.layerID == mLayer->id() )
    {
      selectLabel( labelPos );
      return;
    }
  }

  // Then, look for a feature below the cursor
  QgsFeatureIterator fit = mLayer->getFeatures( QgsFeatureRequest( selectRect ) );
  if ( fit.nextFeature( feature ) && feature.attribute( "text" ).toString().isEmpty() )
  {
    selectFeature( feature );
  }
}

void QgsRedliningEditTool::selectLabel( const QgsLabelPosition &labelPos )
{
  mCurrentLabel = labelPos;
  mMode = TextSelected;
  mRubberBand = new QgsGeometryRubberBand( mCanvas, QGis::Line );
  const QgsRectangle& rect = mCurrentLabel.labelRect;
  QgsLineStringV2* geom = new QgsLineStringV2();
  geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMinimum() ) );
  geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMaximum() ) );
  geom->addVertex( QgsPointV2( rect.xMaximum(), rect.yMaximum() ) );
  geom->addVertex( QgsPointV2( rect.xMaximum(), rect.yMinimum() ) );
  geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMinimum() ) );
  mRubberBand->setGeometry( geom );
  mRubberBand->setOutlineColor( QColor( 0, 255, 0, 150 ) );
  mRubberBand->setOutlineWidth( 3 );
  mRubberBand->setLineStyle( Qt::DotLine );
  QgsFeature f;
  mLayer->getFeatures( QgsFeatureRequest( mCurrentLabel.featureId ) ).nextFeature( f );
  mLabelIsForPoint = f.geometry()->type() == QGis::Point;
  emit featureSelected( f );
}

void QgsRedliningEditTool::selectFeature( const QgsFeature &feature )
{
  mMode = FeatureSelected;
  emit featureSelected( feature );
  mRubberBand = new QgsGeometryRubberBand( mCanvas, feature.geometry()->type() );
  mRubberBand->setOutlineColor( QColor( 0, 255, 0, 170 ) );
  mRubberBand->setFillColor( QColor( 0, 255, 0, 15 ) );
  mRubberBand->setOutlineWidth( 3 );
  mRubberBand->setLineStyle( Qt::DotLine );
  mCurrentFeature = new QgsSelectedFeature( feature.id(), mLayer, mCanvas );
  connect( mCurrentFeature.data(), SIGNAL( destroyed( QObject* ) ), this, SLOT( clearCurrent() ) ); // When current layer changes

  mIsRectangle = false;
  foreach ( const QString& flag, feature.attribute( "flags" ).toString().split( "," ) )
  {
    if ( flag.startsWith( "rect" ) )
    {
      mIsRectangle = true;
      mRectangleCRS = flag.mid( 5 );
    }
  }
  if ( feature.geometry()->geometry()->wkbType() == QgsWKBTypes::LineString )
  {
    mRubberBand->setMeasurementMode( QgsGeometryRubberBand::MEASURE_LINE_AND_SEGMENTS, QGis::Meters );
  }
  else if ( feature.geometry()->geometry()->wkbType() == QgsWKBTypes::Polygon )
  {
    mRubberBand->setMeasurementMode( mIsRectangle ? QgsGeometryRubberBand::MEASURE_RECTANGLE : QgsGeometryRubberBand::MEASURE_POLYGON, QGis::Meters );
  }
  else if ( feature.geometry()->geometry()->wkbType() == QgsWKBTypes::CurvePolygon )
  {
    mRubberBand->setMeasurementMode( mIsRectangle ? QgsGeometryRubberBand::MEASURE_CIRCLE : QgsGeometryRubberBand::MEASURE_CIRCLE, QGis::Meters );
  }
  QgsCoordinateTransform ct( mLayer->crs(), mCanvas->mapSettings().destinationCrs() );
  mRubberBand->setGeometry( feature.geometry()->geometry()->transformed( ct ) );
  checkVertexSelection();
}

void QgsRedliningEditTool::checkVertexSelection()
{
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
}

void QgsRedliningEditTool::showContextMenu( QMouseEvent *e )
{
  QMenu menu;
  QAction* actionRemoveObject = 0;
  QAction* actionRemoveNode = 0;
  QAction* actionAddNode = 0;
  QAction* actionEdit = 0;
  QgsFeatureId currentFeatureId = mCurrentFeature ? mCurrentFeature->featureId() : mCurrentLabel.featureId;

  if ( mCurrentFeature && mCurrentFeature->geometry()->type() != QGis::Point && !mIsRectangle )
  {
    if ( mCurrentVertex != -1 )
    {
      actionRemoveNode = menu.addAction( tr( "Remove node" ) );
    }
    else
    {
      actionAddNode = menu.addAction( tr( "Add node" ) );
    }
    menu.addSeparator();
  }
  actionRemoveObject = menu.addAction( tr( "Remove object" ) );
  if ( mEditor != 0 )
  {
    menu.addSeparator();
    actionEdit = menu.addAction( tr( "Edit %1" ).arg( mEditor->getName() ) );
  }
  QAction* clickedAction = menu.exec( e->globalPos() );
  if ( clickedAction == 0 )
  {
    // pass
  }
  else if ( clickedAction == actionRemoveNode )
  {
    deleteCurrentVertex();
  }
  else if ( clickedAction == actionAddNode )
  {
    addVertex( e->pos() );
  }
  else if ( clickedAction == actionRemoveObject )
  {
    mLayer->deleteFeature( currentFeatureId );
    clearCurrent();
  }
  else if ( clickedAction == actionEdit )
  {
    runEditor( currentFeatureId );
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
        const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( mCurrentFeature->vlayer()->crs().authid(), mRectangleCRS );
        int n = mCurrentFeature->vertexMap().size() - 1;
        int iPrev = ( mCurrentVertex - 1 + n ) % n;
        int iNext = ( mCurrentVertex + 1 + n ) % n;
        QgsPoint pPrev = t->transform( mCurrentFeature->vertexMap()[iPrev]->pointV1() );
        QgsPoint pNext = t->transform( mCurrentFeature->vertexMap()[iNext]->pointV1() );
        QgsPoint pCurr = t->transform( layerPos );
        if (( mCurrentVertex % 2 ) == 0 )
        {
          mCurrentFeature->geometry()->moveVertex( t->transform( QgsPoint( pCurr.x(), pPrev.y() ), QgsCoordinateTransform::ReverseTransform ), iPrev );
          mCurrentFeature->geometry()->moveVertex( t->transform( QgsPoint( pNext.x(), pCurr.y() ), QgsCoordinateTransform::ReverseTransform ), iNext );
        }
        else
        {
          mCurrentFeature->geometry()->moveVertex( t->transform( QgsPoint( pPrev.x(), pCurr.y() ), QgsCoordinateTransform::ReverseTransform ), iPrev );
          mCurrentFeature->geometry()->moveVertex( t->transform( QgsPoint( pCurr.x(), pNext.y() ), QgsCoordinateTransform::ReverseTransform ), iNext );
        }
      }
    }
    else
    {
      mCurrentFeature->geometry()->translate( layerPos.x() - prevLayerPos.x(), layerPos.y() - prevLayerPos.y() );
    }
    mCurrentFeature->updateVertexMarkersPosition();
    const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( mLayer->crs().authid(), mCanvas->mapSettings().destinationCrs().authid() );
    mRubberBand->setGeometry( mCurrentFeature->geometry()->geometry()->transformed( *ct ) );
  }
  else if ( mMode == TextSelected && mLabelIsForPoint )
  {
    mRubberBand->setTranslationOffset( p.x() - mPressPos.x(), p.y() - mPressPos.y() );
    mRubberBand->updatePosition();
  }
  mPrevPos = p;
}

void QgsRedliningEditTool::canvasReleaseEvent( QMouseEvent */*e*/ )
{
  if ( mMode == TextSelected && mLabelIsForPoint )
  {
    double dx, dy;
    mRubberBand->translationOffset( dx, dy );
    QgsFeature f;
    mLayer->getFeatures( QgsFeatureRequest( mCurrentLabel.featureId ) ).nextFeature( f );
    QgsPointV2* prevLayerPos = static_cast<QgsPointV2*>( f.geometry()->geometry() );
    QgsPoint prevPos = toMapCoordinates( mLayer, QgsPoint( prevLayerPos->x(), prevLayerPos->y() ) );
    prevPos.setX( prevPos.x() + dx );
    prevPos.setY( prevPos.y() + dy );
    mCurrentLabel.cornerPoints[0] += QgsVector( dx, dy );
    mCurrentLabel.cornerPoints[1] += QgsVector( dx, dy );
    mCurrentLabel.cornerPoints[2] += QgsVector( dx, dy );
    mCurrentLabel.cornerPoints[3] += QgsVector( dx, dy );
    QgsRectangle& labelRect = mCurrentLabel.labelRect;
    labelRect.setXMinimum( labelRect.xMinimum() + dx );
    labelRect.setYMinimum( labelRect.yMinimum() + dy );
    labelRect.setXMaximum( labelRect.xMaximum() + dx );
    labelRect.setYMaximum( labelRect.yMaximum() + dy );
    mRubberBand->setTranslationOffset( 0, 0 );
    QgsLineStringV2* geom = new QgsLineStringV2();
    geom->addVertex( QgsPointV2( labelRect.xMinimum(), labelRect.yMinimum() ) );
    geom->addVertex( QgsPointV2( labelRect.xMinimum(), labelRect.yMaximum() ) );
    geom->addVertex( QgsPointV2( labelRect.xMaximum(), labelRect.yMaximum() ) );
    geom->addVertex( QgsPointV2( labelRect.xMaximum(), labelRect.yMinimum() ) );
    geom->addVertex( QgsPointV2( labelRect.xMinimum(), labelRect.yMinimum() ) );
    mRubberBand->setGeometry( geom );
    QgsPoint pos = toLayerCoordinates( mLayer, prevPos );
    mLayer->changeGeometry( mCurrentLabel.featureId, new QgsGeometry( new QgsPointV2( pos.x(), pos.y() ) ) );
    mLayer->triggerRepaint();
  }
  else if ( mMode == FeatureSelected )
  {
    mLayer->changeGeometry( mCurrentFeature->featureId(), mCurrentFeature->geometry() );
    if ( mCurrentVertex >= 0 )
      mCurrentFeature->selectVertex( mCurrentVertex );
    mLayer->triggerRepaint();
  }
}

void QgsRedliningEditTool::canvasDoubleClickEvent( QMouseEvent *e )
{
  if ( mMode == TextSelected )
  {
    runEditor( mCurrentLabel.featureId );
  }
  else if ( mMode == FeatureSelected && mCurrentFeature->geometry()->type() != QGis::Point && !mIsRectangle )
  {
    addVertex( e->pos() );
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
      if ( mCurrentFeature && mCurrentVertex >= 0 )
      {
        deleteCurrentVertex();
      }
      else
      {
        mLayer->deleteFeature( mCurrentFeature->featureId() );
        clearCurrent();
      }
      e->ignore();
    }
  }
  else if ( e->key() == Qt::Key_Escape )
  {
    canvas()->unsetMapTool( this );
  }
}

void QgsRedliningEditTool::addVertex( const QPoint &pos )
{
  QgsSnapper snapper( mCanvas->mapSettings() );
  snapper.setSnapMode( QgsSnapper::SnapWithResultsWithinTolerances );
  QgsSnapper::SnapLayer snapLayer;
  snapLayer.mLayer = mLayer;
  snapLayer.mSnapTo = QgsSnapper::SnapToSegment;
  snapLayer.mTolerance = 10;
  snapLayer.mUnitType = QgsTolerance::Pixels;
  snapper.setSnapLayers( QList<QgsSnapper::SnapLayer>() << snapLayer );
  QList<QgsSnappingResult> results;
  snapper.snapMapPoint( toMapCoordinates( pos ), results );
  foreach ( const QgsSnappingResult& result, results )
  {
    if ( result.snappedAtGeometry == mCurrentFeature->featureId() && result.afterVertexNr != -1 )
    {
      QgsPoint layerCoord = toLayerCoordinates( mLayer, results.first().snappedVertex );
      mLayer->insertVertex( layerCoord.x(), layerCoord.y(), mCurrentFeature->featureId(), result.afterVertexNr );
      const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( mLayer->crs().authid(), mCanvas->mapSettings().destinationCrs().authid() );
      mRubberBand->setGeometry( mCurrentFeature->geometry()->geometry()->transformed( *ct ) );
      mLayer->triggerRepaint();
      break;
    }
  }
}

void QgsRedliningEditTool::deleteCurrentVertex()
{
  if ( !mIsRectangle )
  {
    mCurrentFeature->deleteSelectedVertexes();
    mCurrentVertex = -1;
    const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( mLayer->crs().authid(), mCanvas->mapSettings().destinationCrs().authid() );
    mRubberBand->setGeometry( mCurrentFeature->geometry()->geometry()->transformed( *ct ) );
    mLayer->triggerRepaint();
  }
}

void QgsRedliningEditTool::runEditor( const QgsFeatureId& featureId )
{
  QgsFeature feature;
  mLayer->getFeatures( QgsFeatureRequest( featureId ) ).nextFeature( feature );
  QStringList changedAttributes;
  if ( mEditor && mEditor->exec( feature, changedAttributes ) )
  {
    foreach ( const QString& attrib, changedAttributes )
    {
      mLayer->changeAttributeValue( feature.id(), mLayer->pendingFields().indexFromName( attrib ), feature.attribute( attrib ) );
    }
    mLayer->triggerRepaint();
  }
}

void QgsRedliningEditTool::deactivate()
{
  QgsMapTool::deactivate();
  clearCurrent();
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
  mIsRectangle = false;
  if ( mCurrentFeature && mCurrentVertex >= 0 )
    mCurrentFeature->deselectVertex( mCurrentVertex );
  delete mCurrentFeature.data();
  mCurrentFeature = 0;
  mCurrentVertex = -1;
  if ( refresh )
  {
    mLayer->triggerRepaint();
  }
}

void QgsRedliningEditTool::updateLabelBoundingBox()
{
  if ( mMode == TextSelected )
  {
    // Try to find the label again
    const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
    if ( labelingResults )
    {
      foreach ( const QgsLabelPosition& labelPos, labelingResults->labelsWithinRect( mCurrentLabel.labelRect ) )
      {
        if ( labelPos.layerID == mCurrentLabel.layerID &&
             labelPos.featureId == mCurrentLabel.featureId &&
             labelPos.labelRect != mCurrentLabel.labelRect )
        {
          mCurrentLabel = labelPos;
          const QgsRectangle& rect = mCurrentLabel.labelRect;
          mRubberBand->setTranslationOffset( 0, 0 );
          QgsLineStringV2* geom = new QgsLineStringV2();
          geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMinimum() ) );
          geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMaximum() ) );
          geom->addVertex( QgsPointV2( rect.xMaximum(), rect.yMaximum() ) );
          geom->addVertex( QgsPointV2( rect.xMaximum(), rect.yMinimum() ) );
          geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMinimum() ) );
          mRubberBand->setGeometry( geom );
          return;
        }
      }
    }
    // Label disappeared? Should not happen
  }
}
