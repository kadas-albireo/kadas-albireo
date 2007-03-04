/***************************************************************************
                                 qgsmeasure.h
                               ------------------
        begin                : March 2005
        copyright            : (C) 2005 by Radim Blazek
        email                : blazek@itc.it 
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsmeasure.h"

#include "qgscontexthelp.h"
#include "qgsdistancearea.h"
#include "qgsmapcanvas.h"
#include "qgsmaprender.h"
#include "qgsmaptopixel.h"
#include "qgsrubberband.h"
#include "qgsspatialrefsys.h"

#include "QMessageBox"
#include <QSettings>
#include <QLocale>
#include <iostream>


QgsMeasure::QgsMeasure(bool measureArea, QgsMapCanvas *mc, Qt::WFlags f)
  : QDialog(mc->topLevelWidget(), f), QgsMapTool(mc)
{
    setupUi(this);
#ifdef Q_WS_MAC
    // Mac buttons are larger than X11 and require a larger minimum width to be drawn correctly
    frame4->setMinimumSize(QSize(224, 0));
#endif
    connect(mRestartButton, SIGNAL(clicked()), this, SLOT(restart()));
    connect(mCloseButton, SIGNAL(clicked()), this, SLOT(close()));

    mMeasureArea = measureArea;
    mMapCanvas = mc;
    mTotal = 0.;

    mTable->setLeftMargin(0); // hide row labels

    // Set one cell row where to update current distance
    // If measuring area, the table doesn't get shown
    mTable->setNumRows(1);
    mTable->setText(0, 0, QString::number(0, 'f',1));

    //mTable->horizontalHeader()->setLabel( 0, tr("Segments (in meters)") );
    //mTable->horizontalHeader()->setLabel( 1, tr("Total") );
    //mTable->horizontalHeader()->setLabel( 2, tr("Azimuth") );

    mTable->setColumnStretchable ( 0, true );
    //mTable->setColumnStretchable ( 1, true );
    //mTable->setColumnStretchable ( 2, true );

    updateUi();
    
    connect( mMapCanvas, SIGNAL(renderComplete(QPainter*)), this, SLOT(mapCanvasChanged()) );
    
    //mCalc = new QgsDistanceArea;

    mRubberBand = new QgsRubberBand(mMapCanvas, mMeasureArea);

    mCanvas->setCursor(Qt::CrossCursor);

    mRightMouseClicked = false;
}

void QgsMeasure::activate()
{
  restorePosition();
  QgsMapTool::activate();
  mRightMouseClicked = false;

  // ensure that we have correct settings
  updateProjection();
  
  // If we suspect that they have data that is projected, yet the
  // map SRS is set to a geographic one, warn them.
  if (mCanvas->mapRender()->distArea()->geographic() &&
      (mMapCanvas->extent().height() > 360 || 
       mMapCanvas->extent().width() > 720))
  {
    QMessageBox::warning(this, tr("Incorrect measure results"),
        tr("<p>This map is defined with a geographic coordinate system "
           "(latitude/longitude) "
           "but the map extents suggest that it is actually a projected "
           "coordinate system (e.g., Mercator). "
           "If so, the results from line or area measurements will be "
           "incorrect.</p>"
           "<p>To fix this, explicitly set an appropriate map coordinate "
           "system using the <tt>Settings:Project Properties</tt> menu."));
    mWrongProjectProjection = true;
  }
}
    
void QgsMeasure::deactivate()
{
  close();
  QgsMapTool::deactivate();
}


void QgsMeasure::setMeasureArea(bool measureArea)
{
  saveWindowLocation();
  mMeasureArea = measureArea;
  restart();
  restorePosition();
}


QgsMeasure::~QgsMeasure()
{
//  delete mCalc;
  delete mRubberBand;
}

void QgsMeasure::restart(void )
{
    updateProjection();
    mPoints.clear();
    // Set one cell row where to update current distance
    // If measuring area, the table doesn't get shown
    mTable->setNumRows(1);
    mTable->setText(0, 0, QString::number(0, 'f',1));
    mTotal = 0.;
    
    updateUi();

    mRubberBand->reset(mMeasureArea);

    // re-read color settings
    QSettings settings;
    int myRed = settings.value("/qgis/default_measure_color_red", 180).toInt();
    int myGreen = settings.value("/qgis/default_measure_color_green", 180).toInt();
    int myBlue = settings.value("/qgis/default_measure_color_blue", 180).toInt();
    mRubberBand->setColor(QColor(myRed, myGreen, myBlue));

    mRightMouseClicked = false;
    mWrongProjectProjection = false;
}

void QgsMeasure::addPoint(QgsPoint &point)
{
#ifdef QGISDEBUG
    std::cout << "QgsMeasure::addPoint" << point.x() << ", " << point.y() << std::endl;
#endif

    if (mWrongProjectProjection)
    {
      updateProjection();
      mWrongProjectProjection = false;
    }

    // don't add points with the same coordinates
    if (mPoints.size() > 0 && point == mPoints[0])
      return;
    
    QgsPoint pnt(point);
    mPoints.append(pnt);
    
    if (mMeasureArea && mPoints.size() > 2)
    {
      double area = mCanvas->mapRender()->distArea()->measurePolygon(mPoints);
      editTotal->setText(formatArea(area));
    }
    else if (!mMeasureArea && mPoints.size() > 1)
    {
      int last = mPoints.size()-2;
        
      QgsPoint p1 = mPoints[last], p2 = mPoints[last+1];
      
      double d = mCanvas->mapRender()->distArea()->measureLine(p1,p2);
            
      mTotal += d;
      editTotal->setText(formatDistance(mTotal));
	

      int row = mPoints.size()-2;
      mTable->setText(row, 0, QLocale::system().toString(d, 'f', 2));
      mTable->setNumRows ( mPoints.size() );
      
      mTable->setText(row + 1, 0, QLocale::system().toString(0.0, 'f', 2));
      mTable->ensureCellVisible(row + 1,0);
    }

    mRubberBand->addPoint(point);
}

void QgsMeasure::mousePress(QgsPoint &point)
{
  if (mPoints.size() == 0)
  {
    addPoint(point);
    this->show();
  }
  raise();

  mouseMove(point);
}

void QgsMeasure::mouseMove(QgsPoint &point)
{
#ifdef QGISDEBUG
    //std::cout << "QgsMeasure::mouseMove" << point.x() << ", " << point.y() << std::endl;
#endif

  mRubberBand->movePoint(point);
  
  // show current distance/area while moving the point
  // by creating a temporary copy of point array
  // and adding moving point at the end
  QList<QgsPoint> tmpPoints = mPoints;
  tmpPoints.append(point);
  if (mMeasureArea && tmpPoints.size() > 2)
  {
    double area = mCanvas->mapRender()->distArea()->measurePolygon(tmpPoints);
    editTotal->setText(formatArea(area));
  }
  else if (!mMeasureArea && tmpPoints.size() > 1)
  {
    int last = tmpPoints.size()-2;
    QgsPoint p1 = tmpPoints[last], p2 = tmpPoints[last+1];

    double d = mCanvas->mapRender()->distArea()->measureLine(p1,p2);
    //mTable->setText(last, 0, QString::number(d, 'f',1));
    mTable->setText(last, 0, QLocale::system().toString(d, 'f', 2));
    editTotal->setText(formatDistance(mTotal + d));
  }
}

void QgsMeasure::mapCanvasChanged()
{
}

void QgsMeasure::close(void)
{
    restart();
    saveWindowLocation();
    hide();
}

void QgsMeasure::closeEvent(QCloseEvent *e)
{
    saveWindowLocation();
    e->accept();
}

void QgsMeasure::restorePosition()
{
  QSettings settings;
  int ww = settings.readNumEntry("/Windows/Measure/w", 150);
  int wh;
  if (mMeasureArea)
    wh = settings.readNumEntry("/Windows/Measure/hNoTable", 70);
  else
    wh = settings.readNumEntry("/Windows/Measure/h", 200);    
  int wx = settings.readNumEntry("/Windows/Measure/x", 100);
  int wy = settings.readNumEntry("/Windows/Measure/y", 100);
//  setUpdatesEnabled(false);
  adjustSize();
  resize(ww,wh);
  move(wx,wy);
//  setUpdatesEnabled(true);
  this->show();
}

void QgsMeasure::saveWindowLocation()
{
  QSettings settings;
  QPoint p = this->pos();
  QSize s = this->size();
  settings.writeEntry("/Windows/Measure/x", p.x());
  settings.writeEntry("/Windows/Measure/y", p.y());
  settings.writeEntry("/Windows/Measure/w", s.width());
  if (mMeasureArea)
    settings.writeEntry("/Windows/Measure/hNoTable", s.height());
  else
    settings.writeEntry("/Windows/Measure/h", s.height());
} 

void QgsMeasure::on_btnHelp_clicked()
{
  QgsContextHelp::run(context_id);
}


QString QgsMeasure::formatDistance(double distance)
{
  QString txt;
  QString unitLabel;

  QGis::units myMapUnits = mCanvas->mapUnits();
  return QgsDistanceArea::textUnit(distance, 2, myMapUnits, false);
}

QString QgsMeasure::formatArea(double area)
{
  QGis::units myMapUnits = mCanvas->mapUnits();
  return QgsDistanceArea::textUnit(area, 2, myMapUnits, true);
}

void QgsMeasure::updateUi()
{
  
  QGis::units myMapUnits = mCanvas->mapUnits();
  switch (myMapUnits)
  {
    case QGis::METERS: 
      mTable->horizontalHeader()->setLabel( 0, tr("Segments (in meters)") );
      break;
    case QGis::FEET:
      mTable->horizontalHeader()->setLabel( 0, tr("Segments (in feet)") );
      break;
    case QGis::DEGREES:
      mTable->horizontalHeader()->setLabel( 0, tr("Segments (in degrees)") );
      break;
    case QGis::UNKNOWN:
      mTable->horizontalHeader()->setLabel( 0, tr("Segments") );
  };

  if (mMeasureArea)
  {
    mTable->hide();
    editTotal->setText(formatArea(0));
  }
  else
  {
    mTable->show();
    editTotal->setText(formatDistance(0));
  }
  
}

void QgsMeasure::updateProjection()
{
  // set ellipsoid
  QSettings settings;
  // QString ellipsoid = settings.readEntry("/qgis/measure/ellipsoid", "WGS84");
  // mCalc->setEllipsoid(ellipsoid);

  // set source SRS and projections enabled flag
  // QgsMapRender* mapRender = mCanvas->mapRender();
  // mCalc->setProjectionsEnabled(mapRender->projectionsEnabled());
  // int srsid = mapRender->destinationSrs().srsid();
  // mCalc->setSourceSRS(srsid);
  
  int myRed = settings.value("/qgis/default_measure_color_red", 180).toInt();
  int myGreen = settings.value("/qgis/default_measure_color_green", 180).toInt();
  int myBlue = settings.value("/qgis/default_measure_color_blue", 180).toInt();
  mRubberBand->setColor(QColor(myRed, myGreen, myBlue));

}

//////////////////////////

void QgsMeasure::canvasPressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton)
  {
    if (mRightMouseClicked)
      restart();

    QgsPoint  idPoint = mCanvas->getCoordinateTransform()->toMapCoordinates(e->x(), e->y());
    mousePress(idPoint);
  }
}


void QgsMeasure::canvasMoveEvent(QMouseEvent * e)
{
  if (!mRightMouseClicked)
    {
      QgsPoint point = mCanvas->getCoordinateTransform()->toMapCoordinates(e->pos().x(), e->pos().y());
      mouseMove(point);
    }
}


void QgsMeasure::canvasReleaseEvent(QMouseEvent * e)
{
  QgsPoint point = mCanvas->getCoordinateTransform()->toMapCoordinates(e->x(), e->y());

  if(e->button() == Qt::RightButton && (e->state() & Qt::LeftButton) == 0) // restart
  {
    if (mRightMouseClicked)
      restart();
    else
      mRightMouseClicked = true;
  } 
  else if (e->button() == Qt::LeftButton)
  {
    addPoint(point);
    show();
  }

}
