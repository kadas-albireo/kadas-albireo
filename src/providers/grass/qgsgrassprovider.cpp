/***************************************************************************
    qgsgrassprovider.cpp -  Data provider for GRASS format
                             -------------------
    begin                : March, 2004
    copyright            : (C) 2004 by Gary E.Sherman, Radim Blazek
    email                : sherman@mrcc.com, blazek@itc.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <cstring>
#include <vector>
#include <cfloat>

#include <QString>
#include <QDateTime>

#include "qgis.h"
#include "qgsdataprovider.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsrectangle.h"

#include "qgsgrass.h"
#include "qgsgrassprovider.h"
#include "qgsgrassfeatureiterator.h"

#include "qgsapplication.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsfield.h"
#include "qgslogger.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextCodec>

#ifdef _MSC_VER
// enable grass prototypes
#define __STDC__ 1
#endif

extern "C"
{
#include <grass/version.h>
#include <grass/gprojects.h>
#include <grass/gis.h>
#include <grass/dbmi.h>
#if GRASS_VERSION_MAJOR < 7
#include <grass/Vect.h>
#else
#include <grass/vector.h>
#define BOUND_BOX bound_box
#endif
}

#ifdef _MSC_VER
#undef __STDC__
#endif


QVector<GLAYER> QgsGrassProvider::mLayers;
QVector<GMAP> QgsGrassProvider::mMaps;


static QString GRASS_KEY = "grass"; // XXX verify this
static QString GRASS_DESCRIPTION = "Grass provider"; // XXX verify this



QgsGrassProvider::QgsGrassProvider( QString uri )
    : QgsVectorDataProvider( uri )
{
  QgsDebugMsg( QString( "QgsGrassProvider URI: %1" ).arg( uri ) );

  QgsGrass::init();

  QTime time;
  time.start();

  mValid = false;

  // Parse URI
  QDir dir( uri );   // it is not a directory in fact
  QString myURI = dir.path();  // no dupl '/'

  mLayer = dir.dirName();
  myURI = myURI.left( dir.path().lastIndexOf( '/' ) );
  dir = QDir( myURI );
  mMapName = dir.dirName();
  dir.cdUp();
  mMapset = dir.dirName();
  dir.cdUp();
  mLocation = dir.dirName();
  dir.cdUp();
  mGisdbase = dir.path();

  QgsDebugMsg( QString( "gisdbase: %1" ).arg( mGisdbase ) );
  QgsDebugMsg( QString( "location: %1" ).arg( mLocation ) );
  QgsDebugMsg( QString( "mapset: %1" ).arg( mMapset ) );
  QgsDebugMsg( QString( "mapName: %1" ).arg( mMapName ) );
  QgsDebugMsg( QString( "layer: %1" ).arg( mLayer ) );

  /* Parse Layer, supported layers <field>_point, <field>_line, <field>_area
  *  Layer is opened even if it is empty (has no features)
  */
  mLayerField = -1;
  if ( mLayer.compare( "boundary" ) == 0 ) // currently not used
  {
    mLayerType = BOUNDARY;
    mGrassType = GV_BOUNDARY;
  }
  else if ( mLayer.compare( "centroid" ) == 0 ) // currently not used
  {
    mLayerType = CENTROID;
    mGrassType = GV_CENTROID;
  }
  else if ( mLayer == "topo_point" )
  {
    mLayerType = TOPO_POINT;
    mGrassType = GV_POINTS;
  }
  else if ( mLayer == "topo_line" )
  {
    mLayerType = TOPO_LINE;
    mGrassType = GV_LINES;
  }
  else if ( mLayer == "topo_node" )
  {
    mLayerType = TOPO_NODE;
    mGrassType = 0;
  }
  else
  {
    mLayerField = grassLayer( mLayer );
    if ( mLayerField == -1 )
    {
      QgsDebugMsg( QString( "Invalid layer name, no underscore found: %1" ).arg( mLayer ) );
      return;
    }

    mGrassType = grassLayerType( mLayer );

    if ( mGrassType == GV_POINT )
    {
      mLayerType = POINT;
    }
    else if ( mGrassType == GV_LINES )
    {
      mLayerType = LINE;
    }
    else if ( mGrassType == GV_FACE )
    {
      mLayerType = FACE;
    }
    else if ( mGrassType == GV_AREA )
    {
      mLayerType = POLYGON;
    }
    else
    {
      QgsDebugMsg( QString( "Invalid layer name, wrong type: %1" ).arg( mLayer ) );
      return;
    }

  }
  QgsDebugMsg( QString( "mLayerField: %1" ).arg( mLayerField ) );
  QgsDebugMsg( QString( "mLayerType: %1" ).arg( mLayerType ) );

  if ( mLayerType == BOUNDARY || mLayerType == CENTROID )
  {
    QgsDebugMsg( "Layer type not supported." );
    return;
  }

  // Set QGIS type
  switch ( mLayerType )
  {
    case POINT:
    case CENTROID:
    case TOPO_POINT:
    case TOPO_NODE:
      mQgisType = QGis::WKBPoint;
      break;
    case LINE:
    case BOUNDARY:
    case TOPO_LINE:
      mQgisType = QGis::WKBLineString;
      break;
    case POLYGON:
    case FACE:
      mQgisType = QGis::WKBPolygon;
      break;
  }

  mLayerId = openLayer( mGisdbase, mLocation, mMapset, mMapName, mLayerField );
  if ( mLayerId < 0 )
  {
    QgsDebugMsg( QString( "Cannot open GRASS layer:%1" ).arg( myURI ) );
    return;
  }
  QgsDebugMsg( QString( "mLayerId: %1" ).arg( mLayerId ) );

  mMap = layerMap( mLayerId );

  loadMapInfo();
  setTopoFields();

  mMapVersion = mMaps[mLayers[mLayerId].mapId].version;

  mValid = true;

  QgsDebugMsg( QString( "New GRASS layer opened, time (ms): %1" ).arg( time.elapsed() ) );
}

void QgsGrassProvider::loadMapInfo()
{
  // Getting the total number of features in the layer
  mNumberFeatures = 0;
  mCidxFieldIndex = -1;
  if ( mLayerType == TOPO_POINT )
  {
    mNumberFeatures = Vect_get_num_primitives( mMap, GV_POINTS );
  }
  else if ( mLayerType == TOPO_LINE )
  {
    mNumberFeatures = Vect_get_num_primitives( mMap, GV_LINES );
  }
  else if ( mLayerType == TOPO_NODE )
  {
    mNumberFeatures = Vect_get_num_nodes( mMap );
  }
  else
  {
    if ( mLayerField >= 0 )
    {
      mCidxFieldIndex = Vect_cidx_get_field_index( mMap, mLayerField );
      if ( mCidxFieldIndex >= 0 )
      {
        mNumberFeatures = Vect_cidx_get_type_count( mMap, mLayerField, mGrassType );
        mCidxFieldNumCats = Vect_cidx_get_num_cats_by_index( mMap, mCidxFieldIndex );
      }
    }
    else
    {
      // TODO nofield layers
      mNumberFeatures = 0;
      mCidxFieldNumCats = 0;
    }
  }
  QgsDebugMsg( QString( "mNumberFeatures = %1 mCidxFieldIndex = %2 mCidxFieldNumCats = %3" ).arg( mNumberFeatures ).arg( mCidxFieldIndex ).arg( mCidxFieldNumCats ) );

}

void QgsGrassProvider::update( void )
{
  QgsDebugMsg( "entered." );

  mValid = false;

  if ( !mMaps[mLayers[mLayerId].mapId].valid )
    return;

  // Getting the total number of features in the layer
  // It may happen that the field disappeares from the map (deleted features, new map without that field)
  loadMapInfo();

  mMapVersion = mMaps[mLayers[mLayerId].mapId].version;

  mValid = true;
}

