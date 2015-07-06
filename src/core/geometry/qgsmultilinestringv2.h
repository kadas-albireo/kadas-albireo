/***************************************************************************
                        qgsmultilinestringv2.h
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

#ifndef QGSMULTILINESTRINGV2_H
#define QGSMULTILINESTRINGV2_H

#include "qgsgeometrycollectionv2.h"

class CORE_EXPORT QgsMultiLineStringV2: public QgsGeometryCollectionV2
{
  public:
    virtual QString geometryType() const override { return "MultiLineString"; }
    QgsAbstractGeometryV2* clone() const override;

    bool fromWkt( const QString& wkt ) override;

    // inherited: int wkbSize() const;
    // inherited: unsigned char* asWkb( int& binarySize ) const;
    // inherited: QString asWkt( int precision = 17 ) const;
    QDomElement asGML2( QDomDocument& doc, int precision = 17, const QString& ns = "gml" ) const override;
    QDomElement asGML3( QDomDocument& doc, int precision = 17, const QString& ns = "gml" ) const override;
    QString asJSON( int precision = 17 ) const override;


    /**Adds a geometry and takes ownership. Returns true in case of success*/
    virtual bool addGeometry( QgsAbstractGeometryV2* g ) override;
};

#endif // QGSMULTILINESTRINGV2_H
