/***************************************************************************
    offline_editing.cpp

    Offline Editing Plugin
    a QGIS plugin
     --------------------------------------
    Date                 : 22-Jul-2010
    Copyright            : (C) 2010 by Sourcepole
    Email                : info at sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "offline_editing.h"
#include "offline_editing_progress_dialog.h"

#include <qgsapplication.h>
#include <qgsdatasourceuri.h>
#include <qgsgeometry.h>
#include <qgslegendinterface.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerregistry.h>
#include <qgsproject.h>
#include <qgsvectordataprovider.h>

#include <QDir>
#include <QDomDocument>
#include <QDomNode>
#include <QFile>
#include <QMessageBox>

extern "C"
{
#include <sqlite3.h>
#include <spatialite.h>
}

// TODO: DEBUG
#include <QDebug>
// END

#define CUSTOM_PROPERTY_IS_OFFLINE_EDITABLE "isOfflineEditable"
#define CUSTOM_PROPERTY_REMOTE_SOURCE "remoteSource"
#define CUSTOM_PROPERTY_REMOTE_PROVIDER "remoteProvider"
#define PROJECT_ENTRY_SCOPE_OFFLINE "OfflineEditingPlugin"
#define PROJECT_ENTRY_KEY_OFFLINE_DB_PATH "/OfflineDbPath"

QgsOfflineEditing::QgsOfflineEditing( QgsOfflineEditingProgressDialog* progressDialog )
{
  mProgressDialog = progressDialog;
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWasAdded( QgsMapLayer* ) ), this, SLOT( layerAdded( QgsMapLayer* ) ) );
}

QgsOfflineEditing::~QgsOfflineEditing()
{
  if ( mProgressDialog != NULL )
  {
    delete mProgressDialog;
  }
  disconnect( QgsMapLayerRegistry::instance(), SIGNAL( layerWasAdded( QgsMapLayer* ) ), this, SLOT( layerAdded( QgsMapLayer* ) ) );
}

/**
 * convert current project to offline project
 * returns offline project file path
 */
bool QgsOfflineEditing::convertToOfflineProject( const QString& offlineDataPath, const QString& offlineDbFile, const QStringList& layerIds )
{
  if ( layerIds.isEmpty() )
  {
    return false;
  }
  QString dbPath = QDir( offlineDataPath ).absoluteFilePath( offlineDbFile );
  if ( createSpatialiteDB( dbPath ) )
  {
    spatialite_init( 0 );
    sqlite3* db;
    int rc = sqlite3_open( dbPath.toStdString().c_str(), &db );
    if ( rc != SQLITE_OK )
    {
      showWarning( tr( "Could not open the spatialite database" ) );
    }
    else
    {
      // create logging tables
      createLoggingTables( db );

      mProgressDialog->setTitle( "Converting to offline project" );
      mProgressDialog->show();

      // copy selected vector layers to SpatiaLite
      for ( int i = 0; i < layerIds.count(); i++ )
      {
        mProgressDialog->setCurrentLayer( i + 1, layerIds.count() );

        QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerIds.at( i ) );
        copyVectorLayer( qobject_cast<QgsVectorLayer*>( layer ), db, dbPath );
      }

      mProgressDialog->hide();

      sqlite3_close( db );

      // save offline project
      QString projectTitle = QgsProject::instance()->title();
      if ( projectTitle.isEmpty() )
      {
        projectTitle = QFileInfo( QgsProject::instance()->fileName() ).fileName();
      }
      projectTitle += " (offline)";
      QgsProject::instance()->title( projectTitle );

      QgsProject::instance()->writeEntry( PROJECT_ENTRY_SCOPE_OFFLINE, PROJECT_ENTRY_KEY_OFFLINE_DB_PATH, dbPath );

      return true;
    }
  }

  return false;

  // Workflow:
  // copy layers to spatialite
  // create spatialite db at offlineDataPath
  // create table for each layer
  // add new spatialite layer
  // copy features
  // save as offline project
  // mark offline layers
  // remove remote layers
  // mark as offline project
}

bool QgsOfflineEditing::isOfflineProject()
{
  return !QgsProject::instance()->readEntry( PROJECT_ENTRY_SCOPE_OFFLINE, PROJECT_ENTRY_KEY_OFFLINE_DB_PATH ).isEmpty();
}

