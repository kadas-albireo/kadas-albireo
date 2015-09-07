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
