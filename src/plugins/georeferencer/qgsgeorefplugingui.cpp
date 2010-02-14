/***************************************************************************
     QgsGeorefPluginGui.cpp
     --------------------------------------
    Date                 : Sun Sep 16 12:03:52 AKDT 2007
    Copyright            : (C) 2007 by Gary E. Sherman
    Email                : sherman at mrcc dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include <QDesktopWidget>
#include <QDialogButtonBox>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QTextStream>

#include "qgisinterface.h"
#include "qgslegendinterface.h"
#include "qgsapplication.h"

#include "qgsmapcanvas.h"
#include "qgsmapcoordsdialog.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptoolzoom.h"
#include "qgsmaptoolpan.h"

#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "../../app/qgsrasterlayerproperties.h"

#include "qgsgeorefdatapoint.h"
#include "qgsgeoreftooladdpoint.h"
#include "qgsgeoreftooldeletepoint.h"
#include "qgsgeoreftoolmovepoint.h"

#include "qgsleastsquares.h"
#include "qgsgcplistwidget.h"

#include "qgsgeorefconfigdialog.h"
#include "qgsgeorefdescriptiondialog.h"
#include "qgstransformsettingsdialog.h"

#include "qgsgeorefplugingui.h"

QgsGeorefPluginGui::QgsGeorefPluginGui( QgisInterface* theQgisInterface, QWidget* parent, Qt::WFlags fl )
  : QMainWindow( parent, fl )
  , mTransformParam(QgsGeorefTransform::InvalidTransform)
  , mIface( theQgisInterface )
  , mLayer( 0 )
  , mMovingPoint(0)
  , mMapCoordsDialog(0)
  , mUseZeroForTrans(false)
  , mLoadInQgis(false)
{
  setupUi( this );

  createActions();
  createActionGroups();
  createMenus();
  createMapCanvas();
  createDockWidgets();
  createStatusBar();

  setAddPointTool();
  setupConnections();
  readSettings();

  mActionLinkGeorefToQGis->setEnabled(false);
  mActionLinkQGisToGeoref->setEnabled(false);

  mCanvas->clearExtentHistory(); // reset zoomnext/zoomlast
}

QgsGeorefPluginGui::~QgsGeorefPluginGui()
{
  QgsTransformSettingsDialog::resetSettings();
  clearGCPData();

  // delete layer (and don't signal it as it's our private layer)
  if ( mLayer )
  {
    QgsMapLayerRegistry::instance()->removeMapLayer( mLayer->getLayerID(), FALSE );
  }

  delete mToolZoomIn;
  delete mToolZoomOut;
  delete mToolPan;
  delete mToolAddPoint;
  delete mToolDeletePoint;
  delete mToolMovePoint;
}

// ----------------------------- protected --------------------------------- //
void QgsGeorefPluginGui::closeEvent(QCloseEvent *e)
{
  switch (checkNeedGCPSave())
  {
  case QgsGeorefPluginGui::GCPSAVE:
    if (mGCPpointsFileName.isEmpty())
      saveGCPsDialog();
    else
      saveGCPs();
    writeSettings();
    e->accept();
    return;
  case QgsGeorefPluginGui::GCPSILENTSAVE:
    if (!mGCPpointsFileName.isEmpty())
      saveGCPs();
    return;
  case QgsGeorefPluginGui::GCPDISCARD:
    writeSettings();
    e->accept();
    return;
  case QgsGeorefPluginGui::GCPCANCEL:
    e->ignore();
    return;
  }
}

// -------------------------- private slots -------------------------------- //
// File slots
void QgsGeorefPluginGui::openRaster()
{
  //  clearLog();
  switch (checkNeedGCPSave())
  {
  case QgsGeorefPluginGui::GCPSAVE:
    saveGCPsDialog();
    break;
  case QgsGeorefPluginGui::GCPSILENTSAVE:
    if (!mGCPpointsFileName.isEmpty())
      saveGCPs();
    break;
  case QgsGeorefPluginGui::GCPDISCARD:
    break;
  case QgsGeorefPluginGui::GCPCANCEL:
    return;
  }

  QSettings s;
  QString dir = s.value( "/Plugin-GeoReferencer/rasterdirectory" ).toString();
  if ( dir.isEmpty() )
    dir = ".";

  QString otherFiles = tr("All other files (*)");
  QString lastUsedFilter = s.value("/Plugin-GeoReferencer/lastusedfilter", otherFiles).toString();

  QString filters;
  QgsRasterLayer::buildSupportedRasterFileFilter( filters );
  filters.prepend(otherFiles + ";;");
  filters.chop(otherFiles.size() + 2);
  mRasterFileName = QFileDialog::getOpenFileName(this, tr("Open raster"), dir, filters, &lastUsedFilter);
  mModifiedRasterFileName = "";

  if (mRasterFileName.isEmpty())
    return;

  QString errMsg;
  if (!QgsRasterLayer::isValidRasterFileName(mRasterFileName, errMsg))
  {
    QString msg = tr( "%1 is not a supported raster data source" ).arg( mRasterFileName );

    if ( errMsg.size() > 0 )
      msg += "\n" + errMsg;

    QMessageBox::information( this, tr( "Unsupported Data Source" ), msg );
    return;
  }

  QFileInfo fileInfo( mRasterFileName );
  s.setValue( "/Plugin-GeoReferencer/rasterdirectory", fileInfo.path() );
  s.setValue("/Plugin-GeoReferencer/lastusedfilter", lastUsedFilter);

  mGeorefTransform.selectTransformParametrisation(mTransformParam);
  statusBar()->showMessage(tr("Raster loaded: %1").arg(mRasterFileName));
  setWindowTitle(tr("Georeferencer - %1").arg(fileInfo.fileName()));

  //  showMessageInLog(tr("Input raster"), mRasterFileName);

  //delete old points
  clearGCPData();

  //delete any old rasterlayers
  if ( mLayer )
    QgsMapLayerRegistry::instance()->removeMapLayer( mLayer->getLayerID(), FALSE );

  //add new raster layer
  mLayer = new QgsRasterLayer( mRasterFileName, "Raster" );;

  // add to map layer registry, do not signal addition
  // so layer is not added to legend
  QgsMapLayerRegistry::instance()->addMapLayer( mLayer, FALSE );

  // add layer to map canvas
  QList<QgsMapCanvasLayer> layers;
  layers.append(QgsMapCanvasLayer( mLayer ) );
  mCanvas->setLayerSet( layers );

  // load previously added points
  mGCPpointsFileName = mRasterFileName + ".points";
  loadGCPs();

  mCanvas->setExtent( mLayer->extent() );
  mCanvas->refresh();
  mIface->mapCanvas()->refresh();

  mActionLinkGeorefToQGis->setChecked(false);
  mActionLinkQGisToGeoref->setChecked(false);
  mActionLinkGeorefToQGis->setEnabled(false);
  mActionLinkQGisToGeoref->setEnabled(false);

  mCanvas->clearExtentHistory(); // reset zoomnext/zoomlast
}

void QgsGeorefPluginGui::doGeoreference()
{
  if (georeference())
  {
    if (mLoadInQgis)
    {
      if (QgsGeorefTransform::Linear == mTransformParam)
      {
        mIface->addRasterLayer(mRasterFileName);
      }
      else
      {
        mIface->addRasterLayer(mModifiedRasterFileName);
      }

      //      showMessageInLog(tr("Modified raster saved in"), mModifiedRasterFileName);
      //      saveGCPs();

      //      mTransformParam = QgsGeorefTransform::InvalidTransform;
      //      mGeorefTransform.selectTransformParametrisation(mTransformParam);
      //      mGCPListWidget->setGeorefTransform(&mGeorefTransform);
      //      mTransformParamLabel->setText(tr("Transform: ") + convertTransformEnumToString(mTransformParam));

      mActionLinkGeorefToQGis->setEnabled(false);
      mActionLinkQGisToGeoref->setEnabled(false);
    }
  }
}

bool QgsGeorefPluginGui::getTransformSettings()
{
  QgsTransformSettingsDialog d(mRasterFileName, mModifiedRasterFileName, mPoints.size());
  if (!d.exec())
  {
    return false;
  }

  d.getTransformSettings(mTransformParam, mResamplingMethod, mCompressionMethod,
                         mModifiedRasterFileName, mProjection, mUseZeroForTrans, mLoadInQgis);
  mTransformParamLabel->setText(tr("Transform: ") + convertTransformEnumToString(mTransformParam));
  mGeorefTransform.selectTransformParametrisation(mTransformParam);
  mGCPListWidget->setGeorefTransform(&mGeorefTransform);
  mWorldFileName = guessWorldFileName(mRasterFileName);

  //  showMessageInLog(tr("Output raster"), mModifiedRasterFileName.isEmpty() ? tr("Non set") : mModifiedRasterFileName);
  //  showMessageInLog(tr("Target projection"), mProjection.isEmpty() ? tr("Non set") : mProjection);
  //  logTransformOptions();
  //  logRequaredGCPs();

  if (QgsGeorefTransform::InvalidTransform != mTransformParam)
  {
    mActionLinkGeorefToQGis->setEnabled(true);
    mActionLinkQGisToGeoref->setEnabled(true);
  }
  else
  {
    mActionLinkGeorefToQGis->setEnabled(false);
    mActionLinkQGisToGeoref->setEnabled(false);
  }

  return true;
}

void QgsGeorefPluginGui::generateGDALScript()
{
  if (!checkReadyGeoref())
    return;

  if (QgsGeorefTransform::Linear != mTransformParam
      && QgsGeorefTransform::Helmert != mTransformParam)
  {
    QString gdalwarpCommand;
    QString resamplingStr = convertResamplingEnumToString(mResamplingMethod);
    if (QgsGeorefTransform::ThinPlateSpline == mTransformParam)
      gdalwarpCommand = gdalwarpCommandTPS(resamplingStr, mCompressionMethod, mUseZeroForTrans);
    else
      gdalwarpCommand = gdalwarpCommandGCP(resamplingStr, mCompressionMethod, mUseZeroForTrans, polynomeOrder(mTransformParam));

    showGDALScript(2, gdal_translateCommand().toAscii().data(), gdalwarpCommand.toAscii().data());
  }
  else
  {
    QMessageBox::information(this, tr("Info"), tr("GDAL scripting is not supported for %1 transformation")
                             .arg(convertTransformEnumToString(mTransformParam)));
  }
}

// Edit slots
void QgsGeorefPluginGui::setAddPointTool()
{
  mCanvas->setMapTool( mToolAddPoint );
}

void QgsGeorefPluginGui::setDeletePointTool()
{
  mCanvas->setMapTool( mToolDeletePoint );
}

void QgsGeorefPluginGui::setMovePointTool()
{
  mCanvas->setMapTool(mToolMovePoint);
}

// View slots
void QgsGeorefPluginGui::setPanTool()
{
  mCanvas->setMapTool( mToolPan );
}

void QgsGeorefPluginGui::setZoomInTool()
{
  mCanvas->setMapTool( mToolZoomIn );
}

void QgsGeorefPluginGui::setZoomOutTool()
{
  mCanvas->setMapTool( mToolZoomOut );
}

void QgsGeorefPluginGui::zoomToLayerTool()
{
  if ( mLayer )
  {
    mCanvas->setExtent( mLayer->extent() );
    mCanvas->refresh();
  }
}

void QgsGeorefPluginGui::zoomToLast()
{
  mCanvas->zoomToPreviousExtent();
}

void QgsGeorefPluginGui::zoomToNext()
{
  mCanvas->zoomToNextExtent();
}

void QgsGeorefPluginGui::linkGeorefToQGis( bool link )
{
  if ( link )
  {
    if (QgsGeorefTransform::InvalidTransform != mTransformParam)
      extentsChangedGeorefCanvas();
    else
      mActionLinkGeorefToQGis->setEnabled(false);
  }
}

void QgsGeorefPluginGui::linkQGisToGeoref( bool link )
{
  if ( link )
  {
    if (QgsGeorefTransform::InvalidTransform != mTransformParam)
      extentsChangedQGisCanvas();
    else
      mActionLinkQGisToGeoref->setEnabled(false);
  }
}

// GCPs slots
void QgsGeorefPluginGui::addPoint( const QgsPoint& pixelCoords, const QgsPoint& mapCoords,
                                   bool enable, bool refreshCanvas/*, bool verbose*/ )
{
  QgsGeorefDataPoint* pnt = new QgsGeorefDataPoint( mCanvas, mIface->mapCanvas(),
                                                    pixelCoords, mapCoords, enable );
  mPoints.append( pnt );
  mGCPsDirty = true;
  mGCPListWidget->setGCPList(&mPoints);
  if ( refreshCanvas )
  {
    mCanvas->refresh();
    mIface->mapCanvas()->refresh();
  }

  connect(mCanvas, SIGNAL(extentsChanged()), pnt, SLOT(updateCoords()));

  //  if (verbose)
  //    logRequaredGCPs();
}

