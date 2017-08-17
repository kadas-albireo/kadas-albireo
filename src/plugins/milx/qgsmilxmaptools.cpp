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

#include "qgsclipboard.h"
#include "qgscrscache.h"
#include "qgsfloatinginputwidget.h"
#include "qgsmilxmaptools.h"
#include "qgsmilxannotationitem.h"
#include "qgsmilxio.h"
#include "qgsmilxlayer.h"
#include "qgisinterface.h"
#include "layertree/qgslayertreeview.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"

#include <QApplication>
#include <QComboBox>
#include <QDesktopWidget>
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
    mLibrary->focusFilter();
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsMilXCreateTool::QgsMilXCreateTool( QgisInterface *iface, QgsMilXLibrary* library )
    : QgsMapTool( iface->mapCanvas() ), mIface( iface ), mItem( 0 ), mLayer( 0 ), mInputWidget( 0 )
{
  mBottomBar = new QgsMilxCreateBottomBar( this, library );
  connect( this, SIGNAL( deactivated() ), this, SLOT( deleteLater() ) );

  if ( QSettings().value( "/qgis/showNumericInput", false ).toBool() )
  {
    mInputWidget = new QgsFloatingInputWidget( canvas() );

    QgsFloatingInputWidgetField* xinput = new QgsFloatingInputWidgetField();
    xinput->setProperty( "coordinate", 0 );
    connect( xinput, SIGNAL( inputChanged( ) ), this, SLOT( updatePoint( ) ) );
    connect( xinput, SIGNAL( inputConfirmed() ), this, SLOT( confirmPoint() ) );
    mInputWidget->addInputField( "x:", xinput, true );

    QgsFloatingInputWidgetField* yinput = new QgsFloatingInputWidgetField();
    yinput->setProperty( "coordinate", 1 );
    connect( yinput, SIGNAL( inputChanged( ) ), this, SLOT( updatePoint( ) ) );
    connect( yinput, SIGNAL( inputConfirmed() ), this, SLOT( confirmPoint() ) );
    mInputWidget->addInputField( "y:", yinput );

    mInputWidget->show();
  }
}

QgsMilXCreateTool::~QgsMilXCreateTool()
{
  delete mInputWidget;
  mInputWidget = 0;
  delete mBottomBar;
  mBottomBar = 0;
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
      initializeItem( toMapCoordinates( e->pos() ) );
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
      finalizeItem(); // Max points reached, stop
    }
  }
  else if ( e->button() == Qt::RightButton && mItem )
  {
    if ( mNPressedPoints + 1 >= mSymbolTemplate.minNPoints )
    {
      finalizeItem(); // Done with N point symbol, stop
    }
    else if ( mNPressedPoints + 1 < mSymbolTemplate.minNPoints )
    {
      reset(); // Premature stop, reset
    }
  }
}

void QgsMilXCreateTool::initializeItem( const QgsPoint& position )
{
  // Deselect any previously selected items
  QgsAnnotationItem* selectedItem = mCanvas->selectedAnnotationItem();
  if ( selectedItem )
  {
    selectedItem->setSelected( false );
  }

  setCursor( Qt::CrossCursor );
  mItem = new QgsMilXAnnotationItem( mCanvas );
  mItem->setMapPosition( position );
  mItem->setSymbolXml( mSymbolTemplate.symbolXml, mSymbolTemplate.symbolMilitaryName );
  // Ensure attributes are populated
  mItem->updateSymbol( true );
  mItem->setSelected( true );
  mNPressedPoints = 1;
  // Only actually add the point if more than the minimum number have been specified
  // The server automatically adds points up to the minimum number
  if ( mNPressedPoints >= mSymbolTemplate.minNPoints && mSymbolTemplate.hasVariablePoints )
  {
    mItem->appendPoint( toCanvasCoordinates( position ) );
  }
  if ( mInputWidget && !mItem->attributes().isEmpty() )
  {
    for ( int i = 0, n = mItem->attributes().size(); i < n; ++i )
    {
      const QPair<int, double>& attr = mItem->attributes()[i];
      QDoubleValidator* validator = new QDoubleValidator();
      if ( attr.first != MilXClient::AttributeAttitude )
      {
        validator->setBottom( 0 );
      }
      QgsFloatingInputWidgetField* input = new QgsFloatingInputWidgetField( validator );
      input->setText( QString::number( attr.second, 'f', attr.first == MilXClient::AttributeAttitude ? 1 : 0 ) );
      input->setProperty( "attridx", i );
      connect( input, SIGNAL( inputChanged( ) ), this, SLOT( updateAttribute( ) ) );
      mInputWidget->addInputField( MilXClient::attributeName( attr.first ) + ":", input );
    }
  }
}