QgsGrassProvider::~QgsGrassProvider()
{
  QgsDebugMsg( "entered." );

  closeLayer( mLayerId );
}


QgsAbstractFeatureSource* QgsGrassProvider::featureSource() const
{
  const_cast<QgsGrassProvider*>( this )->ensureUpdated();
  return new QgsGrassFeatureSource( this );
}


QString QgsGrassProvider::storageType() const
{
  return "GRASS (Geographic Resources Analysis and Support System) file";
}



QgsFeatureIterator QgsGrassProvider::getFeatures( const QgsFeatureRequest& request )
{
  if ( isEdited() || isFrozen() || !mValid )
    return QgsFeatureIterator();

  // check if outdated and update if necessary
  ensureUpdated();

  return QgsFeatureIterator( new QgsGrassFeatureIterator( new QgsGrassFeatureSource( this ), true, request ) );
}




QgsRectangle QgsGrassProvider::extent()
{
  BOUND_BOX box;
  Vect_get_map_box( mMap, &box );

  return QgsRectangle( box.W, box.S, box.E, box.N );
}

/**
* Return the feature type
*/
QGis::WkbType QgsGrassProvider::geometryType() const
{
  return mQgisType;
}
/**
* Return the feature type
*/
long QgsGrassProvider::featureCount() const
{
  return mNumberFeatures;
}

/**
* Return fields
*/
const QgsFields & QgsGrassProvider::fields() const
{
  if ( !isTopoType() )
  {
    return mLayers[mLayerId].fields;
  }
  return mTopoFields;
}

int QgsGrassProvider::keyField()
{
  return mLayers[mLayerId].keyColumn;
}

QVariant QgsGrassProvider::minimumValue( int index )
{
  if ( index < 0 || index >= mLayers[mLayerId].fields.count() )
  {
    QgsDebugMsg( "Warning: access requested to invalid field index: " + QString::number( index ) );
    return QVariant();
  }
  return QVariant( mLayers[mLayerId].minmax[index][0] );
}


QVariant QgsGrassProvider::maxValue( int index )
{
  if ( index < 0 || index >= mLayers[mLayerId].fields.count() )
  {
    QgsDebugMsg( "Warning: access requested to invalid field index: " + QString::number( index ) );
    return QVariant();
  }
  return QVariant( mLayers[mLayerId].minmax[index][1] );
}

bool QgsGrassProvider::isValid()
{
  QgsDebugMsg( QString( "returned: %1" ).arg( mValid ? "true" : "false" ) );

  return mValid;
}

// ------------------------------------------------------------------------------------------------------
// Compare categories in GATT
int QgsGrassProvider::cmpAtt( const void *a, const void *b )
{
  GATT *p1 = ( GATT * ) a;
  GATT *p2 = ( GATT * ) b;
  return ( p1->cat - p2->cat );
}

/* returns layerId or -1 on error */
int QgsGrassProvider::openLayer( QString gisdbase, QString location, QString mapset, QString mapName, int field )
{
  QgsDebugMsg( QString( "gisdbase: %1" ).arg( gisdbase ) );
  QgsDebugMsg( QString( "location: %1" ).arg( location ) );
  QgsDebugMsg( QString( "mapset: %1" ).arg( mapset ) );
  QgsDebugMsg( QString( "mapName: %1" ).arg( mapName ) );
  QgsDebugMsg( QString( "field: %1" ).arg( field ) );

  // Check if this layer is already opened

  for ( int i = 0; i <  mLayers.size(); i++ )
  {
    if ( !( mLayers[i].valid ) )
      continue;

    GMAP *mp = &( mMaps[mLayers[i].mapId] );

    if ( mp->gisdbase == gisdbase && mp->location == location &&
         mp->mapset == mapset && mp->mapName == mapName && mLayers[i].field == field )
    {
      // the layer already exists, return layer id
      QgsDebugMsg( QString( "The layer is already opened with ID = %1" ).arg( i ) );
      mLayers[i].nUsers++;
      return i;
    }
  }

  // Create a new layer
  GLAYER layer;
  layer.valid = false;
  layer.field = field;
  layer.nUsers = 1;

  // Open map
  QgsGrass::init();

  try
  {
    layer.mapId = openMap( gisdbase, location, mapset, mapName );
  }
  catch ( QgsGrass::Exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( QString( "Cannot open vector map: %1" ).arg( e.what() ) );
    return -1;
  }

  if ( layer.mapId < 0 )
  {
    QgsDebugMsg( "Cannot open vector map" );
    return -1;
  }
  QgsDebugMsg( QString( "layer.mapId = %1" ).arg( layer.mapId ) );
  layer.map = mMaps[layer.mapId].map;

  layer.attributes = 0; // because loadLayerSourcesFromMap will release old
  loadLayerSourcesFromMap( layer );

  layer.valid = true;

  // Add new layer to layers
  mLayers.push_back( layer );

  QgsDebugMsg( QString( "New layer successfully opened: %1" ).arg( layer.nAttributes ) );

  return mLayers.size() - 1;
}

void QgsGrassProvider::loadLayerSourcesFromMap( GLAYER &layer )
{
  QgsDebugMsg( "entered." );

  // Reset and free
  layer.fields.clear();
  if ( layer.attributes )
  {
    for ( int i = 0; i < layer.nAttributes; i ++ )
    {
      for ( int j = 0; j < layer.nColumns; j ++ )
      {
        if ( layer.attributes[i].values[j] )
          free( layer.attributes[i].values[j] );
      }
      free( layer.attributes[i].values );
    }
    free( layer.attributes );
  }
  loadAttributes( layer );
}

