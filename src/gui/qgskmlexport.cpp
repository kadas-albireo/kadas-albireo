#include "qgskmlexport.h"
#include "qgsabstractgeometryv2.h"
#include "qgsannotationitem.h"
#include "qgsbillboardregistry.h"
#include "qgscrscache.h"
#include "qgsgeometry.h"
#include "qgskmlpallabeling.h"
#include "qgsmaplayerrenderer.h"
#include "qgspluginlayer.h"
#include "qgsrasterlayer.h"
#include "qgsrendercontext.h"
#include "qgsrendererv2.h"
#include "qgssymbolv2.h"
#include "qgssymbollayerv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgsvectorlayer.h"
#include "qgstextannotationitem.h"
#include "qgsgeoimageannotationitem.h"
#include <QIODevice>
#include <QTextCodec>
#include <QTextStream>
#include <quazip/quazipfile.h>
#include <QUuid>

QgsKMLExport::QgsKMLExport()
{

}

QgsKMLExport::~QgsKMLExport()
{

}

int QgsKMLExport::writeToDevice( QIODevice *d, const QgsMapSettings& settings, QStringList& usedTemporaryFiles, QList<QgsMapLayer*>& superOverlayLayers )
{
  if ( !d )
  {
    return 1;
  }

  if ( !d->isOpen() && !d->open( QIODevice::WriteOnly ) )
  {
    return 2;
  }



  QTextStream outStream( d );
  outStream.setCodec( QTextCodec::codecForName( "UTF-8" ) );

  outStream << "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" << "\n";
  outStream << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << "\n";
  outStream << "<Document>" << "\n";

  writeSchemas( outStream );

  //todo: get extent from all layers. Units always degrees
  QgsKMLPalLabeling labeling( &outStream, bboxFromLayers(), settings.scale(), QGis::Degrees );
  QgsRenderContext& labelingRc = labeling.renderContext();

  QgsRenderContext rc = QgsRenderContext::fromMapSettings( settings );

  QList<QgsMapLayer*>::iterator layerIt = mLayers.begin();
  QgsMapLayer* ml = 0;
  for ( ; layerIt != mLayers.end(); ++layerIt )
  {

    ml = *layerIt;
    if ( !ml )
    {
      continue;
    }

    QgsRectangle layerWgs84Extent = wgs84LayerExtent( ml );

    if ( ml->type() == QgsMapLayer::VectorLayer || ml->type() == QgsMapLayer::RedliningLayer )
    {
      QgsVectorLayer* vl = dynamic_cast<QgsVectorLayer*>( ml );

      QStringList attributes;
      const QgsFields& fields = vl->pendingFields();
      for ( int i = 0; i < fields.size(); ++i )
      {
        attributes.append( fields.at( i ).name() );
      }

      bool labelLayer = labeling.prepareLayer( vl, attributes, labelingRc ) != 0;

      outStream << "<Folder>" << "\n";
      outStream << "<name>" << vl->name() << "</name>" << "\n";
      writeBillboards( vl->id(), outStream, usedTemporaryFiles );
      writeVectorLayerFeatures( vl, outStream, labelLayer, labeling, rc );
      outStream << "</Folder>" << "\n";
    }
    else if ( ml->type() == QgsMapLayer::PluginLayer )
    {
      QgsPluginLayer* pl = static_cast<QgsPluginLayer*>( ml );
      if ( pl && pl->pluginLayerType() == "MilX_Layer" )
      {
        outStream << "<Folder>" << "\n";
        outStream << "<name>" << pl->name() << "</name>" << "\n";
        writeBillboards( pl->id(), outStream, usedTemporaryFiles );

        //reference to start kml (start with quadratic extent 256x256)
        QgsRectangle overlayStartExtent = superOverlayStartExtent( layerWgs84Extent );

        writeNetworkLink( outStream, overlayStartExtent, ml->id() + "_0" + ".kml" );
        superOverlayLayers.append( ml );

        outStream << "</Folder>" << "\n";
      }
    }
    else if ( ml->type() == QgsMapLayer::RasterLayer )
    {
#if 0
      //wms layer?
      QgsRasterLayer* rl = dynamic_cast<QgsRasterLayer*>( ml );
      if ( rl && rl->providerType() == "wms" )
      {
        QString wmsString = rl->source();
        QStringList wmsParamsList = wmsString.split( "&" );
        QStringList::const_iterator paramIt = wmsParamsList.constBegin();
        QMap<QString, QString> parameterMap;
        for ( ; paramIt != wmsParamsList.constEnd(); ++paramIt )
        {
          QStringList eqSplit = paramIt->split( "=" );
          if ( eqSplit.size() >= 2 )
          {
            parameterMap.insert( eqSplit.at( 0 ).toUpper(), eqSplit.at( 1 ) );
          }
        }
        writeWMSOverlay( outStream, wgs84LayerExtent( ml ), parameterMap.value( "URL" ) , parameterMap.value( "VERSION" ), parameterMap.value( "FORMAT" ), parameterMap.value( "LAYERS" ), parameterMap.value( "STYLES" ) );
      }
      else //normal raster layer
      {
#endif
        outStream << "<Folder>" << "\n";
        outStream << "<name>" << ml->name() << "</name>" << "\n";
        writeBillboards( ml->id(), outStream, usedTemporaryFiles );

        //reference to start kml (start with quadratic extent 256x256)
        QgsRectangle overlayStartExtent = superOverlayStartExtent( wgs84LayerExtent( ml ) );

        writeNetworkLink( outStream, overlayStartExtent, ml->id() + "_0" + ".kml" );
        superOverlayLayers.append( ml );
        outStream << "</Folder>" << "\n";
#if 0
      }
#endif
    }
  }

  labeling.drawLabeling( labelingRc );

  if ( mExportAnnotations )
  {
    outStream << "<Folder>" << "\n";
    outStream << "<name>" << "Annotations" << "</name>" << "\n";
    writeBillboards( "", outStream, usedTemporaryFiles );
    outStream << "</Folder>" << "\n";
  }

  outStream << "</Document>" << "\n";
  outStream << "</kml>";
  return 0;
}

