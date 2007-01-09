/***************************************************************************
                              qgscomposition.cpp
                             -------------------
    begin                : January 2005
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
#include "qgscomposition.h"

#include "qgscomposer.h"
#include "qgscomposeritem.h"
#include "qgscomposerlabel.h"
#include "qgscomposermap.h"
#include "qgscomposerpicture.h"
#include "qgscomposerscalebar.h"
#include "qgscomposervectorlegend.h"
#include "qgscomposerview.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"

#include <Q3CanvasRectangle>
#include <QMatrix>
#include <QMessageBox>

#include <iostream>

QgsCompositionPaper::QgsCompositionPaper ( QString name, int w, int h, bool c) 
  :mName(name), mWidth(w), mHeight(h), mCustom(c)
{
}

QgsCompositionPaper::~QgsCompositionPaper ( )
{
}


QgsComposition::QgsComposition( QgsComposer *c, int id ) 
{
  setupUi(this);
  connect(mPaperWidthLineEdit, SIGNAL(returnPressed()), this, SLOT(paperSizeChanged()));
  connect(mPaperHeightLineEdit, SIGNAL(returnPressed()), this, SLOT(paperSizeChanged()));
  connect(mPaperOrientationComboBox, SIGNAL(activated(int)), this, SLOT(paperSizeChanged()));
  connect(mPaperSizeComboBox, SIGNAL(activated(int)), this, SLOT(paperSizeChanged()));
  connect(mResolutionLineEdit, SIGNAL(returnPressed()), this, SLOT(resolutionChanged()));

#ifdef QGISDEBUG
  std::cerr << "QgsComposition::QgsComposition()" << std::endl;
#endif

  mId = id;
  mNextItemId = 1;
  mCanvas = 0;
  mPaperItem = 0;

  mComposer = c;
  mMapCanvas = c->mapCanvas();
  mView = c->view();
  mSelectedItem = 0;
  mPlotStyle = Preview;
  
  // Attention: Qt4.1 writes line width to PS/PDF still as integer 
  // (using QPen->width() to get the value) so we MUST use mScale > 1
  
  // Note: It seems that Qt4.2 is using widthF so that we can set mScale to 1
  //       and hopefuly we can remove mScale completely

  // Note: scale 10 make it inacceptable slow: QgsComposerMap 900x900mm on paper 1500x1000
  //          cannot be smoothly moved even if mPreviewMode == Rectangle and no zoom in
  //       scale 2 results in minimum line width 0.5 mmm which is too much
  mScale = 1;

  // Add paper sizes and set default. 
  mPapers.push_back ( QgsCompositionPaper( tr("Custom"), 0, 0, 1 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("A5 (148x210 mm)"), 148, 210 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("A4 (210x297 mm)"), 210, 297 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("A3 (297x420 mm)"), 297, 420 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("A2 (420x594 mm)"), 420, 594 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("A1 (594x841 mm)"), 594, 841 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("A0 (841x1189 mm)"), 841, 1189 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("B5 (176 x 250 mm)"), 176, 250 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("B4 (250 x 353 mm)"), 250, 353 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("B3 (353 x 500 mm)"), 353, 500 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("B2 (500 x 707 mm)"), 500, 707 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("B1 (707 x 1000 mm)"), 707, 1000 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("B0 (1000 x 1414 mm)"), 1000, 1414 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("Letter (8.5x11 inches)"),  216, 279 ) );
  mPapers.push_back ( QgsCompositionPaper( tr("Legal (8.5x14 inches)"), 216, 356 ) );

  mPaper = mDefaultPaper = mCustomPaper = 0;
  for( int i = 0; i < mPapers.size(); i++ ) {
    mPaperSizeComboBox->insertItem( mPapers[i].mName );
    // Map - A4 land for now, if future read from template
    if ( mPapers[i].mWidth == 210 && mPapers[i].mHeight == 297 ){
      mDefaultPaper = i;
    }
    if ( mPapers[i].mCustom ) mCustomPaper = i;
  }

  // Orientation
  mPaperOrientationComboBox->insertItem( tr("Portrait"), Portrait );
  mPaperOrientationComboBox->insertItem( tr("Landscape"), Landscape );
  mPaperOrientation = Landscape;

  mPaperUnitsComboBox->insertItem( "mm" );

  // Create canvas 
  mPaperWidth = 1;
  mPaperHeight = 1;
  createCanvas();

  // Tool
  mRectangleItem = 0;
  mNewCanvasItem = 0;
  mTool = Select;
  mToolStep = 0;
}

void QgsComposition::createDefault(void) 
{
  mPaperSizeComboBox->setCurrentItem(mDefaultPaper);
  mPaperOrientationComboBox->setCurrentItem(Landscape);

  mUserPaperWidth = mPapers[mDefaultPaper].mWidth;
  mUserPaperHeight = mPapers[mDefaultPaper].mHeight;

  recalculate();

  mResolution = 300;

  setOptions();

  // Add the map to coposition
  /*
     QgsComposerMap *m = new QgsComposerMap ( this, mNextItemId++, 
     mScale*15, mScale*15, mScale*180, mScale*180 );
     mItems.push_back(m);
     */

  // Add vector legend
  /*
     QgsComposerVectorLegend *vl = new QgsComposerVectorLegend ( this, mNextItemId++, 
     mScale*210, mScale*100, 10 );
     mItems.push_back(vl);
     */

  // Title
  /*
     QgsComposerLabel *tit = new QgsComposerLabel ( this, mNextItemId++, 
     mScale*238, mScale*40, "Map", 24 );
     mItems.push_back(tit);
     */

  // Tool
  mRectangleItem = 0;
  mNewCanvasItem = 0;
  mTool = Select;
  mToolStep = 0;

  writeSettings();
}

