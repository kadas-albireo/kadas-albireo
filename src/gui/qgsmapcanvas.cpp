/***************************************************************************
  qgsmapcanvas.cpp  -  description
  -------------------
begin                : Sun Jun 30 2002
copyright            : (C) 2002 by Gary E.Sherman
email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id: qgsmapcanvas.cpp 5400 2006-04-30 20:14:08Z wonder $ */


#include <QtGlobal>
#include <QApplication>
#include <QCursor>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QRect>
#include <QResizeEvent>
#include <QString>
#include <QStringList>
#include <QWheelEvent>

#include "qgis.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvasmap.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptoolpan.h"
#include "qgsmaptoolzoom.h"
#include "qgsmaptopixel.h"
#include "qgsmapoverviewcanvas.h"
#include "qgsmaprender.h"
#include "qgsmessageviewer.h"
#include "qgsproject.h"
#include "qgsrubberband.h"
#include "qgsvectorlayer.h"
#include <math.h>

/**  @DEPRECATED: to be deleted, stuff from here should be moved elsewhere */
class QgsMapCanvas::CanvasProperties
{
  public:

    CanvasProperties() : panSelectorDown( false ) { }

    //!Flag to indicate status of mouse button
    bool mouseButtonDown;

    //! Last seen point of the mouse
    QPoint mouseLastXY;

    //! Beginning point of a rubber band
    QPoint rubberStartPoint;

    //! Flag to indicate the pan selector key is held down by user
    bool panSelectorDown;

};



  QgsMapCanvas::QgsMapCanvas(QWidget * parent, const char *name)
: QGraphicsView(parent),
  mCanvasProperties(new CanvasProperties)
{
  mScene = new QGraphicsScene();
  setScene(mScene);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  
  mCurrentLayer = NULL;
  mMapOverview = NULL;
  mMapTool = NULL;
  mLastNonZoomMapTool = NULL;
  
  mDrawing = false;
  mFrozen = false;
  mDirty = true;
  
  setWheelAction(WheelZoom);
  
  // by default, the canvas is rendered
  mRenderFlag = true;
  
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);
  
  mMapRender = new QgsMapRender;

  // create map canvas item which will show the map
  mMap = new QgsMapCanvasMap(this);
  mScene->addItem(mMap);
  mScene->update(); // porting??
  
  moveCanvasContents(TRUE);
  
  connect(mMapRender, SIGNAL(updateMap()), this, SLOT(updateMap()));
  connect(mMapRender, SIGNAL(drawError(QgsMapLayer*)), this, SLOT(showError(QgsMapLayer*)));
  
  // project handling
  connect(QgsProject::instance(), SIGNAL(readProject(const QDomDocument &)),
          this, SLOT(readProject(const QDomDocument &)));
  connect(QgsProject::instance(), SIGNAL(writeProject(QDomDocument &)),
          this, SLOT(writeProject(QDomDocument &)));
  
} // QgsMapCanvas ctor


QgsMapCanvas::~QgsMapCanvas()
{
  if (mMapTool)
  {
    mMapTool->deactivate();
    mMapTool = NULL;
  }
  mLastNonZoomMapTool = NULL;
  
  // delete canvas items prior to deleteing the canvas
  // because they might try to update canvas when it's
  // already being destructed, ends with segfault
  QList<QGraphicsItem*> list = mScene->items();
  QList<QGraphicsItem*>::iterator it = list.begin();
  while (it != list.end())
  {
    QGraphicsItem* item = *it;
    delete item;
    it++;
  }
  
  delete mScene;

  delete mMapRender;
  // mCanvasProperties auto-deleted via std::auto_ptr
  // CanvasProperties struct has its own dtor for freeing resources
  
} // dtor

void QgsMapCanvas::enableAntiAliasing(bool theFlag)
{
  mMap->enableAntiAliasing(theFlag);
  if (mMapOverview)
    mMapOverview->enableAntiAliasing(theFlag);
} // anti aliasing