void QgsGeorefPluginGui::deleteDataPoint( const QPoint &coords )
{
  for (QgsGCPList::iterator it = mPoints.begin(); it != mPoints.end(); ++it)
  {
    QgsGeorefDataPoint* pt = *it;
    if (/*pt->pixelCoords() == coords ||*/ pt->contains(coords)) // first operand for removing from GCP table
    {
      int row = mPoints.indexOf(*it);
      mGCPListWidget->model()->removeRow(row);

      delete *it;
      mPoints.erase( it );

      mGCPListWidget->updateGCPList();
      //      mGCPListWidget->setGCPList(&mPoints);
      //      logRequaredGCPs();

      mCanvas->refresh();
      break;
    }
  }
}

void QgsGeorefPluginGui::deleteDataPoint(int index)
{
  mGCPListWidget->model()->removeRow(index);
  delete mPoints.takeAt(index);
  mGCPListWidget->updateGCPList();
}

void QgsGeorefPluginGui::selectPoint(const QPoint &p)
{
  for (QgsGCPList::iterator it = mPoints.begin(); it != mPoints.end(); ++it)
  {
    if ((*it)->contains(p))
    {
      mMovingPoint = *it;
      break;
    }
  }
}

void QgsGeorefPluginGui::movePoint(const QPoint &p)
{
  if (mMovingPoint)
  {
    mMovingPoint->moveTo(p);
    mGCPListWidget->updateGCPList();
  }
}

