/***************************************************************************
    qgsmapidentifydialog.cpp
    ------------------------
    begin                : Aprile 2018
    copyright            : (C) 2018 by Sandro Mani
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
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTreeWidget>
#include <QVBoxLayout>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
# include <qjson/parser.h>
#else
# include <QJsonDocument>
# include <QJsonObject>
#endif

#include "qgsabstractgeometryv2.h"
#include "qgsarcgisrestutils.h"
#include "qgscrscache.h"
#include "qgsdatasourceuri.h"
#include "qgsgeometry.h"
#include "qgsgeometryrubberband.h"
#include "qgsmapcanvas.h"
#include "qgsmapidentifydialog.h"
#include "qgsmaplayerregistry.h"
#include "qgsnetworkaccessmanager.h"
#include "qgspinannotationitem.h"
#include "qgsrasterlayer.h"
#include "qgsrendercontext.h"
#include "qgsrendererv2.h"
#include "qgsvectorlayer.h"


const int QgsMapIdentifyDialog::sGeometryRole = Qt::UserRole + 1;
const int QgsMapIdentifyDialog::sGeometryCrsRole = Qt::UserRole + 2;

QgsMapIdentifyDialog::QgsMapIdentifyDialog( QgsMapCanvas *canvas, const QgsPoint &mapPos )
    : mCanvas( canvas )
{
  setWindowTitle( tr( "Identify results" ) );
  setAttribute( Qt::WA_DeleteOnClose );
  setLayout( new QVBoxLayout() );
  resize( 480, 480 );

  mTreeWidget = new QTreeWidget( this );
  mTreeWidget->setColumnCount( 2 );
  mTreeWidget->header()->setStretchLastSection( true );
  mTreeWidget->header()->resizeSection( 0, 200 );
  mTreeWidget->setHeaderLabels( QStringList() << "" << "" );
  mTreeWidget->setDropIndicatorShown( false );
  mTreeWidget->setVerticalScrollMode( QTreeWidget::ScrollPerPixel );
  layout()->addWidget( mTreeWidget );
  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
  connect( bbox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  layout()->addWidget( bbox );

  connect( mTreeWidget, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( onItemClicked( QTreeWidgetItem*, int ) ) );

  collectInfo( mapPos );

  mPosPin = new QgsPinAnnotationItem( mCanvas );
  mPosPin->setMapPosition( QgsPoint( mapPos.x(), mapPos.y() ) );
  mPosPin->setFilePath( ":/images/themes/default/pin_red.svg" );
}

QgsMapIdentifyDialog::~QgsMapIdentifyDialog()
{
  delete mRubberBand.data();
  delete mPin.data();
  delete mPosPin.data();
  qDeleteAll( mGeometries );
  if ( mRasterIdentifyReply )
  {
    mRasterIdentifyReply->abort();
    mRasterIdentifyReply->deleteLater();
  }
}

void QgsMapIdentifyDialog::onItemClicked( QTreeWidgetItem *item, int /*col*/ )
{
  delete mRubberBand.data();
  mRubberBand = 0;
  delete mPin.data();
  mPin = 0;

  while ( item && item->data( 0, sGeometryRole ).isNull() )
    item = item->parent();
  if ( !item )
    return;

  int idx = item->data( 0, sGeometryRole ).toInt();
  QgsAbstractGeometryV2* geom = mGeometries[idx];
  QgsCoordinateReferenceSystem crs( item->data( 0, sGeometryCrsRole ).toString() );
  if ( crs != mCanvas->mapSettings().destinationCrs() )
  {
    geom->transform( *QgsCoordinateTransformCache::instance()->transform( crs.authid(), mCanvas->mapSettings().destinationCrs().authid() ) );
    item->setData( 0, sGeometryCrsRole, mCanvas->mapSettings().destinationCrs().authid() );
  }
  if ( dynamic_cast<QgsPointV2*>( geom ) )
  {
    QgsPointV2* p = static_cast<QgsPointV2*>( geom );
    mPin = new QgsPinAnnotationItem( mCanvas );
    mPin->setMapPosition( QgsPoint( p->x(), p->y() ), crs );
    mPin->setFilePath( ":/images/themes/default/pin_blue.svg" );
  }
  else
  {
    mRubberBand = new QgsGeometryRubberBand( mCanvas, static_cast<QGis::GeometryType>( QgsWKBTypes::geometryType( geom->wkbType() ) ) );
    mRubberBand->setGeometry( geom->clone() );
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_NONE );
    mRubberBand->setFillColor( QColor( 0, 0, 255, 127 ) );
    mRubberBand->setOutlineColor( QColor( 0, 0, 255 ) );
  }
}

