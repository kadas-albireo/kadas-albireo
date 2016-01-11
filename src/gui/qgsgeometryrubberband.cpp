/***************************************************************************
                         qgsgeometryrubberband.cpp
                         -------------------------
    begin                : December 2014
    copyright            : (C) 2014 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgeometryrubberband.h"
#include "qgsabstractgeometryv2.h"
#include "qgsmapcanvas.h"
#include "qgspointv2.h"
#include "qgscurvev2.h"
#include "qgspolygonv2.h"
#include "qgslinestringv2.h"
#include "qgscircularstringv2.h"
#include "qgsgeometrycollectionv2.h"
#include "qgsproject.h"
#include <QPainter>

QgsGeometryRubberBand::QgsGeometryRubberBand( QgsMapCanvas* mapCanvas, QGis::GeometryType geomType ): QgsMapCanvasItem( mapCanvas ),
    mGeometry( 0 ), mIconSize( 5 ), mIconType( ICON_BOX ), mIconFill( Qt::transparent ), mGeometryType( geomType ), mMeasurementMode( MEASURE_NONE )
{
  mTranslationOffset[0] = 0.;
  mTranslationOffset[1] = 0.;

  mPen = QPen( QColor( 255, 0, 0 ) );
  mBrush = QBrush( QColor( 255, 0, 0 ) );

  mDa.setSourceCrs( mMapCanvas->mapSettings().destinationCrs() );
  mDa.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", GEO_NONE ) );
  mDa.setEllipsoidalMode( mMapCanvas->mapSettings().hasCrsTransformEnabled() );
  connect( mapCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( redrawMeasurements() ) );
}

QgsGeometryRubberBand::~QgsGeometryRubberBand()
{
  qDeleteAll( mMeasurementLabels );
  delete mGeometry;
}

void QgsGeometryRubberBand::paint( QPainter* painter )
{
  if ( !mGeometry || !painter )
  {
    return;
  }

  painter->save();
  painter->translate( -pos() );

  if ( mGeometryType == QGis::Polygon )
  {
    painter->setBrush( mBrush );
  }
  else
  {
    painter->setBrush( Qt::NoBrush );
  }
  painter->setPen( mPen );


  QgsAbstractGeometryV2* paintGeom = mGeometry->clone();

  QTransform mapToCanvas = mMapCanvas->getCoordinateTransform()->transform();
  QTransform translationOffset = QTransform::fromTranslate( mTranslationOffset[0], mTranslationOffset[1] );

  paintGeom->transform( translationOffset * mapToCanvas );
  paintGeom->draw( *painter );

  //draw vertices
  QgsVertexId vertexId;
  QgsPointV2 vertex;
  while ( paintGeom->nextVertex( vertexId, vertex ) )
  {
    drawVertex( painter, vertex.x(), vertex.y() );
  }

  delete paintGeom;
  painter->restore();
}

void QgsGeometryRubberBand::drawVertex( QPainter* p, double x, double y )
{
  qreal s = ( mIconSize - 1 ) / 2;
  p->save();
  p->setBrush( mIconFill );

  switch ( mIconType )
  {
    case ICON_NONE:
      break;

    case ICON_CROSS:
      p->drawLine( QLineF( x - s, y, x + s, y ) );
      p->drawLine( QLineF( x, y - s, x, y + s ) );
      break;

    case ICON_X:
      p->drawLine( QLineF( x - s, y - s, x + s, y + s ) );
      p->drawLine( QLineF( x - s, y + s, x + s, y - s ) );
      break;

    case ICON_BOX:
      p->drawLine( QLineF( x - s, y - s, x + s, y - s ) );
      p->drawLine( QLineF( x + s, y - s, x + s, y + s ) );
      p->drawLine( QLineF( x + s, y + s, x - s, y + s ) );
      p->drawLine( QLineF( x - s, y + s, x - s, y - s ) );
      break;

    case ICON_FULL_BOX:
      p->drawRect( x - s, y - s, mIconSize, mIconSize );
      break;

    case ICON_CIRCLE:
      p->drawEllipse( x - s, y - s, mIconSize, mIconSize );
      break;
  }
  p->restore();
}

void QgsGeometryRubberBand::redrawMeasurements()
{
  qDeleteAll( mMeasurementLabels );
  mMeasurementLabels.clear();
  mPartMeasurements.clear();
  if ( mGeometry )
  {
    if ( mMeasurementMode != MEASURE_NONE )
    {
      if ( dynamic_cast<QgsGeometryCollectionV2*>( mGeometry ) )
      {
        QgsGeometryCollectionV2* collection = static_cast<QgsGeometryCollectionV2*>( mGeometry );
        for ( int i = 0, n = collection->numGeometries(); i < n; ++i )
        {
          measureGeometry( collection->geometryN( i ) );
        }
      }
      else
      {
        measureGeometry( mGeometry );
      }
    }
  }
}

void QgsGeometryRubberBand::setGeometry( QgsAbstractGeometryV2* geom )
{
  delete mGeometry;
  mGeometry = geom;
  qDeleteAll( mMeasurementLabels );
  mMeasurementLabels.clear();
  mPartMeasurements.clear();

  if ( !mGeometry )
  {
    setRect( QgsRectangle() );
    return;
  }

  setRect( rubberBandRectangle() );

  if ( mMeasurementMode != MEASURE_NONE )
  {
    if ( dynamic_cast<QgsGeometryCollectionV2*>( mGeometry ) )
    {
      QgsGeometryCollectionV2* collection = static_cast<QgsGeometryCollectionV2*>( mGeometry );
      for ( int i = 0, n = collection->numGeometries(); i < n; ++i )
      {
        measureGeometry( collection->geometryN( i ) );
      }
    }
    else
    {
      measureGeometry( mGeometry );
    }
  }
}

void QgsGeometryRubberBand::setTranslationOffset( double dx, double dy )
{
  mTranslationOffset[0] = dx;
  mTranslationOffset[1] = dy;
  if ( mGeometry )
  {
    setRect( rubberBandRectangle() );
  }
}

void QgsGeometryRubberBand::moveVertex( const QgsVertexId& id, const QgsPointV2& newPos )
{
  if ( mGeometry )
  {
    mGeometry->moveVertex( id, newPos );
    setRect( rubberBandRectangle() );
  }
}

void QgsGeometryRubberBand::setFillColor( const QColor& c )
{
  mBrush.setColor( c );
}

void QgsGeometryRubberBand::setOutlineColor( const QColor& c )
{
  mPen.setColor( c );
}

void QgsGeometryRubberBand::setOutlineWidth( int width )
{
  mPen.setWidth( width );
}

void QgsGeometryRubberBand::setLineStyle( Qt::PenStyle penStyle )
{
  mPen.setStyle( penStyle );
}

void QgsGeometryRubberBand::setBrushStyle( Qt::BrushStyle brushStyle )
{
  mBrush.setStyle( brushStyle );
}

void QgsGeometryRubberBand::setMeasurementMode( MeasurementMode measurementMode, QGis::UnitType displayUnits , AngleUnit angleUnit )
{
  mMeasurementMode = measurementMode;
  mDisplayUnits = displayUnits;
  mAngleUnit = angleUnit;
  redrawMeasurements();
}

QString QgsGeometryRubberBand::getTotalMeasurement() const
{
  if ( mMeasurementMode == MEASURE_ANGLES )
  {
    return ""; /* Does not make sense for angles */
  }
  double total = 0;
  foreach ( double value, mPartMeasurements )
  {
    total += value;
  }
  if ( mMeasurementMode == MEASURE_LINE || mMeasurementMode == MEASURE_LINE_AND_SEGMENTS )
  {
    return formatMeasurement( total, false );
  }
  else
  {
    return formatMeasurement( total, true );
  }
}

