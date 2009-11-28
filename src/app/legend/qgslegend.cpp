/***************************************************************************
                          qgslegend.cpp  -  description
                             -------------------
    begin                : Sun Jul 28 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc dot com
               Romans 3:23=>Romans 6:23=>Romans 10:9,10=>Romans 12
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

#include "qgsapplication.h"
#include "qgisapp.h"
#include "qgslogger.h"
#include "qgslegend.h"
#include "qgslegendgroup.h"
#include "qgslegendlayer.h"
#include "qgslegendpropertygroup.h"
#include "qgslegendsymbologyitem.h"
#include "qgsmapcanvas.h"
#include "qgsmapcanvasmap.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaprenderer.h"
#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "qgsrasterlayerproperties.h"
#include "qgsvectorlayerproperties.h"

#include "qgsattributetabledialog.h"

#include <cfloat>

#include <QFont>
#include <QDomDocument>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPixmap>
#include <QTreeWidgetItem>

static const char *const ident_ = "$Id$";

const int AUTOSCROLL_MARGIN = 16;

/**
   @note

   set mItemBeingMoved pointer to 0 to prevent SuSE 9.0 crash
*/
QgsLegend::QgsLegend( QWidget * parent, const char *name )
    : QTreeWidget( parent ),
    mMousePressedFlag( false ),
    mItemBeingMoved( 0 ),
    mToggleEditingAction( 0 ),
    mMapCanvas( 0 ),
    mMinimumIconSize( 20, 20 )
{
  connect( this, SIGNAL( itemChanged( QTreeWidgetItem*, int ) ),
           this, SLOT( handleItemChange( QTreeWidgetItem*, int ) ) );

  connect( this, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ),
           this, SLOT( handleCurrentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );

  // project handling
  connect( QgsProject::instance(), SIGNAL( readProject( const QDomDocument & ) ),
           this, SLOT( readProject( const QDomDocument & ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument & ) ),
           this, SLOT( writeProject( QDomDocument & ) ) );

  // Initialise the line indicator widget.
  mInsertionLine = new QWidget( viewport() );
  hideLine();
  mInsertionLine->setAutoFillBackground( true );
  QPalette pal = mInsertionLine->palette();
  pal.setColor( mInsertionLine->backgroundRole(), Qt::blue );
  mInsertionLine->setPalette( pal );

  setSortingEnabled( false );
  setDragEnabled( false );
  setAutoScroll( true );
  QFont f( "Arial", 10, QFont::Normal );
  setFont( f );
  QPalette palette;
  palette.setColor( backgroundRole(), QColor( 192, 192, 192 ) );
  setPalette( palette );

  setColumnCount( 1 );
  header()->setHidden( 1 );
  setRootIsDecorated( true );
  initPixmaps();
}

QgsLegend::~QgsLegend()
{
  delete mInsertionLine;
}

void QgsLegend::handleCurrentItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous )
{
  QgsMapLayer *layer = currentLayer();

  if ( mMapCanvas )
  {
    mMapCanvas->setCurrentLayer( layer );
  }
  emit currentLayerChanged( layer );
}

void QgsLegend::addGroup()
{
  QgsLegendGroup* group = new QgsLegendGroup( this, tr( "group" ) );
  group->setData( 0, Qt::UserRole, Qt::Checked );
  setExpanded( indexFromItem( group ), true );
}

void QgsLegend::removeAll()
{
  clear();
  mPixmapWidthValues.clear();
  mPixmapHeightValues.clear();
  updateMapCanvasLayerSet();
  setIconSize( mMinimumIconSize );
  mItemBeingMoved = 0;
  mDropTarget = 0;
}

void QgsLegend::selectAll( bool select )
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  // Turn off rendering to improve speed.
  bool renderFlagState = mMapCanvas->renderFlag();
  mMapCanvas->setRenderFlag( false );

  QTreeWidgetItem* theItem = firstItem();

  while ( theItem )
  {
    QgsLegendItem* litem = dynamic_cast<QgsLegendItem *>( theItem );
    if ( litem && litem->type() == QgsLegendItem::LEGEND_LAYER_FILE )
    {
      theItem->setCheckState( 0, ( select ? Qt::Checked : Qt::Unchecked ) );
      handleItemChange( theItem, 0 );
    }
    theItem = nextItem( theItem );
  }
  // Turn on rendering (if it was on previously)
  mMapCanvas->setRenderFlag( renderFlagState );
}

void QgsLegend::removeLayer( QString layer_key )
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  QTreeWidgetItem* theItem = firstItem();
  QgsDebugMsg( "called." );

  while ( theItem )
  {
    QgsLegendItem *li = dynamic_cast<QgsLegendItem *>( theItem );
    if ( li )
    {
      // save legend layer (parent of a legend layer file we're going to delete)
      QgsLegendLayer* ll = qobject_cast<QgsLegendLayer *>( li );

      if ( ll && ll->layer() && ll->layer()->getLayerID() == layer_key )
      {
        removeItem( ll );
        delete ll;

        break;
      }

    }
    theItem = nextItem( theItem );
  }

  updateMapCanvasLayerSet();
  adjustIconSize();
}

void QgsLegend::mousePressEvent( QMouseEvent * e )
{
  if ( e->button() == Qt::LeftButton )
  {
    mLastPressPos = e->pos();
    mMousePressedFlag = true;
  }
  else if ( e->button() == Qt::RightButton )
  {
    QTreeWidgetItem* item = itemAt( e->pos() );
    setCurrentItem( item );
    handleRightClickEvent( item, e->globalPos() );
  }
  QTreeWidget::mousePressEvent( e );
}                               // contentsMousePressEvent