void QgsGrassProvider::loadAttributes( GLAYER &layer )
{
  QgsDebugMsg( "entered." );

  // TODO: free old attributes

  if ( !layer.map )
    return;

  // Attributes are not loaded for topo layers in which case field == 0

  // Get field info
  layer.fieldInfo = Vect_get_field( layer.map, layer.field ); // should work also with field = 0

  // Read attributes
  layer.nColumns = 0;
  layer.nAttributes = 0;
  layer.attributes = 0;
  layer.fields.clear();
  layer.keyColumn = -1;
  if ( !layer.fieldInfo )
  {
    QgsDebugMsg( "No field info -> no attribute table" );
  }
  else
  {
    QgsDebugMsg( "Field info found -> open database" );
    dbDriver *databaseDriver = db_start_driver_open_database( layer.fieldInfo->driver,
                               layer.fieldInfo->database );

    if ( !databaseDriver )
    {
      QgsDebugMsg( QString( "Cannot open database %1 by driver %2" ).arg( layer.fieldInfo->database ).arg( layer.fieldInfo->driver ) );
    }
    else
    {
      QgsDebugMsg( "Database opened -> open select cursor" );
      dbString dbstr;
      db_init_string( &dbstr );
      db_set_string( &dbstr, ( char * )"select * from " );
      db_append_string( &dbstr, layer.fieldInfo->table );

      QgsDebugMsg( QString( "SQL: %1" ).arg( db_get_string( &dbstr ) ) );
      dbCursor databaseCursor;
      if ( db_open_select_cursor( databaseDriver, &dbstr, &databaseCursor, DB_SCROLL ) != DB_OK )
      {
        layer.nColumns = 0;
        db_close_database_shutdown_driver( databaseDriver );
        QMessageBox::warning( 0, "Warning", "Cannot select attributes from table '" +
                              QString( layer.fieldInfo->table ) + "'" );
      }
      else
      {
        int nRecords = db_get_num_rows( &databaseCursor );
        QgsDebugMsg( QString( "Number of records: %1" ).arg( nRecords ) );

        dbTable  *databaseTable = db_get_cursor_table( &databaseCursor );
        layer.nColumns = db_get_table_number_of_columns( databaseTable );

        layer.minmax = new double[layer.nColumns][2];

        // Read columns' description
        for ( int i = 0; i < layer.nColumns; i++ )
        {
          layer.minmax[i][0] = DBL_MAX;
          layer.minmax[i][1] = -DBL_MAX;

          dbColumn *column = db_get_table_column( databaseTable, i );

          int ctype = db_sqltype_to_Ctype( db_get_column_sqltype( column ) );
          QVariant::Type qtype = QVariant::String; //default to string
          QgsDebugMsg( QString( "column = %1 ctype = %2" ).arg( db_get_column_name( column ) ).arg( ctype ) );

          QString ctypeStr;
          switch ( ctype )
          {
            case DB_C_TYPE_INT:
              ctypeStr = "integer";
              qtype = QVariant::Int;
              break;
            case DB_C_TYPE_DOUBLE:
              ctypeStr = "double";
              qtype = QVariant::Double;
              break;
            case DB_C_TYPE_STRING:
              ctypeStr = "string";
              qtype = QVariant::String;
              break;
            case DB_C_TYPE_DATETIME:
              ctypeStr = "datetime";
              qtype = QVariant::String;
              break;
          }
          layer.fields.append( QgsField( db_get_column_name( column ), qtype, ctypeStr,
                                         db_get_column_length( column ), db_get_column_precision( column ) ) );

          if ( G_strcasecmp( db_get_column_name( column ), layer.fieldInfo->key ) == 0 )
          {
            layer.keyColumn = i;
          }
        }

        if ( layer.keyColumn < 0 )
        {
          layer.fields.clear();
          layer.nColumns = 0;

          QMessageBox::warning( 0, "Warning", "Key column '" + QString( layer.fieldInfo->key ) +
                                "' not found in the table '" + QString( layer.fieldInfo->table ) + "'" );
        }
        else
        {
          // Read attributes to the memory
          layer.attributes = ( GATT * ) malloc( nRecords * sizeof( GATT ) );
          while ( 1 )
          {
            int more;

            if ( db_fetch( &databaseCursor, DB_NEXT, &more ) != DB_OK )
            {
              QgsDebugMsg( "Cannot fetch DB record" );
              break;
            }
            if ( !more )
              break; // no more records

            // Check cat value
            dbColumn *column = db_get_table_column( databaseTable, layer.keyColumn );
            dbValue *value = db_get_column_value( column );

            if ( db_test_value_isnull( value ) )
              continue;
            layer.attributes[layer.nAttributes].cat = db_get_value_int( value );
            if ( layer.attributes[layer.nAttributes].cat < 0 )
              continue;

            layer.attributes[layer.nAttributes].values = ( char ** ) malloc( layer.nColumns * sizeof( char* ) );

            for ( int i = 0; i < layer.nColumns; i++ )
            {
              column = db_get_table_column( databaseTable, i );
              int sqltype = db_get_column_sqltype( column );
              int ctype = db_sqltype_to_Ctype( sqltype );
              value = db_get_column_value( column );
              db_convert_value_to_string( value, sqltype, &dbstr );

#if QGISDEBUG > 3
              QgsDebugMsg( QString( "column: %1" ).arg( db_get_column_name( column ) ) );
              QgsDebugMsg( QString( "value: %1" ).arg( db_get_string( &dbstr ) ) );
#endif

              layer.attributes[layer.nAttributes].values[i] = strdup( db_get_string( &dbstr ) );
              if ( !db_test_value_isnull( value ) )
              {
                double dbl;
                if ( ctype == DB_C_TYPE_INT )
                {
                  dbl = db_get_value_int( value );
                }
                else if ( ctype == DB_C_TYPE_DOUBLE )
                {
                  dbl = db_get_value_double( value );
                }
                else
                {
                  dbl = 0;
                }

                if ( dbl < layer.minmax[i][0] )
                {
                  layer.minmax[i][0] = dbl;
                }
                if ( dbl > layer.minmax[i][1] )
                {
                  layer.minmax[i][1] = dbl;
                }
              }
            }
            layer.nAttributes++;
          }
          // Sort attributes by category
          qsort( layer.attributes, layer.nAttributes, sizeof( GATT ), cmpAtt );
        }
        db_close_cursor( &databaseCursor );
        db_close_database_shutdown_driver( databaseDriver );
        db_free_string( &dbstr );

        QgsDebugMsg( QString( "fields.size = %1" ).arg( layer.fields.size() ) );
        QgsDebugMsg( QString( "number of attributes = %1" ).arg( layer.nAttributes ) );

      }
    }
  }

  // Add cat if no attribute fields exist (otherwise qgis crashes)
  if ( layer.nColumns == 0 )
  {
    layer.keyColumn = 0;
    layer.fields.append( QgsField( "cat", QVariant::Int, "integer" ) );
    layer.minmax = new double[1][2];
    layer.minmax[0][0] = 0;
    layer.minmax[0][1] = 0;

    int cidx = Vect_cidx_get_field_index( layer.map, layer.field );
    if ( cidx >= 0 )
    {
      int ncats, cat, type, id;

      ncats = Vect_cidx_get_num_cats_by_index( layer.map, cidx );

      if ( ncats > 0 )
      {
        Vect_cidx_get_cat_by_index( layer.map, cidx, 0, &cat, &type, &id );
        layer.minmax[0][0] = cat;

        Vect_cidx_get_cat_by_index( layer.map, cidx, ncats - 1, &cat, &type, &id );
        layer.minmax[0][1] = cat;
      }
    }
  }

  GMAP *map = &( mMaps[layer.mapId] );

  QFileInfo di( map->gisdbase + "/" + map->location + "/" + map->mapset + "/vector/" + map->mapName + "/dbln" );
  map->lastAttributesModified = di.lastModified();
}

void QgsGrassProvider::closeLayer( int layerId )
{
  if ( layerId < 0 )
    return;

  QgsDebugMsg( QString( "Close layer %1 nUsers = %2" ).arg( layerId ).arg( mLayers[layerId].nUsers ) );

  // TODO: not tested because delete is never used for providers
  mLayers[layerId].nUsers--;

  if ( mLayers[layerId].nUsers == 0 )   // No more users, free sources
  {
    QgsDebugMsg( "No more users -> delete layer" );

    mLayers[layerId].valid = false;

    // Column names/types
    mLayers[layerId].fields.clear();

    // Attributes
    QgsDebugMsg( "Delete attribute values" );
    for ( int i = 0; i < mLayers[layerId].nAttributes; i++ )
    {
      free( mLayers[layerId].attributes[i].values );
    }
    free( mLayers[layerId].attributes );

    delete[] mLayers[layerId].minmax;

    // Field info
    G_free( mLayers[layerId].fieldInfo );

    closeMap( mLayers[layerId].mapId );
  }
}

