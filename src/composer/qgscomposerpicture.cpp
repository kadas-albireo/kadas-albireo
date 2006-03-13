/***************************************************************************
                           qgscomposerpicture.cpp
                             -------------------
    begin                : September 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : radim.blazek@gmail.com
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

#include "qgscomposerpicture.h"
#include "qgsproject.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <Q3StrList>

#include <cmath>
#include <iostream>

#define PI 3.14159265358979323846

QgsComposerPicture::QgsComposerPicture ( QgsComposition *composition, 
					int id, QString file ) 
    : QWidget(composition),
      Q3CanvasPolygonalItem(0),
      mPicturePath ( file ),
      mPictureValid(false),
      mCX(-10), mCY(-10),
      mWidth(0), mHeight(0), mAngle(0),
      mFrame(false),
      mAreaPoints(4),
      mBoundingRect(0,0,0,0)
{
    setupUi(this);

#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::QgsComposerPicture()" << std::endl;
#endif

    mComposition = composition;
    mId  = id;

    init();
    loadPicture();

    // Add to canvas
    setCanvas(mComposition->canvas());

    Q3CanvasPolygonalItem::show();
    Q3CanvasPolygonalItem::update();
     
    writeSettings();
}

QgsComposerPicture::QgsComposerPicture ( QgsComposition *composition, int id ) :
    QWidget(),
    Q3CanvasPolygonalItem(0),
    mFrame(false),
    mAreaPoints(4),
    mBoundingRect(0,0,0,0)
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::QgsComposerPicture()" << std::endl;
#endif

    mComposition = composition;
    mId  = id;

    init();

    readSettings();

    loadPicture();
    adjustPictureSize();

    // Add to canvas
    setCanvas(mComposition->canvas());

    recalculate();

    Q3CanvasPolygonalItem::show();
    Q3CanvasPolygonalItem::update();
}

void QgsComposerPicture::init ( void ) 
{
    mSelected = false;
    for ( int i = 0; i < 4; i++ ) 
    {
	mAreaPoints[i] = QPoint( 0, 0 );
    }

    // Rectangle
    Q3CanvasPolygonalItem::setZ(60);
    setActive(true);
}

void QgsComposerPicture::loadPicture ( void ) 
{
#ifdef QGISDEBUG
    std::cerr << "QgsComposerPicture::loadPicture() mPicturePath = " << mPicturePath.toLocal8Bit().data() << std::endl;
#endif
    mPicture = Q3Picture(); 
    mPictureValid = false;

    if ( !mPicturePath.isNull() ) 
    {
	if ( mPicturePath.lower().right(3) == "svg" )
	{
	    if ( !mPicture.load ( mPicturePath, "svg" ) )
	    {
		std::cerr << "Cannot load svg" << std::endl;
	    }	
	    else
	    {
		mPictureValid = true;
	    }
	}
	else
	{
	    QImage image;
	    if ( !image.load(mPicturePath) )
	    {
		std::cerr << "Cannot load raster" << std::endl;
	    }
	    else
	    {	
		QPainter  p;
		p.begin( &mPicture );
		p.drawImage ( 0, 0, image ); 
		p.end();	
		mPictureValid = true;
	    }
	}
    }

    if ( !mPictureValid ) 
    {
        // Dummy picture
        QPainter  p;
	p.begin( &mPicture );
  	p.setPen( QPen(QColor(0,0,0), 1) );
	p.setBrush( QBrush( QColor( 150, 150, 150) ) );

        int w, h; 
        if ( mWidth > 0 && mHeight > 0 
             && mWidth/mHeight > 0.001 && mWidth/mHeight < 1000 ) 
	{
	    w = mWidth;
	    h = mHeight;
        }
	else
 	{
	    w = 100;
	    h = 100;
	}
	
	p.drawRect ( 0, 0, w, h ); 
	p.drawLine ( 0, 0, w-1, h-1 );
	p.drawLine ( w-1, 0, 0, h-1 );

	p.end();	

 	mPicture.setBoundingRect ( QRect ( 0, 0, w, h ) ); 
    }
}

bool QgsComposerPicture::pictureValid ( void )
{
    return mPictureValid;
}

QgsComposerPicture::~QgsComposerPicture()
{
#ifdef QGISDEBUG
    std::cerr << "QgsComposerPicture::~QgsComposerPicture()" << std::endl;
#endif
    Q3CanvasItem::hide();
}

void QgsComposerPicture::draw ( QPainter & painter )
{
#ifdef QGISDEBUG
    std::cerr << "QgsComposerPicture::draw()" << std::endl;
#endif

    QRect box = mPicture.boundingRect();
    double scale = 1. * mWidth / box.width(); 
    
    painter.save();

    painter.translate ( mX, mY );
    painter.scale ( scale, scale );
    painter.rotate ( -mAngle );
    
    painter.drawPicture ( -box.x(), -box.y(), mPicture );
    
    painter.restore();

    if ( mFrame ) {
	// TODO: rect is not correct, +/- 1 pixle - Qt3?
  	painter.setPen( QPen(QColor(0,0,0), 1) );
	painter.setBrush( QBrush( Qt::NoBrush ) );

	painter.save();
        painter.translate ( mX, mY );
        painter.rotate ( -mAngle );
      
	painter.drawRect ( 0, 0, mWidth, mHeight ); 
  	painter.restore();
    }

    // Show selected / Highlight
    if ( mSelected && plotStyle() == QgsComposition::Preview ) {
        painter.setPen( mComposition->selectionPen() );
        painter.setBrush( mComposition->selectionBrush() );
  
      	int s = mComposition->selectionBoxSize();

	for ( int i = 0; i < 4; i++ ) 
	{
	    painter.save();
	    painter.translate ( mAreaPoints.point(i).x(), mAreaPoints.point(i).y() );
	    painter.rotate ( -mAngle + i * 90 );
	  
	    painter.drawRect ( 0, 0, s, s );
	    painter.restore();
	}
    }
}

void QgsComposerPicture::drawShape( QPainter & painter )
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::drawShape" << std::endl;
#endif
    draw ( painter );
}

void QgsComposerPicture::setBox ( int x1, int y1, int x2, int y2 )
{
    int tmp;

    if ( x1 > x2 ) { tmp = x1; x1 = x2; x2 = tmp; }
    if ( y1 > y2 ) { tmp = y1; y1 = y2; y2 = tmp; }
   
    // Center
    mCX = (x1 + x2) / 2;
    mCY = (y1 + y2) / 2;

    QRect box = mPicture.boundingRect();
    std::cout << "box.width() = " << box.width() << " box.height() = " << box.height() << std::endl;

    mWidth = x2-x1;
    mHeight = y2-y1;
    adjustPictureSize(); 

    recalculate();
}

void QgsComposerPicture::moveBy( double x, double y )
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::moveBy()" << std::endl;
#endif

    mCX += (int) x; 
    mCY += (int) y; 
    recalculate();
}

void QgsComposerPicture::recalculate()
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::recalculate" << std::endl;
#endif
    
    Q3CanvasPolygonalItem::invalidate();

    QRect box = mPicture.boundingRect();

    double angle = PI * mAngle / 180;
    
    // Angle between vertical in picture space and the vector 
    // from center to upper left corner of the picture
    double anglePicture = atan2 ( (double)box.width(), (double)box.height() );

    // Angle (clockwise) between horizontal in paper space
    // and the vector from center to upper left corner of the picture
    double anglePaper = PI / 2 - anglePicture - angle;

    // Distance from center to upper left corner in canvas units
    double r = sqrt ((double)(mWidth*mWidth/4 + mHeight*mHeight/4) );

    // Position of upper left corner in map
    int dx = (int) ( r * cos ( anglePaper ) );
    int dy = (int) ( r * sin ( anglePaper ) );

    mX = mCX - dx;
    mY = mCY - dy;
    
    // Area points
    mAreaPoints[0] = QPoint( mCX-dx, mCY-dy );
    mAreaPoints[2] = QPoint( mCX+dx, mCY+dy );

    anglePaper = angle + PI / 2 - anglePicture;
    dx = (int) ( r * cos ( anglePaper ) );
    dy = (int) ( r * sin ( anglePaper ) );
    mAreaPoints[1] = QPoint( mCX+dx, mCY-dy );
    mAreaPoints[3] = QPoint( mCX-dx, mCY+dy );

    mBoundingRect = mAreaPoints.boundingRect();
    
    Q3CanvasPolygonalItem::canvas()->setChanged(mBoundingRect);
    Q3CanvasPolygonalItem::update();
    Q3CanvasPolygonalItem::canvas()->update();
}

QRect QgsComposerPicture::boundingRect ( void ) const
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::boundingRect" << std::endl;
#endif
    return mBoundingRect;
}

Q3PointArray QgsComposerPicture::areaPoints() const
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::areaPoints" << std::endl;
#endif

    return mAreaPoints;
}


void QgsComposerPicture::on_mFrameCheckBox_stateChanged ( int )
{
    mFrame = mFrameCheckBox->isChecked();

    Q3CanvasPolygonalItem::update();
    Q3CanvasPolygonalItem::canvas()->update();

    writeSettings();
}

void QgsComposerPicture::on_mAngleLineEdit_returnPressed ( )
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::on_mAngleLineEdit_returnPressed()" << std::endl;
#endif

    mAngle = mAngleLineEdit->text().toDouble();

    recalculate();

    writeSettings();
}

void QgsComposerPicture::on_mWidthLineEdit_returnPressed ( )
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::on_mWidthLineEdit_returnPressed()" << std::endl;
#endif

    mWidth = mComposition->fromMM ( mWidthLineEdit->text().toDouble() );

    QRect box = mPicture.boundingRect();
    mHeight = mWidth*box.height()/box.width();
    setOptions();

    recalculate();

    writeSettings();
}

void QgsComposerPicture::on_mPictureBrowseButton_clicked ( )
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::on_mPictureBrowseButton_clicked()" << std::endl;
#endif
 
    QString file = QgsComposerPicture::pictureDialog();

    if ( file.isNull() ) return;
    
    mPicturePath = file;
    mPictureLineEdit->setText ( mPicturePath );

    pictureChanged();
}

void QgsComposerPicture::pictureChanged ( )
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::pictureChanged()" << std::endl;
#endif

    mPicturePath = mPictureLineEdit->text();

    loadPicture();

    if ( !mPictureValid ) {
        QMessageBox::warning( 0, "Warning",
                        "Cannot load picture." );
    }
    else
    {
        adjustPictureSize();
        setOptions();
        recalculate();
    }
}

void QgsComposerPicture::on_mPictureLineEdit_returnPressed ( )
{
  pictureChanged();
}

void QgsComposerPicture::adjustPictureSize ( )
{
    // Addjust to original size
    QRect box = mPicture.boundingRect();

    if ( box.width() == 0 || box.height() == 0
	 || mWidth == 0 || mHeight == 0 )
    {
	mWidth = 0;
	mHeight = 0;	
        return;
    }

    if ( 1.*box.width()/box.height() > 1.*mWidth/mHeight )
    {
	mHeight = mWidth*box.height()/box.width();
    }
    else
    {
	mWidth = mHeight*box.width()/box.height();
    }
}

void QgsComposerPicture::setOptions ( void )
{ 
    mPictureLineEdit->setText ( mPicturePath );
    mWidthLineEdit->setText ( QString("%1").arg( mComposition->toMM(mWidth), 0,'g') );
    mHeightLineEdit->setText ( QString("%1").arg( mComposition->toMM(mHeight), 0,'g') );
    mAngleLineEdit->setText ( QString::number ( mAngle ) );
    mFrameCheckBox->setChecked ( mFrame );
}

void QgsComposerPicture::setSelected (  bool s ) 
{
    mSelected = s;
    Q3CanvasPolygonalItem::update(); // show highlight
}    

bool QgsComposerPicture::selected( void )
{
    return mSelected;
}

QWidget *QgsComposerPicture::options ( void )
{
    setOptions ();
    return ( dynamic_cast <QWidget *> (this) ); 
}

QString QgsComposerPicture::pictureDialog ( void )
{
    QString filters = "Pictures ( *.svg *.SVG ";
    Q3StrList formats = QPictureIO::outputFormats();

    for ( int i = 0; i < formats.count(); i++ )
    {
        QString frmt = QPictureIO::outputFormats().at( i );
        QString fltr = " *." + frmt.lower() + " *." + frmt.upper();
        filters += fltr;
    }
    filters += " )";

    QString file = QFileDialog::getOpenFileName(
                    0,
                    "Choose a file",
                    ".",
                    filters );

    return file; 
}

bool QgsComposerPicture::writeSettings ( void )  
{
#ifdef QGISDEBUG
    std::cout << "QgsComposerPicture::writeSettings" << std::endl;
#endif

    QString path;
    path.sprintf("/composition_%d/picture_%d/", mComposition->id(), mId ); 

    QgsProject::instance()->writeEntry( "Compositions", path+"picture", mPicturePath );

    QgsProject::instance()->writeEntry( "Compositions", path+"x", mComposition->toMM(mCX) );
    QgsProject::instance()->writeEntry( "Compositions", path+"y", mComposition->toMM(mCY) );
    QgsProject::instance()->writeEntry( "Compositions", path+"width", mComposition->toMM(mWidth) );
    QgsProject::instance()->writeEntry( "Compositions", path+"height", mComposition->toMM(mHeight) );

    QgsProject::instance()->writeEntry( "Compositions", path+"angle", mAngle );

    QgsProject::instance()->writeEntry( "Compositions", path+"frame", mFrame );

    return true; 
}

bool QgsComposerPicture::readSettings ( void )
{
    bool ok;
    QString path;
    path.sprintf("/composition_%d/picture_%d/", mComposition->id(), mId );

    mPicturePath = QgsProject::instance()->readEntry( "Compositions", path+"picture", "", &ok) ;

    mCX = mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"x", 0, &ok));
    mCY = mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"y", 0, &ok));
    mWidth = mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"width", 0, &ok));
    mHeight = mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"height", 0, &ok));

    mAngle = mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"angle", 0, &ok));

    mFrame = QgsProject::instance()->readBoolEntry("Compositions", path+"frame", true, &ok);

    return true;
}

bool QgsComposerPicture::removeSettings( void )
{
#ifdef QGISDEBUG
    std::cerr << "QgsComposerPicture::deleteSettings" << std::endl;
#endif

    QString path;
    path.sprintf("/composition_%d/picture_%d", mComposition->id(), mId ); 
    return QgsProject::instance()->removeEntry ( "Compositions", path );
}

bool QgsComposerPicture::writeXML( QDomNode & node, QDomDocument & document, bool temp )
{
    return true;
}

bool QgsComposerPicture::readXML( QDomNode & node )
{
    return true;
}

