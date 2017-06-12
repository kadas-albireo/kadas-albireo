/***************************************************************************
 *  qgsmilxmaptools.cpp                                                    *
 *  -------------------                                                    *
 *  begin                : Oct 01, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmilxmaptools.h"
#include "qgsmilxannotationitem.h"
#include "qgsmilxlayer.h"
#include "qgisinterface.h"
#include "layertree/qgslayertreeview.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"

#include <QGraphicsRectItem>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>


QgsMilXCreateTool::QgsMilXCreateTool( QgsMapCanvas* canvas, QgsMilXLayer* layer, const QString& symbolXml, const QString &symbolMilitaryName, int nMinPoints, bool hasVariablePoints, const QPixmap& preview )
    : QgsMapTool( canvas ), mSymbolXml( symbolXml ), mSymbolMilitaryName( symbolMilitaryName ), mMinNPoints( nMinPoints ), mNPressedPoints( 0 ), mHasVariablePoints( hasVariablePoints ), mItem( 0 ), mLayer( layer )
{
  setCursor( QCursor( preview, -0.5 * preview.width(), -0.5 * preview.height() ) );
  // If layer is deleted or layers are changed, quit tool
  connect( mLayer, SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
  connect( canvas, SIGNAL( layersChanged( QStringList ) ), this, SLOT( deleteLater() ) );
}

QgsMilXCreateTool::~QgsMilXCreateTool()
{
  // If an item is still set, it means that positioning was not finished when the tool is disabled -> delete it
  delete mItem.data();
}

void QgsMilXCreateTool::canvasPressEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
  {
    if ( mItem == 0 )
    {
      // Deselect any previously selected items
      QgsAnnotationItem* selectedItem = mCanvas->selectedAnnotationItem();
      if ( selectedItem )
      {
        selectedItem->setSelected( false );
      }

      mItem = new QgsMilXAnnotationItem( mCanvas );
      mItem->setMapPosition( toMapCoordinates( e->pos() ) );
      mItem->setSymbolXml( mSymbolXml, mSymbolMilitaryName );
      mItem->setSelected( true );
      setCursor( Qt::CrossCursor );
      mNPressedPoints = 1;
      // Only actually add the point if more than the minimum number have been specified
      // The server automatically adds points up to the minimum number
      if ( mNPressedPoints >= mMinNPoints && mHasVariablePoints )
      {
        mItem->appendPoint( e->pos() );
      }
    }
    else if ( mNPressedPoints < mMinNPoints || mHasVariablePoints )
    {
      ++mNPressedPoints;
      // Only actually add the point if more than the minimum number have been specified
      // The server automatically adds points up to the minimum number
      if ( mNPressedPoints >= mMinNPoints && mHasVariablePoints )
      {
        mItem->appendPoint( e->pos() );
      }
    }

    if ( mNPressedPoints >= mMinNPoints && !mHasVariablePoints )
    {
      // Max points reached, stop
      mItem->finalize();
      mLayer->addItem( mItem->toMilxItem() );
      mNPressedPoints = 0;
      // Delay delete until after refresh, to avoid flickering
      connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( deleteLater() ) );
      mLayer->triggerRepaint();
    }
  }
  else if ( e->button() == Qt::RightButton && mItem != 0 )
  {
    if ( mNPressedPoints + 1 >= mMinNPoints )
    {
      // Done with N point symbol, stop
      mItem->finalize();
      mLayer->addItem( mItem->toMilxItem() );
      // Delay delete until after refresh, to avoid flickering
      connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), this, SLOT( deleteLater() ) );
      mLayer->triggerRepaint();
    }
    else if ( mNPressedPoints + 1 < mMinNPoints )
    {
      // premature stop
      deleteLater();
    }
  }
}

void QgsMilXCreateTool::canvasMoveEvent( QMouseEvent * e )
{
  if ( mItem != 0 && e->buttons() == Qt::NoButton )
  {
    mItem->movePoint( mItem->absolutePointIdx( mNPressedPoints ), e->pos() );
  }
}

void QgsMilXCreateTool::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    deleteLater(); // quit tool
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsMilxEditBottomBar::QgsMilxEditBottomBar( QgsMilXEditTool *tool )
    : QgsBottomBar( tool->canvas() ), mTool( tool )
{
  QHBoxLayout* layout = new QHBoxLayout( this );
  setLayout( layout );
  mStatusLabel = new QLabel( tr( "<b>%1 symbol(s)</b>" ).arg( mTool->mItems.size() ) );
  layout->addWidget( mStatusLabel );

  mCopyMenu = new QMenu( this );
  mCopyButton = new QPushButton( tr( "Copy to..." ), this );
  mCopyButton->setIcon( QIcon( ":/images/themes/default/mActionEditCopy.png" ) );
  mCopyButton->setMenu( mCopyMenu );
  layout->addWidget( mCopyButton );

  mMoveMenu = new QMenu( this );
  mMoveButton = new QPushButton( tr( "Move to..." ), this );
  mMoveButton->setIcon( QIcon( ":/images/themes/default/mActionEditCut.png" ) );
  mMoveButton->setMenu( mMoveMenu );
  layout->addWidget( mMoveButton );

  QPushButton* closeButton = new QPushButton( this );
  closeButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
  closeButton->setIcon( QIcon( ":/images/themes/default/mIconClose.png" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, SIGNAL( clicked( bool ) ), this, SLOT( onClose() ) );
  layout->addWidget( closeButton );

  connect( mTool, SIGNAL( itemsChanged() ), this, SLOT( updateStatus() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( repopulateLayers() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersRemoved( QStringList ) ), this, SLOT( repopulateLayers() ) );

  repopulateLayers();
  show();
  setFixedWidth( width() );
  updatePosition();
}

void QgsMilxEditBottomBar::onClose()
{
  mTool->deleteLater();
}

void QgsMilxEditBottomBar::repopulateLayers()
{
  mCopyMenu->clear();
  mMoveMenu->clear();
  mCopyMenu->addAction( tr( "New layer..." ), this, SLOT( copyToNewLayer() ) );
  mMoveMenu->addAction( tr( "New layer..." ), this, SLOT( moveToNewLayer() ) );
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers() )
  {
    if ( layer != mTool->mLayer && qobject_cast<QgsMilXLayer*>( layer ) )
    {
      mCopyMenu->addAction( layer->name(), this, SLOT( copyToLayer() ) )->setData( layer->id() );
      mMoveMenu->addAction( layer->name(), this, SLOT( moveToLayer() ) )->setData( layer->id() );
    }
  }
}

QString QgsMilxEditBottomBar::createLayer()
{
  QString layerName = QInputDialog::getText( mTool->canvas(), tr( "Layer Name" ), tr( "Enter name of new MilX layer:" ) );
  if ( !layerName.isEmpty() )
  {
    QgsMilXLayer* layer = new QgsMilXLayer( mTool->mIface->layerTreeView()->menuProvider(), layerName );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    return layer->id();
  }
  return QString();
}

void QgsMilxEditBottomBar::copyMoveSymbols( const QString &targetLayerId, bool move )
{
  QgsMilXLayer* layer = qobject_cast<QgsMilXLayer*>( QgsMapLayerRegistry::instance()->mapLayer( targetLayerId ) );
  if ( !layer )
  {
    return;
  }
  foreach ( QgsMilXAnnotationItem* item, mTool->mItems )
  {
    layer->addItem( item->toMilxItem() );
    if ( move )
    {
      item->deleteLater();
    }
  }
  mTool->mIface->messageBar()->pushMessage( tr( "%1 symbol(s) %2" ).arg( mTool->mItems.size() ).arg( move ? tr( "moved" ) : tr( "copied" ) ), "", QgsMessageBar::INFO, 5 );
  layer->triggerRepaint();
  mTool->mLayer->triggerRepaint();
  if ( move )
  {
    mTool->deleteLater();
  }
}

void QgsMilxEditBottomBar::updateStatus()
{
  mStatusLabel->setText( tr( "<b>%1 symbols(s)</b>" ).arg( mTool->mItems.size() ) );
}

///////////////////////////////////////////////////////////////////////////////

QgsMilXEditTool::QgsMilXEditTool( QgisInterface *iface, QgsMilXLayer* layer, QgsMilXItem* layerItem )
    : QgsMapTool( iface->mapCanvas() ), mIface( iface ), mLayer( layer ), mPanning( false ), mActiveAnnotation( 0 ), mAnnotationMoveAction( QgsAnnotationItem::NoAction )
{
  QgsMilXAnnotationItem* item = new QgsMilXAnnotationItem( iface->mapCanvas() );
  item->fromMilxItem( layerItem );
  mItems.append( item );
  connect( item, SIGNAL( destroyed( QObject* ) ), this, SLOT( removeItemFromList() ) );
  connect( this, SIGNAL( deactivated() ), this, SLOT( deleteLater() ) );
  // If layer is deleted or layers are changed, quit tool
  connect( mLayer, SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
  connect( iface->mapCanvas(), SIGNAL( layersChanged( QStringList ) ), this, SLOT( checkLayerHidden( ) ) );
  connect( iface->mapCanvas(), SIGNAL( renderComplete( QPainter* ) ), this, SLOT( updateRect() ) );

  // Todo: replace with QgsMapCanvasItem derived class to avoid the renderComplete hack
  mRectItem = new QGraphicsRectItem();
  mRectItem->setPen( QPen( Qt::black, 2, Qt::DashLine ) );
  iface->mapCanvas()->scene()->addItem( mRectItem );

  mBottomBar = new QgsMilxEditBottomBar( this );
}

QgsMilXEditTool::~QgsMilXEditTool()
{
  delete mBottomBar;
  delete mRectItem;
  if ( mLayer )
  {
    foreach ( QgsMilXAnnotationItem* item, mItems )
    {
      mLayer->addItem( item->toMilxItem() );
      connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), item, SLOT( deleteLater() ) );
    }
    // Delay delete until after refresh, to avoid flickering
    mLayer->triggerRepaint();
  }
  else
  {
    foreach ( QgsMilXAnnotationItem* item, mItems )
    {
      delete item;
    }
  }
}

void QgsMilXEditTool::activate()
{
  // Done here becase QgsMapToolPan deselects selected annotation items when it is deactived
  mItems.front()->setSelected( true );
}

void QgsMilXEditTool::canvasPressEvent( QMouseEvent* e )
{
  if ( e->button() == Qt::LeftButton && ( e->modifiers() & Qt::ControlModifier ) == 0 )
  {
    mMouseMoveLastXY = e->pos();
    QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( mCanvas->annotationItemAtPos( e->pos() ) );
    if ( item && mItems.contains( item ) )
    {
      mActiveAnnotation = item;
      mAnnotationMoveAction = item->moveActionForPosition( e->posF() );
    }
  }
}

void QgsMilXEditTool::canvasMoveEvent( QMouseEvent * e )
{
  QCursor cursor = mCursor;
  if (( e->buttons() & Qt::LeftButton ) )
  {
    if ( mActiveAnnotation && mAnnotationMoveAction != QgsAnnotationItem::NoAction )
    {
      mActiveAnnotation->handleMoveAction( mAnnotationMoveAction, e->posF(), mMouseMoveLastXY );
      mMouseMoveLastXY = e->pos();
      updateRect();
    }
    else
    {
      mCanvas->panAction( e );
      cursor = QCursor( Qt::ClosedHandCursor );
      mPanning = true;
    }
  }
  else
  {
    QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( mCanvas->annotationItemAtPos( e->pos() ) );
    if ( item )
    {
      int moveAction = item->moveActionForPosition( e -> pos() );
      if ( moveAction != QgsAnnotationItem::NoAction )
        cursor = QCursor( item->cursorShapeForAction( moveAction ) );
    }
  }
  mCanvas->setCursor( cursor );
}

void QgsMilXEditTool::canvasReleaseEvent( QMouseEvent * e )
{
  if ( mPanning )
  {
    mCanvas->panActionEnd( e->pos() );
    mPanning = false;
    mCanvas->setCursor( mCursor );
  }
  else if ( mActiveAnnotation )
  {
    mActiveAnnotation = 0;
    mAnnotationMoveAction = QgsAnnotationItem::NoAction;
  }
  else if ( e->button() == Qt::LeftButton )
  {
    QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( canvas()->annotationItemAtPos( e->pos() ) );
    if ( e->modifiers() == Qt::ControlModifier && item && mItems.contains( item ) )
    {
      // CTRL+Clicked a selected item, deselect it
      mItems.removeAll( item );
      mLayer->addItem( item->toMilxItem() );
      connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), item, SLOT( deleteLater() ) );
      mLayer->triggerRepaint();
      updateRect();
    }
    else
    {
      QVariant pickResult;
      if ( mLayer->testPick( toMapCoordinates( e->pos() ), canvas()->mapSettings(), pickResult ) )
      {
        // Clicked a new item
        QgsMilXItem* layerItem = mLayer->takeItem( pickResult.toInt() );
        QgsMilXAnnotationItem* item = new QgsMilXAnnotationItem( canvas() );
        item->fromMilxItem( layerItem );
        item->setSelected( true );
        connect( item, SIGNAL( destroyed( QObject* ) ), this, SLOT( removeItemFromList() ) );
        delete layerItem;
        if ( e->modifiers() != Qt::ControlModifier )
        {
          // Clicked a new item without CTRL item, add it as sole selected item
          foreach ( QgsMilXAnnotationItem* item, mItems )
          {
            mLayer->addItem( item->toMilxItem() );
            connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), item, SLOT( deleteLater() ) );
          }
          mItems.clear();
        }
        mItems.append( item );
        mLayer->triggerRepaint();
        updateRect();
      }
      else if ( e->modifiers() != Qt::ControlModifier )
      {
        // Didn't hit an item, CTRL was not pressed => quit
        deleteLater(); // quit tool
      }
    }
  }
  else if ( e->button() == Qt::RightButton )
  {
    QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( canvas()->annotationItemAtPos( e->pos() ) );
    if ( item && mItems.contains( item ) )
    {
      item->showContextMenu( canvas()->mapToGlobal( e->pos() ) );
    }
    else if ( !mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
    {
      deleteLater(); // quit tool
    }
  }
}

void QgsMilXEditTool::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    deleteLater(); // quit tool
  }
}

void QgsMilXEditTool::removeItemFromList()
{
  QObject* object = QObject::sender();
  mItems.removeAll( static_cast<QgsMilXAnnotationItem*>( object ) );
  updateRect();
}

void QgsMilXEditTool::updateRect()
{
  emit itemsChanged();
  if ( mItems.isEmpty() )
  {
    deleteLater(); // quit tool if no items remain
    return;
  }
  int n = mItems.size();
  QRectF rect = mItems.front()->boundingRect().translated( mItems.front()->pos() );
  for ( int i = 1; i < n; ++i )
  {
    rect = rect.unite( mItems[i]->boundingRect().translated( mItems[i]->pos() ) );
  }
  mRectItem->setRect( rect );
  mRectItem->setVisible( n > 1 );
}

void QgsMilXEditTool::checkLayerHidden()
{
  if ( !canvas()->layers().contains( mLayer ) )
  {
    deleteLater();
  }
}