void QgsMapCanvas::useQImageToRender(bool theFlag)
{
  mMap->useQImageToRender(theFlag);
}

QgsMapCanvasMap* QgsMapCanvas::map()
{
  return mMap;
}

QgsMapRender* QgsMapCanvas::mapRender()
{
  return mMapRender;
}


QgsMapLayer* QgsMapCanvas::getZpos(int index)
{
  std::deque<QString>& layers = mMapRender->layerSet();
  if (index >= 0 && index < (int) layers.size())
    return QgsMapLayerRegistry::instance()->mapLayer(layers[index]);
  else
    return NULL;
}


void QgsMapCanvas::setCurrentLayer(QgsMapLayer* layer)
{
  mCurrentLayer = layer;
}

double QgsMapCanvas::getScale()
{
  return mMapRender->scale();
} // getScale

void QgsMapCanvas::setDirty(bool dirty)
{
  mDirty = dirty;
}

bool QgsMapCanvas::isDirty() const
{
  return mDirty;
}



bool QgsMapCanvas::isDrawing()
{
  return mDrawing;
} // isDrawing


// return the current coordinate transform based on the extents and
// device size
QgsMapToPixel * QgsMapCanvas::getCoordinateTransform()
{
  return mMapRender->coordXForm();
}

void QgsMapCanvas::setLayerSet(QList<QgsMapCanvasLayer>& layers)
{
  int i;
  
  // create layer set
  std::deque<QString> layerSet, layerSetOverview;
  
  for (i = 0; i < layers.size(); i++)
  {
    QgsMapCanvasLayer& lyr = layers[i];
    if (lyr.visible())
    {
      layerSet.push_back(lyr.layer()->getLayerID());
    }
    if (lyr.inOverview())
    {
      layerSetOverview.push_back(lyr.layer()->getLayerID());
    }
  }
  
  std::deque<QString>& layerSetOld = mMapRender->layerSet();
  
  bool layerSetChanged = (layerSetOld != layerSet);
  
  // update only if needed
  if (layerSetChanged)
  {
    for (i = 0; i < layerCount(); i++)
    {
      disconnect(getZpos(i), SIGNAL(repaintRequested()), this, SLOT(refresh()));
      disconnect(getZpos(i), SIGNAL(selectionChanged()), this, SLOT(refresh()));
    }
    
    mMapRender->setLayerSet(layerSet);
  
    for (i = 0; i < layerCount(); i++)
    {
      connect(getZpos(i), SIGNAL(repaintRequested()), this, SLOT(refresh()));
      connect(getZpos(i), SIGNAL(selectionChanged()), this, SLOT(refresh()));
    }
  }
  

  if (mMapOverview)
  {
    std::deque<QString>& layerSetOvOld = mMapOverview->layerSet();
    if (layerSetOvOld != layerSetOverview)
    {
      mMapOverview->setLayerSet(layerSetOverview);
      updateOverview();
    }
  }
  
  if (layerSetChanged)
  {
    QgsDebugMsg("Layers have changed, refreshing");
    emit layersChanged();
  
    refresh();
  }

} // addLayer

void QgsMapCanvas::setOverview(QgsMapOverviewCanvas* overview)
{
  if (mMapOverview)
  {
    // disconnect old map overview if exists
    disconnect(mMapRender, SIGNAL(projectionsEnabled(bool)),
               mMapOverview, SLOT(projectionsEnabled(bool)));
    disconnect(mMapRender, SIGNAL(destinationSrsChanged()),
               mMapOverview, SLOT(destinationSrsChanged()));

    // map overview is not owned by map canvas so don't delete it...
  }
  
  if (overview)
  {
    mMapOverview = overview;
  
    // connect to the map render to copy its projection settings
    connect(mMapRender, SIGNAL(projectionsEnabled(bool)),
            overview,     SLOT(projectionsEnabled(bool)));
    connect(mMapRender, SIGNAL(destinationSrsChanged()),
            overview,     SLOT(destinationSrsChanged()));
  }
}