QgsRectangle QgsGeometryRubberBand::rubberBandRectangle() const
{
  qreal scale = mMapCanvas->mapUnitsPerPixel();
  qreal s = ( mIconSize - 1 ) / 2.0 * scale;
  qreal p = mPen.width() * scale;
  QgsRectangle rect = mGeometry->boundingBox().buffer( s + p );
  rect.setXMinimum( rect.xMinimum() + mTranslationOffset[0] );
  rect.setYMinimum( rect.yMinimum() + mTranslationOffset[1] );
  rect.setXMaximum( rect.xMaximum() + mTranslationOffset[0] );
  rect.setYMaximum( rect.yMaximum() + mTranslationOffset[1] );
  return rect;
}

void QgsGeometryRubberBand::measureGeometry( QgsAbstractGeometryV2 *geometry )
{
  QStringList measurements;
  switch ( mMeasurementMode )
  {
    case MEASURE_LINE:
      if ( dynamic_cast<QgsCurveV2*>( geometry ) )
      {
        double length = mDa.measureLine( static_cast<QgsCurveV2*>( geometry ) );
        mPartMeasurements.append( length );
        measurements.append( formatMeasurement( length, false ) );
      }
      break;
    case MEASURE_LINE_AND_SEGMENTS:
      if ( dynamic_cast<QgsCurveV2*>( geometry ) )
      {
        QList<QgsPointV2> points;
        static_cast<QgsCurveV2*>( geometry )->points( points );
        for ( int i = 0, n = points.size() - 1; i < n; ++i )
        {
          QgsPoint p1( points[i].x(), points[i].y() );
          QgsPoint p2( points[i+1].x(), points[i+1].y() );
          QString segmentLength = formatMeasurement( mDa.measureLine( p1, p2 ), false );
          addMeasurements( QStringList() << segmentLength, QgsPointV2( 0.5 * ( p1.x() + p2.x() ), 0.5 * ( p1.y() + p2.y() ) ) );
        }
        if ( !points.isEmpty() )
        {
          double length = mDa.measureLine( static_cast<QgsCurveV2*>( geometry ) );
          mPartMeasurements.append( length );
          QString totLength = QApplication::translate( "QgsGeometryRubberBand", "Tot.: %1" ).arg( formatMeasurement( length, false ) );
          addMeasurements( QStringList() << totLength, points.last() );
        }
      }
      break;
    case MEASURE_ANGLES:
      if ( dynamic_cast<QgsCurveV2*>( geometry ) )
      {
        QList<QgsPointV2> points;
        static_cast<QgsCurveV2*>( geometry )->points( points );
        for ( int i = 0, n = points.size() - 2; i < n; ++i )
        {
          QgsPoint p1( points[i].x(), points[i].y() );
          QgsPoint p2( points[i+1].x(), points[i+1].y() );
          QgsPoint p3( points[i+2].x(), points[i+2].y() );

          double angle = mDa.bearing( p3, p2 ) - mDa.bearing( p2, p1 );
          mPartMeasurements.append( angle );
          QString segmentLength = formatAngle( angle );
          addMeasurements( QStringList() << segmentLength, QgsPointV2( p2.x(), p2.y() ) );
        }
      }
      break;
    case MEASURE_POLYGON:
      if ( dynamic_cast<QgsCurvePolygonV2*>( geometry ) )
      {
        double area = mDa.measurePolygon( static_cast<QgsCurvePolygonV2*>( geometry )->exteriorRing() );
        mPartMeasurements.append( area );
        measurements.append( formatMeasurement( area, true ) );
      }
      break;
    case MEASURE_RECTANGLE:
      if ( dynamic_cast<QgsPolygonV2*>( geometry ) )
      {
        double area = mDa.measurePolygon( static_cast<QgsCurvePolygonV2*>( geometry )->exteriorRing() );
        mPartMeasurements.append( area );
        measurements.append( formatMeasurement( area, true ) );
        QgsCurveV2* ring = static_cast<QgsPolygonV2*>( geometry )->exteriorRing();
        if ( ring->vertexCount() >= 4 )
        {
          QList<QgsPointV2> points;
          ring->points( points );
          QgsPoint p1( points[0].x(), points[0].y() );
          QgsPoint p2( points[2].x(), points[2].y() );
          QString width = formatMeasurement( mDa.measureLine( p1, QgsPoint( p2.x(), p1.y() ) ), false );
          QString height = formatMeasurement( mDa.measureLine( p1, QgsPoint( p1.x(), p2.y() ) ), false );
          measurements.append( QString( "(%1 x %2)" ).arg( width ).arg( height ) );
        }
      }
      break;
    case MEASURE_CIRCLE:
      if ( dynamic_cast<QgsCurvePolygonV2*>( geometry ) )
      {
        QgsCurveV2* ring = static_cast<QgsCurvePolygonV2*>( geometry )->exteriorRing();
        QgsLineStringV2* polyline = ring->curveToLine();
        double area = mDa.measurePolygon( polyline );
        mPartMeasurements.append( area );
        measurements.append( formatMeasurement( area, true ) );
        delete polyline;
        QgsPointV2 p1 = ring->vertexAt( QgsVertexId( 0, 0, 0 ) );
        QgsPointV2 p2 = ring->vertexAt( QgsVertexId( 0, 0, 1 ) );
        measurements.append( QApplication::translate( "QgsGeometryRubberBand", "Radius: %1" ).arg( formatMeasurement( mDa.measureLine( QgsPoint( p1.x(), p1.y() ), QgsPoint( p2.x(), p2.y() ) ), false ) ) );
      }
      break;
    case MEASURE_NONE:
      break;
  }
  if ( !measurements.isEmpty() )
  {
    addMeasurements( measurements, geometry->centroid() );
  }
}