void QgsOfflineEditing::synchronize( QgsLegendInterface* legendInterface )
{
  // open logging db
  sqlite3* db = openLoggingDb();
  if ( db == NULL )
  {
    return;
  }

  mProgressDialog->setTitle( "Synchronizing to remote layers" );
  mProgressDialog->show();

  // restore and sync remote layers
  QList<QgsMapLayer*> offlineLayers;
  QMap<QString, QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
  for ( QMap<QString, QgsMapLayer*>::iterator layer_it = mapLayers.begin() ; layer_it != mapLayers.end(); ++layer_it )
  {
    QgsMapLayer* layer = layer_it.value();
    if ( layer->customProperty( CUSTOM_PROPERTY_IS_OFFLINE_EDITABLE, false ).toBool() )
    {
      offlineLayers << layer;
    }
  }

  for ( int l = 0; l < offlineLayers.count(); l++ )
  {
    QgsMapLayer* layer = offlineLayers[l];

    mProgressDialog->setCurrentLayer( l + 1, offlineLayers.count() );

    QString remoteSource = layer->customProperty( CUSTOM_PROPERTY_REMOTE_SOURCE, "" ).toString();
    QString remoteProvider = layer->customProperty( CUSTOM_PROPERTY_REMOTE_PROVIDER, "" ).toString();
    QString remoteName = layer->name();
    remoteName.remove( QRegExp( " \\(offline\\)$" ) );

    QgsVectorLayer* remoteLayer = new QgsVectorLayer( remoteSource, remoteName, remoteProvider );
    if ( remoteLayer->isValid() )
    {
      // TODO: only add remote layer if there are log entries?

      QgsVectorLayer* offlineLayer = qobject_cast<QgsVectorLayer*>( layer );

      // copy style
      copySymbology( offlineLayer, remoteLayer );

      // register this layer with the central layers registry
      QgsMapLayerRegistry::instance()->addMapLayer( remoteLayer, true );

      // apply layer edit log
      QString qgisLayerId = layer->getLayerID();
      QString sql = QString( "SELECT \"id\" FROM 'log_layer_ids' WHERE \"qgis_id\" = '%1'" ).arg( qgisLayerId );
      int layerId = sqlQueryInt( db, sql, -1 );
      if ( layerId != -1 )
      {
        remoteLayer->startEditing();

        // TODO: only get commitNos of this layer?
        int commitNo = getCommitNo( db );
        for ( int i = 0; i < commitNo; i++ )
        {
          // apply commits chronologically
          applyAttributesAdded( remoteLayer, db, layerId, i );
          applyAttributeValueChanges( offlineLayer, remoteLayer, db, layerId, i );
          applyGeometryChanges( remoteLayer, db, layerId, i );
        }

        applyFeaturesAdded( offlineLayer, remoteLayer, db, layerId );
        applyFeaturesRemoved( remoteLayer, db, layerId );

        if ( remoteLayer->commitChanges() )
        {
          // update fid lookup
          updateFidLookup( remoteLayer, db, layerId );

          // clear edit log for this layer
          sql = QString( "DELETE FROM 'log_added_attrs' WHERE \"layer_id\" = %1" ).arg( layerId );
          sqlExec( db, sql );
          sql = QString( "DELETE FROM 'log_added_features' WHERE \"layer_id\" = %1" ).arg( layerId );
          sqlExec( db, sql );
          sql = QString( "DELETE FROM 'log_removed_features' WHERE \"layer_id\" = %1" ).arg( layerId );
          sqlExec( db, sql );
          sql = QString( "DELETE FROM 'log_feature_updates' WHERE \"layer_id\" = %1" ).arg( layerId );
          sqlExec( db, sql );
          sql = QString( "DELETE FROM 'log_geometry_updates' WHERE \"layer_id\" = %1" ).arg( layerId );
          sqlExec( db, sql );

          // reset commitNo
          QString sql = QString( "UPDATE 'log_indices' SET 'last_index' = 0 WHERE \"name\" = 'commit_no'" );
          sqlExec( db, sql );
        }
        else
        {
          showWarning( remoteLayer->commitErrors().join( "\n" ) );
        }
      }

      // remove offline layer
      QgsMapLayerRegistry::instance()->removeMapLayer( qgisLayerId, true );

      // disable offline project
      QString projectTitle = QgsProject::instance()->title();
      projectTitle.remove( QRegExp( " \\(offline\\)$" ) );
      QgsProject::instance()->title( projectTitle );
      QgsProject::instance()->removeEntry( PROJECT_ENTRY_SCOPE_OFFLINE, PROJECT_ENTRY_KEY_OFFLINE_DB_PATH );
      remoteLayer->reload(); //update with other changes
    }
  }

  mProgressDialog->hide();

  sqlite3_close( db );
}

bool QgsOfflineEditing::createSpatialiteDB( const QString& offlineDbPath )
{
  QFile newDb( offlineDbPath );
  if ( newDb.exists() )
  {
    QFile::remove( offlineDbPath );
  }

  // see also QgsNewSpatialiteLayerDialog::createDb()

  // copy the spatialite template to the user specified path
  QString spatialiteTemplate = QgsApplication::qgisSpatialiteDbTemplatePath();
  QFile spatialiteTemplateDb( spatialiteTemplate );

  QFileInfo fullPath = QFileInfo( offlineDbPath );
  QDir path = fullPath.dir();

  // Must be sure there is destination directory ~/.qgis
  QDir().mkpath( path.absolutePath( ) );

  // now copy the template db file into the chosen location
  if ( !spatialiteTemplateDb.copy( newDb.fileName() ) )
  {
    showWarning( tr( "Could not copy the template database to new location" ) );
    return false;
  }

  return true;
}