void QgsMapCanvas::updateOverview()
{
  // redraw overview
  if (mMapOverview)
  {
    mMapOverview->refresh();
  }
}


QgsMapLayer* QgsMapCanvas::currentLayer()
{
  return mCurrentLayer;
}


void QgsMapCanvas::refresh()
{
  if (mRenderFlag)
  {
    clear();
    
    // schedule update of map
    mMap->update();
  }
} // refresh


void QgsMapCanvas::render()
{
  QgsDebugMsg("Starting rendering");

  // Tell the user we're going to be a while
  QApplication::setOverrideCursor(Qt::WaitCursor);

  mMap->render();
  mDirty = false;

  // notify any listeners that rendering is complete
  QPainter p;
  p.begin(&mMap->pixmap());
  emit renderComplete(&p);
  p.end();
  
  // notifies current map tool
  if (mMapTool)
    mMapTool->renderComplete();

  // Tell the user we've finished going to be a while
  QApplication::restoreOverrideCursor();

} // render


//the format defaults to "PNG" if not specified
void QgsMapCanvas::saveAsImage(QString theFileName, QPixmap * theQPixmap, QString theFormat)
{
  //
  //check if the optional QPaintDevice was supplied
  //
  if (theQPixmap != NULL)
  {
    // render
    QPainter painter;
    painter.begin(theQPixmap);
    mMapRender->render(&painter);
    painter.end();
    
    theQPixmap->save(theFileName,theFormat.toLocal8Bit().data());
  }
  else //use the map view
  {
    mMap->pixmap().save(theFileName,theFormat.toLocal8Bit().data());
  }
} // saveAsImage



QgsRect QgsMapCanvas::extent() const
{
  return mMapRender->extent();
} // extent

QgsRect QgsMapCanvas::fullExtent() const
{
  return mMapRender->fullExtent();
} // extent

void QgsMapCanvas::updateFullExtent()
{
  // projection settings have changed
  
  QgsDebugMsg("updating full extent");

  mMapRender->updateFullExtent();
  if (mMapOverview)
  {
    mMapOverview->updateFullExtent();
    mMapOverview->reflectChangedExtent();
    updateOverview();
  }
  refresh();
}


void QgsMapCanvas::setExtent(QgsRect const & r)
{
  QgsRect current = extent();
  mMapRender->setExtent(r);
  emit extentsChanged();
  updateScale();
  if (mMapOverview)
    mMapOverview->reflectChangedExtent();
  mLastExtent = current;
  
  // notify canvas items of change
  updateCanvasItemsPositions();

} // setExtent
  

void QgsMapCanvas::updateScale()
{
  double scale = mMapRender->scale();
  QString myScaleString = tr("Scale ");
  int thePrecision = 0;
  if (scale == 0)
    myScaleString = "";
  else if (scale >= 1)
    myScaleString += QString("1: ") + QString::number(scale,'f',thePrecision);
  else
    myScaleString += QString::number(1.0/scale, 'f', thePrecision) + QString(": 1");
  emit scaleChanged(myScaleString);
}


void QgsMapCanvas::clear()
{
  // Indicate to the next paint event that we need to rebuild the canvas contents
  setDirty(TRUE);

} // clear



void QgsMapCanvas::zoomFullExtent()
{
  QgsRect extent = fullExtent();
  // If the full extent is an empty set, don't do the zoom
  if (!extent.isEmpty())
    setExtent(extent);
  refresh();

} // zoomFullExtent



void QgsMapCanvas::zoomPreviousExtent()
{
  QgsRect current = extent();
  setExtent(mLastExtent);
  mLastExtent = current;
  refresh();
} // zoomPreviousExtent


bool QgsMapCanvas::projectionsEnabled()
{
  return mMapRender->projectionsEnabled();
}