void QgsLegend::mouseMoveEvent( QMouseEvent * e )
{
  if ( mMousePressedFlag )
  {
    //set the flag back such that the else if(mItemBeingMoved)
    //code part is passed during the next mouse moves
    mMousePressedFlag = false;

    // remember item we've pressed as the one being moved
    // and where it was originally
    QTreeWidgetItem* item = itemAt( mLastPressPos );
    if ( item )
    {
      mItemBeingMoved = item;
      mItemBeingMovedOrigPos = getItemPos( mItemBeingMoved );

      //store information to insert the item back to the original position
      storeInitialPosition( mItemBeingMoved );

      setCursor( Qt::SizeVerCursor );
    }
  }
  else if ( mItemBeingMoved )
  {
    QPoint p( e->pos() );
    mLastPressPos = p;

    // change the cursor appropriate to if drop is allowed
    QTreeWidgetItem* item = itemAt( p );

    hideLine();
    updateLineWidget();
    scrollToItem( item );

    QgsLegendItem* origin = dynamic_cast<QgsLegendItem *>( mItemBeingMoved );
    QgsLegendItem* dest = dynamic_cast<QgsLegendItem *>( item );

    if ( item )
    {
      mDropTarget = item;
      QgsLegendItem::DRAG_ACTION action = dest->accept( origin );
      if ( item != mItemBeingMoved )
      {
        if ( yCoordAboveCenter( dest, e->y() ) ) //over center of item
        {
          int line_y    = visualItemRect( item ).top() + 1;
          int line_left = visualItemRect( item ).left();

          if ( action == QgsLegendItem::REORDER ||  action == QgsLegendItem::INSERT )
          {
            QgsDebugMsg( "mouseMoveEvent::INSERT or REORDER" );
            mDropAction = BEFORE;
            showLine( line_y, line_left );
            setCursor( QCursor( Qt::SizeVerCursor ) );
          }
          else //no action
          {
            QgsDebugMsg( "mouseMoveEvent::NO_ACTION" );
            mDropAction = NO_ACTION;
            setCursor( QCursor( Qt::ForbiddenCursor ) );
          }
        }
        else // below center of item
        {
          int line_y    = visualItemRect( item ).bottom() - 2;
          int line_left = visualItemRect( item ).left();

          if ( action == QgsLegendItem::REORDER )
          {
            QgsDebugMsg( "mouseMoveEvent::REORDER bottom half" );
            mDropAction = AFTER;
            showLine( line_y, line_left );
            setCursor( QCursor( Qt::SizeVerCursor ) );
          }
          else if ( action == QgsLegendItem::INSERT )
          {
            QgsDebugMsg( "mouseMoveEvent::INSERT" );
            mDropAction = INTO_GROUP;
            showLine( line_y, line_left );
            setCursor( QCursor( Qt::SizeVerCursor ) );
          }
          else//no action
          {
            mDropAction = NO_ACTION;
            QgsDebugMsg( "mouseMoveEvent::NO_ACTION" );
            setCursor( QCursor( Qt::ForbiddenCursor ) );
          }
        }
      }
      else
      {
        setCursor( QCursor( Qt::ForbiddenCursor ) );
      }
    }
    else if ( !item && e->pos().y() >= 0 && e->pos().y() < viewport()->height() &&  e->pos().x() >= 0 && e->pos().x() < viewport()->width() )
    {
      // Outside the listed items, but check if we are in the empty area
      // of the viewport, so we can drop after the last top level item.
      QgsDebugMsg( "You are below the table" );
      mDropTarget = topLevelItem( topLevelItemCount() - 1 );
      dest = dynamic_cast<QgsLegendItem *>( mDropTarget );
      QgsLegendItem::DRAG_ACTION action = dest->accept( origin );
      if ( action == QgsLegendItem::REORDER ||  action == QgsLegendItem::INSERT )
      {
        QgsDebugMsg( "mouseMoveEvent::INSERT or REORDER" );
        mDropAction = AFTER;
        showLine( visualItemRect( lastVisibleItem() ).bottom() + 1, 0 );
        setCursor( QCursor( Qt::SizeVerCursor ) );
      }
      else //no action
      {
        QgsDebugMsg( "mouseMoveEvent::NO_ACTION" );
        mDropAction = NO_ACTION;
        setCursor( QCursor( Qt::ForbiddenCursor ) );
      }
    }

    else
    {
      QgsDebugMsg( "No item here" );
      mDropTarget = NULL;
      setCursor( QCursor( Qt::ForbiddenCursor ) );
    }
  }
}

void QgsLegend::mouseReleaseEvent( QMouseEvent * e )
{
  QTreeWidget::mouseReleaseEvent( e );
  setCursor( QCursor( Qt::ArrowCursor ) );

  mMousePressedFlag = false;

  if ( !mItemBeingMoved )
  {
    return;
  }

  hideLine();

  QTreeWidgetItem *destItem = mDropTarget;

  QgsLegendItem* origin = dynamic_cast<QgsLegendItem *>( mItemBeingMoved );
  mItemBeingMoved = NULL;

  QgsLegendItem* dest = dynamic_cast<QgsLegendItem *>( destItem );

  // no change?
  if ( !dest || !origin || ( dest == origin ) )
  {
    checkLayerOrderUpdate();
    return;
  }

  {
    // Do the actual move here.
    QgsDebugMsg( "Drag'n'drop happened!" );
    if ( mDropAction == AFTER ) //over center of item
    {
      QgsDebugMsg( "Drop AFTER" );
      if ( dest->nextSibling() != origin )
      {
        moveItem( origin, dest );
        setCurrentItem( origin );
      }
    }
    else if ( mDropAction == BEFORE )// below center of item
    {
      QgsDebugMsg( "Drop BEFORE" );
      if ( dest->findYoungerSibling() != origin )
      {
        moveItem( origin, dest ); // Insert after, as above...
        moveItem( dest, origin ); // ... and then switch places!
        setCurrentItem( origin );
      }
    }
    else if ( mDropAction == INTO_GROUP )
    {
      QgsDebugMsg( "Drop INTO_GROUP" );
      if ( origin->parent() != dest )
      {
        insertItem( origin, dest );
        setCurrentItem( origin );
      }
    }
    else//no action
    {
      QgsDebugMsg( "Drop NO_ACTION" );
    }
  }

  checkLayerOrderUpdate();
}

void QgsLegend::mouseDoubleClickEvent( QMouseEvent* e )
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }
  legendLayerShowProperties();
}

void QgsLegend::handleRightClickEvent( QTreeWidgetItem* item, const QPoint& position )
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  QMenu theMenu;

  QgsLegendItem* li = dynamic_cast<QgsLegendItem *>( item );
  if ( li )
  {

    if ( li->type() == QgsLegendItem::LEGEND_LAYER )
    {
      ( static_cast<QgsLegendLayer*>( li ) )->addToPopupMenu( theMenu, mToggleEditingAction );

      if ( li->parent() )
      {
        theMenu.addAction( tr( "&Make to toplevel item" ), this, SLOT( makeToTopLevelItem() ) );
      }

    }
    else if ( li->type() == QgsLegendItem::LEGEND_GROUP )
    {
      theMenu.addAction( QgisApp::getThemeIcon( "/mActionRemove.png" ),
                         tr( "&Remove" ), this, SLOT( legendGroupRemove() ) );
    }

    if ( li->type() == QgsLegendItem::LEGEND_LAYER || li->type() == QgsLegendItem::LEGEND_GROUP )
    {
      theMenu.addAction( tr( "Re&name" ), this, SLOT( openEditor() ) );
    }

  }

  theMenu.addAction( QgisApp::getThemeIcon( "/folder_new.png" ), tr( "&Add group" ), this, SLOT( addGroup() ) );
  theMenu.addAction( QgisApp::getThemeIcon( "/mActionExpandTree.png" ), tr( "&Expand all" ), this, SLOT( expandAll() ) );
  theMenu.addAction( QgisApp::getThemeIcon( "/mActionCollapseTree.png" ), tr( "&Collapse all" ), this, SLOT( collapseAll() ) );

  theMenu.exec( position );
}

