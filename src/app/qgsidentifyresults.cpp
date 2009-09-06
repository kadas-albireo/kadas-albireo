/***************************************************************************
                     qgsidentifyresults.cpp  -  description
                              -------------------
      begin                : Fri Oct 25 2002
      copyright            : (C) 2002 by Gary E.Sherman
      email                : sherman at mrcc dot com
      Romans 3:23=>Romans 6:23=>Romans 5:8=>Romans 10:9,10=>Romans 12
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

#include "qgsidentifyresults.h"
#include "qgscontexthelp.h"
#include "qgsapplication.h"
#include "qgisapp.h"
#include "qgsmaplayer.h"
#include "qgsvectorlayer.h"
#include "qgsrubberband.h"
#include "qgsgeometry.h"
#include "qgsattributedialog.h"
#include "qgsmapcanvas.h"

#include <QCloseEvent>
#include <QLabel>
#include <QAction>
#include <QTreeWidgetItem>
#include <QPixmap>
#include <QSettings>
#include <QMenu>
#include <QClipboard>

#include "qgslogger.h"

// Tree hierachy
//
// layer [userrole: QgsMapLayer]
//   feature: displayfield|displayvalue [userrole: fid]
//     derived attributes
//       name value
//     name value
//     name value
//     name value
//   feature
//     derived attributes
//       name value
//     name value

QgsIdentifyResults::QgsIdentifyResults( QgsMapCanvas *canvas, QWidget *parent, Qt::WFlags f )
    : QDialog( parent, f ),
    mActionPopup( 0 ),
    mRubberBand( 0 ),
    mCanvas( canvas )
{
  setupUi( this );
  lstResults->setColumnCount( 2 );
  setColumnText( 0, tr( "Feature" ) );
  setColumnText( 1, tr( "Value" ) );

  connect( buttonCancel, SIGNAL( clicked() ),
           this, SLOT( close() ) );

  connect( lstResults, SIGNAL( itemExpanded( QTreeWidgetItem* ) ),
           this, SLOT( itemExpanded( QTreeWidgetItem* ) ) );

  connect( lstResults, SIGNAL( currentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ),
           this, SLOT( handleCurrentItemChanged( QTreeWidgetItem*, QTreeWidgetItem* ) ) );
}

QgsIdentifyResults::~QgsIdentifyResults()
{
  delete mActionPopup;
}

QTreeWidgetItem *QgsIdentifyResults::layerItem( QObject *layer )
{
  QTreeWidgetItem *item;

  for ( int i = 0; i < lstResults->topLevelItemCount(); i++ )
  {
    item = lstResults->topLevelItem( i );
    if ( item->data( 0, Qt::UserRole ).value<QObject*>() == layer )
      return item;
  }

  return 0;
}

void QgsIdentifyResults::addFeature( QgsMapLayer *layer, int fid,
                                     QString displayField, QString displayValue,
                                     const QMap<QString, QString> &attributes,
                                     const QMap<QString, QString> &derivedAttributes )
{
  QTreeWidgetItem *item = layerItem( layer );

  if ( item == 0 )
  {
    item = new QTreeWidgetItem( QStringList() << layer->name() << tr( "Layer" ) );
    item->setData( 0, Qt::UserRole, QVariant::fromValue( dynamic_cast<QObject*>( layer ) ) );
    lstResults->addTopLevelItem( item );

    connect( layer, SIGNAL( destroyed() ), this, SLOT( layerDestroyed() ) );

    QgsVectorLayer *vlayer = dynamic_cast<QgsVectorLayer*>( layer );
    if ( vlayer )
      connect( vlayer, SIGNAL( featureDeleted( int ) ), this, SLOT( featureDeleted( int ) ) );
  }

  QTreeWidgetItem *featItem = new QTreeWidgetItem( QStringList() << displayField << displayValue );
  featItem->setData( 0, Qt::UserRole, fid );

  for ( QMap<QString, QString>::const_iterator it = attributes.begin(); it != attributes.end(); it++ )
  {
    featItem->addChild( new QTreeWidgetItem( QStringList() << it.key() << it.value() ) );
  }

  if ( derivedAttributes.size() >= 0 )
  {
    QTreeWidgetItem *derivedItem = new QTreeWidgetItem( QStringList() << tr( "(Derived)" ) );
    featItem->addChild( derivedItem );

    for ( QMap< QString, QString>::const_iterator it = derivedAttributes.begin(); it != derivedAttributes.end(); it++ )
    {
      derivedItem->addChild( new QTreeWidgetItem( QStringList() << it.key() << it.value() ) );
    }
  }

  item->addChild( featItem );
}

// Call to show the dialog box.
void QgsIdentifyResults::show()
{
  // Enfore a few things before showing the dialog box
  lstResults->sortItems( 0, Qt::AscendingOrder );
  expandColumnsToFit();

  QDialog::show();
}
// Slot called when user clicks the Close button
// (saves the current window size/position)
void QgsIdentifyResults::close()
{
  saveWindowLocation();
  done( 0 );
}
// Save the current window size/position before closing
// from window menu or X in titlebar
void QgsIdentifyResults::closeEvent( QCloseEvent *e )
{
  // We'll close in our own good time thanks...
  e->ignore();
  close();
}

// Popup (create if necessary) a context menu that contains a list of
// actions that can be applied to the data in the identify results
// dialog box.

void QgsIdentifyResults::contextMenuEvent( QContextMenuEvent* event )
{
  QTreeWidgetItem *item = lstResults->itemAt( lstResults->viewport()->mapFrom( this, event->pos() ) );
  // if the user clicked below the end of the attribute list, just return
  if ( !item )
    return;

  if ( mActionPopup == 0 )
  {
    QgsVectorLayer *vlayer = vectorLayer( item );
    if ( vlayer == 0 )
      return;

    QgsAttributeAction *actions = vlayer->actions();

    mActionPopup = new QMenu();

    QAction *a;

    if ( vlayer->isEditable() )
    {
      a = mActionPopup->addAction( tr( "Edit feature" ) );
      a->setEnabled( true );
      a->setData( QVariant::fromValue( -4 ) );
    }

    a = mActionPopup->addAction( tr( "Zoom to feature" ) );
    a->setEnabled( true );
    a->setData( QVariant::fromValue( -3 ) );

    a = mActionPopup->addAction( tr( "Copy attribute value" ) );
    a->setEnabled( true );
    a->setData( QVariant::fromValue( -2 ) );

    a = mActionPopup->addAction( tr( "Copy feature attributes" ) );
    a->setEnabled( true );
    a->setData( QVariant::fromValue( -1 ) );

    if ( actions && actions->size() > 0 )
    {
      // The assumption is made that an instance of QgsIdentifyResults is
      // created for each new Identify Results dialog box, and that the
      // contents of the popup menu doesn't change during the time that
      // such a dialog box is around.
      a = mActionPopup->addAction( tr( "Run action" ) );
      a->setEnabled( false );
      mActionPopup->addSeparator();

      QgsAttributeAction::aIter iter = actions->begin();
      for ( int j = 0; iter != actions->end(); ++iter, ++j )
      {
        QAction* a = mActionPopup->addAction( iter->name() );
        // The menu action stores an integer that is used later on to
        // associate an menu action with an actual qgis action.
        a->setData( QVariant::fromValue( j ) );
      }
    }

    connect( mActionPopup, SIGNAL( triggered( QAction* ) ), this, SLOT( popupItemSelected( QAction* ) ) );
  }

  mActionPopup->popup( event->globalPos() );
}

// Restore last window position/size and show the window
void QgsIdentifyResults::restorePosition()
{
  QSettings settings;
  restoreGeometry( settings.value( "/Windows/Identify/geometry" ).toByteArray() );
  show();
}

// Save the current window location (store in ~/.qt/qgisrc)
void QgsIdentifyResults::saveWindowLocation()
{
  QSettings settings;
  settings.setValue( "/Windows/Identify/geometry", saveGeometry() );
}

void QgsIdentifyResults::setColumnText( int column, const QString & label )
{
  QTreeWidgetItem* header = lstResults->headerItem();
  header->setText( column, label );
}

// Run the action that was selected in the popup menu
void QgsIdentifyResults::popupItemSelected( QAction* menuAction )
{
  QTreeWidgetItem *item = lstResults->currentItem();
  if ( item == 0 )
    return;

  int id = menuAction->data().toInt();

  if ( id < 0 )
  {
    if ( id == -4 )
    {
      editFeature( item );
    }
    else if ( id == -3 )
    {
      zoomToFeature( item );
    }
    else
    {
      QClipboard *clipboard = QApplication::clipboard();
      QString text;

      if ( id == -2 )
      {
        text = item->data( 1, Qt::DisplayRole ).toString();
      }
      else
      {
        std::vector< std::pair<QString, QString> > attributes;
        retrieveAttributes( item, attributes );

        for ( std::vector< std::pair<QString, QString> >::iterator it = attributes.begin(); it != attributes.end(); it++ )
        {
          text += QString( "%1: %2\n" ).arg( it->first ).arg( it->second );
        }
      }

      QgsDebugMsg( QString( "set clipboard: %1" ).arg( text ) );
      clipboard->setText( text );
    }
  }
  else
  {
    doAction( item );
  }
}

void QgsIdentifyResults::expandColumnsToFit()
{
  lstResults->resizeColumnToContents( 0 );
  lstResults->resizeColumnToContents( 1 );
}

void QgsIdentifyResults::clear()
{
  lstResults->clear();
  clearRubberBand();
}

void QgsIdentifyResults::activate()
{
  if ( mRubberBand )
  {
    mRubberBand->show();
  }

  if ( lstResults->topLevelItemCount() > 0 )
  {
    show();
    raise();
  }
}

void QgsIdentifyResults::deactivate()
{
  if ( mRubberBand )
  {
    mRubberBand->hide();
  }
}

void QgsIdentifyResults::clearRubberBand()
{
  if ( !mRubberBand )
    return;

  delete mRubberBand;
  mRubberBand = 0;
  mRubberBandLayer = 0;
  mRubberBandFid = 0;
}

void QgsIdentifyResults::doAction( QTreeWidgetItem *item )
{
  std::vector< std::pair<QString, QString> > attributes;
  QTreeWidgetItem *featItem = retrieveAttributes( item, attributes );
  if ( !featItem )
    return;

  int id = featItem->data( 0, Qt::UserRole ).toInt();

  QgsVectorLayer *layer = dynamic_cast<QgsVectorLayer *>( featItem->parent()->data( 0, Qt::UserRole ).value<QObject *>() );
  if ( !layer )
    return;

  layer->actions()->doAction( id, attributes, id );
}

QTreeWidgetItem *QgsIdentifyResults::featureItem( QTreeWidgetItem *item )
{
  QTreeWidgetItem *featItem;
  if ( item->parent() )
  {
    if ( item->parent()->parent() )
    {
      // attribute item
      featItem = item->parent();
    }
    else
    {
      // feature item
      featItem = item;
    }
  }
  else
  {
    // layer item
    if ( item->childCount() > 1 )
      return 0;

    featItem = item->child( 0 );
  }

  return featItem;
}

QgsVectorLayer *QgsIdentifyResults::vectorLayer( QTreeWidgetItem *item )
{
  if ( item->parent() )
  {
    item = featureItem( item )->parent();
  }

  return dynamic_cast<QgsVectorLayer *>( item->data( 0, Qt::UserRole ).value<QObject *>() );
}


QTreeWidgetItem *QgsIdentifyResults::retrieveAttributes( QTreeWidgetItem *item, std::vector< std::pair<QString, QString> > &attributes )
{
  QTreeWidgetItem *featItem = featureItem( item );

  attributes.clear();
  for ( int i = 0; i < featItem->childCount(); i++ )
  {
    QTreeWidgetItem *item = featItem->child( i );
    if ( item->childCount() > 0 )
      continue;
    attributes.push_back( std::make_pair( item->data( 0, Qt::DisplayRole ).toString(), item->data( 1, Qt::DisplayRole ).toString() ) );
  }

  return featItem;
}

void QgsIdentifyResults::on_buttonHelp_clicked()
{
  QgsContextHelp::run( context_id );
}

void QgsIdentifyResults::itemExpanded( QTreeWidgetItem* item )
{
  expandColumnsToFit();
}

void QgsIdentifyResults::handleCurrentItemChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous )
{
  if ( current == NULL )
  {
    emit selectedFeatureChanged( 0, 0 );
    return;
  }

  highlightFeature( current );
}

void QgsIdentifyResults::layerDestroyed()
{
  if ( mRubberBandLayer == sender() )
  {
    clearRubberBand();
  }

  delete layerItem( sender() );

  if ( lstResults->topLevelItemCount() == 0 )
  {
    hide();
  }
}

void QgsIdentifyResults::featureDeleted( int fid )
{
  QTreeWidgetItem *layItem = layerItem( sender() );

  if ( !layItem )
    return;

  for ( int i = 0; i < layItem->childCount(); i++ )
  {
    QTreeWidgetItem *featItem = layItem->child( i );

    if ( featItem && featItem->data( 0, Qt::UserRole ).toInt() == fid )
    {
      if ( mRubberBandLayer == sender() && mRubberBandFid == fid )
        clearRubberBand();
      delete featItem;
      break;
    }
  }

  if ( layItem->childCount() == 0 )
  {
    if ( mRubberBandLayer == sender() )
      clearRubberBand();
    delete layItem;
  }

  if ( lstResults->topLevelItemCount() == 0 )
  {
    hide();
  }
}

void QgsIdentifyResults::highlightFeature( QTreeWidgetItem *item )
{
  QgsVectorLayer *layer = vectorLayer( item );
  if ( !layer )
    return;

  QTreeWidgetItem *featItem = featureItem( item );
  if ( !featItem )
    return;

  int fid = featItem->data( 0, Qt::UserRole ).toInt();

  clearRubberBand();

  QgsFeature feat;
  if ( ! layer->featureAtId( fid, feat, true, false ) )
  {
    return;
  }

  if ( !feat.geometry() )
  {
    return;
  }

  mRubberBand = new QgsRubberBand( mCanvas, feat.geometry()->type() == QGis::Polygon );
  if ( mRubberBand )
  {
    mRubberBandLayer = layer;
    mRubberBandFid = fid;
    mRubberBand->setToGeometry( feat.geometry(), layer );
    mRubberBand->setWidth( 2 );
    mRubberBand->setColor( Qt::red );
    mRubberBand->show();
  }
}

void QgsIdentifyResults::zoomToFeature( QTreeWidgetItem *item )
{
  QgsVectorLayer *layer = vectorLayer( item );
  if ( !layer )
    return;

  QTreeWidgetItem *featItem = featureItem( item );
  if ( !featItem )
    return;

  int fid = featItem->data( 0, Qt::UserRole ).toInt();

  QgsFeature feat;
  if ( ! layer->featureAtId( fid, feat, true, false ) )
  {
    return;
  }

  if ( !feat.geometry() )
  {
    return;
  }

  QgsRectangle rect = mCanvas->mapRenderer()->layerExtentToOutputExtent( layer, feat.geometry()->boundingBox() );

  if ( rect.isEmpty() )
  {
    QgsPoint c = rect.center();
    rect = mCanvas->extent();
    rect.expand( 0.25, &c );
  }

  mCanvas->setExtent( rect );
  mCanvas->refresh();
}


void QgsIdentifyResults::editFeature( QTreeWidgetItem *item )
{
  QgsVectorLayer *layer = vectorLayer( item );
  if ( !layer || !layer->isEditable() )
    return;

  QTreeWidgetItem *featItem = featureItem( item );
  if ( !featItem )
    return;

  int fid = featItem->data( 0, Qt::UserRole ).toInt();

  QgsFeature f;
  if ( ! layer->featureAtId( fid, f ) )
    return;

  QgsAttributeMap src = f.attributeMap();

  layer->beginEditCommand( tr( "Attribute changed" ) );
  QgsAttributeDialog *ad = new QgsAttributeDialog( layer, &f );
  if ( ad->exec() )
  {
    const QgsAttributeMap &dst = f.attributeMap();
    for ( QgsAttributeMap::const_iterator it = dst.begin(); it != dst.end(); it++ )
    {
      if ( !src.contains( it.key() ) || it.value() != src[it.key()] )
      {
        layer->changeAttributeValue( f.id(), it.key(), it.value() );
      }
    }
    layer->endEditCommand();
  }
  else
  {
    layer->destroyEditCommand();
  }

  delete ad;
  mCanvas->refresh();
}