void QgsMapCanvas::mapUnitsChanged()
{
  // We assume that if the map units have changed, the changed value
  // will be accessible from QgsMapRender

  // And then force a redraw of the scale number in the status bar
  updateScale();

  // And then redraw the map to force the scale bar to update
  // itself. This is less than ideal as the entire map gets redrawn
  // just to get the scale bar to redraw itself. If we ask the scale
  // bar to redraw itself without redrawing the map, the existing
  // scale bar is not removed, and we end up with two scale bars in
  // the same location. This can perhaps be fixed when/if the scale
  // bar is done as a transparent layer on top of the map canvas.
  refresh();
}

void QgsMapCanvas::zoomToSelected()
{
  QgsVectorLayer *lyr = dynamic_cast < QgsVectorLayer * >(mCurrentLayer);

  if (lyr)
  {
    QgsRect rect = mMapRender->layerExtentToOutputExtent(lyr, lyr->boundingBoxOfSelected());

    // no selected features, only one selected point feature 
    //or two point features with the same x- or y-coordinates
    if(rect.isEmpty())
    {
      return;
    }
    //zoom to an area
    else
    {
      // Expand rect to give a bit of space around the selected
      // objects so as to keep them clear of the map boundaries
      rect.scale(1.1);
      setExtent(rect);
      refresh();
      return;
    }
  }
} // zoomToSelected

void QgsMapCanvas::keyPressEvent(QKeyEvent * e)
{
  QgsDebugMsg("keyPress event");

  emit keyPressed(e);

  if (mCanvasProperties->mouseButtonDown || mCanvasProperties->panSelectorDown)
    return;

  QPainter paint;
  QPen     pen(Qt::gray);
  QgsPoint ll, ur;

  if (! mCanvasProperties->mouseButtonDown )
  {
    // Don't want to interfer with mouse events

    QgsRect currentExtent = mMapRender->extent();
    double dx = fabs((currentExtent.xMax()- currentExtent.xMin()) / 4);
    double dy = fabs((currentExtent.yMax()- currentExtent.yMin()) / 4);

    switch ( e->key() ) 
    {
      case Qt::Key_Left:
        QgsDebugMsg("Pan left");

        currentExtent.setXmin(currentExtent.xMin() - dx);
        currentExtent.setXmax(currentExtent.xMax() - dx);
        setExtent(currentExtent);
        refresh();
        break;

      case Qt::Key_Right:
        QgsDebugMsg("Pan right");

        currentExtent.setXmin(currentExtent.xMin() + dx);
        currentExtent.setXmax(currentExtent.xMax() + dx);
        setExtent(currentExtent);
        refresh();
        break;

      case Qt::Key_Up:
        QgsDebugMsg("Pan up");

        currentExtent.setYmax(currentExtent.yMax() + dy);
        currentExtent.setYmin(currentExtent.yMin() + dy);
        setExtent(currentExtent);
        refresh();
        break;

      case Qt::Key_Down:
        QgsDebugMsg("Pan down");
        
        currentExtent.setYmax(currentExtent.yMax() - dy);
        currentExtent.setYmin(currentExtent.yMin() - dy);
        setExtent(currentExtent);
        refresh();
        break;

      case Qt::Key_Space:
        QgsDebugMsg("Pressing pan selector");

        //mCanvasProperties->dragging = true;
        if ( ! e->isAutoRepeat() )
        {
          mCanvasProperties->panSelectorDown = true;
          mCanvasProperties->rubberStartPoint = mCanvasProperties->mouseLastXY;
        }
        break;

      default:
        // Pass it on
        e->ignore();
        
        QgsDebugMsg("Ignoring key: " + QString::number(e->key()));

    }
  }
} //keyPressEvent()

void QgsMapCanvas::keyReleaseEvent(QKeyEvent * e)
{
  QgsDebugMsg("keyRelease event");

  switch( e->key() )
  {
    case Qt::Key_Space:
      if ( !e->isAutoRepeat() && mCanvasProperties->panSelectorDown)
      {
        QgsDebugMsg("Releaseing pan selector");
        
        mCanvasProperties->panSelectorDown = false;
        panActionEnd(mCanvasProperties->mouseLastXY);
      }
      break;

    default:
      // Pass it on
      e->ignore();
      
      QgsDebugMsg("Ignoring key release: " + QString::number(e->key()));
  }

  emit keyReleased(e);

} //keyReleaseEvent()