void QgsComposition::createCanvas(void) 
{
  if ( mCanvas ) delete mCanvas;

  mCanvas = new Q3Canvas ( (int) mPaperWidth * mScale, (int) mPaperHeight * mScale);
  mCanvas->setBackgroundColor( QColor(180,180,180) );
  // mCanvas->setDoubleBuffering( false );  // makes the move very unpleasant and doesn't make it faster

  // Paper
  if ( mPaperItem ) delete mPaperItem;
  mPaperItem = new Q3CanvasRectangle( 0, 0, (int) mPaperWidth * mScale, 
      (int) mPaperHeight * mScale, mCanvas );
  mPaperItem->setBrush( QColor(255,255,255) );
  mPaperItem->setPen( QPen(QColor(0,0,0), 1) );
  mPaperItem->setZ(0);
  mPaperItem->setActive(false);
  mPaperItem->show();
}

void QgsComposition::resizeCanvas(void) 
{
  mCanvas->resize ( (int) mPaperWidth * mScale, (int) mPaperHeight * mScale );
#ifdef QGISDEBUG
  std::cout << "mCanvas width = " << mCanvas->width() << " height = " << mCanvas->height() << std::endl;
#endif
  mPaperItem->setSize ( (int) mPaperWidth * mScale, (int) mPaperHeight * mScale );
}

QgsComposition::~QgsComposition()
{
#ifdef QGISDEBUG
  std::cerr << "QgsComposition::~QgsComposition" << std::endl;
#endif
  mView->setCanvas ( 0 );

  if ( mPaperItem ) delete mPaperItem;

  for (std::list < QgsComposerItem * >::iterator it = mItems.begin(); 
      it != mItems.end(); ++it) 
  {
    delete *it;
  }

  if ( mCanvas ) delete mCanvas;
}

QgsMapCanvas *QgsComposition::mapCanvas(void) { return mMapCanvas; }

void QgsComposition::setActive (  bool active )
{
  if ( active ) {
    mView->setCanvas ( mCanvas );
    mComposer->showCompositionOptions ( this );
  } else {
    // TODO
  }
}