void QgsMilXCreateTool::finalizeItem()
{

  mItem->finalize();
  mLayer->addItem( mItem->toMilxItem() );
  mItem->setSelected( false );
  // Delay delete until after refresh, to avoid flickering
  connect( mCanvas, SIGNAL( mapCanvasRefreshed() ), mItem, SLOT( deleteLater() ) );
  mItem = 0;
  reset();
  mLayer->triggerRepaint();
}

void QgsMilXCreateTool::canvasMoveEvent( QMouseEvent * e )
{
  if ( mItem != 0 && e->buttons() == Qt::NoButton )
  {
    mItem->movePoint( mItem->absolutePointIdx( mNPressedPoints ), e->pos() );
    if ( mInputWidget )
    {
      int pointidx = mItem->absolutePointIdx( mNPressedPoints );
      QgsPoint p = mItem->point( pointidx );
      bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
      mInputWidget->inputFields()[0]->setText( QString::number( p.x(), 'f', isDegrees ? 4 : 0 ) );
      mInputWidget->inputFields()[1]->setText( QString::number( p.y(), 'f', isDegrees ? 4 : 0 ) );
      mInputWidget->move( e->x() + 5, e->y() - 5 - mInputWidget->height() );
    }
  }
  else if ( mInputWidget )
  {
    QgsPoint p = toMapCoordinates( e->pos() );
    bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
    mInputWidget->inputFields()[0]->setText( QString::number( p.x(), 'f', isDegrees ? 4 : 0 ) );
    mInputWidget->inputFields()[1]->setText( QString::number( p.y(), 'f', isDegrees ? 4 : 0 ) );
    mInputWidget->move( e->x() + 5, e->y() - 5 - mInputWidget->height() );
  }
}

void QgsMilXCreateTool::keyPressEvent( QKeyEvent *e )
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
}

void QgsMilXCreateTool::reset()
{
  if ( !mSymbolTemplate.symbolXml.isEmpty() )
  {
    setCursor( QCursor( mSymbolTemplate.pixmap, -0.5 * mSymbolTemplate.pixmap.width(), -0.5 * mSymbolTemplate.pixmap.height() ) );
  }
  mNPressedPoints = 0;
  delete mItem;
  mItem = 0;
  if ( mInputWidget )
  {
    int n = mInputWidget->inputFields().size();
    while ( n > 2 )
    {
      mInputWidget->removeInputField( --n );
    }
  }
}

void QgsMilXCreateTool::updateAttribute( )
{
  QgsFloatingInputWidgetField* field = qobject_cast<QgsFloatingInputWidgetField*>( QObject::sender() );
  if ( !field || !mItem )
  {
    return;
  }
  int attridx = field->property( "attridx" ).toInt();
  mItem->setAttribute( attridx, field->text().toDouble() );
}

void QgsMilXCreateTool::updatePoint( )
{
  if ( mItem )
  {
    QgsFloatingInputWidgetField* field = qobject_cast<QgsFloatingInputWidgetField*>( QObject::sender() );
    if ( !field )
    {
      return;
    }
    int pointidx = mItem->absolutePointIdx( mNPressedPoints );
    int coordinate = field->property( "coordinate" ).toInt();
    QgsPoint p = mItem->point( pointidx );
    coordinate == 0 ? p.setX( field->text().toDouble() ) : p.setY( field->text().toDouble() );
    mItem->movePoint( pointidx, toCanvasCoordinates( p ) );
    mInputWidget->adjustCursorAndExtent( mItem->point( pointidx ) );
  }
  else
  {
    QgsPoint point;
    point.setX( mInputWidget->inputFields()[0]->text().toDouble() );
    point.setY( mInputWidget->inputFields()[1]->text().toDouble() );
    mInputWidget->adjustCursorAndExtent( point );
  }
}

