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

#include "qgscrscache.h"
#include "qgsmilxmaptools.h"
#include "qgsmilxannotationitem.h"
#include "qgsmilxlayer.h"
#include "qgisinterface.h"
#include "layertree/qgslayertreeview.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"

#include <QComboBox>
#include <QGraphicsRectItem>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolButton>
#include <QWidgetAction>


QgsMilxCreateBottomBar::QgsMilxCreateBottomBar( QgsMilXCreateTool *tool, QgsMilXLibrary *library )
    : QgsBottomBar( tool->canvas() ), mTool( tool ), mLibrary( library )
{
  QHBoxLayout* layout = new QHBoxLayout();
  setLayout( layout );

  layout->addWidget( new QLabel( tr( "<b>Symbol:</b>" ) ) );
  layout->setSpacing( 2 );

  mSymbolButton = new QToolButton();
  mSymbolButton->setText( tr( "Select..." ) );
  mSymbolButton->setIconSize( QSize( 32, 32 ) );
  mSymbolButton->setCheckable( true );
  mSymbolButton->setFixedHeight( 35 );
  layout->addWidget( mSymbolButton );

  layout->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding ) );

  layout->addWidget( new QLabel( tr( "<b>Layer:</b>" ) ) );

  mLayersCombo = new QComboBox( this );
  mLayersCombo->setFixedWidth( 100 );
  connect( mLayersCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setCurrentLayer( int ) ) );
  layout->addWidget( mLayersCombo );

  QToolButton* newLayerButton = new QToolButton();
  newLayerButton->setIcon( QIcon( ":/images/themes/default/mActionAdd.png" ) );
  connect( newLayerButton, SIGNAL( clicked( bool ) ), this, SLOT( createLayer() ) );
  layout->addWidget( newLayerButton );

  layout->addItem( new QSpacerItem( 1, 1, QSizePolicy::Expanding ) );

  QPushButton* closeButton = new QPushButton( this );
  closeButton->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
  closeButton->setIcon( QIcon( ":/images/themes/default/mIconClose.png" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, SIGNAL( clicked( bool ) ), this, SLOT( onClose() ) );
  layout->addWidget( closeButton );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( repopulateLayers() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersRemoved( QStringList ) ), this, SLOT( repopulateLayers() ) );
  connect( mTool->mIface->mapCanvas(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( setCurrentLayer( QgsMapLayer* ) ) );
  connect( mSymbolButton, SIGNAL( clicked( bool ) ), this, SLOT( toggleLibrary( bool ) ) );
  connect( library, SIGNAL( symbolSelected( QgsMilxSymbolTemplate ) ), this, SLOT( symbolSelected( QgsMilxSymbolTemplate ) ) );
  connect( library, SIGNAL( visibilityChanged( bool ) ), mSymbolButton, SLOT( setChecked( bool ) ) );

  repopulateLayers();

  // Auto-create layer if necessary
  if ( mLayersCombo->count() == 0 )
  {
    QgsMilXLayer* layer = new QgsMilXLayer( mTool->mIface->layerTreeView()->menuProvider() );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    setCurrentLayer( layer );
  }

  show();
  setFixedWidth( width() );
  updatePosition();
  toggleLibrary( true );
}

void QgsMilxCreateBottomBar::repopulateLayers()
{
  // Avoid update while updating
  if ( mLayersCombo->signalsBlocked() )
  {
    return;
  }
  mLayersCombo->blockSignals( true );
  mLayersCombo->clear();
  int idx = 0, current = 0;
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( dynamic_cast<QgsMilXLayer*>( layer ) )
    {
      connect( layer, SIGNAL( layerNameChanged() ), this, SLOT( repopulateLayers() ), Qt::UniqueConnection );
      mLayersCombo->addItem( layer->name(), layer->id() );
      if ( mTool->mIface->mapCanvas()->currentLayer() == layer )
      {
        current = idx;
      }
      ++idx;
    }
  }
  mLayersCombo->setCurrentIndex( -1 );
  mLayersCombo->blockSignals( false );
  mLayersCombo->setCurrentIndex( current );
}