void QgsComposition::contentsMousePressEvent(QMouseEvent* e)
{
#ifdef QGISDEBUG
  std::cerr << "QgsComposition::contentsMousePressEvent() mTool = " << mTool << " mToolStep = "
    << mToolStep << std::endl;
#endif

  QPoint p = mView->inverseWorldMatrix().map(e->pos());
  mLastPoint = p;

  double x,y;
  mView->inverseWorldMatrix().map( e->pos().x(), e->pos().y(), &x, &y );

  switch ( mTool ) {
    case Select:
      {
        Q3CanvasItemList l = mCanvas->collisions(p);

        Q3CanvasItem * newItem = 0;

        for ( Q3CanvasItemList::Iterator it=l.fromLast(); it!=l.end(); --it) {
          if (! (*it)->isActive() ) continue;
          newItem = *it;
        }

        if ( newItem ) { // found
          if ( newItem != mSelectedItem ) { // Show options
            if ( mSelectedItem ) {
              QgsComposerItem *coi = dynamic_cast <QgsComposerItem *> (mSelectedItem);
              coi->setSelected ( false );
            }

            QgsComposerItem *coi = dynamic_cast <QgsComposerItem *> (newItem);
            coi->setSelected ( true );

            mComposer->showItemOptions ( coi->options() );
            mSelectedItem = newItem;
          } 
          mLastX = x;
          mLastY = y;
        } else { // not found
          if ( mSelectedItem ) {
            QgsComposerItem *coi = dynamic_cast <QgsComposerItem *> (mSelectedItem);
            coi->setSelected ( false );
          }
          mSelectedItem = 0;
          mComposer->showItemOptions ( (QWidget *) 0 ); // hide old options
        }
        mCanvas->update();
      }
      break;

    case AddMap:
#ifdef QGISDEBUG
      std::cerr << "AddMap" << std::endl;
#endif
      if ( mToolStep == 0 ) {
        mRectangleItem = new Q3CanvasRectangle( p.x(), p.y(), 0, 0, mCanvas );
        mRectangleItem->setBrush( Qt::NoBrush );
        mRectangleItem->setPen( QPen(QColor(0,0,0), 1) );
        mRectangleItem->setZ(100);
        mRectangleItem->setActive(false);
        mRectangleItem->show();
        mToolStep = 1;
      }
#ifdef QGISDEBUG
      std::cerr << "mToolStep = " << mToolStep << std::endl;
#endif
      break;

    case AddVectorLegend:
      {
        mNewCanvasItem->setX( p.x() );
        mNewCanvasItem->setY( p.y() );
        QgsComposerVectorLegend *vl = dynamic_cast <QgsComposerVectorLegend*> (mNewCanvasItem);
        vl->writeSettings();
        mItems.push_back(vl);
        mNewCanvasItem = 0;
        mComposer->selectItem(); // usually just one legend

        // Select and show options
        vl->setSelected ( true );
        mComposer->showItemOptions ( vl->options() );
        mSelectedItem = dynamic_cast <Q3CanvasItem*> (vl);

        mCanvas->update();

	// Remember this position for later
	mLastX = x;
	mLastY = y;
      }
      break;

    case AddLabel:
      {
        mNewCanvasItem->setX( p.x() );
        mNewCanvasItem->setY( p.y() );
        QgsComposerLabel *lab = dynamic_cast <QgsComposerLabel*> (mNewCanvasItem);
        lab->writeSettings();
        mItems.push_back(lab);
        mNewCanvasItem = 0;
        mComposer->selectItem(); // usually just one ???

        // Select and show options
        lab->setSelected ( true );
        mComposer->showItemOptions ( lab->options() );
        mSelectedItem = dynamic_cast <Q3CanvasItem*> (lab);

        mCanvas->update();

	// Remember this position for later
	mLastX = x;
	mLastY = y;
      }
      break;

    case AddScalebar:
      {
        mNewCanvasItem->setX( p.x() );
        mNewCanvasItem->setY( p.y() );
        QgsComposerScalebar *sb = dynamic_cast <QgsComposerScalebar*> (mNewCanvasItem);
        sb->writeSettings();
        mItems.push_back(sb);
        mNewCanvasItem = 0;
        mComposer->selectItem(); // usually just one ???

        // Select and show options
        sb->setSelected ( true );
        mComposer->showItemOptions ( sb->options() );
        mSelectedItem = dynamic_cast <Q3CanvasItem*> (sb);

        mCanvas->update();

	// Remember this position for later
	mLastX = x;
	mLastY = y;
      }
      break;

    case AddPicture:
      {
        //mNewCanvasItem->setX( p.x() );
        //mNewCanvasItem->setY( p.y() );

        QgsComposerPicture *pi = dynamic_cast <QgsComposerPicture*> (mNewCanvasItem);
	//pi->setSize (0, 0);
	pi->setBox ( p.x(), p.y(), p.x(), p.y() );

        mCanvas->update();

        mToolStep = 1;
	
      }
      break;
  }
}

