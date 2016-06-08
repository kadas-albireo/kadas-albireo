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
#include "qgsattributedialog.h"
#include "qgscoordinateformat.h"
#include "qgsgeometry.h"
#include "qgsgeometryrubberband.h"
#include "qgsgpsrouteeditor.h"
#include "qgisinterface.h"
#include "qgsfeature.h"
#include "qgsfeaturepicker.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvascontextmenu.h"
#include "qgsmeasuretoolv2.h"
#include "qgsmeasureheightprofiletool.h"
#include "qgsredlining.h"
#include "qgsredlininglayer.h"
#include "qgsvectorlayer.h"

QgsMapCanvasContextMenu::QgsMapCanvasContextMenu( const QgsPoint& mapPos )
    : mMapPos( mapPos ), mSelectedLayer( 0 ), mSelectedFeature( 0 )
{
  mRubberBand = 0;

  // TODO: Handle MiliX layers
  QgsFeaturePicker::PickResult pickResult = QgsFeaturePicker::pick( QgisApp::instance()->mapCanvas(), mapPos, QGis::AnyGeometry );
  const QgsLabelingResults* labelingResults = QgisApp::instance()->mapCanvas()->labelingResults();
  mLabelPositions = labelingResults ? labelingResults->labelsAtPosition( mMapPos ) : QList<QgsLabelPosition>();
  // A feature was picked
  if ( pickResult.feature.isValid() )
  {
    mSelectedFeature = pickResult.feature;
    mSelectedLayer = static_cast<QgsVectorLayer*>( pickResult.layer );
    QgsCoordinateTransform ct( pickResult.layer->crs(), QgisApp::instance()->mapCanvas()->mapSettings().destinationCrs() );
    mRubberBand = new QgsGeometryRubberBand( QgisApp::instance()->mapCanvas(), pickResult.feature.geometry()->type() );
    mRubberBand->setGeometry( pickResult.feature.geometry()->geometry()->transformed( ct ) );
    if ( pickResult.layer->type() == QgsMapLayer::RedliningLayer )
    {
      addAction( QIcon( ":/images/themes/default/mActionToggleEditing.svg" ), tr( "Edit" ), this, SLOT( editFeature() ) );
      addAction( QIcon( ":/images/themes/default/mActionEditCut.png" ), tr( "Cut" ), this, SLOT( cutFeature() ) );
    }
    else
    {
      addAction( QIcon( ":/images/themes/default/mActionIdentify.svg" ), tr( "Identify" ), this, SLOT( featureAttributes() ) );
    }
    addAction( QIcon( ":/images/themes/default/mActionEditCopy.png" ), tr( "Copy" ), this, SLOT( copyFeature() ) );
  }
  else if ( !mLabelPositions.isEmpty() )
  {
    addAction( QIcon( ":/images/themes/default/mActionToggleEditing.svg" ), tr( "Edit" ), this, SLOT( editLabel() ) );
  }
  if ( !pickResult.feature.isValid() && mLabelPositions.isEmpty() )
  {
    if ( QgisApp::instance()->editCanPaste() )
    {
      addAction( QIcon( ":/images/themes/default/mActionEditPaste.png" ), tr( "Paste" ), this, SLOT( pasteFeature() ) );
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
  QMenu* measureMenu = new QMenu();
  addAction( tr( "Measure" ) )->setMenu( measureMenu );
  measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasure.png" ), tr( "Length" ), this, SLOT( measureLine() ) );
  measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureArea.png" ), tr( "Area" ), this, SLOT( measurePolygon() ) );
  measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureCircle.png" ), tr( "Circle" ), this, SLOT( measureCircle() ) );
  measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureAngle.png" ), tr( "Angle" ), this, SLOT( measureAngle() ) );
  measureMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureHeightProfile.png" ), tr( "Height profile" ), this, SLOT( measureHeightProfile() ) );

  QMenu* analysisMenu = new QMenu();
  addAction( tr( "Terrain analysis" ) )->setMenu( analysisMenu );
  analysisMenu->addAction( QIcon( ":/images/themes/default/slope.svg" ), tr( "Slope" ), this, SLOT( terrainSlope() ) );
  analysisMenu->addAction( QIcon( ":/images/themes/default/hillshade.svg" ), tr( "Hillshade" ), this, SLOT( terrainHillshade() ) );
  analysisMenu->addAction( QIcon( ":/images/themes/default/viewshed.svg" ), tr( "Viewshed" ), this, SLOT( terrainViewshed() ) );
  analysisMenu->addAction( QIcon( ":/images/themes/default/mActionMeasureHeightProfile.png" ), tr( "Line of sight" ), this, SLOT( terrainLineOfSight() ) );

  addAction( QIcon( ":/images/themes/default/mActionCopyCoordinatesToClipboard.png" ), tr( "Copy coordinates" ), this, SLOT( copyCoordinates() ) );
  addAction( QIcon( ":/images/themes/default/mActionSaveMapToClipboard.png" ), tr( "Copy map" ), this, SLOT( copyMap() ) );
  addAction( QIcon( ":/images/themes/default/mActionFilePrint.png" ), tr( "Print" ), this, SLOT( print() ) );
}

QgsMapCanvasContextMenu::~QgsMapCanvasContextMenu()
{
  delete mRubberBand;
}

void QgsMapCanvasContextMenu::featureAttributes()
{
  if ( mSelectedLayer )
  {
    QgsAttributeDialog *dialog = new QgsAttributeDialog( mSelectedLayer, &mSelectedFeature, false, QgisApp::instance() );
    dialog->show( true );
  }
}

void QgsMapCanvasContextMenu::editFeature()
{
  if ( mSelectedLayer == QgisApp::instance()->redlining()->getLayer() )
    QgisApp::instance()->redlining()->editFeature( mSelectedFeature );
  else if ( mSelectedLayer == QgisApp::instance()->gpsRouteEditor()->getLayer() )
    QgisApp::instance()->gpsRouteEditor()->editFeature( mSelectedFeature );
}

void QgsMapCanvasContextMenu::editLabel()
{
  if ( !mLabelPositions.isEmpty() )
  {
    QgsRedliningLayer* redliningLayer = QgisApp::instance()->redlining()->getLayer();
    QgsRedliningLayer* gpsLayer = QgisApp::instance()->gpsRouteEditor()->getLayer();
    if ( redliningLayer && redliningLayer->id() == mLabelPositions.first().layerID )
    {
      QgisApp::instance()->redlining()->editLabel( mLabelPositions.first() );
    }
    else if ( gpsLayer && gpsLayer->id() == mLabelPositions.first().layerID )
    {
      QgisApp::instance()->gpsRouteEditor()->editLabel( mLabelPositions.first() );
    }
  }
}

void QgsMapCanvasContextMenu::cutFeature()
{
  if ( mSelectedLayer && mSelectedLayer == QgisApp::instance()->redlining()->getLayer() )
  {
    QgsFeatureIds prevSelection = mSelectedLayer->selectedFeaturesIds();
    mSelectedLayer->setSelectedFeatures( QgsFeatureIds() << mSelectedFeature.id() );
    mSelectedLayer->startEditing();
    QgisApp::instance()->editCut( mSelectedLayer );
    mSelectedLayer->commitChanges();
    mSelectedLayer->setSelectedFeatures( prevSelection );
  }
}

void QgsMapCanvasContextMenu::copyFeature()
{
  if ( mSelectedLayer )
  {
    QgsFeatureIds prevSelection = mSelectedLayer->selectedFeaturesIds();
    mSelectedLayer->setSelectedFeatures( QgsFeatureIds() << mSelectedFeature.id() );
    QgisApp::instance()->editCopy( mSelectedLayer );
    mSelectedLayer->setSelectedFeatures( prevSelection );
  }
}

void QgsMapCanvasContextMenu::pasteFeature()
{
  QgisApp::instance()->editPaste( QgisApp::instance()->redliningLayer() );
}

void QgsMapCanvasContextMenu::drawPin()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mPinAnnotation );
}

