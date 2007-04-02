/***************************************************************************
  qgsmarkercatalogue.cpp
  -------------------
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
#include <cmath>
#include <iostream>

#include <QPen>
#include <QBrush>
#include <QPainter>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QRect>
#include <QPolygon>
#include <QDir>
#include <QPicture>
#include <QSvgRenderer>

#include "qgis.h"
#include "qgsapplication.h"
#include "qgsmarkercatalogue.h"

QgsMarkerCatalogue *QgsMarkerCatalogue::mMarkerCatalogue = 0;

QgsMarkerCatalogue::QgsMarkerCatalogue()
{
  // Init list

  // Hardcoded markers
  mList.append ( "hard:circle" );
  mList.append ( "hard:rectangle" );
  mList.append ( "hard:diamond" );
  mList.append ( "hard:cross" );
  mList.append ( "hard:cross2" );
  mList.append ( "hard:triangle");
  mList.append ( "hard:star");

  // SVG
  QString svgPath = QgsApplication::svgPath();

  // TODO recursiv ?
  QDir dir ( svgPath );

  QStringList dl = dir.entryList(QDir::Dirs);

  for ( QStringList::iterator it = dl.begin(); it != dl.end(); ++it ) {
    if ( *it == "." || *it == ".." ) continue;

    QDir dir2 ( svgPath + *it );

    QStringList dl2 = dir2.entryList("*.svg",QDir::Files);

    for ( QStringList::iterator it2 = dl2.begin(); it2 != dl2.end(); ++it2 ) {
      // TODO test if it is correct SVG
      mList.append ( "svg:" + svgPath + *it + "/" + *it2 );
    }
  }
}

QStringList QgsMarkerCatalogue::list()
{
  return mList;
}

QgsMarkerCatalogue::~QgsMarkerCatalogue()
{
}

QgsMarkerCatalogue *QgsMarkerCatalogue::instance()
{
  if ( !QgsMarkerCatalogue::mMarkerCatalogue ) {
    QgsMarkerCatalogue::mMarkerCatalogue = new QgsMarkerCatalogue();
  }

  return QgsMarkerCatalogue::mMarkerCatalogue;
}

QImage QgsMarkerCatalogue::imageMarker ( QString fullName, int size, QPen pen, QBrush brush, bool qtBug )
{
  //std::cerr << "QgsMarkerCatalogue::marker " << fullName.toLocal8Bit().data() << " sice:" << size << std::endl;
      
  // 
  // First prepare the paintdevice that the marker will be drawn onto 
  // 
  QImage myImage;
  if ( fullName.left(5) == "hard:" )
  {
    //Note teh +1 offset below is required because the
    //otherwise the icons are getting clipped
    myImage = QImage (size+1,size+1, QImage::Format_ARGB32_Premultiplied);
  }
  else
  {
    // TODO Change this logic so width is size and height is same
    // proportion of scale factor as in oritignal SVG TS XXX
    if (size < 1) size=1;
    //QPixmap myPixmap = QPixmap(width,height);
    myImage = QImage(size,size, QImage::Format_ARGB32_Premultiplied);
  }

  // starting with transparent QImage
  myImage.fill(0);

  QPainter myPainter;
  myPainter.begin(&myImage);
  myPainter.setRenderHint(QPainter::Antialiasing);

  //
  // Now pass the paintdevice along to have the marker rendered on it
  //

  if ( fullName.left(5) == "hard:" )
  {
    hardMarker ( &myPainter, fullName.mid(5), size, pen, brush, qtBug );
    return myImage;
  }
  else if ( fullName.left(4) == "svg:" )
  {
    svgMarker ( &myPainter, fullName.mid(4), size );
    return myImage;
  }
  return QImage(); // empty
}

QPicture QgsMarkerCatalogue::pictureMarker ( QString fullName, int size, QPen pen, QBrush brush, bool qtBug )
{

  //
  // First prepare the paintdevice that the marker will be drawn onto
  //
  QPicture myPicture;
  if ( fullName.left(5) == "hard:" )
  {
    //Note teh +1 offset below is required because the
    //otherwise the icons are getting clipped
    myPicture = QPicture (size+1);
  }
  else
  {
    // TODO Change this logic so width is size and height is same
    // proportion of scale factor as in oritignal SVG TS XXX
    if (size < 1) size=1;
    //QPicture myPicture = QPicture(width,height);
    myPicture = QPicture(size);
  }

  QPainter myPainter(&myPicture);
  myPainter.setRenderHint(QPainter::Antialiasing);

  //
  // Now pass the paintdevice along to have the marker rndered on it
  //

  if ( fullName.left(5) == "hard:" )
  {
    hardMarker ( &myPainter, fullName.mid(5), size, pen, brush, qtBug );
    return myPicture;
  }
  else if ( fullName.left(4) == "svg:" )
  {
    svgMarker ( &myPainter, fullName.mid(4), size );
    return myPicture;
  }
  return QPicture(); // empty
}

void QgsMarkerCatalogue::svgMarker ( QPainter * thepPainter, QString filename, int scaleFactor)
{
  QSvgRenderer mySVG;
  mySVG.load(filename);
  mySVG.render(thepPainter);
}

void QgsMarkerCatalogue::hardMarker (QPainter * thepPainter, QString name, int s, QPen pen, QBrush brush, bool qtBug )
{
  // Size of polygon symbols is calculated so that the area is equal to circle with 
  // diameter mPointSize

  // Size for circle
  int half = (int)floor(s/2.0); // number of points from center
  int size = 2*half + 1;  // must be odd
  double area = 3.14 * (size/2.) * (size/2.);

  // Picture
  QPicture picture;
  thepPainter->begin(&picture);
  thepPainter->setRenderHint(QPainter::Antialiasing);

  // Also width must be odd otherwise there are discrepancies visible in canvas!
  int lw = (int)(2*floor((double)pen.width()/2)+1); // -> lw > 0
  pen.setWidth(lw);
  thepPainter->setPen ( pen );
  thepPainter->setBrush( brush);
  QRect box;

  if ( name == "circle" ) 
  {
    thepPainter->drawEllipse(0, 0, size, size);
  } 
  else if ( name == "rectangle" ) 
  {
    size = (int) (2*floor(sqrt(area)/2.) + 1);
    thepPainter->drawRect(0, 0, size, size);
  } 
  else if ( name == "diamond" ) 
  {
    half = (int) ( sqrt(area/2.) );
    QPolygon pa(4);
    pa.setPoint ( 0, 0, half);
    pa.setPoint ( 1, half, 2*half);
    pa.setPoint ( 2, 2*half, half);
    pa.setPoint ( 3, half, 0);
    thepPainter->drawPolygon ( pa );
  }
  // Warning! if pen width > 0 thepPainter->drawLine(x1,y1,x2,y2) will draw only (x1,y1,x2-1,y2-1) !
  // It is impossible to draw lines as rectangles because line width scaling would not work
  // (QPicture is scaled later in QgsVectorLayer)
  //  -> reset boundingRect for cross, cross2
  else if ( name == "cross" ) 
  {
    int add;
    if ( qtBug ) 
    {
      add = 1;  // lw always > 0
    } 
    else 
    {
      add = 0;
    }

    thepPainter->drawLine(0, half, size-1+add, half); // horizontal
    thepPainter->drawLine(half, 0, half, size-1+add); // vertical
    box.setRect ( 0, 0, size, size );
  }
  else if ( name == "cross2" ) 
  {
    half = (int) floor( s/2/sqrt(2.0));
    size = 2*half + 1;

    int add;
    if ( qtBug ) {
      add = 1;  // lw always > 0
    } else {
      add = 0;
    }

    int addwidth = (int) ( 0.5 * lw ); // width correction, cca lw/2 * cos(45)

    thepPainter->drawLine( 0, 0, size-1+add, size-1+add);
    thepPainter->drawLine( 0, size-1, size-1+add, 0-add);

    box.setRect ( -addwidth, -addwidth, size + 2*addwidth, size + 2*addwidth );	
  }
  else if ( name == "triangle")
    {
      QPolygon pa(3);
      pa.setPoint ( 0, 0, size);
      pa.setPoint ( 1, size, size);
      pa.setPoint ( 2, half, 0);
      thepPainter->drawPolygon ( pa );
    }
  else if (name == "star")
    {
      int oneThird = (int)(floor(size/3+0.5));
      int twoThird = (int)(floor(size/3*2+0.5));
      QPolygon pa(10);
      pa.setPoint(0, half, 0);
      pa.setPoint(1, oneThird, oneThird);
      pa.setPoint(2, 0, oneThird);
      pa.setPoint(3, oneThird, half);
      pa.setPoint(4, 0, size);
      pa.setPoint(5, half, twoThird);
      pa.setPoint(6, size, size);
      pa.setPoint(7, twoThird, half);
      pa.setPoint(8, size, oneThird);
      pa.setPoint(9, twoThird, oneThird);
      thepPainter->drawPolygon ( pa );
    }
  if ( name == "cross" || name == "cross2" ) 
  {
    picture.setBoundingRect ( box ); 
  }
  thepPainter->end();
}
