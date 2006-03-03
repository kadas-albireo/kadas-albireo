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
#include "qgsmaptopixel.h"
#include "qgsrubberband.h"

#include <QSettings>
#include <iostream>


QgsMeasure::QgsMeasure(bool measureArea, QgsMapCanvas *mc, const char * name, Qt::WFlags f)
  : QWidget(mc->topLevelWidget(), name, f), QgsMapTool(mc)
{
    setupUi(this);
    connect(mRestartButton, SIGNAL(clicked()), this, SLOT(restart()));
    connect(mCloseButton, SIGNAL(clicked()), this, SLOT(close()));

    mMeasureArea = measureArea;
    mMapCanvas = mc;
    mTotal = 0.;

    mTable->setLeftMargin(0); // hide row labels

    mTable->horizontalHeader()->setLabel( 0, tr("Segments (in meters)") );
    //mTable->horizontalHeader()->setLabel( 1, tr("Total") );
    //mTable->horizontalHeader()->setLabel( 2, tr("Azimuth") );

    mTable->setColumnStretchable ( 0, true );
    //mTable->setColumnStretchable ( 1, true );
    //mTable->setColumnStretchable ( 2, true );

    // Widget font properties should normally be set in the ui file.
    // This is here to work around the following bug in Qt 4.0.1:
    // Setting another font attribute while using the default font size will
    // cause uic3 to generate code setting the font size to 0.
    // This causes text not to be drawn with Qt/X11 and a crash with Qt/Mac.
    QFont font(lblTotal->font());
    font.setBold(true);
    lblTotal->setFont(font);

    updateUi();
    
    connect( mMapCanvas, SIGNAL(renderComplete(QPainter*)), this, SLOT(mapCanvasChanged()) );
    restorePosition();
    
    mCalc = new QgsDistanceArea;

    mRubberBand = new QgsRubberBand(mMapCanvas, mMeasureArea);
    mRubberBand->show();

    mCanvas->setCursor(Qt::CrossCursor);
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
  delete mCalc;
  delete mRubberBand;
}

void QgsMeasure::restart(void )
{
    mPoints.resize(0);
    mTable->setNumRows(0);
    mTotal = 0.;
    
    updateUi();

    mRubberBand->reset(mMeasureArea);
}

void QgsMeasure::addPoint(QgsPoint &point)
{
#ifdef QGISDEBUG
    std::cout << "QgsMeasure::addPoint" << point.x() << ", " << point.y() << std::endl;
#endif

    // don't add points with the same coordinates
    if (mPoints.size() > 0 && point == mPoints[0])
      return;
    
    QgsPoint pnt(point);
    mPoints.push_back(pnt);
    
    if (mPoints.size() == 1)
    {
      // ensure that we have correct settings
      mCalc->setDefaultEllipsoid();
      mCalc->setProjectAsSourceSRS();
    }

    if (mMeasureArea && mPoints.size() > 2)
    {
      double area = mCalc->measurePolygon(mPoints);
      lblTotal->setText(formatArea(area));
    }
    else if (!mMeasureArea && mPoints.size() > 1)
    {
      int last = mPoints.size()-2;
        
      QgsPoint p1 = mPoints[last], p2 = mPoints[last+1];
      
      double d = mCalc->measureLine(p1,p2);
            
      mTotal += d;
      lblTotal->setText(formatDistance(mTotal));
	
    	mTable->setNumRows ( mPoints.size()-1 );

	    int row = mPoints.size()-2;
      mTable->setText(row, 0, QString::number(d, 'f',1));
      //mTable->setText ( row, 1, QString::number(mTotal) );
      
      mTable->ensureCellVisible(row,0);
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
  
  mouseMove(point);
}

void QgsMeasure::mouseMove(QgsPoint &point)
{
#ifdef QGISDEBUG
    //std::cout << "QgsMeasure::mouseMove" << point.x() << ", " << point.y() << std::endl;
#endif

  mRubberBand->movePoint(point);
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
  if (distance < 1000)
  {
    txt = QString::number(distance,'f',0);
    txt += " m";
  }
  else
  {
    txt = QString::number(distance/1000,'f',1);
    txt += " km";
  }
  return txt;
}

QString QgsMeasure::formatArea(double area)
{
  QString txt;
  if (area < 1000)
  {
    txt = QString::number(area,'f',0);
    txt += " m<sup>2</sup>";
  }
  else
  {
    txt = QString::number(area/1000000,'f',3);
    txt += " km<sup>2</sup>";
  }
  return txt;
}

void QgsMeasure::updateUi()
{
  if (mMeasureArea)
  {
    mTable->hide();
    lblTotal->setText(formatArea(0));
  }
  else
  {
    mTable->show();
    lblTotal->setText(formatDistance(0));
  }
  
}

//////////////////////////

void QgsMeasure::canvasPressEvent(QMouseEvent * e)
{
  if (e->button() == Qt::LeftButton)
  {
    QgsPoint  idPoint = mCanvas->getCoordinateTransform()->toMapCoordinates(e->x(), e->y());
    mousePress(idPoint);
  }
}


void QgsMeasure::canvasMoveEvent(QMouseEvent * e)
{
  if (e->state() & Qt::LeftButton)
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
     restart();
  } 
  else if (e->button() == Qt::LeftButton)
  {
    addPoint(point);
    show();
  }
}
