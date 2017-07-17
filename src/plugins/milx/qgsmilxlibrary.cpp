/***************************************************************************
 *  qgsmilxlibrary.cpp                                                     *
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

#include "qgsapplication.h"
#include "qgsmilxlibrary.h"
#include "qgsmilxlayer.h"
#include "qgsfilterlineedit.h"
#include <QDomDocument>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTreeView>
#include <QVBoxLayout>
#include "MilXClient.hpp"

const int QgsMilXLibrary::SymbolXmlRole = Qt::UserRole + 1;
const int QgsMilXLibrary::SymbolMilitaryNameRole = Qt::UserRole + 2;
const int QgsMilXLibrary::SymbolPointCountRole = Qt::UserRole + 3;
const int QgsMilXLibrary::SymbolVariablePointsRole = Qt::UserRole + 4;


class QgsMilXLibrary::TreeFilterProxyModel : public QSortFilterProxyModel
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


QgsMilXLibrary::QgsMilXLibrary( QWidget *parent )
    : QWidget( parent ), mLoader( 0 )
{
  setWindowFlags( Qt::Popup );
  setObjectName( "QgsMilXLibrary" );
  setStyleSheet( "QWidget#QgsMilXLibrary { background-color: white;}" );

  QVBoxLayout* layout = new QVBoxLayout( this );
  layout->setMargin( 2 );
  layout->setSpacing( 2 );
  setLayout( layout );

  mFilterLineEdit = new QgsFilterLineEdit( this );
  mFilterLineEdit->setPlaceholderText( tr( "Filter..." ) );
  layout->addWidget( mFilterLineEdit );
  connect( mFilterLineEdit, SIGNAL( textChanged( QString ) ), this, SLOT( filterChanged( QString ) ) );

  mTreeView = new QTreeView( this );
  mTreeView->setFrameShape( QTreeView::NoFrame );
  mTreeView->setEditTriggers( QTreeView::NoEditTriggers );
  mTreeView->setHeaderHidden( true );
  mTreeView->setIconSize( QSize( 32, 32 ) );
  layout->addWidget( mTreeView );
  connect( mTreeView, SIGNAL( clicked( QModelIndex ) ), this, SLOT( itemClicked( QModelIndex ) ) );

  mGalleryModel = new QStandardItemModel( this );
  mFilterProxyModel = new TreeFilterProxyModel( this );
  mFilterProxyModel->setSourceModel( mGalleryModel );

  mLoadingModel = new QStandardItemModel( this );
  QStandardItem* loadingItem = new QStandardItem( tr( "Loading..." ) );
  loadingItem->setEnabled( false );
  mLoadingModel->appendRow( loadingItem );

  setCursor( Qt::WaitCursor );
  mTreeView->setModel( mLoadingModel );
  mLoader = new QgsMilXLibraryLoader( this );
  connect( mLoader, SIGNAL( finished() ), this, SLOT( loaderFinished() ) );
  mLoader->start();
}

QgsMilXLibrary::~QgsMilXLibrary()
{
  if ( mLoader )
  {
    mLoader->abort();
    while ( mLoader && !mLoader->isFinished() )
    {
      QApplication::instance()->processEvents( QEventLoop::ExcludeUserInputEvents );
    }
  }
}

void QgsMilXLibrary::focusFilter()
{
  mFilterLineEdit->setFocus();
  mFilterLineEdit->selectAll();
}

void QgsMilXLibrary::loaderFinished()
{
  mLoader->deleteLater();
  mLoader = 0;
  mTreeView->setModel( mFilterProxyModel );
  unsetCursor();
}

void QgsMilXLibrary::filterChanged( const QString &text )
{
  mTreeView->clearSelection();
  mFilterProxyModel->setFilterFixedString( text );
  mFilterProxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
  if ( text.length() >= 3 )
  {
    mTreeView->expandAll();
  }
}

void QgsMilXLibrary::itemClicked( const QModelIndex &index )
{

  QModelIndex sourceIndex = mFilterProxyModel->mapToSource( index );
  QList<QModelIndex> indexStack;
  indexStack.prepend( sourceIndex );
  while ( sourceIndex.parent().isValid() )
  {
    sourceIndex = sourceIndex.parent();
    indexStack.prepend( sourceIndex );
  }

  QStandardItem* item = mGalleryModel->itemFromIndex( indexStack.front() );
  if ( item )
  {
    for ( int i = 1, n = indexStack.size(); i < n; ++i )
    {
      item = item->child( indexStack[i].row() );
    }

    QgsMilxSymbolTemplate symbolTemplate;
    symbolTemplate.symbolXml = item->data( SymbolXmlRole ).toString();
    if ( symbolTemplate.symbolXml.isEmpty() )
    {
      return;
    }
    hide();
    if ( symbolTemplate.symbolXml == "<custom>" )
    {
      MilXClient::SymbolDesc desc;
      if ( !MilXClient::createSymbol( symbolTemplate.symbolXml, desc ) )
      {
        return;
      }
      symbolTemplate.symbolMilitaryName = desc.militaryName;
      symbolTemplate.minNPoints = desc.minNumPoints;
      symbolTemplate.hasVariablePoints = desc.hasVariablePoints;
      symbolTemplate.pixmap = QPixmap::fromImage( desc.icon ).scaled( 32, 32, Qt::KeepAspectRatio );
    }
    else
    {
      symbolTemplate.symbolMilitaryName = item->data( SymbolMilitaryNameRole ).toString();
      symbolTemplate.minNPoints = item->data( SymbolPointCountRole ).toInt();
      symbolTemplate.hasVariablePoints = item->data( SymbolVariablePointsRole ).toInt();
      symbolTemplate.pixmap = item->icon().pixmap( item->icon().actualSize( QSize( 32, 32 ) ) );
    }
    emit symbolSelected( symbolTemplate );
  }
}

QStandardItem* QgsMilXLibrary::addItem( QStandardItem* parent, const QString& value, const QImage& image, bool isLeaf, const QString& symbolXml, const QString& symbolMilitaryName, int symbolPointCount , bool symbolHasVariablePoints )
{
  QIcon icon;
  QSize iconSize = isLeaf ? mTreeView->iconSize() : !image.isNull() ? QSize( 32, 32 ) : QSize( 1, 32 );
  QImage iconImage( iconSize, QImage::Format_ARGB32 );
  iconImage.fill( Qt::transparent );
  if ( !image.isNull() )
  {
    double scale = qMin( 1., image.width() > image.height() ? iconImage.width() / double( image.width() ) : iconImage.height() / double( image.height() ) );
    QPainter painter( &iconImage );
    painter.setRenderHint( QPainter::SmoothPixmapTransform );
    painter.drawImage(
      QRectF( 0.5 * ( iconSize.width() - scale * image.width() ), 0.5 * ( iconSize.height() - scale * image.height() ), scale * image.width(), scale * image.height() ),
      image );
  }
  icon = QIcon( QPixmap::fromImage( iconImage ) );
  if ( !parent )
  {
    parent = mGalleryModel->invisibleRootItem();
  }
  // Create category group item if necessary
  if ( !isLeaf )
  {
    // Don't create subgroups with same text as parent
    if ( parent->text() == value )
    {
      return parent;
    }
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
    QStandardItem* item = new QStandardItem( QString( "%1" ).arg( symbolMilitaryName ) );
    parent->setChild( parent->rowCount(), item );
    item->setData( symbolXml, SymbolXmlRole );
    item->setData( symbolMilitaryName, SymbolMilitaryNameRole );
    item->setData( symbolPointCount, SymbolPointCountRole );
    item->setData( symbolHasVariablePoints, SymbolVariablePointsRole );
    item->setToolTip( item->text() );
    item->setIcon( icon );
    return item;
  }
}

///////////////////////////////////////////////////////////////////////////////

void QgsMilXLibraryLoader::run()
{
  QString galleryPath = QDir( QgsApplication::applicationDirPath() ).absoluteFilePath( "MilXGalleryFiles" );
  if ( !QDir( galleryPath ).exists() )
  {
    galleryPath = QSettings().value( "/milx/milx_gallery_path", "" ).toString();
  }
  QString lang = QSettings().value( "/locale/currentLang", "en" ).toString().left( 2 ).toUpper();

  QDir galleryDir( galleryPath );
  if ( galleryDir.exists() )
  {
    foreach ( const QString& galleryFileName, galleryDir.entryList( QStringList() << "*.xml", QDir::Files ) )
    {
      QString galleryFilePath = galleryDir.absoluteFilePath( galleryFileName );
      QFile galleryFile( galleryFilePath );
      if ( !galleryFilePath.endsWith( "_international.xml", Qt::CaseInsensitive ) && galleryFile.open( QIODevice::ReadOnly ) )
      {
        QImage galleryIcon( QString( galleryFilePath ).replace( QRegExp( ".xml$" ), ".png" ) );
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
            QList<MilXClient::SymbolDesc> symbolDescs;
            MilXClient::getSymbolsMetadata( symbolXmls, symbolDescs );
            foreach ( const MilXClient::SymbolDesc& symbolDesc, symbolDescs )
            {
              if ( mAborted )
                return;
              addItem( subSectionItem, symbolDesc.name, symbolDesc.icon, true, symbolDesc.symbolId, symbolDesc.militaryName, symbolDesc.minNumPoints, symbolDesc.hasVariablePoints );
            }
          }
        }
      }
    }
  }
  addItem( 0, tr( "More Symbols..." ), QImage( ":/images/themes/default/mActionAdd.svg" ), true, "<custom>", tr( "More Symbols..." ) );
}

QStandardItem* QgsMilXLibraryLoader::addItem( QStandardItem* parent, const QString& value, const QImage& image, bool isLeaf, const QString& symbolXml, const QString& symbolMilitaryName, int symbolPointCount, bool hasVariablePoints )
{
  QStandardItem* item;
  QMetaObject::invokeMethod( mLibrary, "addItem",
                             Qt::BlockingQueuedConnection,
                             Q_RETURN_ARG( QStandardItem*, item ),
                             Q_ARG( QStandardItem*, parent ),
                             Q_ARG( QString, value ),
                             Q_ARG( QImage, image ),
                             Q_ARG( bool, isLeaf ),
                             Q_ARG( QString, symbolXml ),
                             Q_ARG( QString, symbolMilitaryName ),
                             Q_ARG( int, symbolPointCount ),
                             Q_ARG( bool, hasVariablePoints ) );
  return item;
}
