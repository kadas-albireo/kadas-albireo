/***************************************************************************
    qgsgpsrouteeditor.cpp - GSP route editor
     --------------------------------------
    Date                 : Sep 2015
    Copyright            : (C) 2015 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgpsrouteeditor.h"
#include "qgisapp.h"
#include "qgsapplayertreeviewmenuprovider.h"
#include "qgscolorbuttonv2.h"
#include "qgscurvev2.h"
#include "qgslayertreeview.h"
#include "qgslinestringv2.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptooladdfeature.h"
#include "qgsmaptoolsredlining.h"
#include "qgsredlininglayer.h"
#include "qgsredliningtextdialog.h"
#include "qgsrubberband.h"
#include "qgssymbollayerv2utils.h"
#include "qgsproject.h"

#include <QDomDocument>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>

QgsGPSRouteEditor::QgsGPSRouteEditor( QgisApp* app )
    : QObject( app ), mApp( app ), mLayer( 0 ), mLayerRefCount( 0 )
{

  QToolBar* gpseditorToolbar = mApp->addToolBar( tr( "GPS Route Editor" ) );

  QAction* actionNewPoint = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Waypoint" ), this );
  actionNewPoint->setCheckable( true );
  connect( actionNewPoint, SIGNAL( triggered( bool ) ), this, SLOT( newPoint() ) );

  QAction* actionNewLine = new QAction( QIcon( ":/images/themes/default/redlining_line.svg" ), tr( "Route" ), this );
  actionNewLine->setCheckable( true );
  connect( actionNewLine, SIGNAL( triggered( bool ) ), this, SLOT( newLine() ) );

  mBtnNewObject = new QToolButton();
  mBtnNewObject->setToolTip( tr( "New Object" ) );
  QMenu* menuNewObject = new QMenu();
  menuNewObject->addAction( actionNewPoint );
  menuNewObject->addAction( actionNewLine );
  mBtnNewObject->setMenu( menuNewObject );
  mBtnNewObject->setPopupMode( QToolButton::MenuButtonPopup );
  mBtnNewObject->setDefaultAction( actionNewPoint );
  connect( menuNewObject, SIGNAL( triggered( QAction* ) ), mBtnNewObject, SLOT( setDefaultAction( QAction* ) ) );
  gpseditorToolbar->addWidget( mBtnNewObject );

  mActionEditObject = new QAction( QIcon( ":/images/themes/default/mActionNodeTool.png" ), QString(), this );
  mActionEditObject->setToolTip( tr( "Edit Object" ) );
  mActionEditObject->setCheckable( true );
  gpseditorToolbar->addAction( mActionEditObject );
  connect( mActionEditObject, SIGNAL( triggered( bool ) ), this, SLOT( editObject() ) );

  connect( mApp, SIGNAL( newProject() ), this, SLOT( clearLayer() ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProject( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProject( QDomDocument& ) ) );

  mActionImportGpx = new QAction( tr( "Import from GPX" ), this );
  connect( mActionImportGpx, SIGNAL( triggered( bool ) ), this, SLOT( importGpx() ) );
  mActionExportGpx = new QAction( tr( "Export to GPX" ), this );
  connect( mActionExportGpx, SIGNAL( triggered( bool ) ), this, SLOT( exportGpx() ) );
  dynamic_cast<QgsAppLayerTreeViewMenuProvider*>( mApp->layerTreeView()->menuProvider() )->addLegendLayerAction( mActionImportGpx, "", "gpxImport", QgsMapLayer::RedliningLayer, false );
  dynamic_cast<QgsAppLayerTreeViewMenuProvider*>( mApp->layerTreeView()->menuProvider() )->addLegendLayerAction( mActionExportGpx, "", "gpxExport", QgsMapLayer::RedliningLayer, false );
}

QgsGPSRouteEditor::~QgsGPSRouteEditor()
{
  dynamic_cast<QgsAppLayerTreeViewMenuProvider*>( mApp->layerTreeView()->menuProvider() )->removeLegendLayerAction( mActionImportGpx );
  dynamic_cast<QgsAppLayerTreeViewMenuProvider*>( mApp->layerTreeView()->menuProvider() )->removeLegendLayerAction( mActionExportGpx );
}

QgsRedliningLayer* QgsGPSRouteEditor::getOrCreateLayer()
{
  if ( mLayer )
  {
    return mLayer;
  }
  mLayer = new QgsRedliningLayer( tr( "GPS Routes" ) );
  QgsMapLayerRegistry::instance()->addMapLayer( mLayer, true, true );
  mLayerRefCount = 0;

  // QueuedConnection to delay execution of the slot until the signal-emitting function has exited,
  // since otherwise the undo stack becomes corrupted (featureChanged change inserted before featureAdded change)
  connect( mLayer, SIGNAL( featureAdded( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ), Qt::QueuedConnection );
  connect( mLayer.data(), SIGNAL( destroyed( QObject* ) ), this, SLOT( clearLayer() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWillBeRemoved( QString ) ), this, SLOT( removeLegendActions( QString ) ) );
  dynamic_cast<QgsAppLayerTreeViewMenuProvider*>( mApp->layerTreeView()->menuProvider() )->addLegendLayerActionForLayer( mActionImportGpx, mLayer );
  dynamic_cast<QgsAppLayerTreeViewMenuProvider*>( mApp->layerTreeView()->menuProvider() )->addLegendLayerActionForLayer( mActionExportGpx, mLayer );

  return mLayer;
}

void QgsGPSRouteEditor::removeLegendActions( const QString& layerId )
{
  if ( mLayer && layerId == mLayer->id() )
  {
    dynamic_cast<QgsAppLayerTreeViewMenuProvider*>( mApp->layerTreeView()->menuProvider() )->removeLegendLayerActionsForLayer( mLayer );
  }
}

void QgsGPSRouteEditor::clearLayer()
{
  mLayer = 0;
  mLayerRefCount = 0;
  deactivateTool();
}

void QgsGPSRouteEditor::editObject()
{
  QgsRedliningEditTool* tool = new QgsRedliningEditTool( mApp->mapCanvas(), getOrCreateLayer() );
  connect( this, SIGNAL( featureStyleChanged() ), tool, SLOT( onStyleChanged() ) );
  connect( tool, SIGNAL( featureSelected( QgsFeatureId ) ), this, SLOT( syncStyleWidgets( QgsFeatureId ) ) );
  connect( tool, SIGNAL( updateFeatureStyle( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ) );
  activateTool( tool, qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsGPSRouteEditor::newPoint()
{
  activateTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "circle" ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsGPSRouteEditor::newLine()
{
  activateTool( new QgsMapToolAddFeature( mApp->mapCanvas(), QgsMapToolCapture::CaptureLine, QGis::WKBLineString ), qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsGPSRouteEditor::activateTool( QgsMapTool *tool, QAction* action )
{
  tool->setAction( action );
  connect( tool, SIGNAL( deactivated() ), this, SLOT( deactivateTool() ) );
  if ( mLayerRefCount == 0 )
  {
    mApp->mapCanvas()->setCurrentLayer( getOrCreateLayer() );
    mLayer->startEditing();
  }
  ++mLayerRefCount;
  mApp->mapCanvas()->setMapTool( tool );
  mRedliningTool = tool;
}

void QgsGPSRouteEditor::deactivateTool()
{
  if ( mRedliningTool )
  {
    if ( mLayer )
    {
      --mLayerRefCount;
      if ( mLayerRefCount == 0 )
      {
        mLayer->commitChanges();
        if ( mApp->mapCanvas()->currentLayer() == mLayer )
        {
          mApp->mapCanvas()->setCurrentLayer( 0 );
        }
      }
    }
    mRedliningTool->deleteLater();
  }
}

void QgsGPSRouteEditor::updateFeatureStyle( const QgsFeatureId &fid )
{
  if ( !mLayer )
  {
    return;
  }
  QgsFeature f;
  if ( !mLayer->getFeatures( QgsFeatureRequest( fid ) ).nextFeature( f ) )
  {
    return;
  }
  const QgsFields& fields = mLayer->pendingFields();
  mLayer->changeAttributeValue( fid, fields.indexFromName( "size" ), 2 );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline" ), QgsSymbolLayerV2Utils::encodeColor( QColor( Qt::yellow ) ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill" ), QgsSymbolLayerV2Utils::encodeColor( QColor( Qt::green ) ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline_style" ), QgsSymbolLayerV2Utils::encodePenStyle( static_cast<Qt::PenStyle>( Qt::SolidLine ) ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill_style" ), QgsSymbolLayerV2Utils::encodeBrushStyle( static_cast<Qt::BrushStyle>( Qt::SolidPattern ) ) );
  mApp->mapCanvas()->clearCache( mLayer->id() );
  mApp->mapCanvas()->refresh();
}

void QgsGPSRouteEditor::readProject( const QDomDocument& doc )
{
  clearLayer();
  QDomNodeList nl = doc.elementsByTagName( "GpsRoutes" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  getOrCreateLayer()->read( nl.at( 0 ).toElement() );
}

void QgsGPSRouteEditor::writeProject( QDomDocument& doc )
{
  if ( !mLayer )
  {
    return;
  }

  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement redliningElem = doc.createElement( "GpsRoutes" );
  mLayer->write( redliningElem );
  qgisElem.appendChild( redliningElem );
}

void QgsGPSRouteEditor::importGpx()
{
  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QString filename = QFileDialog::getOpenFileName( mApp, tr( "Import GPX" ), lastProjectDir, tr( "GPX Files (*.gpx" ) );
  if ( filename.isEmpty() )
  {
    return;
  }

  QFile file( filename );
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    QMessageBox::critical( mApp, tr( "Error" ), tr( "Cannot open file for reading: %1" ).arg( filename ) );
    return;
  }

  int nWpts = 0;
  int nRtes = 0;

  QDomDocument doc;
  doc.setContent( &file );
  QDomNodeList wpts = doc.elementsByTagName( "wpt" );
  for ( int i = 0, n = wpts.size(); i < n; ++i )
  {
    QDomElement wptEl = wpts.at( i ).toElement();
    double lat = wptEl.attribute( "lat" ).toDouble();
    double lon = wptEl.attribute( "lon" ).toDouble();
    QString name = wptEl.firstChildElement( "name" ).text();
    QString flags = "symbol=circle,w=5*\"size\",h=5*\"size\",r=0";
    mLayer->addShape( new QgsGeometry( new QgsPointV2( lon, lat ) ), Qt::yellow, Qt::green, 1, Qt::SolidLine, Qt::SolidPattern, flags, name );
    ++nWpts;
  }
  QDomNodeList rtes = doc.elementsByTagName( "rte" );
  for ( int i = 0, n = rtes.size(); i < n; ++i )
  {
    QDomElement rteEl = rtes.at( i ).toElement();
    QString name = rteEl.firstChildElement( "name" ).text();
    QList<QgsPointV2> pts;
    QDomNodeList rtepts = rteEl.elementsByTagName( "rtept" );
    for ( int j = 0, m = rtepts.size(); j < m; ++j )
    {
      QDomElement rteptEl = rtepts.at( j ).toElement();
      double lat = rteptEl.attribute( "lat" ).toDouble();
      double lon = rteptEl.attribute( "lon" ).toDouble();
      pts.append( QgsPointV2( lon, lat ) );
    }
    QgsLineStringV2* line = new QgsLineStringV2();
    line->setPoints( pts );
    mLayer->addShape( new QgsGeometry( line ), Qt::yellow, Qt::green, 1, Qt::SolidLine, Qt::SolidPattern, QString(), name );
    ++nRtes;
  }
  mApp->mapCanvas()->clearCache( mLayer->id() );
  mApp->mapCanvas()->refresh();
  QMessageBox::information( mApp, tr( "GPX Import" ), tr( "%1 waypoints and %2 routes were read." ).arg( nWpts ).arg( nRtes ) );
}

void QgsGPSRouteEditor::exportGpx()
{
  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QString filename = QFileDialog::getSaveFileName( mApp, tr( "Export to GPX" ), lastProjectDir, tr( "GPX Files (*.gpx" ) );
  if ( filename.isEmpty() )
  {
    return;
  }

  QFile file( filename );
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    QMessageBox::critical( mApp, tr( "Error" ), tr( "Cannot open file for writing: %1" ).arg( filename ) );
    return;
  }

  QDomDocument doc;
  QDomElement gpxEl = doc.createElement( "gpx" );
  gpxEl.setAttribute( "version", "1.1" );
  gpxEl.setAttribute( "creator", "qgis" );
  doc.appendChild( gpxEl );

  // Write waypoints (all point geometries)
  QgsFeatureIterator fit = mLayer->getFeatures();
  QgsFeature feature;
  while ( fit.nextFeature( feature ) )
  {
    if ( feature.geometry()->type() == QGis::Point )
    {
      QgsPointV2* pt = static_cast<QgsPointV2*>( feature.geometry()->geometry() );
      QDomElement wptEl = doc.createElement( "wpt" );
      wptEl.setAttribute( "lon", pt->x() );
      wptEl.setAttribute( "lat", pt->y() );
      QDomElement nameEl = doc.createElement( "name" );
      nameEl.setNodeValue( feature.attribute( "tooltip" ).toString() );
      wptEl.appendChild( nameEl );
      gpxEl.appendChild( wptEl );
    }
  }

  // Write routes (all line geometries)
  int routeNum = 0;
  fit = mLayer->getFeatures();
  while ( fit.nextFeature( feature ) )
  {
    if ( feature.geometry()->type() == QGis::Line )
    {
      QgsCurveV2* curve = static_cast<QgsCurveV2*>( feature.geometry()->geometry() );
      QDomElement rteEl = doc.createElement( "rte" );
      QDomElement nameEl = doc.createElement( "name" );
      nameEl.setNodeValue( feature.attribute( "tooltip" ).toString() );
      rteEl.appendChild( nameEl );
      QDomElement numberEl = doc.createElement( "number" );
      numberEl.setNodeValue( QString::number( ++routeNum ) );
      rteEl.appendChild( numberEl );
      QList<QgsPointV2> pts;
      curve->points( pts );
      foreach ( const QgsPointV2& pt, pts )
      {
        QDomElement rteptEl = doc.createElement( "rtept" );
        rteptEl.setAttribute( "lon", pt.x() );
        rteptEl.setAttribute( "lat", pt.y() );
        rteEl.appendChild( rteptEl );
      }
      gpxEl.appendChild( rteEl );
    }
  }

  file.write( doc.toString().toLocal8Bit() );
}
