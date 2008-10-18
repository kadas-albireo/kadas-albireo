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
#include "qgsgeometry.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"
#include "qgsrubberband.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include "qgslogger.h"
#include <QMessageBox>
#include <QMouseEvent>

QgsMapToolAddFeature::QgsMapToolAddFeature( QgsMapCanvas* canvas, enum CaptureTool tool ): QgsMapToolCapture( canvas, tool )
{

}

QgsMapToolAddFeature::~QgsMapToolAddFeature()
{

}

void QgsMapToolAddFeature::canvasReleaseEvent( QMouseEvent * e )
{
  QgsVectorLayer *vlayer = dynamic_cast <QgsVectorLayer*>( mCanvas->currentLayer() );

  if ( !vlayer )
  {
    QMessageBox::information( 0, QObject::tr( "Not a vector layer" ),
                              QObject::tr( "The current layer is not a vector layer" ) );
    return;
  }

  QGis::WkbType layerWKBType = vlayer->wkbType();

  //no support for adding features to 2.5D types yet
  if ( layerWKBType == QGis::WKBLineString25D || layerWKBType == QGis::WKBPolygon25D ||
       layerWKBType == QGis::WKBMultiLineString25D || layerWKBType == QGis::WKBPoint25D || layerWKBType == QGis::WKBMultiPoint25D )
  {
    QMessageBox::critical( 0, QObject::tr( "2.5D shape type not supported" ), QObject::tr( "Adding features to 2.5D shapetypes is not supported yet" ) );
    delete mRubberBand;
    mRubberBand = NULL;
    mCapturing = FALSE;
    mCaptureList.clear();
    mCanvas->refresh();
    return;
  }

  QgsVectorDataProvider* provider = vlayer->dataProvider();

  if ( !( provider->capabilities() & QgsVectorDataProvider::AddFeatures ) )
  {
    QMessageBox::information( 0, QObject::tr( "Layer cannot be added to" ),
                              QObject::tr( "The data provider for this layer does not support the addition of features." ) );
    return;
  }

  if ( !vlayer->isEditable() )
  {
    QMessageBox::information( 0, QObject::tr( "Layer not editable" ),
                              QObject::tr( "Cannot edit the vector layer. To make it editable, go to the file item "
                                           "of the layer, right click and check 'Allow Editing'." ) );
    return;
  }

  // POINT CAPTURING
  if ( mTool == CapturePoint )
  {
    //check we only use this tool for point/multipoint layers
    if ( vlayer->geometryType() != QGis::Point )
    {
      QMessageBox::information( 0, QObject::tr( "Wrong editing tool" ),
                                QObject::tr( "Cannot apply the 'capture point' tool on this vector layer" ) );
      return;
    }


    QgsPoint idPoint; //point in map coordinates
    QList<QgsSnappingResult> snapResults;
    QgsPoint savePoint; //point in layer coordinates

    if ( mSnapper.snapToBackgroundLayers( e->pos(), snapResults ) == 0 )
    {
      idPoint = snapPointFromResults( snapResults, e->pos() );
      try
      {
        savePoint = toLayerCoordinates( vlayer, idPoint );
        QgsDebugMsg( "savePoint = " + savePoint.toString() );
      }
      catch ( QgsCsException &cse )
      {
        Q_UNUSED( cse );
        QMessageBox::information( 0, QObject::tr( "Coordinate transform error" ),
                                  QObject::tr( "Cannot transform the point to the layers coordinate system" ) );
        return;
      }
    }

    // emit signal - QgisApp can catch it and save point position to clipboard
    // FIXME: is this still actual or something old that's not used anymore?
    //emit xyClickCoordinates(idPoint);

    //only do the rest for provider with feature addition support
    //note that for the grass provider, this will return false since
    //grass provider has its own mechanism of feature addition
    if ( provider->capabilities() & QgsVectorDataProvider::AddFeatures )
    {
      QgsFeature* f = new QgsFeature( 0, "WKBPoint" );

      int size = 0;
      char end = QgsApplication::endian();
      unsigned char *wkb = NULL;
      int wkbtype = 0;
      double x = savePoint.x();
      double y = savePoint.y();

      if ( layerWKBType == QGis::WKBPoint )
      {
        size = 1 + sizeof( int ) + 2 * sizeof( double );
        wkb = new unsigned char[size];
        wkbtype = QGis::WKBPoint;
        memcpy( &wkb[0], &end, 1 );
        memcpy( &wkb[1], &wkbtype, sizeof( int ) );
        memcpy( &wkb[5], &x, sizeof( double ) );
        memcpy( &wkb[5] + sizeof( double ), &y, sizeof( double ) );
      }
      else if ( layerWKBType == QGis::WKBMultiPoint )
      {
        size = 2 + 3 * sizeof( int ) + 2 * sizeof( double );
        wkb = new unsigned char[size];
        wkbtype = QGis::WKBMultiPoint;
        int position = 0;
        memcpy( &wkb[position], &end, 1 );
        position += 1;
        memcpy( &wkb[position], &wkbtype, sizeof( int ) );
        position += sizeof( int );
        int npoint = 1;
        memcpy( &wkb[position], &npoint, sizeof( int ) );
        position += sizeof( int );
        memcpy( &wkb[position], &end, 1 );
        position += 1;
        int pointtype = QGis::WKBPoint;
        memcpy( &wkb[position], &pointtype, sizeof( int ) );
        position += sizeof( int );
        memcpy( &wkb[position], &x, sizeof( double ) );
        position += sizeof( double );
        memcpy( &wkb[position], &y, sizeof( double ) );
      }

      f->setGeometryAndOwnership( &wkb[0], size );
      // add the fields to the QgsFeature
      const QgsFieldMap fields = vlayer->pendingFields();
      for ( QgsFieldMap::const_iterator it = fields.constBegin(); it != fields.constEnd(); ++it )
      {
        f->addAttribute( it.key(), provider->defaultValue( it.key() ) );
      }

      // show the dialog to enter attribute values
      QgsAttributeDialog * mypDialog = new QgsAttributeDialog( vlayer, f );
      if ( mypDialog->exec() )
      {
        qDebug( "Adding feature to layer" );
        vlayer->addFeature( *f );
      }
      else
      {
        qDebug( "Adding feature to layer failed" );
        delete f;
      }
      delete mypDialog;
      mCanvas->refresh();
    }

  }
  else if ( mTool == CaptureLine || mTool == CapturePolygon )
  {
    //check we only use the line tool for line/multiline layers
    if ( mTool == CaptureLine && vlayer->geometryType() != QGis::Line )
    {
      QMessageBox::information( 0, QObject::tr( "Wrong editing tool" ),
                                QObject::tr( "Cannot apply the 'capture line' tool on this vector layer" ) );
      return;
    }

    //check we only use the polygon tool for polygon/multipolygon layers
    if ( mTool == CapturePolygon && vlayer->geometryType() != QGis::Polygon )
    {
      QMessageBox::information( 0, QObject::tr( "Wrong editing tool" ),
                                QObject::tr( "Cannot apply the 'capture polygon' tool on this vector layer" ) );
      return;
    }

    //add point to list and to rubber band
    int error = addVertex( e->pos() );
    if ( error == 1 )
    {
      //current layer is not a vector layer
      return;
    }
    else if ( error == 2 )
    {
      //problem with coordinate transformation
      QMessageBox::information( 0, QObject::tr( "Coordinate transform error" ),
                                QObject::tr( "Cannot transform the point to the layers coordinate system" ) );
      return;
    }

    if ( e->button() == Qt::LeftButton )
    {
      mCapturing = TRUE;
    }
    else if ( e->button() == Qt::RightButton )
    {
      // End of string

      mCapturing = FALSE;

      //lines: bail out if there are not at least two vertices
      if ( mTool == CaptureLine && mCaptureList.size() < 2 )
      {
        delete mRubberBand;
        mRubberBand = NULL;
        mCaptureList.clear();
        return;
      }

      //polygons: bail out if there are not at least two vertices
      if ( mTool == CapturePolygon && mCaptureList.size() < 3 )
      {
        delete mRubberBand;
        mRubberBand = NULL;
        mCaptureList.clear();
        return;
      }

      //create QgsFeature with wkb representation
      QgsFeature* f = new QgsFeature( 0, "WKBLineString" );
      unsigned char* wkb;
      int size;
      char end = QgsApplication::endian();

      if ( mTool == CaptureLine )
      {
        if ( layerWKBType == QGis::WKBLineString )
        {
          size = 1 + 2 * sizeof( int ) + 2 * mCaptureList.size() * sizeof( double );
          wkb = new unsigned char[size];
          int wkbtype = QGis::WKBLineString;
          int length = mCaptureList.size();
          memcpy( &wkb[0], &end, 1 );
          memcpy( &wkb[1], &wkbtype, sizeof( int ) );
          memcpy( &wkb[1+sizeof( int )], &length, sizeof( int ) );
          int position = 1 + 2 * sizeof( int );
          double x, y;
          for ( QList<QgsPoint>::iterator it = mCaptureList.begin();it != mCaptureList.end();++it )
          {
            QgsPoint savePoint = *it;
            x = savePoint.x();
            y = savePoint.y();

            memcpy( &wkb[position], &x, sizeof( double ) );
            position += sizeof( double );

            memcpy( &wkb[position], &y, sizeof( double ) );
            position += sizeof( double );
          }
        }
        else if ( layerWKBType == QGis::WKBMultiLineString )
        {
          size = 1 + 2 * sizeof( int ) + 1 + 2 * sizeof( int ) + 2 * mCaptureList.size() * sizeof( double );
          wkb = new unsigned char[size];
          int position = 0;
          int wkbtype = QGis::WKBMultiLineString;
          memcpy( &wkb[position], &end, 1 );
          position += 1;
          memcpy( &wkb[position], &wkbtype, sizeof( int ) );
          position += sizeof( int );
          int nlines = 1;
          memcpy( &wkb[position], &nlines, sizeof( int ) );
          position += sizeof( int );
          memcpy( &wkb[position], &end, 1 );
          position += 1;
          int linewkbtype = QGis::WKBLineString;
          memcpy( &wkb[position], &linewkbtype, sizeof( int ) );
          position += sizeof( int );
          int length = mCaptureList.size();
          memcpy( &wkb[position], &length, sizeof( int ) );
          position += sizeof( int );
          double x, y;
          for ( QList<QgsPoint>::iterator it = mCaptureList.begin();it != mCaptureList.end();++it )
          {
            QgsPoint savePoint = *it;
            x = savePoint.x();
            y = savePoint.y();

            memcpy( &wkb[position], &x, sizeof( double ) );
            position += sizeof( double );

            memcpy( &wkb[position], &y, sizeof( double ) );
            position += sizeof( double );
          }
        }
        else
        {
          QMessageBox::critical( 0, QObject::tr( "Error" ), QObject::tr( "Cannot add feature. Unknown WKB type" ) );
          delete mRubberBand;
          mRubberBand = NULL;
          mCaptureList.clear();
          return; //unknown wkbtype
        }
        f->setGeometryAndOwnership( &wkb[0], size );
      }
      else // polygon
      {
        if ( layerWKBType == QGis::WKBPolygon )
        {
          size = 1 + 3 * sizeof( int ) + 2 * ( mCaptureList.size() + 1 ) * sizeof( double );
          wkb = new unsigned char[size];
          int wkbtype = QGis::WKBPolygon;
          int length = mCaptureList.size() + 1;//+1 because the first point is needed twice
          int numrings = 1;
          memcpy( &wkb[0], &end, 1 );
          memcpy( &wkb[1], &wkbtype, sizeof( int ) );
          memcpy( &wkb[1+sizeof( int )], &numrings, sizeof( int ) );
          memcpy( &wkb[1+2*sizeof( int )], &length, sizeof( int ) );
          int position = 1 + 3 * sizeof( int );
          double x, y;
          QList<QgsPoint>::iterator it;
          for ( it = mCaptureList.begin();it != mCaptureList.end();++it )
          {
            QgsPoint savePoint = *it;
            x = savePoint.x();
            y = savePoint.y();

            memcpy( &wkb[position], &x, sizeof( double ) );
            position += sizeof( double );

            memcpy( &wkb[position], &y, sizeof( double ) );
            position += sizeof( double );
          }
          // close the polygon
          it = mCaptureList.begin();
          QgsPoint savePoint = *it;
          x = savePoint.x();
          y = savePoint.y();

          memcpy( &wkb[position], &x, sizeof( double ) );
          position += sizeof( double );

          memcpy( &wkb[position], &y, sizeof( double ) );
        }
        else if ( layerWKBType == QGis::WKBMultiPolygon )
        {
          size = 2 + 5 * sizeof( int ) + 2 * ( mCaptureList.size() + 1 ) * sizeof( double );
          wkb = new unsigned char[size];
          int wkbtype = QGis::WKBMultiPolygon;
          int polygontype = QGis::WKBPolygon;
          int length = mCaptureList.size() + 1;//+1 because the first point is needed twice
          int numrings = 1;
          int numpolygons = 1;
          int position = 0; //pointer position relative to &wkb[0]
          memcpy( &wkb[position], &end, 1 );
          position += 1;
          memcpy( &wkb[position], &wkbtype, sizeof( int ) );
          position += sizeof( int );
          memcpy( &wkb[position], &numpolygons, sizeof( int ) );
          position += sizeof( int );
          memcpy( &wkb[position], &end, 1 );
          position += 1;
          memcpy( &wkb[position], &polygontype, sizeof( int ) );
          position += sizeof( int );
          memcpy( &wkb[position], &numrings, sizeof( int ) );
          position += sizeof( int );
          memcpy( &wkb[position], &length, sizeof( int ) );
          position += sizeof( int );
          double x, y;
          QList<QgsPoint>::iterator it;
          for ( it = mCaptureList.begin();it != mCaptureList.end();++it )//add the captured points to the polygon
          {
            QgsPoint savePoint = *it;
            x = savePoint.x();
            y = savePoint.y();

            memcpy( &wkb[position], &x, sizeof( double ) );
            position += sizeof( double );

            memcpy( &wkb[position], &y, sizeof( double ) );
            position += sizeof( double );
          }
          // close the polygon
          it = mCaptureList.begin();
          QgsPoint savePoint = *it;
          x = savePoint.x();
          y = savePoint.y();
          memcpy( &wkb[position], &x, sizeof( double ) );
          position += sizeof( double );
          memcpy( &wkb[position], &y, sizeof( double ) );
        }
        else
        {
          QMessageBox::critical( 0, QObject::tr( "Error" ), QObject::tr( "Cannot add feature. Unknown WKB type" ) );
          delete mRubberBand;
          mRubberBand = NULL;
          mCaptureList.clear();
          return; //unknown wkbtype
        }
        f->setGeometryAndOwnership( &wkb[0], size );

        //is automatic polygon intersection removal activated?
        int avoidPolygonIntersections = QgsProject::instance()->readNumEntry( "Digitizing", "/AvoidPolygonIntersections", 0 );

        if ( avoidPolygonIntersections != 0 )
        {
          if ( vlayer->removePolygonIntersections( f->geometry() ) != 0 )
          {
            QMessageBox::critical( 0, QObject::tr( "Error" ), QObject::tr( "Could not remove polygon intersection" ) );
          }
        }
      }

      // add the fields to the QgsFeature
      const QgsFieldMap fields = vlayer->pendingFields();
      for ( QgsFieldMap::const_iterator it = fields.begin(); it != fields.end(); ++it )
      {
        f->addAttribute( it.key(), provider->defaultValue( it.key() ) );
      }

      QgsAttributeDialog * mypDialog = new QgsAttributeDialog( vlayer, f );
      if ( mypDialog->exec() )
      {
        if ( vlayer->addFeature( *f ) )
        {
          //add points to other features to keep topology up-to-date
          int topologicalEditing = QgsProject::instance()->readNumEntry( "Digitizing", "/TopologicalEditing", 0 );
          if ( topologicalEditing )
          {
            vlayer->addTopologicalPoints( f->geometry() );
          }
        }
      }
      delete f;
      delete mypDialog;

      delete mRubberBand;
      mRubberBand = NULL;

      // delete the elements of mCaptureList
      mCaptureList.clear();
      mCanvas->refresh();
    }
  }
}
