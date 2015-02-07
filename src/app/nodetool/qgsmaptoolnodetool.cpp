/***************************************************************************
    qgsmaptoolnodetool.cpp  - add/move/delete vertex integrated in one tool
    ---------------------
    begin                : April 2009
    copyright            : (C) 2009 by Richard Kostecky
    email                : csf dot kostej at mail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaptoolnodetool.h"

#include "nodetool/qgsselectedfeature.h"
#include "nodetool/qgsvertexentry.h"

#include "qgisapp.h"
#include "qgslayertreeview.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"
#include "qgsrubberband.h"
#include "qgssnappingutils.h"
#include "qgsvectorlayer.h"

#include <QMouseEvent>
#include <QRubberBand>

//! Match filter that does not accept only one particular point
struct QgsExcludePointFilter : public QgsPointLocator::MatchFilter
{
  QgsExcludePointFilter( const QgsPoint& exclPoint ) : mExclPoint( exclPoint ) {}
  bool acceptMatch( const QgsPointLocator::Match& match ) override { return match.point() != mExclPoint; }
  QgsPoint mExclPoint;
};

//! Match filter that accepts only matches from a particular feature ID
struct QgsFeatureIdFilter : public QgsPointLocator::MatchFilter
{
  QgsFeatureIdFilter( const QgsFeatureId& fid ) : mFid( fid ) {}
  bool acceptMatch( const QgsPointLocator::Match& match ) override { return match.featureId() == mFid; }
  QgsFeatureId mFid;
};

QgsMapToolNodeTool::QgsMapToolNodeTool( QgsMapCanvas* canvas )
    : QgsMapToolEdit( canvas )
    , mSelectedFeature( 0 )
    , mSelectionRectangle( false )
    , mMoving( true )
    , mClicked( false )
    , mCtrl( false )
    , mSelectAnother( false )
    , mAnother( 0 )
    , mSelectionRubberBand( 0 )
    , mRect( NULL )
    , mIsPoint( false )
    , mDeselectOnRelease( -1 )
{
  mToolName =  tr( "Node tool" );
}

QgsMapToolNodeTool::~QgsMapToolNodeTool()
{
  cleanTool();
  delete mRect;
  delete mSelectionRubberBand;
}

void QgsMapToolNodeTool::createMovingRubberBands()
{
  int topologicalEditing = QgsProject::instance()->readNumEntry( "Digitizing", "/TopologicalEditing", 0 );

  Q_ASSERT( mSelectedFeature );

  QgsVectorLayer *vlayer = mSelectedFeature->vlayer();
  Q_ASSERT( vlayer );

  QList<QgsVertexEntry*> &vertexMap = mSelectedFeature->vertexMap();
  QgsGeometry* geometry = mSelectedFeature->geometry();
  int beforeVertex, afterVertex;
  int lastRubberBand = 0;
  int vertex;
  for ( int i = 0; i < vertexMap.size(); i++ )
  {
    // create rubberband
    if ( vertexMap[i]->isSelected() && !vertexMap[i]->isInRubberBand() )
    {
      geometry->adjacentVertices( i, beforeVertex, afterVertex );
      vertex = i;
      while ( beforeVertex !=  -1 )
      {
        // move forward NOTE: end if whole cycle is selected
        if ( vertexMap[beforeVertex]->isSelected() && beforeVertex != i ) // and take care of cycles
        {
          vertex = beforeVertex;
          geometry->adjacentVertices( vertex, beforeVertex, afterVertex );
        }
        else
        {
          // break if cycle is found
          break;
        }
      }
      // we have first vertex of moving part
      // create rubberband and set default paramaters
      QgsRubberBand* rb = new QgsRubberBand( mCanvas, QGis::Line );
      rb->setWidth( 2 );
      rb->setColor( Qt::blue );
      int index = 0;
      if ( beforeVertex != -1 ) // adding first point which is not moving
      {
        rb->addPoint( toMapCoordinates( vlayer, vertexMap[beforeVertex]->point() ), false );
        vertexMap[beforeVertex]->setRubberBandValues( true, lastRubberBand, index );
        index++;
      }
      while ( vertex != -1 && vertexMap[vertex]->isSelected() && !vertexMap[vertex]->isInRubberBand() )
      {
        // topology rubberband creation if needed
        if ( topologicalEditing )
        {
          createTopologyRubberBands( vlayer, vertexMap, vertex );
        }
        // adding point which will be moved
        rb->addPoint( toMapCoordinates( vlayer, vertexMap[vertex]->point() ), false );
        // setting values about added vertex
        vertexMap[vertex]->setRubberBandValues( true, lastRubberBand, index );
        index++;
        geometry->adjacentVertices( vertex, beforeVertex, vertex );
      }
      if ( vertex != -1 && !vertexMap[vertex]->isSelected() ) // add last point not moving if exists
      {
        rb->addPoint( toMapCoordinates( vlayer, vertexMap[vertex]->point() ), true );
        vertexMap[vertex]->setRubberBandValues( true, lastRubberBand, index );
        index++;
      }
      mRubberBands.append( rb );
      lastRubberBand++;
    }
  }
}

void QgsMapToolNodeTool::createTopologyRubberBands( QgsVectorLayer* vlayer, const QList<QgsVertexEntry*> &vertexMap, int vertex )
{
  QMultiMap<double, QgsSnappingResult> currentResultList;
  QgsGeometry *geometry = mSelectedFeature->geometry();

  // snap from current vertex
  currentResultList.clear();
  vlayer->snapWithContext( vertexMap[vertex]->point(), ZERO_TOLERANCE, currentResultList, QgsSnapper::SnapToVertex );
  QMultiMap<double, QgsSnappingResult>::iterator resultIt =  currentResultList.begin();

  for ( ; resultIt != currentResultList.end(); ++resultIt )
  {
    // move all other
    if ( mSelectedFeature->featureId() != resultIt.value().snappedAtGeometry )
    {
      if ( mTopologyMovingVertexes.contains( resultIt.value().snappedAtGeometry ) )
      {
        if ( mTopologyMovingVertexes[resultIt.value().snappedAtGeometry]->contains( resultIt.value().snappedVertexNr ) )
        {
          // skip vertex already exists in some rubberband
          continue;
        }
      }
      QgsRubberBand* trb = new QgsRubberBand( mCanvas, QGis::Line );
      mTopologyRubberBand.append( trb );
      int rbId = mTopologyRubberBand.size() - 1;
      trb->setWidth( 1 );
      trb->setColor( Qt::red );

      int tVertex = resultIt.value().snappedVertexNr;
      int tVertexBackup = -1, tVertexAfter = -1;
      int tVertexFirst = tVertex; // vertex number to check for cycling
      QgsFeature topolFeature;

      vlayer->getFeatures( QgsFeatureRequest().setFilterFid( resultIt.value().snappedAtGeometry ).setSubsetOfAttributes( QgsAttributeList() ) ).nextFeature( topolFeature );
      QgsGeometry* topolGeometry = topolFeature.geometry();

      while ( tVertex != -1 ) // looking for first vertex to rubberband
      {
        tVertexBackup = tVertex;
        topolGeometry->adjacentVertices( tVertex, tVertex, tVertexAfter );
        if ( tVertex == -1 || tVertex == tVertexFirst )
          break; // check if this is not first vertex of the feature or cycling error
        // if closest vertex is not from selected feature or is not selected end
        double dist;
        QgsPoint point = topolGeometry->vertexAt( tVertex );
        int at, before, after;
        geometry->closestVertex( point, at, before, after, dist );
        if ( dist > ZERO_TOLERANCE || !vertexMap[at]->isSelected() ) // problem with double precision
        {
          break; // found first vertex
        }
      }

      int movingPointIndex = 0;
      Vertexes* movingPoints = new Vertexes();
      Vertexes* addedPoints = 0;
      if ( mTopologyMovingVertexes.contains( resultIt.value().snappedAtGeometry ) )
      {
        addedPoints = mTopologyMovingVertexes[ resultIt.value().snappedAtGeometry ];
      }
      else
      {
        addedPoints = new Vertexes();
      }
      if ( tVertex == -1 ) // adding first point if needed
      {
        tVertex = tVertexBackup;
      }
      else
      {
        trb->addPoint( toMapCoordinates( vlayer, topolGeometry->vertexAt( tVertex ) ) );
        if ( tVertex == tVertexFirst ) // cycle first vertex need to be added also
        {
          movingPoints->insert( movingPointIndex );
        }
        movingPointIndex = 1;
        topolGeometry->adjacentVertices( tVertex, tVertexAfter, tVertex );
      }

      while ( tVertex != -1 )
      {
        // if closest vertex is not from selected feature or is not selected end
        double dist;
        QgsPoint point = topolGeometry->vertexAt( tVertex );
        int at, before, after;
        geometry->closestVertex( point, at, before, after, dist );
        // find first no matching vertex
        if ( dist > ZERO_TOLERANCE || !vertexMap[at]->isSelected() ) // problem with double precision
        {
          trb->addPoint( toMapCoordinates( vlayer, topolGeometry->vertexAt( tVertex ) ) );
          break; // found first vertex
        }
        else // add moving point to rubberband
        {
          if ( addedPoints->contains( tVertex ) )
            break; // just preventing to circle
          trb->addPoint( toMapCoordinates( vlayer, topolGeometry->vertexAt( tVertex ) ) );
          movingPoints->insert( movingPointIndex );
          movingPointIndex++;
          addedPoints->insert( tVertex );
        }
        topolGeometry->adjacentVertices( tVertex, tVertexAfter, tVertex );
      }
      mTopologyMovingVertexes.insert( resultIt.value().snappedAtGeometry, addedPoints );
      mTopologyRubberBandVertexes.insert( rbId, movingPoints );
    }
  }
}

void QgsMapToolNodeTool::canvasMoveEvent( QMouseEvent * e )
{
  if ( !mSelectedFeature || !mClicked )
    return;

  QgsVectorLayer* vlayer = mSelectedFeature->vlayer();
  Q_ASSERT( vlayer );

  mSelectAnother = false;

  if ( mMoving )
  {
    // create rubberband, if none exists
    if ( mRubberBands.empty() )
    {
      if ( mIsPoint )
      {
        QList<QgsVertexEntry*> &vertexMap = mSelectedFeature->vertexMap();
        for ( int i = 0; i < vertexMap.size(); i++ )
        {
          if ( vertexMap[i]->isSelected() )
          {
            QgsRubberBand* rb = createRubberBandMarker( vertexMap[i]->point(), vlayer );
            mRubberBands.append( rb );
          }
        }
      }
      createMovingRubberBands();

      mPosMapCoordBackup = toMapCoordinates( e->pos() );
    }
    else
    {
      // move rubberband
      QgsPoint posMapCoord, pressMapCoords;
      QgsExcludePointFilter excludePointFilter( mClosestMapVertex );
      QgsPointLocator::Match match = mCanvas->snappingUtils()->snapToMap( e->pos(), &excludePointFilter );
      if ( match.isValid() )
      {
        posMapCoord = match.point();
        pressMapCoords = mClosestMapVertex;
      }
      else
      {
        posMapCoord = toMapCoordinates( e->pos() );
        pressMapCoords = toMapCoordinates( mPressCoordinates );
      }

      QgsVector offset = posMapCoord - pressMapCoords;

      // handle points
      if ( mIsPoint )
      {
        for ( int i = 0; i < mRubberBands.size(); i++ )
        {
          mRubberBands[i]->setTranslationOffset( offset.x(), offset.y() );
        }
        return;
      }

      // move points
      QList<QgsVertexEntry*> &vertexMap = mSelectedFeature->vertexMap();
      for ( int i = 0; i < vertexMap.size(); i++ )
      {
        if ( !vertexMap[i]->isSelected() )
          continue;

        QgsPoint p = toMapCoordinates( vlayer, vertexMap[i]->point() ) + offset;

        mRubberBands[vertexMap[i]->rubberBandNr()]->movePoint( vertexMap[i]->rubberBandIndex(), p );

        if ( vertexMap[i]->rubberBandIndex() == 0 )
        {
          mRubberBands[vertexMap[i]->rubberBandNr()]->movePoint( 0, p );
        }
      }

      // topological editing
      offset = posMapCoord - mPosMapCoordBackup;
      for ( int i = 0; i < mTopologyRubberBand.size(); i++ )
      {
        for ( int pointIndex = 0; pointIndex < mTopologyRubberBand[i]->numberOfVertices(); pointIndex++ )
        {
          if ( mTopologyRubberBandVertexes[i]->contains( pointIndex ) )
          {
            const QgsPoint* point = mTopologyRubberBand[i]->getPoint( 0, pointIndex );
            if ( point == 0 )
            {
              break;
            }
            mTopologyRubberBand[i]->movePoint( pointIndex, *point + offset );
          }
        }
      }

      mPosMapCoordBackup = posMapCoord;
    }
  }
  else
  {
    if ( !mSelectionRectangle )
    {
      mSelectionRectangle = true;
      mSelectionRubberBand = new QRubberBand( QRubberBand::Rectangle, mCanvas );
      mRect = new QRect();
      mRect->setTopLeft( mPressCoordinates );
    }
    mRect->setBottomRight( e->pos() );
    QRect normalizedRect = mRect->normalized();
    mSelectionRubberBand->setGeometry( normalizedRect );
    mSelectionRubberBand->show();
  }
}

void QgsMapToolNodeTool::canvasPressEvent( QMouseEvent * e )
{
  QgsDebugCall;

  mClicked = true;
  mPressCoordinates = e->pos();
  if ( !mSelectedFeature )
  {
    QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( mCanvas->currentLayer() );
    if ( !vlayer )
      return;

    mSelectAnother = false;
    QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToCurrentLayer( e->pos(), QgsPointLocator::Vertex | QgsPointLocator::Edge );
    if ( !m.isValid() )
    {
      emit messageEmitted( tr( "could not snap to a segment on the current layer." ) );
      return;
    }

    // remove previous warning
    emit messageDiscarded();

    mSelectedFeature = new QgsSelectedFeature( m.featureId(), vlayer, mCanvas );
    connect( QgisApp::instance()->layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( currentLayerChanged( QgsMapLayer* ) ) );
    connect( mSelectedFeature, SIGNAL( destroyed() ), this, SLOT( selectedFeatureDestroyed() ) );
    connect( vlayer, SIGNAL( editingStopped() ), this, SLOT( editingToggled() ) );
    mIsPoint = vlayer->geometryType() == QGis::Point;
  }
  else
  {
    // remove previous warning
    emit messageDiscarded();

    Q_ASSERT( mSelectedFeature->vlayer() );

    // try to find a piece of currently selected geometry
    QgsFeatureIdFilter filterFid( mSelectedFeature->featureId() );
    QgsPointLocator::Match mSel = mCanvas->snappingUtils()->snapToCurrentLayer( e->pos(), QgsPointLocator::Vertex | QgsPointLocator::Edge, &filterFid );

    if ( mSel.hasVertex() )
    {
      // mouse pressed on a vertex:
      // - if clicked on already selected vertex - deselect it
      // - if clicked on vertex that is not selected - select it
      // - if clicked with CTRL - invert selection state of the vertex
      // - if pressed+dragging on already selected vertex - will move selected vertices

      mMoving = true;
      mClosestMapVertex = mSel.point();
      int atVertex = mSel.vertexIndex();

      if ( mSelectedFeature->isSelected( atVertex ) )
      {
        mDeselectOnRelease = atVertex;
      }
      else if ( mCtrl )
      {
        mSelectedFeature->invertVertexSelection( atVertex );
      }
      else
      {
        mSelectedFeature->deselectAllVertexes();
        mSelectedFeature->selectVertex( atVertex );
      }
    }
    else if ( mSel.hasEdge() )
    {
      // mouse pressed on an edge:
      // - if clicked - select just vertices of that edge
      // - if clicked with CTRL - invert selection state of vertices of the edge
      // - if pressed+dragging - will move vertices of the edge

      mMoving = true;
      QgsPoint p1, p2;
      mSel.edgePoints( p1, p2 );
      mClosestMapVertex = p1.sqrDist( mSel.point() ) < p2.sqrDist( mSel.point() ) ? p1 : p2;

      if ( !mCtrl )
      {
        mSelectedFeature->deselectAllVertexes();
        mSelectedFeature->selectVertex( mSel.vertexIndex() + 1 );
        mSelectedFeature->selectVertex( mSel.vertexIndex() );
      }
      else
      {
        mSelectedFeature->invertVertexSelection( mSel.vertexIndex() + 1 );
        mSelectedFeature->invertVertexSelection( mSel.vertexIndex() );
      }

    }
    else
    {
      // nothing from the feature is acceptable:
      // - if clicked - try to select a different feature. if nothing is around, at least deselect all vertices

      QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToCurrentLayer( e->pos(), QgsPointLocator::Vertex | QgsPointLocator::Edge );
      if ( m.isValid() )
      {
        // if this will be just a click, on release we will select this new feature
        mAnother = m.featureId();
        mSelectAnother = true;
      }
      else if ( !mCtrl )
      {
        mSelectedFeature->deselectAllVertexes();
      }
    }

  }
}

void QgsMapToolNodeTool::selectedFeatureDestroyed()
{
  QgsDebugCall;
  cleanTool( false );
}

void QgsMapToolNodeTool::currentLayerChanged( QgsMapLayer *layer )
{
  if ( mSelectedFeature && layer != mSelectedFeature->vlayer() )
  {
    cleanTool();
  }
}

void QgsMapToolNodeTool::editingToggled()
{
  cleanTool();
}

void QgsMapToolNodeTool::canvasReleaseEvent( QMouseEvent * e )
{
  if ( !mSelectedFeature )
    return;

  removeRubberBands();

  QgsVectorLayer *vlayer = mSelectedFeature->vlayer();
  Q_ASSERT( vlayer );

  mClicked = false;
  mSelectionRectangle = false;

  if ( mSelectionRubberBand )
  {
    mSelectionRubberBand->close();
    delete mSelectionRubberBand;
    mSelectionRubberBand = 0;
  }

  if ( mRect )
  {
    delete mRect;
    mRect = 0;
  }

  if ( mPressCoordinates == e->pos() )
  {
    if ( mSelectAnother )
    {
      // select another feature
      mSelectedFeature->setSelectedFeature( mAnother, vlayer, mCanvas );
      mIsPoint = vlayer->geometryType() == QGis::Point;
      mSelectAnother = false;
    }
  }
  else
  {
    if ( mMoving )
    {
      mMoving = false;
      QgsPoint releaseMapCoords, pressMapCoords;

      QgsExcludePointFilter excludePointFilter( mClosestMapVertex );
      QgsPointLocator::Match match = mCanvas->snappingUtils()->snapToMap( e->pos(), &excludePointFilter );

      if ( match.isValid() )
      {
        releaseMapCoords = match.point();
        pressMapCoords = mClosestMapVertex;
      }
      else
      {
        releaseMapCoords = toMapCoordinates( e->pos() );
        pressMapCoords = toMapCoordinates( mPressCoordinates );
      }

      QgsPoint releaseLayerCoords = toLayerCoordinates( vlayer, releaseMapCoords );
      QgsPoint pressLayerCoords = toLayerCoordinates( vlayer, pressMapCoords );

      if ( match.isValid() )
      {
        int topologicalEditing = QgsProject::instance()->readNumEntry( "Digitizing", "/TopologicalEditing", 0 );
        if ( topologicalEditing )
        {
          addTopologicalPoints( QList<QgsPoint>() << releaseMapCoords );
        }
      }

      mSelectedFeature->moveSelectedVertexes( releaseLayerCoords - pressLayerCoords );
      mCanvas->refresh();
    }
    else // selecting vertexes by rubberband
    {
      // coordinates has to be coordinates from layer not canvas
      QgsRectangle r( toLayerCoordinates( vlayer, mPressCoordinates ),
                      toLayerCoordinates( vlayer, e->pos() ) );

      QList<QgsVertexEntry*> &vertexMap = mSelectedFeature->vertexMap();
      if ( !mCtrl )
      {
        mSelectedFeature->deselectAllVertexes();
      }

      for ( int i = 0; i < vertexMap.size(); i++ )
      {
        if ( r.contains( vertexMap[i]->point() ) )
        {
          // inverting selection is enough because all were deselected if ctrl is not pressed
          mSelectedFeature->invertVertexSelection( i, false );
        }
      }
    }
  }

  mMoving = false;

  if ( mDeselectOnRelease != -1 )
  {
    if ( mCtrl )
    {
      mSelectedFeature->invertVertexSelection( mDeselectOnRelease );
    }
    else
    {
      mSelectedFeature->deselectAllVertexes();
      mSelectedFeature->selectVertex( mDeselectOnRelease );
    }

    mDeselectOnRelease = -1;
  }

  mExcludePoint.clear();
}

void QgsMapToolNodeTool::deactivate()
{
  cleanTool();
  delete mRect;
  mRect = 0;
  delete mSelectionRubberBand;
  mSelectionRubberBand = 0;
  mSelectAnother = false;
  mCtrl = false;
  mMoving = true;
  mClicked = false;

  QgsMapTool::deactivate();
}

void QgsMapToolNodeTool::removeRubberBands()
{
  // cleanup rubberbands and list
  foreach ( QgsRubberBand *rb, mRubberBands )
  {
    delete rb;
  }
  mRubberBands.clear();

  foreach ( QgsRubberBand *rb, mTopologyRubberBand )
  {
    delete rb;
  }
  mTopologyRubberBand.clear();

  mTopologyMovingVertexes.clear();
  mTopologyRubberBandVertexes.clear();

  // remove all data from selected feature (no change to rubberbands itself)
  if ( mSelectedFeature )
    mSelectedFeature->cleanRubberBandsData();
}

void QgsMapToolNodeTool::cleanTool( bool deleteSelectedFeature )
{
  removeRubberBands();

  if ( mSelectedFeature )
  {
    QgsVectorLayer *vlayer = mSelectedFeature->vlayer();
    Q_ASSERT( vlayer );

    disconnect( QgisApp::instance()->layerTreeView(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( currentLayerChanged( QgsMapLayer* ) ) );
    disconnect( mSelectedFeature, SIGNAL( destroyed() ), this, SLOT( selectedFeatureDestroyed() ) );
    disconnect( vlayer, SIGNAL( editingStopped() ), this, SLOT( editingToggled() ) );

    if ( deleteSelectedFeature ) delete mSelectedFeature;
    mSelectedFeature = 0;
  }
}

void QgsMapToolNodeTool::canvasDoubleClickEvent( QMouseEvent * e )
{
  if ( !mSelectedFeature )
    return;

  QgsVectorLayer *vlayer = mSelectedFeature->vlayer();
  Q_ASSERT( vlayer );

  int topologicalEditing = QgsProject::instance()->readNumEntry( "Digitizing", "/TopologicalEditing", 0 );
  QMultiMap<double, QgsSnappingResult> currentResultList;

  mMoving = false;
  QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToCurrentLayer( e->pos(), QgsPointLocator::Edge );
  if ( !m.isValid() || m.featureId() != mSelectedFeature->featureId() )
    return;

  // some segment selected
  QgsPoint layerCoords = toLayerCoordinates( vlayer, m.point() );
  if ( topologicalEditing )
  {
    // snap from adding position to this vertex when topological editing is enabled
    currentResultList.clear();
    vlayer->snapWithContext( layerCoords, ZERO_TOLERANCE, currentResultList, QgsSnapper::SnapToSegment );
  }

  vlayer->beginEditCommand( tr( "Inserted vertex" ) );

  // add vertex
  vlayer->insertVertex( layerCoords.x(), layerCoords.y(), mSelectedFeature->featureId(), m.vertexIndex() + 1 );

  if ( topologicalEditing )
  {
    QMultiMap<double, QgsSnappingResult>::iterator resultIt =  currentResultList.begin();

    for ( ; resultIt != currentResultList.end(); ++resultIt )
    {
      // create vertexes on same position when topological editing is enabled
      if ( mSelectedFeature->featureId() !=  resultIt.value().snappedAtGeometry )
        vlayer->insertVertex( layerCoords.x(), layerCoords.y(), resultIt.value().snappedAtGeometry, resultIt.value().afterVertexNr );
    }
  }

  vlayer->endEditCommand();

  // make sure that new node gets its vertex marker
  mCanvas->refresh();
}

void QgsMapToolNodeTool::keyPressEvent( QKeyEvent* e )
{
  if ( e->key() == Qt::Key_Control )
  {
    mCtrl = true;
    return;
  }

  if ( mSelectedFeature && ( e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete ) )
  {
    int firstSelectedIndex = firstSelectedVertex();
    if ( firstSelectedIndex == -1 )
      return;

    mSelectedFeature->deleteSelectedVertexes();
    safeSelectVertex( firstSelectedIndex );
    mCanvas->refresh();

    // Override default shortcut management in MapCanvas
    e->ignore();
  }
  else if ( mSelectedFeature && ( e->key() == Qt::Key_Less || e->key() == Qt::Key_Comma ) )
  {
    int firstSelectedIndex = firstSelectedVertex();
    if ( firstSelectedIndex == -1 )
      return;

    mSelectedFeature->deselectAllVertexes();
    safeSelectVertex( firstSelectedIndex - 1 );
  }
  else if ( mSelectedFeature && ( e->key() == Qt::Key_Greater || e->key() == Qt::Key_Period ) )
  {
    int firstSelectedIndex = firstSelectedVertex();
    if ( firstSelectedIndex == -1 )
      return;

    mSelectedFeature->deselectAllVertexes();
    safeSelectVertex( firstSelectedIndex + 1 );
  }
}

void QgsMapToolNodeTool::keyReleaseEvent( QKeyEvent* e )
{
  if ( e->key() == Qt::Key_Control )
  {
    mCtrl = false;
    return;
  }
}

QgsRubberBand* QgsMapToolNodeTool::createRubberBandMarker( QgsPoint center, QgsVectorLayer* vlayer )
{

  // create rubberband marker for moving points
  QgsRubberBand* marker = new QgsRubberBand( mCanvas, QGis::Point );
  marker->setColor( Qt::red );
  marker->setWidth( 2 );
  marker->setIcon( QgsRubberBand::ICON_FULL_BOX );
  marker->setIconSize( 8 );
  QgsPoint pom = toMapCoordinates( vlayer, center );
  marker->addPoint( pom );
  return marker;
}

int QgsMapToolNodeTool::firstSelectedVertex()
{
  if ( mSelectedFeature )
  {
    QList<QgsVertexEntry*> &vertexMap = mSelectedFeature->vertexMap();
    int vertexNr = 0;

    foreach ( QgsVertexEntry *entry, vertexMap )
    {
      if ( entry->isSelected() )
      {
        return vertexNr;
      }
      vertexNr++;
    }
  }
  return -1;
}

int QgsMapToolNodeTool::safeSelectVertex( int vertexNr )
{
  if ( mSelectedFeature )
  {
    QList<QgsVertexEntry*> &vertexMap = mSelectedFeature->vertexMap();

    if ( vertexNr >= vertexMap.size() ) vertexNr -= vertexMap.size();
    if ( vertexNr < 0 ) vertexNr = vertexMap.size() - 1 + vertexNr;

    mSelectedFeature->selectVertex( vertexNr );
    return vertexNr;
  }
  return -1;
}