/* returns mapId or -1 on error */
int QgsGrassProvider::openMap( QString gisdbase, QString location, QString mapset, QString mapName )
{
  QgsDebugMsg( "entered." );

  QString tmpPath = gisdbase + "/" + location + "/" + mapset + "/" + mapName;

  // Check if this map is already opened
  for ( int i = 0; i <  mMaps.size(); i++ )
  {
    if ( mMaps[i].valid && mMaps[i].path == tmpPath )
    {
      // the map is already opened, return map id
      QgsDebugMsg( QString( "The map is already opened with ID = %1" ).arg( i ) );
      mMaps[i].nUsers++;
      return i;
    }
  }

  GMAP map;
  map.valid = false;
  map.frozen = false;
  map.gisdbase = gisdbase;
  map.location = location;
  map.mapset = mapset;
  map.mapName = mapName;
  map.path = tmpPath;
  map.nUsers = 1;
  map.version = 1;
  map.update = 0;
  map.map = ( struct Map_info * ) malloc( sizeof( struct Map_info ) );

  // Set GRASS location
  QgsGrass::setLocation( gisdbase, location );
  QgsDebugMsg( QString( "Setting  gisdbase, location: %1, %2" ).arg( gisdbase ).arg( location ) );

  // Find the vector
  const char *ms = G_find_vector2( mapName.toUtf8().data(), mapset.toUtf8().data() );

  if ( !ms )
  {
    QgsDebugMsg( "Cannot find GRASS vector" );
    return -1;
  }

  // Read the time of vector dir before Vect_open_old, because it may take long time (when the vector
  // could be owerwritten)
  QFileInfo di( gisdbase + "/" + location + "/" + mapset + "/vector/" + mapName );
  map.lastModified = di.lastModified();

  di.setFile( gisdbase + "/" + location + "/" + mapset + "/vector/" + mapName + "/dbln" );
  map.lastAttributesModified = di.lastModified();

  // Do we have topology and cidx (level2)
  int level = -1;
  G_TRY
  {
    //Vect_set_open_level( 2 );
    level = Vect_open_old_head( map.map, mapName.toUtf8().data(), mapset.toUtf8().data() );
    Vect_close( map.map );
  }
  G_CATCH( QgsGrass::Exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( QString( "Cannot open GRASS vector head on level2: %1" ).arg( e.what() ) );
    level = -1;
  }

  if ( level == -1 )
  {
    QgsDebugMsg( "Cannot open GRASS vector head" );
    return -1;
  }
  else if ( level == 1 )
  {
    QMessageBox::StandardButton ret = QMessageBox::question( 0, "Warning",
                                      tr( "GRASS vector map %1 does not have topology. Build topology?" ).arg( mapName ),
                                      QMessageBox::Ok | QMessageBox::Cancel );

    if ( ret == QMessageBox::Cancel )
      return -1;
  }

  // Open vector
  G_TRY
  {
    Vect_set_open_level( level );
    Vect_open_old( map.map, mapName.toUtf8().data(), mapset.toUtf8().data() );
  }
  G_CATCH( QgsGrass::Exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( QString( "Cannot open GRASS vector: %1" ).arg( e.what() ) );
    return -1;
  }

  if ( level == 1 )
  {
    G_TRY
    {
#if defined(GRASS_VERSION_MAJOR) && defined(GRASS_VERSION_MINOR) && \
    ( ( GRASS_VERSION_MAJOR == 6 && GRASS_VERSION_MINOR >= 4 ) || GRASS_VERSION_MAJOR > 6 )
      Vect_build( map.map );
#else
      Vect_build( map.map, stderr );
#endif
    }
    G_CATCH( QgsGrass::Exception &e )
    {
      Q_UNUSED( e );
      QgsDebugMsg( QString( "Cannot build topology: %1" ).arg( e.what() ) );
      return -1;
    }
  }

  QgsDebugMsg( "GRASS map successfully opened" );

  map.valid = true;

  // Add new map to maps
  mMaps.push_back( map );

  return mMaps.size() - 1; // map id
}

void QgsGrassProvider::updateMap( int mapId )
{
  QgsDebugMsg( QString( "mapId = %1" ).arg( mapId ) );

  /* Close map */
  GMAP *map = &( mMaps[mapId] );

  bool closeMap = map->valid;
  map->valid = false;
  map->version++;

  QgsGrass::setLocation( map->gisdbase, map->location );

  // TODO: Should be done better / in other place ?
  // TODO: Is it necessary for close ?
  G__setenv(( char * )"MAPSET", map->mapset.toUtf8().data() );

  if ( closeMap )
    Vect_close( map->map );

  QFileInfo di( map->gisdbase + "/" + map->location + "/" + map->mapset + "/vector/" + map->mapName );
  map->lastModified = di.lastModified();

  di.setFile( map->gisdbase + "/" + map->location + "/" + map->mapset + "/vector/" + map->mapName + "/dbln" );
  map->lastAttributesModified = di.lastModified();

  // Reopen vector
  G_TRY
  {
    Vect_set_open_level( 2 );
    Vect_open_old( map->map, map->mapName.toUtf8().data(), map->mapset.toUtf8().data() );
  }
  G_CATCH( QgsGrass::Exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( QString( "Cannot reopen GRASS vector: %1" ).arg( e.what() ) );

    // if reopen fails, mLayers should be also updated
    for ( int i = 0; i <  mLayers.size(); i++ )
    {
      if ( mLayers[i].mapId == mapId )
      {
        closeLayer( i );
      }
    }
    return;
  }

  QgsDebugMsg( "GRASS map successfully reopened for reading." );

  for ( int i = 0; i <  mLayers.size(); i++ )
  {
    // if ( !(mLayers[i].valid) )
    //   continue; // ?

    if ( mLayers[i].mapId == mapId )
    {
      loadLayerSourcesFromMap( mLayers[i] );
    }
  }

  map->valid = true;
}

void QgsGrassProvider::closeMap( int mapId )
{
  GMAP *map = &mMaps[mapId];
  QgsDebugMsg( QString( "Close map %1 nUsers = %2" ).arg( mapId ).arg( map->nUsers ) );

  // TODO: not tested because delete is never used for providers
  map->nUsers--;

  if ( map->nUsers == 0 )   // No more users, free sources
  {
    QgsDebugMsg( "No more users -> delete map" );

    // TODO: do this better, probably maintain QgsGrassEdit as one user
    if ( map->update )
    {
      QMessageBox::warning( 0, "Warning", "The vector was currently edited, "
                            "you can expect crash soon." );
    }

    if ( map->valid )
    {
      bool mapsetunset = !G__getenv( "MAPSET" ) || !*G__getenv( "MAPSET" );
      if ( mapsetunset )
        G__setenv(( char * )"MAPSET", map->mapset.toUtf8().data() );

      if ( !map->frozen )
      {
        try
        {
          Vect_close( map->map );
        }
        catch ( QgsGrass::Exception &e )
        {
          Q_UNUSED( e );
          QgsDebugMsg( QString( "Vect_close failed: %1" ).arg( e.what() ) );
        }
      }

      if ( mapsetunset )
        G__setenv(( char * )"MAPSET", "" );
    }
    map->valid = false;
  }
}

bool QgsGrassProvider::mapOutdated( int mapId )
{
  QgsDebugMsg( "entered." );

  GMAP *map = &( mMaps[mapId] );

  QString dp = map->gisdbase + "/" + map->location + "/" + map->mapset + "/vector/" + map->mapName;
  QFileInfo di( dp );

  if ( map->lastModified < di.lastModified() )
  {
    // If the cidx file has been deleted, the map is currently being modified
    // by an external tool. Do not update until the cidx file has been recreated.
    if ( !QFileInfo( dp, "cidx" ).exists() )
    {
      QgsDebugMsg( QString( "**** The map %1 is being modified and is unavailable ****" ).arg( mapId ) );
      return false;
    }
    QgsDebugMsg( QString( "**** The map %1 was modified ****" ).arg( mapId ) );

    return true;
  }

  return false;
}

bool QgsGrassProvider::attributesOutdated( int mapId )
{
  QgsDebugMsg( "entered." );

  GMAP *map = &( mMaps[mapId] );

  QString dp = map->gisdbase + "/" + map->location + "/" + map->mapset + "/vector/" + map->mapName + "/dbln";
  QFileInfo di( dp );

  if ( map->lastAttributesModified < di.lastModified() )
  {
    QgsDebugMsg( QString( "**** The attributes of the map %1 were modified ****" ).arg( mapId ) );

    return true;
  }

  return false;
}


void QgsGrassProvider::ensureUpdated()
{
  int mapId = mLayers[mLayerId].mapId;
  if ( mapOutdated( mapId ) )
  {
    updateMap( mapId );
  }
  if ( mMapVersion < mMaps[mapId].version )
  {
    update();
  }
  if ( attributesOutdated( mapId ) )
  {
    loadAttributes( mLayers[mLayerId] );
  }
}


/** Get pointer to map */
struct Map_info *QgsGrassProvider::layerMap( int layerId )
{
  return ( mMaps[mLayers[layerId].mapId].map );
}


QgsCoordinateReferenceSystem QgsGrassProvider::crs()
{
  return QgsGrass::crs( mGisdbase, mLocation );
}