void QgsComposition::contentsMouseMoveEvent(QMouseEvent* e)
{
#ifdef QGISDEBUG
  std::cerr << "QgsComposition::contentsMouseMoveEvent() mTool = " << mTool << " mToolStep = "
    << mToolStep << std::endl;
#endif

  QPoint p = mView->inverseWorldMatrix().map(e->pos());

  switch ( mTool ) {
    case Select:
      if ( mSelectedItem ) {
        double x,y;
        mView->inverseWorldMatrix().map( e->pos().x(), e->pos().y(), &x, &y );

        mSelectedItem->moveBy ( x - mLastX, y - mLastY );

        mLastX = x;
        mLastY = y;
        mCanvas->update();
      }
      break;

    case AddMap:
      if ( mToolStep == 1 ) { // draw rectangle
        double x, y;
        int w, h;

        x = p.x() < mRectangleItem->x() ? p.x() : mRectangleItem->x();
        y = p.y() < mRectangleItem->y() ? p.y() : mRectangleItem->y();

        w = abs ( p.x() - (int)mRectangleItem->x() );
        h = abs ( p.y() - (int)mRectangleItem->y() );

        mRectangleItem->setX(x);
        mRectangleItem->setY(y);

        mRectangleItem->setSize(w,h);

        mCanvas->update();
      }
      break;

    case AddVectorLegend:
    case AddLabel:
    case AddScalebar:
      mNewCanvasItem->move ( p.x(), p.y() );
      mCanvas->update();
      break;

    case AddPicture:
      if ( mToolStep == 1 ) { // draw rectangle
        QgsComposerPicture *pi = dynamic_cast <QgsComposerPicture*> (mNewCanvasItem);

	pi->setBox ( mLastPoint.x(), mLastPoint.y(), p.x(), p.y() );
/*
        double x, y;
        int w, h;

        x = p.x() < pi->QCanvasRectangle::x() ? p.x() : pi->QCanvasRectangle::x();
        y = p.y() < pi->QCanvasRectangle::y() ? p.y() : pi->QCanvasRectangle::y();

        w = abs ( p.x() - (int)pi->QCanvasRectangle::x() );
        h = abs ( p.y() - (int)pi->QCanvasRectangle::y() );

        pi->setX(x);
        pi->setY(y);

        pi->setSize(w,h);
*/

        mCanvas->update();
      	break;
      }
  }
}