void QgsMilxCreateBottomBar::setCurrentLayer( int idx )
{
  if ( idx >= 0 )
  {
    QgsMilXLayer* layer = qobject_cast<QgsMilXLayer*>( QgsMapLayerRegistry::instance()->mapLayer( mLayersCombo->itemData( idx ).toString() ) );
    if ( layer && mTool->mIface->layerTreeView()->currentLayer() != layer )
    {
      mTool->mIface->layerTreeView()->setCurrentLayer( layer );
    }
    mTool->setTargetLayer( layer );
  }
  else
  {
    mTool->setTargetLayer( 0 );
  }
}

void QgsMilxCreateBottomBar::setCurrentLayer( QgsMapLayer *layer )
{
  int idx = layer ? mLayersCombo->findData( layer->id() ) : -1;
  if ( idx >= 0 )
  {
    mLayersCombo->setCurrentIndex( idx );
  }
}

void QgsMilxCreateBottomBar::onClose()
{
  mTool->deleteLater();
}

void QgsMilxCreateBottomBar::createLayer()
{
  QString layerName = QInputDialog::getText( this, tr( "Layer Name" ), tr( "Enter name of new MilX layer:" ) );
  if ( !layerName.isEmpty() )
  {
    QgsMilXLayer* layer = new QgsMilXLayer( mTool->mIface->layerTreeView()->menuProvider(), layerName );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    setCurrentLayer( layer );
  }
}

void QgsMilxCreateBottomBar::symbolSelected( const QgsMilxSymbolTemplate &symbolTemplate )
{
  mSymbolButton->setIcon( QIcon( symbolTemplate.pixmap ) );
  mSymbolButton->setText( "" );
  mTool->setSymbolTemplate( symbolTemplate );
}

void QgsMilxCreateBottomBar::toggleLibrary( bool visible )
{
  if ( visible )
  {
    mLibrary->resize( width(), 320 );
    mLibrary->move( QPoint( mapToGlobal( QPoint( 0, 0 ) ).x(), mSymbolButton->mapToGlobal( QPoint( 0, -320 ) ).y() ) );
    mLibrary->show();
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsMilXCreateTool::QgsMilXCreateTool( QgisInterface *iface, QgsMilXLibrary* library )
    : QgsMapTool( iface->mapCanvas() ), mIface( iface ), mItem( 0 ), mLayer( 0 )
{
  mBottomBar = new QgsMilxCreateBottomBar( this, library );
  connect( this, SIGNAL( deactivated() ), this, SLOT( deleteLater() ) );
}

QgsMilXCreateTool::~QgsMilXCreateTool()
{
  delete mBottomBar;
  reset();
}

void QgsMilXCreateTool::canvasPressEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::RightButton && !mItem )
  {
    deleteLater(); // quit tool
    return;
  }
  if ( mSymbolTemplate.symbolXml.isEmpty() )
  {
    return;
  }
  if ( !mLayer )
  {
    mIface->messageBar()->pushMessage( tr( "No MilX Layer Selected" ), "", QgsMessageBar::WARNING, 5 );
    return;
  }
  if ( mLayer->isApproved() )
  {
    mIface->messageBar()->pushMessage( tr( "Non-editable MilX Layer Selected" ), tr( "Approved layers cannot be edited." ), QgsMessageBar::WARNING, 5 );
    return;
  }
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
      mItem->setSymbolXml( mSymbolTemplate.symbolXml, mSymbolTemplate.symbolMilitaryName );
      mItem->setSelected( true );
      mNPressedPoints = 1;
      // Only actually add the point if more than the minimum number have been specified
      // The server automatically adds points up to the minimum number
      if ( mNPressedPoints >= mSymbolTemplate.minNPoints && mSymbolTemplate.hasVariablePoints )
      {
        mItem->appendPoint( e->pos() );
      }
    }
    else if ( mNPressedPoints < mSymbolTemplate.minNPoints || mSymbolTemplate.hasVariablePoints )
    {
      ++mNPressedPoints;
      // Only actually add the point if more than the minimum number have been specified
      // The server automatically adds points up to the minimum number
      if ( mNPressedPoints >= mSymbolTemplate.minNPoints && mSymbolTemplate.hasVariablePoints )
      {
        mItem->appendPoint( e->pos() );
      }
    }

    if ( mNPressedPoints >= mSymbolTemplate.minNPoints && !mSymbolTemplate.hasVariablePoints )
    {
      // Max points reached, stop
      mItem->finalize();
      mLayer->addItem( mItem->toMilxItem() );
      mItem->setSelected( false );
      mNPressedPoints = 0;
      // Delay delete until after refresh, to avoid flickering
      connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), mItem, SLOT( deleteLater() ) );
      mItem = 0;
      mNPressedPoints = 0;
      mLayer->triggerRepaint();
    }
  }
  else if ( e->button() == Qt::RightButton && mItem )
  {
    if ( mNPressedPoints + 1 >= mSymbolTemplate.minNPoints )
    {
      // Done with N point symbol, stop
      mItem->finalize();
      mLayer->addItem( mItem->toMilxItem() );
      mItem->setSelected( false );
      // Delay delete until after refresh, to avoid flickering
      connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), mItem, SLOT( deleteLater() ) );
      mItem = 0;
      mNPressedPoints = 0;
      mLayer->triggerRepaint();
    }
    else if ( mNPressedPoints + 1 < mSymbolTemplate.minNPoints )
    {
      reset();
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
    if ( mItem )
    {
      reset();
    }
    else
    {
      deleteLater(); // quit tool
    }
  }
}