void QgsMilXCreateTool::confirmPoint()
{
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
  QgsPoint point;
  point.setX( mInputWidget->inputFields()[0]->text().toDouble() );
  point.setY( mInputWidget->inputFields()[1]->text().toDouble() );
  if ( mItem == 0 )
  {
    initializeItem( point );
  }
  else if ( mNPressedPoints < mSymbolTemplate.minNPoints || mSymbolTemplate.hasVariablePoints )
  {
    int prevPointIdx = mItem->absolutePointIdx( mNPressedPoints ) - 1;
    QgsPoint prevPoint = mItem->point( prevPointIdx );
    if ( toCanvasCoordinates( prevPoint ) == toCanvasCoordinates( point ) )
    {
      if ( mNPressedPoints >= mSymbolTemplate.minNPoints )
      {
        // Finalize item if previous point is confirmed
        finalizeItem();
      }
    }
    else
    {
      ++mNPressedPoints;
      // Only actually add the point if more than the minimum number have been specified
      // The server automatically adds points up to the minimum number
      if ( mNPressedPoints >= mSymbolTemplate.minNPoints && mSymbolTemplate.hasVariablePoints )
      {
        mItem->appendPoint( toCanvasCoordinates( point ) );
      }
    }
  }

  if ( mNPressedPoints >= mSymbolTemplate.minNPoints && !mSymbolTemplate.hasVariablePoints )
  {
    finalizeItem(); // Max points reached, stop
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
  updateStatus();
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
    if ( qobject_cast<QgsMilXLayer*>( layer ) )
    {
      connect( layer, SIGNAL( layerNameChanged() ), this, SLOT( repopulateLayers() ), Qt::UniqueConnection );
      mCopyMenu->addAction( layer->name(), this, SLOT( copyToLayer() ) )->setData( layer->id() );
      if ( layer != mTool->mLayer )
      {
        mMoveMenu->addAction( layer->name(), this, SLOT( moveToLayer() ) )->setData( layer->id() );
      }
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
  if ( !move )
  {
    foreach ( QgsMilXAnnotationItem* item, mTool->mItems )
    {
      mTool->mLayer->addItem( item->toMilxItem() );
    }
  }
  mTool->mLayer->triggerRepaint();
  if ( layer != mTool->mLayer.data() )
  {
    // Keep selection but switch layer, effectively moving/copying symbols to that layer once the selection is dismissed and the items added to the layer
    mTool->setLayer( layer );
    mTool->mIface->messageBar()->pushMessage( tr( "%1 symbol(s) %2" ).arg( mTool->mItems.size() ).arg( move ? tr( "moved" ) : tr( "copied" ) ), "", QgsMessageBar::INFO, 5 );
  }
  else
  {
    // Offset symbols when pasting to same layer
    mTool->mIface->mapCanvas()->freeze( true );
    foreach ( QgsMilXAnnotationItem* item, mTool->mItems )
    {
      QPoint canvasPos = mTool->toCanvasCoordinates( item->mapPosition() ) + QPoint( 25, 25 );
      item->setMapPosition( mTool->toMapCoordinates( canvasPos ) );
    }
    mTool->mIface->mapCanvas()->freeze( false );
  }
  updateStatus();
}

void QgsMilxEditBottomBar::updateStatus()
{
  mStatusLabel->setText( tr( "<b>%1 symbol(s) of layer %2</b>" ).arg( mTool->mItems.size() ).arg( mTool->mLayer->name() ) );
  setFixedWidth( sizeHint().width() );
  updatePosition();
}

///////////////////////////////////////////////////////////////////////////////

QgsMilXEditTool::QgsMilXEditTool( QgisInterface *iface, QgsMilXLayer* layer, QgsMilXItem* layerItem )
    : QgsMapTool( iface->mapCanvas() ), mIface( iface ), mLayer( layer ), mDraggingRect( false ), mActiveAnnotation( 0 ), mAnnotationMoveAction( QgsAnnotationItem::NoAction ), mInputWidget( 0 ), mMoving( false )
{
  QgsMilXAnnotationItem* item = new QgsMilXAnnotationItem( iface->mapCanvas() );
  item->fromMilxItem( layerItem );
  mItems.append( item );
  mActiveAnnotation = item;
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
  delete mInputWidget;
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
      item->deleteLater();
    }
  }
}

