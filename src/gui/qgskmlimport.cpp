/***************************************************************************
 *  qgskmlimport.cpp                                                       *
 *  -----------                                                            *
 *  begin                : March 2018                                      *
 *  copyright            : (C) 2018 by Sandro Mani / Sourcepole AG         *
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

#include "qgskmlimport.h"
#include "qgsannotationlayer.h"
#include "qgscrscache.h"
#include "qgsgeoimageannotationitem.h"
#include "qgsgeometry.h"
#include "qgslinestringv2.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgspointv2.h"
#include "qgspolygonv2.h"
#include "qgsrasterlayer.h"
#include "qgsredlininglayer.h"
#include "qgssymbollayerv2utils.h"
#include "qgstemporaryfile.h"

#include <QDomDocument>
#include <QFileInfo>
#include <QUuid>

#if 0//QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <quazip/quazipfile.h>
#else
#include <quazip5/quazipfile.h>
#endif

QgsKMLImport::QgsKMLImport( QgsMapCanvas *canvas, QgsRedliningLayer *redliningLayer )
    : mCanvas( canvas )
    , mRedliningLayer( redliningLayer )
{
}

bool QgsKMLImport::importFile( const QString& filename, QString& errMsg )
{
  if ( filename.endsWith( ".kmz", Qt::CaseInsensitive ) )
  {
    QuaZip quaZip( filename );
    if ( !quaZip.open( QuaZip::mdUnzip ) )
    {
      errMsg = tr( "Unable to open %1." ).arg( QFileInfo( filename ).fileName() );
      return false;
    }
    // Search for kml file to open
    QStringList kmzFileList = quaZip.getFileNameList();
    int mainKmlIndex = kmzFileList.indexOf( QRegExp( "[^/]+.kml", Qt::CaseInsensitive ) );
    if ( mainKmlIndex == -1 || !quaZip.setCurrentFile( kmzFileList[mainKmlIndex] ) )
    {
      errMsg = tr( "Corrupt KMZ file." );
      return false;
    }
    QDomDocument doc;
    QuaZipFile file( &quaZip );
    if ( !file.open( QIODevice::ReadOnly ) || !doc.setContent( &file ) )
    {
      errMsg = tr( "Corrupt KMZ file." );
      return false;
    }
    file.close();
    return importDocument( doc, errMsg, &quaZip );
  }
  else if ( filename.endsWith( ".kml", Qt::CaseInsensitive ) )
  {
    QFile file( filename );
    QDomDocument doc;
    if ( !file.open( QIODevice::ReadOnly ) || !doc.setContent( &file ) )
    {
      errMsg = tr( "Unable to open %1." ).arg( QFileInfo( filename ).fileName() );
      return false;
    }
    return importDocument( doc, errMsg );
  }
  return false;
}

bool QgsKMLImport::importDocument( const QDomDocument &doc, QString& errMsg, QuaZip *zip )
{
  QDomElement documentEl = doc.firstChildElement( "kml" ).firstChildElement( "Document" );
  if ( documentEl.isNull() )
  {
    errMsg = tr( "Corrupt KMZ file." );
    return false;
  }

  // Styles / StyleMaps with id
  QMap<QString, StyleData> styleMap;
  QDomNodeList styleEls = documentEl.elementsByTagName( "Style" );
  for ( int iStyle = 0, nStyles = styleEls.size(); iStyle < nStyles; ++iStyle )
  {
    QDomElement styleEl = styleEls.at( iStyle ).toElement();
    if ( !styleEl.attribute( "id" ).isEmpty() )
    {
      styleMap.insert( QString( "#%1" ).arg( styleEl.attribute( "id" ) ), parseStyle( styleEl, zip ) );
    }
  }
  QDomNodeList styleMapEls = documentEl.elementsByTagName( "StyleMap" );
  for ( int iStyleMap = 0, nStyleMaps = styleMapEls.size(); iStyleMap < nStyleMaps; ++iStyleMap )
  {
    QDomElement styleMapEl = styleMapEls.at( iStyleMap ).toElement();
    if ( !styleMapEl.attribute( "id" ).isEmpty() )
    {
      QString id = QString( "#%1" ).arg( styleMapEl.attribute( "id" ) );
      // Just pick the first item of the StyleMap
      QDomElement pairEl = styleMapEl.firstChildElement( "Pair" );
      QDomElement styleEl = pairEl.firstChildElement( "Style" );
      QDomElement styleUrlEl = pairEl.firstChildElement( "styleUrl" );
      if ( !styleEl.isNull() )
      {
        styleMap.insert( id, parseStyle( styleEl, zip ) );
      }
      else if ( !styleUrlEl.isNull() )
      {
        styleMap.insert( id, styleMap.value( styleUrlEl.text() ) );
      }
    }
  }


  // Placemarks
  QDomNodeList placemarkEls = documentEl.elementsByTagName( "Placemark" );
  for ( int iPlacemark = 0, nPlacemarks = placemarkEls.size(); iPlacemark < nPlacemarks; ++iPlacemark )
  {
    QDomElement placemarkEl = placemarkEls.at( iPlacemark ).toElement();
    QString name = placemarkEl.firstChildElement( "name" ).text();

    // Geometry
    QList<QgsAbstractGeometryV2*> geoms = parseGeometries( placemarkEl );

    if ( geoms.isEmpty() )
    {
      // Placemark without geometry
      QgsDebugMsg( "Could not parse placemark geometry" );
      continue;
    }

    // Style
    QDomElement styleEl = placemarkEl.firstChildElement( "Style" );
    QDomElement styleUrlEl = placemarkEl.firstChildElement( "styleUrl" );
    StyleData style;
    if ( !styleEl.isNull() )
    {
      style = parseStyle( styleEl, zip );
    }
    else if ( !styleUrlEl.isNull() )
    {
      style = styleMap.value( styleUrlEl.text() );
    }

    // If there is an icon and the geometry is a point, add as annotation item, otherwise as redlining symbol
    if ( geoms.size() == 1 && !style.icon.isEmpty() && dynamic_cast<QgsPointV2*>( geoms.front() ) )
    {
      QgsPointV2* point = static_cast<QgsPointV2*>( geoms.front() );
      QgsGeoImageAnnotationItem* item = new QgsGeoImageAnnotationItem( mCanvas );
      item->setItemFlags( QgsAnnotationItem::ItemAnchorIsNotMoveable );
      item->setFilePath( style.icon );
      item->setMapPosition( QgsPoint( point->x(), point->y() ), QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
      QgsAnnotationLayer::getLayer( mCanvas, "geoImage", tr( "Pictures" ) )->addItem( item );
      delete geoms.front();
    }
    else
    {
      // Attempt to recover redlining attributes
      QString flags;
      QString tooltip;
      QString text;
      StyleData redliningStyle;
      QDomNodeList simpleDataEls = placemarkEl.elementsByTagName( "SimpleData" );
      for ( int iSimpleData = 0, nSimpleData = simpleDataEls.size(); iSimpleData < nSimpleData; ++iSimpleData )
      {
        QDomElement simpleDataEl = simpleDataEls.at( iSimpleData ).toElement();
        if ( simpleDataEl.attribute( "name" ) == "tooltip" )
        {
          tooltip = simpleDataEl.text();
        }
        else if ( simpleDataEl.attribute( "name" ) == "flags" )
        {
          flags = simpleDataEl.text();
        }
        else if ( simpleDataEl.attribute( "name" ) == "text" )
        {
          text = simpleDataEl.text();
        }
        else if ( simpleDataEl.attribute( "name" ) == "outline_style" )
        {
          redliningStyle.outlineStyle = QgsSymbolLayerV2Utils::decodePenStyle( simpleDataEl.text() );
        }
        else if ( simpleDataEl.attribute( "name" ) == "fill_style" )
        {
          redliningStyle.fillStyle = QgsSymbolLayerV2Utils::decodeBrushStyle( simpleDataEl.text() );
        }
        else if ( simpleDataEl.attribute( "name" ) == "size" )
        {
          redliningStyle.outlineSize = simpleDataEl.text().toInt();
        }
        else if ( simpleDataEl.attribute( "name" ) == "outline" )
        {
          redliningStyle.outlineColor = QgsSymbolLayerV2Utils::decodeColor( simpleDataEl.text() );
        }
        else if ( simpleDataEl.attribute( "name" ) == "fill" )
        {
          redliningStyle.fillColor = QgsSymbolLayerV2Utils::decodeColor( simpleDataEl.text() );
        }
      }
      // If flags is empty, it is not a redlining object. Clear text, because it messes up rendering without matching flags
      if ( flags.isEmpty() )
      {
        text == "";
      }
      else
      {
        style = redliningStyle;
      }

    for ( QgsAbstractGeometryV2* geom : geoms )
      {
        QString geomFlags = flags;
        if ( geomFlags.isEmpty() )
        {
          QgsWKBTypes::Type type = QgsWKBTypes::singleType( QgsWKBTypes::flatType( geom->wkbType() ) );
          if ( type == QgsWKBTypes::Point )
          {
            // KML does not have point styles, so points without associated icon are rendered as text
            text = name;
            geomFlags = "bold=1,family=sans,fontSize=14,shape=point,symbol=";
          }
          else if ( type == QgsWKBTypes::LineString )
          {
            geomFlags = "shape=line";
          }
          else
          {
            geomFlags = "shape=polygon";
          }
        }
        geom->transform( *QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", "EPSG:3857" ) );
        mRedliningLayer->addShape( new QgsGeometry( geom ), style.outlineColor, style.fillColor, style.outlineSize, style.outlineStyle, style.fillStyle, geomFlags, tooltip, text );
      }
    }
  }

  // Ground overlays: group by name of parent folder, if possible
  if ( zip )
  {
    QMap<QString, OverlayData> overlays;
    QDomNodeList groundOverlayEls = documentEl.elementsByTagName( "GroundOverlay" );
    for ( int iOverlay = 0, nOverlays = groundOverlayEls.size(); iOverlay < nOverlays; ++iOverlay )
    {
      QDomElement groundOverlayEl = groundOverlayEls.at( iOverlay ).toElement();
      QDomElement bboxEl = groundOverlayEl.firstChildElement( "LatLonBox" ).toElement();
      QString name = groundOverlayEl.firstChildElement( "name" ).text();
      // If tile contained in folder, group by folder
      if ( groundOverlayEl.parentNode().nodeName() == "Folder" )
      {
        name = groundOverlayEl.parentNode().firstChildElement( "name" ).text();
      }
      TileData tile;
      tile.iconHref = groundOverlayEl.firstChildElement( "Icon" ).firstChildElement( "href" ).text();
      tile.bbox.setXMinimum( bboxEl.firstChildElement( "west" ).text().toDouble() );
      tile.bbox.setXMaximum( bboxEl.firstChildElement( "east" ).text().toDouble() );
      tile.bbox.setYMinimum( bboxEl.firstChildElement( "south" ).text().toDouble() );
      tile.bbox.setYMaximum( bboxEl.firstChildElement( "north" ).text().toDouble() );
      overlays[name].tiles.append( tile );
      if ( overlays[name].bbox.isEmpty() )
      {
        overlays[name].bbox = tile.bbox;
      }
      else
      {
        overlays[name].bbox.unionRect( tile.bbox );
      }
    }

    // Build VRTs for each overlay group
    for ( auto it = overlays.begin(), itEnd = overlays.end(); it != itEnd; ++it )
    {
      buildVSIVRT( it.key(), it.value(), zip );
    }
  }

  return true;
}

void QgsKMLImport::buildVSIVRT( const QString& name, OverlayData &overlayData, QuaZip* kmzZip ) const
{
  if ( overlayData.tiles.empty() )
  {
    return;
  }

  // Prepare vsi output
  QString vsifilename = QgsTemporaryFile::createNewFile( QString( "%1.zip" ).arg( name ) );
  QuaZip vsiZip( vsifilename );
  if ( !vsiZip.open( QuaZip::mdCreate ) )
  {
    return;
  }
  // Copy tile images to vsi, determine tile sizes
for ( TileData& tile : overlayData.tiles )
  {
    if ( !kmzZip->setCurrentFile( tile.iconHref ) )
    {
      continue;
    }
    QuaZipFile kmzTileFile( kmzZip );
    QuaZipFile vsiTileFile( &vsiZip );
    if ( !kmzTileFile.open( QIODevice::ReadOnly ) || !vsiTileFile.open( QIODevice::WriteOnly, QuaZipNewInfo( QFileInfo( tile.iconHref ).completeBaseName() + ".png" ) ) )
    {
      continue;
    }
    QImage tileImage = QImage::fromData( kmzTileFile.readAll() );
    tile.size = tileImage.size();
    tileImage.save( &vsiTileFile, "PNG" );
    vsiTileFile.close();
  }

  // Get total size by assuming all tiles have same resolution
  const TileData& firstTile = overlayData.tiles.front();
  QSize totSize(
    qRound( firstTile.size.width() / firstTile.bbox.width() * overlayData.bbox.width() ),
    qRound( firstTile.size.height() / firstTile.bbox.height() * overlayData.bbox.height() )
  );

  // Write vrt
  QString vrtString;
  QTextStream vrtStream( &vrtString, QIODevice::WriteOnly );
  vrtStream << "<VRTDataset rasterXSize=\"" << totSize.width() << "\" rasterYSize=\"" << totSize.height() << "\">" << endl;
  vrtStream << " <SRS>" << QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ).toProj4().toHtmlEscaped() << "</SRS>" << endl;
  vrtStream << " <GeoTransform>" << endl;
  vrtStream << "  " << overlayData.bbox.xMinimum() << "," << ( overlayData.bbox.width() / totSize.width() ) << ", 0," << endl;
  vrtStream << "  " << overlayData.bbox.yMaximum() << ", 0," << ( -overlayData.bbox.height() / totSize.height() ) << endl;
  vrtStream << " </GeoTransform>" << endl;
for ( const QPair<int, QString>& band : QList<QPair<int, QString>> {qMakePair( 1, QString( "Red" ) ), qMakePair( 2, QString( "Green" ) ), qMakePair( 3, QString( "Blue" ) ), qMakePair( 4, QString( "Alpha" ) )} )
  {
    vrtStream << " <VRTRasterBand dataType=\"Byte\" band=\"" << band.first << "\">" << endl;
    vrtStream << "  <ColorInterp>" << band.second << "</ColorInterp>" << endl;
  for ( const TileData& tile : overlayData.tiles )
    {
      int i = qRound(( tile.bbox.xMinimum() - overlayData.bbox.xMinimum() ) / overlayData.bbox.width() * totSize.width() );
      int j = qRound(( overlayData.bbox.yMaximum() - tile.bbox.yMaximum() ) / overlayData.bbox.height() * totSize.height() );
      vrtStream << "  <SimpleSource>" << endl;
      vrtStream << "   <SourceFilename relativeToVRT=\"1\">" << tile.iconHref << "</SourceFilename>" << endl;
      vrtStream << "   <SourceBand>" << band.first << "</SourceBand>" << endl;
      vrtStream << "   <SourceProperties RasterXSize=\"" << tile.size.width() << "\" RasterYSize=\"" << tile.size.height() << "\" DataType=\"Byte\" BlockXSize=\"" << tile.size.width() << "\" BlockYSize=\"1\" />" << endl;
      vrtStream << "   <SrcRect xOff=\"0\" yOff=\"0\" xSize=\"" << tile.size.width() << "\" ySize=\"" << tile.size.height() << "\" />" << endl;
      vrtStream << "   <DstRect xOff=\"" << i << "\" yOff=\"" << j << "\" xSize=\"" << tile.size.width() << "\" ySize=\"" << tile.size.height() << "\" />" << endl;
      vrtStream << "  </SimpleSource>" << endl;
    }
    vrtStream << " </VRTRasterBand>" << endl;
  }
  vrtStream << "</VRTDataset>" << endl;
  vrtStream.flush();

  QuaZipFile vsiVrtFile( &vsiZip );
  if ( !vsiVrtFile.open( QIODevice::WriteOnly, QuaZipNewInfo( "dataset.vrt" ) ) )
  {
    return;
  }
  vsiVrtFile.write( vrtString.toLocal8Bit() );
  vsiVrtFile.close();
  vsiZip.close();

  QgsRasterLayer* rasterLayer = new QgsRasterLayer( QString( "/vsizip/%1/dataset.vrt" ).arg( vsifilename ), name, "gdal" );
  QgsMapLayerRegistry::instance()->addMapLayer( rasterLayer );
}

QList<QgsPointV2> QgsKMLImport::parseCoordinates( const QDomElement& geomEl ) const
{
  QStringList coordinates = geomEl.firstChildElement( "coordinates" ).text().split( QRegExp( "\\s+" ), QString::SkipEmptyParts );
  QList<QgsPointV2> points;
  for ( int i = 0, n = coordinates.size(); i < n; ++i )
  {
    QStringList coordinate = coordinates[i].split( "," );
    if ( coordinate.size() >= 3 )
    {
      QgsPointV2 p( QgsWKBTypes::PointZ );
      p.setX( coordinate[0].toDouble() );
      p.setY( coordinate[1].toDouble() );
      p.setZ( coordinate[2].toDouble() );
      points.append( p );
    }
    else if ( coordinate.size() == 2 )
    {
      QgsPointV2 p( QgsWKBTypes::Point );
      p.setX( coordinate[0].toDouble() );
      p.setY( coordinate[1].toDouble() );
      points.append( p );
    }
  }
  return points;
}

QgsKMLImport::StyleData QgsKMLImport::parseStyle( const QDomElement &styleEl, QuaZip *zip ) const
{
  StyleData style;

  QDomElement lineStyleEl = styleEl.firstChildElement( "LineStyle" );
  QDomElement lineWidthEl = lineStyleEl.firstChildElement( "width" );
  style.outlineSize = lineWidthEl.isNull() ? 1 : lineWidthEl.text().toDouble();

  QDomElement lineColorEl = lineStyleEl.firstChildElement( "color" );
  style.outlineColor = lineColorEl.isNull() ? QColor( Qt::black ) : parseColor( lineColorEl.text() );

  QDomElement polyStyleEl = styleEl.firstChildElement( "PolyStyle" );
  QDomElement fillColorEl = polyStyleEl.firstChildElement( "color" );
  style.fillColor = fillColorEl.isNull() ? QColor( Qt::white ) : parseColor( fillColorEl.text() );

  if ( polyStyleEl.firstChildElement( "fill" ).text() == "0" )
  {
    style.fillColor = QColor( Qt::transparent );
  }
  if ( polyStyleEl.firstChildElement( "outline" ).text() == "0" )
  {
    style.outlineColor = QColor( Qt::transparent );
  }

  QDomElement iconHRefEl = styleEl.firstChildElement( "IconStyle" ).firstChildElement( "Icon" ).firstChildElement( "href" );
  if ( !iconHRefEl.isNull() )
  {
    QString filename = iconHRefEl.text();
    // Only local files in KMZ are supported (also for security reasons)
    if ( zip && zip->setCurrentFile( filename ) )
    {
      QuaZipFile file( zip );
      if ( file.open( QIODevice::ReadOnly ) )
      {
        QImage icon = QImage::fromData( file.readAll() );
        style.icon = QgsTemporaryFile::createNewFile( "kml_import.png" );
        icon.save( style.icon );
      }
    }
  }
  return style;
}

QList<QgsAbstractGeometryV2*> QgsKMLImport::parseGeometries( const QDomElement& containerEl )
{
  QList<QgsAbstractGeometryV2*> geoms;

  QDomElement pointEl = containerEl.firstChildElement( "Point" );
  if ( !pointEl.isNull() )
  {
    QList<QgsPointV2> points = parseCoordinates( pointEl );
    if ( !points.isEmpty() )
    {
      geoms.append( points[0].clone() );
    }
  }

  QDomElement lineStringEl = containerEl.firstChildElement( "LineString" );
  if ( !lineStringEl.isNull() )
  {
    QgsLineStringV2* line = new QgsLineStringV2();
    line->setPoints( parseCoordinates( lineStringEl ) );
    geoms.append( line );
  }

  QDomElement polygonEl = containerEl.firstChildElement( "Polygon" );
  if ( !polygonEl.isNull() )
  {
    QDomElement outerRingEl = polygonEl.firstChildElement( "outerBoundaryIs" ).firstChildElement( "LinearRing" );
    QgsLineStringV2* exterior = new QgsLineStringV2();
    exterior->setPoints( parseCoordinates( outerRingEl ) );
    QgsPolygonV2* poly = new QgsPolygonV2();
    poly->setExteriorRing( exterior );

    QDomNodeList innerBoundaryEls = polygonEl.elementsByTagName( "innerBoundaryIs" );
    for ( int iRing = 0, nRings = innerBoundaryEls.size(); iRing < nRings; ++iRing )
    {
      QDomElement innerRingEl = innerBoundaryEls.at( iRing ).toElement().firstChildElement( "LinearRing" );
      QgsLineStringV2* interior = new QgsLineStringV2();
      interior->setPoints( parseCoordinates( innerRingEl ) );
      poly->addInteriorRing( interior );
    }
    geoms.append( poly );
  }

  QDomElement multiGeometryEl = containerEl.firstChildElement( "MultiGeometry" );
  if ( !multiGeometryEl.isNull() )
  {
    geoms.append( parseGeometries( multiGeometryEl ) );
  }
  return geoms;
}

QColor QgsKMLImport::parseColor( const QString& abgr ) const
{
  if ( abgr.length() < 8 )
  {
    return Qt::black;
  }
  int a = abgr.mid( 0, 2 ).toInt( nullptr, 16 );
  int b = abgr.mid( 2, 2 ).toInt( nullptr, 16 );
  int g = abgr.mid( 4, 2 ).toInt( nullptr, 16 );
  int r = abgr.mid( 6, 2 ).toInt( nullptr, 16 );
  return QColor( r, g, b, a );
}