void QgsMapIdentifyDialog::collectInfo( const QgsPoint &mapPos )
{
  // Prepare for raster layers
  const QgsCoordinateReferenceSystem& mapCrs = mCanvas->mapSettings().destinationCrs();
  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mapCrs.authid(), "EPSG:4326" );
  QgsPoint worldPos = crst->transform( mapPos );
  QgsRectangle worldExtent = crst->transform( mCanvas->extent() );
  QStringList rlayerIds;
  QMap<QString, QVariant> rlayerMap;

  // Prepare for vector layers
  QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( mCanvas->mapSettings() );
  double radiusmm = QSettings().value( "/Map/searchRadiusMM", QGis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  radiusmm = radiusmm > 0 ? radiusmm : QGis::DEFAULT_SEARCH_RADIUS_MM;
  double radiusmu = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
  QgsRectangle filterRect;
  filterRect.setXMinimum( mapPos.x() - radiusmu );
  filterRect.setXMaximum( mapPos.x() + radiusmu );
  filterRect.setYMinimum( mapPos.y() - radiusmu );
  filterRect.setYMaximum( mapPos.y() + radiusmu );

  foreach ( QgsMapLayer* layer, mCanvas->layers() )
  {
    if ( dynamic_cast<QgsPluginLayer*>( layer ) )
    {
      QgsPluginLayer* pluginLayer = static_cast<QgsPluginLayer*>( layer );
      QList<QgsPluginLayer::IdentifyResult> results = pluginLayer->identify( mapPos, mCanvas->mapSettings() );
      if ( !results.isEmpty() )
      {
        addPluginLayerResults( pluginLayer, results );
      }
    }
    else if ( dynamic_cast<QgsRasterLayer*>( layer ) )
    {
      const QgsRasterLayer* rlayer = static_cast<const QgsRasterLayer*>( layer );

      // Detect ArcGIS Rest MapServer layers
      QgsDataSourceURI dataSource( rlayer->dataProvider()->dataSourceUri() );
      QStringList urlParts = dataSource.param( "url" ).split( "/", QString::SkipEmptyParts );
      int nParts = urlParts.size();
      if ( nParts > 4 && urlParts[nParts - 1] == "MapServer" && urlParts[nParts - 4] == "services" )
      {
        rlayerIds.append( urlParts[nParts - 3] + ":" + urlParts[nParts - 2] );
        rlayerMap.insert( urlParts[nParts - 2], rlayer->id() );
      }

      // Detect geo.admin.ch layers
      dataSource.setEncodedUri( rlayer->dataProvider()->dataSourceUri() );
      if ( dataSource.param( "url" ).contains( "geo.admin.ch" ) )
      {
        QStringList sublayerIds = dataSource.params( "layers" );
        rlayerIds.append( sublayerIds );
        foreach ( const QString& id, sublayerIds )
        {
          rlayerMap.insert( id, rlayer->id() );
        }
      }
    }
    else if ( dynamic_cast<QgsVectorLayer*>( layer ) )
    {
      QgsVectorLayer* vlayer = static_cast<QgsVectorLayer*>( layer );
      if ( vlayer->hasScaleBasedVisibility() &&
           ( vlayer->minimumScale() > mCanvas->mapSettings().scale() ||
             vlayer->maximumScale() <= mCanvas->mapSettings().scale() ) )
      {
        continue;
      }

      QgsFeatureRendererV2* renderer = vlayer->rendererV2();
      bool filteredRendering = false;
      if ( renderer && renderer->capabilities() & QgsFeatureRendererV2::ScaleDependent )
      {
        // setup scale for scale dependent visibility (rule based)
        renderer->startRender( renderContext, vlayer->pendingFields() );
        filteredRendering = renderer->capabilities() & QgsFeatureRendererV2::Filter;
      }

      QgsRectangle layerFilterRect = mCanvas->mapSettings().mapToLayerCoordinates( vlayer, filterRect );
      QgsFeatureIterator fit = vlayer->getFeatures( QgsFeatureRequest( layerFilterRect ).setFlags( QgsFeatureRequest::ExactIntersect ) );
      QgsFeature feature;
      while ( fit.nextFeature( feature ) )
      {
        if ( filteredRendering && !renderer->willRenderFeature( feature ) )
        {
          continue;
        }
        addVectorLayerResult( vlayer, feature );
      }
      if ( renderer && renderer->capabilities() & QgsFeatureRendererV2::ScaleDependent )
      {
        renderer->stopRender( renderContext );
      }
    }
  }

  // Raster layer query
  if ( !rlayerIds.isEmpty() )
  {
    QUrl identifyUrl( QSettings().value( "vbs/identifyurl", "" ).toString() );
    identifyUrl.addQueryItem( "geometryType", "esriGeometryPoint" );
    identifyUrl.addQueryItem( "geometry", QString( "%1,%2" ).arg( worldPos.x(), 0, 'f', 10 ).arg( worldPos.y(), 0, 'f', 10 ) );
    identifyUrl.addQueryItem( "imageDisplay", QString( "%1,%2,%3" ).arg( mCanvas->width() ).arg( mCanvas->height() ).arg( mCanvas->mapSettings().outputDpi() ) );
    identifyUrl.addQueryItem( "mapExtent", QString( "%1,%2,%3,%4" ).arg( worldExtent.xMinimum(), 0, 'f', 10 )
                              .arg( worldExtent.yMinimum(), 0, 'f', 10 )
                              .arg( worldExtent.xMaximum(), 0, 'f', 10 )
                              .arg( worldExtent.yMaximum(), 0, 'f', 10 ) );
    identifyUrl.addQueryItem( "tolerance", "15" );
    identifyUrl.addQueryItem( "layers", rlayerIds.join( "," ) );

    QNetworkRequest req( identifyUrl );
    mRasterIdentifyReply = QgsNetworkAccessManager::instance()->get( req );
    mRasterIdentifyReply->setProperty( "layerMap", rlayerMap );
    mTimeoutTimer = new QTimer( mRasterIdentifyReply );
    mTimeoutTimer->setSingleShot( true );
    connect( mRasterIdentifyReply, SIGNAL( finished() ), this, SLOT( rasterIdentifyFinished() ) );
    connect( mTimeoutTimer, SIGNAL( timeout() ), this, SLOT( rasterIdentifyFinished() ) );
    mTimeoutTimer->start( 4000 );
  }
}