bool QgsKMLExport::addSuperOverlayLayer( QgsMapLayer* mapLayer, QuaZip* quaZip, const QString& filePath, int drawingOrder,
    const QgsCoordinateReferenceSystem& mapCRS, double mapUnitsPerPixel )
{
  if ( !mapLayer )
  {
    return false;
  }

  int currentTileNumber = -1; //kml files and .png tiles are named <layerid>_<currentTileNumber>.kml / png

  QgsRectangle mapCRSRect = QgsCoordinateTransformCache::instance()->transform( mapLayer->crs().authid(), mapCRS.authid() )->transformBoundingBox( mapLayer->extent() );

  //add margin
  double extension = mapUnitsPerPixel * mapLayer->margin();
  mapCRSRect.setXMinimum( mapCRSRect.xMinimum() - extension );
  mapCRSRect.setYMinimum( mapCRSRect.yMinimum() - extension );
  mapCRSRect.setXMaximum( mapCRSRect.xMaximum() + extension );
  mapCRSRect.setYMaximum( mapCRSRect.yMaximum() + extension );

  //transform back to wgs84
  QgsRectangle overlayStartExtent = QgsCoordinateTransformCache::instance()->transform( mapCRS.authid(), "EPSG:4326" )->transformBoundingBox( mapCRSRect );

  //start extent needs to be a square
  QgsPoint centerPoint = overlayStartExtent.center();
  double midPointDist = ( overlayStartExtent.width() > overlayStartExtent.height() ) ? overlayStartExtent.width() / 2.0 : overlayStartExtent.height() / 2.0;
  overlayStartExtent = QgsRectangle( centerPoint.x() - midPointDist, centerPoint.y() - midPointDist, centerPoint.x() + midPointDist, centerPoint.y() + midPointDist );

  addOverlay( overlayStartExtent, mapLayer, quaZip, filePath, currentTileNumber, drawingOrder );
  return true;
}