void QgsLegend::initPixmaps()
{
  mPixmaps.mOriginalPixmap = QgisApp::getThemePixmap( "/mActionFileSmall.png" );
  mPixmaps.mInOverviewPixmap = QgisApp::getThemePixmap( "/mActionInOverview.png" );
  mPixmaps.mEditablePixmap = QgisApp::getThemePixmap( "/mIconEditable.png" );
  mPixmaps.mProjectionErrorPixmap = QgisApp::getThemePixmap( "/mIconProjectionProblem.png" );
}

int QgsLegend::getItemPos( QTreeWidgetItem* item )
{
  int counter = 1;
  QTreeWidgetItem* theItem = firstItem();
  while ( theItem )
  {
    if ( theItem == item )
    {
      return counter;
    }
    theItem = nextItem( theItem );
    ++counter;
  }
  return -1;
}

void QgsLegend::addLayer( QgsMapLayer * layer )
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  QgsLegendLayer* llayer = new QgsLegendLayer( layer );

  llayer->updateIcon();
  llayer->setToolTip( 0, layer->publicSource() );

  //set the correct check states
  blockSignals( true );
  if ( llayer->isVisible() )
  {
    llayer->setCheckState( 0, Qt::Checked );
    llayer->setData( 0, Qt::UserRole, Qt::Checked );
  }
  else
  {
    llayer->setCheckState( 0, Qt::Unchecked );
    llayer->setData( 0, Qt::UserRole, Qt::Unchecked );
  }
  blockSignals( false );

  insertTopLevelItem( 0, llayer );
  setItemExpanded( llayer, true );

  refreshLayerSymbology( layer->getLayerID() );

  updateMapCanvasLayerSet();

  // first layer?
  if ( mMapCanvas->layerCount() == 1 )
  {
    mMapCanvas->zoomToFullExtent();
    mMapCanvas->clearExtentHistory();
  }
  setCurrentItem( llayer );
  //make the QTreeWidget item up-to-date
  doItemsLayout();

  // setup connections that will update the layer icons
  if ( qobject_cast<QgsVectorLayer *>( layer ) )
  {
    QgsDebugMsg( "Connecting signals for updating icons, layer " + layer->name() );
    connect( layer, SIGNAL( editingStarted() ), llayer, SLOT( updateIcon() ) );
    connect( layer, SIGNAL( editingStopped() ), llayer, SLOT( updateIcon() ) );
  }
}

QgsLegendLayer* QgsLegend::currentLegendLayer()
{
  QgsLegendItem* citem = dynamic_cast<QgsLegendItem *>( currentItem() );

  if ( citem )
  {
    QgsLegendLayer* ll = qobject_cast<QgsLegendLayer *>( citem );
    if ( ll )
    {
      return ll; //the current item is a legend layer, so return it
    }

    QgsLegendLayer* lpl = dynamic_cast<QgsLegendLayer *>( citem->parent() );
    if ( lpl )
    {
      return lpl; //the parent of the current item is a legend layer, return it
    }
  }

  return 0;
}

QgsMapLayer* QgsLegend::currentLayer()
{
  QgsLegendLayer* ll = currentLegendLayer();
  if ( ll )
  {
    return ll->layer();
  }
  else
  {
    return 0;
  }
}

void QgsLegend::legendGroupRemove()
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  QgsLegendGroup* lg = dynamic_cast<QgsLegendGroup *>( currentItem() );
  if ( lg )
  {
    //delete the legend layers first
    QTreeWidgetItem * child = lg->child( 0 );
    while ( child )
    {
      setCurrentItem( child );
      removeCurrentLayer();
      child = lg->child( 0 );
    }
    delete lg;
    adjustIconSize();
  }
}

void QgsLegend::removeCurrentLayer()
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  //if the current item is a legend layer: remove all layers of the current legendLayer
  QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer *>( currentItem() );
  if ( !ll )
    return;

  removeLayer( ll->layer(), true );

  adjustIconSize();
}

bool QgsLegend::removeLayer( QgsMapLayer* ml, bool askCancelOnEditable )
{
  if ( !ml )
  {
    return false;
  }

  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer *>( ml );
  if ( vl )
  {
    //is layer editable and changed?
    if ( vl->isEditable() && vl->isModified() )
    {
      QMessageBox::StandardButton commit;
      if ( askCancelOnEditable )
      {
        commit = QMessageBox::information( this,
                                           tr( "Stop editing" ),
                                           tr( "Do you want to save the changes to layer %1?" ).arg( vl->name() ),
                                           QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel );
        if ( commit == QMessageBox::Cancel )
        {
          return false;
        }
      }
      else
      {
        commit = QMessageBox::information( this,
                                           tr( "Stop editing" ),
                                           tr( "Do you want to save the changes to layer %1?" ).arg( vl->name() ),
                                           QMessageBox::Save | QMessageBox::Discard );
      }

      if ( commit == QMessageBox::Save )
      {
        if ( !vl->commitChanges() )
        {
          return false;
        }
      }
      else if ( commit == QMessageBox::Discard )
      {
        if ( !vl->rollBack() )
        {
          return false;
        }
      }
    }
  }
  QgsMapLayerRegistry::instance()->removeMapLayer( ml->getLayerID() );
  return true;
}



void QgsLegend::legendLayerShowProperties()
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  QgsLegendItem* li = dynamic_cast<QgsLegendItem *>( currentItem() );

  if ( !li )
  {
    return;
  }

  if ( li->type() != QgsLegendItem::LEGEND_LAYER )
    return;

  QgsLegendLayer* ll = qobject_cast<QgsLegendLayer *>( li );
  if ( !ll )
    return;

  //QgsDebugMsg("Showing layer properties dialog");

  QgsMapLayer* ml = ll->layer();

  /*
  TODO: Consider reusing the property dialogs again.
  Sometimes around mid 2005, the property dialogs were saved for later reuse;
  this resulted in a time savings when reopening the dialog. The code below
  cannot be used as is, however, simply by saving the dialog pointer here.
  Either the map layer needs to be passed as an argument to sync or else
  a separate copy of the dialog pointer needs to be stored with each layer.
  */

  if ( ml->type() == QgsMapLayer::RasterLayer )
  {
    QgsRasterLayerProperties *rlp = NULL; // See note above about reusing this
    if ( rlp )
    {
      rlp->sync();
    }
    else
    {
      rlp = new QgsRasterLayerProperties( ml );
      connect( rlp, SIGNAL( refreshLegend( QString, bool ) ), this, SLOT( refreshLayerSymbology( QString, bool ) ) );
    }
    rlp->exec();
    delete rlp; // delete since dialog cannot be reused without updating code
  }
  else // VECTOR
  {
    QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer *>( ml );

    QgsVectorLayerProperties *vlp = NULL; // See note above about reusing this
    if ( vlp )
    {
      vlp->reset();
    }
    else
    {
      vlp = new QgsVectorLayerProperties( vlayer );
      connect( vlp, SIGNAL( refreshLegend( QString, bool ) ), this, SLOT( refreshLayerSymbology( QString, bool ) ) );
    }
    vlp->exec();
    delete vlp; // delete since dialog cannot be reused without updating code
  }

  ll->updateIcon();

}