void QgsOfflineEditing::createLoggingTables( sqlite3* db )
{
  // indices
  QString sql = "CREATE TABLE 'log_indices' ('name' TEXT, 'last_index' INTEGER)";
  sqlExec( db, sql );

  sql = "INSERT INTO 'log_indices' VALUES ('commit_no', 0)";
  sqlExec( db, sql );

  sql = "INSERT INTO 'log_indices' VALUES ('layer_id', 0)";
  sqlExec( db, sql );

  // layername <-> layer id
  sql = "CREATE TABLE 'log_layer_ids' ('id' INTEGER, 'qgis_id' TEXT)";
  sqlExec( db, sql );

  // offline fid <-> remote fid
  sql = "CREATE TABLE 'log_fids' ('layer_id' INTEGER, 'offline_fid' INTEGER, 'remote_fid' INTEGER)";
  sqlExec( db, sql );

  // added attributes
  sql = "CREATE TABLE 'log_added_attrs' ('layer_id' INTEGER, 'commit_no' INTEGER, ";
  sql += "'name' TEXT, 'type' INTEGER, 'length' INTEGER, 'precision' INTEGER, 'comment' TEXT)";
  sqlExec( db, sql );

  // added features
  sql = "CREATE TABLE 'log_added_features' ('layer_id' INTEGER, 'fid' INTEGER)";
  sqlExec( db, sql );

  // removed features
  sql = "CREATE TABLE 'log_removed_features' ('layer_id' INTEGER, 'fid' INTEGER)";
  sqlExec( db, sql );

  // feature updates
  sql = "CREATE TABLE 'log_feature_updates' ('layer_id' INTEGER, 'commit_no' INTEGER, 'fid' INTEGER, 'attr' INTEGER, 'value' TEXT)";
  sqlExec( db, sql );

  // geometry updates
  sql = "CREATE TABLE 'log_geometry_updates' ('layer_id' INTEGER, 'commit_no' INTEGER, 'fid' INTEGER, 'geom_wkt' TEXT)";
  sqlExec( db, sql );

  /* TODO: other logging tables
    - attr delete (not supported by SpatiaLite provider)
  */
}

void QgsOfflineEditing::copyVectorLayer( QgsVectorLayer* layer, sqlite3* db, const QString& offlineDbPath )
{
  if ( layer == NULL )
  {
    return;
  }

  QString tableName = layer->name();

  // create table
  QString sql = QString( "CREATE TABLE '%1' (" ).arg( tableName );
  QString delim = "";
  const QgsFieldMap& fields = layer->dataProvider()->fields();
  for ( QgsFieldMap::const_iterator it = fields.begin(); it != fields.end() ; ++it )
  {
    QString dataType = "";
    QVariant::Type type = it.value().type();
    if ( type == QVariant::Int )
    {
      dataType = "INTEGER";
    }
    else if ( type == QVariant::Double )
    {
      dataType = "REAL";
    }
    else if ( type == QVariant::String )
    {
      dataType = "TEXT";
    }
    else
    {
      showWarning( tr( "Unknown data type %1" ).arg( type ) );
    }

    sql += delim + QString( "'%1' %2" ).arg( it.value().name() ).arg( dataType );
    delim = ",";
  }
  sql += ")";

  // add geometry column
  QString geomType = "";
  switch ( layer->geometryType() )
  {
    case QGis::Point:
      geomType = "POINT";
      break;
    case QGis::Line:
      geomType = "LINESTRING";
      break;
    case QGis::Polygon:
      geomType = "POLYGON";
      break;
    default:
      showWarning( tr( "Unknown QGIS geometry type %1" ).arg( layer->geometryType() ) );
      break;
  };
  QString sqlAddGeom = QString( "SELECT AddGeometryColumn('%1', 'Geometry', %2, '%3', 2)" )
                       .arg( tableName )
                       .arg( layer->crs().epsg() )
                       .arg( geomType );

  // create spatial index
  QString sqlCreateIndex = QString( "SELECT CreateSpatialIndex('%1', 'Geometry')" ).arg( tableName );

  int rc = sqlExec( db, sql );
  if ( rc == SQLITE_OK )
  {
    rc = sqlExec( db, sqlAddGeom );
    if ( rc == SQLITE_OK )
    {
      rc = sqlExec( db, sqlCreateIndex );
    }
  }

  if ( rc == SQLITE_OK )
  {
    // add new layer
    QgsVectorLayer* newLayer = new QgsVectorLayer( QString( "dbname='%1' table='%2'(Geometry) sql=" )
        .arg( offlineDbPath ).arg( tableName ), tableName + " (offline)", "spatialite" );
    if ( newLayer->isValid() )
    {
      // mark as offline layer
      newLayer->setCustomProperty( CUSTOM_PROPERTY_IS_OFFLINE_EDITABLE, true );

      // store original layer source
      newLayer->setCustomProperty( CUSTOM_PROPERTY_REMOTE_SOURCE, layer->source() );
      newLayer->setCustomProperty( CUSTOM_PROPERTY_REMOTE_PROVIDER, layer->providerType() );

      // copy style
      bool hasLabels = layer->hasLabelsEnabled();
      if ( !hasLabels )
      {
        // NOTE: copy symbology before adding the layer so it is displayed correctly
        copySymbology( layer, newLayer );
      }

      // register this layer with the central layers registry
      QgsMapLayerRegistry::instance()->addMapLayer( newLayer );

      if ( hasLabels )
      {
        // NOTE: copy symbology of layers with labels enabled after adding to project, as it will crash otherwise (WORKAROUND)
        copySymbology( layer, newLayer );
      }

      // TODO: layer order

      // copy features
      newLayer->startEditing();
      QgsFeature f;

      // NOTE: force feature recount for PostGIS layer, else only visible features are counted, before iterating over all features (WORKAROUND)
      layer->setSubsetString( "" );

      layer->select( layer->pendingAllAttributesList(), QgsRectangle(), true, false );

      mProgressDialog->setupProgressBar( tr( "%v / %m features copied" ), layer->featureCount() );
      int featureCount = 1;

      QList<int> remoteFeatureIds;
      while ( layer->nextFeature( f ) )
      {
        remoteFeatureIds << f.id();

        // NOTE: Spatialite provider ignores position of geometry column
        // fill gap in QgsAttributeMap if geometry column is not last (WORKAROUND)
        int column = 0;
        QgsAttributeMap newAttrMap;
        QgsAttributeMap attrMap = f.attributeMap();
        for ( QgsAttributeMap::const_iterator it = attrMap.begin(); it != attrMap.end(); ++it )
        {
          newAttrMap.insert( column++, it.value() );
        }
        f.setAttributeMap( newAttrMap );

        newLayer->addFeature( f, false );

        mProgressDialog->setProgressValue( featureCount++ );
      }
      if ( newLayer->commitChanges() )
      {
        mProgressDialog->setupProgressBar( tr( "%v / %m features processed" ), layer->featureCount() );
        featureCount = 1;

        // update feature id lookup
        int layerId = getOrCreateLayerId( db, newLayer->getLayerID() );
        QList<int> offlineFeatureIds;
        newLayer->select( QgsAttributeList(), QgsRectangle(), false, false );
        while ( newLayer->nextFeature( f ) )
        {
          offlineFeatureIds << f.id();
        }

        // NOTE: insert fids in this loop, as the db is locked during newLayer->nextFeature()
        sqlExec( db, "BEGIN" );
        for ( int i = 0; i < remoteFeatureIds.size(); i++ )
        {
          addFidLookup( db, layerId, offlineFeatureIds.at( i ), remoteFeatureIds.at( i ) );

          mProgressDialog->setProgressValue( featureCount++ );
        }
        sqlExec( db, "COMMIT" );
      }
      else
      {
        showWarning( newLayer->commitErrors().join( "\n" ) );
      }

      // remove remote layer
      QgsMapLayerRegistry::instance()->removeMapLayer( layer->getLayerID() );
    }
  }
}

