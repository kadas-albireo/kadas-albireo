/***************************************************************************
    qgsmaptooledit.cpp  -  base class for editing map tools
    ---------------------
    begin                : Juli 2007
    copyright            : (C) 2007 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaptooledit.h"
#include "qgsproject.h"
#include "qgsmapcanvas.h"
#include "qgsrubberband.h"
#include "qgsvectorlayer.h"

#include <QKeyEvent>
#include <QSettings>


QgsMapToolEdit::QgsMapToolEdit( QgsMapCanvas* canvas )
    : QgsMapToolAdvancedDigitizing( canvas )
{
}

QgsMapToolEdit::~QgsMapToolEdit()
{
}


QgsRubberBand* QgsMapToolEdit::createRubberBand( QGis::GeometryType geometryType, bool alternativeBand )
{
  QSettings settings;
  QgsRubberBand* rb = new QgsRubberBand( mCanvas, geometryType );
  rb->setWidth( settings.value( "/qgis/digitizing/line_width", 1 ).toInt() );
  QColor color( settings.value( "/qgis/digitizing/line_color_red", 255 ).toInt(),
                settings.value( "/qgis/digitizing/line_color_green", 0 ).toInt(),
                settings.value( "/qgis/digitizing/line_color_blue", 0 ).toInt() );
  double myAlpha = settings.value( "/qgis/digitizing/line_color_alpha", 200 ).toInt() / 255.0;
  if ( alternativeBand )
  {
    myAlpha = myAlpha * settings.value( "/qgis/digitizing/line_color_alpha_scale", 0.75 ).toDouble();
    rb->setLineStyle( Qt::DotLine );
  }
  if ( geometryType == QGis::Polygon )
  {
    color.setAlphaF( myAlpha );
  }
  color.setAlphaF( myAlpha );
  rb->setColor( color );
  rb->show();
  return rb;
}

QgsVectorLayer* QgsMapToolEdit::currentVectorLayer()
{
  QgsMapLayer* currentLayer = mCanvas->currentLayer();
  if ( !currentLayer )
  {
    return 0;
  }

  QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( currentLayer );
  if ( !vlayer )
  {
    return 0;
  }
  return vlayer;
}


int QgsMapToolEdit::addTopologicalPoints( const QList<QgsPoint>& geom )
{
  if ( !mCanvas )
  {
    return 1;
  }

  //find out current vector layer
  QgsVectorLayer *vlayer = currentVectorLayer();

  if ( !vlayer )
  {
    return 2;
  }

  QList<QgsPoint>::const_iterator list_it = geom.constBegin();
  for ( ; list_it != geom.constEnd(); ++list_it )
  {
    vlayer->addTopologicalPoints( *list_it );
  }
  return 0;
}

void QgsMapToolEdit::notifyNotVectorLayer()
{
  emit messageEmitted( tr( "No active vector layer" ) );
}

void QgsMapToolEdit::notifyNotEditableLayer()
{
  emit messageEmitted( tr( "Layer not editable" ) );
}