void QgsLegend::legendLayerShowInOverview()
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  QgsLegendItem* li = dynamic_cast<QgsLegendItem *>( currentItem() );
  if ( !li )
    return;

  if ( li->type() == QgsLegendItem::LEGEND_LAYER )
    return;

  QgsLegendLayer* ll = qobject_cast<QgsLegendLayer *>( li );
  if ( !ll ) return;
  ll->showInOverview();
}

void QgsLegend::expandAll()
{
  QTreeWidgetItem* theItem = firstItem();
  while ( theItem )
  {
    setExpanded( indexFromItem( theItem ), true );
    theItem = nextItem( theItem );
  }
}

void QgsLegend::collapseAll()
{
  QTreeWidgetItem* theItem = firstItem();
  while ( theItem )
  {
    setExpanded( indexFromItem( theItem ), false );
    theItem = nextItem( theItem );
  }
}

bool QgsLegend::writeXML( QDomNode & legendnode, QDomDocument & document )
{
  QDomNode tmplegendnode = legendnode; /*copy of the legendnode*/
  QDomElement legendgroupnode;
  QDomElement legendlayernode;
  QDomElement layerfilegroupnode;
  QDomElement legendsymbolnode;
  QDomElement legendpropertynode;
  QDomElement legendlayerfilenode;
  Qt::CheckState cstate; //check state for legend layers and legend groups

  QTreeWidgetItem* currentItem = firstItem();
  while ( currentItem )
  {
    QgsLegendItem *item = dynamic_cast<QgsLegendItem *>( currentItem );
    if ( item )
    {
      switch ( item->type() )
      {
        case QgsLegendItem::LEGEND_GROUP:
          //make sure the legendnode is 'legend' again after a legend group
          if ( !( item->parent() ) )
          {
            legendnode = tmplegendnode;
          }
          legendgroupnode = document.createElement( "legendgroup" );
          legendgroupnode.setAttribute( "open", isItemExpanded( item ) ? "true" : "false" );
          legendgroupnode.setAttribute( "name", item->text( 0 ) );
          cstate = item->checkState( 0 );
          if ( cstate == Qt::Checked )
          {
            legendgroupnode.setAttribute( "checked", "Qt::Checked" );
          }
          else if ( cstate == Qt::Unchecked )
          {
            legendgroupnode.setAttribute( "checked", "Qt::Unchecked" );
          }
          else if ( cstate == Qt::PartiallyChecked )
          {
            legendgroupnode.setAttribute( "checked", "Qt::PartiallyChecked" );
          }
          legendnode.appendChild( legendgroupnode );
          tmplegendnode =  legendnode;
          legendnode = legendgroupnode;
          break;

        case QgsLegendItem::LEGEND_LAYER:
          //make sure the legendnode is 'legend' again after a legend group
          if ( !( item->parent() ) )
          {
            legendnode = tmplegendnode;
          }
          legendlayernode = document.createElement( "legendlayer" );
          legendlayernode.setAttribute( "open", isItemExpanded( item ) ? "true" : "false" );
          cstate = item->checkState( 0 );
          if ( cstate == Qt::Checked )
          {
            legendlayernode.setAttribute( "checked", "Qt::Checked" );
          }
          else if ( cstate == Qt::Unchecked )
          {
            legendlayernode.setAttribute( "checked", "Qt::Unchecked" );
          }
          else if ( cstate == Qt::PartiallyChecked )
          {
            legendlayernode.setAttribute( "checked", "Qt::PartiallyChecked" );
          }
          legendlayernode.setAttribute( "name", item->text( 0 ) );
          legendnode.appendChild( legendlayernode );

          // save the information about layer
          // emulate a legend layer file group and a legend layer file
          // to keep it compatible with older projects
          {
            QgsLegendLayer *ll = dynamic_cast<QgsLegendLayer *>( item );
            QgsMapLayer* layer = ll->layer();

            layerfilegroupnode = document.createElement( "filegroup" );
            layerfilegroupnode.setAttribute( "open", isItemExpanded( item ) ? "true" : "false" );
            layerfilegroupnode.setAttribute( "hidden", isItemHidden( item ) ? "true" : "false" );
            legendlayernode.appendChild( layerfilegroupnode );

            legendlayerfilenode = document.createElement( "legendlayerfile" );

            // layer id
            legendlayerfilenode.setAttribute( "layerid", layer->getLayerID() );
            layerfilegroupnode.appendChild( legendlayerfilenode );

            // visible flag
            legendlayerfilenode.setAttribute( "visible", ll->isVisible() );

            // show in overview flag
            legendlayerfilenode.setAttribute( "isInOverview", ll->isInOverview() );
          }
          break;

        case QgsLegendItem::LEGEND_PROPERTY_GROUP:
          legendpropertynode = document.createElement( "propertygroup" );
          legendpropertynode.setAttribute( "open", isItemExpanded( item ) ? "true" : "false" );
          legendlayernode.appendChild( legendpropertynode );
          break;

        case QgsLegendItem::LEGEND_SYMBOL_GROUP:
          legendsymbolnode = document.createElement( "symbolgroup" );
          legendsymbolnode.setAttribute( "open", isItemExpanded( item ) ? "true" : "false" );
          legendlayernode.appendChild( legendsymbolnode );
          break;

        default: //do nothing for the leaf nodes
          break;
      }
    }
    currentItem = nextItem( currentItem );
  }
  return true;
}