void QgsMapIdentifyDialog::addPluginLayerResults( QgsPluginLayer* pLayer, const QList<QgsPluginLayer::IdentifyResult>& results )
{
  if ( !mLayerTreeItemMap[pLayer->id()] )
  {
    QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << pLayer->name() );
    item->setFirstColumnSpanned( true );
    QFont font = item->font( 0 );
    font.setBold( true );
    item->setFont( 0, font );
    mTreeWidget->invisibleRootItem()->addChild( item );
    mLayerTreeItemMap.insert( pLayer->id(), item );
    item->setExpanded( true );
  }

for ( const QgsPluginLayer::IdentifyResult& result : results )
  {
    QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << result.featureName() );

    QgsAbstractGeometryV2* geomv2 = result.geometry().geometry()->clone();
    mGeometries.append( geomv2 );
    item->setData( 0, sGeometryRole, mGeometries.size() - 1 );
    item->setData( 0, sGeometryCrsRole, pLayer->crs().authid() );
    mLayerTreeItemMap[pLayer->id()]->addChild( item );

    for ( auto it = result.attributes().begin(), itEnd = result.attributes().end(); it != itEnd; ++it )
    {
      item->addChild( new QTreeWidgetItem( QStringList() << it.key() << it.value().toString() ) );
    }
    item->setExpanded( true );
  }
}

