/***************************************************************************
                           qgscomposerscalebar.cpp
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
#include <math.h>
#include <iostream>
#include <typeinfo>
#include <map>
#include <vector>

#include <qwidget.h>
#include <qrect.h>
#include <q3combobox.h>
#include <qdom.h>
#include <q3canvas.h>
#include <qpainter.h>
#include <qstring.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qlineedit.h>
#include <q3pointarray.h>
#include <qfont.h>
#include <qfontmetrics.h>
#include <qfontdialog.h>
#include <qpen.h>
#include <qrect.h>
#include <q3listview.h>
#include <q3popupmenu.h>
#include <qlabel.h>
#include <q3pointarray.h>
#include <qrect.h>
#include <qspinbox.h>

#include "qgsrect.h"
#include "qgsmaptopixel.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsvectorlayer.h"
#include "qgsdlgvectorlayerproperties.h"
#include "qgscomposition.h"
#include "qgscomposermap.h"
#include "qgscomposerscalebar.h"

#include "qgssymbol.h"

#include "qgsrenderer.h"
#include "qgsrenderitem.h"
#include "qgsrangerenderitem.h"

#include "qgscontinuouscolrenderer.h"
#include "qgsgraduatedsymrenderer.h"
#include "qgssinglesymrenderer.h"
#include "qgsuniquevalrenderer.h"
#include "qgssvgcache.h"

QgsComposerScalebar::QgsComposerScalebar ( QgsComposition *composition, int id, 
	                                            int x, int y )
    : Q3CanvasPolygonalItem(0),
    mComposition(composition),
    mMap(0),
    mBrush(QColor(150,150,150))
{
    std::cout << "QgsComposerScalebar::QgsComposerScalebar()" << std::endl;
    mId = id;
    mSelected = false;

    mMapCanvas = mComposition->mapCanvas();

    Q3CanvasPolygonalItem::setX(x);
    Q3CanvasPolygonalItem::setY(y);

    init();

    // Set map to the first available if any
    std::vector<QgsComposerMap*> maps = mComposition->maps();
    if ( maps.size() > 0 ) {
	mMap = maps[0]->id();
    }

    // Set default according to the map
    QgsComposerMap *map = mComposition->map ( mMap );
    if ( map ) {
	mMapUnitsPerUnit = 1.;
	mUnitLabel = "m";

	// make one segment cca 1/10 of map width and it will be 1xxx, 2xxx or 5xxx
	double mapwidth = 1. * map->Q3CanvasRectangle::width() / map->scale();

	mSegmentLength = mapwidth / 10;
	
	int powerOf10 = int(pow(10.0, int(log(mSegmentLength) / log(10.0)))); // from scalebar plugin

	int isize = (int) ceil ( mSegmentLength / powerOf10 ); 

	if ( isize == 3 ) isize = 2;
	else if ( isize == 4 ) isize = 5;
	else if ( isize > 5  && isize < 8 ) isize = 5;
	else if ( isize > 7  ) isize = 10;
	
	mSegmentLength = isize * powerOf10 ;
	
	// the scale bar will take cca 1/4 of the map width
	mNumSegments = (int) ( mapwidth / 4 / mSegmentLength ); 
    
	int segsize = (int) ( mSegmentLength * map->scale() );
	mFont.setPointSize ( (int) ( segsize/10) );
    } 
    else 
    {
	mMapUnitsPerUnit = 1.;
	mUnitLabel = "m";
	mSegmentLength = 1000.;
	mNumSegments = 5;
	mFont.setPointSize ( 8  );
    }
    
    // Calc size
    recalculate();

    // Add to canvas
    setCanvas(mComposition->canvas());

    Q3CanvasPolygonalItem::show();
    Q3CanvasPolygonalItem::update();
     
    writeSettings();
}

QgsComposerScalebar::QgsComposerScalebar ( QgsComposition *composition, int id ) 
    : Q3CanvasPolygonalItem(0),
    mComposition(composition),
    mMap(0),
    mBrush(QColor(150,150,150))
{
    std::cout << "QgsComposerScalebar::QgsComposerScalebar()" << std::endl;
    mId = id;
    mSelected = false;

    mMapCanvas = mComposition->mapCanvas();

    init();

    readSettings();

    // Calc size
    recalculate();

    // Add to canvas
    setCanvas(mComposition->canvas());

    Q3CanvasPolygonalItem::show();
    Q3CanvasPolygonalItem::update();
}

void QgsComposerScalebar::init ( void ) 
{
    mUnitLabel = "m";

    // Rectangle
    Q3CanvasPolygonalItem::setZ(50);
    setActive(true);

    // Plot style
    setPlotStyle ( QgsComposition::Preview );
    
    connect ( mComposition, SIGNAL(mapChanged(int)), this, SLOT(mapChanged(int)) ); 
}

QgsComposerScalebar::~QgsComposerScalebar()
{
    std::cerr << "QgsComposerScalebar::~QgsComposerScalebar()" << std::endl;
    Q3CanvasItem::hide();
}

QRect QgsComposerScalebar::render ( QPainter *p )
{
    std::cout << "QgsComposerScalebar::render p = " << p << std::endl;

    // Painter can be 0, create dummy to avoid many if below
    QPainter *painter;
    QPixmap *pixmap;
    if ( p ) {
	painter = p;
    } else {
	pixmap = new QPixmap(1,1);
	painter = new QPainter( pixmap );
    }

    std::cout << "mComposition->scale() = " << mComposition->scale() << std::endl;

    // Draw background rectangle
    painter->setPen( QPen(QColor(255,255,255), 1) );
    painter->setBrush( QBrush( QColor(255,255,255), Qt::SolidPattern) );

    painter->drawRect ( mBoundingRect.x(), mBoundingRect.y(), 
	               mBoundingRect.width()+1, mBoundingRect.height()+1 ); // is it right?
    

    // Font size in canvas units
    float size = 25.4 * mComposition->scale() * mFont.pointSizeFloat() / 72;

    // Metrics 
    QFont font ( mFont );
    font.setPointSizeFloat ( size );
    QFontMetrics metrics ( font );
    
    // TODO: For output to Postscript the font must be scaled. But how?
    //       The factor is an empirical value.
    //       In any case, each font scales in in different way even if painter.scale()
    //       is used instead of font size!!! -> Postscript is never exactly the same as
    //       in preview.
    double factor = QgsComposition::psFontScaleFactor();

    double pssize = factor * 72.0 * mFont.pointSizeFloat() / mComposition->resolution();
    double psscale = pssize/size;

    // Not sure about Style Strategy, QFont::PreferMatch?
    font.setStyleStrategy ( (QFont::StyleStrategy) (QFont::PreferOutline | QFont::PreferAntialias) );

    int xmin; // min x
    int xmax; // max x
    int ymin; // min y
    int ymax; // max y

    int cx = (int) Q3CanvasPolygonalItem::x();
    int cy = (int) Q3CanvasPolygonalItem::y();

    painter->setPen ( mPen );
    painter->setBrush ( mBrush );
    painter->setFont ( font );

    QgsComposerMap *map = mComposition->map ( mMap );
    if ( map ) {
	// width of the whole scalebar in canvas points
	int segwidth = (int) ( mSegmentLength * map->scale() );
	int width = (int) ( segwidth * mNumSegments );
	
	int barLx = (int) ( cx - width/2 );

	int rectadd = 0;
        if ( plotStyle() == QgsComposition::Preview ) {
	    rectadd  = 1; // add 1 pixel in preview, must not be in PS
	}

	// fill odd
	for ( int i = 0; i < mNumSegments; i += 2 ) {
	    painter->drawRect( barLx+i*segwidth, cy, segwidth+rectadd, mHeight );
	}

	// ticks
	int ticksize = (int ) (3./4*mHeight);
	for ( int i = 0; i <= mNumSegments; i++ ) {
	    painter->drawLine( barLx+i*segwidth, cy, barLx+i*segwidth, cy-ticksize );
	}

	painter->setBrush( Qt::NoBrush );

	painter->drawRect( barLx, cy, width+rectadd, mHeight );
	
	// labels
	int h = metrics.height();
	int offset = (int ) (1./2*ticksize);
	for ( int i = 0; i <= mNumSegments; i++ ) {
	    int lab = (int) (1. * i * mSegmentLength / mMapUnitsPerUnit);
	    QString txt = QString::number(lab);
	    int w = metrics.width ( txt );
	    int shift = (int) w/2;
	    
	    if ( i == 0 ) { 
		xmin = (int) barLx - w/2; 
	    }

	    if ( i == mNumSegments ) { 
		txt.append ( " " + mUnitLabel );
		w = metrics.width ( txt );

		xmax = (int) barLx + width - shift + w; 
	    }
	    
	    int x = barLx+i*segwidth-shift;
	    int y = cy-ticksize-offset-metrics.descent();

            if ( plotStyle() == QgsComposition::Postscript ) {
		painter->save();
		painter->translate(x,y);
		painter->scale ( psscale, psscale );
		painter->drawText( 0, 0, txt );
		painter->restore();
	    } else {
	        painter->drawText( x, y, txt );
	    }
	}
	
	ymin = cy - ticksize - offset - h;
	ymax = cy + mHeight;	
    } 
    else 
    {
	int width = 50 * mComposition->scale(); 

	int barLx = (int) ( cx - width/2 );
	painter->drawRect( (int)barLx, (int)(cy-mHeight/2), width, mHeight );

	xmin = barLx;
        xmax = barLx + width;
 	ymin = cy - mHeight;
	ymax = cy + mHeight;	
    }

    if ( !p ) {
	delete painter;
	delete pixmap;
    }

    return QRect ( xmin-mMargin, ymin-mMargin, xmax-xmin+2*mMargin, ymax-ymin+2*mMargin);
}

void QgsComposerScalebar::draw ( QPainter & painter )
{
    std::cout << "draw mPlotStyle = " << plotStyle() << std::endl;

    render( &painter );

    // Show selected / Highlight
    if ( mSelected && plotStyle() == QgsComposition::Preview ) {
        painter.setPen( mComposition->selectionPen() );
        painter.setBrush( mComposition->selectionBrush() );
	
	int s = mComposition->selectionBoxSize();
	QRect r = boundingRect();

	painter.drawRect ( r.x(), r.y(), s, s );
	painter.drawRect ( r.x()+r.width()-s, r.y(), s, s );
	painter.drawRect ( r.x()+r.width()-s, r.y()+r.height()-s, s, s );
	painter.drawRect ( r.x(), r.y()+r.height()-s, s, s );
    }
}

void QgsComposerScalebar::drawShape ( QPainter & painter )
{
    draw ( painter );
}

void QgsComposerScalebar::changeFont ( void ) 
{
    bool result;
    
    mFont = QFontDialog::getFont(&result, mFont, this );

    if ( result ) {
	recalculate();
	Q3CanvasPolygonalItem::update();
	Q3CanvasPolygonalItem::canvas()->update();
	writeSettings();
    }
}

void QgsComposerScalebar::unitLabelChanged (  )
{
    mUnitLabel = mUnitLabelLineEdit->text();
    recalculate();
    Q3CanvasPolygonalItem::update();
    Q3CanvasPolygonalItem::canvas()->update();
    writeSettings();
}

void QgsComposerScalebar::mapSelectionChanged ( int i )
{
    mMap = mMaps[i];
    recalculate();
    Q3CanvasPolygonalItem::update();
    Q3CanvasPolygonalItem::canvas()->update();
    writeSettings();
}

void QgsComposerScalebar::mapChanged ( int id )
{
    if ( id != mMap ) return;
    recalculate();
    Q3CanvasPolygonalItem::update();
    Q3CanvasPolygonalItem::canvas()->update();
}

void QgsComposerScalebar::sizeChanged ( )
{
    mSegmentLength = mSegmentLengthLineEdit->text().toDouble();
    mNumSegments = mNumSegmentsLineEdit->text().toInt();
    mPen.setWidth ( mLineWidthSpinBox->value() );
    mMapUnitsPerUnit = mMapUnitsPerUnitLineEdit->text().toInt();
    recalculate();
    Q3CanvasPolygonalItem::update();
    Q3CanvasPolygonalItem::canvas()->update();
    writeSettings();
}

void QgsComposerScalebar::moveBy(double x, double y )
{
    std::cout << "QgsComposerScalebar::move" << std::endl;
    Q3CanvasItem::moveBy ( x, y );

    recalculate();
    //writeSettings(); // not necessary called by composition
}

void QgsComposerScalebar::recalculate ( void ) 
{
    std::cout << "QgsComposerScalebar::recalculate" << std::endl;
    
    mHeight = (int) ( 25.4 * mComposition->scale() * mFont.pointSize() / 72);
    mMargin = (int) (3.*mHeight/2);
    
    // !!! invalidate() MUST BE called before the value returned by areaPoints() changes
    Q3CanvasPolygonalItem::invalidate();
    
    mBoundingRect = render(0);
    
    Q3CanvasItem::update();
}

QRect QgsComposerScalebar::boundingRect ( void ) const
{
    std::cout << "QgsComposerScalebar::boundingRect" << std::endl;
    return mBoundingRect;
}

Q3PointArray QgsComposerScalebar::areaPoints() const
{
    std::cout << "QgsComposerScalebar::areaPoints" << std::endl;

    QRect r = boundingRect();
    Q3PointArray pa(4);
    pa[0] = QPoint( r.x(), r.y() );
    pa[1] = QPoint( r.x()+r.width(), r.y() );
    pa[2] = QPoint( r.x()+r.width(), r.y()+r.height() );
    pa[3] = QPoint( r.x(), r.y()+r.height() );
    return pa ;
}

void QgsComposerScalebar::setOptions ( void )
{ 
    mSegmentLengthLineEdit->setText( QString::number(mSegmentLength) );
    mNumSegmentsLineEdit->setText( QString::number( mNumSegments ) );
    mUnitLabelLineEdit->setText( mUnitLabel );
    mMapUnitsPerUnitLineEdit->setText( QString::number(mMapUnitsPerUnit ) );

    mLineWidthSpinBox->setValue ( mPen.width() );
    
    // Maps
    mMapComboBox->clear();
    std::vector<QgsComposerMap*> maps = mComposition->maps();

    mMaps.clear();
    
    bool found = false;
    mMapComboBox->insertItem ( "", 0 );
    mMaps.push_back ( 0 );
    for ( int i = 0; i < maps.size(); i++ ) {
	mMapComboBox->insertItem ( maps[i]->name(), i+1 );
	mMaps.push_back ( maps[i]->id() );

	if ( maps[i]->id() == mMap ) {
	    found = true;
	    mMapComboBox->setCurrentItem ( i+1 );
	}
    }

    if ( ! found ) {
	mMap = 0;
	mMapComboBox->setCurrentItem ( 0 );
    }
}

void QgsComposerScalebar::setSelected (  bool s ) 
{
    mSelected = s;
    Q3CanvasPolygonalItem::update(); // show highlight
}    

bool QgsComposerScalebar::selected( void )
{
    return mSelected;
}

QWidget *QgsComposerScalebar::options ( void )
{
    setOptions ();
    return ( dynamic_cast <QWidget *> (this) ); 
}

bool QgsComposerScalebar::writeSettings ( void )  
{
    std::cout << "QgsComposerScalebar::writeSettings" << std::endl;
    QString path;
    path.sprintf("/composition_%d/scalebar_%d/", mComposition->id(), mId ); 

    QgsProject::instance()->writeEntry( "Compositions", path+"x", mComposition->toMM((int)Q3CanvasPolygonalItem::x()) );
    QgsProject::instance()->writeEntry( "Compositions", path+"y", mComposition->toMM((int)Q3CanvasPolygonalItem::y()) );

    QgsProject::instance()->writeEntry( "Compositions", path+"map", mMap );

    QgsProject::instance()->writeEntry( "Compositions", path+"unit/label", mUnitLabel  );
    QgsProject::instance()->writeEntry( "Compositions", path+"unit/mapunits", mMapUnitsPerUnit );

    QgsProject::instance()->writeEntry( "Compositions", path+"segmentsize", mSegmentLength );
    QgsProject::instance()->writeEntry( "Compositions", path+"numsegments", mNumSegments );

    QgsProject::instance()->writeEntry( "Compositions", path+"font/size", mFont.pointSize() );
    QgsProject::instance()->writeEntry( "Compositions", path+"font/family", mFont.family() );
    QgsProject::instance()->writeEntry( "Compositions", path+"font/weight", mFont.weight() );
    QgsProject::instance()->writeEntry( "Compositions", path+"font/underline", mFont.underline() );
    QgsProject::instance()->writeEntry( "Compositions", path+"font/strikeout", mFont.strikeOut() );
    
    QgsProject::instance()->writeEntry( "Compositions", path+"pen/width", (int)mPen.width() );

    return true; 
}

bool QgsComposerScalebar::readSettings ( void )
{
    bool ok;
    QString path;
    path.sprintf("/composition_%d/scalebar_%d/", mComposition->id(), mId );

    Q3CanvasPolygonalItem::setX( mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"x", 0, &ok)) );
    Q3CanvasPolygonalItem::setY( mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"y", 0, &ok)) );
    
    mMap = QgsProject::instance()->readNumEntry("Compositions", path+"map", 0, &ok);
    mUnitLabel = QgsProject::instance()->readEntry("Compositions", path+"unit/label", "???", &ok);
    mMapUnitsPerUnit = QgsProject::instance()->readDoubleEntry("Compositions", path+"unit/mapunits", 1., &ok);

    mSegmentLength = QgsProject::instance()->readDoubleEntry("Compositions", path+"segmentsize", 1000., &ok);
    mNumSegments = QgsProject::instance()->readNumEntry("Compositions", path+"numsegments", 5, &ok);
     
    mFont.setFamily ( QgsProject::instance()->readEntry("Compositions", path+"font/family", "", &ok) );
    mFont.setPointSize ( QgsProject::instance()->readNumEntry("Compositions", path+"font/size", 10, &ok) );
    mFont.setWeight(  QgsProject::instance()->readNumEntry("Compositions", path+"font/weight", (int)QFont::Normal, &ok) );
    mFont.setUnderline(  QgsProject::instance()->readBoolEntry("Compositions", path+"font/underline", false, &ok) );
    mFont.setStrikeOut(  QgsProject::instance()->readBoolEntry("Compositions", path+"font/strikeout", false, &ok) );

    mPen.setWidth(  QgsProject::instance()->readNumEntry("Compositions", path+"pen/width", 1, &ok) );
    
    recalculate();
    
    return true;
}

bool QgsComposerScalebar::removeSettings( void )
{
    std::cerr << "QgsComposerScalebar::deleteSettings" << std::endl;

    QString path;
    path.sprintf("/composition_%d/scalebar_%d", mComposition->id(), mId ); 
    return QgsProject::instance()->removeEntry ( "Compositions", path );
}

bool QgsComposerScalebar::writeXML( QDomNode & node, QDomDocument & document, bool temp )
{
    return true;
}

bool QgsComposerScalebar::readXML( QDomNode & node )
{
    return true;
}

