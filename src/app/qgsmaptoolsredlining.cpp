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
#include <QMouseEvent>

QgsRedliningPointMapTool::QgsRedliningPointMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& shape )
    : QgsMapToolDrawPoint( canvas ), mLayer( layer ), mShape( shape )
{
}

void QgsRedliningPointMapTool::onFinished()
{
  QgsFeature f( mLayer->pendingFields() );
  QString flags = QString( "symbol=%1,w=3*\"size\",h=3*\"size\",r=0" ).arg( mShape );
  f.setAttribute( "flags", flags );
  f.setGeometry( new QgsGeometry( createGeometry( mLayer->crs() ) ) );
  mLayer->addFeature( f );
  reset();
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningRectangleMapTool::QgsRedliningRectangleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer )
    : QgsMapToolDrawRectangle( canvas ), mLayer( layer )
{
  setMeasurementMode( QgsGeometryRubberBand::MEASURE_RECTANGLE, QGis::Meters );
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

QgsRedliningPolylineMapTool::QgsRedliningPolylineMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, bool closed )
    : QgsMapToolDrawPolyLine( canvas, closed ), mLayer( layer )
{
  setMeasurementMode( closed ? QgsGeometryRubberBand::MEASURE_POLYGON : QgsGeometryRubberBand::MEASURE_LINE_AND_SEGMENTS, QGis::Meters );
}

void QgsRedliningPolylineMapTool::onFinished()
{
  QgsFeature f( mLayer->pendingFields() );
  f.setGeometry( new QgsGeometry( createGeometry( mLayer->crs() ) ) );
  mLayer->addFeature( f );
  reset();
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningCircleMapTool::QgsRedliningCircleMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer )
    : QgsMapToolDrawCircle( canvas ), mLayer( layer )
{
  setMeasurementMode( QgsGeometryRubberBand::MEASURE_CIRCLE, QGis::Meters );
}

void QgsRedliningCircleMapTool::onFinished()
{
  QgsFeature f( mLayer->pendingFields() );
  f.setGeometry( new QgsGeometry( createGeometry( mLayer->crs() ) ) );
  mLayer->addFeature( f );
  reset();
}

///////////////////////////////////////////////////////////////////////////////

void QgsRedliningTextTool::canvasReleaseEvent( QMouseEvent *e )
{
  QgsRedliningTextDialog textDialog( "", "", 0 );
  if ( textDialog.exec() == QDialog::Accepted && !textDialog.currentText().isEmpty() )
  {
    QgsPoint pos = toLayerCoordinates( mLayer, e->pos() );
    QgsFeature f( mLayer->pendingFields() );
    f.setAttribute( "text", textDialog.currentText() );
    f.setAttribute( "text_x", pos.x() );
    f.setAttribute( "text_y", pos.y() );
    QFont font = textDialog.currentFont();
    f.setAttribute( "flags", QString( "family=%1,italic=%2,bold=%3,rotation=%4" ).arg( font.family() ).arg( font.italic() ).arg( font.bold() ).arg( textDialog.rotation() ) );
    f.setGeometry( new QgsGeometry( new QgsPointV2( pos.x(), pos.y() ) ) );
    mLayer->addFeature( f );
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
      if ( labelPos.layerID == mLayer->id() && labelPos.labelRect == mCurrentLabel.labelRect )
      {
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
        return;
      }
    }
  }

  // Else, start anew
  clearCurrent( true );

  // First, look for a label below the cursor
  foreach ( const QgsLabelPosition& labelPos, labelPositions )
  {
    if ( labelPos.layerID == mLayer->id() )
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
      emit featureSelected( mCurrentLabel.featureId );
      return;
    }
  }

  // Then, look for a feature below the cursor
  QgsFeatureIterator fit = mLayer->getFeatures( QgsFeatureRequest( selectRect ) );
  if ( fit.nextFeature( feature ) && feature.attribute( "text" ).toString().isEmpty() )
  {
    mMode = FeatureSelected;
    emit featureSelected( feature.id() );
    mRubberBand = new QgsGeometryRubberBand( mCanvas, feature.geometry()->type() );
    mRubberBand->setOutlineColor( QColor( 0, 255, 0, 170 ) );
    mRubberBand->setFillColor( QColor( 0, 255, 0, 15 ) );
    mRubberBand->setOutlineWidth( 3 );
    mRubberBand->setLineStyle( Qt::DotLine );
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
    return;
  }
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
    QgsCoordinateTransform ct( mLayer->crs(), mCanvas->mapSettings().destinationCrs() );
    mRubberBand->setGeometry( mCurrentFeature->geometry()->geometry()->transformed( ct ) );
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
    double bboxOffsetX = mCurrentLabel.cornerPoints[0].x() - mCurrentLabel.labelRect.xMinimum();
    double bboxOffsetY = mCurrentLabel.cornerPoints[0].y() - mCurrentLabel.labelRect.yMinimum();
    mRubberBand->translationOffset( dx, dy );
    QgsRectangle& rect = mCurrentLabel.labelRect;
    rect.setXMinimum( rect.xMinimum() + dx );
    rect.setYMinimum( rect.yMinimum() + dy );
    rect.setXMaximum( rect.xMaximum() + dx );
    rect.setYMaximum( rect.yMaximum() + dy );
    mCurrentLabel.cornerPoints[0] += QgsVector( dx, dy );
    mCurrentLabel.cornerPoints[1] += QgsVector( dx, dy );
    mCurrentLabel.cornerPoints[2] += QgsVector( dx, dy );
    mCurrentLabel.cornerPoints[3] += QgsVector( dx, dy );
    mRubberBand->setTranslationOffset( 0, 0 );
    QgsLineStringV2* geom = new QgsLineStringV2();
    geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMinimum() ) );
    geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMaximum() ) );
    geom->addVertex( QgsPointV2( rect.xMaximum(), rect.yMaximum() ) );
    geom->addVertex( QgsPointV2( rect.xMaximum(), rect.yMinimum() ) );
    geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMinimum() ) );
    mRubberBand->setGeometry( geom );
    QgsPoint pos = toLayerCoordinates( mLayer, QgsPoint( rect.xMinimum() + bboxOffsetX, rect.yMinimum() + bboxOffsetY ) );
    mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().fieldNameIndex( "text_x" ), pos.x() );
    mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().fieldNameIndex( "text_y" ), pos.y() );
    mCanvas->clearCache( mLayer->id() );
    mCanvas->refresh();
  }
  else if ( mMode == FeatureSelected )
  {
    mLayer->changeGeometry( mCurrentFeature->featureId(), mCurrentFeature->geometry() );
    if ( mCurrentVertex >= 0 )
      mCurrentFeature->selectVertex( mCurrentVertex );
    mCanvas->clearCache( mLayer->id() );
    mCanvas->refresh();
  }
}