bool QgsLegend::readXML( QDomNode& legendnode )
{
  QDomElement childelem;
  QDomNode child;
  QgsLegendGroup* lastGroup = 0; //pointer to the last inserted group
  QgsLegendLayer* lastLayer = 0; //pointer to the last inserted legendlayer

  child = legendnode.firstChild();

  // For some unexplained reason, collapsing/expanding the legendLayer items
  // immediately after they have been created doesn't work (they all end up
  // expanded). The legendGroups and legendLayerFiles seems ok through. The
  // workaround is to store the required states of the legendLayers and set
  // them at the end of this function.
  QList<QTreeWidgetItem*> collapsed, expanded;

  if ( !child.isNull() )
  {
    clear(); //remove all items first

    do
    {
      QDomElement childelem = child.toElement();
      QString name = childelem.attribute( "name" );

      //test every possibility of element...
      if ( childelem.tagName() == "legendgroup" )
      {
        QgsLegendGroup* theGroup = new QgsLegendGroup( this, name );
        childelem.attribute( "open" ) == "true" ? expanded.push_back( theGroup ) : collapsed.push_back( theGroup );
        //set the checkbox of the legend group to the right state
        blockSignals( true );
        QString checked = childelem.attribute( "checked" );
        if ( checked == "Qt::Checked" )
        {
          theGroup->setCheckState( 0, Qt::Checked );
          theGroup->setData( 0, Qt::UserRole, Qt::Checked );
        }
        else if ( checked == "Qt::Unchecked" )
        {
          theGroup->setCheckState( 0, Qt::Unchecked );
          theGroup->setData( 0, Qt::UserRole, Qt::Checked );
        }
        else if ( checked == "Qt::PartiallyChecked" )
        {
          theGroup->setCheckState( 0, Qt::PartiallyChecked );
          theGroup->setData( 0, Qt::UserRole, Qt::PartiallyChecked );
        }
        blockSignals( false );
        lastGroup = theGroup;
      }
      else if ( childelem.tagName() == "legendlayer" )
      {
        bool isOpen; // to receive info whether the item is open or closed
        lastLayer = readLayerFromXML( childelem, isOpen );

        if ( lastLayer )
        {

          // add to tree - either as a top-level node or a child of a group
          if ( child.parentNode().toElement().tagName() == "legendgroup" )
          {
            lastGroup->addChild( lastLayer );
          }
          else
          {
            addTopLevelItem( lastLayer );
            lastGroup = 0;
          }

          // expanded or collapsed
          isOpen ? expanded.push_back( lastLayer ) : collapsed.push_back( lastLayer );

          // load symbology
          refreshLayerSymbology( lastLayer->layer()->getLayerID() );
        }
      }
      else if ( childelem.tagName() == "legendlayerfile" )
      {
        // do nothing, this has been handled in readLayerFromXML()
      }
      else if ( childelem.tagName() == "filegroup" )
      {
        // do nothing, this has been handled in readLayerFromXML()
      }
      else if ( childelem.tagName() == "propertygroup" )
      {
        QgsLegendPropertyGroup* thePropertyGroup = new QgsLegendPropertyGroup( lastLayer, "Properties" );
        childelem.attribute( "open" ) == "true" ? expandItem( thePropertyGroup ) : collapseItem( thePropertyGroup );
      }
      child = nextDomNode( child );
    }
    while ( !( child.isNull() ) );
  }

  // Do the tree item expands and collapses.
  for ( int i = 0; i < expanded.size(); ++i )
    expandItem( expanded[i] );
  for ( int i = 0; i < collapsed.size(); ++i )
    collapseItem( collapsed[i] );

  return true;
}


QgsLegendLayer* QgsLegend::readLayerFromXML( QDomElement& childelem, bool& isOpen )
{
  QDomElement fileGroupElem = childelem.firstChildElement( "filegroup" );
  if ( fileGroupElem.isNull() )
    return NULL; // we need a file group!

  QDomElement fileElem = fileGroupElem.firstChildElement( "legendlayerfile" );
  if ( fileElem.isNull() ) // we need a file element!
    return NULL;

  QgsMapLayer* theMapLayer = QgsMapLayerRegistry::instance()->mapLayer( fileElem.attribute( "layerid" ) );
  if ( theMapLayer == NULL ) //the layer cannot be found (e.g. the file has been moved)
  {
    return NULL;
  }

  // create the item
  QgsLegendLayer* ll = new QgsLegendLayer( theMapLayer );

  // load layer's visibility and 'show in overview' flag
  ll->setInOverview( atoi( fileElem.attribute( "isInOverview" ).toUtf8() ) );

  //set the layer type icon
  ll->updateIcon();

  // setup connections that will update the layer icons
  if ( qobject_cast<QgsVectorLayer *>( theMapLayer ) )
  {
    QgsDebugMsg( "Connecting signals for updating icons, layer " + theMapLayer->name() );
    connect( theMapLayer, SIGNAL( editingStarted() ), ll, SLOT( updateIcon() ) );
    connect( theMapLayer, SIGNAL( editingStopped() ), ll, SLOT( updateIcon() ) );
  }

  // expanded or collapsed
  isOpen = ( childelem.attribute( "open" ) == "true" );

  //set the checkbox of the legend layer to the right state
  blockSignals( true );
  QString checked = childelem.attribute( "checked", "Qt::Checked" ); // Default is to show
  if ( checked == "Qt::Checked" )
  {
    ll->setVisible( true );
    ll->setCheckState( 0, Qt::Checked );
    ll->setData( 0, Qt::UserRole, Qt::Checked );
  }
  else if ( checked == "Qt::Unchecked" )
  {
    ll->setVisible( false );
    ll->setCheckState( 0, Qt::Unchecked );
    ll->setData( 0, Qt::UserRole, Qt::Unchecked );
  }
  blockSignals( false );

  return ll;
}


void QgsLegend::storeInitialPosition( QTreeWidgetItem* li )
{
  if ( li == firstItem() ) //the item is the first item in the list view
  {
    mRestoreInformation = FIRST_ITEM;
    mRestoreItem = 0;
  }
  else if ( li->parent() == 0 ) //li is a toplevel item, but not the first one
  {
    mRestoreInformation = YOUNGER_SIBLING;
    mRestoreItem = (( QgsLegendItem* )( li ) )->findYoungerSibling();
  }
  else if ( li == li->parent()->child( 0 ) )//li is not a toplevel item, but the first child
  {
    mRestoreInformation = FIRST_CHILD;
    mRestoreItem = li->parent();
  }
  else
  {
    mRestoreInformation = YOUNGER_SIBLING;
    mRestoreItem = (( QgsLegendItem* )( li ) )->findYoungerSibling();
  }
  mLayersPriorToMove = layerIDs();
}

