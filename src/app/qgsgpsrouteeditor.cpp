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
#include "qgsrubberband.h"
#include "qgssymbollayerv2utils.h"
#include "qgsproject.h"

#include <QDomDocument>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QSettings>
#include <QSpinBox>


int QgsGPSRouteEditor::sFeatureSize = 2;


class QgsGPSRouteEditor::WaypointEditor : public QgsRedliningAttribEditor
{
    Q_DECLARE_TR_FUNCTIONS( WaypointEditor )
  public:
    WaypointEditor()
        : QgsRedliningAttribEditor( tr( "Waypoint" ) )
    {
      QGridLayout* layout = new QGridLayout();
      setLayout( layout );
      layout->setContentsMargins( 2, 2, 2, 2 );
      layout->setSpacing( 2 );

      layout->addWidget( new QLabel( tr( "Name:" ) ), 0, 0, 1, 1 );
      mNameEdit = new QLineEdit;
      layout->addWidget( mNameEdit, 0, 1, 1, 3 );
      mNameEdit->selectAll();

      layout->addWidget( new QLabel( tr( "Color:" ) ), 1, 0, 1, 1 );
      mColorButton = new QgsColorButtonV2( this );
      layout->addWidget( mColorButton, 1, 1, 1, 1 );

      layout->addWidget( new QLabel( tr( "Size:" ) ), 1, 2, 1, 1 );
      mSpinBoxSize = new QSpinBox( this );
      mSpinBoxSize->setRange( 1, 99 );
      layout->addWidget( mSpinBoxSize, 1, 3, 1, 1 );
    }
    void setTool( QgsMapToolDrawShape* tool )
    {
      mTool = tool;
    }
    void set( const QgsAttributes &attribs, const QgsFields &fields ) override
    {
      mNameEdit->setText( attribs.value( fields.fieldNameIndex( "text" ) ).toString() );
      mSpinBoxSize->setValue( attribs.value( fields.fieldNameIndex( "size" ) ).toInt() );
      mColorButton->setColor( QgsSymbolLayerV2Utils::decodeColor( attribs.value( fields.fieldNameIndex( "outline" ) ).toString() ) );
    }
    void get( QgsAttributes &attribs, const QgsFields &fields ) const override
    {
      attribs[fields.fieldNameIndex( "text" )] = mNameEdit->text();
      attribs[fields.fieldNameIndex( "outline" )] = QgsSymbolLayerV2Utils::encodeColor( mColorButton->color() );
      attribs[fields.fieldNameIndex( "fill" )] = QgsSymbolLayerV2Utils::encodeColor( mColorButton->color() );
      attribs[fields.fieldNameIndex( "size" )] = QString( "%1" ).arg( mSpinBoxSize->value() );
      QMap<QString, QString> flagsMap = QgsRedliningLayer::deserializeFlags( attribs[fields.fieldNameIndex( "flags" )].toString() );
      QFont font;
      flagsMap["family"] = font.family();
      flagsMap["italic"] = QString( "%1" ).arg( font.italic() );
      flagsMap["bold"] = "1";
      flagsMap["rotation"] = QString( "%1" ).arg( 0. );
      flagsMap["fontSize"] = QString( "%1" ).arg( font.pointSize() );
      attribs[fields.fieldNameIndex( "flags" )] = QgsRedliningLayer::serializeFlags( flagsMap );

      QSettings().setValue( "/GPXRoutes/WaypointSize", attribs[fields.fieldNameIndex( "size" )] );
      QSettings().setValue( "/GPXRoutes/WaypointColor", attribs[fields.fieldNameIndex( "outline" )] );
      mTool->updateStyle( mSpinBoxSize->value(), mColorButton->color(), mColorButton->color(), Qt::SolidLine, Qt::SolidPattern );
    }
    void setFocus() override
    {
      mNameEdit->setFocus();
    }

  private:
    QgsMapToolDrawShape* mTool;
    QLineEdit* mNameEdit;
    QSpinBox* mSpinBoxSize;
    QgsColorButtonV2* mColorButton;
};