void QgsOfflineEditing::applyAttributesAdded( QgsVectorLayer* remoteLayer, sqlite3* db, int layerId, int commitNo )
{
  QString sql = QString( "SELECT \"name\", \"type\", \"length\", \"precision\", \"comment\" FROM 'log_added_attrs' WHERE \"layer_id\" = %1 AND \"commit_no\" = %2" ).arg( layerId ).arg( commitNo );
  QList<QgsField> fields = sqlQueryAttributesAdded( db, sql );

  const QgsVectorDataProvider* provider = remoteLayer->dataProvider();
  QList<QgsVectorDataProvider::NativeType> nativeTypes = provider->nativeTypes();

  // NOTE: uses last matching QVariant::Type of nativeTypes
  QMap < QVariant::Type, QString /*typeName*/ > typeNameLookup;
  for ( int i = 0; i < nativeTypes.size(); i++ )
  {
    QgsVectorDataProvider::NativeType nativeType = nativeTypes.at( i );
    typeNameLookup[ nativeType.mType ] = nativeType.mTypeName;
  }

  mProgressDialog->setupProgressBar( tr( "%v / %m fields added" ), fields.size() );

  for ( int i = 0; i < fields.size(); i++ )
  {
    // lookup typename from layer provider
    QgsField field = fields[i];
    if ( typeNameLookup.contains( field.type() ) )
    {
      QString typeName = typeNameLookup[ field.type()];
      field.setTypeName( typeName );
      remoteLayer->addAttribute( field );
    }
    else
    {
      showWarning( QString( "Could not add attribute '%1' of type %2" ).arg( field.name() ).arg( field.type() ) );
    }

    mProgressDialog->setProgressValue( i + 1 );
  }
}

void QgsOfflineEditing::applyFeaturesAdded( QgsVectorLayer* offlineLayer, QgsVectorLayer* remoteLayer, sqlite3* db, int layerId )
{
  QString sql = QString( "SELECT \"fid\" FROM 'log_added_features' WHERE \"layer_id\" = %1" ).arg( layerId );
  QList<int> newFeatureIds = sqlQueryInts( db, sql );

  // get new features from offline layer
  QgsFeatureList features;
  for ( int i = 0; i < newFeatureIds.size(); i++ )
  {
    QgsFeature feature;
    if ( offlineLayer->featureAtId( newFeatureIds.at( i ), feature, true, true ) )
    {
      features << feature;
    }
  }

  // copy features to remote layer
  mProgressDialog->setupProgressBar( tr( "%v / %m features added" ), features.size() );

  int i = 1;
  for ( QgsFeatureList::iterator it = features.begin(); it != features.end(); ++it )
  {
    QgsFeature f = *it;

    // NOTE: Spatialite provider ignores position of geometry column
    // restore gap in QgsAttributeMap if geometry column is not last (WORKAROUND)
    QMap<int, int> attrLookup = attributeLookup( offlineLayer, remoteLayer );
    QgsAttributeMap newAttrMap;
    QgsAttributeMap attrMap = f.attributeMap();
    for ( QgsAttributeMap::const_iterator it = attrMap.begin(); it != attrMap.end(); ++it )
    {
      newAttrMap.insert( attrLookup[ it.key()], it.value() );
    }
    f.setAttributeMap( newAttrMap );

    remoteLayer->addFeature( f, false );

    mProgressDialog->setProgressValue( i++ );
  }
}