void QgsGeorefPluginGui::releasePoint(const QPoint &p)
{
  mMovingPoint = 0;
}

void QgsGeorefPluginGui::showCoordDialog(const QgsPoint &pixelCoords)
{
  if (mLayer && !mMapCoordsDialog)
  {
    mMapCoordsDialog = new QgsMapCoordsDialog(mIface->mapCanvas(), pixelCoords, this);
    connect(mMapCoordsDialog, SIGNAL(pointAdded(const QgsPoint &, const QgsPoint &)),
            this, SLOT(addPoint(const QgsPoint &, const QgsPoint &)));
    mMapCoordsDialog->show();
  }
}

void QgsGeorefPluginGui::loadGCPsDialog()
{
  QString selectedFile = mRasterFileName.isEmpty() ? "" : mRasterFileName + ".points";
  mGCPpointsFileName = QFileDialog::getOpenFileName(this, tr("Load GCP points"),
                                                    selectedFile, "GCP file (*.points)");
  if (mGCPpointsFileName.isEmpty())
    return;

  loadGCPs();
}

void QgsGeorefPluginGui::saveGCPsDialog()
{
  if (mPoints.isEmpty())
  {
    QMessageBox::information(this, tr("Info"), tr("No GCP points to save"));
    return;
  }

  QString selectedFile = mRasterFileName.isEmpty() ? "" : mRasterFileName + ".points";
  mGCPpointsFileName = QFileDialog::getSaveFileName(this, tr("Save GCP points"),
                                                    selectedFile,
                                                    "GCP file (*.points)");

  if (mGCPpointsFileName.isEmpty())
    return;

  if ( mGCPpointsFileName.right( 7 ) != ".points" )
    mGCPpointsFileName += ".points";

  saveGCPs();
}

// Settings slots
void QgsGeorefPluginGui::showRasterPropertiesDialog()
{
  if (mLayer)
  {
    mIface->showLayerProperties(mLayer);
  }
  else
  {
    QMessageBox::information(this, tr("Info"), tr("Please load raster to be georeferenced"));
  }
}

void QgsGeorefPluginGui::showGeorefConfigDialog()
{
  QgsGeorefConfigDialog config;
  config.exec();
  mCanvas->refresh();
  mIface->mapCanvas()->refresh();
}

// Info slots
void QgsGeorefPluginGui::contextHelp()
{
  QgsGeorefDescriptionDialog dlg( this );
  dlg.exec();
}

// Comfort slots
void QgsGeorefPluginGui::jumpToGCP(uint theGCPIndex)
{
  if ((int)theGCPIndex >= mPoints.size())
  {
    return;
  }

  // qgsmapcanvas doesn't seem to have a method for recentering the map
  QgsRectangle ext = mCanvas->extent();

  QgsPoint center = ext.center();
  QgsPoint new_center = mPoints[theGCPIndex]->pixelCoords();

  QgsPoint diff(new_center.x() - center.x(), new_center.y() - center.y());
  QgsRectangle new_extent(ext.xMinimum() + diff.x(), ext.yMinimum() + diff.y(),
                          ext.xMaximum() + diff.x(), ext.yMaximum() + diff.y());
  mCanvas->setExtent(new_extent);
  mCanvas->refresh();
}

// This slot is called whenever the georeference canvas changes the displayed extent
void QgsGeorefPluginGui::extentsChangedGeorefCanvas()
{
  // Guard against endless recursion by ping-pong updates
  if (mExtentsChangedRecursionGuard)
  {
    return;
  }

  if (mActionLinkGeorefToQGis->isChecked())
  {
    if (!updateGeorefTransform())
    {
      return;
    }

    // Reproject the georeference plugin canvas into world coordinates and fit axis aligned bounding box
    QgsRectangle boundingBox = transformViewportBoundingBox(mCanvas->extent(), mGeorefTransform, true);

    mExtentsChangedRecursionGuard = true;
    // Just set the whole extent for now
    // TODO: better fitting function which acounts for differing aspect ratios etc.
    mIface->mapCanvas()->setExtent(boundingBox);
    mIface->mapCanvas()->refresh();
    mExtentsChangedRecursionGuard = false;
  }
}

// This slot is called whenever the qgis main canvas changes the displayed extent
void QgsGeorefPluginGui::extentsChangedQGisCanvas()
{
  // Guard against endless recursion by ping-pong updates
  if (mExtentsChangedRecursionGuard)
  {
    return;
  }

  if (mActionLinkQGisToGeoref->isChecked())
  {
    // Update transform if necessary
    if (!updateGeorefTransform())
    {
      return;
    }

    // Reproject the canvas into raster coordinates and fit axis aligned bounding box
    QgsRectangle boundingBox = transformViewportBoundingBox(mIface->mapCanvas()->extent(), mGeorefTransform, false);

    mExtentsChangedRecursionGuard = true;
    // Just set the whole extent for now
    // TODO: better fitting function which acounts for differing aspect ratios etc.
    mCanvas->setExtent(boundingBox);
    mCanvas->refresh();
    mExtentsChangedRecursionGuard = false;
  }
}

// Canvas info slots (copy/pasted from QGIS :) )
void QgsGeorefPluginGui::showMouseCoords(QgsPoint p)
{
  mCoordsLabel->setText( p.toString( mMousePrecisionDecimalPlaces ) );
  // Set minimum necessary width
  if ( mCoordsLabel->width() > mCoordsLabel->minimumWidth() )
  {
    mCoordsLabel->setMinimumWidth( mCoordsLabel->width() );
  }
}