void QgsMilXCreateTool::setTargetLayer( QgsMilXLayer *layer )
{
  if ( mLayer )
  {
    disconnect( layer, SIGNAL( approvedChanged( bool ) ), this, SLOT( reset() ) );
  }
  mLayer = layer;
  if ( mLayer )
  {
    connect( layer, SIGNAL( approvedChanged( bool ) ), this, SLOT( reset() ) );
    mIface->layerTreeView()->setLayerVisible( layer, true );
  }
  // Reset any drawing operation which might be in progress
  reset();
}

void QgsMilXCreateTool::setSymbolTemplate( const QgsMilxSymbolTemplate& symbolTemplate )
{
  mSymbolTemplate = symbolTemplate;
  // Reset any drawing operation which might be in progress
  reset();
  setCursor( QCursor( symbolTemplate.pixmap, -0.5 * symbolTemplate.pixmap.width(), -0.5 * symbolTemplate.pixmap.height() ) );
}

void QgsMilXCreateTool::reset()
{
  mNPressedPoints = 0;
  delete mItem;
  mItem = 0;
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
  closeButton->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Maximum );
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
      connect( layer, SIGNAL( layerNameChanged() ), this, SLOT( repopulateLayers() ), Qt::UniqueConnection );
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
  mStatusLabel->setText( tr( "<b>%1 symbol(s)</b>" ).arg( mTool->mItems.size() ) );
}

///////////////////////////////////////////////////////////////////////////////

