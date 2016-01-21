/***************************************************************************
 *  qgsvbsmilixlibrary.cpp                                                 *
 *  -------------------                                                    *
 *  begin                : Sep 29, 2015                                    *
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

#include "qgsvbsmilixlibrary.h"
#include "qgsvbsmaptoolmilix.h"
#include "qgsfilterlineedit.h"
#include "qgslogger.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include <QAction>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QFile>
#include <QListWidget>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>
#include "Client/VBSMilixClient.hpp"

const int QgsVBSMilixLibrary::SymbolXmlRole = Qt::UserRole + 1;
const int QgsVBSMilixLibrary::SymbolPointCountRole = Qt::UserRole + 2;


class QgsVBSMilixLibrary::TreeFilterProxyModel : public QSortFilterProxyModel
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


QgsVBSMilixLibrary::QgsVBSMilixLibrary( QgisInterface* iface, QgsVBSMilixManager* manager, QWidget *parent )
    : QDialog( parent ), mIface( iface ), mManager( manager )
{
  setWindowTitle( tr( "MilX Symbol Gallery" ) );
  setLayout( new QVBoxLayout( this ) );

  mFilterLineEdit = new QgsFilterLineEdit( this );
  mFilterLineEdit->setPlaceholderText( tr( "Filter..." ) );
  layout()->addWidget( mFilterLineEdit );
  connect( mFilterLineEdit, SIGNAL( textChanged( QString ) ), this, SLOT( filterChanged( QString ) ) );

  mTreeView = new QTreeView( this );
  mTreeView->setFrameShape( QTreeView::NoFrame );
  mTreeView->setEditTriggers( QTreeView::NoEditTriggers );
  mTreeView->setDragEnabled( true );
  mTreeView->setHeaderHidden( true );
  mTreeView->setIconSize( QSize( 32, 32 ) );
  layout()->addWidget( mTreeView );
  connect( mTreeView, SIGNAL( doubleClicked( QModelIndex ) ), this, SLOT( itemDoubleClicked( QModelIndex ) ) );

  mGalleryModel = new QStandardItemModel( this );
  mFilterProxyModel = new TreeFilterProxyModel( this );

  QDialogButtonBox* buttonBox = new QDialogButtonBox( this );
  buttonBox->addButton( QDialogButtonBox::Close );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );

  connect( this, SIGNAL( finished( int ) ), this, SLOT( unsetMilixTool() ) );

  populateLibrary( );
}

void QgsVBSMilixLibrary::populateLibrary( )
{
  setCursor( Qt::WaitCursor );
  QString galleryPath = QSettings().value( "/vbsfunctionality/milix_gallery_path" ).toString();
  QString lang = QSettings().value( "/locale/currentLang", "en" ).toString().left( 2 ).toUpper();

  QDir galleryDir( galleryPath );
  if ( galleryDir.exists() )
  {
    foreach ( const QString& galleryFilePath, galleryDir.entryList( QStringList() << "*.xml", QDir::Files ) )
    {
      QFile galleryFile( galleryFilePath );
      if ( galleryFile.open( QIODevice::ReadOnly ) )
      {
        QIcon galleryIcon( QString( galleryFilePath ).replace( QRegExp( ".xml$" ), ".png" ) );
        QDomDocument doc;
        doc.setContent( &galleryFile );
        QDomElement mssGalleryEl = doc.firstChildElement( "MssGallery" );
        QDomElement galleryNameEl = mssGalleryEl.firstChildElement( QString( "Name_%1" ).arg( lang ) );
        if ( galleryNameEl.isNull() )
        {
          galleryNameEl = mssGalleryEl.firstChildElement( "Name_EN" );
        }
        QStandardItem* galleryItem = addItem( 0, galleryNameEl.text(), galleryIcon );

        QDomNodeList sectionNodes = mssGalleryEl.elementsByTagName( "Section" );
        for ( int iSection = 0, nSections = sectionNodes.size(); iSection < nSections; ++iSection )
        {
          QDomElement sectionEl = sectionNodes.at( iSection ).toElement();
          QDomElement sectionNameEl = sectionEl.firstChildElement( QString( "Name_%1" ).arg( lang ) );
          if ( sectionNameEl.isNull() )
          {
            sectionNameEl = mssGalleryEl.firstChildElement( "Name_EN" );
          }
          QStandardItem* sectionItem = addItem( galleryItem, sectionNameEl.text() );

          QDomNodeList subSectionNodes = sectionEl.elementsByTagName( "SubSection" );
          for ( int iSubSection = 0, nSubSections = subSectionNodes.size(); iSubSection < nSubSections; ++iSubSection )
          {
            QDomElement subSectionEl = subSectionNodes.at( iSubSection ).toElement();
            QDomElement subSectionNameEl = subSectionEl.firstChildElement( QString( "Name_%1" ).arg( lang ) );
            if ( subSectionNameEl.isNull() )
            {
              subSectionNameEl = mssGalleryEl.firstChildElement( "Name_EN" );
            }
            QStandardItem* subSectionItem = addItem( sectionItem, subSectionNameEl.text() );

            QDomNodeList memberNodes = subSectionEl.elementsByTagName( "Member" );
            QStringList symbolXmls;
            for ( int iMember = 0, nMembers = memberNodes.size(); iMember < nMembers; ++iMember )
            {
              symbolXmls.append( memberNodes.at( iMember ).toElement().attribute( "MssStringXML" ) );
            }
            QList<VBSMilixClient::SymbolDesc> symbolDescs;
            VBSMilixClient::getSymbols( symbolXmls, symbolDescs );
            foreach ( const VBSMilixClient::SymbolDesc& symbolDesc, symbolDescs )
            {
              addItem( subSectionItem, symbolDesc.name, symbolDesc.icon, true, symbolDesc.symbolId, symbolDesc.minNumPoints );
            }
          }
        }
      }
    }
  }

  unsetCursor();
}

void QgsVBSMilixLibrary::unsetMilixTool()
{
  delete mCurTool.data();
}


void QgsVBSMilixLibrary::filterChanged( const QString &text )
{
  mTreeView->clearSelection();
  mFilterProxyModel->setFilterFixedString( text );
  mFilterProxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
  if ( text.length() >= 3 )
  {
    mTreeView->expandAll();
  }
}

void QgsVBSMilixLibrary::itemDoubleClicked( QModelIndex index )
{
  QList<QModelIndex> indexStack;
  indexStack.prepend( index );
  while ( index.parent().isValid() )
  {
    index = index.parent();
    indexStack.prepend( index );
  }

  QStandardItem* item = mGalleryModel->itemFromIndex( indexStack.front() );
  if ( item )
  {
    for ( int i = 1, n = indexStack.size(); i < n; ++i )
    {
      item = item->child( indexStack[i].row() );
    }

    QString symbolXml = item->data( SymbolXmlRole ).toString();
    int pointCount = item->data( SymbolPointCountRole ).toInt();
#pragma message("TODO: render a pointer into the preview pixmap")
    mCurTool = new QgsVBSMapToolMilix( mIface->mapCanvas(), mManager, symbolXml, pointCount, item->icon().pixmap( item->icon().actualSize( QSize( 128, 128 ) ) ) );
    connect( mCurTool.data(), SIGNAL( deactivated() ), mCurTool.data(), SLOT( deleteLater() ) );
    mIface->mapCanvas()->setMapTool( mCurTool.data() );
  }
}

QStandardItem* QgsVBSMilixLibrary::addItem( QStandardItem* parent, const QString& value, const QIcon& icon, bool isLeaf, const QString& symbolXml, int symbolPointCount )
{
  if ( !parent )
  {
    parent = mGalleryModel->invisibleRootItem();
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
      groupItem->setIcon( icon );
    }
    return groupItem;
  }
  else
  {
    QStandardItem* item = new QStandardItem( value );
    parent->setChild( parent->rowCount(), item );
    item->setData( symbolXml, SymbolXmlRole );
    item->setData( symbolPointCount, SymbolPointCountRole );
    item->setToolTip( symbolXml );
    item->setIcon( icon );
    return item;
  }
}
