/***************************************************************************
    qgsmaptooladdfeature.cpp
    ------------------------
    begin                : April 2007
    copyright            : (C) 2007 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsmaptooladdfeature.h"
#include "qgsapplication.h"
#include "qgsattributedialog.h"
#include "qgscsexception.h"
#include "qgsfield.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"
#include "qgsrubberband.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include <QMessageBox>

QgsMapToolAddFeature::QgsMapToolAddFeature(QgsMapCanvas* canvas, enum CaptureTool tool): QgsMapToolCapture(canvas, tool)
{

}
  
QgsMapToolAddFeature::~QgsMapToolAddFeature()
{

}

void QgsMapToolAddFeature::canvasReleaseEvent(QMouseEvent * e)
{
  QgsVectorLayer *vlayer = dynamic_cast <QgsVectorLayer*>(mCanvas->currentLayer());
  
  if (!vlayer)
    {
      QMessageBox::information(0, QObject::tr("Not a vector layer"),
			       QObject::tr("The current layer is not a vector layer"));
      return;
    }
  
  QgsVectorDataProvider* provider = vlayer->getDataProvider();
  
  if(!(provider->capabilities() & QgsVectorDataProvider::AddFeatures))
    {
      QMessageBox::information(0, QObject::tr("Layer cannot be added to"),
			       QObject::tr("The data provider for this layer does not support the addition of features."));
      return;
    }
  
  if (!vlayer->isEditable())
    {
      QMessageBox::information(0, QObject::tr("Layer not editable"),
			       QObject::tr("Cannot edit the vector layer. To make it editable, go to the file item "
					   "of the layer, right click and check 'Allow Editing'."));
      return;
    }
  
  double tolerance  = QgsProject::instance()->readDoubleEntry("Digitizing","/Tolerance",0);
  
  // POINT CAPTURING
  if (mTool == CapturePoint)
    {
      //check we only use this tool for point/multipoint layers
      if(vlayer->vectorType() != QGis::Point)
	{
	  QMessageBox::information(0, QObject::tr("Wrong editing tool"),
				   QObject::tr("Cannot apply the 'capture point' tool on this vector layer"));
	  return;
	}
      QgsPoint idPoint = toMapCoords(e->pos());
      
      // emit signal - QgisApp can catch it and save point position to clipboard
      // FIXME: is this still actual or something old that's not used anymore?
      //emit xyClickCoordinates(idPoint);
      
      //only do the rest for provider with feature addition support
      //note that for the grass provider, this will return false since
      //grass provider has its own mechanism of feature addition
      if(provider->capabilities() & QgsVectorDataProvider::AddFeatures)
	{
	  QgsFeature* f = new QgsFeature(0,"WKBPoint");
	  // project to layer's SRS
	  QgsPoint savePoint;
	  try
	    {
	      savePoint = toLayerCoords(vlayer, idPoint);
	    }
	  catch(QgsCsException &cse)
	    {
	      QMessageBox::information(0, QObject::tr("Coordinate transform error"), \
				   QObject::tr("Cannot transform the point to the layers coordinate system"));
	      return;
	    }
	  // snap point to points within the vector layer snapping tolerance
	  vlayer->snapPoint(savePoint, tolerance);
	  
	  int size;
	  char end=QgsApplication::endian();
	  unsigned char *wkb;
	  int wkbtype;
	  double x = savePoint.x();
	  double y = savePoint.y();
	  
	  if(vlayer->geometryType() == QGis::WKBPoint)
	    {
	      size=1+sizeof(int)+2*sizeof(double);
	      wkb = new unsigned char[size];
	      wkbtype=QGis::WKBPoint;
	      memcpy(&wkb[0],&end,1);
	      memcpy(&wkb[1],&wkbtype, sizeof(int));
	      memcpy(&wkb[5], &x, sizeof(double));
	      memcpy(&wkb[5]+sizeof(double), &y, sizeof(double));
	    }
	  else if(vlayer->geometryType() == QGis::WKBMultiPoint)
	    {
	      size = 2+3*sizeof(int)+2*sizeof(double);
	      wkb = new unsigned char[size];
	      wkbtype=QGis::WKBMultiPoint;
	      int position = 0;
	      memcpy(&wkb[position], &end, 1);
	      position += 1;
	      memcpy(&wkb[position], &wkbtype, sizeof(int));
	      position += sizeof(int);
	      int npoint = 1;
	      memcpy(&wkb[position], &npoint, sizeof(int));
	      position += sizeof(int);
	      memcpy(&wkb[position], &end, 1);
	      position += 1;
	      int pointtype = QGis::WKBPoint;
	      memcpy(&wkb[position],&pointtype, sizeof(int));
	      position += sizeof(int);
	      memcpy(&wkb[position], &x, sizeof(double));
	      position += sizeof(double);
	      memcpy(&wkb[position], &y, sizeof(double));
	    }
	  
	  f->setGeometryAndOwnership(&wkb[0],size);
	  // add the fields to the QgsFeature
	  const QgsFieldMap fields=provider->fields();
	  for(QgsFieldMap::const_iterator it = fields.constBegin(); it != fields.constEnd(); ++it)
	    {
	      f->addAttribute(it.key(), provider->getDefaultValue(it.key()) );
	    }
	  
	  // show the dialog to enter attribute values
	  if (QgsAttributeDialog::queryAttributes(fields, *f))
	    vlayer->addFeature(*f);
	  else
	    delete f;
	  
	  mCanvas->refresh();
	}
      
    }  
  else if (mTool == CaptureLine || mTool == CapturePolygon)
    {
      //check we only use the line tool for line/multiline layers
      if(mTool == CaptureLine && vlayer->vectorType() != QGis::Line)
	{
	  QMessageBox::information(0, QObject::tr("Wrong editing tool"),
				   QObject::tr("Cannot apply the 'capture line' tool on this vector layer"));
	  return;
	}
      
      //check we only use the polygon tool for polygon/multipolygon layers
      if(mTool == CapturePolygon && vlayer->vectorType() != QGis::Polygon)
	{
	  QMessageBox::information(0, QObject::tr("Wrong editing tool"),
				   QObject::tr("Cannot apply the 'capture polygon' tool on this vector layer"));
	  return;
	}

      //add point to list and to rubber band
      int error = addVertex(e->pos());
      if(error == 1)
	{
	  //current layer is not a vector layer
	  return;
	}
      else if (error == 2)
	{
	  //problem with coordinate transformation
	  QMessageBox::information(0, QObject::tr("Coordinate transform error"), \
				   QObject::tr("Cannot transform the point to the layers coordinate system"));
	  return;
	}
      
      if (e->button() == Qt::LeftButton)
	{
	  mCapturing = TRUE;
	}
      else if (e->button() == Qt::RightButton)
	{
	  // End of string
	  
	  mCapturing = FALSE;
	  
	  delete mRubberBand;
	  mRubberBand = NULL;
	  
	  //create QgsFeature with wkb representation
	  QgsFeature* f = new QgsFeature(0,"WKBLineString");
	  unsigned char* wkb;
	  int size;
	  char end=QgsApplication::endian();
	  if(mTool == CaptureLine)
	    {
	      if(vlayer->geometryType() == QGis::WKBLineString) 
		{
		  size=1+2*sizeof(int)+2*mCaptureList.size()*sizeof(double);
		  wkb= new unsigned char[size];
		  int wkbtype=QGis::WKBLineString;
		  int length=mCaptureList.size();
		  memcpy(&wkb[0],&end,1);
		  memcpy(&wkb[1],&wkbtype, sizeof(int));
		  memcpy(&wkb[1+sizeof(int)],&length, sizeof(int));
		  int position=1+2*sizeof(int);
		  double x,y;
		  for(std::list<QgsPoint>::iterator it=mCaptureList.begin();it!=mCaptureList.end();++it)
		    {
		      QgsPoint savePoint = *it;
		      x = savePoint.x();
		      y = savePoint.y();
		      
		      memcpy(&wkb[position],&x,sizeof(double));
		      position+=sizeof(double);
		      
		      memcpy(&wkb[position],&y,sizeof(double));
		      position+=sizeof(double);
		    }
		}
	      else if(vlayer->geometryType() == QGis::WKBMultiLineString)
		{
		  size = 1+2*sizeof(int)+1+2*sizeof(int)+2*mCaptureList.size()*sizeof(double);
		  wkb= new unsigned char[size];
		  int position = 0;
		  int wkbtype=QGis::WKBMultiLineString;
		  memcpy(&wkb[position], &end, 1);
		  position += 1;
		  memcpy(&wkb[position], &wkbtype, sizeof(int));
		  position += sizeof(int);
		  int nlines = 1;
		  memcpy(&wkb[position], &nlines, sizeof(int));
		  position += sizeof(int);
		  memcpy(&wkb[position], &end, 1);
		  position += 1;
		  int linewkbtype = QGis::WKBLineString;
		  memcpy(&wkb[position], &linewkbtype, sizeof(int));
		  position += sizeof(int);
		  int length=mCaptureList.size();
		  memcpy(&wkb[position], &length, sizeof(int));
		  position += sizeof(int);
		  double x,y;
		  for(std::list<QgsPoint>::iterator it=mCaptureList.begin();it!=mCaptureList.end();++it)
		    {
		      QgsPoint savePoint = *it;
		      x = savePoint.x();
		      y = savePoint.y();
		      
		      memcpy(&wkb[position],&x,sizeof(double));
		      position+=sizeof(double);
		      
		      memcpy(&wkb[position],&y,sizeof(double));
		      position+=sizeof(double);
		    }
		}     
	    }
	  else // polygon
	    {
	      if(vlayer->geometryType() == QGis::WKBPolygon)
		{
		  size=1+3*sizeof(int)+2*(mCaptureList.size()+1)*sizeof(double);
		  wkb= new unsigned char[size];
		  int wkbtype=QGis::WKBPolygon;
		  int length=mCaptureList.size()+1;//+1 because the first point is needed twice
		  int numrings=1;
		  memcpy(&wkb[0],&end,1);
		  memcpy(&wkb[1],&wkbtype, sizeof(int));
		  memcpy(&wkb[1+sizeof(int)],&numrings,sizeof(int));
		  memcpy(&wkb[1+2*sizeof(int)],&length, sizeof(int));
		  int position=1+3*sizeof(int);
		  double x,y;
		  std::list<QgsPoint>::iterator it;
		  for(it=mCaptureList.begin();it!=mCaptureList.end();++it)
		    {
		      QgsPoint savePoint = *it;
		      x = savePoint.x();
		      y = savePoint.y();
		      
		      memcpy(&wkb[position],&x,sizeof(double));
		      position+=sizeof(double);
		      
		      memcpy(&wkb[position],&y,sizeof(double));
		      position+=sizeof(double);
		    }
		  // close the polygon
		  it=mCaptureList.begin();
		  QgsPoint savePoint = *it;
		  x = savePoint.x();
		  y = savePoint.y();
		  
		  memcpy(&wkb[position],&x,sizeof(double));
		  position+=sizeof(double);
		  
		  memcpy(&wkb[position],&y,sizeof(double));
		}
	      else if(vlayer->geometryType() == QGis::WKBMultiPolygon)
		{
		  size = 2+5*sizeof(int)+2*(mCaptureList.size()+1)*sizeof(double);
		  wkb = new unsigned char[size];
		  int wkbtype = QGis::WKBMultiPolygon;
		  int polygontype = QGis::WKBPolygon;
		  int length = mCaptureList.size()+1;//+1 because the first point is needed twice
		  int numrings = 1;
		  int numpolygons = 1;
		  int position = 0; //pointer position relative to &wkb[0]
		  memcpy(&wkb[position],&end,1);
		  position += 1;
		  memcpy(&wkb[position],&wkbtype, sizeof(int));
		  position += sizeof(int);
		  memcpy(&wkb[position], &numpolygons, sizeof(int));
		  position += sizeof(int);
		  memcpy(&wkb[position], &end, 1);
		  position += 1;
		  memcpy(&wkb[position], &polygontype, sizeof(int));
		  position += sizeof(int);
		  memcpy(&wkb[position], &numrings, sizeof(int));
		  position += sizeof(int);
		  memcpy(&wkb[position], &length, sizeof(int));
		  position += sizeof(int);
		  double x,y;
		  std::list<QgsPoint>::iterator it;
		  for(it=mCaptureList.begin();it!=mCaptureList.end();++it)//add the captured points to the polygon
		    {
		      QgsPoint savePoint = *it;
		      x = savePoint.x();
		      y = savePoint.y();
		      
		      memcpy(&wkb[position],&x,sizeof(double));
		      position+=sizeof(double);
		      
		      memcpy(&wkb[position],&y,sizeof(double));
		      position+=sizeof(double);
		    }
		  // close the polygon
		  it=mCaptureList.begin();
		  QgsPoint savePoint = *it;
		  x = savePoint.x();
		  y = savePoint.y();
		  memcpy(&wkb[position],&x,sizeof(double));
		  position+=sizeof(double);
		  memcpy(&wkb[position],&y,sizeof(double));
		}
	    }
	  f->setGeometryAndOwnership(&wkb[0],size);
	  
	  // add the fields to the QgsFeature
	  const QgsFieldMap fields = provider->fields();
	  for(QgsFieldMap::const_iterator it = fields.begin(); it != fields.end(); ++it)
	    {
	      f->addAttribute(it.key(), provider->getDefaultValue(it.key()));
	    }
	  
	  if (QgsAttributeDialog::queryAttributes(fields, *f))
	    {
	      vlayer->addFeature(*f);
	    }
	  delete f;
	  
	  // delete the elements of mCaptureList
	  mCaptureList.clear();
	  mCanvas->refresh();
	}     
    }
}
