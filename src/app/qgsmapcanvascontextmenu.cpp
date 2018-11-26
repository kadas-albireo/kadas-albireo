/***************************************************************************
                          qgsmapcanvasmenu.cpp
                          --------------------
    begin                : Wed Dec 02 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : smani@sourcepole.ch
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>
#include <QClipboard>
#include <QToolButton>
#include "qgisapp.h"
#include "qgsannotationitem.h"
#include "qgsannotationlayer.h"
#include "qgsattributedialog.h"
#include "qgscoordinateformat.h"
#include "qgscrscache.h"
#include "qgsgeometry.h"
#include "qgsgeometryrubberband.h"
#include "qgsgpsrouteeditor.h"
#include "qgisinterface.h"
#include "qgslinestringv2.h"
#include "qgsfeature.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvascontextmenu.h"
#include "qgsmapidentifydialog.h"
#include "qgsmaptoolannotation.h"
#include "qgsmaptoolslope.h"
#include "qgsmaptoolhillshade.h"
#include "qgsmeasuretoolv2.h"
#include "qgsmeasureheightprofiletool.h"
#include "qgspinannotationitem.h"
#include "qgspluginlayer.h"
#include "qgsredlining.h"
#include "qgsredlininglayer.h"
#include "qgsvectorlayer.h"

QgsMapCanvasContextMenu::QgsMapCanvasContextMenu( QgsMapCanvas* canvas, const QPoint& canvasPos, const QgsPoint& mapPos )
    : mMapPos( mapPos ), mCanvas( canvas ), mRubberBand( 0 ), mRectItem( 0 )
{
  mPickResult = QgsFeaturePicker::pick( mCanvas, canvasPos, mapPos, QGis::AnyGeometry );

  addAction( QIcon( ":/images/themes/default/mActionIdentify.svg" ), tr( "Identify" ), this, SLOT( identify() ) );
  addSeparator();

  if ( mPickResult.annotation )
  {
    addAction( QIcon( ":/images/themes/default/mActionToggleEditing.svg" ), tr( "Edit" ), this, SLOT( editAnnotation() ) );
    if ( dynamic_cast<QgsPinAnnotationItem*>( mPickResult.annotation ) )
    {
      addAction( QIcon( ":/images/themes/default/mActionCopyCoordinatesToClipboard.png" ), tr( "Copy position" ), static_cast<QgsPinAnnotationItem*>( mPickResult.annotation ), SLOT( copyPosition() ) );
      addAction( QIcon( ":/images/themes/default/mIconPointLayer.svg" ), tr( "Convert to waypoint" ), static_cast<QgsPinAnnotationItem*>( mPickResult.annotation ), SLOT( convertToWaypoint() ) );
    }
    addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteAnnotation() ) );
    mRectItem = new QGraphicsRectItem();
    mRectItem->setRect( mPickResult.boundingBox );
    mRectItem->setPen( QPen( Qt::red, 2 ) );
    mCanvas->scene()->addItem( mRectItem );
  }
  else if ( mPickResult.otherResult.isValid() && dynamic_cast<QgsPluginLayer*>( mPickResult.layer ) )
  {
    addAction( QIcon( ":/images/themes/default/mActionToggleEditing.svg" ), tr( "Edit" ), this, SLOT( editOtherResult() ) );
    addAction( QIcon( ":/images/themes/default/mActionEditCut.png" ), tr( "Cut" ), this, SLOT( cutOtherResult() ) );
    addAction( QIcon( ":/images/themes/default/mActionEditCopy.png" ), tr( "Copy" ), this, SLOT( copyOtherResult() ) );
    addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteOtherResult() ) );
    mRectItem = new QGraphicsRectItem();
    mRectItem->setRect( mPickResult.boundingBox );
    mRectItem->setPen( QPen( Qt::red, 2 ) );
    mCanvas->scene()->addItem( mRectItem );
  }
  else if ( mPickResult.feature.isValid() && mPickResult.layer )
  {
    const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( mPickResult.layer->crs().authid(), mCanvas->mapSettings().destinationCrs().authid() );
    mRubberBand = new QgsGeometryRubberBand( mCanvas, mPickResult.feature.geometry()->type() );
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_NONE );
    mRubberBand->setOutlineWidth( 2 );
    mRubberBand->setGeometry( mPickResult.feature.geometry()->geometry()->transformed( *ct ) );
    if ( mPickResult.layer->type() == QgsMapLayer::RedliningLayer )
    {
      addAction( QIcon( ":/images/themes/default/mActionToggleEditing.svg" ), tr( "Edit" ), this, SLOT( editFeature() ) );
      if ( mPickResult.feature.geometry()->type() == QGis::Point )
      {
        addAction( QIcon( ":/images/themes/default/pin_red.svg" ), tr( "Convert to pin" ), this, SLOT( convertToPin() ) );
      }
      addAction( QIcon( ":/images/themes/default/mActionEditCut.png" ), tr( "Cut" ), this, SLOT( cutFeature() ) );
    }
    addAction( QIcon( ":/images/themes/default/mActionEditCopy.png" ), tr( "Copy" ), this, SLOT( copyFeature() ) );
    if ( mPickResult.layer->type() == QgsMapLayer::RedliningLayer )
    {
      addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteFeature() ) );
    }
  }
  else if ( mPickResult.labelPos.featureId != -1 && mPickResult.layer->type() == QgsMapLayer::RedliningLayer )
  {
    addAction( QIcon( ":/images/themes/default/mActionEditCut.png" ), tr( "Cut" ), this, SLOT( cutFeature() ) );
    addAction( QIcon( ":/images/themes/default/mActionEditCopy.png" ), tr( "Copy" ), this, SLOT( copyFeature() ) );
    addAction( QIcon( ":/images/themes/default/mActionToggleEditing.svg" ), tr( "Edit" ), this, SLOT( editLabel() ) );
    addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteLabel() ) );
    mRubberBand = new QgsGeometryRubberBand( mCanvas, QGis::Line );
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_NONE );
    mRubberBand->setOutlineWidth( 2 );
    QgsLineStringV2* lineString = new QgsLineStringV2();
    foreach ( const QgsPoint& p, mPickResult.labelPos.cornerPoints )
    {
      lineString->addVertex( QgsPointV2( p.x(), p.y() ) );
    }
    lineString->addVertex( lineString->vertexAt( QgsVertexId( 0, 0, 0 ) ) );
    mRubberBand->setGeometry( lineString );
  }
  if ( mPickResult.isEmpty() )
  {
    if ( QgisApp::instance()->canPaste() )
    {
      addAction( QIcon( ":/images/themes/default/mActionEditPaste.png" ), tr( "Paste" ), this, SLOT( paste() ) );
    }
    QMenu* drawMenu = new QMenu();
    addAction( tr( "Draw" ) )->setMenu( drawMenu );
    drawMenu->addAction( QIcon( ":/images/themes/default/pin_red.svg" ), tr( "Pin marker" ), this, SLOT( drawPin() ) );
    drawMenu->addAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Point marker" ), this, SLOT( drawPointMarker() ) );
    drawMenu->addAction( QIcon( ":/images/themes/default/redlining_square.svg" ), tr( "Square marker" ), this, SLOT( drawSquareMarker() ) );
    drawMenu->addAction( QIcon( ":/images/themes/default/redlining_triangle.svg" ), tr( "Triangle marker" ), this, SLOT( drawTriangleMarker() ) );
    drawMenu->addAction( QIcon( ":/images/themes/default/redlining_line.svg" ), tr( "Line" ), this, SLOT( drawLine() ) );
    drawMenu->addAction( QIcon( ":/images/themes/default/redlining_rectangle.svg" ), tr( "Rectangle" ), this, SLOT( drawRectangle() ) );
    drawMenu->addAction( QIcon( ":/images/themes/default/redlining_polygon.svg" ), tr( "Polygon" ), this, SLOT( drawPolygon() ) );
    drawMenu->addAction( QIcon( ":/images/themes/default/redlining_circle.svg" ), tr( "Circle" ), this, SLOT( drawCircle() ) );
    drawMenu->addAction( QIcon( ":/images/themes/default/redlining_text.svg" ), tr( "Text" ), this, SLOT( drawText() ) );
    addAction( QIcon( ":/images/themes/default/mIconSelectRemove.svg" ), tr( "Delete items" ), this, SLOT( deleteItems() ) );
  }
  addSeparator();
  if (( mPickResult.isEmpty() || ( mPickResult.feature.isValid() && mPickResult.feature.geometry()->type() != QGis::Point ) ) )
  {
    bool isCircle = mPickResult.feature.isValid() && mPickResult.feature.geometry()->geometry()->wkbType() == QgsWKBTypes::CurvePolygon;
    QMenu* measureMenu = new QMenu();
    addAction( tr( "Measure" ) )->setMenu( measureMenu );
    if ( !mPickResult.feature.isValid() || !isCircle )
    {
      measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasure.png" ), tr( "Distance" ), this, SLOT( measureLine() ) );
    }
    if ( !mPickResult.feature.isValid() || ( !isCircle && mPickResult.feature.geometry()->type() == QGis::Polygon ) )
    {
      measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureArea.png" ), tr( "Area" ), this, SLOT( measurePolygon() ) );
    }
    if ( !mPickResult.feature.isValid() || isCircle )
    {
      measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureCircle.png" ), tr( "Circle" ), this, SLOT( measureCircle() ) );
    }
    if ( !mPickResult.feature.isValid() )
    {
      measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureAngle.png" ), tr( "Angle" ), this, SLOT( measureAngle() ) );
    }
    if ( !mPickResult.feature.isValid() || !isCircle )
    {
      measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureHeightProfile.png" ), tr( "Height profile" ), this, SLOT( measureHeightProfile() ) );
    }
    QMenu* analysisMenu = new QMenu();
    addAction( tr( "Terrain analysis" ) )->setMenu( analysisMenu );
    analysisMenu->addAction( QIcon( ":/images/themes/default/slope.svg" ), tr( "Slope" ), this, SLOT( terrainSlope() ) );
    analysisMenu->addAction( QIcon( ":/images/themes/default/hillshade.svg" ), tr( "Hillshade" ), this, SLOT( terrainHillshade() ) );
    if ( !mPickResult.feature.isValid() )
    {
      analysisMenu->addAction( QIcon( ":/images/themes/default/viewshed.svg" ), tr( "Viewshed" ), this, SLOT( terrainViewshed() ) );
    }
    if ( !mPickResult.feature.isValid() || ( mPickResult.feature.geometry()->type() == QGis::Line && mPickResult.feature.geometry()->geometry()->vertexCount() == 2 ) )
    {
      analysisMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureHeightProfile.png" ), tr( "Line of sight" ), this, SLOT( measureHeightProfile() ) );
    }
  }

  if ( mPickResult.isEmpty() )
  {
    addAction( QIcon( ":/images/themes/default/mActionCopyCoordinatesToClipboard.png" ), tr( "Copy coordinates" ), this, SLOT( copyCoordinates() ) );
    addAction( QIcon( ":/images/themes/default/mActionSaveMapToClipboard.png" ), tr( "Copy map" ), this, SLOT( copyMap() ) );
    addAction( QIcon( ":/images/themes/default/mActionFilePrint.png" ), tr( "Print" ), this, SLOT( print() ) );
  }
}

QgsMapCanvasContextMenu::~QgsMapCanvasContextMenu()
{
  delete mRubberBand;
  delete mRectItem;
}

void QgsMapCanvasContextMenu::identify()
{
  QgsMapIdentifyDialog* dialog = new QgsMapIdentifyDialog( mCanvas, mMapPos );
  dialog->show();
}

void QgsMapCanvasContextMenu::deleteAnnotation()
{
  mPickResult.annotation->deleteLater();
}

void QgsMapCanvasContextMenu::deleteFeature()
{
  QgsRedliningLayer* layer = qobject_cast<QgsRedliningLayer*>( mPickResult.layer );
  if ( layer )
  {
    layer->deleteFeature( mPickResult.feature.id() );
    layer->triggerRepaint();
  }
}

void QgsMapCanvasContextMenu::deleteLabel()
{
  static_cast<QgsRedliningLayer*>( mPickResult.layer )->deleteFeature( mPickResult.labelPos.featureId );
  static_cast<QgsRedliningLayer*>( mPickResult.layer )->triggerRepaint();
}

void QgsMapCanvasContextMenu::deleteOtherResult()
{
  static_cast<QgsPluginLayer*>( mPickResult.layer )->deleteItems( QVariantList() << mPickResult.otherResult );
  static_cast<QgsPluginLayer*>( mPickResult.layer )->triggerRepaint();
}

void QgsMapCanvasContextMenu::editAnnotation()
{
  mCanvas->setMapTool( new QgsMapToolEditAnnotation( mCanvas, mPickResult.annotation ) );
}

void QgsMapCanvasContextMenu::editFeature()
{
  if ( mPickResult.layer == QgisApp::instance()->redlining()->getLayer() )
    QgisApp::instance()->redlining()->editFeature( mPickResult.feature );
  else if ( mPickResult.layer == QgisApp::instance()->gpsRouteEditor()->getLayer() )
    QgisApp::instance()->gpsRouteEditor()->editFeature( mPickResult.feature );
}

void QgsMapCanvasContextMenu::editOtherResult()
{
  static_cast<QgsPluginLayer*>( mPickResult.layer )->handlePick( mPickResult.otherResult );
}

void QgsMapCanvasContextMenu::editLabel()
{
  if ( mPickResult.layer == QgisApp::instance()->redlining()->getLayer() )
    QgisApp::instance()->redlining()->editLabel( mPickResult.labelPos );
  else if ( mPickResult.layer == QgisApp::instance()->gpsRouteEditor()->getLayer() )
    QgisApp::instance()->gpsRouteEditor()->editLabel( mPickResult.labelPos );
}

void QgsMapCanvasContextMenu::convertToPin()
{
  if ( !mPickResult.feature.geometry() )
  {
    return;
  }
  QgsPointV2* p = dynamic_cast<QgsPointV2*>( mPickResult.feature.geometry()->geometry() );
  if ( !p )
  {
    return;
  }
  QgsRedliningLayer* layer = qobject_cast<QgsRedliningLayer*>( mPickResult.layer );
  if ( !layer )
  {
    return;
  }
  const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( mPickResult.layer->crs().authid(), mCanvas->mapSettings().destinationCrs().authid() );
  QgsPinAnnotationItem* pinItem = new QgsPinAnnotationItem( mCanvas );
  pinItem->setMapPosition( ct->transform( p->x(), p->y() ) );
  pinItem->setSelected( true );
  pinItem->setName( mPickResult.feature.attribute( "text" ).toString() );
  QgsAnnotationLayer::getLayer( mCanvas, "mapPins", tr( "Pins" ) )->addItem( pinItem );

  layer->deleteFeature( mPickResult.feature.id() );
  layer->triggerRepaint();
}

void QgsMapCanvasContextMenu::cutFeature()
{
  if ( mPickResult.layer && mPickResult.layer == QgisApp::instance()->redlining()->getLayer() )
  {
    QgsVectorLayer* vlayer = static_cast<QgsVectorLayer*>( mPickResult.layer );
    QgsFeatureIds prevSelection = vlayer->selectedFeaturesIds();
    QgsFeatureIds newSelection;
    if ( mPickResult.feature.isValid() )
    {
      newSelection.insert( mPickResult.feature.id() );
    }
    else
    {
      newSelection.insert( mPickResult.labelPos.featureId );
    }
    vlayer->setSelectedFeatures( newSelection );
    vlayer->startEditing();
    QgisApp::instance()->cutFeatures( vlayer );
    vlayer->commitChanges();
    vlayer->setSelectedFeatures( prevSelection );
  }
}

void QgsMapCanvasContextMenu::cutOtherResult()
{
  static_cast<QgsPluginLayer*>( mPickResult.layer )->copyItems( QVariantList() << mPickResult.otherResult, true );
  static_cast<QgsPluginLayer*>( mPickResult.layer )->triggerRepaint();
}

void QgsMapCanvasContextMenu::copyFeature()
{
  if ( mPickResult.layer )
  {
    QgsVectorLayer* vlayer = static_cast<QgsVectorLayer*>( mPickResult.layer );
    QgsFeatureIds prevSelection = vlayer->selectedFeaturesIds();
    QgsFeatureIds newSelection;
    if ( mPickResult.feature.isValid() )
    {
      newSelection.insert( mPickResult.feature.id() );
    }
    else
    {
      newSelection.insert( mPickResult.labelPos.featureId );
    }
    vlayer->setSelectedFeatures( newSelection );
    QgisApp::instance()->copyFeatures( vlayer );
    vlayer->setSelectedFeatures( prevSelection );
  }
}

void QgsMapCanvasContextMenu::copyOtherResult()
{
  static_cast<QgsPluginLayer*>( mPickResult.layer )->copyItems( QVariantList() << mPickResult.otherResult, false );
}

void QgsMapCanvasContextMenu::paste()
{
  QgisApp::instance()->paste( mMapPos );
}

void QgsMapCanvasContextMenu::drawPin()
{
  mCanvas->setMapTool( QgisApp::instance()->mapTools()->mPinAnnotation );
}

void QgsMapCanvasContextMenu::drawPointMarker()
{
  QgisApp::instance()->redlining()->setPointTool();
}

void QgsMapCanvasContextMenu::drawSquareMarker()
{
  QgisApp::instance()->redlining()->setSquareTool();
}

void QgsMapCanvasContextMenu::drawTriangleMarker()
{
  QgisApp::instance()->redlining()->setTriangleTool();
}

void QgsMapCanvasContextMenu::drawLine()
{
  QgisApp::instance()->redlining()->setLineTool();
}

void QgsMapCanvasContextMenu::drawRectangle()
{
  QgisApp::instance()->redlining()->setRectangleTool();
}

void QgsMapCanvasContextMenu::drawPolygon()
{
  QgisApp::instance()->redlining()->setPolygonTool();
}

void QgsMapCanvasContextMenu::drawCircle()
{
  QgisApp::instance()->redlining()->setCircleTool();
}

void QgsMapCanvasContextMenu::drawText()
{
  QgisApp::instance()->redlining()->setTextTool();
}

void QgsMapCanvasContextMenu::measureLine()
{
  mCanvas->setMapTool( QgisApp::instance()->mapTools()->mMeasureDist );
  if ( mPickResult.layer )
  {
    static_cast<QgsMeasureToolV2*>( QgisApp::instance()->mapTools()->mMeasureDist )->addGeometry( mPickResult.feature.geometry(), static_cast<QgsVectorLayer*>( mPickResult.layer ) );
  }
}

void QgsMapCanvasContextMenu::measurePolygon()
{
  mCanvas->setMapTool( QgisApp::instance()->mapTools()->mMeasureArea );
  if ( mPickResult.layer )
  {
    static_cast<QgsMeasureToolV2*>( QgisApp::instance()->mapTools()->mMeasureArea )->addGeometry( mPickResult.feature.geometry(), static_cast<QgsVectorLayer*>( mPickResult.layer ) );
  }
}

void QgsMapCanvasContextMenu::measureCircle()
{
  mCanvas->setMapTool( QgisApp::instance()->mapTools()->mMeasureCircle );
  if ( mPickResult.layer )
  {
    static_cast<QgsMeasureToolV2*>( QgisApp::instance()->mapTools()->mMeasureCircle )->addGeometry( mPickResult.feature.geometry(), static_cast<QgsVectorLayer*>( mPickResult.layer ) );
  }
}

void QgsMapCanvasContextMenu::measureAngle()
{
  mCanvas->setMapTool( QgisApp::instance()->mapTools()->mMeasureAngle );
  if ( mPickResult.layer )
  {
    static_cast<QgsMeasureToolV2*>( QgisApp::instance()->mapTools()->mMeasureAngle )->addGeometry( mPickResult.feature.geometry(), static_cast<QgsVectorLayer*>( mPickResult.layer ) );
  }
}

void QgsMapCanvasContextMenu::measureHeightProfile()
{
  mCanvas->setMapTool( QgisApp::instance()->mapTools()->mMeasureHeightProfile );
  if ( mPickResult.layer )
  {
    static_cast<QgsMeasureHeightProfileTool*>( QgisApp::instance()->mapTools()->mMeasureHeightProfile )->setGeometry( mPickResult.feature.geometry(), static_cast<QgsVectorLayer*>( mPickResult.layer ) );
  }
}

void QgsMapCanvasContextMenu::terrainSlope()
{
  if ( mPickResult.feature.isValid() )
  {
    QgsRectangle extent = mPickResult.feature.geometry()->boundingBox();
    const QgsCoordinateReferenceSystem& crs = mPickResult.layer->crs();
    static_cast<QgsMapToolSlope*>( QgisApp::instance()->mapTools()->mSlope )->compute( extent, crs );
  }
  else
  {
    mCanvas->setMapTool( QgisApp::instance()->mapTools()->mSlope );
  }
}

void QgsMapCanvasContextMenu::terrainHillshade()
{
  if ( mPickResult.feature.isValid() )
  {
    QgsRectangle extent = mPickResult.feature.geometry()->boundingBox();
    const QgsCoordinateReferenceSystem& crs = mPickResult.layer->crs();
    static_cast<QgsMapToolHillshade*>( QgisApp::instance()->mapTools()->mHillshade )->compute( extent, crs );
  }
  else
  {
    mCanvas->setMapTool( QgisApp::instance()->mapTools()->mHillshade );
  }
}

void QgsMapCanvasContextMenu::terrainViewshed()
{
  mCanvas->setMapTool( QgisApp::instance()->mapTools()->mViewshed );
}

void QgsMapCanvasContextMenu::copyCoordinates()
{
  const QgsCoordinateReferenceSystem& mapCrs = mCanvas->mapSettings().destinationCrs();
  QString posStr = QgsCoordinateFormat::instance()->getDisplayString( mMapPos, mapCrs );
  if ( posStr.isEmpty() )
  {
    posStr = QString( "%1 (%2)" ).arg( mMapPos.toString() ).arg( mapCrs.authid() );
  }
  QString text = QString( "%1\n%2" )
                 .arg( posStr )
                 .arg( QgsCoordinateFormat::instance()->getHeightAtPos( mMapPos, mapCrs ) );
  QApplication::clipboard()->setText( text );
}

void QgsMapCanvasContextMenu::copyMap()
{
  QgisApp::instance()->saveMapToClipboard();
}

void QgsMapCanvasContextMenu::print()
{
  QAction* printAction = QgisApp::instance()->findAction( "mActionPrint" );
  if ( printAction && !printAction->isChecked() )
  {
    printAction->trigger();
  }
}

void QgsMapCanvasContextMenu::deleteItems()
{
  mCanvas->setMapTool( QgisApp::instance()->mapTools()->mDeleteItems );
}