void QgsLegend::resetToInitialPosition( QTreeWidgetItem* li )
{
  QgsLegendItem* formerParent = dynamic_cast<QgsLegendItem *>( li->parent() ); //todo: make sure legend layers are updated
  if ( mRestoreInformation == FIRST_ITEM )
  {
    QgsDebugMsg( "FIRST_ITEM" );

    removeItem( li );
    insertTopLevelItem( 0, li );
  }
  else if ( mRestoreInformation == FIRST_CHILD )
  {
    QgsDebugMsg( "FIRST_CHILD" );

    removeItem( li );
    if ( formerParent )
    {
      formerParent->release(( QgsLegendItem* )li );
    }
    mRestoreItem->insertChild( 0, li );
    (( QgsLegendItem* )mRestoreItem )->receive(( QgsLegendItem* )li );
  }
  else if ( mRestoreInformation == YOUNGER_SIBLING )
  {
    QgsDebugMsg( "YOUNGER_SIBLING" );

    if ( formerParent )
    {
      formerParent->release(( QgsLegendItem* )li );
    }
    dynamic_cast<QgsLegendItem *>( li )->moveItem( dynamic_cast<QgsLegendItem*>( mRestoreItem ) );
    if ( mRestoreItem->parent() )
    {
      (( QgsLegendItem* )( mRestoreItem->parent() ) )->receive(( QgsLegendItem* )li );
    }
  }
}

QgsLegendLayer* QgsLegend::findLegendLayer( const QString& layerKey )
{
  QgsLegendLayer* theLegendLayer = 0;
  QTreeWidgetItem* theItem = firstItem();
  do
  {
    theLegendLayer = dynamic_cast<QgsLegendLayer *>( theItem );
    if ( theLegendLayer ) //item is a legend layer
    {
      if ( theLegendLayer->layer()->getLayerID() == layerKey )
      {
        return theLegendLayer;
      }
    }
  }
  while (( theItem = nextItem( theItem ) ) );
  return 0;
}

void QgsLegend::adjustIconSize()
{
  if ( mPixmapWidthValues.size() > 0 && mPixmapHeightValues.size() > 0 )
  {
    std::multiset<int>::const_reverse_iterator width_it = mPixmapWidthValues.rbegin();
    std::multiset<int>::const_reverse_iterator height_it = mPixmapHeightValues.rbegin();
    int maxWidth = *width_it;
    int maxHeight = *height_it;

    QSize currentIconSize = iconSize();
    if ( maxWidth == currentIconSize.width() && maxHeight == currentIconSize.height() )
    {
      //no resizing necessary
      return;
    }

    //keep the minimum size
    if ( maxWidth < mMinimumIconSize.width() )
    {
      maxWidth = mMinimumIconSize.width();
    }
    if ( maxHeight < mMinimumIconSize.height() )
    {
      maxHeight = mMinimumIconSize.height();
    }

    setIconSize( QSize( maxWidth, maxHeight ) );
  }
}

bool QgsLegend::yCoordAboveCenter( QgsLegendItem* it, int ycoord )
{
  QRect rect = visualItemRect( it );
  int height = rect.height();
  int top = rect.top();
  int mid = top + ( height / 2 );
  if ( ycoord > mid ) //bottom, remember the y-coordinate increases downwards
  {
    return false;
  }
  else//top
  {
    return true;
  }
}

/**Returns the first item in the hierarchy*/
QTreeWidgetItem* QgsLegend::firstItem()
{
  return topLevelItem( 0 );
}

/**Returns the next item (next sibling or next item on level above)*/
QTreeWidgetItem* QgsLegend::nextItem( QTreeWidgetItem* item )
{
  QgsLegendItem* litem = dynamic_cast<QgsLegendItem *>( item );
  if ( !litem )
  {
    return 0;
  }
  else if ( litem->childCount() > 0 )
  {
    return litem->child( 0 );
  }
  else if ( litem->nextSibling() )
  {
    return litem->nextSibling();
  }
  else if ( !litem->parent() )
  {
    return 0;
  }
  //go to other levels to find the next item
  else if ( litem->parent() && dynamic_cast<QgsLegendItem *>( litem->parent() )->nextSibling() )
  {
    return dynamic_cast<QgsLegendItem *>( litem->parent() )->nextSibling();
  }
  else if ( litem->parent() && litem->parent()->parent() && dynamic_cast<QgsLegendItem*>( litem->parent()->parent() )->nextSibling() )
  {
    return dynamic_cast<QgsLegendItem *>( litem->parent()->parent() )->nextSibling();
  }
  else if ( litem->parent() && litem->parent()->parent() && litem->parent()->parent()->parent() &&
            dynamic_cast<QgsLegendItem *>( litem->parent()->parent()->parent() )->nextSibling() )//maximum four nesting states in the current legend
  {
    return dynamic_cast<QgsLegendItem *>( litem->parent()->parent()->parent() )->nextSibling();
  }
  else
  {
    return 0;
  }
}

QTreeWidgetItem* QgsLegend::nextSibling( QTreeWidgetItem* item )
{
  QModelIndex thisidx = indexFromItem( item );
  QModelIndex nextsidx = thisidx.sibling( thisidx.row() + 1, thisidx.column() );
  if ( nextsidx.isValid() )
  {
    return dynamic_cast<QgsLegendItem *>( itemFromIndex( nextsidx ) );
  }
  else
  {
    return 0;
  }
}

QTreeWidgetItem* QgsLegend::previousSibling( QTreeWidgetItem* item )
{
  QModelIndex thisidx = indexFromItem( item );
  QModelIndex nextsidx = thisidx.sibling( thisidx.row() - 1, thisidx.column() );
  if ( nextsidx.isValid() )
  {
    return dynamic_cast<QgsLegendItem *>( itemFromIndex( nextsidx ) );
  }
  else
  {
    return 0;
  }
}

QDomNode QgsLegend::nextDomNode( const QDomNode& theNode )
{
  if ( !theNode.firstChild().isNull() )
  {
    return theNode.firstChild();
  }

  QDomNode currentNode = theNode;
  do
  {
    if ( !currentNode.nextSibling().isNull() )
    {
      return currentNode.nextSibling();
    }
    currentNode = currentNode.parentNode();
  }
  while ( !currentNode.isNull() );

  QDomNode nullNode;
  return nullNode;
}

void QgsLegend::insertItem( QTreeWidgetItem* move, QTreeWidgetItem* into )
{
  QgsLegendItem* movedItem = dynamic_cast<QgsLegendItem *>( move );
  QgsLegendItem* intoItem = dynamic_cast<QgsLegendItem *>( into );

  if ( movedItem && intoItem )
  {
    QgsLegendItem* parentItem = dynamic_cast<QgsLegendItem *>( movedItem->parent() );
    movedItem->storeAppearanceSettings();//store settings in the moved item and its children
    removeItem( movedItem );
    intoItem->insert( movedItem );
    if ( parentItem )
    {
      parentItem->release( movedItem ); //give the former parent item the possibility to do cleanups
    }
    intoItem->receive( movedItem );
    movedItem->restoreAppearanceSettings();//apply the settings again
  }
}