int QgsGrassProvider::grassLayer()
{
  return mLayerField;
}

int QgsGrassProvider::grassLayer( QString name )
{
  // Get field number
  int pos = name.indexOf( '_' );

  if ( pos == -1 )
  {
    return -1;
  }

  return name.left( pos ).toInt();
}

int QgsGrassProvider::grassLayerType( QString name )
{
  int pos = name.indexOf( '_' );

  if ( pos == -1 )
  {
    return -1;
  }

  QString ts = name.right( name.length() - pos - 1 );
  if ( ts.compare( "point" ) == 0 )
  {
    return GV_POINT; // ?! centroids may be points
  }
  else if ( ts.compare( "line" ) == 0 )
  {
    return GV_LINES;
  }
  else if ( ts.compare( "face" ) == 0 )
  {
    return GV_FACE;
  }
  else if ( ts.compare( "polygon" ) == 0 )
  {
    return GV_AREA;
  }

  return -1;
}

//-----------------------------------------  Edit -------------------------------------------------------

bool QgsGrassProvider::isGrassEditable( void )
{
  QgsDebugMsg( "entered." );

  if ( !isValid() )
    return false;

  /* Check if current user is owner of mapset */
  if ( G__mapset_permissions2( mGisdbase.toUtf8().data(), mLocation.toUtf8().data(), mMapset.toUtf8().data() ) != 1 )
    return false;

  // TODO: check format? (cannot edit OGR layers)

  return true;
}

bool QgsGrassProvider::isEdited( void )
{
  QgsDebugMsgLevel( "entered.", 3 );

  GMAP *map = &( mMaps[mLayers[mLayerId].mapId] );
  return ( map->update );
}

bool QgsGrassProvider::isFrozen( void )
{
  QgsDebugMsgLevel( "entered.", 3 );

  GMAP *map = &( mMaps[mLayers[mLayerId].mapId] );
  return ( map->frozen );
}

void QgsGrassProvider::freeze()
{
  QgsDebugMsg( "entered." );

  if ( !isValid() )
    return;

  GMAP *map = &( mMaps[mLayers[mLayerId].mapId] );

  if ( map->frozen )
    return;

  map->frozen = true;
  Vect_close( map->map );
}

void QgsGrassProvider::thaw()
{
  QgsDebugMsg( "entered." );

  if ( !isValid() )
    return;
  GMAP *map = &( mMaps[mLayers[mLayerId].mapId] );

  if ( !map->frozen )
    return;

  if ( reopenMap() )
  {
    map->frozen = false;
  }
}

bool QgsGrassProvider::startEdit( void )
{
  QgsDebugMsg( "entered." );
  QgsDebugMsg( QString( "  uri = %1" ).arg( dataSourceUri() ) );
  QgsDebugMsg( QString( "  mMaps.size() = %1" ).arg( mMaps.size() ) );

  if ( !isGrassEditable() )
    return false;

  // Check number of maps (the problem may appear if static variables are not shared - runtime linker)
  if ( mMaps.size() == 0 )
  {
    QMessageBox::warning( 0, "Warning", "No maps opened in mMaps, probably problem in runtime linking, "
                          "static variables are not shared by provider and plugin." );
    return false;
  }

  /* Close map */
  GMAP *map = &( mMaps[mLayers[mLayerId].mapId] );
  map->valid = false;

  QgsGrass::setLocation( map->gisdbase, map->location );

  // Set current mapset (mapset was previously checked by isGrassEditable() )
  // TODO: Should be done better / in other place ?
  G__setenv(( char * )"MAPSET", map->mapset.toUtf8().data() );

  Vect_close( map->map );

  // TODO: Catch error
  int level = -1;
  try
  {
    level = Vect_open_update( map->map, map->mapName.toUtf8().data(), map->mapset.toUtf8().data() );
    if ( level < 2 )
    {
      QgsDebugMsg( "Cannot open GRASS vector for update on level 2." );
    }
  }
  catch ( QgsGrass::Exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( QString( "Cannot open GRASS vector for update: %1" ).arg( e.what() ) );
  }

  if ( level < 2 )
  {
    // reopen vector for reading
    try
    {
      Vect_set_open_level( 2 );
      level = Vect_open_old( map->map, map->mapName.toUtf8().data(), map->mapset.toUtf8().data() );
      if ( level < 2 )
      {
        QgsDebugMsg( QString( "Cannot reopen GRASS vector: %1" ).arg( QgsGrass::errorMessage() ) );
      }
    }
    catch ( QgsGrass::Exception &e )
    {
      Q_UNUSED( e );
      QgsDebugMsg( QString( "Cannot reopen GRASS vector: %1" ).arg( e.what() ) );
    }

    if ( level >= 2 )
      map->valid = true;

    return false;
  }
  Vect_set_category_index_update( map->map );

  // Write history
  Vect_hist_command( map->map );

  QgsDebugMsg( "Vector successfully reopened for update." );

  map->update = true;
  map->valid = true;

  return true;
}

bool QgsGrassProvider::closeEdit( bool newMap )
{
  QgsDebugMsg( "entered." );

  if ( !isValid() )
    return false;

  /* Close map */
  GMAP *map = &( mMaps[mLayers[mLayerId].mapId] );

  if ( !( map->update ) )
    return false;

  map->valid = false;
  map->version++;

  QgsGrass::setLocation( map->gisdbase, map->location );

  // Set current mapset (mapset was previously checked by isGrassEditable() )
  // TODO: Should be done better / in other place ?
  // TODO: Is it necessary for build/close ?
  G__setenv(( char * ) "MAPSET", map->mapset.toUtf8().data() );

#if defined(GRASS_VERSION_MAJOR) && defined(GRASS_VERSION_MINOR) && \
    ( ( GRASS_VERSION_MAJOR == 6 && GRASS_VERSION_MINOR >= 4 ) || GRASS_VERSION_MAJOR > 6 )
  Vect_build_partial( map->map, GV_BUILD_NONE );
  Vect_build( map->map );
#else
  Vect_build_partial( map->map, GV_BUILD_NONE, NULL );
  Vect_build( map->map, stderr );
#endif

  // If a new map was created close the map and return
  if ( newMap )
  {
    QgsDebugMsg( QString( "mLayers.size() = %1" ).arg( mLayers.size() ) );
    map->update = false;
    // Map must be set as valid otherwise it is not closed and topo is not written
    map->valid = true;
    closeLayer( mLayerId );
    return true;
  }

  Vect_close( map->map );

  map->update = false;

  if ( !reopenMap() )
    return false;

  map->valid = true;

  return true;
}

bool QgsGrassProvider::reopenMap()
{
  GMAP *map = &( mMaps[mLayers[mLayerId].mapId] );

  QFileInfo di( mGisdbase + "/" + mLocation + "/" + mMapset + "/vector/" + mMapName );
  map->lastModified = di.lastModified();

  di.setFile( mGisdbase + "/" + mLocation + "/" + mMapset + "/vector/" + mMapset + "/dbln" );
  map->lastAttributesModified = di.lastModified();

  // Reopen vector
  try
  {
    Vect_set_open_level( 2 );
    Vect_open_old( map->map, map->mapName.toUtf8().data(), map->mapset.toUtf8().data() );
  }
  catch ( QgsGrass::Exception &e )
  {
    Q_UNUSED( e );
    QgsDebugMsg( QString( "Cannot reopen GRASS vector: %1" ).arg( e.what() ) );
    return false;
  }

  QgsDebugMsg( "GRASS map successfully reopened for reading." );

  // Reload sources to layers
  for ( int i = 0; i <  mLayers.size(); i++ )
  {
    // if ( !(mLayers[i].valid) )
    //   continue; // ?

    if ( mLayers[i].mapId == mLayers[mLayerId].mapId )
    {
      loadLayerSourcesFromMap( mLayers[i] );
    }
  }

  return true;
}