void QgsGeorefPluginGui::updateMouseCoordinatePrecision()
{
  // Work out what mouse display precision to use. This only needs to
  // be when the s change or the zoom level changes. This
  // function needs to be called every time one of the above happens.

  // Get the display precision from the project s
  bool automatic = QgsProject::instance()->readBoolEntry( "PositionPrecision", "/Automatic" );
  int dp = 0;

  if ( automatic )
  {
    // Work out a suitable number of decimal places for the mouse
    // coordinates with the aim of always having enough decimal places
    // to show the difference in position between adjacent pixels.
    // Also avoid taking the log of 0.
    if ( mCanvas->mapUnitsPerPixel() != 0.0 )
      dp = static_cast<int>( ceil( -1.0 * log10( mCanvas->mapUnitsPerPixel() ) ) );
  }
  else
    dp = QgsProject::instance()->readNumEntry( "PositionPrecision", "/DecimalPlaces" );

  // Keep dp sensible
  if ( dp < 0 ) dp = 0;

  mMousePrecisionDecimalPlaces = dp;
}

// ------------------------------ private ---------------------------------- //
// Gui
void QgsGeorefPluginGui::createActions()
{
  // File actions
  mActionOpenRaster->setIcon(getThemeIcon("/mActionOpenRaster.png"));
  connect(mActionOpenRaster, SIGNAL(triggered()), this, SLOT(openRaster()));

  mActionStartGeoref->setIcon(getThemeIcon("/mActionStartGeoref.png"));
  connect(mActionStartGeoref, SIGNAL(triggered()), this, SLOT(doGeoreference()));

  mActionGDALScript->setIcon(getThemeIcon("/mActionGDALScript.png"));
  connect(mActionGDALScript, SIGNAL(triggered()), this, SLOT(generateGDALScript()));

  mActionLoadGCPpoints->setIcon( getThemeIcon( "/mActionLoadGCPpoints.png" ) );
  connect( mActionLoadGCPpoints, SIGNAL(triggered()), this, SLOT(loadGCPsDialog()));

  mActionSaveGCPpoints->setIcon(getThemeIcon("/mActionSaveGCPpointsAs.png"));
  connect(mActionSaveGCPpoints, SIGNAL(triggered()), this, SLOT(saveGCPsDialog()));

  mActionTransformSettings->setIcon(getThemeIcon("/mActionTransformSettings.png"));
  connect(mActionTransformSettings, SIGNAL(triggered()), this, SLOT(getTransformSettings()));

  // Edit actions
  mActionAddPoint->setIcon(getThemeIcon( "/mActionCapturePoint.png" ));
  connect( mActionAddPoint, SIGNAL( triggered() ), this, SLOT( setAddPointTool() ) );

  mActionDeletePoint->setIcon(getThemeIcon( "/mActionDeleteSelected.png" ));
  connect( mActionDeletePoint, SIGNAL( triggered() ), this, SLOT( setDeletePointTool() ) );

  mActionMoveGCPPoint->setIcon(getThemeIcon("/mActionMoveGCPPoint.png"));
  connect(mActionMoveGCPPoint, SIGNAL(triggered()), this, SLOT(setMovePointTool()));

  // View actions
  mActionPan->setIcon(getThemeIcon( "/mActionPan.png" ));
  connect( mActionPan, SIGNAL( triggered() ), this, SLOT( setPanTool() ) );

  mActionZoomIn->setIcon(getThemeIcon( "/mActionZoomIn.png" ));
  connect( mActionZoomIn, SIGNAL( triggered() ), this, SLOT( setZoomInTool() ) );

  mActionZoomOut->setIcon(getThemeIcon( "/mActionZoomOut.png" ));
  connect( mActionZoomOut, SIGNAL( triggered() ), this, SLOT( setZoomOutTool() ) );

  mActionZoomToLayer->setIcon(getThemeIcon( "/mActionZoomToLayer.png" ));
  connect( mActionZoomToLayer, SIGNAL( triggered() ), this, SLOT( zoomToLayerTool() ) );

  mActionZoomLast->setIcon(getThemeIcon("/mActionZoomLast.png"));
  connect(mActionZoomLast, SIGNAL(triggered()), this, SLOT(zoomToLast()));

  mActionZoomNext->setIcon(getThemeIcon("/mActionZoomNext.png"));
  connect(mActionZoomNext, SIGNAL(triggered()), this, SLOT(zoomToNext()));

  mActionLinkGeorefToQGis->setIcon( getThemeIcon( "/mActionLinkGeorefToQGis.png" ) );
  connect( mActionLinkGeorefToQGis, SIGNAL( triggered( bool ) ), this, SLOT( linkGeorefToQGis( bool ) ) );

  mActionLinkQGisToGeoref->setIcon( getThemeIcon( "/mActionLinkQGisToGeoref.png" ) );
  connect( mActionLinkQGisToGeoref, SIGNAL( triggered(bool) ), this, SLOT( linkQGisToGeoref( bool ) ) );

  // Settings actions
  mActionRasterProperties->setIcon(getThemeIcon("/mActionRasterProperties.png"));
  connect(mActionRasterProperties, SIGNAL(triggered()), this, SLOT(showRasterPropertiesDialog()));

  mActionGeorefConfig->setIcon(getThemeIcon("/mActionGeorefConfig.png"));
  connect(mActionGeorefConfig, SIGNAL(triggered()), this, SLOT(showGeorefConfigDialog()));

  // Help actions
  mActionHelp = new QAction(tr("Help"), this);
  connect(mActionHelp, SIGNAL(triggered()), this, SLOT(contextHelp()));

  mActionQuit->setIcon(getThemeIcon("/mActionQuit.png"));
  mActionQuit->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::CTRL + Qt::Key_Q)
                            << QKeySequence(Qt::Key_Escape));
  connect(mActionQuit, SIGNAL(triggered()), this, SLOT(close()));
}

void QgsGeorefPluginGui::createActionGroups()
{
  QActionGroup *mapToolGroup = new QActionGroup( this );
  mActionPan->setCheckable( true );
  mapToolGroup->addAction( mActionPan );
  mActionZoomIn->setCheckable( true );
  mapToolGroup->addAction( mActionZoomIn );
  mActionZoomOut->setCheckable( true );
  mapToolGroup->addAction( mActionZoomOut );

  mActionAddPoint->setCheckable( true );
  mapToolGroup->addAction( mActionAddPoint );
  mActionDeletePoint->setCheckable( true );
  mapToolGroup->addAction( mActionDeletePoint );
  mActionMoveGCPPoint->setCheckable(true);
  mapToolGroup->addAction(mActionMoveGCPPoint);
}

