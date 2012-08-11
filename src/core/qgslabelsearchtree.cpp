/***************************************************************************
    qgslabelsearchtree.cpp
    ---------------------
    begin                : November 2010
    copyright            : (C) 2010 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgslabelsearchtree.h"
#include "labelposition.h"

bool searchCallback( QgsLabelPosition* pos, void* context )
{
  QList<QgsLabelPosition*>* list = static_cast< QList<QgsLabelPosition*>* >( context );
  list->push_back( pos );
  return true;
}

QgsLabelSearchTree::QgsLabelSearchTree()
{
}

QgsLabelSearchTree::~QgsLabelSearchTree()
{
  clear();
}

void QgsLabelSearchTree::label( const QgsPoint& p, QList<QgsLabelPosition*>& posList )
{
  double c_min[2]; c_min[0] = p.x() - 0.1; c_min[1] = p.y() - 0.1;
  double c_max[2]; c_max[0] = p.x() + 0.1; c_max[1] = p.y() + 0.1;

  QList<QgsLabelPosition*> searchResults;
  mSpatialIndex.Search( c_min, c_max, searchCallback, &searchResults );

  //tolerance +-0.1 could be high in case of degree crs, so check if p is really contained in the results
  posList.clear();
  QList<QgsLabelPosition*>::const_iterator resultIt = searchResults.constBegin();
  for ( ; resultIt != searchResults.constEnd(); ++resultIt )
  {
    if (( *resultIt )->labelRect.contains( p ) )
    {
      posList.push_back( *resultIt );
    }
  }
}

void QgsLabelSearchTree::labelsInRect( const QgsRectangle& r, QList<QgsLabelPosition*>& posList )
{
  double c_min[2]; c_min[0] = r.xMinimum(); c_min[1] = r.yMinimum();
  double c_max[2]; c_max[0] = r.xMaximum(); c_max[1] = r.yMaximum();

  QList<QgsLabelPosition*> searchResults;
  mSpatialIndex.Search( c_min, c_max, searchCallback, &searchResults );

  posList.clear();
  QList<QgsLabelPosition*>::const_iterator resultIt = searchResults.constBegin();
  for ( ; resultIt != searchResults.constEnd(); ++resultIt )
  {
    posList.push_back( *resultIt );
  }
}

bool QgsLabelSearchTree::insertLabel( LabelPosition* labelPos, int featureId, const QString& layerName, bool diagram, bool pinned )
{
  if ( !labelPos )
  {
    return false;
  }

  double c_min[2];
  double c_max[2];
  labelPos->getBoundingBox( c_min, c_max );

  QVector<QgsPoint> cornerPoints;
  for ( int i = 0; i < 4; ++i )
  {
    cornerPoints.push_back( QgsPoint( labelPos->getX( i ), labelPos->getY( i ) ) );
  }
  QgsLabelPosition* newEntry = new QgsLabelPosition( featureId, labelPos->getAlpha(), cornerPoints, QgsRectangle( c_min[0], c_min[1], c_max[0], c_max[1] ),
      labelPos->getWidth(), labelPos->getHeight(), layerName, labelPos->getUpsideDown(), diagram, pinned );
  mSpatialIndex.Insert( c_min, c_max, newEntry );
  return true;
}

void QgsLabelSearchTree::clear()
{
  RTree<QgsLabelPosition*, double, 2, double>::Iterator indexIt;
  mSpatialIndex.GetFirst( indexIt );
  while ( !mSpatialIndex.IsNull( indexIt ) )
  {
    delete mSpatialIndex.GetAt( indexIt );
    mSpatialIndex.GetNext( indexIt );
  }
  mSpatialIndex.RemoveAll();
}
