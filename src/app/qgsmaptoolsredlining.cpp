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
    mGeometry = 0; // no delete since ownership taken by QgsGeometry above
    delete mRubberBand;
    mRubberBand = 0;
  }
}

///////////////////////////////////////////////////////////////////////////////

void QgsRedliningPointMapTool::canvasReleaseEvent( QMouseEvent *e )
{
  QgsFeature f( mLayer->pendingFields() );
  QString flags = QString( "symbol=%1,w=5*\"size\",h=5*\"size\",r=0" ).arg( mShape );
  f.setAttribute( "flags", flags );
  f.setGeometry( new QgsGeometry( new QgsPointV2( toLayerCoordinates( mLayer, e->pos() ) ) ) );
  mLayer->addFeature( f );
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
    static_cast<QgsPolygonV2*>( mGeometry )->setExteriorRing( exterior );
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
    static_cast<QgsCurvePolygonV2*>( mGeometry )->setExteriorRing( exterior );
    QgsGeometry geom( mGeometry->segmentize() );
    mRubberBand->setToGeometry( &geom, mLayer );
  }
}

///////////////////////////////////////////////////////////////////////////////

void QgsRedliningTextTool::canvasReleaseEvent( QMouseEvent *e )
{
  QgsRedliningTextDialog textDialog( "", "" );
  if ( textDialog.exec() == QDialog::Accepted && !textDialog.currentText().isEmpty() )
  {
    QgsPoint pos = toLayerCoordinates( mLayer, e->pos() );
    QgsFeature f( mLayer->pendingFields() );
    f.setAttribute( "text", textDialog.currentText() );
    f.setAttribute( "text_x", pos.x() );
    f.setAttribute( "text_y", pos.y() );
    QFont font = textDialog.currentFont();
    f.setAttribute( "flags", QString( "family=%1,italic=%2,bold=%3" ).arg( font.family() ).arg( font.italic() ).arg( font.bold() ) );
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
  if ( fit.nextFeature( feature ) && feature.attribute( "text" ).toString().isEmpty() )
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
    mCanvas->clearCache( mLayer->id() );
    mCanvas->refresh();
  }
  else if ( mMode == FeatureSelected )
  {
    mLayer->changeGeometry( mCurrentFeature->featureId(), mCurrentFeature->geometry() );
    mCurrentFeature->deselectVertex( mCurrentVertex );
    mCanvas->clearCache( mLayer->id() );
    mCanvas->refresh();
  }
}

void QgsRedliningEditTool::canvasDoubleClickEvent( QMouseEvent */*e*/ )
{
  QgsFeature feature;
  if ( mMode == TextSelected && mLayer->getFeatures( QgsFeatureRequest( mCurrentLabel.featureId ) ).nextFeature( feature ) )
  {
    QgsRedliningTextDialog textDialog( mCurrentLabel.labelText, mCurrentLabel.labelFont.toString() );
    if ( textDialog.exec() == QDialog::Accepted && !textDialog.currentText().isEmpty() )
    {
      mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().indexFromName( "text" ), textDialog.currentText() );
      QFont font = textDialog.currentFont();
      QString flags = QString( "family=%1,italic=%2,bold=%3" ).arg( font.family() ).arg( font.italic() ).arg( font.bold() );
      QgsDebugMsg( flags );
      mLayer->changeAttributeValue( mCurrentLabel.featureId, mLayer->pendingFields().indexFromName( "flags" ), flags );
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