void QgsGeorefPluginGui::createMapCanvas()
{
  // set up the canvas
  mCanvas = new QgsMapCanvas( this, "georefCanvas" );
  mCanvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  mCanvas->setCanvasColor(Qt::white);
  mCanvas->setMinimumWidth( 400 );
  setCentralWidget(mCanvas);

  // set up map tools
  mToolZoomIn = new QgsMapToolZoom( mCanvas, false /* zoomOut */ );
  mToolZoomIn->setAction( mActionZoomIn );

  mToolZoomOut = new QgsMapToolZoom( mCanvas, true /* zoomOut */ );
  mToolZoomOut->setAction( mActionZoomOut );

  mToolPan = new QgsMapToolPan( mCanvas );
  mToolPan->setAction( mActionPan );

  mToolAddPoint = new QgsGeorefToolAddPoint( mCanvas );
  mToolAddPoint->setAction( mActionAddPoint );
  connect(mToolAddPoint, SIGNAL(showCoordDailog(const QgsPoint &)),
          this, SLOT(showCoordDialog(const QgsPoint &)));

  mToolDeletePoint = new QgsGeorefToolDeletePoint( mCanvas );
  mToolDeletePoint->setAction( mActionDeletePoint );
  connect(mToolDeletePoint, SIGNAL(deleteDataPoint( const QPoint & )),
          this, SLOT( deleteDataPoint(const QPoint& )));

  mToolMovePoint = new QgsGeorefToolMovePoint(mCanvas);
  mToolMovePoint->setAction(mActionMoveGCPPoint);
  connect(mToolMovePoint, SIGNAL(pointPressed(const QPoint &)),
          this, SLOT(selectPoint(const QPoint &)));
  connect(mToolMovePoint, SIGNAL(pointMoved(const QPoint &)),
          this, SLOT(movePoint(const QPoint &)));
  connect(mToolMovePoint, SIGNAL(pointReleased(const QPoint &)),
          this, SLOT(releasePoint(const QPoint &)));

  QSettings s;
  int action = s.value( "/qgis/wheel_action", 0 ).toInt();
  double zoomFactor = s.value( "/qgis/zoom_factor", 2 ).toDouble();
  mCanvas->setWheelAction(( QgsMapCanvas::WheelAction ) action, zoomFactor );

  mExtentsChangedRecursionGuard = false;

  mGeorefTransform.selectTransformParametrisation(QgsGeorefTransform::Linear);
  mGCPsDirty = true;

  // Connect main canvas and georef canvas signals so we are aware if any of the viewports change
  // (used by the map follow mode)
  connect( mCanvas, SIGNAL( extentsChanged() ), this, SLOT( extentsChangedGeorefCanvas() ) );
  connect( mIface->mapCanvas(), SIGNAL( extentsChanged() ), this, SLOT( extentsChangedQGisCanvas() ) );
}

void QgsGeorefPluginGui::createMenus()
{
  // Get platform for menu layout customization (Gnome, Kde, Mac, Win)
  QDialogButtonBox::ButtonLayout layout =
      QDialogButtonBox::ButtonLayout( style()->styleHint( QStyle::SH_DialogButtonLayout, 0, this ) );

  mPanelMenu = new QMenu(tr("Panels"));
  mPanelMenu->addAction(dockWidgetGCPpoints->toggleViewAction());
  //  mPanelMenu->addAction(dockWidgetLogView->toggleViewAction());

  mToolbarMenu = new QMenu(tr("Toolbars"));
  mToolbarMenu->addAction(toolBarFile->toggleViewAction());
  mToolbarMenu->addAction(toolBarEdit->toggleViewAction());
  mToolbarMenu->addAction(toolBarView->toggleViewAction());

  // View menu
  if (layout != QDialogButtonBox::KdeLayout)
  {
    menuView->addSeparator();
    menuView->addMenu(mPanelMenu);
    menuView->addMenu(mToolbarMenu);
  }
  else // if ( layout == QDialogButtonBox::KdeLayout )
  {
    menuSettings->addSeparator();
    menuSettings->addMenu( mPanelMenu );
    menuSettings->addMenu( mToolbarMenu );
  }

  menuBar()->addAction(tr("Help"), this, SLOT(contextHelp()));
}

void QgsGeorefPluginGui::createDockWidgets()
{
  //  mLogViewer = new QPlainTextEdit;
  //  mLogViewer->setReadOnly(true);
  //  mLogViewer->setWordWrapMode(QTextOption::NoWrap);
  //  dockWidgetLogView->setWidget(mLogViewer);

  mGCPListWidget = new QgsGCPListWidget(this);
  mGCPListWidget->setGeorefTransform(&mGeorefTransform);
  dockWidgetGCPpoints->setWidget(mGCPListWidget);

  connect(mGCPListWidget, SIGNAL(jumpToGCP(uint)), this, SLOT(jumpToGCP(uint)));
  connect(mGCPListWidget, SIGNAL(replaceDataPoint(QgsGeorefDataPoint*,int)),
          this, SLOT(replaceDataPoint(QgsGeorefDataPoint*,int)));
  connect(mGCPListWidget, SIGNAL(deleteDataPoint(int)),
          this, SLOT(deleteDataPoint(int)));
}

void QgsGeorefPluginGui::createStatusBar()
{
  QFont myFont( "Arial", 9 );

  mCoordsLabel = new QLabel( QString(), statusBar() );
  mCoordsLabel->setFont( myFont );
  mCoordsLabel->setMinimumWidth( 10 );
  mCoordsLabel->setMaximumHeight( 20 );
  mCoordsLabel->setMaximumWidth(100);
  mCoordsLabel->setMargin( 3 );
  mCoordsLabel->setAlignment( Qt::AlignCenter );
  mCoordsLabel->setFrameStyle( QFrame::NoFrame );
  mCoordsLabel->setText( tr( "Coordinate: " ) );
  mCoordsLabel->setToolTip( tr( "Current map coordinate" ) );
  statusBar()->addPermanentWidget( mCoordsLabel, 0 );

  mTransformParamLabel = new QLabel( statusBar() );
  mTransformParamLabel->setFont( myFont );
  mTransformParamLabel->setMinimumWidth( 10 );
  mTransformParamLabel->setMaximumHeight( 20 );
  mTransformParamLabel->setMargin( 3 );
  mTransformParamLabel->setAlignment( Qt::AlignCenter );
  mTransformParamLabel->setFrameStyle( QFrame::NoFrame );
  mTransformParamLabel->setText( tr( "Transform: " ) + convertTransformEnumToString(mTransformParam) );
  mTransformParamLabel->setToolTip( tr( "Current transform parametrisation" ) );
  statusBar()->addPermanentWidget( mTransformParamLabel, 0 );
}

void QgsGeorefPluginGui::setupConnections()
{
  connect(mCanvas, SIGNAL(xyCoordinates(QgsPoint)), this, SLOT(showMouseCoords(QgsPoint)));
  connect(mCanvas, SIGNAL(scaleChanged(double)), this, SLOT(updateMouseCoordinatePrecision()));

  // Connect status from ZoomLast/ZoomNext to corresponding action
  connect( mCanvas, SIGNAL( zoomLastStatusChanged( bool ) ), mActionZoomLast, SLOT( setEnabled( bool ) ) );
  connect( mCanvas, SIGNAL( zoomNextStatusChanged( bool ) ), mActionZoomNext, SLOT( setEnabled( bool ) ) );
}