void QgsLegend::moveItem( QTreeWidgetItem* move, QTreeWidgetItem* after )
{
  QgsDebugMsg( QString( "Moving layer : %1 (%2)" ).arg( move->text( 0 ) ).arg( move->type() ) );
  QgsDebugMsg( QString( "after layer  : %1 (%2)" ).arg( after->text( 0 ) ).arg( after->type() ) );

  static_cast<QgsLegendItem*>( move )->storeAppearanceSettings();//store settings in the moved item and its childern
  if ( move->parent() )
  {
    move->parent()->takeChild( move->parent()->indexOfChild( move ) );
  }
  else //move is toplevel item
  {
    takeTopLevelItem( indexOfTopLevelItem( move ) );
  }
  if ( after->parent() )
  {
    after->parent()->insertChild( after->parent()->indexOfChild( after ) + 1, move );
  }
  else //toplevel item
  {
    insertTopLevelItem( indexOfTopLevelItem( after ) + 1, move );
  }
  static_cast<QgsLegendItem*>( move )->restoreAppearanceSettings();//apply the settings again
}

void QgsLegend::removeItem( QTreeWidgetItem* item )
{
  if ( item->parent() )
  {
    item->parent()->takeChild( item->parent()->indexOfChild( item ) );
  }
  else
  {
    takeTopLevelItem( indexOfTopLevelItem( item ) );
  }
}

void QgsLegend::updateMapCanvasLayerSet()
{
  QList<QgsMapCanvasLayer> layers;

  // create list of the layers
  QTreeWidgetItem* theItem = firstItem();
  while ( theItem )
  {
    QgsLegendItem *li = dynamic_cast<QgsLegendItem *>( theItem );
    QgsLegendLayer* ll = qobject_cast<QgsLegendLayer *>( li );
    if ( ll )
    {
      QgsMapCanvasLayer& lyr = ll->canvasLayer();
      layers.append( lyr );
    }
    theItem = nextItem( theItem );
  }

  // set layers in canvas
  mMapCanvas->setLayerSet( layers );
}

void QgsLegend::updateOverview()
{
  mMapCanvas->updateOverview();
}

void QgsLegend::enableOverviewModeAllLayers( bool isInOverview )
{
  QTreeWidgetItem* theItem = firstItem();
  while ( theItem )
  {
    QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer *>( theItem );
    if ( ll )
    {
      ll->setInOverview( isInOverview );
    }
    theItem = nextItem( theItem );
  }
  updateMapCanvasLayerSet();
  updateOverview();
}

std::deque<QString> QgsLegend::layerIDs()
{
  std::deque<QString> layers;
  QTreeWidgetItem* theItem = firstItem();
  while ( theItem )
  {
    QgsLegendItem *li = dynamic_cast<QgsLegendItem *>( theItem );
    QgsLegendLayer* ll = qobject_cast<QgsLegendLayer *>( li );
    if ( ll )
    {
      QgsMapLayer *lyr = ll->layer();
      layers.push_front( lyr->getLayerID() );
    }
    theItem = nextItem( theItem );
  }

#ifdef QGISDEBUG
  QgsDebugMsg( "QgsLegend::layerIDs()" );
  for ( std::deque<QString>::iterator it = layers.begin(); it != layers.end(); ++it )
  {
    QgsDebugMsg( *it );
  }
#endif

  return layers;
}


void QgsLegend::refreshLayerSymbology( QString key, bool expandItem )
{
  QgsLegendLayer* theLegendLayer = findLegendLayer( key );
  if ( !theLegendLayer )
  {
    return;
  }

  //store the current item
  QTreeWidgetItem* theCurrentItem = currentItem();

  double widthScale = 1.0;
  if ( mMapCanvas && mMapCanvas->map() )
  {
    widthScale = mMapCanvas->map()->paintDevice().logicalDpiX() / 25.4;
  }

  theLegendLayer->refreshSymbology( key, widthScale );

  //restore the current item again
  setCurrentItem( theCurrentItem );
  adjustIconSize();
  if ( expandItem )
  {
    setItemExpanded( theLegendLayer, true );//make sure the symbology items are visible
  }
}


void QgsLegend::addPixmapWidthValue( int width )
{
  mPixmapWidthValues.insert( width );
}

void QgsLegend::addPixmapHeightValue( int height )
{
  mPixmapHeightValues.insert( height );
}

void QgsLegend::removePixmapWidthValue( int width )
{
  std::multiset<int>::iterator it = mPixmapWidthValues.find( width );
  if ( it != mPixmapWidthValues.end() )
  {
    mPixmapWidthValues.erase( it );
  }
  //todo: adapt the icon size if width is the largest value and the size of the next element is higher than the minimum
}

void QgsLegend::removePixmapHeightValue( int height )
{
  std::multiset<int>::iterator it = mPixmapHeightValues.find( height );
  if ( it != mPixmapHeightValues.end() )
  {
    mPixmapHeightValues.erase( height );
  }
  //todo: adapt the icon size if height is the largest value and the size of the next element is higher than the minimum
}