class QgsGPSRouteEditor::RouteEditor : public QgsRedliningAttribEditor
{
    Q_DECLARE_TR_FUNCTIONS( RouteEditor )
  public:
    RouteEditor() : QgsRedliningAttribEditor( tr( "Route" ) )
    {
      QGridLayout* layout = new QGridLayout();
      layout->setContentsMargins( 2, 2, 2, 2 );
      layout->setSpacing( 2 );
      setLayout( layout );

      layout->addWidget( new QLabel( tr( "Name:" ) ), 0, 0, 1, 1 );
      mNameEdit = new QLineEdit();
      mNameEdit->selectAll();
      layout->addWidget( mNameEdit, 0, 1, 1, 3 );

      layout->addWidget( new QLabel( tr( "Number:" ) ), 1, 0, 1, 1 );
      mNumberEdit = new QLineEdit();
      mNumberEdit->setValidator( new QIntValidator( 0, std::numeric_limits<int>::max() ) );
      layout->addWidget( mNumberEdit, 1, 1, 1, 3 );

      layout->addWidget( new QLabel( tr( "Color:" ) ), 2, 0, 1, 1 );
      mColorButton = new QgsColorButtonV2( this );
      layout->addWidget( mColorButton, 2, 1, 1, 1 );

      layout->addWidget( new QLabel( tr( "Size:" ) ), 2, 2, 1, 1 );
      mSpinBoxSize = new QSpinBox( this );
      mSpinBoxSize->setRange( 1, 99 );
      layout->addWidget( mSpinBoxSize, 2, 3, 1, 1 );
    }
    void setTool( QgsMapToolDrawShape* tool )
    {
      mTool = tool;
    }
    void set( const QgsAttributes &attribs, const QgsFields &fields ) override
    {
      mNameEdit->setText( attribs.value( fields.fieldNameIndex( "text" ) ).toString() );
      QMap<QString, QString> flagsMap = QgsRedliningLayer::deserializeFlags( attribs[fields.fieldNameIndex( "flags" )].toString() );
      mNumberEdit->setText( flagsMap["routeNumber"] );
      mSpinBoxSize->setValue( attribs.value( fields.fieldNameIndex( "size" ) ).toInt() );
      mColorButton->setColor( QgsSymbolLayerV2Utils::decodeColor( attribs.value( fields.fieldNameIndex( "outline" ) ).toString() ) );
    }
    void get( QgsAttributes &attribs, const QgsFields &fields ) const override
    {
      attribs[fields.fieldNameIndex( "text" )] = mNameEdit->text();
      attribs[fields.fieldNameIndex( "outline" )] = QgsSymbolLayerV2Utils::encodeColor( mColorButton->color() );
      attribs[fields.fieldNameIndex( "fill" )] = QgsSymbolLayerV2Utils::encodeColor( mColorButton->color() );
      attribs[fields.fieldNameIndex( "size" )] = QString( "%1" ).arg( mSpinBoxSize->value() );

      QMap<QString, QString> flagsMap = QgsRedliningLayer::deserializeFlags( attribs[fields.fieldNameIndex( "flags" )].toString() );
      QFont font;
      flagsMap["family"] = font.family();
      flagsMap["italic"] = QString( "%1" ).arg( font.italic() );
      flagsMap["bold"] = "1";
      flagsMap["rotation"] = QString( "%1" ).arg( 0. );
      flagsMap["fontSize"] = QString( "%1" ).arg( font.pointSize() );
      flagsMap["routeNumber"] = mNumberEdit->text();
      attribs[fields.fieldNameIndex( "flags" )] = QgsRedliningLayer::serializeFlags( flagsMap );

      QSettings().setValue( "/GPXRoutes/RouteSize", attribs[fields.fieldNameIndex( "size" )] );
      QSettings().setValue( "/GPXRoutes/RouteColor", attribs[fields.fieldNameIndex( "outline" )] );
      mTool->updateStyle( mSpinBoxSize->value(), mColorButton->color(), mColorButton->color(), Qt::SolidLine, Qt::SolidPattern );
    }
    void setFocus() override
    {
      mNameEdit->setFocus();
    }
  private:
    QgsMapToolDrawShape* mTool;
    QLineEdit* mNameEdit;
    QLineEdit* mNumberEdit;
    QSpinBox* mSpinBoxSize;
    QgsColorButtonV2* mColorButton;
};