int QgsGrassProvider::numLines( void )
{
  QgsDebugMsg( "entered." );

  return ( Vect_get_num_lines( mMap ) );
}

int QgsGrassProvider::numNodes( void )
{
  QgsDebugMsg( "entered." );

  return ( Vect_get_num_nodes( mMap ) );
}

int QgsGrassProvider::readLine( struct line_pnts *Points, struct line_cats *Cats, int line )
{
  QgsDebugMsgLevel( "entered.", 3 );

  if ( Points )
    Vect_reset_line( Points );

  if ( Cats )
    Vect_reset_cats( Cats );

  if ( !Vect_line_alive( mMap, line ) )
    return -1;

  return ( Vect_read_line( mMap, Points, Cats, line ) );
}

bool QgsGrassProvider::nodeCoor( int node, double *x, double *y )
{
  QgsDebugMsgLevel( "entered.", 3 );

  if ( !Vect_node_alive( mMap, node ) )
  {
    *x = 0.0;
    *y = 0.0;
    return false;
  }

  Vect_get_node_coor( mMap, node, x, y, NULL );
  return true;
}

bool QgsGrassProvider::lineNodes( int line, int *node1, int *node2 )
{
  QgsDebugMsgLevel( "entered.", 3 );

  if ( !Vect_line_alive( mMap, line ) )
  {
    *node1 = 0;
    *node2 = 0;
    return false;
  }

#if GRASS_VERSION_MAJOR < 7
  Vect_get_line_nodes( mMap, line, node1, node2 );
#else
  /* points don't have topology in GRASS >= 7 */
  *node1 = 0;
  *node2 = 0;
#endif
  return true;
}

int QgsGrassProvider::writeLine( int type, struct line_pnts *Points, struct line_cats *Cats )
{
  QgsDebugMsg( QString( "n_points = %1 n_cats = %2" ).arg( Points->n_points ).arg( Cats->n_cats ) );

  if ( !isEdited() )
    return -1;

  return (( int ) Vect_write_line( mMap, type, Points, Cats ) );
}

int QgsGrassProvider::rewriteLine( int line, int type, struct line_pnts *Points, struct line_cats *Cats )
{
  QgsDebugMsg( QString( "n_points = %1 n_cats = %2" ).arg( Points->n_points ).arg( Cats->n_cats ) );

  if ( !isEdited() )
    return -1;

  return ( Vect_rewrite_line( mMap, line, type, Points, Cats ) );
}


int QgsGrassProvider::deleteLine( int line )
{
  QgsDebugMsg( "entered." );

  if ( !isEdited() )
    return -1;

  return ( Vect_delete_line( mMap, line ) );
}

int QgsGrassProvider::findLine( double x, double y, int type, double threshold )
{
  QgsDebugMsgLevel( "entered", 3 );

  return ( Vect_find_line( mMap, x, y, 0, type, threshold, 0, 0 ) );
}

int QgsGrassProvider::findNode( double x, double y, double threshold )
{
  return ( Vect_find_node( mMap, x, y, 0, threshold, 0 ) );
}

bool QgsGrassProvider::lineAreas( int line, int *left, int *right )
{
  QgsDebugMsgLevel( "entered.", 3 );

  if ( !Vect_line_alive( mMap, line ) )
  {
    *left = 0;
    *right = 0;
    return false;
  }

  Vect_get_line_areas( mMap, line, left, right );
  return true;
}

int QgsGrassProvider::isleArea( int isle )
{
  QgsDebugMsgLevel( "entered.", 3 );

  if ( !Vect_isle_alive( mMap, isle ) )
  {
    return 0;
  }

  return ( Vect_get_isle_area( mMap, isle ) );
}

int QgsGrassProvider::centroidArea( int centroid )
{
  QgsDebugMsgLevel( "entered.", 3 );

  if ( !Vect_line_alive( mMap, centroid ) )
  {
    return 0;
  }

  return ( Vect_get_centroid_area( mMap, centroid ) );
}

int QgsGrassProvider::nodeNLines( int node )
{
  QgsDebugMsgLevel( "entered.", 3 );

  if ( !Vect_node_alive( mMap, node ) )
  {
    return 0;
  }

  return ( Vect_get_node_n_lines( mMap, node ) );
}

int QgsGrassProvider::nodeLine( int node, int idx )
{
  QgsDebugMsgLevel( "entered.", 3 );

  if ( !Vect_node_alive( mMap, node ) )
  {
    return 0;
  }

  return ( Vect_get_node_line( mMap, node, idx ) );
}

int QgsGrassProvider::lineAlive( int line )
{
  QgsDebugMsgLevel( "entered.", 3 );

  return ( Vect_line_alive( mMap, line ) );
}

int QgsGrassProvider::nodeAlive( int node )
{
  QgsDebugMsgLevel( "QgsGrassProvider::nodeAlive", 3 );

  return ( Vect_node_alive( mMap, node ) );
}

int QgsGrassProvider::numUpdatedLines( void )
{
  QgsDebugMsg( QString( "numUpdatedLines = %1" ).arg( Vect_get_num_updated_lines( mMap ) ) );

  return ( Vect_get_num_updated_lines( mMap ) );
}

int QgsGrassProvider::numUpdatedNodes( void )
{
  QgsDebugMsg( QString( "numUpdatedNodes = %1" ).arg( Vect_get_num_updated_nodes( mMap ) ) );

  return ( Vect_get_num_updated_nodes( mMap ) );
}

int QgsGrassProvider::updatedLine( int idx )
{
  QgsDebugMsg( QString( "idx = %1" ).arg( idx ) );
  QgsDebugMsg( QString( "  updatedLine = %1" ).arg( Vect_get_updated_line( mMap, idx ) ) );

  return ( Vect_get_updated_line( mMap, idx ) );
}

int QgsGrassProvider::updatedNode( int idx )
{
  QgsDebugMsg( QString( "idx = %1" ).arg( idx ) );
  QgsDebugMsg( QString( "  updatedNode = %1" ).arg( Vect_get_updated_node( mMap, idx ) ) );

  return ( Vect_get_updated_node( mMap, idx ) );
}

// ------------------ Attributes -------------------------------------------------

QString QgsGrassProvider::key( int field )
{
  QgsDebugMsg( QString( "field = %1" ).arg( field ) );

  struct field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0
  if ( !fi )
  {
    QgsDebugMsg( "No field info -> no attributes" );
    return QString();
  }

  return QString::fromUtf8( fi->key );
}

QVector<QgsField> *QgsGrassProvider::columns( int field )
{
  QgsDebugMsg( QString( "field = %1" ).arg( field ) );

  QVector<QgsField> *col = new QVector<QgsField>;

  struct  field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0

  // Read attributes
  if ( !fi )
  {
    QgsDebugMsg( "No field info -> no attributes" );
    return col;
  }

  QgsDebugMsg( "Field info found -> open database" );
  QgsGrass::setMapset( mGisdbase, mLocation, mMapset );
  dbDriver *driver = db_start_driver_open_database( fi->driver, fi->database );

  if ( !driver )
  {
    QgsDebugMsg( QString( "Cannot open database %1 by driver %2" ).arg( fi->database ).arg( fi->driver ) );
    return col;
  }

  QgsDebugMsg( "Database opened -> describe table" );

  dbString tableName;
  db_init_string( &tableName );
  db_set_string( &tableName, fi->table );

  dbTable *table;
  if ( db_describe_table( driver, &tableName, &table ) != DB_OK )
  {
    QgsDebugMsg( "Cannot describe table" );
    return col;
  }

  int nCols = db_get_table_number_of_columns( table );

  for ( int c = 0; c < nCols; c++ )
  {
    dbColumn *column = db_get_table_column( table, c );

    int ctype = db_sqltype_to_Ctype( db_get_column_sqltype( column ) );
    QString type;
    QVariant::Type qtype = QVariant::String; //default to string to prevent compiler warnings
    switch ( ctype )
    {
      case DB_C_TYPE_INT:
        type = "int";
        qtype = QVariant::Int;
        break;
      case DB_C_TYPE_DOUBLE:
        type = "double";
        qtype = QVariant::Double;
        break;
      case DB_C_TYPE_STRING:
        type = "string";
        qtype = QVariant::String;
        break;
      case DB_C_TYPE_DATETIME:
        type = "datetime";
        qtype = QVariant::String;
        break;
    }
    col->push_back( QgsField( db_get_column_name( column ), qtype, type, db_get_column_length( column ), 0 ) );
  }

  db_close_database_shutdown_driver( driver );

  return col;
}