void QgsComposition::contentsMouseReleaseEvent(QMouseEvent* e)
{
#ifdef QGISDEBUG
  std::cerr << "QgsComposition::contentsMouseReleaseEvent() mTool = " << mTool 
    << " mToolStep = " << mToolStep << std::endl;
#endif

  QPoint p = mView->inverseWorldMatrix().map(e->pos());

  switch ( mTool ) {
    case AddMap: // mToolStep should be always 1 but rectangle can be 0 size
      {
        int x = (int) mRectangleItem->x();
        int y = (int) mRectangleItem->y();
        int w = mRectangleItem->width();
        int h = mRectangleItem->height();
        delete mRectangleItem;
        mRectangleItem = 0;

        if ( w > 0 && h > 0 ) {
          mComposer->selectItem(); // usually just one map

          QgsComposerMap *m = new QgsComposerMap ( this, mNextItemId++, x, y, w, h );

          m->setUserExtent( mMapCanvas->extent());
          mItems.push_back(m);
          m->setSelected ( true );

          if ( mSelectedItem ) {
            QgsComposerItem *coi = dynamic_cast <QgsComposerItem *> (mSelectedItem);
            coi->setSelected ( false );
          }

          m->setSelected ( true );
          mComposer->showItemOptions ( m->options() );
          mSelectedItem = dynamic_cast <Q3CanvasItem *> (m);
        } else {
          mToolStep = 0;
        }
        mCanvas->setChanged ( QRect( x, y, w, h) ); // Should not be necessary
        mCanvas->update();
      }
      break;

    case AddPicture:
      {
        QgsComposerPicture *pi = dynamic_cast <QgsComposerPicture*> (mNewCanvasItem);

        if (  mLastPoint.x()-p.x() != 0 && mLastPoint.y()-p.y() != 0 ) {
	    mNewCanvasItem = 0; // !!! Must be before mComposer->selectItem()
	    mComposer->selectItem(); // usually just one ???

	    pi->writeSettings();
	    mItems.push_back(pi);

	    pi->setSelected ( true );
	    mComposer->showItemOptions ( pi->options() );
	    mSelectedItem = dynamic_cast <Q3CanvasItem*> (pi);

            mCanvas->update();

        } else {
            mToolStep = 0;
        }
      }
      break;

    case Select:
      if ( mSelectedItem ) {
        // the object was probably moved
        QgsComposerItem *ci = dynamic_cast <QgsComposerItem *> (mSelectedItem);
        ci->writeSettings();
      }
      break;
  }
}

void QgsComposition::keyPressEvent ( QKeyEvent * e )
{
#ifdef QGISDEBUG
  std::cout << "QgsComposition::keyPressEvent() key = " << e->key() << std::endl;
#endif

  if ( e->key() == Qt::Key_Delete && mSelectedItem ) { // delete

    QgsComposerItem *coi = dynamic_cast <QgsComposerItem *> (mSelectedItem);
    coi->setSelected ( false );
    coi->removeSettings();
    for (std::list < QgsComposerItem * >::iterator it = mItems.begin(); 
        it != mItems.end(); ++it) 
    {
      if ( (*it) == coi ) {
        mItems.erase ( it );
        break;
      }
    }
    delete (mSelectedItem);
    mSelectedItem = 0;
    mCanvas->update();
  }
}

void QgsComposition::paperSizeChanged ( void )
{
#ifdef QGISDEBUG
  std::cout << "QgsComposition::paperSizeChanged" << std::endl;
#endif

  mPaper = mPaperSizeComboBox->currentItem();
  mPaperOrientation = mPaperOrientationComboBox->currentItem();
#ifdef QGISDEBUG
  std::cout << "custom = " << mPapers[mPaper].mCustom << std::endl;
  std::cout << "orientation = " << mPaperOrientation << std::endl;
#endif
  if ( mPapers[mPaper].mCustom ) {
    mUserPaperWidth = mPaperWidthLineEdit->text().toDouble();
    mUserPaperHeight = mPaperHeightLineEdit->text().toDouble();
    mPaperWidthLineEdit->setEnabled( TRUE );
    mPaperHeightLineEdit->setEnabled( TRUE );
  } else {
    mUserPaperWidth = mPapers[mPaper].mWidth;
    mUserPaperHeight = mPapers[mPaper].mHeight;
    mPaperWidthLineEdit->setEnabled( FALSE );
    mPaperHeightLineEdit->setEnabled( FALSE );
    setOptions();
  }

  try
  {
    recalculate();
  }
  catch (std::bad_alloc& ba)
  {
    // A better solution here would be to set the canvas back to the
    // original size and carry on, but for the moment this will
    // prevent a crash due to an uncaught exception.
    QMessageBox::critical( 0, tr("Out of memory"),
                           tr("Qgis is unable to resize the paper size due to "
                              "insufficient memory.\n It is best that you avoid "
                              "using the map composer until you restart qgis.\n") );
  }

  mView->repaintContents();
  writeSettings();
}