void QgsMapCanvas::mousePressEvent(QMouseEvent * e)
{
  // call handler of current map tool
  if (mMapTool)
    mMapTool->canvasPressEvent(e);  
  
  if (mCanvasProperties->panSelectorDown)
    return;

  mCanvasProperties->mouseButtonDown = true;
  mCanvasProperties->rubberStartPoint = e->pos();

} // mousePressEvent


void QgsMapCanvas::mouseReleaseEvent(QMouseEvent * e)
{
  // call handler of current map tool
  if (mMapTool)
  {
    // right button was pressed in zoom tool? return to previous non zoom tool
    if (e->button() == Qt::RightButton && mMapTool->isZoomTool())
    {
      QgsDebugMsg("Right click in map tool zoom or pan, last tool is " +
            QString(mLastNonZoomMapTool ? "not null." : "null.") );

      // change to older non-zoom tool
      if (mLastNonZoomMapTool)
      {
        QgsMapTool* t = mLastNonZoomMapTool;
        mLastNonZoomMapTool = NULL;
        setMapTool(t);
      }
      return;
    }
    mMapTool->canvasReleaseEvent(e);
  }

  
  mCanvasProperties->mouseButtonDown = false;

  if (mCanvasProperties->panSelectorDown)
    return;

} // mouseReleaseEvent

void QgsMapCanvas::resizeEvent(QResizeEvent * e)
{
  int width = e->size().width(), height = e->size().height();
//  int width = visibleWidth(), height = visibleHeight();
  mScene->setSceneRect(QRectF(0,0,width, height));

  mMap->resize(QSize(width,height));

  // notify canvas items of change
  updateCanvasItemsPositions();
  
  updateScale();
  refresh();
} // resizeEvent


void QgsMapCanvas::updateCanvasItemsPositions()
{
  QList<QGraphicsItem*> list = mScene->items();
  QList<QGraphicsItem*>::iterator it = list.begin();
  while (it != list.end())
  {
    QgsMapCanvasItem* item = dynamic_cast<QgsMapCanvasItem*>(*it);
    
    if (item)
    {
      item->updatePosition();
    }
  
    it++;
  }
}


void QgsMapCanvas::wheelEvent(QWheelEvent *e)
{
  // Zoom the map canvas in response to a mouse wheel event. Moving the
  // wheel forward (away) from the user zooms in
  
  QgsDebugMsg("Wheel event delta " + QString::number(e->delta()));

  switch (mWheelAction)
  {
    case WheelZoom:
      // zoom without changing extent
      zoom(e->delta() > 0);
      break;
      
    case WheelZoomAndRecenter:
      // zoom and don't change extent
      zoomWithCenter(e->x(), e->y(), e->delta() > 0);
      break;
      
    case WheelNothing:
      // well, nothing!
      break;
  }
}

void QgsMapCanvas::setWheelAction(WheelAction action, double factor)
{
  mWheelAction = action;
  mWheelZoomFactor = factor;
}

void QgsMapCanvas::zoom(bool zoomIn)
{
  double scaleFactor = (zoomIn ? 1/mWheelZoomFactor : mWheelZoomFactor);
  
  QgsRect r = mMapRender->extent();
  r.scale(scaleFactor);
  setExtent(r);
  refresh();
}

void QgsMapCanvas::zoomWithCenter(int x, int y, bool zoomIn)
{
  double scaleFactor = (zoomIn ? 1/mWheelZoomFactor : mWheelZoomFactor);

  // transform the mouse pos to map coordinates
  QgsPoint center  = getCoordinateTransform()->toMapPoint(x, y);
  QgsRect r = mMapRender->extent();
  r.scale(scaleFactor, &center);
  setExtent(r);
  refresh();
}


