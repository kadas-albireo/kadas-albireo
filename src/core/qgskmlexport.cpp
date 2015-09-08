#include "qgskmlexport.h"
#include "qgsabstractgeometryv2.h"
#include "qgsgeometry.h"
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

int QgsKMLExport::writeToDevice( QIODevice *d )
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

  QList<QgsMapLayer*>::iterator layerIt = mLayers.begin();
  QgsMapLayer* ml = 0;
  for ( ; layerIt != mLayers.end(); ++layerIt )
  {
    qWarning( "Exporting maplayer to KML" );
    ml = *layerIt;
    if ( !ml )
    {
      continue;
    }

    if ( ml->type() == QgsMapLayer::VectorLayer )
    {
      writeVectorLayerFeatures( dynamic_cast<QgsVectorLayer*>( ml ), outStream );
    }
  }

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

bool QgsKMLExport::writeVectorLayerFeatures( QgsVectorLayer* vl, QTextStream& outStream )
{
  if ( !vl )
  {
    return false;
  }

  QgsCoordinateReferenceSystem wgs84;
  wgs84.createFromId( 4326 );
  QgsCoordinateTransform ct( vl->crs(), wgs84 );

  QgsFeatureIterator it = vl->getFeatures();
  QgsFeature f;
  QgsAbstractGeometryV2* geom = 0;
  while ( it.nextFeature( f ) )
  {
    outStream << "<Placemark>" << "\n";
    if ( f.geometry() )
    {
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

      geom = f.geometry()->geometry();
      if ( geom )
      {
        geom->transform( ct ); //KML must be WGS84
        outStream << geom->asKML( 3 );
      }
    }
    outStream << "</Placemark>" << "\n";
  }

  return true;
}