void QgsComposition::recalculate ( void ) 
{
  if ( (mPaperOrientation == Portrait &&  mUserPaperWidth < mUserPaperHeight) ||
      (mPaperOrientation == Landscape &&  mUserPaperWidth > mUserPaperHeight) ) 
  {
    mPaperWidth = mUserPaperWidth;
    mPaperHeight = mUserPaperHeight;
  } else {
    mPaperWidth = mUserPaperHeight;
    mPaperHeight = mUserPaperWidth;
  }
#ifdef QGISDEBUG
  std::cout << "mPaperWidth = " << mPaperWidth << " mPaperHeight = " << mPaperHeight << std::endl;
#endif
  resizeCanvas();
  mComposer->zoomFull();
}

void QgsComposition::resolutionChanged ( void )
{
  mResolution = mResolutionLineEdit->text().toInt();
  writeSettings();
}

void QgsComposition::setOptions ( void )
{
  mPaperSizeComboBox->setCurrentItem(mPaper);
  mPaperOrientationComboBox->setCurrentItem(mPaperOrientation);
  mPaperWidthLineEdit->setText ( QString("%1").arg(mUserPaperWidth,0,'g') );
  mPaperHeightLineEdit->setText ( QString("%1").arg(mUserPaperHeight,0,'g') );
  mResolutionLineEdit->setText ( QString("%1").arg(mResolution) );
}

void QgsComposition::setPlotStyle (  PlotStyle p )
{
  mPlotStyle = p;

  // Set all items
  for (std::list < QgsComposerItem * >::iterator it = mItems.begin(); it != mItems.end(); ++it) {
    (*it)->setPlotStyle( p ) ;
  }

  // Remove paper if Print, reset if Preview
  if ( mPlotStyle == Print ) {
    mPaperItem->setCanvas(0);
    mCanvas->setBackgroundColor( Qt::white );
  } else { 
    mPaperItem->setCanvas(mCanvas);
    mCanvas->setBackgroundColor( QColor(180,180,180) );

  }
}

double QgsComposition::viewScale ( void ) 
{
  double scale = mView->worldMatrix().m11();
  return scale; 
}

void QgsComposition::refresh()
{
  // TODO add signals to map canvas
  for (std::list < QgsComposerItem * >::iterator it = mItems.begin(); it != mItems.end(); ++it)    {
    QgsComposerItem *ci = (*it);
    if (  typeid (*ci) == typeid(QgsComposerMap) ) {
      QgsComposerMap *cm = dynamic_cast<QgsComposerMap*>(ci);
      cm->setCacheUpdated(false);
    } else if (  typeid (*ci) == typeid(QgsComposerVectorLegend) ) {
      QgsComposerVectorLegend *vl = dynamic_cast<QgsComposerVectorLegend*>(ci);
      vl->recalculate();
    }
  }

}

int QgsComposition::id ( void ) { return mId; }

QgsComposer *QgsComposition::composer(void) { return mComposer; }

Q3Canvas *QgsComposition::canvas(void) { return mCanvas; }

double QgsComposition::paperWidth ( void ) { return mPaperWidth; }

double QgsComposition::paperHeight ( void ) { return mPaperHeight; }

int QgsComposition::paperOrientation ( void ) { return mPaperOrientation; }

int QgsComposition::resolution ( void ) { return mResolution; }

int QgsComposition::scale( void ) { 
  return mScale; 
}

double QgsComposition::toMM ( int v ) { return v/mScale ; }

int QgsComposition::fromMM ( double v ) { return (int) (v * mScale); }