void QgsMapIdentifyDialog::addVectorLayerResult( QgsVectorLayer *vLayer, const QgsFeature &feature )
{
  if ( !mLayerTreeItemMap[vLayer->id()] )
  {
    QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << vLayer->name() );
    item->setFirstColumnSpanned( true );
    QFont font = item->font( 0 );
    font.setBold( true );
    item->setFont( 0, font );
    mTreeWidget->invisibleRootItem()->addChild( item );
    mLayerTreeItemMap.insert( vLayer->id(), item );
    item->setExpanded( true );
  }

  QString label = vLayer->displayField().isEmpty() ? QString::number( feature.id() ) : QString( "%1 [%2]" ).arg( feature.attribute( vLayer->displayField() ).toString() ).arg( feature.id() );
  QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << label );
  QgsAbstractGeometryV2* geomv2 = feature.geometry()->geometry()->clone();
  mGeometries.append( geomv2 );
  item->setData( 0, sGeometryRole, mGeometries.size() - 1 );
  item->setData( 0, sGeometryCrsRole, vLayer->crs().authid() );
  mLayerTreeItemMap[vLayer->id()]->addChild( item );

  if ( vLayer->type() == QgsMapLayer::RedliningLayer )
  {
    QStringList attributes = feature.attribute( "attributes" ).toString().split( QRegExp( "&(?!amp;)" ) );
  for ( const QString& attribute : attributes )
    {
      QStringList pair = attribute.split( "=" );
      item->addChild( new QTreeWidgetItem( pair ) );
    }
  }
  else
  {
    QgsAttributes attributes = feature.attributes();
    for ( int i = 0, n = attributes.size(); i < n; ++i )
    {
      QString attribName = vLayer->attributeDisplayName( i );
      QStringList pair = QStringList() << attribName << attributes.at( i ).toString();
      item->addChild( new QTreeWidgetItem( pair ) );
    }
  }
  item->setExpanded( true );
}

void QgsMapIdentifyDialog::rasterIdentifyFinished()
{
  if ( !mRasterIdentifyReply )
  {
    return;
  }

  QVariantList results;
  if ( mRasterIdentifyReply->error() == QNetworkReply::NoError && mTimeoutTimer->isActive() )
  {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    results = QJson::Parser().parse( mRasterIdentifyReply->readAll() ).toMap()["results"].toList();
#else
    results = QJsonDocument::fromJson( mRasterIdentifyReply->readAll() ).object().toVariantMap()["results"].toList();
#endif
  }

  QMap<QString, QVariant> layerMap = mRasterIdentifyReply->property( "layerMap" ).toMap();

  mRasterIdentifyReply->deleteLater();
  mRasterIdentifyReply = nullptr;
  mTimeoutTimer = nullptr;

  // Add results to tree
  for ( int i = 0, n = results.size(); i < n; ++i )
  {
    QVariantMap result = results[i].toMap();
    QString layerId = result["layerId"].toString();
    if ( !layerMap.contains( layerId ) )
    {
      continue;
    }
    QString qgisLayerId = layerMap[layerId].toString();
    if ( !mLayerTreeItemMap.contains( layerId ) )
    {
      QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( qgisLayerId );
      if ( !layer )
      {
        mLayerTreeItemMap.insert( layerId, 0 );
      }
      else
      {
        QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << layer->name() );
        item->setFirstColumnSpanned( true );
        QFont font = item->font( 0 );
        font.setBold( true );
        item->setFont( 0, font );
        mLayerTreeItemMap.insert( layerId, item );
        mTreeWidget->invisibleRootItem()->addChild( item );
        item->setExpanded( true );
      }
    }
    QTreeWidgetItem* parent = mLayerTreeItemMap[layerId];
    if ( !parent )
    {
      continue;
    }
    QTreeWidgetItem* resultItem = new QTreeWidgetItem( QStringList() << "" );
    QgsCoordinateReferenceSystem crs;
    QgsAbstractGeometryV2* geometryV2 = QgsArcGisRestUtils::parseEsriGeoJSON( result["geometry"].toMap(), result["geometryType"].toString(), false, false, &crs );
    mGeometries.append( geometryV2 );

    resultItem->setData( 0, sGeometryRole, mGeometries.size() - 1 );
    resultItem->setData( 0, sGeometryCrsRole, crs.authid() );

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
    resultItem->setExpanded( true );
  }
}

