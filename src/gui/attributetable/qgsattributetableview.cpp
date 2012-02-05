/***************************************************************************
     QgsAttributeTableView.cpp
     --------------------------------------
    Date                 : Feb 2009
    Copyright            : (C) 2009 Vita Cizek
    Email                : weetya (at) gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QKeyEvent>
#include <QSettings>
#include <QHeaderView>
#include <QMenu>

#include "qgsattributetableview.h"
#include "qgsattributetablemodel.h"
#include "qgsattributetablememorymodel.h"
#include "qgsattributetabledelegate.h"
#include "qgsattributetablefiltermodel.h"
#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"

QgsAttributeTableView::QgsAttributeTableView( QWidget *parent )
    : QTableView( parent ), mModel( 0 ), mFilterModel( 0 ), mActionPopup( 0 )
{
  QSettings settings;
  restoreGeometry( settings.value( "/BetterAttributeTable/geometry" ).toByteArray() );

  verticalHeader()->setDefaultSectionSize( 20 );
  horizontalHeader()->setHighlightSections( false );

  setItemDelegate( new QgsAttributeTableDelegate( this ) );

  setSelectionBehavior( QAbstractItemView::SelectRows );
  setSelectionMode( QAbstractItemView::NoSelection );
  setSortingEnabled( true );
}

void QgsAttributeTableView::setCanvasAndLayer( QgsMapCanvas *canvas, QgsVectorLayer *layer )
{
  if ( !layer )
  {
    setModel( 0 );
    delete mModel;
    mModel = 0;
    delete mFilterModel;
    mFilterModel = 0;
    return;
  }

  QgsAttributeTableModel* oldModel = mModel;
  QgsAttributeTableFilterModel* filterModel = mFilterModel;

  // in case the provider allows fast access to features
  // we will use the model that calls featureAtId() to fetch only the
  // features in the current view. Otherwise we'll have to store
  // everything in the memory because using featureAtId() would be too slow
  if ( layer->dataProvider()->capabilities() & QgsVectorDataProvider::SelectAtId )
  {
    QgsDebugMsg( "SelectAtId supported" );
    mModel = new QgsAttributeTableModel( canvas, layer );
  }
  else
  {
    QgsDebugMsg( "SelectAtId NOT supported" );
    mModel = new QgsAttributeTableMemoryModel( canvas, layer );
  }

  connect( mModel, SIGNAL( progress( int, bool& ) ), this, SIGNAL( progress( int, bool& ) ) );
  connect( mModel, SIGNAL( finished() ), this, SIGNAL( finished() ) );
  mModel->loadLayer();

  mFilterModel = new QgsAttributeTableFilterModel( mModel->layer() );
  mFilterModel->setSourceModel( mModel );
  setModel( mFilterModel );

  delete oldModel;
  delete filterModel;
}

QgsAttributeTableView::~QgsAttributeTableView()
{
  delete mModel;
  delete mFilterModel;
  delete mActionPopup;
}

void QgsAttributeTableView::closeEvent( QCloseEvent *e )
{
  Q_UNUSED( e );
  QSettings settings;
  settings.setValue( "/BetterAttributeTable/geometry", QVariant( saveGeometry() ) );
}

void QgsAttributeTableView::contextMenuEvent( QContextMenuEvent *event )
{
  if ( mActionPopup )
  {
    delete mActionPopup;
    mActionPopup = 0;
  }

  QModelIndex idx = indexAt( event->pos() );
  if ( !idx.isValid() )
  {
    return;
  }

  QgsVectorLayer *vlayer = mModel->layer();
  if ( !vlayer )
    return;

  mActionPopup = new QMenu();

  // let some other parts of the application add some actions
  emit willShowContextMenu( mActionPopup, idx );

  if ( mActionPopup->actions().count() > 0 )
  {
    mActionPopup->popup( event->globalPos() );
  }
}
