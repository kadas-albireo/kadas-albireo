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
#include <QInputDialog>
#include <QFileDialog>
#include <QMenu>
#include <QSettings>


int QgsGPSRouteEditor::sFeatureSize = 2;

class QgsGPSRouteEditor::GPXAttribEditor : public QgsRedliningAttributeEditor
{
    bool exec( QgsFeature& feature, QStringList& changedAttributes ) override
    {
      QMap<QString, QString> flagsMap;
      foreach ( const QString& flag, feature.attribute( "flags" ).toString().split( "," ) )
      {
        int pos = flag.indexOf( "=" );
        flagsMap.insert( flag.left( pos ), pos >= 0 ? flag.mid( pos + 1 ) : QString() );
      }
      QString name = QInputDialog::getText( 0, tr( "GPX Attributes" ), tr( "Name:" ), QLineEdit::Normal, feature.attribute( "text" ).toString() );
      if ( name.isEmpty() )
      {
        return false;
      }
      feature.setAttribute( "text", name );
      QFont font;
      flagsMap["family"] = font.family();
      flagsMap["italic"] = QString( "%1" ).arg( font.italic() );
      flagsMap["bold"] = QString( "%1" ).arg( font.bold() );
      flagsMap["rotation"] = QString( "%1" ).arg( 0. );
      flagsMap["fontSize"] = QString( "%1" ).arg( font.pointSize() );

      QString flags;
      foreach ( const QString& key, flagsMap.keys() )
      {
        flags += QString( "%1=%2," ).arg( key ).arg( flagsMap.value( key ) );
      }
      feature.setAttribute( "flags", flags );
      changedAttributes.append( "flags" );
      changedAttributes.append( "text" );
      return true;
    }
};

