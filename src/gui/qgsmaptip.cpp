/***************************************************************************
    qgsmaptips.cpp  -  Query a layer and show a maptip on the canvas
    ---------------------
    begin                : October 2007
    copyright            : (C) 2007 by Gary Sherman
    email                : sherman @ mrcc dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
// QGIS includes
#include "qgsmapcanvas.h"
#include "qgsvectorlayer.h"
#include "qgsexpression.h"
#include "qgslogger.h"

// Qt includes
#include <QPoint>
#include <QToolTip>
#include <QSettings>

#include "qgsmaptip.h"

QgsMapTip::QgsMapTip()
{
  // init the visible flag
  mMapTipVisible = false;
}

QgsMapTip::~QgsMapTip()
{

}

void QgsMapTip::showMapTip( QgsMapLayer *thepLayer,
                            QgsPoint & theMapPosition,
                            QPoint & thePixelPosition,
                            QgsMapCanvas *thepMapCanvas )
{
  // Do the search using the active layer and the preferred label
  // field for the layer. The label field must be defined in the layer configuration
  // file/database. The code required to do this is similar to identify, except
  // we only want the first qualifying feature and we will only display the
  // field defined as the label field in the layer configuration file/database.
  //
  // TODO: Define the label (display) field for each map layer in the map configuration file/database

  // Show the maptip on the canvas
  QString myTipText = fetchFeature( thepLayer, theMapPosition, thepMapCanvas );
  mMapTipVisible = !myTipText.isEmpty();

  if ( mMapTipVisible )
  {
    QToolTip::showText( thepMapCanvas->mapToGlobal( thePixelPosition ), myTipText, thepMapCanvas );
    // store the point so we can use it to clear the maptip later
    mLastPosition = thePixelPosition;
  }
}

void QgsMapTip::clear( QgsMapCanvas *mpMapCanvas )
{
  if ( !mMapTipVisible )
    return;

  // set the maptip to blank
  QToolTip::showText( mpMapCanvas->mapToGlobal( mLastPosition ), "", mpMapCanvas );
  // reset the visible flag
  mMapTipVisible = false;
}

QString QgsMapTip::fetchFeature( QgsMapLayer *layer, QgsPoint &mapPosition, QgsMapCanvas *mpMapCanvas )
{
  QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>( layer );
  if ( !vlayer )
    return "";

  // Get the setting for the search radius from user preferences, if it exists
  QSettings settings;
  double identifyValue = settings.value( "/Map/identifyRadius", QGis::DEFAULT_IDENTIFY_RADIUS ).toDouble();

  // create the search rectangle
  double searchRadius = mpMapCanvas->extent().width() * ( identifyValue / 100.0 );

  QgsRectangle r;
  r.setXMinimum( mapPosition.x() - searchRadius );
  r.setYMinimum( mapPosition.y() - searchRadius );
  r.setXMaximum( mapPosition.x() + searchRadius );
  r.setYMaximum( mapPosition.y() + searchRadius );

  r = mpMapCanvas->mapRenderer()->mapToLayerCoordinates( layer, r );

  QgsFeature feature;

  vlayer->select( vlayer->pendingAllAttributesList() , r, true, true );
  if ( !vlayer->nextFeature( feature ) )
    return "";

  int idx = vlayer->fieldNameIndex( vlayer->displayField() );
  if ( idx < 0 )
    return QgsExpression::replaceExpressionText( vlayer->displayField(), feature, vlayer );
  else
    return feature.attributeMap().value( idx, "" ).toString();
}
