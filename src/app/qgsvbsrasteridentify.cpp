/***************************************************************************
    qgsvbsrasteridentify.cpp
    ------------------------
    begin                : July 2016
    copyright            : (C) 2016 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <qjson/parser.h>

#include "qgisapp.h"
#include "qgsabstractgeometryv2.h"
#include "qgsarcgisrestutils.h"
#include "qgscrscache.h"
#include "qgsdatasourceuri.h"
#include "qgsgeometryrubberband.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgspinannotationitem.h"
#include "qgspointv2.h"
#include "qgsnetworkaccessmanager.h"
#include "qgsrasterlayer.h"
#include "qgsvbsrasteridentify.h"


void QgsVBSRasterIdentify::identify( const QgsMapCanvas *canvas, const QgsPoint &canvasPos )
{
  if ( instance()->mIdentifyReply )
  {
    disconnect( instance()->mIdentifyReply, SIGNAL( finished() ), instance(), SLOT( replyFinished() ) );
    disconnect( instance()->mTimeoutTimer, SIGNAL( timeout() ), instance(), SLOT( replyFinished() ) );
    instance()->mIdentifyReply->deleteLater(); // mTimeoutTimer is a child of mIdentifyReply
    instance()->mIdentifyReply = 0;
    instance()->mTimeoutTimer = 0;
  }

  const QgsCoordinateReferenceSystem& mapCrs = canvas->mapSettings().destinationCrs();
  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mapCrs.authid(), "EPSG:4326" );
  QgsPoint worldPos = crst->transform( canvasPos );
  QgsRectangle worldExtent = crst->transform( canvas->extent() );

  // List queryable rasters
  QStringList layerIds;
  QMap<QString, QVariant> layerMap;
  foreach ( const QgsMapLayer* layer, canvas->layers() )
  {
    const QgsRasterLayer* rlayer = qobject_cast<const QgsRasterLayer*>( layer );
    if ( !rlayer )
      continue;

    // Detect ArcGIS Rest MapServer layers
    QgsDataSourceURI dataSource( rlayer->dataProvider()->dataSourceUri() );
    QStringList urlParts = dataSource.param( "url" ).split( "/", QString::SkipEmptyParts );
    int nParts = urlParts.size();
    if ( nParts > 4 && urlParts[nParts - 1] == "MapServer" && urlParts[nParts - 4] == "services" )
    {
      layerIds.append( urlParts[nParts - 3] + ":" + urlParts[nParts - 2] );
      layerMap.insert( urlParts[nParts - 2], rlayer->id() );
    }

    // Detect geo.admin.ch layers
    dataSource.setEncodedUri( rlayer->dataProvider()->dataSourceUri() );
    if ( dataSource.param( "url" ).contains( "geo.admin.ch" ) )
    {
      QStringList sublayerIds = dataSource.params( "layers" );
      layerIds.append( sublayerIds );
      foreach ( const QString& id, sublayerIds )
      {
        layerMap.insert( id, rlayer->id() );
      }
    }
  }
  if ( layerIds.isEmpty() )
  {
    return;
  }

  QUrl identifyUrl( QSettings().value( "vbs/identifyurl", "" ).toString() );
  identifyUrl.addQueryItem( "geometryType", "esriGeometryPoint" );
  identifyUrl.addQueryItem( "geometry", QString( "%1,%2" ).arg( worldPos.x(), 0, 'f', 10 ).arg( worldPos.y(), 0, 'f', 10 ) );
  identifyUrl.addQueryItem( "imageDisplay", QString( "%1,%2,%3" ).arg( canvas->width() ).arg( canvas->height() ).arg( canvas->mapSettings().outputDpi() ) );
  identifyUrl.addQueryItem( "mapExtent", QString( "%1,%2,%3,%4" ).arg( worldExtent.xMinimum(), 0, 'f', 10 )
                            .arg( worldExtent.yMinimum(), 0, 'f', 10 )
                            .arg( worldExtent.xMaximum(), 0, 'f', 10 )
                            .arg( worldExtent.yMaximum(), 0, 'f', 10 ) );
  identifyUrl.addQueryItem( "tolerance", "15" );
  identifyUrl.addQueryItem( "layers", layerIds.join( "," ) );

  QNetworkRequest req( identifyUrl );
  instance()->mIdentifyReply = QgsNetworkAccessManager::instance()->get( req );
  instance()->mIdentifyReply->setProperty( "layerMap", layerMap );
  instance()->mTimeoutTimer = new QTimer( instance()->mIdentifyReply );
  instance()->mTimeoutTimer->setSingleShot( true );
  connect( instance()->mIdentifyReply, SIGNAL( finished() ), instance(), SLOT( replyFinished() ) );
  connect( instance()->mTimeoutTimer, SIGNAL( timeout() ), instance(), SLOT( replyFinished() ) );
  instance()->mTimeoutTimer->start( 4000 );
}

void QgsVBSRasterIdentify::replyFinished()
{
  if ( !mIdentifyReply )
  {
    return;
  }

  if ( mDialog )
    mDialog->close();

  QVariantList results;
  if ( mIdentifyReply->error() == QNetworkReply::NoError && mTimeoutTimer->isActive() )
  {
    results = QJson::Parser().parse( mIdentifyReply->readAll() ).toMap()["results"].toList();
  }
  else
  {
    QgisApp::instance()->messageBar()->pushCritical( tr( "Identify failed" ), tr( "The request timed out or failed." ) );
  }

  QMap<QString, QVariant> layerMap = mIdentifyReply->property( "layerMap" ).toMap();
  mDialog = new QgsVBSRasterIdentifyResultDialog( results, layerMap );
  mDialog->show();

  mIdentifyReply->deleteLater();
  mIdentifyReply = 0;
  mTimeoutTimer = 0;
}

QgsVBSRasterIdentify* QgsVBSRasterIdentify::instance()
{
  static QgsVBSRasterIdentify sInstance;
  return &sInstance;
}


const int QgsVBSRasterIdentifyResultDialog::sGeometryRole = Qt::UserRole + 1;
const int QgsVBSRasterIdentifyResultDialog::sGeometryTypeRole = Qt::UserRole + 2;

QgsVBSRasterIdentifyResultDialog::QgsVBSRasterIdentifyResultDialog( const QVariantList &results, const QMap<QString, QVariant> &layerMap )
    : QDialog( QgisApp::instance() ), mRubberBand( 0 ), mPin( 0 )
{
  setWindowTitle( tr( "Identify results" ) );
  setAttribute( Qt::WA_DeleteOnClose );
  setLayout( new QVBoxLayout() );
  resize( 480, 480 );

  QTreeWidget* treeWidget = new QTreeWidget( this );
  treeWidget->setColumnCount( 2 );
  treeWidget->header()->setStretchLastSection( true );
  treeWidget->header()->resizeSection( 0, 200 );
  treeWidget->setHeaderLabels( QStringList() << tr( "Attribute" ) << tr( "Value" ) );
  treeWidget->setDropIndicatorShown( false );
  connect( treeWidget, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( onItemClicked( QTreeWidgetItem*, int ) ) );
  layout()->addWidget( treeWidget );
  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
  connect( bbox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  layout()->addWidget( bbox );

  QList<QString> layersWithResults;
  QMap<QString, QTreeWidgetItem*> layerTreeItemMap;
  for ( int i = 0, n = results.size(); i < n; ++i )
  {
    QVariantMap result = results[i].toMap();
    QString layerId = result["layerId"].toString();
    if ( !layerMap.contains( layerId ) )
    {
      continue;
    }
    layersWithResults.append( layerId );
    QString qgisLayerId = layerMap[layerId].toString();
    if ( !layerTreeItemMap.contains( layerId ) )
    {
      QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( qgisLayerId );
      if ( !layer )
      {
        layerTreeItemMap.insert( layerId, 0 );
      }
      else
      {
        QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << layer->name() );
        item->setFirstColumnSpanned( true );
        QFont font = item->font( 0 );
        font.setBold( true );
        item->setFont( 0, font );
        layerTreeItemMap.insert( layerId, item );
        treeWidget->invisibleRootItem()->addChild( item );
      }
    }
    QTreeWidgetItem* parent = layerTreeItemMap[layerId];
    if ( !parent )
    {
      continue;
    }
    QTreeWidgetItem* resultItem = new QTreeWidgetItem( QStringList() << "" );
    resultItem->setData( 0, sGeometryRole, result["geometry"].toMap() );
    resultItem->setData( 0, sGeometryTypeRole, result["geometryType"].toString() );

    resultItem->setFirstColumnSpanned( true );
    QString displayFieldName = result["displayFieldName"].toString();
    QVariantMap attributes = result["attributes"].toMap();
    foreach ( const QString& key, attributes.keys() )
    {
      QString value = attributes[key].toString();
      QTreeWidgetItem* attrItem = new QTreeWidgetItem( QStringList() << key << value );
      if ( key == displayFieldName )
        resultItem->setText( 0, value );
      resultItem->addChild( attrItem );
    }
    parent->addChild( resultItem );
  }
  foreach ( const QString& layerId, layerMap.keys() )
  {
    QString qgisLayerId = layerMap[layerId].toString();
    QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( qgisLayerId );
    if ( !layersWithResults.contains( layerId ) )
    {
      QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << layer->name() );
      item->setFirstColumnSpanned( true );
      QFont font = item->font( 0 );
      font.setBold( true );
      item->setFont( 0, font );
      layerTreeItemMap.insert( layerId, item );
      treeWidget->invisibleRootItem()->addChild( item );
      item->addChild( new QTreeWidgetItem( QStringList() << tr( "No results" ) ) );
    }
  }

  treeWidget->expandAll();
}

QgsVBSRasterIdentifyResultDialog::~QgsVBSRasterIdentifyResultDialog()
{
  delete mRubberBand.data();
  delete mPin.data();
}

void QgsVBSRasterIdentifyResultDialog::onItemClicked( QTreeWidgetItem *item, int /*col*/ )
{
  delete mRubberBand.data();
  mRubberBand = 0;
  delete mPin.data();
  mPin = 0;
  QVariantMap geometryMap;
  while ( item && ( geometryMap = item->data( 0, sGeometryRole ).toMap() ).isEmpty() )
    item = item->parent();
  if ( geometryMap.isEmpty() )
    return;
  QString geometryType = item->data( 0, sGeometryTypeRole ).toString();
  QgsCoordinateReferenceSystem crs;
  QgsAbstractGeometryV2* geometryV2 = QgsArcGisRestUtils::parseEsriGeoJSON( geometryMap, geometryType, false, false, &crs );
  if ( !geometryV2 )
    return;
  QgsMapCanvas* canvas = QgisApp::instance()->mapCanvas();
  QGis::GeometryType geomType = ( QGis::GeometryType ) QgsWKBTypes::geometryType( geometryV2->wkbType() ) ;
  if ( geomType == QGis::Point && dynamic_cast<QgsPointV2*>( geometryV2 ) )
  {
    mPin = new QgsPinAnnotationItem( canvas );
    QgsPointV2* p = static_cast<QgsPointV2*>( geometryV2 );
    mPin = new QgsPinAnnotationItem( canvas );
    mPin->setMapPosition( QgsPoint( p->x(), p->y() ), crs );
    mPin->setFilePath( ":/images/themes/default/pin_blue.svg" );
    delete geometryV2;
  }
  else
  {
    geometryV2->transform( *QgsCoordinateTransformCache::instance()->transform( crs.authid(), canvas->mapSettings().destinationCrs().authid() ) );
    mRubberBand = new QgsGeometryRubberBand( canvas, geomType );
    mRubberBand->setGeometry( geometryV2 );
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_NONE );
  }
}
