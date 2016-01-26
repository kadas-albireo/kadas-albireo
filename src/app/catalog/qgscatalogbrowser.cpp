/***************************************************************************
 *  qgscatalogbrowser.cpp                                                  *
 *  ---------------------                                                  *
 *  begin                : January, 2016                                   *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
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

#include "qgisapp.h"
#include "qgscatalogbrowser.h"
#include "qgscatalogprovider.h"
#include "qgsarcgisrestcatalogprovider.h"
#include "qgsgeoadminrestcatalogprovider.h"
#include "qgsfilterlineedit.h"
#include "qgsmimedatautils.h"
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QTreeView>
#include <QVBoxLayout>
#include <QTextStream>
#include <QUrl>

class QgsCatalogBrowser::CatalogModel : public QStandardItemModel
{
  public:
    CatalogModel( QObject* parent = 0 ) : QStandardItemModel( parent )
    {
    }

    QStandardItem* addItem( QStandardItem* parent, const QString& value, bool isLeaf, QMimeData* mimeData )
    {
      if ( !parent )
      {
        parent = invisibleRootItem();
      }
      // Create category group item if necessary
      if ( !isLeaf )
      {
        QStandardItem* groupItem = 0;
        for ( int i = 0, n = parent->rowCount(); i < n; ++i )
        {
          if ( parent->child( i )->text() == value )
          {
            groupItem = parent->child( i );
            break;
          }
        }
        if ( !groupItem )
        {
          groupItem = new QStandardItem( value );
          groupItem->setDragEnabled( false );
          parent->setChild( parent->rowCount(), groupItem );
        }
        return groupItem;
      }
      else
      {
        QStandardItem* item = new QStandardItem( value );
        parent->setChild( parent->rowCount(), item );
        item->setData( QgsMimeDataUtils::decodeUriList( mimeData ).front().data() );
        item->setToolTip( QgsMimeDataUtils::decodeUriList( mimeData ).front().uri );
        return item;
      }
    }

    QMimeData* mimeData( const QModelIndexList &indexes ) const override
    {
      QList<QModelIndex> indexStack;
      QModelIndex index = indexes.front();
      indexStack.prepend( index );
      while ( index.parent().isValid() )
      {
        index = index.parent();
        indexStack.prepend( index );
      }

      QStandardItem* item = itemFromIndex( indexStack.front() );
      if ( item )
      {
        for ( int i = 1, n = indexStack.size(); i < n; ++i )
        {
          item = item->child( indexStack[i].row() );
        }
        QgsMimeDataUtils::Uri uri( item->data().toString() );

        QMimeData* data = QgsMimeDataUtils::encodeUriList( QgsMimeDataUtils::UriList() << uri );
        return data;
      }
      return 0;
    }

    QStringList mimeTypes() const override
    {
      return QStringList() << "text/uri-list" << "application/x-vnd.qgis.qgis.uri";
    }
};

class QgsCatalogBrowser::TreeFilterProxyModel : public QSortFilterProxyModel
{
  public:
    TreeFilterProxyModel( QObject* parent = 0 ) : QSortFilterProxyModel( parent )
    {
    }

    bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const override
    {
      if ( QSortFilterProxyModel::filterAcceptsRow( source_row, source_parent ) )
      {
        return true;
      }
      QModelIndex parent = source_parent;
      while ( parent.isValid() )
      {
        if ( QSortFilterProxyModel::filterAcceptsRow( parent.row(), parent.parent() ) )
        {
          return true;
        }
        parent = parent.parent();
      }
      return acceptsAnyChildren( source_row, source_parent );
    }

  private:
    bool acceptsAnyChildren( int source_row, const QModelIndex &source_parent ) const
    {
      QModelIndex item = sourceModel()->index( source_row, 0, source_parent );
      if ( item.isValid() )
      {
        for ( int i = 0, n = item.model()->rowCount( item ); i < n; ++i )
        {
          if ( QSortFilterProxyModel::filterAcceptsRow( i, item ) || acceptsAnyChildren( i, item ) )
            return true;
        }
      }
      return false;
    }
};

QgsCatalogBrowser::QgsCatalogBrowser( QWidget *parent )
    : QWidget( parent )
{
  setLayout( new QVBoxLayout() );
  layout()->setContentsMargins( 0, 0, 0, 0 );
  layout()->setSpacing( 0 );

  mFilterLineEdit = new QgsFilterLineEdit( this );
  mFilterLineEdit->setPlaceholderText( tr( "Filter catalog..." ) );
  mFilterLineEdit->setObjectName( "mCatalogBrowserLineEdit" );
  mFilterLineEdit->setStyleSheet( "QLineEdit { border-style: solid; border-color: #e4e4e4; border-width: 1px 0px 1px 0px; }" );
  layout()->addWidget( mFilterLineEdit );
  connect( mFilterLineEdit, SIGNAL( textChanged( QString ) ), this, SLOT( filterChanged( QString ) ) );

  mTreeView = new QTreeView( this );
  mTreeView->setFrameShape( QTreeView::NoFrame );
  mTreeView->setEditTriggers( QTreeView::NoEditTriggers );
  mTreeView->setDragEnabled( true );
  layout()->addWidget( mTreeView );
  connect( mTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( itemDoubleClicked( QModelIndex ) ) );
  mCatalogModel = new CatalogModel( this );
  mTreeView->setHeaderHidden( true );

  mFilterProxyModel = new TreeFilterProxyModel( this );

  mLoadingModel = new QStandardItemModel( this );
  QStandardItem* loadingItem = new QStandardItem( tr( "Loading..." ) );
  loadingItem->setEnabled( false );
  mLoadingModel->appendRow( loadingItem );

  QStringList catalogUris = QSettings().value( "/qgis/geodatacatalogs" ).toString().split( ";;" );
  foreach ( const QString& catalogUri, catalogUris )
  {
    QUrl u = QUrl::fromEncoded( "?" + catalogUri.toLocal8Bit() );
    QString type = u.queryItemValue( "type" );
    QString url = u.queryItemValue( "url" );
    if ( type == "geoadmin" )
    {
      addProvider( new QgsGeoAdminRestCatalogProvider( url, this ) );
    }
    else if ( type == "arcgisrest" )
    {
      addProvider( new QgsArcGisRestCatalogProvider( url, this ) );
    }
  }
}

void QgsCatalogBrowser::reload()
{
  if ( mProviders.size() > 0 )
  {
    mCatalogModel->setRowCount( 0 );
    mTreeView->setModel( mLoadingModel );

    mFinishedProviders = 0;
    foreach ( QgsCatalogProvider* provider, mProviders )
    {
      connect( provider, SIGNAL( finished() ), this, SLOT( providerFinished() ) );
      provider->load();
    }
  }
}

void QgsCatalogBrowser::providerFinished()
{
  mFinishedProviders += 1;
  if ( mFinishedProviders == mProviders.size() )
  {
    mCatalogModel->invisibleRootItem()->sortChildren( 0 );
    mFilterProxyModel->setSourceModel( mCatalogModel );
    mTreeView->setModel( mFilterProxyModel );
  }
}

void QgsCatalogBrowser::filterChanged( const QString &text )
{
  mTreeView->clearSelection();
  mFilterProxyModel->setFilterFixedString( text );
  mFilterProxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
  if ( text.length() >= 3 )
  {
    mTreeView->expandAll();
  }
}

void QgsCatalogBrowser::itemDoubleClicked( const QModelIndex &index )
{
  QMimeData* data = mCatalogModel->mimeData( QModelIndexList() << mFilterProxyModel->mapToSource( index ) );
  if ( data )
  {
    QgsMimeDataUtils::UriList uriList = QgsMimeDataUtils::decodeUriList( data );
    if ( !uriList.isEmpty() )
    {
      QgisApp::instance()->addRasterLayer( uriList[0].uri, uriList[0].name, QString( "wms" ) );
    }
    delete data;
  }
}

QStandardItem *QgsCatalogBrowser::addItem( QStandardItem *parent, QString text, bool isLeaf, QMimeData *mimeData )
{
  return mCatalogModel->addItem( parent, text, isLeaf, mimeData );
}