void QgsOfflineEditing::applyFeaturesRemoved( QgsVectorLayer* remoteLayer, sqlite3* db, int layerId )
{
  QString sql = QString( "SELECT \"fid\" FROM 'log_removed_features' WHERE \"layer_id\" = %1" ).arg( layerId );
  QgsFeatureIds values = sqlQueryFeaturesRemoved( db, sql );

  mProgressDialog->setupProgressBar( tr( "%v / %m features removed" ), values.size() );

  int i = 1;
  for ( QgsFeatureIds::const_iterator it = values.begin(); it != values.end(); ++it )
  {
    int fid = remoteFid( db, layerId, *it );
    remoteLayer->deleteFeature( fid );

    mProgressDialog->setProgressValue( i++ );
  }
}

void QgsOfflineEditing::applyAttributeValueChanges( QgsVectorLayer* offlineLayer, QgsVectorLayer* remoteLayer, sqlite3* db, int layerId, int commitNo )
{
  QString sql = QString( "SELECT \"fid\", \"attr\", \"value\" FROM 'log_feature_updates' WHERE \"layer_id\" = %1 AND \"commit_no\" = %2 " ).arg( layerId ).arg( commitNo );
  AttributeValueChanges values = sqlQueryAttributeValueChanges( db, sql );

  mProgressDialog->setupProgressBar( tr( "%v / %m feature updates" ), values.size() );

  QMap<int, int> attrLookup = attributeLookup( offlineLayer, remoteLayer );

  for ( int i = 0; i < values.size(); i++ )
  {
    int fid = remoteFid( db, layerId, values.at( i ).fid );

    remoteLayer->changeAttributeValue( fid, attrLookup[ values.at( i ).attr ], values.at( i ).value, false );

    mProgressDialog->setProgressValue( i + 1 );
  }
}

void QgsOfflineEditing::applyGeometryChanges( QgsVectorLayer* remoteLayer, sqlite3* db, int layerId, int commitNo )
{
  QString sql = QString( "SELECT \"fid\", \"geom_wkt\" FROM 'log_geometry_updates' WHERE \"layer_id\" = %1 AND \"commit_no\" = %2" ).arg( layerId ).arg( commitNo );
  GeometryChanges values = sqlQueryGeometryChanges( db, sql );

  mProgressDialog->setupProgressBar( tr( "%v / %m feature geometry updates" ), values.size() );

  for ( int i = 0; i < values.size(); i++ )
  {
    int fid = remoteFid( db, layerId, values.at( i ).fid );
    remoteLayer->changeGeometry( fid, QgsGeometry::fromWkt( values.at( i ).geom_wkt ) );

    mProgressDialog->setProgressValue( i + 1 );
  }
}

void QgsOfflineEditing::updateFidLookup( QgsVectorLayer* remoteLayer, sqlite3* db, int layerId )
{
  // update fid lookup for added features

  // get remote added fids
  // NOTE: use QMap for sorted fids
  QMap < int, bool /*dummy*/ > newRemoteFids;
  QgsFeature f;
  remoteLayer->select( QgsAttributeList(), QgsRectangle(), false, false );

  mProgressDialog->setupProgressBar( tr( "%v / %m features processed" ), remoteLayer->featureCount() );

  int i = 1;
  while ( remoteLayer->nextFeature( f ) )
  {
    if ( offlineFid( db, layerId, f.id() ) == -1 )
    {
      newRemoteFids[ f.id()] = true;
    }

    mProgressDialog->setProgressValue( i++ );
  }

  // get local added fids
  // NOTE: fids are sorted
  QString sql = QString( "SELECT \"fid\" FROM 'log_added_features' WHERE \"layer_id\" = %1" ).arg( layerId );
  QList<int> newOfflineFids = sqlQueryInts( db, sql );

  if ( newRemoteFids.size() != newOfflineFids.size() )
  {
    //showWarning( QString( "Different number of new features on offline layer (%1) and remote layer (%2)" ).arg(newOfflineFids.size()).arg(newRemoteFids.size()) );
  }
  else
  {
    // add new fid lookups
    i = 0;
    sqlExec( db, "BEGIN" );
    for ( QMap<int, bool>::const_iterator it = newRemoteFids.begin(); it != newRemoteFids.end(); ++it )
    {
      addFidLookup( db, layerId, newOfflineFids.at( i++ ), it.key() );
    }
    sqlExec( db, "COMMIT" );
  }
}

void QgsOfflineEditing::copySymbology( const QgsVectorLayer* sourceLayer, QgsVectorLayer* targetLayer )
{
  QString error;
  QDomDocument doc;
  QDomElement node = doc.createElement( "symbology" );
  doc.appendChild( node );
  sourceLayer->writeSymbology( node, doc, error );

  if ( error.isEmpty() )
  {
    targetLayer->readSymbology( node, error );
  }
  if ( !error.isEmpty() )
  {
    showWarning( error );
  }
}

// NOTE: use this to map column indices in case the remote geometry column is not last
QMap<int, int> QgsOfflineEditing::attributeLookup( QgsVectorLayer* offlineLayer, QgsVectorLayer* remoteLayer )
{
  const QgsAttributeList& offlineAttrs = offlineLayer->pendingAllAttributesList();
  const QgsAttributeList& remoteAttrs = remoteLayer->pendingAllAttributesList();

  QMap < int /*offline attr*/, int /*remote attr*/ > attrLookup;
  // NOTE: use size of remoteAttrs, as offlineAttrs can have new attributes not yet synced
  for ( int i = 0; i < remoteAttrs.size(); i++ )
  {
    attrLookup.insert( offlineAttrs.at( i ), remoteAttrs.at( i ) );
  }

  return attrLookup;
}