void QgsLegend::handleItemChange( QTreeWidgetItem* item, int row )
{
  if ( !item )
  {
    return;
  }

  bool renderFlagState = mMapCanvas->renderFlag();

  //if the text of a QgsLegendLayer has changed, change the display names of all its maplayers
  // TODO: is this still necessary?
  QgsLegendLayer* theLegendLayer = dynamic_cast<QgsLegendLayer *>( item ); //item is a legend layer
  if ( theLegendLayer )
  {
    theLegendLayer->layer()->setLayerName( theLegendLayer->text( 0 ) );
  }

  // has the checkState changed?
  if ( item->data( 0, Qt::UserRole ).toInt() == item->checkState( 0 ) )
    return;

  QgsLegendGroup* lg = dynamic_cast<QgsLegendGroup *>( item ); //item is a legend group
  if ( lg )
  {
    //set all the child layer files to the new check state
    std::list<QgsLegendLayer*> subfiles = lg->legendLayers();
    bool renderFlagState = mMapCanvas->renderFlag();
    mMapCanvas->setRenderFlag( false );
    for ( std::list<QgsLegendLayer*>::iterator iter = subfiles.begin(); iter != subfiles.end(); ++iter )
    {
#ifdef QGISDEBUG
      if ( item->checkState( 0 ) == Qt::Checked )
      {
        QgsDebugMsg( "item checked" );
      }
      else if ( item->checkState( 0 ) == Qt::Unchecked )
      {
        QgsDebugMsg( "item unchecked" );
      }
      else if ( item->checkState( 0 ) == Qt::PartiallyChecked )
      {
        QgsDebugMsg( "item partially checked" );
      }
#endif
      blockSignals( true );
      ( *iter )->setCheckState( 0, item->checkState( 0 ) );
      blockSignals( false );
      item->setData( 0, Qt::UserRole, item->checkState( 0 ) );
      if (( *iter )->layer() )
      {
        ( *iter )->setVisible( item->checkState( 0 ) == Qt::Checked );
      }
    }

    // If it was on, turn it back on, otherwise leave it
    // off, as turning it on causes a refresh.
    if ( renderFlagState )
      mMapCanvas->setRenderFlag( true );
    item->setData( 0, Qt::UserRole, item->checkState( 0 ) );
  }

  QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer *>( item ); //item is a legend layer
  if ( ll )
  {
    blockSignals( true );
    ll->setCheckState( 0, item->checkState( 0 ) );
    blockSignals( false );
    ll->setData( 0, Qt::UserRole, ll->checkState( 0 ) );
    if ( ll->layer() )
    {
      ll->setVisible( item->checkState( 0 ) == Qt::Checked );
    }

    if ( ll->parent() )
    {
      static_cast<QgsLegendGroup*>( ll->parent() )->updateCheckState();
      ll->parent()->setData( 0, Qt::UserRole, ll->parent()->checkState( 0 ) );
    }
    // If it was on, turn it back on, otherwise leave it
    // off, as turning it on causes a refresh.
    if ( renderFlagState )
    {
      mMapCanvas->setRenderFlag( true );
    }
    mMapCanvas->freeze( false );
    //update check state of the legend group
    item->setData( 0, Qt::UserRole, item->checkState( 0 ) );
  }

  // update layer set
  updateMapCanvasLayerSet();
}

void QgsLegend::openEditor()
{
  QTreeWidgetItem* theItem = currentItem();
  if ( theItem )
  {
    editItem( theItem, 0 );
  }
}

void QgsLegend::makeToTopLevelItem()
{
  QgsLegendItem* theItem = dynamic_cast<QgsLegendItem *>( currentItem() );
  if ( theItem )
  {
    theItem->storeAppearanceSettings();
    removeItem( theItem );
    addTopLevelItem( theItem );
    theItem->restoreAppearanceSettings();
  }
}


void QgsLegend::legendLayerZoom()
{
  //find current Layer
  QgsLegendLayer* currentLayer = dynamic_cast<QgsLegendLayer *>( currentItem() );
  if ( !currentLayer )
    return;

  QgsMapLayer* theLayer = currentLayer->layer();
  QgsRectangle extent = theLayer->extent();

  // Increase bounding box with 5%, so that layer is a bit inside the borders
  extent.scale( 1.05 );

  //zoom to bounding box
  mMapCanvas->setExtent( extent );
  mMapCanvas->refresh();

  // notify the project we've made a change
  QgsProject::instance()->dirty( true );
}

void QgsLegend::legendLayerZoomNative()
{
  //find current Layer
  QgsLegendLayer* currentLayer = dynamic_cast<QgsLegendLayer *>( currentItem() );
  if ( !currentLayer )
    return;

  QgsRasterLayer *layer =  qobject_cast<QgsRasterLayer *>( currentLayer->layer() );
  if ( layer )
  {
    QgsDebugMsg( "Raster units per pixel  : " + QString::number( layer->rasterUnitsPerPixel() ) );
    QgsDebugMsg( "MapUnitsPerPixel before : " + QString::number( mMapCanvas->mapUnitsPerPixel() ) );

    mMapCanvas->zoomByFactor( fabs( layer->rasterUnitsPerPixel() / mMapCanvas->mapUnitsPerPixel() ) );
    mMapCanvas->refresh();

    QgsDebugMsg( "MapUnitsPerPixel after  : " + QString::number( mMapCanvas->mapUnitsPerPixel() ) );

    // notify the project we've made a change
    QgsProject::instance()->dirty( true );
  }
}

void QgsLegend::legendLayerAttributeTable()
{
  if ( !mMapCanvas || mMapCanvas->isDrawing() )
  {
    return;
  }

  QgsVectorLayer *vlayer = 0;

  // try whether it's a legend layer
  QgsLegendLayer* ll = dynamic_cast<QgsLegendLayer *>( currentItem() );
  if ( !ll )
  {
    // nothing selected
    QMessageBox::information( this,
                              tr( "No Layer Selected" ),
                              tr( "To open an attribute table, you must select a vector layer in the legend" ) );
    return;
  }

  vlayer = qobject_cast<QgsVectorLayer *>( ll->layer() );
  if ( vlayer )
  {
    QgsAttributeTableDialog *mDialog = new QgsAttributeTableDialog( vlayer );
    mDialog->show();
    // the dialog will be deleted by itself on close
  }
}

void QgsLegend::readProject( const QDomDocument & doc )
{
  QDomNodeList nodes = doc.elementsByTagName( "legend" );
  if ( nodes.count() )
  {
    QDomNode node = nodes.item( 0 );
    readXML( node );

    // update canvas layers to match the order defined by legend
    updateMapCanvasLayerSet();
  }
  else
  {
    QgsDebugMsg( "Couldn't read legend information from project" );
  }
}

void QgsLegend::writeProject( QDomDocument & doc )
{
  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( !nl.count() )
  {
    QgsDebugMsg( "Unable to find qgis element in project file" );
    return;
  }
  QDomNode qgisNode = nl.item( 0 );  // there should only be one, so zeroth element ok

  QDomElement mapcanvasNode = doc.createElement( "legend" );
  qgisNode.appendChild( mapcanvasNode );
  writeXML( mapcanvasNode, doc );
}


bool QgsLegend::checkLayerOrderUpdate()
{
  std::deque<QString> layersAfterRelease = layerIDs(); //test if canvas redraw is really necessary
  if ( layersAfterRelease != mLayersPriorToMove )
  {
    // z-order has changed - update layer set
    updateMapCanvasLayerSet();
    return true;
  }
  return false;
}

void QgsLegend::hideLine()
{
  mInsertionLine->setGeometry( 0, -100, 1, 1 );
}

void QgsLegend::showLine( int y, int left )
{
  mInsertionLine->setGeometry( left, y, viewport()->width(), 2 );
}

void QgsLegend::updateLineWidget()
{
  mInsertionLine->repaint();
}


QTreeWidgetItem * QgsLegend::lastVisibleItem()
{
  QTreeWidgetItem *current;
  QTreeWidgetItem *next;

  current = topLevelItem( topLevelItemCount() - 1 );
  while (( next = itemBelow( current ) ) )
  {
    current = next;
  }
  return current;
}