void QgsMilXEditTool::setLayer( QgsMilXLayer* layer )
{
  disconnect( mLayer, SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
  mLayer = layer;
  connect( mLayer, SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
}

void QgsMilXEditTool::activate()
{
  // Done here becase QgsMapToolPan deselects selected annotation items when it is deactived
  mItems.front()->setSelected( true );
}

void QgsMilXEditTool::canvasPressEvent( QMouseEvent* e )
{
  mMouseMoveLastXY = e->posF();
  if ( e->button() == Qt::LeftButton && ( e->modifiers() & Qt::ControlModifier ) == 0 &&
       !mActiveAnnotation && mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
  {
    mDraggingRect = true;
    mCanvas->setCursor( Qt::SizeAllCursor );
  }
  else if ( e->button() == Qt::RightButton )
  {
    QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( canvas()->annotationItemAtPos( e->pos() ) );
    if ( item && item == mActiveAnnotation )
    {
      item->showContextMenu( canvas()->mapToGlobal( e->pos() ) );
    }
    else if ( mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
    {
      QMenu menu;
      menu.addAction( QIcon( ":/images/themes/default/mActionEditCopy.png" ), tr( "Copy" ), this, SLOT( copy() ) );
      menu.addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteAll() ) );
      menu.exec( e->globalPos() );
    }
  }
}

void QgsMilXEditTool::canvasMoveEvent( QMouseEvent * e )
{
  // Deleting mInputWidget can trigger an extra move event if the mouse was previously
  // over the input widget, can cause mInputWidget to be deleted again before it is reset.
  if ( mMoving )
  {
    return;
  }
  mMoving = true;
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
      if ( mInputWidget )
      {
        bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
        foreach ( QgsFloatingInputWidgetField* field, mInputWidget->inputFields() )
        {
          if ( field->property( "attridx" ).isValid() )
          {
            int attridx = field->property( "attridx" ).toInt();
            const QPair<int, double>& attr = mActiveAnnotation->attributes()[attridx];
            field->setText( QString::number( attr.second, 'f', attr.first == MilXClient::AttributeAttitude ? 1 : 0 ) );
          }
          else if ( field->property( "pointidx" ).isValid() )
          {
            int pointidx = field->property( "pointidx" ).toInt();
            int coordinate = field->property( "coordinate" ).toInt();
            const QgsPoint& p = mActiveAnnotation->point( pointidx );
            field->setText( QString::number( coordinate == 0 ? p.x() : p.y(), 'f', isDegrees ? 4 : 0 ) );
          }
        }
        mInputWidget->move( e->x() + 5, e->y() - 5 - mInputWidget->height() );
      }
      updateRect();
    }
  }
  else
  {
    QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( mCanvas->annotationItemAtPos( e->pos() ) );
    if ( mItems.size() == 1 && item )
    {
      int moveAction = item->moveActionForPosition( e->posF() );
      if ( item != mActiveAnnotation || moveAction != mAnnotationMoveAction )
      {
        delete mInputWidget;
        mInputWidget = 0;
        mActiveAnnotation = item;
        mAnnotationMoveAction = item->moveActionForPosition( e->posF() );

        if ( mAnnotationMoveAction != QgsAnnotationItem::NoAction )
          cursor = QCursor( item->cursorShapeForAction( mAnnotationMoveAction ) );
        if ( QSettings().value( "/qgis/showNumericInput", false ).toBool() )
        {
          int pointidx = item->pointIndexForMoveAction( mAnnotationMoveAction );
          int attridx = item->attributeIndexForMoveAction( mAnnotationMoveAction );
          if ( pointidx >= 0 || attridx >= 0 )
          {
            mInputWidget = new QgsFloatingInputWidget( canvas() );
          }
          if ( pointidx >= 0 )
          {
            bool isDegrees = canvas()->mapSettings().destinationCrs().mapUnits() == QGis::Degrees;
            const QgsPoint& point = item->point( pointidx );
            QgsFloatingInputWidgetField* xinput = new QgsFloatingInputWidgetField();
            xinput->setText( QString::number( point.x(), 'f', isDegrees ? 4 : 0 ) );
            xinput->setProperty( "pointidx", pointidx );
            xinput->setProperty( "coordinate", 0 );
            connect( xinput, SIGNAL( inputChanged( ) ), this, SLOT( updatePoint( ) ) );
            mInputWidget->addInputField( "x:", xinput, true );
            QgsFloatingInputWidgetField* yinput = new QgsFloatingInputWidgetField();
            yinput->setText( QString::number( point.y(), 'f', isDegrees ? 4 : 0 ) );
            yinput->setProperty( "pointidx", pointidx );
            yinput->setProperty( "coordinate", 1 );
            connect( yinput, SIGNAL( inputChanged( ) ), this, SLOT( updatePoint( ) ) );
            mInputWidget->addInputField( "y:", yinput );
          }
          else if ( attridx >= 0 )
          {
            const QPair<int, double>& attr = item->attributes()[attridx];
            QDoubleValidator* validator = new QDoubleValidator();
            if ( attr.first != MilXClient::AttributeAttitude )
            {
              validator->setBottom( 0 );
            }
            QgsFloatingInputWidgetField* input = new QgsFloatingInputWidgetField( validator );
            input->setText( QString::number( attr.second, 'f', attr.first == MilXClient::AttributeAttitude ? 1 : 0 ) );
            input->setProperty( "attridx", attridx );
            connect( input, SIGNAL( inputChanged( ) ), this, SLOT( updateAttribute( ) ) );
            mInputWidget->addInputField( MilXClient::attributeName( attr.first ) + ":", input, true );
          }
          if ( mInputWidget )
          {
            mInputWidget->show();
            mInputWidget->move( e->x() + 5, e->y() - 5 - mInputWidget->height() );
          }
        }
      }
    }
    else
    {
      mActiveAnnotation = 0;
      mAnnotationMoveAction = QgsAnnotationItem::NoAction;
      delete mInputWidget;
      mInputWidget = 0;
    }
  }
  mMouseMoveLastXY = e->posF();
  mCanvas->setCursor( cursor );
  mMoving = false;
}