QgsAttributeMap *QgsGrassProvider::attributes( int field, int cat )
{
  QgsDebugMsg( QString( "field = %1 cat = %2" ).arg( field ).arg( cat ) );

  QgsAttributeMap *att = new QgsAttributeMap;

  struct  field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0

  // Read attributes
  if ( !fi )
  {
    QgsDebugMsg( "No field info -> no attributes" );
    return att;
  }

  QgsDebugMsg( "Field info found -> open database" );
  QgsGrass::setMapset( mGisdbase, mLocation, mMapset );
  dbDriver *driver = db_start_driver_open_database( fi->driver, fi->database );

  if ( !driver )
  {
    QgsDebugMsg( QString( "Cannot open database %1 by driver %2" ).arg( fi->database ).arg( fi->driver ) );
    return att;
  }

  QgsDebugMsg( "Database opened -> read attributes" );

  dbString dbstr;
  db_init_string( &dbstr );
  QString query;
  query.sprintf( "select * from %s where %s = %d", fi->table, fi->key, cat );
  db_set_string( &dbstr, query.toUtf8().data() );

  QgsDebugMsg( QString( "SQL: %1" ).arg( db_get_string( &dbstr ) ) );

  dbCursor databaseCursor;
  if ( db_open_select_cursor( driver, &dbstr, &databaseCursor, DB_SCROLL ) != DB_OK )
  {
    db_close_database_shutdown_driver( driver );
    QgsDebugMsg( "Cannot select attributes from table" );
    return att;
  }

  int nRecords = db_get_num_rows( &databaseCursor );
  QgsDebugMsg( QString( "Number of records: %1" ).arg( nRecords ) );

  if ( nRecords < 1 )
  {
    db_close_database_shutdown_driver( driver );
    QgsDebugMsg( "No DB record" );
    return att;
  }

  dbTable  *databaseTable = db_get_cursor_table( &databaseCursor );
  int nColumns = db_get_table_number_of_columns( databaseTable );

  int more;
  if ( db_fetch( &databaseCursor, DB_NEXT, &more ) != DB_OK )
  {
    db_close_database_shutdown_driver( driver );
    QgsDebugMsg( "Cannot fetch DB record" );
    return att;
  }

  // Read columns' description
  for ( int i = 0; i < nColumns; i++ )
  {
    dbColumn *column = db_get_table_column( databaseTable, i );
    db_convert_column_value_to_string( column, &dbstr );

    QString v = mEncoding->toUnicode( db_get_string( &dbstr ) );
    QgsDebugMsg( QString( "Value: %1" ).arg( v ) );
    att->insert( i, QVariant( v ) );
  }

  db_close_cursor( &databaseCursor );
  db_close_database_shutdown_driver( driver );
  db_free_string( &dbstr );

  return att;
}

QString QgsGrassProvider::updateAttributes( int field, int cat, const QString &values )
{
  QgsDebugMsg( QString( "field = %1 cat = %2" ).arg( field ).arg( cat ) );

  // Read attributes
  struct field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0
  if ( !fi )
  {
    QgsDebugMsg( "No field info -> no attributes" );
    return "Cannot get field info";
  }

  QgsDebugMsg( "Field info found -> open database" );
  QgsGrass::setMapset( mGisdbase, mLocation, mMapset );

  dbDriver *driver = db_start_driver_open_database( fi->driver, fi->database );
  if ( !driver )
  {
    QgsDebugMsg( QString( "Cannot open database %1 by driver %2" ).arg( fi->database ).arg( fi->driver ) );
    return "Cannot open database";
  }

  QgsDebugMsg( "Database opened -> read attributes" );

  dbString dbstr;
  db_init_string( &dbstr );
  QString query;

  query = "update " + QString( fi->table ) + " set " + values + " where " + QString( fi->key )
          + " = " + QString::number( cat );

  QgsDebugMsg( QString( "query: %1" ).arg( query ) );

  // For some strange reason, mEncoding->fromUnicode(query) does not work,
  // but probably it is not correct, because Qt widgets will use current locales for input
  //  -> it is possible to edit only in current locales at present
  // QCString qcs = mEncoding->fromUnicode(query);

  QByteArray qcs = query.toUtf8();
  QgsDebugMsg( QString( "qcs: %1" ).arg( qcs.data() ) );

  char *cs = new char[qcs.length() + 1];
  strcpy( cs, ( const char * )qcs );
  db_set_string( &dbstr, cs );
  delete[] cs;

  QgsDebugMsg( QString( "SQL: %1" ).arg( db_get_string( &dbstr ) ) );

  QString error;
  int ret = db_execute_immediate( driver, &dbstr );
  if ( ret != DB_OK )
  {
    QgsDebugMsg( QString( "Error: %1" ).arg( db_get_error_msg() ) );
    error = QString::fromLatin1( db_get_error_msg() );
  }

  db_close_database_shutdown_driver( driver );
  db_free_string( &dbstr );

  return error;
}

int QgsGrassProvider::numDbLinks( void )
{
  QgsDebugMsg( "entered." );

  return ( Vect_get_num_dblinks( mMap ) );
}

int QgsGrassProvider::dbLinkField( int link )
{
  QgsDebugMsg( "entered." );

  struct  field_info *fi = Vect_get_dblink( mMap, link );

  if ( !fi )
    return 0;

  return ( fi->number );
}

QString QgsGrassProvider::executeSql( int field, const QString &sql )
{
  QgsDebugMsg( QString( "field = %1" ).arg( field ) );

  // Read attributes
  struct  field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0
  if ( !fi )
  {
    QgsDebugMsg( "No field info -> no attributes" );
    return "Cannot get field info";
  }

  QgsDebugMsg( "Field info found -> open database" );

  QgsGrass::setMapset( mGisdbase, mLocation, mMapset );
  dbDriver *driver = db_start_driver_open_database( fi->driver, fi->database );

  if ( !driver )
  {
    QgsDebugMsg( QString( "Cannot open database %1 by driver %2" ).arg( fi->database ).arg( fi->driver ) );
    return "Cannot open database";
  }

  QgsDebugMsg( "Database opened" );

  dbString dbstr;
  db_init_string( &dbstr );
  db_set_string( &dbstr, sql.toLatin1().data() );

  QgsDebugMsg( QString( "SQL: %1" ).arg( db_get_string( &dbstr ) ) );

  QString error;

  int ret = db_execute_immediate( driver, &dbstr );
  if ( ret != DB_OK )
  {
    QgsDebugMsg( QString( "Error: %1" ).arg( db_get_error_msg() ) );
    error = QString::fromLatin1( db_get_error_msg() );
  }

  db_close_database_shutdown_driver( driver );
  db_free_string( &dbstr );

  return error;

}