// Settings
void QgsGeorefPluginGui::readSettings()
{
  QSettings s;
  QRect georefRect = QApplication::desktop()->screenGeometry(mIface->mainWindow());
  resize(s.value("/Plugin-GeoReferencer/size", QSize(georefRect.width() / 2 + georefRect.width() / 5,
                                                     mIface->mainWindow()->height())).toSize());
  move(s.value("/Plugin-GeoReferencer/pos", QPoint(parentWidget()->width() / 2 - width() / 2, 0)).toPoint());
  restoreState(s.value("/Plugin-GeoReferencer/uistate").toByteArray());

  // warp options
  mResamplingMethod = (QgsImageWarper::ResamplingMethod)s.value("/Plugin-GeoReferencer/resamplingmethod",
                                                                QgsImageWarper::NearestNeighbour).toInt();
  mCompressionMethod = s.value("/Plugin-GeoReferencer/compressionmethod", "NONE").toString();
  mUseZeroForTrans = s.value("/Plugin-GeoReferencer/usezerofortrans", false).toBool();
}

void QgsGeorefPluginGui::writeSettings()
{
  QSettings s;
  s.setValue("/Plugin-GeoReferencer/pos", pos());
  s.setValue("/Plugin-GeoReferencer/size", size());
  s.setValue("/Plugin-GeoReferencer/uistate", saveState());

  // warp options
  s.setValue("/Plugin-GeoReferencer/transformparam", mTransformParam);
  s.setValue("/Plugin-GeoReferencer/resamplingmethod", mResamplingMethod);
  s.setValue("/Plugin-GeoReferencer/compressionmethod", mCompressionMethod);
  s.setValue("/Plugin-GeoReferencer/usezerofortrans", mUseZeroForTrans);
}

// GCP points
void QgsGeorefPluginGui::loadGCPs(/*bool verbose*/)
{
  QFile pointFile( mGCPpointsFileName );
  if ( pointFile.open( QIODevice::ReadOnly ) )
  {
    clearGCPData();

    QTextStream points( &pointFile );
    QString line = points.readLine();
    int i = 0;
    while ( !points.atEnd() )
    {
      line = points.readLine();
      QStringList ls;
      if (line.contains(QRegExp(","))) // in previous format "\t" is delemiter of points in new - ","
      {
        // points from new georeferencer
        ls = line.split(",");
      }
      else
      {
        // points from prev georeferencer
        ls = line.split("\t");
      }

      QgsPoint mapCoords( ls.at(0).toDouble(), ls.at(1).toDouble() ); // map x,y
      QgsPoint pixelCoords( ls.at(2).toDouble(), ls.at(3).toDouble() ); // pixel x,y
      if (ls.count() == 5)
      {
        bool enable = ls.at(4).toInt();
        addPoint( pixelCoords, mapCoords, enable, false/*, verbose*/ ); // enabled
      }
      else
        addPoint(pixelCoords, mapCoords, true, false);

      ++i;
    }

    mInitialPoints = mPoints;
    //    showMessageInLog(tr("GCP points loaded from"), mGCPpointsFileName);
    mCanvas->refresh();
  }
}

void QgsGeorefPluginGui::saveGCPs()
{
  QFile pointFile( mGCPpointsFileName );
  if ( pointFile.open( QIODevice::WriteOnly ))
  {
    QTextStream points( &pointFile );
    points << "mapX,mapY,pixelX,pixelY,enable" << endl;
    foreach (QgsGeorefDataPoint *pt, mPoints)
    {
      points << ( QString( "%1,%2,%3,%4,%5" ).arg( pt->mapCoords().x(), 0, 'f', 15 ).
                  arg( pt->mapCoords().y(), 0, 'f', 15 ).arg( pt->pixelCoords().x(), 0, 'f', 15 ).
                  arg( pt->pixelCoords().y(), 0, 'f', 15 ) ).arg( pt->isEnabled() ) << endl;
    }

    mInitialPoints = mPoints;
  }
  else
  {
    QMessageBox::information(this, tr("Info"), tr("Enable to open GCP points file %1").arg(mGCPpointsFileName));
    return;
  }

  //  showMessageInLog(tr("GCP points saved in"), mGCPpointsFileName);
}

QgsGeorefPluginGui::SaveGCPs QgsGeorefPluginGui::checkNeedGCPSave()
{
  if (0 == mPoints.count())
    return QgsGeorefPluginGui::GCPDISCARD;

  if (!equalGCPlists(mInitialPoints, mPoints))
  {
    QMessageBox::StandardButton a = QMessageBox::information(this, tr("Save GCPs"),
                                                             tr("Save GCP points?"),
                                                             QMessageBox::Save | QMessageBox::Discard
                                                             | QMessageBox::Cancel);
    if (a == QMessageBox::Save)
    {
      return QgsGeorefPluginGui::GCPSAVE;
    }
    else if (a == QMessageBox::Cancel)
    {
      return QgsGeorefPluginGui::GCPCANCEL;
    }
    else if (a == QMessageBox::Discard)
    {
      return QgsGeorefPluginGui::GCPDISCARD;
    }
  }

  return QgsGeorefPluginGui::GCPSILENTSAVE;
}

// Georeference
bool QgsGeorefPluginGui::georeference()
{
  if (!checkReadyGeoref())
    return false;

  if (QgsGeorefTransform::Linear == mGeorefTransform.transformParametrisation())
  {
    QgsPoint origin;
    double pixelXSize, pixelYSize;
    if (!mGeorefTransform.getLinearOriginScale(origin, pixelXSize, pixelYSize))
    {
      QMessageBox::information( this, tr( "Info" ),
                                tr( "Failed to get linear transform parameters" ));
      {
        return false;
      }
    }

    if ( !mWorldFileName.isEmpty() )
    {
      if ( QFile::exists( mWorldFileName ) )
      {
        int r = QMessageBox::question( this, tr("World file exists"),
                                       tr( "<p>The selected file already seems to have a "
                                           "world file! Do you want to replace it with the "
                                           "new world file?</p>"),
                                       QMessageBox::Yes | QMessageBox::Default,
                                       QMessageBox::No | QMessageBox::Escape );
        if ( r == QMessageBox::No )
          return false;
        else
          QFile::remove( mWorldFileName );
      }
    }
    else
    {
      return false;
    }

    return writeWorldFile(origin, pixelXSize, pixelYSize);
  }
  else // Helmert, Polinom 1, Polinom 2, Polinom 3
  {
    QgsImageWarper warper(this);
    int res = warper.warpFile( mRasterFileName, mModifiedRasterFileName, mGeorefTransform,
                               mResamplingMethod, mUseZeroForTrans, mCompressionMethod, mProjection);
    if (res == 0) // fault to compute GCP transform
    {
      //TODO: be more specific in the error message
      QMessageBox::information( this, tr( "Info" ), tr( "Failed to compute GCP transform: Transform is not solvable" ) );
      return false;
    }
    else if ( res == -1) // operation canceled
    {
      QFileInfo fi(mModifiedRasterFileName);
      fi.dir().remove(mModifiedRasterFileName);
      return false;
    }
    else // 1 all right
    {
      return true;
    }
  }
}