QgsMilXEditTool::QgsMilXEditTool( QgisInterface *iface, QgsMilXLayer* layer, QgsMilXItem* layerItem )
    : QgsMapTool( iface->mapCanvas() ), mIface( iface ), mLayer( layer ), mPanning( false ), mDraggingRect( false ), mActiveAnnotation( 0 ), mAnnotationMoveAction( QgsAnnotationItem::NoAction )
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
  qDeleteAll( mClipboard );
  delete mBottomBar;
  delete mRectItem;
  if ( mLayer )
  {
    foreach ( QgsMilXAnnotationItem* item, mItems )
    {
      mLayer->addItem( item->toMilxItem() );
      item->setSelected( false );
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
    mMouseMoveLastXY = e->posF();
    QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( mCanvas->annotationItemAtPos( e->pos() ) );
    if ( item && mItems.contains( item ) )
    {
      mActiveAnnotation = item;
      mAnnotationMoveAction = item->moveActionForPosition( e->posF() );
    }
    else if ( e->modifiers() != Qt::ControlModifier && mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
    {
      mDraggingRect = true;
      mCanvas->setCursor( Qt::SizeAllCursor );
    }
  }
}

void QgsMilXEditTool::canvasMoveEvent( QMouseEvent * e )
{
  QCursor cursor = mCursor;
  if (( e->buttons() & Qt::LeftButton ) )
  {
    if ( mDraggingRect )
    {
      QPointF newPos = e->posF();
      foreach ( QgsMilXAnnotationItem* item, mItems )
      {
        item->handleMoveAction( QgsAnnotationItem::MoveMapPosition, newPos, mMouseMoveLastXY );
      }
      mRectItem->moveBy( newPos.x() - mMouseMoveLastXY.x(), newPos.y() - mMouseMoveLastXY.y() );
    }
    else if ( mActiveAnnotation && mAnnotationMoveAction != QgsAnnotationItem::NoAction )
    {
      mActiveAnnotation->handleMoveAction( mAnnotationMoveAction, e->posF(), mMouseMoveLastXY );
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
  mMouseMoveLastXY = e->posF();
  mCanvas->setCursor( cursor );
}

void QgsMilXEditTool::canvasReleaseEvent( QMouseEvent * e )
{
  if ( mDraggingRect )
  {
    mDraggingRect = false;
    mCanvas->setCursor( mCursor );
    updateRect();
  }
  else if ( mPanning )
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
      item->setSelected( false );
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
            item->setSelected( false );
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
        QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( mCanvas->annotationItemAtPos( e->pos() ) );
        if ( !item || !mItems.contains( item ) )
        {
          // Didn't hit an item, CTRL was not pressed => quit
          deleteLater(); // quit tool
        }
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

void QgsMilXEditTool::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier )
  {
    qDeleteAll( mClipboard );
    mClipboard.clear();
    mClipboardItemOffsets.clear();
    double cx = 0, cy = 0;
    foreach ( QgsMilXAnnotationItem* item, mItems )
    {
      mClipboard.append( item->toMilxItem() );
      mClipboardItemOffsets.append( mClipboard.back()->points()[0] );
      cx += mClipboardItemOffsets.back().x();
      cy += mClipboardItemOffsets.back().y();
    }
    int n = mClipboardItemOffsets.size();
    cx /= n; cy /= n;
    for ( int i = 0; i < n; ++i )
    {
      mClipboardItemOffsets[i] -= QgsVector( cx, cy );
    }
  }
  else if ( e->key() == Qt::Key_V && e->modifiers() == Qt::ControlModifier )
  {
    if ( !mClipboard.isEmpty() )
    {
      // Clear current selection
      foreach ( QgsMilXAnnotationItem* item, mItems )
      {
        mLayer->addItem( item->toMilxItem() );
        item->setSelected( false );
        connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), item, SLOT( deleteLater() ) );
      }
      mItems.clear();
      // Add new items
      const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mCanvas->mapSettings().destinationCrs().authid(), "EPSG:4326" );
      QgsPoint pastePosWgs = crst->transform( toMapCoordinates( mMouseMoveLastXY.toPoint() ) );
      for ( int i = 0, n = mClipboard.size(); i < n; ++i )
      {
        QgsMilXAnnotationItem* item = new QgsMilXAnnotationItem( canvas() );
        item->fromMilxItem( mClipboard[i] );
        item->setSelected( true );
        item->setMapPosition( crst->transform( pastePosWgs + mClipboardItemOffsets[i], QgsCoordinateTransform::ReverseTransform ) );
        connect( item, SIGNAL( destroyed( QObject* ) ), this, SLOT( removeItemFromList() ) );
        mItems.append( item );
      }
      updateRect();
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

void QgsMilXEditTool::canvasDoubleClickEvent( QMouseEvent *e )
{
  QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( mCanvas->annotationItemAtPos( e->pos() ) );
  if ( item && mItems.contains( item ) )
  {
    item->showItemEditor();
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
  mRectItem->setPos( 0, 0 );
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