void QgsMapCanvas::mouseMoveEvent(QMouseEvent * e)
{
  mCanvasProperties->mouseLastXY = e->pos();

  if (mCanvasProperties->panSelectorDown)
  {
    panAction(e);
  }

  // call handler of current map tool
  if (mMapTool)
    mMapTool->canvasMoveEvent(e);
    
  // show x y on status bar
  QPoint xy = e->pos();
  QgsPoint coord = getCoordinateTransform()->toMapCoordinates(xy);
  emit xyCoordinates(coord);
} // mouseMoveEvent



/** Sets the map tool currently being used on the canvas */
void QgsMapCanvas::setMapTool(QgsMapTool* tool)
{
  if (!tool)
    return;
  
  if (mMapTool)
    mMapTool->deactivate();
  
  if (tool->isZoomTool() && mMapTool && !mMapTool->isZoomTool())
  {        
    // if zoom or pan tool will be active, save old tool
    // to bring it back on right click
    // (but only if it wasn't also zoom or pan tool)
    mLastNonZoomMapTool = mMapTool;
  }
  else
  {
    mLastNonZoomMapTool = NULL;
  }
  
  // set new map tool and activate it
  mMapTool = tool;
  if (mMapTool)
    mMapTool->activate();

} // setMapTool

void QgsMapCanvas::unsetMapTool(QgsMapTool* tool)
{
  if (mMapTool && mMapTool == tool)
  {
    mMapTool->deactivate();
    mMapTool = NULL;
  }

  if (mLastNonZoomMapTool && mLastNonZoomMapTool == tool)
  {
    mLastNonZoomMapTool = NULL;
  }
} 

/** Write property of QColor bgColor. */
void QgsMapCanvas::setCanvasColor(const QColor & theColor)
{
  // background of map's pixmap
  mMap->setBgColor(theColor);
  
  // background of the QGraphicsView
  QBrush bgBrush(theColor);
  setBackgroundBrush(bgBrush);
  /*QPalette palette;
  palette.setColor(backgroundRole(), theColor);
  setPalette(palette);*/
  
  // background of QGraphicsScene
  mScene->setBackgroundBrush(bgBrush);
} // setbgColor


int QgsMapCanvas::layerCount() const
{
  return mMapRender->layerSet().size();
} // layerCount



void QgsMapCanvas::layerStateChange()
{
  // called when a layer has changed visibility setting
  
  refresh();

} // layerStateChange



void QgsMapCanvas::freeze(bool frz)
{
  mFrozen = frz;
} // freeze

bool QgsMapCanvas::isFrozen()
{
  return mFrozen;
} // freeze


QPixmap& QgsMapCanvas::canvasPixmap()
{
  return mMap->pixmap();
} // canvasPixmap



double QgsMapCanvas::mupp() const
{
  return mMapRender->mupp();
} // mupp


void QgsMapCanvas::setMapUnits(QGis::units u)
{
  QgsDebugMsg("Setting map units to " + QString::number(static_cast<int>(u)) );

  mMapRender->setMapUnits(u);
}


QGis::units QgsMapCanvas::mapUnits() const
{
  return mMapRender->mapUnits();
}


void QgsMapCanvas::setRenderFlag(bool theFlag)
{
  mRenderFlag = theFlag;
  // render the map
  if(mRenderFlag)
  {
    refresh();
  }
}

void QgsMapCanvas::connectNotify( const char * signal )
{
  QgsDebugMsg("QgsMapCanvas connected to " + QString(signal));
} //  QgsMapCanvas::connectNotify( const char * signal )



QgsMapTool* QgsMapCanvas::mapTool()
{
  return mMapTool;
}