void QgsKMLExport::addOverlay( const QgsRectangle& extent, QgsMapLayer* mapLayer, QuaZip* quaZip, const QString& filePath, int& currentTileNumber, int drawingOrder )
{
  ++currentTileNumber;
  QString fileBaseName = mapLayer->id() + "_" + QString::number( currentTileNumber );

  //render tile, save file
  QgsRenderContext context;
  QImage img( 256, 256, QImage::Format_ARGB32_Premultiplied );
  img.fill( 0 );
  QPainter p( &img );
  context.setPainter( &p );

  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mapLayer->crs().authid(), "EPSG:4326" );

  context.setCoordinateTransform( crst );
  QgsPoint centerPoint = extent.center();
  QgsMapToPixel mtp( extent.width() / 256.0, centerPoint.x(), centerPoint.y(), 256, 256, 0.0 );
  context.setMapToPixel( mtp );
  context.setExtent( crst->transformBoundingBox( extent, QgsCoordinateTransform::ReverseTransform ) );
  context.setCustomRenderFlags( "kml" );
  QgsMapLayerRenderer* layerRenderer = mapLayer->createMapRenderer( context );
  if ( !layerRenderer || !layerRenderer->render() )
  {
    return;
  }

  QIODevice* imageOutDevice = openDeviceForNewFile( fileBaseName + ".png", quaZip, filePath );
  if ( !imageOutDevice )
  {
    return;
  }
  img.save( imageOutDevice, "PNG" );
  imageOutDevice->close();
  delete imageOutDevice;

  //create kml file
  QIODevice* kmlOutDevice = openDeviceForNewFile( fileBaseName + ".kml", quaZip, filePath );
  if ( !kmlOutDevice )
  {
    return;
  }
  QTextStream outStream( kmlOutDevice );
  outStream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << "\n";
  outStream << "<kml xmlns=\"http://www.opengis.net/kml/2.2\">" << "\n";
  outStream << "<Document>" << "\n";


  //create NetworkLinks for four subtiles if resolution has not yet reached the minimum...
  double resolution = extent.width() / 256.0;
  double minResolution = 0.0001;

  int minLodPixels = ( currentTileNumber == 0 ) ? 0 : 192;
  int maxLodPixels = ( resolution > minResolution ) ? 384 : -1;

  //Region
  outStream << "<Region>" << "\n";
  outStream << "<LatLonAltBox>" << "\n";
  writeRectangle( outStream, extent );
  outStream << "</LatLonAltBox>" << "\n";
  outStream << "<Lod><minLodPixels>" << minLodPixels << "</minLodPixels><maxLodPixels>" << maxLodPixels << "</maxLodPixels></Lod>" << "\n";
  outStream << "</Region>" << "\n";

  writeGroundOverlay( outStream, fileBaseName + ".png", extent, drawingOrder );



  QgsRectangle upperLeft( extent.xMinimum(), centerPoint.y(), centerPoint.x(), extent.yMaximum() );
  QgsRectangle lowerLeft( extent.xMinimum(), extent.yMinimum(), centerPoint.x(), centerPoint.y() );
  QgsRectangle upperRight( centerPoint.x(), centerPoint.y(), extent.xMaximum(), extent.yMaximum() );
  QgsRectangle lowerRight( centerPoint.x(), extent.yMinimum(), extent.xMaximum(), centerPoint.y() );

  //how many levels to go?
  int remainingLevels = levelsToGo( resolution, minResolution );
  int tileNumberOffset = offset( remainingLevels );

  //offset to add to currentTileNumber?

  if ( resolution > minResolution )
  {
    writeNetworkLink( outStream, upperLeft,  mapLayer->id() + "_" + QString::number( currentTileNumber + 1 ) + ".kml" );
    writeNetworkLink( outStream, lowerLeft,  mapLayer->id() + "_" + QString::number( currentTileNumber + 1 + tileNumberOffset ) + ".kml" );
    writeNetworkLink( outStream, upperRight,  mapLayer->id() + "_" + QString::number( currentTileNumber + 1 + 2 * tileNumberOffset ) + ".kml" );
    writeNetworkLink( outStream, lowerRight,  mapLayer->id() + "_" + QString::number( currentTileNumber + 1 + 3 * tileNumberOffset ) + ".kml" );
  }

  outStream << "</Document>" << "\n";
  outStream << "</kml>" << "\n";
  outStream.flush();

  kmlOutDevice->close();
  delete kmlOutDevice;

  if ( resolution > minResolution )
  {
    addOverlay( upperLeft, mapLayer, quaZip, filePath, currentTileNumber, drawingOrder + 1 );
    addOverlay( lowerLeft, mapLayer, quaZip, filePath, currentTileNumber, drawingOrder + 1 );
    addOverlay( upperRight, mapLayer, quaZip, filePath, currentTileNumber, drawingOrder + 1 );
    addOverlay( lowerRight, mapLayer, quaZip, filePath, currentTileNumber, drawingOrder + 1 );
  }
}