void QgsOfflineEditing::showWarning( const QString& message )
{
  QMessageBox::warning( NULL, tr( "Offline Editing Plugin" ), message );
}

sqlite3* QgsOfflineEditing::openLoggingDb()
{
  sqlite3* db = NULL;
  QString dbPath = QgsProject::instance()->readEntry( PROJECT_ENTRY_SCOPE_OFFLINE, PROJECT_ENTRY_KEY_OFFLINE_DB_PATH );
  if ( !dbPath.isEmpty() )
  {
    int rc = sqlite3_open( dbPath.toStdString().c_str(), &db );
    if ( rc != SQLITE_OK )
    {
      showWarning( tr( "Could not open the spatialite logging database" ) );
      sqlite3_close( db );
      db = NULL;
    }
  }
  return db;
}

int QgsOfflineEditing::getOrCreateLayerId( sqlite3* db, const QString& qgisLayerId )
{
  QString sql = QString( "SELECT \"id\" FROM 'log_layer_ids' WHERE \"qgis_id\" = '%1'" ).arg( qgisLayerId );
  int layerId = sqlQueryInt( db, sql, -1 );
  if ( layerId == -1 )
  {
    // next layer id
    sql = "SELECT \"last_index\" FROM 'log_indices' WHERE \"name\" = 'layer_id'";
    int newLayerId = sqlQueryInt( db, sql, -1 );

    // insert layer
    sql = QString( "INSERT INTO 'log_layer_ids' VALUES (%1, '%2')" ).arg( newLayerId ).arg( qgisLayerId );
    sqlExec( db, sql );

    // increase layer_id
    // TODO: use trigger for auto increment?
    sql = QString( "UPDATE 'log_indices' SET 'last_index' = %1 WHERE \"name\" = 'layer_id'" ).arg( newLayerId + 1 );
    sqlExec( db, sql );

    layerId = newLayerId;
  }

  return layerId;
}

int QgsOfflineEditing::getCommitNo( sqlite3* db )
{
  QString sql = "SELECT \"last_index\" FROM 'log_indices' WHERE \"name\" = 'commit_no'";
  return sqlQueryInt( db, sql, -1 );
}

void QgsOfflineEditing::increaseCommitNo( sqlite3* db )
{
  QString sql = QString( "UPDATE 'log_indices' SET 'last_index' = %1 WHERE \"name\" = 'commit_no'" ).arg( getCommitNo( db ) + 1 );
  sqlExec( db, sql );
}

void QgsOfflineEditing::addFidLookup( sqlite3* db, int layerId, int offlineFid, int remoteFid )
{
  QString sql = QString( "INSERT INTO 'log_fids' VALUES ( %1, %2, %3 )" ).arg( layerId ).arg( offlineFid ).arg( remoteFid );
  sqlExec( db, sql );
}

int QgsOfflineEditing::remoteFid( sqlite3* db, int layerId, int offlineFid )
{
  QString sql = QString( "SELECT \"remote_fid\" FROM 'log_fids' WHERE \"layer_id\" = %1 AND \"offline_fid\" = %2" ).arg( layerId ).arg( offlineFid );
  return sqlQueryInt( db, sql, -1 );
}

int QgsOfflineEditing::offlineFid( sqlite3* db, int layerId, int remoteFid )
{
  QString sql = QString( "SELECT \"offline_fid\" FROM 'log_fids' WHERE \"layer_id\" = %1 AND \"remote_fid\" = %2" ).arg( layerId ).arg( remoteFid );
  return sqlQueryInt( db, sql, -1 );
}

bool QgsOfflineEditing::isAddedFeature( sqlite3* db, int layerId, int fid )
{
  QString sql = QString( "SELECT COUNT(\"fid\") FROM 'log_added_features' WHERE \"layer_id\" = %1 AND \"fid\" = %2" ).arg( layerId ).arg( fid );
  return ( sqlQueryInt( db, sql, 0 ) > 0 );
}

int QgsOfflineEditing::sqlExec( sqlite3* db, const QString& sql )
{
  char * errmsg;
  int rc = sqlite3_exec( db, sql.toUtf8(), NULL, NULL, &errmsg );
  if ( rc != SQLITE_OK )
  {
    showWarning( errmsg );
  }
  return rc;
}

int QgsOfflineEditing::sqlQueryInt( sqlite3* db, const QString& sql, int defaultValue )
{
  sqlite3_stmt* stmt = NULL;
  if ( sqlite3_prepare_v2( db, sql.toUtf8().constData(), -1, &stmt, NULL ) != SQLITE_OK )
  {
    showWarning( sqlite3_errmsg( db ) );
    return defaultValue;
  }

  int value = defaultValue;
  int ret = sqlite3_step( stmt );
  if ( ret == SQLITE_ROW )
  {
    value = sqlite3_column_int( stmt, 0 );
  }
  sqlite3_finalize( stmt );

  return value;
}

