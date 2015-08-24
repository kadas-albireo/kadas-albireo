/***************************************************************************
                        qgsmultilinestringv2.cpp
  -------------------------------------------------------------------
Date                 : 28 Oct 2014
Copyright            : (C) 2014 by Marco Hugentobler
email                : marco.hugentobler at sourcepole dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmultilinestringv2.h"
#include "qgsapplication.h"
#include "qgscurvev2.h"
#include "qgscircularstringv2.h"
#include "qgscompoundcurvev2.h"
#include "qgsgeometryutils.h"
#include "qgslinestringv2.h"

QgsMultiLineStringV2* QgsMultiLineStringV2::clone() const
{
  return new QgsMultiLineStringV2( *this );
}

bool QgsMultiLineStringV2::fromWkt( const QString& wkt )
{
  return fromCollectionWkt( wkt, QList<QgsAbstractGeometryV2*>() << new QgsLineStringV2, "LineString" );
}

QDomElement QgsMultiLineStringV2::asGML2( QDomDocument& doc, int precision, const QString& ns ) const
{
  QDomElement elemMultiLineString = doc.createElementNS( ns, "MultiLineString" );
  foreach ( const QgsAbstractGeometryV2 *geom, mGeometries )
  {
    if ( dynamic_cast<const QgsLineStringV2*>( geom ) )
    {
      const QgsLineStringV2* lineString = static_cast<const QgsLineStringV2*>( geom );

      QDomElement elemLineStringMember = doc.createElementNS( ns, "lineStringMember" );
      elemLineStringMember.appendChild( lineString->asGML2( doc, precision, ns ) );
      elemMultiLineString.appendChild( elemLineStringMember );

      delete lineString;
    }
  }

  return elemMultiLineString;
}

QDomElement QgsMultiLineStringV2::asGML3( QDomDocument& doc, int precision, const QString& ns ) const
{
  QDomElement elemMultiCurve = doc.createElementNS( ns, "MultiLineString" );
  foreach ( const QgsAbstractGeometryV2 *geom, mGeometries )
  {
    if ( dynamic_cast<const QgsLineStringV2*>( geom ) )
    {
      const QgsLineStringV2* lineString = static_cast<const QgsLineStringV2*>( geom );

      QDomElement elemCurveMember = doc.createElementNS( ns, "curveMember" );
      elemCurveMember.appendChild( lineString->asGML3( doc, precision, ns ) );
      elemMultiCurve.appendChild( elemCurveMember );
    }
  }

  return elemMultiCurve;
}

QString QgsMultiLineStringV2::asJSON( int precision ) const
{
  QString json = "{\"type\": \"MultiLineString\", \"coordinates\": [";
  foreach ( const QgsAbstractGeometryV2 *geom, mGeometries )
  {
    if ( dynamic_cast<const QgsCurveV2*>( geom ) )
    {
      const QgsLineStringV2* lineString = static_cast<const QgsLineStringV2*>( geom );
      QList<QgsPointV2> pts;
      lineString->points( pts );
      json += QgsGeometryUtils::pointsToJSON( pts, precision ) + ", ";
    }
  }
  if ( json.endsWith( ", " ) )
  {
    json.chop( 2 ); // Remove last ", "
  }
  json += "] }";
  return json;
}

bool QgsMultiLineStringV2::addGeometry( QgsAbstractGeometryV2* g )
{
  if ( !dynamic_cast<QgsLineStringV2*>( g ) )
  {
    delete g;
    return false;
  }

  setZMTypeFromSubGeometry( g, QgsWKBTypes::MultiLineString );
  return QgsGeometryCollectionV2::addGeometry( g );
}