bool QgsGeorefPluginGui::writeWorldFile(QgsPoint origin, double pixelXSize, double pixelYSize)
{
  // write the world file
  QFile file( mWorldFileName );
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    QMessageBox::critical( this, tr( "Error" ),
                           tr( "Could not write to " ) + mWorldFileName );
    return false;
  }
  QTextStream stream( &file );
  stream << QString::number( pixelXSize, 'f', 15 ) << endl
      << 0 << endl
      << 0 << endl
      << QString::number( -pixelYSize, 'f', 15 ) << endl
      << QString::number(  origin.x(), 'f', 15 ) << endl
      << QString::number(  origin.y(), 'f', 15 ) << endl;
  return true;
}

// Gdal script
void QgsGeorefPluginGui::showGDALScript(int argNum...)
{
  QString script;
  va_list vl;
  va_start(vl, argNum);
  while (argNum--)
  {
    script.append(va_arg(vl, char *));
    script.append("\n");
  }
  va_end(vl);

  // create window to show gdal script
  QDialogButtonBox *bbxGdalScript = new QDialogButtonBox(QDialogButtonBox::Cancel, Qt::Horizontal, this);
  QPushButton *pbnCopyInClipBoard = new QPushButton(getThemeIcon("/mPushButtonEditPaste.png"),
                                                    tr("Copy in clipboard"), bbxGdalScript);
  bbxGdalScript->addButton(pbnCopyInClipBoard, QDialogButtonBox::AcceptRole);

  QPlainTextEdit *pteScript = new QPlainTextEdit();
  pteScript->setReadOnly(true);
  pteScript->setWordWrapMode(QTextOption::NoWrap);
  pteScript->setPlainText(tr("%1").arg(script));

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(pteScript);
  layout->addWidget(bbxGdalScript);

  QDialog *dlgShowGdalScrip = new QDialog(this);
  dlgShowGdalScrip->setWindowTitle(tr("GDAL script"));
  dlgShowGdalScrip->setLayout(layout);

  connect(bbxGdalScript, SIGNAL(accepted()), dlgShowGdalScrip, SLOT(accept()));
  connect(bbxGdalScript, SIGNAL(rejected()), dlgShowGdalScrip, SLOT(reject()));

  if (dlgShowGdalScrip->exec() == QDialog::Accepted)
  {
    QApplication::clipboard()->setText(pteScript->toPlainText());
  }
}

QString QgsGeorefPluginGui::gdal_translateCommand(bool generateTFW)
{
  QStringList gdalCommand;
  gdalCommand << "gdal_translate" << "-of GTiff";
  if ( generateTFW )
  {
    // say gdal generate associated ESRI world file
    gdalCommand << "-co TFW=YES";
  }

  foreach (QgsGeorefDataPoint *pt, mPoints)
  {
    gdalCommand << QString("-gcp %1 %2 %3 %4").arg(pt->pixelCoords().x()).arg(pt->pixelCoords().y())
        .arg(pt->mapCoords().x()).arg(pt->mapCoords().y());
  }

  QFileInfo rasterFileInfo(mRasterFileName);
  mTranslatedRasterFileName = QDir::tempPath() + "/" + rasterFileInfo.fileName();
  gdalCommand << mRasterFileName << mTranslatedRasterFileName;

  return gdalCommand.join(" ");
}

QString QgsGeorefPluginGui::gdalwarpCommandGCP(QString resampling, QString compress,
                                               bool useZeroForTrans, int order)
{
  QStringList gdalCommand;
  gdalCommand << "gdalwarp" << "-r" << resampling << "-order" << QString::number(order)
      << "-co COMPRESS" << compress << (useZeroForTrans ? "-dstalpha" : "")
      << mTranslatedRasterFileName << mModifiedRasterFileName;

  return gdalCommand.join(" ");
}

QString QgsGeorefPluginGui::gdalwarpCommandTPS(QString resampling, QString compress, bool useZeroForTrans)
{
  QStringList gdalCommand;
  gdalCommand << "gdalwarp" << "-r" << resampling << "-tps"
      << "-co COMPRESS" << compress << (useZeroForTrans ? "-dstalpha" : "")
      << mTranslatedRasterFileName << mModifiedRasterFileName;

  return gdalCommand.join(" ");
}

// Log
//void QgsGeorefPluginGui::showMessageInLog(const QString &description, const QString &msg)
//{
//  QString logItem = QString("<code>%1: %2</code>").arg(description).arg(msg);
//
//  mLogViewer->appendHtml(logItem);
//}
//
//void QgsGeorefPluginGui::clearLog()
//{
//  mLogViewer->clear();
//}

// Helpers
bool QgsGeorefPluginGui::checkReadyGeoref()
{
  if (mRasterFileName.isEmpty())
  {
    QMessageBox::information(this, tr("Info"), tr("Please load raster to be georeferenced"));
    return false;
  }

  bool ok = false;
  while (!ok)
  {
    if (QgsGeorefTransform::InvalidTransform == mTransformParam)
    {
      QMessageBox::information(this, tr("Info"), tr("Please set transformation type"));
      if (!getTransformSettings())
        return false;

      continue;
    }

    if (mModifiedRasterFileName.isEmpty() && QgsGeorefTransform::Linear != mTransformParam)
    {
      QMessageBox::information(this, tr("Info"), tr("Please set output raster name"));
      if (!getTransformSettings())
        return false;

      continue;
    }

    if (mPoints.count() < (int)mGeorefTransform.getMinimumGCPCount())
    {
      QMessageBox::information(this, tr("Info"), tr("%1 requires at least %2 GCPs. Please define more")
                               .arg(convertTransformEnumToString(mTransformParam)).arg(mGeorefTransform.getMinimumGCPCount()));
      if (!getTransformSettings())
        return false;

      continue;
    }

    ok = true;
  }

  // Update the transform if necessary
  if (!updateGeorefTransform())
  {
    QMessageBox::information(this, tr( "Info" ), tr( "Failed to compute GCP transform: Transform is not solvable" ) );
    //    logRequaredGCPs();
    return false;
  }

  return true;
}

bool QgsGeorefPluginGui::updateGeorefTransform()
{
  if (mGCPsDirty || !mGeorefTransform.parametersInitialized())
  {
    std::vector<QgsPoint> mapCoords, pixelCoords;
    if (mGCPListWidget->gcpList())
      mGCPListWidget->gcpList()->createGCPVectors(mapCoords, pixelCoords);
    else
      return false;

    // Parametrize the transform with GCPs
    if (!mGeorefTransform.updateParametersFromGCPs(mapCoords, pixelCoords))
    {
      return false;
    }

    mGCPsDirty = false;
  }
  return true;
}