QList<int> QgsOfflineEditing::sqlQueryInts( sqlite3* db, const QString& sql )
{
  QList<int> values;

  sqlite3_stmt* stmt = NULL;
  if ( sqlite3_prepare_v2( db, sql.toUtf8().constData(), -1, &stmt, NULL ) != SQLITE_OK )
  {
    showWarning( sqlite3_errmsg( db ) );
    return values;
  }

  int ret = sqlite3_step( stmt );
  while ( ret == SQLITE_ROW )
  {
    values << sqlite3_column_int( stmt, 0 );

    ret = sqlite3_step( stmt );
  }
  sqlite3_finalize( stmt );

  return values;
}

QList<QgsField> QgsOfflineEditing::sqlQueryAttributesAdded( sqlite3* db, const QString& sql )
{
  QList<QgsField> values;

  sqlite3_stmt* stmt = NULL;
  if ( sqlite3_prepare_v2( db, sql.toUtf8().constData(), -1, &stmt, NULL ) != SQLITE_OK )
  {
    showWarning( sqlite3_errmsg( db ) );
    return values;
  }

  int ret = sqlite3_step( stmt );
  while ( ret == SQLITE_ROW )
  {
    QgsField field( QString(( const char* )sqlite3_column_text( stmt, 0 ) ),
                    ( QVariant::Type )sqlite3_column_int( stmt, 1 ),
                    "", // typeName
                    sqlite3_column_int( stmt, 2 ),
                    sqlite3_column_int( stmt, 3 ),
                    QString(( const char* )sqlite3_column_text( stmt, 4 ) ) );
    values << field;

    ret = sqlite3_step( stmt );
  }
  sqlite3_finalize( stmt );

  return values;
}

QgsFeatureIds QgsOfflineEditing::sqlQueryFeaturesRemoved( sqlite3* db, const QString& sql )
{
  QgsFeatureIds values;

  sqlite3_stmt* stmt = NULL;
  if ( sqlite3_prepare_v2( db, sql.toUtf8().constData(), -1, &stmt, NULL ) != SQLITE_OK )
  {
    showWarning( sqlite3_errmsg( db ) );
    return values;
  }

  int ret = sqlite3_step( stmt );
  while ( ret == SQLITE_ROW )
  {
    values << sqlite3_column_int( stmt, 0 );

    ret = sqlite3_step( stmt );
  }
  sqlite3_finalize( stmt );

  return values;
}

QgsOfflineEditing::AttributeValueChanges QgsOfflineEditing::sqlQueryAttributeValueChanges( sqlite3* db, const QString& sql )
{
  AttributeValueChanges values;

  sqlite3_stmt* stmt = NULL;
  if ( sqlite3_prepare_v2( db, sql.toUtf8().constData(), -1, &stmt, NULL ) != SQLITE_OK )
  {
    showWarning( sqlite3_errmsg( db ) );
    return values;
  }

  int ret = sqlite3_step( stmt );
  while ( ret == SQLITE_ROW )
  {
    AttributeValueChange change;
    change.fid = sqlite3_column_int( stmt, 0 );
    change.attr = sqlite3_column_int( stmt, 1 );
    change.value = QString(( const char* )sqlite3_column_text( stmt, 2 ) );
    values << change;

    ret = sqlite3_step( stmt );
  }
  sqlite3_finalize( stmt );

  return values;
}

QgsOfflineEditing::GeometryChanges QgsOfflineEditing::sqlQueryGeometryChanges( sqlite3* db, const QString& sql )
{
  GeometryChanges values;

  sqlite3_stmt* stmt = NULL;
  if ( sqlite3_prepare_v2( db, sql.toUtf8().constData(), -1, &stmt, NULL ) != SQLITE_OK )
  {
    showWarning( sqlite3_errmsg( db ) );
    return values;
  }

  int ret = sqlite3_step( stmt );
  while ( ret == SQLITE_ROW )
  {
    GeometryChange change;
    change.fid = sqlite3_column_int( stmt, 0 );
    change.geom_wkt = QString(( const char* )sqlite3_column_text( stmt, 1 ) );
    values << change;

    ret = sqlite3_step( stmt );
  }
  sqlite3_finalize( stmt );

  return values;
}

void QgsOfflineEditing::layerAdded( QgsMapLayer* layer )
{
  // detect offline layer
  if ( layer->customProperty( CUSTOM_PROPERTY_IS_OFFLINE_EDITABLE, false ).toBool() )
  {
    // enable logging
    connect( layer, SIGNAL( committedAttributesAdded( const QString&, const QList<QgsField>& ) ),
             this, SLOT( committedAttributesAdded( const QString&, const QList<QgsField>& ) ) );
    connect( layer, SIGNAL( committedFeaturesAdded( const QString&, const QgsFeatureList& ) ),
             this, SLOT( committedFeaturesAdded( const QString&, const QgsFeatureList& ) ) );
    connect( layer, SIGNAL( committedFeaturesRemoved( const QString&, const QgsFeatureIds& ) ),
             this, SLOT( committedFeaturesRemoved( const QString&, const QgsFeatureIds& ) ) );
    connect( layer, SIGNAL( committedAttributeValuesChanges( const QString&, const QgsChangedAttributesMap& ) ),
             this, SLOT( committedAttributeValuesChanges( const QString&, const QgsChangedAttributesMap& ) ) );
    connect( layer, SIGNAL( committedGeometriesChanges( const QString&, const QgsGeometryMap& ) ),
             this, SLOT( committedGeometriesChanges( const QString&, const QgsGeometryMap& ) ) );
  }
}

