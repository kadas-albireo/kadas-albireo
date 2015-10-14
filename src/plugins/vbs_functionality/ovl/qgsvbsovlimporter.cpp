/***************************************************************************
 *  qgsvbsovlimporter.cpp                                                  *
 *  -------------------                                                    *
 *  begin                : Oct 07, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbsovlimporter.h"
#include "qgisinterface.h"
#include "qgsgeometry.h"
#include "qgscircularstringv2.h"
#include "qgslinestringv2.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgspoint.h"
#include "qgspolygonv2.h"
#include "qgsredlininglayer.h"

#include <QDomDocument>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <quazip/quazipfile.h>

void QgsVBSOvlImporter::import( QString filename ) const
{
  if ( filename.isEmpty() )
  {
    QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
    filename = QFileDialog::getOpenFileName( mIface->mainWindow(), tr( "Select OVL File" ), lastProjectDir, tr( "OVL Files (*.ovl);;" ) );
    QFileInfo finfo( filename );
    if ( !finfo.exists() )
    {
      return;
    }
    QSettings().setValue( "/UI/lastProjectDir", finfo.absolutePath() );
  }

  QuaZipFile geogrid( filename, "geogrid50.xml" );
  if ( !geogrid.open( QIODevice::ReadOnly ) )
  {
    QMessageBox::warning( mIface->mainWindow(), tr( "Error" ), tr( "The OVL file could not be opened:\n%1" ).arg( QFileInfo( filename ).fileName() ) );
    return;
  }
  QDomDocument doc;
  doc.setContent( &geogrid );
  QDomElement objectList = doc.firstChildElement( "geogridOvl" ).firstChildElement( "objectList" );
  QDomElement object = objectList.firstChildElement( "object" );
  int count = 0;
  while ( !object.isNull() )
  {
    QString clsid = object.attribute( "clsid" );
    if ( clsid == "{352CC905-847C-403A-8EE1-C991C86CCE58}" ) // Overlay
    {
      // TODO ??
    }
    else if ( clsid == "{F848EC6F-7105-4C54-9CC1-520E6D38549A}" ) // Group
    {
      // TODO ??
    }
    else if ( clsid == "{E2CCBD8B-E6DC-4B30-894F-D082A434922B}" ) // Rectangle
    {
      parseRectangleTriangleCircle( object );
      ++count;
    }
    else if ( clsid == "{FD1F97C1-54FF-4FB2-A7DC-7B27C4ED0BE2}" ) // Triangle
    {
      parseRectangleTriangleCircle( object );
      ++count;
    }
    else if ( clsid == "{4B866664-04FF-41A9-B741-15E705BA6DAD}" ) // Circle
    {
      parseRectangleTriangleCircle( object );
      ++count;
    }
    else if ( clsid == "{48A64475-69D0-41DC-AF3C-A910B2C0603F}" ) // Line
    {
      parseLine( object );
      ++count;
    }
    else if ( clsid == "{1130C93F-EE80-4F0A-A42E-5234CCA0364F}" ) // Text
    {
      parseText( object );
      ++count;
    }
    else if ( clsid == "{A56DD602-D748-4231-A9CB-439F3390257F}" ) // Bitmap
    {

    }
    else if ( clsid == "{E821201B-F7F4-4FA9-80C9-AAD95C285849}" ) // Polygon
    {
      parsePolygon( object );
      ++count;
    }
    else if ( clsid == "{6074C216-B8DF-4207-883F-862EFE9346BC}" ) // GeoBitmap
    {

    }
    else
    {
      QgsDebugMsg( QString( "Unhandled clsid %1" ).arg( clsid ) );
    }

    object = object.nextSiblingElement( "object" );
  }
  mIface->mapCanvas()->clearCache( mIface->redliningLayer()->id() );
  mIface->mapCanvas()->refresh();
  QMessageBox::information( mIface->mainWindow(), tr( "OVL Import" ), tr( "%1 features were imported." ).arg( count ) );
}

void QgsVBSOvlImporter::parseGraphic( QDomElement& attribute, QList<QgsPointV2>& points )
{
  QDomElement coord = attribute.firstChildElement( "coordList" ).firstChildElement( "coord" );
  while ( !coord.isNull() )
  {
    double x = coord.attribute( "x" ).toDouble();
    double y = coord.attribute( "y" ).toDouble();
    points.append( QgsPointV2( x, y ) );
    coord = coord.nextSiblingElement( "coord" );
  }
}

void QgsVBSOvlImporter::parseGraphicTooltip( QDomElement &attribute, QString& tooltip )
{
  bool enabled = attribute.firstChildElement( "enable" ).text() == "true";
  tooltip = enabled ? attribute.firstChildElement( "text" ).text() : "";
}

void QgsVBSOvlImporter::parseGraphicLineAttributes( QDomElement &attribute, QColor& outline, int& lineSize, Qt::PenStyle& lineStyle )
{
  QDomElement color = attribute.firstChildElement( "color" );
  QDomElement colorAlpha = attribute.firstChildElement( "colorAlpha" );
  outline = QColor(
              color.attribute( "red" ).toInt(),
              color.attribute( "green" ).toInt(),
              color.attribute( "blue" ).toInt(),
              colorAlpha.text().toInt() );
  lineSize = attribute.firstChildElement( "size" ).text().toInt();
  lineStyle = convertLineStyle( attribute.firstChildElement( "lineStyle" ).text().toInt() );
}

void QgsVBSOvlImporter::parseGraphicFillAttributes( QDomElement &attribute, QColor& fill, Qt::BrushStyle& fillStyle )
{
  QDomElement color = attribute.firstChildElement( "color" );
  QDomElement colorAlpha = attribute.firstChildElement( "colorAlpha" );
  fill = QColor(
           color.attribute( "red" ).toInt(),
           color.attribute( "green" ).toInt(),
           color.attribute( "blue" ).toInt(),
           colorAlpha.text().toInt() );
  fillStyle = convertFillStyle( attribute.firstChildElement( "fillStyle" ).text().toInt() );
}

void QgsVBSOvlImporter::parseGraphicDimmAttributes( QDomElement &attribute, QColor& dimm, double& factor )
{
  QDomElement color = attribute.firstChildElement( "color" );
  dimm = QColor(
           color.attribute( "red" ).toInt(),
           color.attribute( "green" ).toInt(),
           color.attribute( "blue" ).toInt() );
  factor = attribute.firstChildElement( "factor" ).text().toInt();
}

void QgsVBSOvlImporter::parseGraphicSinglePointAttributes( QDomElement &attribute, double& width, double& height, double& rotation )
{
  height = attribute.firstChildElement( "height" ).text().toDouble();
  width = attribute.firstChildElement( "height" ).text().toDouble();
  rotation = attribute.firstChildElement( "rotation" ).text().toDouble();
}

void QgsVBSOvlImporter::parseGraphicWHUSetable( QDomElement &attribute, WidthHeightUnit& whu )
{
  whu = static_cast<WidthHeightUnit>( attribute.firstChildElement( "unit" ).text().toInt() );
}

void QgsVBSOvlImporter::parseGraphicDelimiter( QDomElement &attribute, GraphicDelimiter &startDelimiter, GraphicDelimiter &endDelimiter )
{
  startDelimiter = static_cast<GraphicDelimiter>( attribute.firstChildElement( "startDelimiter" ).text().toInt() );
  endDelimiter = static_cast<GraphicDelimiter>( attribute.firstChildElement( "endDelimiter" ).text().toInt() );
}

void QgsVBSOvlImporter::parseGraphicRoundable( QDomElement &attribute, bool &roundable )
{
  roundable = attribute.firstChildElement( "roundable" ).text() == "true";
}

void QgsVBSOvlImporter::parseGraphicCloseable( QDomElement &attribute, bool &roundable )
{
  roundable = attribute.firstChildElement( "closeable" ).text() == "true";
}

void QgsVBSOvlImporter::parseGraphicTextAttributes( QDomElement &attribute, QString &text, QColor &textColor, QFont& font )
{
  QDomElement color = attribute.firstChildElement( "color" );
  QDomElement colorAlpha = attribute.firstChildElement( "colorAlpha" );
  textColor = QColor(
                color.attribute( "red" ).toInt(),
                color.attribute( "green" ).toInt(),
                color.attribute( "blue" ).toInt(),
                colorAlpha.text().toInt() );
  text = attribute.firstChildElement( "text" ).text();
  font.setFamily( attribute.firstChildElement( "fontFamily" ).text() );
  font.setItalic( attribute.firstChildElement( "italic" ).text().toInt() );
  font.setBold( attribute.firstChildElement( "bold" ).text().toInt() );
}

void QgsVBSOvlImporter::applyDimm( double factor, const QColor& dimmColor, QColor* outline, QColor* fill )
{
  if ( outline )
  {
    outline->setRed( outline->red() * ( 1. - factor ) + dimmColor.red() * factor );
    outline->setGreen( outline->green() * ( 1. - factor ) + dimmColor.green() * factor );
    outline->setBlue( outline->blue() * ( 1. - factor ) + dimmColor.blue() * factor );
  }
  if ( fill )
  {
    fill->setRed( fill->red() * ( 1. - factor ) + dimmColor.red() * factor );
    fill->setGreen( fill->green() * ( 1. - factor ) + dimmColor.green() * factor );
    fill->setBlue( fill->blue() * ( 1. - factor ) + dimmColor.blue() * factor );
  }
}

void QgsVBSOvlImporter::parseRectangleTriangleCircle( QDomElement &object ) const
{
  QgsPointV2 point;
  double width = 0., height = 0., rotation = 0., dimmFactor = 0.;
  int lineSize = 1;
  Qt::PenStyle lineStyle = Qt::SolidLine;
  Qt::BrushStyle fillStyle = Qt::SolidPattern;
  QColor outline, fill, dimmColor;
  QString tooltip;
  WidthHeightUnit whu;

  QDomElement attribute = object.firstChildElement( "attributeList" ).firstChildElement( "attribute" );
  while ( !attribute.isNull() )
  {
    QString iidName = attribute.attribute( "iidName" );
    if ( iidName == "IID_IGraphic" )
    {
      QList<QgsPointV2> points;
      parseGraphic( attribute, points );
      if ( !points.isEmpty() )
        point = points.first();
    }
    else if ( iidName == "IID_IGraphicTooltip" )
      parseGraphicTooltip( attribute, tooltip );
    else if ( iidName == "IID_IGraphicLineAttributes" )
      parseGraphicLineAttributes( attribute, outline, lineSize, lineStyle );
    else if ( iidName == "IID_IGraphicFillAttributes" )
      parseGraphicFillAttributes( attribute, fill, fillStyle );
    else if ( iidName == "IID_IGraphicDimmAttributes" )
      parseGraphicDimmAttributes( attribute, dimmColor, dimmFactor );
    else if ( iidName == "IID_IGraphicSinglePointAttributes" )
      parseGraphicSinglePointAttributes( attribute, width, height, rotation );
    else if ( iidName == "IID_IGraphicWHUSetable" )
      parseGraphicWHUSetable( attribute, whu );

    attribute = attribute.nextSiblingElement( "attribute" );
  }

  applyDimm( dimmFactor, dimmColor, &outline, &fill );

  QString clsid = object.attribute( "clsid" );
  if ( whu == Meters )
  {
    QgsDistanceArea da;
    QGis::UnitType unit = QGis::Meters;
    da.convertMeasurement( width, unit, QGis::Degrees, false );
    unit = QGis::Meters;
    da.convertMeasurement( height, unit, QGis::Degrees, false );
    QgsCurveV2* ring;
    if ( clsid == "{E2CCBD8B-E6DC-4B30-894F-D082A434922B}" ) // Rectangle
    {
      ring = new QgsLineStringV2();
      static_cast<QgsLineStringV2*>( ring )->setPoints( QList<QgsPointV2>() <<
          QgsPointV2( point.x() - .5 * width, point.y() - .5 * height ) <<
          QgsPointV2( point.x() + .5 * width, point.y() - .5 * height ) <<
          QgsPointV2( point.x() + .5 * width, point.y() + .5 * height ) <<
          QgsPointV2( point.x() - .5 * width, point.y() + .5 * height ) <<
          QgsPointV2( point.x() - .5 * width, point.y() - .5 * height ) );
    }
    else if ( clsid == "{FD1F97C1-54FF-4FB2-A7DC-7B27C4ED0BE2}" ) // Triangle
    {
      ring = new QgsLineStringV2();
      static_cast<QgsLineStringV2*>( ring )->setPoints( QList<QgsPointV2>() <<
          QgsPointV2( point.x() - .5 * width, point.y() - .5 * height ) <<
          QgsPointV2( point.x() + .5 * width, point.y() - .5 * height ) <<
          QgsPointV2( point.x() + .5 * width, point.y() + .5 * height ) <<
          QgsPointV2( point.x() - .5 * width, point.y() + .5 * height ) <<
          QgsPointV2( point.x() - .5 * width, point.y() - .5 * height ) );
    }
    else if ( clsid == "{4B866664-04FF-41A9-B741-15E705BA6DAD}" ) // Circle
    {
      ring = new QgsCircularStringV2();
      static_cast<QgsCircularStringV2*>( ring )->setPoints( QList<QgsPointV2>() <<
          QgsPointV2( point.x() + .5 * width, point.y() ) <<
          QgsPointV2( point.x(), point.y() ) <<
          QgsPointV2( point.x() + .5 * width, point.y() ) );
    }
    QgsCurvePolygonV2* poly = new QgsCurvePolygonV2();
    poly->setExteriorRing( ring );
    mIface->redliningLayer()->addShape( new QgsGeometry( poly ), outline, fill, lineSize, lineStyle, fillStyle );
  }
  else
  {
    QString shape;
    if ( clsid == "{E2CCBD8B-E6DC-4B30-894F-D082A434922B}" ) // Rectangle
      shape = "rectangle";
    else if ( clsid == "{FD1F97C1-54FF-4FB2-A7DC-7B27C4ED0BE2}" ) // Triangle
      shape = "triangle";
    else if ( clsid == "{4B866664-04FF-41A9-B741-15E705BA6DAD}" ) // Circle
      shape = "circle";

    QString flags = QString( "symbol=%1,w=%2,h=%3,r=%4" ).arg( shape ).arg( width ).arg( height ).arg( rotation );
    mIface->redliningLayer()->addShape( new QgsGeometry( point.clone() ), outline, fill, lineSize, lineStyle, fillStyle, flags );
  }
}

void QgsVBSOvlImporter::parseLine( QDomElement &object ) const
{
  QList<QgsPointV2> points;
  int lineSize = 1;
  Qt::PenStyle lineStyle = Qt::SolidLine;
  QColor outline, dimmColor;
  GraphicDelimiter startDelimiter = NoDelimiter, endDelimiter = NoDelimiter;
  QString tooltip;
  double dimmFactor = 0.;
  bool roundable = false, closeable = false;

  QDomElement attribute = object.firstChildElement( "attributeList" ).firstChildElement( "attribute" );
  while ( !attribute.isNull() )
  {
    QString iidName = attribute.attribute( "iidName" );
    if ( iidName == "IID_IGraphic" )
      parseGraphic( attribute, points );
    else if ( iidName == "IID_IGraphicDelimiter" )
      parseGraphicDelimiter( attribute, startDelimiter, endDelimiter );
    else if ( iidName == "IID_IGraphicTooltip" )
      parseGraphicTooltip( attribute, tooltip );
    else if ( iidName == "IID_IGraphicLineAttributes" )
      parseGraphicLineAttributes( attribute, outline, lineSize, lineStyle );
    else if ( iidName == "IID_IGraphicDimmAttributes" )
      parseGraphicDimmAttributes( attribute, dimmColor, dimmFactor );
    else if ( iidName == "IID_IGraphicRoundable" )
      parseGraphicRoundable( attribute, roundable );
    else if ( iidName == "IID_IGraphicCloseable" )
      parseGraphicCloseable( attribute, closeable );

    attribute = attribute.nextSiblingElement( "attribute" );
  }

  applyDimm( dimmFactor, dimmColor, &outline );

  QgsLineStringV2* line = new QgsLineStringV2();
  line->setPoints( points );
  if ( closeable )
  {
    line->addVertex( points.front() );
  }

  // TODO: roundable, startDelimiter, endDelimiter, tooltip
  mIface->redliningLayer()->addShape( new QgsGeometry( line ), outline, Qt::black, lineSize, lineStyle, Qt::SolidPattern );
}

void QgsVBSOvlImporter::parsePolygon( QDomElement &object ) const
{
  QList<QgsPointV2> points;
  int lineSize = 1;
  Qt::PenStyle lineStyle = Qt::SolidLine;
  Qt::BrushStyle fillStyle = Qt::SolidPattern;
  QColor outline, fill, dimmColor;
  QString tooltip;
  double dimmFactor = 0.;
  bool roundable = false;

  QDomElement attribute = object.firstChildElement( "attributeList" ).firstChildElement( "attribute" );
  while ( !attribute.isNull() )
  {
    QString iidName = attribute.attribute( "iidName" );
    if ( iidName == "IID_IGraphic" )
      parseGraphic( attribute, points );
    else if ( iidName == "IID_IGraphicTooltip" )
      parseGraphicTooltip( attribute, tooltip );
    else if ( iidName == "IID_IGraphicLineAttributes" )
      parseGraphicLineAttributes( attribute, outline, lineSize, lineStyle );
    else if ( iidName == "IID_IGraphicFillAttributes" )
      parseGraphicFillAttributes( attribute, fill, fillStyle );
    else if ( iidName == "IID_IGraphicDimmAttributes" )
      parseGraphicDimmAttributes( attribute, dimmColor, dimmFactor );
    else if ( iidName == "IID_IGraphicRoundable" )
      parseGraphicRoundable( attribute, roundable );

    attribute = attribute.nextSiblingElement( "attribute" );
  }

  applyDimm( dimmFactor, dimmColor, &outline, &fill );

  // TODO: roundable, tooltip
  QgsPolygonV2* poly = new QgsPolygonV2();
  QgsLineStringV2* ring = new QgsLineStringV2();
  ring->setPoints( points );
  ring->addVertex( points.first() );
  poly->setExteriorRing( ring );

  mIface->redliningLayer()->addShape( new QgsGeometry( poly ), outline, fill, lineSize, lineStyle, fillStyle );
}

void QgsVBSOvlImporter::parseText( QDomElement &object ) const
{
  QgsPointV2 point;
  QString text, tooltip;
  QFont font;
  QColor color, dimmColor;
  double width = 0., height = 0., rotation = 0., dimmFactor = 0.;
  WidthHeightUnit whu;

  QDomElement attribute = object.firstChildElement( "attributeList" ).firstChildElement( "attribute" );
  while ( !attribute.isNull() )
  {
    QString iidName = attribute.attribute( "iidName" );
    if ( iidName == "IID_IGraphic" )
    {
      QList<QgsPointV2> points;
      parseGraphic( attribute, points );
      if ( !points.isEmpty() )
        point = points.first();
    }
    else if ( iidName == "IID_IGraphicTooltip" )
      parseGraphicTooltip( attribute, tooltip );
    /* These are in the spec but make little sense */
    /*
    else if(iidName == "IID_IGraphicLineAttributes")
      parseGraphicLineAttributes(attribute, outline, lineSize, lineStyle);
    else if(iidName == "IID_IGraphicFillAttributes")
      parseGraphicFillAttributes(attribute, fill, fillStyle);
    */
    else if ( iidName == "IID_IGraphicDimmAttributes" )
      parseGraphicDimmAttributes( attribute, dimmColor, dimmFactor );
    else if ( iidName == "IID_IGraphicSinglePointAttributes" )
      parseGraphicSinglePointAttributes( attribute, width, height, rotation );
    else if ( iidName == "IID_IGraphicWHUSetable" )
      parseGraphicWHUSetable( attribute, whu );
    else if ( iidName == "IID_IGraphicTextAttributes" )
      parseGraphicTextAttributes( attribute, text, color, font );

    attribute = attribute.nextSiblingElement( "attribute" );
  }

  applyDimm( dimmFactor, dimmColor, &color );

  mIface->redliningLayer()->addText( text, point, color, font );
}

Qt::PenStyle QgsVBSOvlImporter::convertLineStyle( int lineStyle )
{
  if ( lineStyle == 0 )
    return Qt::NoPen;
  else if ( lineStyle == 1 )
    return Qt::SolidLine;
  else if ( lineStyle == 2 )
    return Qt::DashLine;
  else if ( lineStyle == 3 )
    return Qt::DashDotLine;
  else if ( lineStyle == 4 )
    return Qt::DotLine;
  else
    return Qt::SolidLine;
}

Qt::BrushStyle QgsVBSOvlImporter::convertFillStyle( int fillStyle )
{
  if ( fillStyle == 0 )
    return Qt::NoBrush;
  else if ( fillStyle == 1 )
    return Qt::SolidPattern;
  else if ( fillStyle == 2 )
    return Qt::HorPattern;
  else if ( fillStyle == 3 )
    return Qt::VerPattern;
  else if ( fillStyle == 4 )
    return Qt::BDiagPattern;
  else if ( fillStyle == 5 )
    return Qt::DiagCrossPattern;
  else if ( fillStyle == 6 )
    return Qt::FDiagPattern;
  else if ( fillStyle == 7 )
    return Qt::CrossPattern;
  else
    return Qt::SolidPattern;
}