void QgsMapCanvasContextMenu::drawPointMarker()
{
  QgisApp::instance()->redlining()->newPoint();
}

void QgsMapCanvasContextMenu::drawSquareMarker()
{
  QgisApp::instance()->redlining()->newSquare();
}

void QgsMapCanvasContextMenu::drawTriangleMarker()
{
  QgisApp::instance()->redlining()->newTriangle();
}

void QgsMapCanvasContextMenu::drawLine()
{
  QgisApp::instance()->redlining()->newLine();
}

void QgsMapCanvasContextMenu::drawRectangle()
{
  QgisApp::instance()->redlining()->newRectangle();
}

void QgsMapCanvasContextMenu::drawPolygon()
{
  QgisApp::instance()->redlining()->newPolygon();
}

void QgsMapCanvasContextMenu::drawCircle()
{
  QgisApp::instance()->redlining()->newCircle();
}

void QgsMapCanvasContextMenu::drawText()
{
  QgisApp::instance()->redlining()->newText();
}

void QgsMapCanvasContextMenu::measureLine()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mMeasureDist );
  if ( mSelectedLayer )
  {
    static_cast<QgsMeasureToolV2*>( QgisApp::instance()->mapTools()->mMeasureDist )->addGeometry( mSelectedFeature.geometry(), mSelectedLayer );
  }
}

void QgsMapCanvasContextMenu::measurePolygon()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mMeasureArea );
  if ( mSelectedLayer )
  {
    static_cast<QgsMeasureToolV2*>( QgisApp::instance()->mapTools()->mMeasureArea )->addGeometry( mSelectedFeature.geometry(), mSelectedLayer );
  }
}

void QgsMapCanvasContextMenu::measureCircle()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mMeasureCircle );
  if ( mSelectedLayer )
  {
    static_cast<QgsMeasureToolV2*>( QgisApp::instance()->mapTools()->mMeasureCircle )->addGeometry( mSelectedFeature.geometry(), mSelectedLayer );
  }
}

void QgsMapCanvasContextMenu::measureAngle()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mMeasureAngle );
  if ( mSelectedLayer )
  {
    static_cast<QgsMeasureToolV2*>( QgisApp::instance()->mapTools()->mMeasureAngle )->addGeometry( mSelectedFeature.geometry(), mSelectedLayer );
  }
}

void QgsMapCanvasContextMenu::measureHeightProfile()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mMeasureHeightProfile );
  if ( mSelectedLayer )
  {
    static_cast<QgsMeasureHeightProfileTool*>( QgisApp::instance()->mapTools()->mMeasureHeightProfile )->setGeometry( mSelectedFeature.geometry(), mSelectedLayer );
  }
}

void QgsMapCanvasContextMenu::terrainSlope()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mSlope );
}

void QgsMapCanvasContextMenu::terrainHillshade()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mHillshade );
}

void QgsMapCanvasContextMenu::terrainViewshed()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mViewshed );
}

void QgsMapCanvasContextMenu::terrainLineOfSight()
{
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mMeasureHeightProfile );
}

void QgsMapCanvasContextMenu::copyCoordinates()
{
  const QgsCoordinateReferenceSystem& mapCrs = QgisApp::instance()->mapCanvas()->mapSettings().destinationCrs();
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
  QgisApp::instance()->mapCanvas()->setMapTool( QgisApp::instance()->mapTools()->mDeleteItems );
}