QgsGPSRouteEditor::QgsGPSRouteEditor( QgisApp* app, QAction *actionCreateWaypoints, QAction *actionCreateRoutes )
    : QgsRedliningManager( app )
    , mApp( app )
    , mActionCreateWaypoints( actionCreateWaypoints )
    , mActionCreateRoutes( actionCreateRoutes )
    , mLayer( 0 )
    , mLayerRefCount( 0 )
{
  mActionEdit = new QAction( this );

  connect( actionCreateWaypoints, SIGNAL( triggered( bool ) ), this, SLOT( setWaypointsTool( bool ) ) );
  connect( actionCreateRoutes, SIGNAL( triggered( bool ) ), this, SLOT( setRoutesTool( bool ) ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProject( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProject( QDomDocument& ) ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWillBeRemoved( QString ) ), this, SLOT( checkLayerRemoved( QString ) ) );
}

QgsRedliningLayer* QgsGPSRouteEditor::getOrCreateLayer()
{
  if ( !mLayer )
  {
    QgsRedliningLayer* layer = new QgsRedliningLayer( tr( "GPS Routes" ), "EPSG:4326" );
    QgsMapLayerRegistry::instance()->addMapLayer( layer, true, true );
    setLayer( layer );
  }
  return mLayer;
}

void QgsGPSRouteEditor::setLayer( QgsRedliningLayer *layer )
{
  if ( !layer )
    return;
  mLayer = layer;
  // Labeling tweaks to make both point and line labeling appear more or less sensible
  mLayer->setCustomProperty( "labeling/placement", 2 );
  mLayer->setCustomProperty( "labeling/placementFlags", 10 );
  mLayer->setCustomProperty( "labeling/dist", 2 );
  mLayer->setCustomProperty( "labeling/distInMapUnits", false );
  mLayer->setCustomProperty( "labeling/bufferDraw", true );
  mLayer->setCustomProperty( "labeling/bufferSize", 1 );
  QgsMapLayerRegistry::instance()->addMapLayer( mLayer, true, true );
  mLayerRefCount = 0;
}

QgsRedliningLayer* QgsGPSRouteEditor::getLayer() const
{
  return mLayer.data();
}

void QgsGPSRouteEditor::editFeature( const QgsFeature& feature )
{
  QString flags = feature.attribute( "flags" ).toString();
  QRegExp shapeRe( "\\bshape=(\\w+)\\b" );
  shapeRe.indexIn( flags );
  QString shape = shapeRe.cap( 1 );
  if ( shape == "point" )
  {
    setWaypointsTool( true, &feature );
  }
  else if ( shape == "line" )
  {
    setRoutesTool( true, &feature );
  }
}

void QgsGPSRouteEditor::editLabel( const QgsLabelPosition &labelPos )
{
  QgsFeature feature;
  getOrCreateLayer()->getFeatures( QgsFeatureRequest( labelPos.featureId ) ).nextFeature( feature );
  editFeature( feature );
}

void QgsGPSRouteEditor::editFeatures( const QList<QgsFeature> &features )
{
  int tot = features.size();
  if ( tot == 1 )
  {
    editFeature( features.first() );
  }
  else if ( tot > 1 )
  {
    mApp->mapCanvas()->setMapTool( new QgsRedliningEditGroupMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), features, QList<QgsLabelPosition>() ) );
  }
}

void QgsGPSRouteEditor::checkLayerRemoved( const QString &layerId )
{
  if ( mLayer && mLayer->id() == layerId )
  {
    deactivateTool();
    mLayerRefCount = 0;
    mLayer = 0;
  }
}

void QgsGPSRouteEditor::setWaypointsTool( bool active, const QgsFeature *editFeature )
{
  WaypointEditor* editor = new WaypointEditor;
  if ( setTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), "circle", editFeature, editor ), mActionCreateWaypoints, active ) )
  {
    editor->setTool( mRedliningTool.data() );
    int size = QSettings().value( "/GPXRoutes/WaypointSize", sFeatureSize ).toInt();
    QColor color = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/GPXRoutes/WaypointColor", QgsSymbolLayerV2Utils::encodeColor( Qt::yellow ) ).toString() );
    mRedliningTool->updateStyle( size, color, color, Qt::SolidLine, Qt::SolidPattern );
  }
}

void QgsGPSRouteEditor::setRoutesTool( bool active, const QgsFeature *editFeature )
{
  RouteEditor* editor = new RouteEditor;
  if ( setTool( new QgsRedliningPolylineMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), false, editFeature, editor ), mActionCreateRoutes, active ) )
  {
    editor->setTool( mRedliningTool.data() );
    int size = QSettings().value( "/GPXRoutes/RouteSize", sFeatureSize ).toInt();
    QColor color = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/GPXRoutes/RouteColor", QgsSymbolLayerV2Utils::encodeColor( Qt::yellow ) ).toString() );
    mRedliningTool->updateStyle( size, color, color, Qt::SolidLine, Qt::SolidPattern );
  }
}

bool QgsGPSRouteEditor::setTool( QgsMapToolDrawShape *tool, QAction* action , bool active )
{
  if ( active && ( mApp->mapCanvas()->mapTool() == 0 || mApp->mapCanvas()->mapTool()->action() != action ) )
  {
    tool->setAction( action );
    connect( tool, SIGNAL( deactivated() ), this, SLOT( deactivateTool() ) );
    if ( mLayerRefCount == 0 )
    {
      mApp->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
      mApp->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
    }
    ++mLayerRefCount;
    mApp->mapCanvas()->setMapTool( tool );
    mRedliningTool = tool;
    return true;
  }
  else if ( !active && mApp->mapCanvas()->mapTool() && mApp->mapCanvas()->mapTool()->action() == action )
  {
    mApp->mapCanvas()->unsetMapTool( mApp->mapCanvas()->mapTool() );
  }
  return false;
}