QString QgsGeometryRubberBand::formatMeasurement( double value, bool isArea ) const
{
  int decimals = QSettings().value( "/qgis/measure/decimalplaces", "3" ).toInt();
  QGis::UnitType measureUnits = mMapCanvas->mapSettings().mapUnits();
  mDa.convertMeasurement( value, measureUnits, mDisplayUnits, isArea );
  return mDa.textUnit( value, decimals, mDisplayUnits, isArea );
}

QString QgsGeometryRubberBand::formatAngle( double value ) const
{
  int decimals = QSettings().value( "/qgis/measure/decimalplaces", "3" ).toInt();
  switch ( mAngleUnit )
  {
    case ANGLE_DEGREES:
      return QString( "%1 deg" ).arg( QLocale::system().toString( value * 180. / M_PI, 'f', decimals ) );
    case ANGLE_RADIANS:
      return QString( "%1 rad" ).arg( QLocale::system().toString( value, 'f', decimals ) );
    case ANGLE_GRADIANS:
      return QString( "%1 gon" ).arg( QLocale::system().toString( value * 200. / M_PI, 'f', decimals ) );
    case ANGLE_MIL:
      return QString( "%1 mil" ).arg( QLocale::system().toString( value * 3200. / M_PI, 'f', decimals ) );
  }
  return "";
}

void QgsGeometryRubberBand::addMeasurements( const QStringList& measurements, const QgsPointV2& mapPos )
{
  if ( measurements.isEmpty() )
  {
    return;
  }
  QGraphicsTextItem* label = new QGraphicsTextItem( "", 0, mMapCanvas->scene() );
  int red = QSettings().value( "/qgis/default_measure_color_red", 222 ).toInt();
  int green = QSettings().value( "/qgis/default_measure_color_green", 155 ).toInt();
  int blue = QSettings().value( "/qgis/default_measure_color_blue", 67 ).toInt();
  label->setDefaultTextColor( QColor( red, green, blue ) );
  QFont font = label->font();
  font.setBold( true );
  label->setFont( font );
  label->setPos( toCanvasCoordinates( QgsPoint( mapPos.x(), mapPos.y() ) ) );
  QString html = QString( "<div style=\"background: rgba(255, 255, 255, 159); padding: 5px; border-radius: 5px;\">" ) + measurements.join( "</div><div style=\"background: rgba(255, 255, 255, 159); padding: 5px; border-radius: 5px;\">" ) + QString( "</div>" );
  label->setHtml( html );
  label->setZValue( zValue() + 1 );
  mMeasurementLabels.append( label );
}