void QgsMapCanvas::panActionEnd(QPoint releasePoint)
{
  // move map image and other items to standard position
  moveCanvasContents(TRUE); // TRUE means reset
  
  // use start and end box points to calculate the extent
  QgsPoint start = getCoordinateTransform()->toMapCoordinates(mCanvasProperties->rubberStartPoint);
  QgsPoint end = getCoordinateTransform()->toMapCoordinates(releasePoint);

  double dx = fabs(end.x() - start.x());
  double dy = fabs(end.y() - start.y());

  // modify the extent
  QgsRect r = mMapRender->extent();

  if (end.x() < start.x())
  {
    r.setXmin(r.xMin() + dx);
    r.setXmax(r.xMax() + dx);
  }
  else
  {
    r.setXmin(r.xMin() - dx);
    r.setXmax(r.xMax() - dx);
  }

  if (end.y() < start.y())
  {
    r.setYmax(r.yMax() + dy);
    r.setYmin(r.yMin() + dy);

  }
  else
  {
    r.setYmax(r.yMax() - dy);
    r.setYmin(r.yMin() - dy);

  }
  
  setExtent(r);
  refresh();
}

void QgsMapCanvas::panAction(QMouseEvent * e)
{
  QgsDebugMsg("panAction: entering.");

  // move all map canvas items
  moveCanvasContents();
  
  // update canvas
  //updateContents(); // TODO: need to update?
}

void QgsMapCanvas::moveCanvasContents(bool reset)
{
  QPoint pnt(0,0);
  if (!reset)
    pnt += mCanvasProperties->mouseLastXY - mCanvasProperties->rubberStartPoint;
  
  QgsDebugMsg("moveCanvasContents: pnt " + QString::number(pnt.x()) + "," + QString::number(pnt.y()) );
  
  mMap->setPanningOffset(pnt);
  
  QList<QGraphicsItem*> list = mScene->items();
  QList<QGraphicsItem*>::iterator it = list.begin();
  while (it != list.end())
  {
    QGraphicsItem* item = *it;
    
    if (item != mMap)
    {
      // this tells map canvas item to draw with offset
      QgsMapCanvasItem* canvasItem = dynamic_cast<QgsMapCanvasItem*>(item);
      if (canvasItem)
        canvasItem->setPanningOffset(pnt);
    }
  
    it++;
  }

  // show items
  updateCanvasItemsPositions();

}


void QgsMapCanvas::updateMap()
{
  // XXX updating is not possible since we're already in paint loop
//  mCanvas->update();
//  QApplication::processEvents();
}


void QgsMapCanvas::showError(QgsMapLayer * mapLayer)
{
//   QMessageBox::warning(
//     this,
//     mapLayer->errorCaptionString(),
//     tr("Could not draw") + " " + mapLayer->name() + " " + tr("because") + ":\n" +
//       mapLayer->errorString()
//   );

  QgsMessageViewer * mv = new QgsMessageViewer(this);
  mv->setCaption( mapLayer->errorCaptionString() );
  mv->setMessageAsPlainText(
    tr("Could not draw") + " " + mapLayer->name() + " " + tr("because") + ":\n" +
    mapLayer->errorString()
  );
  mv->exec();
  delete mv;

}

QPoint QgsMapCanvas::mouseLastXY()
{
  return mCanvasProperties->mouseLastXY;
}

void QgsMapCanvas::readProject(const QDomDocument & doc)
{
  QDomNodeList nodes = doc.elementsByTagName("mapcanvas");
  if (nodes.count())
  {
    QDomNode node = nodes.item(0);
    mMapRender->readXML(node);
  }
  else
  {
    QgsDebugMsg("Couldn't read mapcanvas information from project");
  }

}

void QgsMapCanvas::writeProject(QDomDocument & doc)
{
  // create node "mapcanvas" and call mMapRender->writeXML()

  QDomNodeList nl = doc.elementsByTagName("qgis");
  if (!nl.count())
  {
    QgsDebugMsg("Unable to find qgis element in project file");
    return;
  }
  QDomNode qgisNode = nl.item(0);  // there should only be one, so zeroth element ok
  
  QDomElement mapcanvasNode = doc.createElement("mapcanvas");
  qgisNode.appendChild(mapcanvasNode);
  mMapRender->writeXML(mapcanvasNode, doc);

  
}

void QgsMapCanvas::zoom(double scaleFactor)
{
  
  QgsRect r = mMapRender->extent();
  r.scale(scaleFactor);
  setExtent(r);
  refresh();
}