void QgsRedliningEditTool::canvasDoubleClickEvent( QMouseEvent *e )
{
  QgsFeature feature;
  if ( mMode == TextSelected && mLayer->getFeatures( QgsFeatureRequest( mCurrentLabel.featureId ) ).nextFeature( feature ) )
  {
    QgsRedliningTextDialog textDialog( mCurrentLabel.labelText, mCurrentLabel.labelFont.toString(), mCurrentLabel.rotation / M_PI * 180. );
    if ( textDialog.exec() == QDialog::Accepted && !textDialog.currentText().isEmpty() )
    {
      mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().indexFromName( "text" ), textDialog.currentText() );
      QFont font = textDialog.currentFont();
      QString flags = QString( "family=%1,italic=%2,bold=%3,rotation=%4" ).arg( font.family() ).arg( font.italic() ).arg( font.bold() ).arg( textDialog.rotation() );
      QgsDebugMsg( flags );
      mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().indexFromName( "flags" ), flags );
      mCanvas->refresh();
    }
    return;
  }
  else if ( mMode == FeatureSelected && mCurrentFeature->geometry()->type() != QGis::Point && !mIsRectangle )
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
    snapper.snapMapPoint( toMapCoordinates( e->pos() ), results );
    foreach ( const QgsSnappingResult& result, results )
    {
      if ( result.snappedAtGeometry == mCurrentFeature->featureId() && result.afterVertexNr != -1 )
      {
        QgsPoint layerCoord = toLayerCoordinates( mLayer, results.first().snappedVertex );
        mLayer->insertVertex( layerCoord.x(), layerCoord.y(), mCurrentFeature->featureId(), result.afterVertexNr );
        QgsCoordinateTransform ct( mLayer->crs(), mCanvas->mapSettings().destinationCrs() );
        mRubberBand->setGeometry( mCurrentFeature->geometry()->geometry()->transformed( ct ) );
        mCanvas->clearCache( mLayer->id() );
        mCanvas->refresh();
        break;
      }
    }
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
        if ( !mIsRectangle )
        {
          mCurrentFeature->deleteSelectedVertexes();
          mCurrentVertex = -1;
          QgsCoordinateTransform ct( mLayer->crs(), mCanvas->mapSettings().destinationCrs() );
          mRubberBand->setGeometry( mCurrentFeature->geometry()->geometry()->transformed( ct ) );
          mCanvas->clearCache( mLayer->id() );
          mCanvas->refresh();
        }
      }
      else
      {
        mLayer->deleteFeature( mCurrentFeature->featureId() );
        clearCurrent();
      }
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
  if ( mCurrentFeature && mCurrentVertex >= 0 )
    mCurrentFeature->deselectVertex( mCurrentVertex );
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