QString QgsGrassProvider::createTable( int field, const QString &key, const QString &columns )
{
  QgsDebugMsg( QString( "field = %1" ).arg( field ) );

  // Read attributes
  struct field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0
  if ( fi != NULL )
  {
    QgsDebugMsg( "The table for this field already exists" );
    return "The table for this field already exists";
  }

  QgsDebugMsg( "Field info not found -> create new table" );

  // We must set mapset before Vect_default_field_info
  QgsGrass::setMapset( mGisdbase, mLocation, mMapset );

  int nLinks = Vect_get_num_dblinks( mMap );
  if ( nLinks == 0 )
  {
    fi = Vect_default_field_info( mMap, field, NULL, GV_1TABLE );
  }
  else
  {
    fi = Vect_default_field_info( mMap, field, NULL, GV_MTABLE );
  }

  dbDriver *driver = db_start_driver_open_database( fi->driver, fi->database );
  if ( !driver )
  {
    QgsDebugMsg( QString( "Cannot open database %1 by driver %2" ).arg( fi->database ).arg( fi->driver ) );
    return "Cannot open database";
  }

  QgsDebugMsg( "Database opened -> create table" );

  dbString dbstr;
  db_init_string( &dbstr );
  QString query;

  query.sprintf( "create table %s ( %s )", fi->table, columns.toLatin1().constData() );
  db_set_string( &dbstr, query.toLatin1().data() );

  QgsDebugMsg( QString( "SQL: %1" ).arg( db_get_string( &dbstr ) ) );

  QString error;
  int ret = db_execute_immediate( driver, &dbstr );
  if ( ret != DB_OK )
  {
    QgsDebugMsg( QString( "Error: %1" ).arg( db_get_error_msg() ) );
    error = QString::fromLatin1( db_get_error_msg() );
  }

  db_close_database_shutdown_driver( driver );
  db_free_string( &dbstr );

  if ( !error.isEmpty() )
    return error;

  ret = Vect_map_add_dblink( mMap, field, NULL, fi->table, key.toLatin1().data(),
                             fi->database, fi->driver );

  if ( ret == -1 )
  {
    QgsDebugMsg( "Error: Cannot add dblink" );
    error = QString::fromLatin1( "Cannot create link to the table. The table was created!" );
  }

  return error;
}

QString QgsGrassProvider::addColumn( int field, const QString &column )
{
  QgsDebugMsg( QString( "field = %1" ).arg( field ) );

  // Read attributes
  struct field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0
  if ( !fi )
  {
    QgsDebugMsg( "No field info" );
    return "Cannot get field info";
  }

  QString query;
  query.sprintf( "alter table %s add column %s", fi->table, column.toLatin1().constData() );
  return executeSql( field, query );
}

QString QgsGrassProvider::insertAttributes( int field, int cat )
{
  QgsDebugMsg( QString( "field = %1 cat = %2" ).arg( field ).arg( cat ) );

  // Read attributes
  struct field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0
  if ( !fi )
  {
    QgsDebugMsg( "No field info -> no attributes" );
    return "Cannot get field info";
  }

  QString query;
  query.sprintf( "insert into %s ( %s ) values ( %d )", fi->table, fi->key, cat );
  return executeSql( field, query );
}

QString QgsGrassProvider::deleteAttribute( int field, int cat )
{
  QgsDebugMsg( QString( "field = %1 cat = %2" ).arg( field ).arg( cat ) );

  // Read attributes
  struct field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0
  if ( !fi )
  {
    QgsDebugMsg( "No field info -> no attributes" );
    return "Cannot get field info";
  }

  QString query;
  query.sprintf( "delete from %s where %s = %d", fi->table, fi->key, cat );
  return executeSql( field, query );
}

QString QgsGrassProvider::isOrphan( int field, int cat, int &orphan )
{
  QgsDebugMsg( QString( "field = %1 cat = %2" ).arg( field ).arg( cat ) );

  // Check first if another line with such cat exists
  int fieldIndex = Vect_cidx_get_field_index( mMap, field );
  if ( fieldIndex >= 0 )
  {
    int t, id;
    int ret = Vect_cidx_find_next( mMap, fieldIndex, cat,
                                   GV_POINTS | GV_LINES | GV_FACE, 0, &t, &id );

    if ( ret >= 0 )
    {
      // Category exists
      orphan = false;
      return QString();
    }
  }

  // Check if attribute exists
  // Read attributes
  struct field_info *fi = Vect_get_field( mMap, field ); // should work also with field = 0
  if ( !fi )
  {
    QgsDebugMsg( "No field info -> no attributes" );
    orphan = false;
    return QString();
  }

  QgsDebugMsg( "Field info found -> open database" );
  QgsGrass::setMapset( mGisdbase, mLocation, mMapset );
  dbDriver *driver = db_start_driver_open_database( fi->driver, fi->database );
  if ( !driver )
  {
    QgsDebugMsg( QString( "Cannot open database %1 by driver %2" ).arg( fi->database ).arg( fi->driver ) );
    return "Cannot open database";
  }

  QgsDebugMsg( "Database opened -> select record" );

  dbString dbstr;
  db_init_string( &dbstr );

  QString query;
  query.sprintf( "select %s from %s where %s = %d", fi->key, fi->table, fi->key, cat );
  db_set_string( &dbstr, query.toLatin1().data() );

  QgsDebugMsg( QString( "SQL: %1" ).arg( db_get_string( &dbstr ) ) );

  dbCursor cursor;
  if ( db_open_select_cursor( driver, &dbstr, &cursor, DB_SCROLL ) != DB_OK )
  {
    db_close_database_shutdown_driver( driver );
    return QString( "Cannot query database: %1" ).arg( query );
  }
  int nRecords = db_get_num_rows( &cursor );
  QgsDebugMsg( QString( "Number of records: %1" ).arg( nRecords ) );

  if ( nRecords > 0 )
    orphan = true;

  db_close_database_shutdown_driver( driver );
  db_free_string( &dbstr );

  return QString();
}

bool QgsGrassProvider::isTopoType() const
{
  return isTopoType( mLayerType );
}

bool QgsGrassProvider::isTopoType( int layerType )
{
  return layerType == TOPO_POINT || layerType == TOPO_LINE || layerType == TOPO_NODE;
}

void QgsGrassProvider::setTopoFields()
{
  mTopoFields.append( QgsField( "id", QVariant::Int ) );

  if ( mLayerType == TOPO_POINT )
  {
    mTopoFields.append( QgsField( "type", QVariant::String ) );
    mTopoFields.append( QgsField( "node", QVariant::Int ) );
  }
  else if ( mLayerType == TOPO_LINE )
  {
    mTopoFields.append( QgsField( "type", QVariant::String ) );
    mTopoFields.append( QgsField( "node1", QVariant::Int ) );
    mTopoFields.append( QgsField( "node2", QVariant::Int ) );
    mTopoFields.append( QgsField( "left", QVariant::Int ) );
    mTopoFields.append( QgsField( "right", QVariant::Int ) );
  }
  else if ( mLayerType == TOPO_NODE )
  {
    mTopoFields.append( QgsField( "lines", QVariant::String ) );
  }
}

QString QgsGrassProvider::primitiveTypeName( int type )
{
  switch ( type )
  {
    case GV_POINT: return "point";
    case GV_CENTROID: return "centroid";
    case GV_LINE: return "line";
    case GV_BOUNDARY: return "boundary";
    case GV_FACE: return "face";
    case GV_KERNEL: return "kernel";

  }
  return "unknown";
}

// -------------------------------------------------------------------------------

int QgsGrassProvider::cidxGetNumFields()
{
  return ( Vect_cidx_get_num_fields( mMap ) );
}

int QgsGrassProvider::cidxGetFieldNumber( int idx )
{
  return ( Vect_cidx_get_field_number( mMap, idx ) );
}

int QgsGrassProvider::cidxGetMaxCat( int idx )
{
  int ncats = Vect_cidx_get_num_cats_by_index( mMap, idx );

  int cat, type, id;
  Vect_cidx_get_cat_by_index( mMap, idx, ncats - 1, &cat, &type, &id );

  return ( cat );
}

QString QgsGrassProvider::name() const
{
  return GRASS_KEY;
} // QgsGrassProvider::name()

QString QgsGrassProvider::description() const
{
  return GRASS_DESCRIPTION;
} // QgsGrassProvider::description()

