#include "qgskmlexport.h"
#include "qgsabstractgeometryv2.h"
#include "qgsgeometry.h"
#include "qgskmlpallabeling.h"
#include "qgsrendererv2.h"
#include "qgssymbolv2.h"
#include "qgssymbollayerv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgsvectorlayer.h"
#include <QIODevice>
#include <QTextCodec>
#include <QTextStream>

QgsKMLExport::QgsKMLExport()
{

}

QgsKMLExport::~QgsKMLExport()
{

}

int QgsKMLExport::writeToDevice( QIODevice *d, const QgsMapSettings& settings, bool visibleExtentOnly )
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
      QgsRectangle filterRect;
      if ( visibleExtentOnly )
      {
        filterRect = QgsCoordinateTransform( settings.destinationCrs(), vl->crs() ).transform( settings.extent() );
      }
      writeVectorLayerFeatures( vl, outStream, filterRect, labelLayer, labeling, rc );
    }
  }

  labeling.drawLabeling( labelingRc );
  outStream << "</Document>" << "\n";
  outStream << "</kml>";
  return 0;
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

bool QgsKMLExport::writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream, const QgsRectangle& filterExtent, bool labelLayer, QgsKMLPalLabeling& labeling, QgsRenderContext& rc )
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

  QgsCoordinateReferenceSystem wgs84;
  wgs84.createFromId( 4326 );
  QgsCoordinateTransform ct( vl->crs(), wgs84 );

  QgsFeatureRequest req;
  if ( !filterExtent.isNull() )
  {
    req.setFilterRect( filterExtent );
  }
  QgsFeatureIterator it = vl->getFeatures( req );
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
        outStream << QString( "<name>%1</name>" )
        .arg( QgsExpression( expr ).evaluate( f ).toString() );
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
        geom->transform( ct ); //KML must be WGS84
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
    //first transform extent to WGS84
    QgsCoordinateReferenceSystem wgs84;
    wgs84.createFromId( 4326 );
    QgsCoordinateTransform ct(( *it )->crs(), wgs84 );
    QgsRectangle wgs84BBox = ct.transformBoundingBox(( *it )->extent() );

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

QString QgsKMLExport::convertToHexValue( int value )
{
  QString s = QString::number( value, 16 );
  if ( s.length() < 2 )
  {
    s.prepend( "0" );
  }
  return s;
}