void QgsMilXEditTool::canvasReleaseEvent( QMouseEvent * e )
{
  if ( mDraggingRect )
  {
    mDraggingRect = false;
    mCanvas->setCursor( mCursor );
    updateRect();
  }
  else if ( e->button() == Qt::LeftButton )
  {
    QgsMilXAnnotationItem* item = qobject_cast<QgsMilXAnnotationItem*>( canvas()->annotationItemAtPos( e->pos() ) );
    if ( e->modifiers() == Qt::ControlModifier && item && mItems.contains( item ) )
    {
      mActiveAnnotation = 0;
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
      QRect boundingBox;
      if ( mLayer->testPick( toMapCoordinates( e->pos() ), canvas()->mapSettings(), pickResult, boundingBox ) )
      {
        // Clicked a new item
        QgsMilXItem* layerItem = mLayer->takeItem( pickResult.toInt() );
        QgsMilXAnnotationItem* item = new QgsMilXAnnotationItem( canvas() );
        item->fromMilxItem( layerItem );
        item->setSelected( true );
        connect( item, SIGNAL( destroyed( QObject* ) ), this, SLOT( removeItemFromList() ) );
        delete layerItem;
        if (( e->modifiers() & Qt::ControlModifier ) == 0 )
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
        if ( mItems.size() == 1 )
        {
          mActiveAnnotation = item;
        }
        mLayer->triggerRepaint();
        updateRect();
      }
      else if ( !mActiveAnnotation && ( e->modifiers() & Qt::ControlModifier ) == 0 )
      {
        // Empty-clicked without control, quit tool
        deleteLater();
      }
    }
  }
  else if ( e->button() == Qt::RightButton )
  {
    if ( !mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
    {
      deleteLater(); // quit tool
    }
  }
}

void QgsMilXEditTool::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    deleteLater(); // quit tool
  }
  else if ( e->key() == Qt::Key_Delete )
  {
    deleteAll();
  }
  else if ( e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier )
  {
    copy();
  }
  else if ( e->key() == Qt::Key_V && e->modifiers() == Qt::ControlModifier )
  {
    if ( mIface->clipboard()->hasFormat( QGSCLIPBOARD_MILXITEMS_MIME ) )
    {
      QByteArray data = mIface->clipboard()->mimeData()->data( QGSCLIPBOARD_MILXITEMS_MIME );
      QgsPoint mapPos = toMapCoordinates( mMouseMoveLastXY.toPoint() );
      paste( data, &mapPos );
    }
  }
}