// Samples the given rectangle at numSamples per edge.
// Returns an axis aligned bounding box which contains the transformed samples.
QgsRectangle QgsGeorefPluginGui::transformViewportBoundingBox(const QgsRectangle &canvasExtent,
                                                              const QgsGeorefTransform &t,
                                                              bool rasterToWorld, uint numSamples)
{
  double minX, minY;
  double maxX, maxY;
  minX = minY =  std::numeric_limits<double>::max();
  maxX = maxY = -std::numeric_limits<double>::max();

  double oX = canvasExtent.xMinimum();
  double oY = canvasExtent.yMinimum();
  double dX = canvasExtent.xMaximum();
  double dY = canvasExtent.yMaximum();
  double stepX = numSamples ? (dX-oX)/(numSamples-1) : 0.0;
  double stepY = numSamples ? (dY-oY)/(numSamples-1) : 0.0;
  for (uint s = 0u;  s < numSamples; s++)
  {
    for (uint edge = 0; edge < 4; edge++) {
      QgsPoint src, raster;
      switch (edge) {
      case 0: src = QgsPoint(oX + (double)s*stepX, oY); break;
      case 1: src = QgsPoint(oX + (double)s*stepX, dY); break;
      case 2: src = QgsPoint(oX, oY + (double)s*stepY); break;
      case 3: src = QgsPoint(dX, oY + (double)s*stepY); break;
      }
      t.transform(src, raster, rasterToWorld);
      minX = std::min(raster.x(), minX);
      maxX = std::max(raster.x(), maxX);
      minY = std::min(raster.y(), minY);
      maxY = std::max(raster.y(), maxY);
    }
  }
  return QgsRectangle(minX, minY, maxX, maxY);
}

QString QgsGeorefPluginGui::convertTransformEnumToString(QgsGeorefTransform::TransformParametrisation transform)
{
  switch (transform)
  {
  case QgsGeorefTransform::Linear:
    return tr("Linear");
  case QgsGeorefTransform::Helmert:
    return tr("Helmert");
  case QgsGeorefTransform::PolynomialOrder1:
    return tr("Polynomial 1");
  case QgsGeorefTransform::PolynomialOrder2:
    return tr("Polynomial 2");
  case QgsGeorefTransform::PolynomialOrder3:
    return tr("Polynomial 3");
  case QgsGeorefTransform::ThinPlateSpline:
    return tr("Thin plate spline (TPS)");
  default:
    return tr("Not set");
  }
}

QString QgsGeorefPluginGui::convertResamplingEnumToString(QgsImageWarper::ResamplingMethod resampling)
{
  switch (resampling)
  {
  case QgsImageWarper::NearestNeighbour:
    return tr("near");
  case QgsImageWarper::Bilinear:
    return tr("bilinear");
  case QgsImageWarper::Cubic:
    return tr("cubic");
  case QgsImageWarper::CubicSpline:
    return tr("cubicspline");
  case QgsImageWarper::Lanczos:
    return tr("lanczons");
  }
  return "";
}

int QgsGeorefPluginGui::polynomeOrder(QgsGeorefTransform::TransformParametrisation transform)
{
  switch (transform)
  {
  case QgsGeorefTransform::PolynomialOrder1:
    return 1;
  case QgsGeorefTransform::PolynomialOrder2:
    return 2;
  case QgsGeorefTransform::PolynomialOrder3:
    return 3;
  default:
    return -1;
  }
}

QString QgsGeorefPluginGui::guessWorldFileName( const QString &rasterFileName )
{
  QString worldFileName = "";
  int point = rasterFileName.lastIndexOf( '.' );
  if ( point != -1 && point != rasterFileName.length() - 1 )
    worldFileName = rasterFileName.left( point + 1 ) + "wld";

  return worldFileName;
}

// Note this code is duplicated from qgisapp.cpp because
// I didnt want to make plugins on qgsapplication [TS]
QIcon QgsGeorefPluginGui::getThemeIcon( const QString &theName )
{
  if ( QFile::exists( QgsApplication::activeThemePath() + theName ) )
  {
    return QIcon( QgsApplication::activeThemePath() + theName );
  }
  else if (QFile::exists(QgsApplication::defaultThemePath() + theName ))
  {
    return QIcon( QgsApplication::defaultThemePath() + theName );
  }
  else
  {
    return QIcon(":/icons" + theName);
  }
}

bool QgsGeorefPluginGui::checkFileExisting(QString fileName, QString title, QString question)
{
  if ( !fileName.isEmpty() )
  {
    if ( QFile::exists( fileName ) )
    {
      int r = QMessageBox::question( this, title, question,
                                     QMessageBox::Yes | QMessageBox::Default,
                                     QMessageBox::No | QMessageBox::Escape );
      if ( r == QMessageBox::No )
        return false;
      else
        QFile::remove( fileName );
    }
  }

  return true;
}

bool QgsGeorefPluginGui::equalGCPlists(const QgsGCPList &list1, const QgsGCPList &list2)
{
  if (list1.count() != list2.count())
    return false;

  int count = list1.count();
  int j = 0;
  for (int i = 0; i < count; ++i)
  {
    QgsGeorefDataPoint *p1 = list1.at(i);
    QgsGeorefDataPoint *p2 = list2.at(j);
    qDebug() << "p1" << "pix" << p1->pixelCoords().toString() << "map" << p1->mapCoords().toString();
    qDebug() << "p2" << "pix" << p2->pixelCoords().toString() << "map" << p2->mapCoords().toString();
    if (p1->mapCoords() != p2->mapCoords())
      return false;
    ++j;
  }

  return true;
}

//void QgsGeorefPluginGui::logTransformOptions()
//{
//  showMessageInLog(tr("Interpolation"), convertResamplingEnumToString(mResamplingMethod));
//  showMessageInLog(tr("Compression method"), mCompressionMethod);
//  showMessageInLog(tr("Zero for transparency"), mUseZeroForTrans ? "true" : "false");
//}
//
//void QgsGeorefPluginGui::logRequaredGCPs()
//{
//  if (mGeorefTransform.getMinimumGCPCount() != 0)
//  {
//    if ((uint)mPoints.size() >= mGeorefTransform.getMinimumGCPCount())
//      showMessageInLog(tr("Info"), tr("For georeferencing requared at least %1 GCP points")
//                       .arg(mGeorefTransform.getMinimumGCPCount()));
//    else
//      showMessageInLog(tr("Critical"), tr("For georeferencing requared at least %1 GCP points")
//                       .arg(mGeorefTransform.getMinimumGCPCount()));
//  }
//}

void QgsGeorefPluginGui::clearGCPData()
{
  int rowCount = mGCPListWidget->model()->rowCount();
  mGCPListWidget->model()->removeRows(0, rowCount);

  qDeleteAll(mPoints);
  mPoints.clear();

  mIface->mapCanvas()->refresh();
}