void QgsGPSRouteEditor::deactivateTool()
{
  if ( mRedliningTool )
  {
    if ( mLayer )
    {
      --mLayerRefCount;
      if ( mLayerRefCount == 0 && mApp->mapCanvas()->currentLayer() == mLayer )
      {
        mApp->layerTreeView()->setCurrentLayer( 0 );
      }
    }
    mRedliningTool->deleteLater();
  }
}

void QgsGPSRouteEditor::readProject( const QDomDocument& doc )
{
  QDomNodeList nl = doc.elementsByTagName( "GpsRoutes" );
  if ( nl.isEmpty() || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  setLayer( qobject_cast<QgsRedliningLayer*>( QgsMapLayerRegistry::instance()->mapLayer( nl.at( 0 ).toElement().attribute( "layerid" ) ) ) );
}

void QgsGPSRouteEditor::writeProject( QDomDocument& doc )
{
  if ( !mLayer )
  {
    return;
  }

  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.isEmpty() || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement redliningElem = doc.createElement( "GpsRoutes" );
  redliningElem.setAttribute( "layerid", mLayer->id() );
  qgisElem.appendChild( redliningElem );
}

void QgsGPSRouteEditor::importGpx()
{
  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString filename = QFileDialog::getOpenFileName( mApp, tr( "Import GPX" ), lastDir, tr( "GPX Files (*.gpx)" ) );
  if ( filename.isEmpty() )
  {
    return;
  }
  QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );

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
    QString flags( "shape=point,symbol=circle,w=2*\"size\",h=2*\"size\",r=0,bold=1" );
    mLayer->addShape( new QgsGeometry( new QgsPointV2( lon, lat ) ), Qt::yellow, Qt::yellow, sFeatureSize, Qt::SolidLine, Qt::SolidPattern, flags, QString(), name );
    ++nWpts;
  }
  QDomNodeList rtes = doc.elementsByTagName( "rte" );
  for ( int i = 0, n = rtes.size(); i < n; ++i )
  {
    QDomElement rteEl = rtes.at( i ).toElement();
    QString name = rteEl.firstChildElement( "name" ).text();
    QString number = rteEl.firstChildElement( "name" ).text();
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
    QString flags = QString( "shape=line,routeNumber=%1,bold=1" ).arg( number );
    mLayer->addShape( new QgsGeometry( line ), Qt::yellow, Qt::yellow, sFeatureSize, Qt::SolidLine, Qt::SolidPattern, flags, QString(), name );
    ++nRtes;
  }
  QDomNodeList trks = doc.elementsByTagName( "trk" );
  for ( int i = 0, n = trks.size(); i < n; ++i )
  {
    QDomElement trkEl = trks.at( i ).toElement();
    QString name = trkEl.firstChildElement( "name" ).text();
    QString number = trkEl.firstChildElement( "name" ).text();
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
    QString flags = QString( "shape=line,routeNumber=%1,bold=1" ).arg( number );
    mLayer->addShape( new QgsGeometry( line ), Qt::yellow, Qt::yellow, sFeatureSize, Qt::SolidLine, Qt::SolidPattern, flags, QString(), name );
    ++nTracks;
  }
  mLayer->triggerRepaint();
  mApp->messageBar()->pushInfo( tr( "GPX import complete" ), tr( "%1 waypoints, %2 routes and %3 tracks were read." ).arg( nWpts ).arg( nRtes ).arg( nTracks ) );
}

void QgsGPSRouteEditor::exportGpx()
{
  if ( !mLayer )
  {
    return;
  }
  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString filename = QFileDialog::getSaveFileName( mApp, tr( "Export to GPX" ), lastDir, tr( "GPX Files (*.gpx)" ) );
  if ( filename.isEmpty() )
  {
    return;
  }
  QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
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
  fit = mLayer->getFeatures();
  while ( fit.nextFeature( feature ) )
  {
    if ( feature.geometry()->type() == QGis::Line )
    {
      QMap<QString, QString> flagsMap = QgsRedliningLayer::deserializeFlags( feature.attribute( "flags" ).toString() );
      QgsCurveV2* curve = static_cast<QgsCurveV2*>( feature.geometry()->geometry() );
      QDomElement rteEl = doc.createElement( "rte" );
      QDomElement nameEl = doc.createElement( "name" );
      QDomText nameText = doc.createTextNode( feature.attribute( "text" ).toString() );
      nameEl.appendChild( nameText );
      rteEl.appendChild( nameEl );
      QDomElement numberEl = doc.createElement( "number" );
      QDomText numberText = doc.createTextNode( flagsMap["routeNumber"] );
      numberEl.appendChild( numberText );
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
