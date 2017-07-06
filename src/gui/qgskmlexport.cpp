/***************************************************************************
 *  qgskmlexportdialog.h                                                   *
 *  -----------                                                            *
 *  begin                : October 2015                                    *
 *  copyright            : (C) 2015 by Marco Hugentobler / Sourcepole AG   *
 *  email                : marco@sourcepole.ch                             *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#include "qgskmlexport.h"
#include "qgsbillboardregistry.h"
#include "qgscrscache.h"
#include "qgsgeometry.h"
#include "qgskmlpallabeling.h"
#include "qgsmaplayerrenderer.h"
#include "qgsrasterlayer.h"
#include "qgsrendercontext.h"
#include "qgsrendererv2.h"
#include "qgsscalecalculator.h"
#include "qgssymbollayerv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgsvectorlayer.h"

#include <QApplication>
#include <QProgressDialog>
#include <QIODevice>
#include <QTextStream>
#include <QUuid>
#include <quazip/quazipfile.h>

bool QgsKMLExport::exportToFile( const QString &filename, const QList<QgsMapLayer *> &layers, bool exportAnnotations, const QgsMapSettings& settings )
{
  // Prepare outputs
  bool kmz = filename.endsWith( ".kmz", Qt::CaseInsensitive );
  QuaZip* quaZip = 0;
  if ( kmz )
  {
    quaZip = new QuaZip( filename );
    if ( !quaZip->open( QuaZip::mdCreate ) )
    {
      delete quaZip;
      return false;
    }
  }
  QString outString;
  QTextStream outStream( &outString );

  QProgressDialog progress( tr( "Plase wait..." ), tr( "Cancel" ), 0, 0 );
  progress.setWindowModality( Qt::ApplicationModal );
  progress.setWindowTitle( tr( "KML Export" ) );
  progress.show();
  QApplication::processEvents();

  // Write document header
  outStream << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" << "\n";
  outStream << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << "\n";
  outStream << "<Document>" << "\n";

  // Write schemas
  foreach ( QgsMapLayer* ml, layers )
  {
    QgsVectorLayer* vl = dynamic_cast<QgsVectorLayer*>( ml );
    if ( vl )
    {
      outStream << QString( "<Schema name=\"%1\" id=\"%1\">" ).arg( vl->name() ) << "\n";
      const QgsFields& layerFields = vl->pendingFields();
      for ( int i = 0; i < layerFields.size(); ++i )
      {
        const QgsField& field = layerFields.at( i );
        outStream << QString( "<SimpleField name=\"%1\" type=\"%2\"></SimpleField>" ).arg( field.name() ).arg( QVariant::typeToName( field.type() ) ) << "\n";
      }
      outStream << QString( "</Schema> " ) << "\n";
    }
  }

  // Prepare rendering
  QgsRectangle fullExtent;
  foreach ( QgsMapLayer* ml, layers )
  {
    QgsRectangle layerExtent = QgsCoordinateTransformCache::instance()->transform( ml->crs().authid(), "EPSG:4326" )->transform( ml->extent() );
    if ( fullExtent.isEmpty() )
      fullExtent = layerExtent;
    else
      fullExtent.combineExtentWith( &layerExtent );
  }
  QgsKMLPalLabeling labeling( &outStream, fullExtent, settings.scale(), QGis::Degrees );
  QgsRenderContext& labelingRc = labeling.renderContext();

  QgsRenderContext rc = QgsRenderContext::fromMapSettings( settings );

  // Render layers
  int drawingOrder = 0;
  foreach ( QgsMapLayer* ml, layers )
  {
    QgsRectangle layerExtent = QgsCoordinateTransformCache::instance()->transform( ml->crs().authid(), "EPSG:4326" )->transform( ml->extent() );
    if ( dynamic_cast<QgsVectorLayer*>( ml ) )
    {
      progress.setLabelText( tr( "Rendering layer %1..." ).arg( ml->name() ) );
      progress.setRange( 0, 0 );
      QApplication::processEvents();
      if ( progress.wasCanceled() )
      {
        delete quaZip;
        return false;
      }

      QgsVectorLayer* vl = static_cast<QgsVectorLayer*>( ml );

      QStringList attributes;
      const QgsFields& fields = vl->pendingFields();
      for ( int i = 0; i < fields.size(); ++i )
      {
        attributes.append( fields.at( i ).name() );
      }
      bool labelLayer = labeling.prepareLayer( vl, attributes, labelingRc ) != 0;

      outStream << "<Folder>" << "\n";
      outStream << "<name>" << vl->name() << "</name>" << "\n";
      if ( kmz )
        writeBillboards( ml->id(), outStream, quaZip );
      writeVectorLayerFeatures( vl, outStream, labelLayer, labeling, rc );
      outStream << "</Folder>" << "\n";
    }
    else if ( kmz ) // Non-vector layers only supported in KMZ
    {
      progress.setLabelText( tr( "Rendering layer %1..." ).arg( ml->name() ) );
      progress.setRange( 0, 0 );
      QApplication::processEvents();
      if ( progress.wasCanceled() )
      {
        delete quaZip;
        return false;
      }

      outStream << "<Folder>" << "\n";
      outStream << "<name>" << ml->name() << "</name>" << "\n";
      writeBillboards( ml->id(), outStream, quaZip );
#if 0
      QgsRasterLayer* rl = dynamic_cast<QgsRasterLayer*>( ml );
      if ( rl && rl->providerType() == "wms" )
      {
        QMap<QString, QString> parameterMap;
        foreach ( const QString& param, rl->source().split( "&" ) )
        {
          QStringList pair = param.split( "=" );
          if ( pair.size() >= 2 )
            parameterMap.insert( pair[0].toUpper(), pair[1] );
        }
        QString baseUrl = parameterMap.value( "URL" );
        QString format = parameterMap.value( "FORMAT", "PNG" );
        QString layers = parameterMap.value( "LAYERS" );
        QString styles = parameterMap.value( "STYLES" );
        QString href = baseUrl + "SERVICE=WMS&amp;VERSION=1.1.1&amp;SRS=EPSG:4326&amp;REQUEST=GetMap&amp;TRANSPARENT=TRUE&amp;WIDTH=512&amp;HEIGHT=512&amp;FORMAT=" + format + "&amp;LAYERS=" + layers + "&amp;STYLES=" + styles;
        writeGroundOverlay( outStream, ml->name(), href, layerExtent, ++drawingOrder );
      }
      else
      {
        writeTiles( ml, layerExtent, outStream, ++drawingOrder, quaZip, &progress );
      }
#else
      writeTiles( ml, layerExtent, outStream, ++drawingOrder, quaZip, &progress );
#endif

      outStream << "</Folder>" << "\n";
    }
    if ( progress.wasCanceled() )
    {
      delete quaZip;
      return false;
    }
  }

  labeling.drawLabeling( labelingRc );

  if ( kmz && exportAnnotations )
  {
    progress.setLabelText( tr( "Rendering annotations..." ) );
    progress.setRange( 0, 0 );
    QApplication::processEvents();

    outStream << "<Folder>" << "\n";
    outStream << "<name>" << "Annotations" << "</name>" << "\n";
    writeBillboards( "", outStream, quaZip );
    outStream << "</Folder>" << "\n";
  }

  outStream << "</Document>" << "\n";
  outStream << "</kml>";
  outStream.flush();

  // Write output
  bool success = false;
  if ( !kmz )
  {
    QFile outFile( filename );
    if ( outFile.open( QIODevice::WriteOnly ) )
    {
      outFile.write( outString.toUtf8() );
      success = true;
    }
  }
  else //KMZ
  {
    QuaZipFile outZipFile( quaZip );
    if ( outZipFile.open( QIODevice::WriteOnly, QuaZipNewInfo( QFileInfo( filename ).baseName() + ".kml" ) ) )
    {
      outZipFile.write( outString.toUtf8() );
      outZipFile.close();
      success = true;
    }
    delete quaZip;
  }
  return success;
}

void QgsKMLExport::writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream, bool labelLayer, QgsKMLPalLabeling& labeling, QgsRenderContext& rc )
{
  QgsFeatureRendererV2* renderer = vl->rendererV2();
  if ( !renderer )
  {
    return;
  }
  renderer->startRender( rc, vl->pendingFields() );

  const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( vl->crs().authid(), "EPSG:4326" );

  QgsFeatureIterator it = vl->getFeatures();
  QgsFeature f;
  while ( it.nextFeature( f ) )
  {
    outStream << "<Placemark>" << "\n";
    if ( f.geometry() && f.geometry()->geometry() )
    {
      // Name (from labeling expression, if possible)
      if ( vl->customProperty( "labeling/enabled", false ).toBool() && !vl->customProperty( "labeling/fieldName" ).isNull() )
      {
        QString expr = QString( "\"%1\"" ).arg( vl->customProperty( "labeling/fieldName" ).toString() );
        outStream << QString( "<name>%1</name>\n" ).arg( QgsExpression( expr ).evaluate( f ).toString() );
      }
      else
      {
        outStream << QString( "<name>Feature %1</name>\n" ).arg( f.id() );
      }

      // Style
      addStyle( outStream, f, *renderer, rc );

      // Attributes
      outStream << QString( "<ExtendedData><SchemaData schemaUrl=\"#%1\">" ).arg( vl->name() ) << "\n";
      const QgsFields& fields = *f.fields();
      const QgsAttributes& attributes = f.attributes();
      for ( int i = 0, n = attributes.size(); i < n; ++i )
      {
        outStream << QString( "<SimpleData name=\"%1\">%2</SimpleData>" ).arg( fields[i].name() ).arg( attributes[i].toString() ) << "\n";
      }
      outStream << QString( "</SchemaData></ExtendedData>" );

      // Segmentize immediately, since otherwise for instance circles are distorted if segmentation occurs after transformation
      QgsAbstractGeometryV2* geom = f.geometry()->geometry()->segmentize();
      if ( geom )
      {
        geom->transform( *ct ); //KML must be WGS84
        outStream << geom->asKML( 6 );
      }
      delete geom;
    }
    outStream << "</Placemark>" << "\n";

    if ( labelLayer )
    {
      labeling.registerFeature( vl->id(), f, rc, vl->name() );
    }
  }
  renderer->stopRender( rc );
}

void QgsKMLExport::writeTiles( QgsMapLayer* mapLayer, const QgsRectangle& layerExtent, QTextStream& outStream, int drawingOrder, QuaZip* quaZip, QProgressDialog* progress )
{
  const int tileSize = 512;
  const double exportScale = 25000.; // 1:25000

  // Make extent square
  QgsPoint center = layerExtent.center();
  double extension = qMax( layerExtent.width(), layerExtent.height() ) * 0.5;
  QgsRectangle renderExtent = QgsRectangle( center.x() - extension, center.y() - extension, center.x() + extension, center.y() + extension );

  // Compute pixels to match extent at scale
  // px / dpi * 0.0254 * scale = meters
  QImage image( tileSize, tileSize, QImage::Format_ARGB32 );
  double meters = QgsScaleCalculator().calculateGeographicDistance( renderExtent );
  int dpi = image.logicalDpiX();
  int totPixels = meters / ( exportScale * 0.0254 ) * dpi;
  double resolution = renderExtent.width() / totPixels;

  // Round up to next <tileSize> multiple, adding margin
  totPixels = qCeil(( totPixels + 2 * mapLayer->margin() ) / double( tileSize ) ) * tileSize;
  extension = totPixels * resolution * 0.5;
  renderExtent = QgsRectangle( center.x() - extension, center.y() - extension, center.x() + extension, center.y() + extension );

  progress->setRange( 0, ( totPixels / tileSize ) * ( totPixels / tileSize ) );
  QApplication::processEvents();

  // Render in <tileSize> blocks
  int tileCounter = 0;
  for ( int iy = 0; iy < totPixels; iy += tileSize )
  {
    for ( int ix = 0; ix < totPixels; ix += tileSize )
    {

      progress->setValue( tileCounter );
      QApplication::processEvents();
      if ( progress->wasCanceled() )
      {
        return;
      }

      QgsRectangle tileExtent( renderExtent.xMinimum() + ix * resolution, renderExtent.yMinimum() + iy * resolution,
                               renderExtent.xMinimum() + ( ix + tileSize ) * resolution, renderExtent.yMinimum() + ( iy + tileSize ) * resolution );
      renderTile( image, tileExtent, mapLayer );

      QString filename = QString( "%1_%2.png" ).arg( mapLayer->id() ).arg( tileCounter++ );
      QuaZipFile outputFile( quaZip );
      if ( outputFile.open( QIODevice::WriteOnly, QuaZipNewInfo( filename ) ) && image.save( &outputFile, "PNG" ) )
        writeGroundOverlay( outStream, QString( "Tile %1" ).arg( tileExtent.toString( 3 ) ), filename, tileExtent, drawingOrder );
    }
  }
}

void QgsKMLExport::writeGroundOverlay( QTextStream& outStream, const QString& name, const QString& href, const QgsRectangle& latLongBox, int drawingOrder )
{
  outStream << "<GroundOverlay>" << "\n";
  outStream << "<name>" << name << "</name>" << "\n";
  outStream << "<drawOrder>" << QString::number( drawingOrder ) << "</drawOrder>" << "\n";
  outStream << "<Icon><href>" << href << "</href></Icon>" << "\n";
  outStream << "<LatLonBox>" << "\n";
  outStream << "<north>" << latLongBox.yMaximum() << "</north>" << "\n";
  outStream << "<south>" << latLongBox.yMinimum() << "</south>" << "\n";
  outStream << "<east>" << latLongBox.xMaximum() << "</east>" << "\n";
  outStream << "<west>" << latLongBox.xMinimum() << "</west>" << "\n";
  outStream << "</LatLonBox>" << "\n";
  outStream << "</GroundOverlay>" << "\n";
}

void QgsKMLExport::writeBillboards( const QString& layerId, QTextStream& outStream, QuaZip* quaZip )
{
  foreach ( QgsBillBoardItem* item, QgsBillBoardRegistry::instance()->items() )
  {
    if ( item->layerId == layerId )
    {
      QString fileName = QUuid::createUuid().toString();
      fileName = fileName.mid( 1, fileName.length() - 2 ) + ".png";
      QuaZipFile outputFile( quaZip );
      if ( !outputFile.open( QIODevice::WriteOnly, QuaZipNewInfo( fileName ) ) || !item->image.save( &outputFile, "PNG" ) )
        continue;

      QString id = QUuid::createUuid().toString();
      id = id.mid( 1, id.length() - 2 );
      QString lon = QString::number( item->worldPos.x(), 'f', 10 );
      QString lat = QString::number( item->worldPos.y(), 'f', 10 );
      outStream << "<StyleMap id=\"" << id << "\">" << "\n";
      outStream << "  <Pair>" << "\n";
      outStream << "    <key>normal</key>" << "\n";
      outStream << "    <Style>" << "\n";
      outStream << "      <IconStyle>" << "\n";
      outStream << "        <scale>1.1</scale>" << "\n";
      outStream << "        <Icon><href>" << fileName << "</href></Icon>" << "\n";
      outStream << "      </IconStyle>" << "\n";
      outStream << "    </Style>" << "\n";
      outStream << "  </Pair>" << "\n";
      outStream << "  <Pair>" << "\n";
      outStream << "    <key>highlight</key>" << "\n";
      outStream << "    <Style>" << "\n";
      outStream << "      <IconStyle>" << "\n";
      outStream << "        <scale>1.1</scale>" << "\n";
      outStream << "        <Icon><href>" << fileName << "</href></Icon>" << "\n";
      outStream << "      </IconStyle>" << "\n";
      outStream << "    </Style>" << "\n";
      outStream << "  </Pair>" << "\n";
      outStream << "</StyleMap>" << "\n";
      outStream << "<Placemark>" << "\n";
      outStream << "  <name>" << item->name << "</name>" << "\n";
      outStream << "  <styleUrl>#" << id << "</styleUrl>" << "\n";
      outStream << "  <Point>" << "\n";
      outStream << "    <coordinates>" << lon << "," << lat << ",0</coordinates>" << "\n";
      outStream << "  </Point>" << "\n";
      outStream << "</Placemark>" << "\n";
    }
  }
}

void QgsKMLExport::renderTile( QImage& img, const QgsRectangle& extent, QgsMapLayer* mapLayer )
{
  QTextStream( stdout ) << "** Rendering " << extent.toString( 3 ) << endl;

  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mapLayer->crs().authid(), "EPSG:4326" );
  QgsRenderContext context;
  img.fill( 0 );
  QPainter p( &img );
  context.setPainter( &p );
  context.setCoordinateTransform( crst );
  QgsPoint centerPoint = extent.center();
  QgsMapToPixel mtp( extent.width() / img.width(), centerPoint.x(), centerPoint.y(), img.width(), img.height(), 0.0 );
  context.setMapToPixel( mtp );
  context.setExtent( crst->transformBoundingBox( extent, QgsCoordinateTransform::ReverseTransform ) );
  context.setCustomRenderFlags( "kml" );
  QgsMapLayerRenderer* layerRenderer = mapLayer->createMapRenderer( context );
  if ( layerRenderer )
    layerRenderer->render();
  delete layerRenderer;
}

void QgsKMLExport::addStyle( QTextStream& outStream, QgsFeature& f, QgsFeatureRendererV2& r, QgsRenderContext& rc )
{
  // Take first symbollayer
  QgsSymbolV2List symbolList = r.symbolsForFeature( f );
  if ( symbolList.isEmpty() )
  {
    return;
  }

  QgsSymbolV2* s = symbolList.first();
  if ( !s || s->symbolLayerCount() < 1 )
  {
    return;
  }
  QgsExpression* expr = 0;

  outStream << "<Style>";
  if ( s->type() == QgsSymbolV2::Line )
  {
    double width = 1;
    expr = s->symbolLayer( 0 )->expression( "width" );
    if ( expr )
    {
      width = expr->evaluate( f ).toDouble( ) * QgsSymbolLayerV2Utils::lineWidthScaleFactor( rc, QgsSymbolV2::MM );
    }
    else if ( dynamic_cast<QgsLineSymbolLayerV2*>( s->symbolLayer( 0 ) ) )
    {
      QgsLineSymbolLayerV2* lineSymbolLayer = static_cast<QgsLineSymbolLayerV2*>( s->symbolLayer( 0 ) );
      width = lineSymbolLayer->width() * QgsSymbolLayerV2Utils::lineWidthScaleFactor( rc, lineSymbolLayer->widthUnit() );
    }

    QColor c = s->symbolLayer( 0 )->color();
    expr = s->symbolLayer( 0 )->expression( "color" );
    if ( expr )
    {
      c = QgsSymbolLayerV2Utils::decodeColor( expr->evaluate( f ).toString() );
    }

    outStream << QString( "<LineStyle><color>%1</color><width>%2</width></LineStyle>" ).arg( convertColor( c ) ).arg( QString::number( width ) );
  }
  else if ( s->type() == QgsSymbolV2::Fill )
  {
    double width = 1;
    expr = s->symbolLayer( 0 )->expression( "width_border" );
    if ( expr )
    {
      width = expr->evaluate( f ).toDouble( ) * QgsSymbolLayerV2Utils::lineWidthScaleFactor( rc, QgsSymbolV2::MM );
    }

    QColor outlineColor = outlineColor = s->symbolLayer( 0 )->outlineColor();
    expr = s->symbolLayer( 0 )->expression( "color_border" );
    if ( expr )
    {
      outlineColor = QgsSymbolLayerV2Utils::decodeColor( expr->evaluate( f ).toString() );
    }

    QColor fillColor = s->symbolLayer( 0 )->fillColor();
    expr = s->symbolLayer( 0 )->expression( "color" );
    if ( expr )
    {
      fillColor = QgsSymbolLayerV2Utils::decodeColor( expr->evaluate( f ).toString() );
    }

    outStream << QString( "<LineStyle><width>%1</width><color>%2</color></LineStyle><PolyStyle><fill>1</fill><color>%3</color></PolyStyle>" )
    .arg( width ).arg( convertColor( outlineColor ) ).arg( convertColor( fillColor ) );
  }
  outStream << "</Style>\n";
}

QString QgsKMLExport::convertColor( const QColor& c )
{
  return QString( "%1%2%3%4" )
         .arg( c.alpha(), 2, 16, QChar( '0' ) )
         .arg( c.blue(), 2, 16, QChar( '0' ) )
         .arg( c.green(), 2, 16, QChar( '0' ) )
         .arg( c.red(), 2, 16, QChar( '0' ) );
}