void QgsComposition::setTool ( Tool tool )
{
  // Stop old in progress
  mView->viewport()->setMouseTracking ( false ); // stop mouse tracking
  if ( mSelectedItem ) {
    QgsComposerItem *coi = dynamic_cast <QgsComposerItem *> (mSelectedItem);
    coi->setSelected ( false );
    mCanvas->update();
  }
  mSelectedItem = 0;
  mComposer->showItemOptions ( (QWidget *) 0 );

  if ( mNewCanvasItem ) {
    mNewCanvasItem->setX( -1000 );
    mNewCanvasItem->setY( -1000 );
    mCanvas->update();

    delete mNewCanvasItem;
    mNewCanvasItem = 0;
  }

  if ( mRectangleItem ) {
    delete mRectangleItem;
    mRectangleItem = 0;
  }

  // Start new
  if ( tool == AddVectorLegend ) { // Create temporary object
    if ( mNewCanvasItem ) delete mNewCanvasItem;

    // Create new object outside the visible area
    QgsComposerVectorLegend *vl = new QgsComposerVectorLegend ( this, mNextItemId++, 
        (-1000)*mScale, (-1000)*mScale, (int) (mScale*mPaperHeight/50) );
    mNewCanvasItem = dynamic_cast <Q3CanvasItem *> (vl);

    mComposer->showItemOptions ( vl->options() );

    mView->viewport()->setMouseTracking ( true ); // to recieve mouse move
  } else if ( tool == AddLabel ) {
    if ( mNewCanvasItem ) delete mNewCanvasItem;

    // Create new object outside the visible area
    QgsComposerLabel *lab = new QgsComposerLabel ( this, mNextItemId++, 
        (-1000)*mScale, (-1000)*mScale, tr("Label"), (int) (mScale*mPaperHeight/40) );
    mNewCanvasItem = dynamic_cast <Q3CanvasItem *> (lab);
    mComposer->showItemOptions ( lab->options() );

    mView->viewport()->setMouseTracking ( true ); // to recieve mouse move
  } else if ( tool == AddScalebar ) {
    if ( mNewCanvasItem ) delete mNewCanvasItem;

    // Create new object outside the visible area
    QgsComposerScalebar *sb = new QgsComposerScalebar ( this, mNextItemId++, 
        (-1000)*mScale, (-1000)*mScale );
    mNewCanvasItem = dynamic_cast <Q3CanvasItem *> (sb);
    mComposer->showItemOptions ( sb->options() );

    mView->viewport()->setMouseTracking ( true ); // to recieve mouse move
  } else if ( tool == AddPicture ) {
    if ( mNewCanvasItem ) delete mNewCanvasItem;

    while ( 1 )
    {
	QString file = QgsComposerPicture::pictureDialog();

        if ( file.isNull() )
	{
	    // TODO: This is not nice, because selectItem() calls 
            //       this function, do it better
	    mComposer->selectItem();
	    tool = Select;
	    break;
        }

	// Create new object outside the visible area
	QgsComposerPicture *pi = new QgsComposerPicture ( this, mNextItemId++,      			         		   file );

	if ( pi->pictureValid() )
	{
  	    std::cout << "picture is valid" << std::endl;
	    mNewCanvasItem = dynamic_cast <Q3CanvasItem *> (pi);
	    mComposer->showItemOptions ( pi->options() );

	    mView->viewport()->setMouseTracking ( true ); // to recieve mouse move
            break;
	}
	else
        {
	    QMessageBox::warning( 0, tr("Warning"),
                         tr("Cannot load picture.") );

	    delete pi;
	}
    }
  }

  mTool = tool;
  mToolStep = 0;
}

std::vector<QgsComposerMap*> QgsComposition::maps(void) 
{
  std::vector<QgsComposerMap*> v;
  for (std::list < QgsComposerItem * >::iterator it = mItems.begin(); it != mItems.end(); ++it) {
    QgsComposerItem *ci = (*it);
    if (  typeid (*ci) == typeid(QgsComposerMap) ) {
      v.push_back ( dynamic_cast<QgsComposerMap*>(ci) );
    }
  }

  return v;
}

QgsComposerMap* QgsComposition::map ( int id ) 
{ 
  for (std::list < QgsComposerItem * >::iterator it = mItems.begin(); it != mItems.end(); ++it) {
    QgsComposerItem *ci = (*it);
    if (  ci->id() == id ) {
      return ( dynamic_cast<QgsComposerMap*>(ci) );
    }
  }
  return 0;
}

int QgsComposition::selectionBoxSize ( void )
{
  // Scale rectangle, keep rectangle of fixed size in screen points
  return (int) (7/viewScale());
}