void QgsOfflineEditing::committedAttributesAdded( const QString& qgisLayerId, const QList<QgsField>& addedAttributes )
{
  sqlite3* db = openLoggingDb();
  if ( db == NULL )
  {
    return;
  }

  // insert log
  int layerId = getOrCreateLayerId( db, qgisLayerId );
  int commitNo = getCommitNo( db );

  for ( QList<QgsField>::const_iterator it = addedAttributes.begin(); it != addedAttributes.end(); ++it )
  {
    QgsField field = *it;
    QString sql = QString( "INSERT INTO 'log_added_attrs' VALUES ( %1, %2, '%3', %4, %5, %6, '%7' )" )
                  .arg( layerId )
                  .arg( commitNo )
                  .arg( field.name() )
                  .arg( field.type() )
                  .arg( field.length() )
                  .arg( field.precision() )
                  .arg( field.comment() );
    sqlExec( db, sql );
  }

  increaseCommitNo( db );
  sqlite3_close( db );
}

void QgsOfflineEditing::committedFeaturesAdded( const QString& qgisLayerId, const QgsFeatureList& addedFeatures )
{
  sqlite3* db = openLoggingDb();
  if ( db == NULL )
  {
    return;
  }

  // insert log
  int layerId = getOrCreateLayerId( db, qgisLayerId );

  // get new feature ids from db
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( qgisLayerId );
  QgsDataSourceURI uri = QgsDataSourceURI( layer->source() );

  // only store feature ids
  QString sql = QString( "SELECT ROWID FROM '%1' ORDER BY ROWID DESC LIMIT %2" ).arg( uri.table() ).arg( addedFeatures.size() );
  QList<int> newFeatureIds = sqlQueryInts( db, sql );
  for ( int i = newFeatureIds.size() - 1; i >= 0; i-- )
  {
    QString sql = QString( "INSERT INTO 'log_added_features' VALUES ( %1, %2 )" )
                  .arg( layerId )
                  .arg( newFeatureIds.at( i ) );
    sqlExec( db, sql );
  }

  sqlite3_close( db );
}

void QgsOfflineEditing::committedFeaturesRemoved( const QString& qgisLayerId, const QgsFeatureIds& deletedFeatureIds )
{
  sqlite3* db = openLoggingDb();
  if ( db == NULL )
  {
    return;
  }

  // insert log
  int layerId = getOrCreateLayerId( db, qgisLayerId );

  for ( QgsFeatureIds::const_iterator it = deletedFeatureIds.begin(); it != deletedFeatureIds.end(); ++it )
  {
    if ( isAddedFeature( db, layerId, *it ) )
    {
      // remove from added features log
      QString sql = QString( "DELETE FROM 'log_added_features' WHERE \"layer_id\" = %1 AND \"fid\" = %2" ).arg( layerId ).arg( *it );
      sqlExec( db, sql );
    }
    else
    {
      QString sql = QString( "INSERT INTO 'log_removed_features' VALUES ( %1, %2)" )
                    .arg( layerId )
                    .arg( *it );
      sqlExec( db, sql );
    }
  }

  sqlite3_close( db );
}

void QgsOfflineEditing::committedAttributeValuesChanges( const QString& qgisLayerId, const QgsChangedAttributesMap& changedAttrsMap )
{
  sqlite3* db = openLoggingDb();
  if ( db == NULL )
  {
    return;
  }

  // insert log
  int layerId = getOrCreateLayerId( db, qgisLayerId );
  int commitNo = getCommitNo( db );

  for ( QgsChangedAttributesMap::const_iterator cit = changedAttrsMap.begin(); cit != changedAttrsMap.end(); ++cit )
  {
    int fid = cit.key();
    if ( isAddedFeature( db, layerId, fid ) )
    {
      // skip added features
      continue;
    }
    QgsAttributeMap attrMap = cit.value();
    for ( QgsAttributeMap::const_iterator it = attrMap.begin(); it != attrMap.end(); ++it )
    {
      QString sql = QString( "INSERT INTO 'log_feature_updates' VALUES ( %1, %2, %3, %4, '%5' )" )
                    .arg( layerId )
                    .arg( commitNo )
                    .arg( fid )
                    .arg( it.key() ) // attr
                    .arg( it.value().toString() ); // value
      sqlExec( db, sql );
    }
  }

  increaseCommitNo( db );
  sqlite3_close( db );
}

void QgsOfflineEditing::committedGeometriesChanges( const QString& qgisLayerId, const QgsGeometryMap& changedGeometries )
{
  sqlite3* db = openLoggingDb();
  if ( db == NULL )
  {
    return;
  }

  // insert log
  int layerId = getOrCreateLayerId( db, qgisLayerId );
  int commitNo = getCommitNo( db );

  for ( QgsGeometryMap::const_iterator it = changedGeometries.begin(); it != changedGeometries.end(); ++it )
  {
    int fid = it.key();
    if ( isAddedFeature( db, layerId, fid ) )
    {
      // skip added features
      continue;
    }
    QgsGeometry geom = it.value();
    QString sql = QString( "INSERT INTO 'log_geometry_updates' VALUES ( %1, %2, %3, '%4' )" )
                  .arg( layerId )
                  .arg( commitNo )
                  .arg( fid )
                  .arg( geom.exportToWkt() );
    sqlExec( db, sql );

    // TODO: use WKB instead of WKT?
  }

  increaseCommitNo( db );
  sqlite3_close( db );
}