void QgsKMLExport::writeSchemas( QTextStream& outStream )
{
  QList<QgsMapLayer*>::const_iterator layerIt = mLayers.constBegin();
  for ( ; layerIt != mLayers.constEnd(); ++layerIt )
  {
    const QgsVectorLayer* vl = dynamic_cast<const QgsVectorLayer*>( *layerIt );
    if ( !vl )
    {
      continue;
    }
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

bool QgsKMLExport::writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream, bool labelLayer, QgsKMLPalLabeling& labeling, QgsRenderContext& rc )
{
  if ( !vl )
  {
    return false;
  }

  QgsFeatureRendererV2* renderer = vl->rendererV2();
  if ( !renderer )
  {
    return false;
  }
  renderer->startRender( rc, vl->pendingFields() );

  const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( vl->crs().authid(), "EPSG:4326" );

  QgsFeatureIterator it = vl->getFeatures( );
  QgsFeature f;
  while ( it.nextFeature( f ) )
  {
    outStream << "<Placemark>" << "\n";
    if ( f.geometry() && f.geometry()->geometry() )
    {
      // name (from labeling expression)
      if ( vl->customProperty( "labeling/enabled", false ).toBool() == true &&
           !vl->customProperty( "labeling/fieldName" ).isNull() )
      {
        QString expr = QString( "\"%1\"" ).arg( vl->customProperty( "labeling/fieldName" ).toString() );
        outStream << QString( "<name>%1</name>\n" )
        .arg( QgsExpression( expr ).evaluate( f ).toString() );
      }
      else
      {
        outStream << QString( "<name>Feature %1</name>\n" ).arg( f.id() );
      }

      //style
      addStyle( outStream, f, *renderer, rc );

      //attributes
      outStream << QString( "<ExtendedData><SchemaData schemaUrl=\"#%1\">" ).arg( vl->name() ) << "\n";

      const QgsFields* fields = f.fields();


      const QgsAttributes& attributes = f.attributes();
      QgsAttributes::const_iterator attIt = attributes.constBegin();

      int fieldIndex = 0;
      for ( ; attIt != attributes.constEnd(); ++attIt )
      {
        outStream << QString( "<SimpleData name=\"%1\">%2</SimpleData>" ).arg( fields->at( fieldIndex ).name() ).arg( attIt->toString() ) << "\n";
        ++fieldIndex;
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
  return true;
}

void QgsKMLExport::writeBillboards( const QString& layerId, QTextStream& outStream, QStringList& usedTemporaryFiles )
{
  //write billboards
  foreach ( QgsBillBoardItem* item, QgsBillBoardRegistry::instance()->items() )
  {
    if ( item->layerId == layerId )
    {
      QTemporaryFile tfile( QDir::temp().absoluteFilePath( "XXXXXX.png" ) );
      tfile.setAutoRemove( false );
      if ( !item->image.save( &tfile ) )
      {
        continue;
      }
      usedTemporaryFiles << tfile.fileName();
      QString id = QUuid().toString();
      id = id.mid( 1, id.length() - 2 );
      outStream << QString( "<StyleMap id=\"%1\">\n" ).arg( id );
      outStream << "  <Pair>\n";
      outStream << "    <key>normal</key>\n";
      outStream << "    <Style>\n";
      outStream << "      <IconStyle>\n";
      outStream << "        <scale>1.1</scale>\n";
      outStream << QString( "        <Icon><href>%1</href></Icon>\n" ).arg( QFileInfo( tfile ).fileName() );
      outStream << "      </IconStyle>\n";
      outStream << "    </Style>\n";
      outStream << "  </Pair>\n";
      outStream << "  <Pair>\n";
      outStream << "    <key>highlight</key>\n";
      outStream << "    <Style>\n";
      outStream << "      <IconStyle>\n";
      outStream << "        <scale>1.1</scale>\n";
      outStream << QString( "        <Icon><href>%1</href></Icon>\n" ).arg( QFileInfo( tfile ).fileName() );
      outStream << "      </IconStyle>\n";
      outStream << "    </Style>\n";
      outStream << "  </Pair>\n";
      outStream << "</StyleMap>\n";
      outStream << "<Placemark>\n";
      outStream << QString( "  <name>%1</name>\n" ).arg( item->name );
      outStream << QString( "  <styleUrl>#%1</styleUrl>\n" ).arg( id );
      outStream << "  <Point>\n";
      outStream << QString( "    <coordinates>%1,%2,0</coordinates>\n" ).arg( item->worldPos.x(), 0, 'f', 10 ).arg( item->worldPos.y(), 0, 'f', 10 );
      outStream << "  </Point>\n";
      outStream << "</Placemark>\n";
    }
  }
}

void QgsKMLExport::addStyle( QTextStream& outStream, QgsFeature& f, QgsFeatureRendererV2& r, QgsRenderContext& rc )
{
  //take first symbollayer
  QgsSymbolV2List symbolList = r.symbolsForFeature( f );
  if ( symbolList.size() < 1 )
  {
    return;
  }

  QgsSymbolV2* s = symbolList.first();
  if ( !s || s->symbolLayerCount() < 1 )
  {
    return;
  }
  QgsExpression* expr;

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
  QString colorString;
  colorString.append( convertToHexValue( c.alpha() ) );
  colorString.append( convertToHexValue( c.blue() ) );
  colorString.append( convertToHexValue( c.green() ) );
  colorString.append( convertToHexValue( c.red() ) );
  return colorString;
}

QgsRectangle QgsKMLExport::bboxFromLayers()
{
  QgsRectangle extent;
  QList<QgsMapLayer*>::const_iterator it = mLayers.constBegin();
  for ( ; it != mLayers.constEnd(); ++it )
  {
    QgsRectangle wgs84BBox = wgs84LayerExtent( *it );
    if ( extent.isEmpty() )
    {
      extent = wgs84BBox;
    }
    else
    {
      extent.combineExtentWith( &wgs84BBox );
    }
  }

  return extent;
}

QgsRectangle QgsKMLExport::wgs84LayerExtent( QgsMapLayer* ml )
{
  if ( !ml )
  {
    return QgsRectangle();
  }
  return QgsCoordinateTransformCache::instance()->transform( ml->crs().authid(), "EPSG:4326" )->transformBoundingBox( ml->extent() );
}

QgsRectangle QgsKMLExport::superOverlayStartExtent( const QgsRectangle& wgs84Extent )
{
  QgsPoint centerPoint = wgs84Extent.center();
  double midPointDist = ( wgs84Extent.width() > wgs84Extent.height() ) ? wgs84Extent.width() / 2.0 : wgs84Extent.height() / 2.0;
  return QgsRectangle( centerPoint.x() - midPointDist, centerPoint.y() - midPointDist, centerPoint.x() + midPointDist, centerPoint.y() + midPointDist );
}

QString QgsKMLExport::convertToHexValue( int value )
{
  QString s = QString::number( value, 16 );
  if ( s.length() < 2 )
  {
    s.prepend( "0" );
  }
  return s;
}

QIODevice* QgsKMLExport::openDeviceForNewFile( const QString& fileName, QuaZip* quaZip, const QString& filePath )
{
  if ( !quaZip )
  {
    return 0;
  }

  QuaZipFile* outputFile = new QuaZipFile( quaZip );
  outputFile->open( QIODevice::WriteOnly, QuaZipNewInfo( fileName ) );
  return outputFile;
}

void QgsKMLExport::writeRectangle( QTextStream& outStream, const QgsRectangle& rect )
{
  outStream << "<north>" << rect.yMaximum() << "</north>" << "\n";
  outStream << "<south>" << rect.yMinimum() << "</south>" << "\n";
  outStream << "<east>" << rect.xMaximum() << "</east>" << "\n";
  outStream << "<west>" << rect.xMinimum() << "</west>" << "\n";
}

void QgsKMLExport::writeNetworkLink( QTextStream& outStream, const QgsRectangle& rect, const QString& link )
{
  outStream << "<NetworkLink>" << "\n";
  outStream << "<name>Tiles</name>" << "\n";
  outStream << "<Region>" << "\n";
  outStream << "<LatLonAltBox>" << "\n";
  writeRectangle( outStream, rect );
  outStream << "</LatLonAltBox>" << "\n";
  outStream << "<Lod><minLodPixels>128</minLodPixels><maxLodPixels>-1</maxLodPixels></Lod>" << "\n";
  outStream << "</Region>" << "\n";
  outStream << "<Link>" << "\n";
  outStream << "<href>" << link << "</href>" << "\n";
  outStream << "</Link>" << "\n";
  outStream << "</NetworkLink>" << "\n";
}

void QgsKMLExport::writeWMSOverlay( QTextStream& outStream, const QgsRectangle& latLongBox, const QString& baseUrl, const QString& version, const QString& format, const QString& layers, const QString& styles )
{
  QString href = baseUrl + "SERVICE=WMS&amp;VERSION=1.1.1&amp;SRS=EPSG:4326&amp;REQUEST=GetMap&amp;TRANSPARENT=TRUE&amp;WIDTH=512&amp;HEIGHT=512&amp;FORMAT=" + format + "&amp;LAYERS=" + layers + "&amp;STYLES=" + styles;
  writeGroundOverlay( outStream, href, latLongBox, -1 );
}

void QgsKMLExport::writeGroundOverlay( QTextStream& outStream, const QString& href, const QgsRectangle& latLongBox, int drawingOrder )
{
  outStream << "<GroundOverlay>" << "\n";
  if ( drawingOrder >= 0 )
  {
    outStream << "<drawOrder>" << QString::number( drawingOrder ) << "</drawOrder>" << "\n";
  }
  outStream << "<Icon><href>" << href << "</href></Icon>" << "\n";
  outStream << "<LatLonBox>" << "\n";
  writeRectangle( outStream, latLongBox );
  outStream << "</LatLonBox>" << "\n";
  outStream << "</GroundOverlay>" << "\n";
}

int QgsKMLExport::levelsToGo( double resolution, double minResolution )
{
  int levelsToGo = 0;
  while ( resolution > minResolution )
  {
    ++levelsToGo;
    resolution /= 2;
  }
  return levelsToGo;
}

int QgsKMLExport::offset( int nLevelsToGo )
{
  int offset = 0;
  int exponent = 0;
  for ( int i = 0; i < nLevelsToGo; ++i )
  {
    offset += qPow( 4.0, exponent );
    ++exponent;
  }
  return offset;
}