QPen QgsComposition::selectionPen ( void ) 
{
  return QPen( QColor(0,0,255), 0) ;
}

QBrush QgsComposition::selectionBrush ( void )
{
  return QBrush ( QBrush(QColor(0,0,255), Qt::SolidPattern) );
}

void QgsComposition::emitMapChanged ( int id )
{
  emit mapChanged ( id );
}

bool QgsComposition::writeSettings ( void )
{
  QString path, val;
  path.sprintf("/composition_%d/", mId );
  QgsProject::instance()->writeEntry( "Compositions", path+"width", mUserPaperWidth );
  QgsProject::instance()->writeEntry( "Compositions", path+"height", mUserPaperHeight );
  QgsProject::instance()->writeEntry( "Compositions", path+"resolution", mResolution );

  if ( mPaperOrientation == Landscape ) {
    val = "landscape";
  } else {
    val = "portrait";
  }
  QgsProject::instance()->writeEntry( "Compositions", path+"orientation", val );


  return true;
}

bool QgsComposition::readSettings ( void )
{
#ifdef QGISDEBUG
  std::cout << "QgsComposition::readSettings" << std::endl;
#endif

  bool ok;

  mPaper = mCustomPaper;

  QString path, val;
  path.sprintf("/composition_%d/", mId );
  mUserPaperWidth = QgsProject::instance()->readDoubleEntry( "Compositions", path+"width", 297, &ok);
  mUserPaperHeight = QgsProject::instance()->readDoubleEntry( "Compositions", path+"height", 210, &ok);
  mResolution = QgsProject::instance()->readNumEntry( "Compositions", path+"resolution", 300, &ok);

  val = QgsProject::instance()->readEntry( "Compositions", path+"orientation", "landscape", &ok);
  if ( val.compare("landscape") == 0 ) {
    mPaperOrientation = Landscape;
  } else {
    mPaperOrientation = Portrait;
  }

  recalculate();
  setOptions();

  // Create objects
  path.sprintf("/composition_%d", mId );
  QStringList el = QgsProject::instance()->subkeyList ( "Compositions", path );

  // First maps because they can be used by other objects
  for ( QStringList::iterator it = el.begin(); it != el.end(); ++it ) {
#ifdef QGISDEBUG
    std::cout << "key: " << (*it).toLocal8Bit().data() << std::endl;
#endif

    QStringList l = QStringList::split( '_', (*it) );
    if ( l.size() == 2 ) {
      QString name = l.first();
      QString ids = l.last();
      int id = ids.toInt();

      if ( name.compare("map") == 0 ) {
        QgsComposerMap *map = new QgsComposerMap ( this, id );
        mItems.push_back(map);
      }

      if ( id >= mNextItemId ) mNextItemId = id + 1;
    }
  }

  for ( QStringList::iterator it = el.begin(); it != el.end(); ++it ) {
#ifdef QGISDEBUG
    std::cout << "key: " << (*it).toLocal8Bit().data() << std::endl;
#endif

    QStringList l = QStringList::split( '_', (*it) );
    if ( l.size() == 2 ) {
      QString name = l.first();
      QString ids = l.last();
      int id = ids.toInt();

      if ( name.compare("vectorlegend") == 0 ) {
        QgsComposerVectorLegend *vl = new QgsComposerVectorLegend ( this, id );
        mItems.push_back(vl);
      } else if ( name.compare("label") == 0 ) {
        QgsComposerLabel *lab = new QgsComposerLabel ( this, id );
        mItems.push_back(lab);
      } else if ( name.compare("scalebar") == 0 ) {
        QgsComposerScalebar *sb = new QgsComposerScalebar ( this, id );
        mItems.push_back(sb);
      } else if ( name.compare("picture") == 0 ) {
        QgsComposerPicture *pi = new QgsComposerPicture ( this, id );
        mItems.push_back(pi);
      }

      if ( id >= mNextItemId ) mNextItemId = id + 1;
    }
  }


  mCanvas->update();

  return true;
}