QgsGPSRouteEditor::QgsGPSRouteEditor( QgisApp* app, QAction *actionCreateWaypoints, QAction *actionCreateRoutes )
    : QObject( app )
    , mApp( app )
    , mActionCreateWaypoints( actionCreateWaypoints )
    , mActionCreateRoutes( actionCreateRoutes )
    , mLayer( 0 )
    , mLayerRefCount( 0 )
{
  mActionEdit = new QAction( this );

  connect( actionCreateWaypoints, SIGNAL( triggered( bool ) ), this, SLOT( createWaypoints( bool ) ) );
  connect( actionCreateRoutes, SIGNAL( triggered( bool ) ), this, SLOT( createRoutes( bool ) ) );
  connect( app, SIGNAL( newProject() ), this, SLOT( clearLayer() ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProject( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProject( QDomDocument& ) ) );
}

QgsRedliningLayer* QgsGPSRouteEditor::getOrCreateLayer()
{
  if ( mLayer )
  {
    return mLayer;
  }
  mLayer = new QgsRedliningLayer( tr( "GPS Routes" ), "EPSG:4326" );
  // Avoid labels overlapping with point symbols
  mLayer->setCustomProperty( "labeling/placement", 1 );
  mLayer->setCustomProperty( "labeling/xOffset", 3 );
  mLayer->setCustomProperty( "labeling/quadOffset", 5 );
  mLayer->setCustomProperty( "labeling/labelOffsetInMapUnits", false );
  QgsMapLayerRegistry::instance()->addMapLayer( mLayer, true, true );
  mLayerRefCount = 0;

  // QueuedConnection to delay execution of the slot until the signal-emitting function has exited,
  // since otherwise the undo stack becomes corrupted (featureChanged change inserted before featureAdded change)
  connect( mLayer, SIGNAL( featureAdded( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ), Qt::QueuedConnection );
  connect( mLayer.data(), SIGNAL( destroyed( QObject* ) ), this, SLOT( clearLayer() ) );

  return mLayer;
}

QgsRedliningLayer* QgsGPSRouteEditor::getLayer() const
{
  return mLayer.data();
}

void QgsGPSRouteEditor::editFeature( const QgsFeature& feature )
{
  QgsRedliningEditTool* tool = new QgsRedliningEditTool( mApp->mapCanvas(), getOrCreateLayer(), new GPXAttribEditor );
  connect( this, SIGNAL( featureStyleChanged() ), tool, SLOT( onStyleChanged() ) );
  connect( tool, SIGNAL( updateFeatureStyle( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ) );
  tool->selectFeature( feature );
  tool->setUnsetOnMiss( true );
  setTool( tool, mActionEdit );
}

void QgsGPSRouteEditor::editLabel( const QgsLabelPosition &labelPos )
{
  QgsRedliningEditTool* tool = new QgsRedliningEditTool( mApp->mapCanvas(), getOrCreateLayer(), new GPXAttribEditor );
  connect( this, SIGNAL( featureStyleChanged() ), tool, SLOT( onStyleChanged() ) );
  connect( tool, SIGNAL( featureSelected( QgsFeature ) ), this, SLOT( syncStyleWidgets( QgsFeature ) ) );
  connect( tool, SIGNAL( updateFeatureStyle( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ) );
  tool->selectLabel( labelPos );
  tool->setUnsetOnMiss( true );
  setTool( tool, mActionEdit );
}

void QgsGPSRouteEditor::clearLayer()
{
  mLayer = 0;
  mLayerRefCount = 0;
  deactivateTool();
}

void QgsGPSRouteEditor::editObject()
{
  QgsRedliningEditTool* tool = new QgsRedliningEditTool( mApp->mapCanvas(), getOrCreateLayer(), new GPXAttribEditor );
  connect( this, SIGNAL( featureStyleChanged() ), tool, SLOT( onStyleChanged() ) );
  connect( tool, SIGNAL( updateFeatureStyle( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ) );
  setTool( tool, mActionEdit );
}

void QgsGPSRouteEditor::createWaypoints( bool active )
{
  setTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "circle", new GPXAttribEditor ), mActionCreateWaypoints, active );
}

void QgsGPSRouteEditor::createRoutes( bool active )
{
  setTool( new QgsRedliningPolylineMapTool( mApp->mapCanvas(), getOrCreateLayer(), false ), mActionCreateRoutes, active );
}

void QgsGPSRouteEditor::setTool( QgsMapTool *tool, QAction* action , bool active )
{
  if ( active && ( mApp->mapCanvas()->mapTool() == 0 || mApp->mapCanvas()->mapTool()->action() != action ) )
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
  else if ( !active && mApp->mapCanvas()->mapTool() && mApp->mapCanvas()->mapTool()->action() == action )
  {
    mApp->mapCanvas()->unsetMapTool( mApp->mapCanvas()->mapTool() );
  }
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
  mLayer->changeAttributeValue( fid, fields.indexFromName( "size" ), sFeatureSize );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline" ), QgsSymbolLayerV2Utils::encodeColor( QColor( Qt::yellow ) ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill" ), QgsSymbolLayerV2Utils::encodeColor( QColor( Qt::yellow ) ) );
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
  QString filename = QFileDialog::getOpenFileName( mApp, tr( "Import GPX" ), lastProjectDir, tr( "GPX Files (*.gpx)" ) );
  if ( filename.isEmpty() )
  {
    return;
  }

  QFile file( filename );
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    mApp->messageBar()->pushCritical( tr( "GPX import failed" ), tr( "Cannot read file" ) );
    return;
  }
  getOrCreateLayer();

  int nWpts = 0;
  int nRtes = 0;
  int nTracks = 0;

  QDomDocument doc;
  doc.setContent( &file );
  QDomNodeList wpts = doc.elementsByTagName( "wpt" );
  for ( int i = 0, n = wpts.size(); i < n; ++i )
  {
    QDomElement wptEl = wpts.at( i ).toElement();
    double lat = wptEl.attribute( "lat" ).toDouble();
    double lon = wptEl.attribute( "lon" ).toDouble();
    QString name = wptEl.firstChildElement( "name" ).text();
    QString extraFlags( ",symbol=circle,w=2*\"size\",h=2*\"size\",r=0" );
    mLayer->addText( name, QgsPointV2( lon, lat ), Qt::yellow, QFont(), QString(), 0, sFeatureSize, extraFlags );
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
    mLayer->addShape( new QgsGeometry( line ), Qt::yellow, Qt::yellow, sFeatureSize, Qt::SolidLine, Qt::SolidPattern, QString(), name );
    ++nRtes;
  }
  QDomNodeList trks = doc.elementsByTagName( "trk" );
  for ( int i = 0, n = trks.size(); i < n; ++i )
  {
    QDomElement trkEl = trks.at( i ).toElement();
    QString name = trkEl.firstChildElement( "name" ).text();
    QList<QgsPointV2> pts;
    QDomNodeList trkpts = trkEl.firstChildElement( "trkseg" ).elementsByTagName( "trkpt" );
    for ( int j = 0, m = trkpts.size(); j < m; ++j )
    {
      QDomElement trkptEl = trkpts.at( j ).toElement();
      double lat = trkptEl.attribute( "lat" ).toDouble();
      double lon = trkptEl.attribute( "lon" ).toDouble();
      pts.append( QgsPointV2( lon, lat ) );
    }
    QgsLineStringV2* line = new QgsLineStringV2();
    line->setPoints( pts );
    mLayer->addShape( new QgsGeometry( line ), Qt::yellow, Qt::yellow, sFeatureSize, Qt::SolidLine, Qt::SolidPattern, QString(), name );
    ++nTracks;
  }
  mApp->mapCanvas()->clearCache( mLayer->id() );
  mApp->mapCanvas()->refresh();
  mApp->messageBar()->pushInfo( tr( "GPX import complete" ), tr( "%1 waypoints, %2 routes and %3 tracks were read." ).arg( nWpts ).arg( nRtes ).arg( nTracks ) );
}

void QgsGPSRouteEditor::exportGpx()
{
  if ( !mLayer )
  {
    return;
  }
  QString lastProjectDir = QSettings().value( "/UI/lastProjectDir", "." ).toString();
  QString filename = QFileDialog::getSaveFileName( mApp, tr( "Export to GPX" ), lastProjectDir, tr( "GPX Files (*.gpx)" ) );
  if ( filename.isEmpty() )
  {
    return;
  }
  if ( !filename.endsWith( ".gpx", Qt::CaseInsensitive ) )
  {
    filename += ".gpx";
  }

  QFile file( filename );
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    mApp->messageBar()->pushCritical( tr( "GPX export failed" ), tr( "Cannot write to file" ) );
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
      QDomText nameText = doc.createTextNode( feature.attribute( "text" ).toString() );
      nameEl.appendChild( nameText );
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