void QgsMilXEditTool::canvasDoubleClickEvent( QMouseEvent */*e*/ )
{
  if ( mActiveAnnotation )
  {
    mActiveAnnotation->showItemEditor();
  }
}

void QgsMilXEditTool::deleteAll()
{
  foreach ( QgsMilXAnnotationItem* item, mItems )
  {
    item->deleteLater();
  }
  mItems.clear();
  deleteLater();
}

void QgsMilXEditTool::copy()
{
  QList<QgsMilXItem*> milxItems;
  foreach ( QgsMilXAnnotationItem* item, mItems )
  {
    milxItems.append( item->toMilxItem() );
  }
  QgsMilXIO::copyToClipboard( milxItems, mIface );
  qDeleteAll( milxItems );
}

void QgsMilXEditTool::paste( const QByteArray &data, const QgsPoint* mapPos )
{
  mItems.clear();

  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( mCanvas->mapSettings().destinationCrs().authid(), "EPSG:4326" );
  QgsPoint pastePosWgs = mapPos ? crst->transform( *mapPos ) : QgsPoint();

  QDomDocument doc;
  doc.setContent( data );

  QDomElement symbolGroup = doc.firstChildElement( "SymbolGroup" );

  int dpi = QApplication::desktop()->logicalDpiX();
  float symbolSize = symbolGroup.firstChildElement( "SymbolSize" ).text().toFloat(); // This is in mm
  symbolSize = ( symbolSize * dpi ) / 25.4; // mm to px

  QDomElement itemsEl = symbolGroup.firstChildElement( "GraphicList" );
  QDomNodeList itemsList = itemsEl.elementsByTagName( "MilXGraphic" );
  for ( int i = 0, n = itemsList.size(); i < n; ++i )
  {
    QDomElement itemEl = itemsList.at( i ).toElement();
    QgsVector offset( itemEl.attribute( "offsetX" ).toDouble(), itemEl.attribute( "offsetY" ).toDouble() );
    QgsMilXItem milxItem;
    milxItem.readMilx( itemEl, 0, symbolSize );
    QgsMilXAnnotationItem* item = new QgsMilXAnnotationItem( canvas() );
    item->fromMilxItem( &milxItem );
    item->setSelected( true );
    if ( mapPos )
    {
      item->setMapPosition( crst->transform( pastePosWgs + offset, QgsCoordinateTransform::ReverseTransform ) );
    }
    connect( item, SIGNAL( destroyed( QObject* ) ), this, SLOT( removeItemFromList() ) );
    mItems.append( item );
  }
  updateRect();
}

void QgsMilXEditTool::removeItemFromList()
{
  QObject* object = QObject::sender();
  QgsMilXAnnotationItem* item = static_cast<QgsMilXAnnotationItem*>( object );
  mItems.removeAll( item );
  if ( mActiveAnnotation == item )
  {
    mActiveAnnotation = 0;
    mAnnotationMoveAction = QgsAnnotationItem::NoAction;
  }
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

void QgsMilXEditTool::updatePoint( )
{
  QgsFloatingInputWidgetField* field = qobject_cast<QgsFloatingInputWidgetField*>( QObject::sender() );
  if ( !field  || !mActiveAnnotation )
  {
    return;
  }
  int pointidx = field->property( "pointidx" ).toInt();
  int coordinate = field->property( "coordinate" ).toInt();
  QgsPoint p = mActiveAnnotation->point( pointidx );
  coordinate == 0 ? p.setX( field->text().toDouble() ) : p.setY( field->text().toDouble() );
  mActiveAnnotation->movePoint( pointidx, toCanvasCoordinates( p ) );
  mInputWidget->adjustCursorAndExtent( mActiveAnnotation->point( pointidx ) );
  updateRect();
}

void QgsMilXEditTool::updateAttribute( )
{
  QgsFloatingInputWidgetField* field = qobject_cast<QgsFloatingInputWidgetField*>( QObject::sender() );
  if ( !field  || !mActiveAnnotation )
  {
    return;
  }
  int attridx = field->property( "attridx" ).toInt();
  mActiveAnnotation->setAttribute( attridx, field->text().toDouble() );
  mInputWidget->adjustCursorAndExtent( mActiveAnnotation->attributePoints()[attridx].second );
  updateRect();
}
