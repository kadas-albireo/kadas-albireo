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
#include "qgsvbsmilixlayer.h"
#include "qgsfilterlineedit.h"
#include "qgslogger.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include <QAction>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QFile>
#include <QGridLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QToolButton>
#include <QTreeView>
#include "Client/VBSMilixClient.hpp"

const int QgsVBSMilixLibrary::SymbolXmlRole = Qt::UserRole + 1;
const int QgsVBSMilixLibrary::SymbolMilitaryNameRole = Qt::UserRole + 2;
const int QgsVBSMilixLibrary::SymbolPointCountRole = Qt::UserRole + 3;
const int QgsVBSMilixLibrary::SymbolVariablePointsRole = Qt::UserRole + 4;


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


QgsVBSMilixLibrary::QgsVBSMilixLibrary( QgisInterface* iface, QWidget *parent )
    : QDialog( parent ), mIface( iface )
{
  setWindowTitle( tr( "MilX Symbol Gallery" ) );
  QGridLayout* layout = new QGridLayout( this );
  setLayout( layout );

  layout->addWidget( new QLabel( tr( "Layer:" ) ), 0, 0, 1, 1 );
  mLayersCombo = new QComboBox( this );
  connect( mLayersCombo, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setCurrentLayer( int ) ) );
  layout->addWidget( mLayersCombo, 0, 1, 1, 1 );

  QToolButton* newLayerButton = new QToolButton();
  newLayerButton->setIcon( QIcon( ":/images/themes/default/mActionAdd.png" ) );
  connect( newLayerButton, SIGNAL( clicked( bool ) ), this, SLOT( addMilXLayer() ) );
  layout->addWidget( newLayerButton, 0, 2, 1, 1 );

  QFrame* separator = new QFrame( this );
  separator->setFrameShape( QFrame::HLine );
  separator->setFrameShadow( QFrame::Sunken );
  layout->addWidget( separator, 1, 0, 1, 3 );

  mFilterLineEdit = new QgsFilterLineEdit( this );
  mFilterLineEdit->setPlaceholderText( tr( "Filter..." ) );
  layout->addWidget( mFilterLineEdit, 2, 0, 1, 3 );
  connect( mFilterLineEdit, SIGNAL( textChanged( QString ) ), this, SLOT( filterChanged( QString ) ) );

  mTreeView = new QTreeView( this );
  mTreeView->setFrameShape( QTreeView::NoFrame );
  mTreeView->setEditTriggers( QTreeView::NoEditTriggers );
  mTreeView->setHeaderHidden( true );
  mTreeView->setIconSize( QSize( 32, 32 ) );
  layout->addWidget( mTreeView, 3, 0, 1, 3 );
  connect( mTreeView, SIGNAL( clicked( QModelIndex ) ), this, SLOT( itemClicked( QModelIndex ) ) );

  mGalleryModel = new QStandardItemModel( this );
  mFilterProxyModel = new TreeFilterProxyModel( this );
  mFilterProxyModel->setSourceModel( mGalleryModel );
//  mTreeView->setModel( mFilterProxyModel );

  mLoadingModel = new QStandardItemModel( this );
  QStandardItem* loadingItem = new QStandardItem( tr( "Loading..." ) );
  loadingItem->setEnabled( false );
  mLoadingModel->appendRow( loadingItem );

  QDialogButtonBox* buttonBox = new QDialogButtonBox( this );
  buttonBox->addButton( QDialogButtonBox::Close );
  layout->addWidget( buttonBox, 4, 0, 1, 3 );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );

  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersAdded( QList<QgsMapLayer*> ) ), this, SLOT( updateLayers() ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layersRemoved( QStringList ) ), this, SLOT( updateLayers() ) );
  connect( mIface->mapCanvas(), SIGNAL( currentLayerChanged( QgsMapLayer* ) ), this, SLOT( setCurrentLayer( QgsMapLayer* ) ) );

  if ( !VBSMilixClient::init() )
  {
    QMessageBox::critical( this, tr( "MilX initialization failed" ), tr( "Failed to initialize the MilX library: %1" ).arg( VBSMilixClient::lastError() ) );
  }
  else
  {
    setCursor( Qt::WaitCursor );
    mTreeView->setModel( mLoadingModel );
    QgsVBSMilixLibraryLoader* loader = new QgsVBSMilixLibraryLoader( this );
    connect( loader, SIGNAL( finished() ), loader, SLOT( deleteLater() ) );
    connect( loader, SIGNAL( finished() ), this, SLOT( loaderFinished() ) );
    loader->start();
  }
}

void QgsVBSMilixLibrary::autocreateLayer()
{
  if ( mLayersCombo->count() == 0 )
  {
    QgsVBSMilixLayer* layer = new QgsVBSMilixLayer();
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    mIface->mapCanvas()->setCurrentLayer( layer );
  }
}

void QgsVBSMilixLibrary::updateLayers()
{
  // Avoid update while updating
  if ( mLayersCombo->signalsBlocked() )
  {
    return;
  }
  mLayersCombo->blockSignals( true );
  mLayersCombo->clear();
  int idx = 0, current = -1;
  foreach ( QgsMapLayer* layer, QgsMapLayerRegistry::instance()->mapLayers().values() )
  {
    if ( dynamic_cast<QgsVBSMilixLayer*>( layer ) )
    {
      mLayersCombo->addItem( layer->name(), layer->id() );
      if ( mIface->mapCanvas()->currentLayer() == layer )
      {
        current = idx;
      }
      ++idx;
    }
  }
  mLayersCombo->blockSignals( false );
  mLayersCombo->setCurrentIndex( current );
}

void QgsVBSMilixLibrary::loaderFinished()
{
  mTreeView->setModel( mFilterProxyModel );
  unsetCursor();
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

void QgsVBSMilixLibrary::itemClicked( QModelIndex index )
{
  index = mFilterProxyModel->mapToSource( index );
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
    QString symbolInfo = item->data( SymbolMilitaryNameRole ).toString();
    int pointCount = item->data( SymbolPointCountRole ).toInt();
    bool hasVariablePoints = item->data( SymbolVariablePointsRole ).toInt();
    if ( !symbolXml.isEmpty() )
    {
      QgsVBSMilixLayer* layer = static_cast<QgsVBSMilixLayer*>( QgsMapLayerRegistry::instance()->mapLayer( mLayersCombo->itemData( mLayersCombo->currentIndex() ).toString() ) );
      if ( !layer )
      {
        mIface->messageBar()->pushMessage( tr( "No MilX Layer Selected" ), "", QgsMessageBar::WARNING, 5 );
      }
      else
      {
        QgsVBSMapToolMilix* tool = new QgsVBSMapToolMilix( mIface->mapCanvas(), layer, symbolXml, symbolInfo, pointCount, hasVariablePoints, item->icon().pixmap( item->icon().actualSize( QSize( 128, 128 ) ) ) );
        connect( tool, SIGNAL( deactivated() ), tool, SLOT( deleteLater() ) );
        mIface->mapCanvas()->setMapTool( tool );
      }
    }
  }
}

QStandardItem* QgsVBSMilixLibrary::addItem( QStandardItem* parent, const QString& value, const QImage& image, bool isLeaf, const QString& symbolXml, const QString& symbolMilitaryName, int symbolPointCount , bool symbolHasVariablePoints )
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
    item->setData( symbolMilitaryName, SymbolMilitaryNameRole );
    item->setData( symbolPointCount, SymbolPointCountRole );
    item->setData( symbolHasVariablePoints, SymbolVariablePointsRole );
    item->setToolTip( symbolXml );
    item->setIcon( icon );
    return item;
  }
}

void QgsVBSMilixLibrary::setCurrentLayer( int idx )
{
  if ( idx >= 0 )
  {
    mIface->mapCanvas()->setCurrentLayer( QgsMapLayerRegistry::instance()->mapLayer( mLayersCombo->itemData( idx ).toString() ) );
  }
}

void QgsVBSMilixLibrary::setCurrentLayer( QgsMapLayer *layer )
{
  int idx = mLayersCombo->findData( layer->id() );
  if ( idx >= 0 )
  {
    mLayersCombo->blockSignals( true );
    mLayersCombo->setCurrentIndex( idx );
    mLayersCombo->blockSignals( false );
  }
}

void QgsVBSMilixLibrary::addMilXLayer()
{
  QString layerName = QInputDialog::getText( this, tr( "Layer Name" ), tr( "Enter name of new MilX layer:" ) );
  if ( !layerName.isEmpty() )
  {
    QgsVBSMilixLayer* layer = new QgsVBSMilixLayer( layerName );
    QgsMapLayerRegistry::instance()->addMapLayer( layer );
    mLayersCombo->addItem( layer->name(), layer->id() );
    mLayersCombo->setCurrentIndex( mLayersCombo->count() - 1 );
  }
}

void QgsVBSMilixLibraryLoader::run()
{
  QString galleryPath = QSettings().value( "/vbsfunctionality/milix_gallery_path", "/home/sandro/Documents/ownCloud/Shared/Realisierung/Mss-MilX/MssGallery_V1.0_2016.01.30/GalleryFiles/" ).toString();
  QString lang = QSettings().value( "/locale/currentLang", "en" ).toString().left( 2 ).toUpper();

  QDir galleryDir( galleryPath );
  if ( galleryDir.exists() )
  {
    foreach ( const QString& galleryFileName, galleryDir.entryList( QStringList() << "*.xml", QDir::Files ) )
    {
      QString galleryFilePath = galleryDir.absoluteFilePath( galleryFileName );
      QFile galleryFile( galleryFilePath );
      if ( galleryFile.open( QIODevice::ReadOnly ) )
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
            QList<VBSMilixClient::SymbolDesc> symbolDescs;
            VBSMilixClient::getSymbols( symbolXmls, symbolDescs );
            foreach ( const VBSMilixClient::SymbolDesc& symbolDesc, symbolDescs )
            {
              addItem( subSectionItem, symbolDesc.name, symbolDesc.icon, true, symbolDesc.symbolId, symbolDesc.militaryName, symbolDesc.minNumPoints );
            }
          }
        }
      }
    }
  }
}

QStandardItem* QgsVBSMilixLibraryLoader::addItem( QStandardItem* parent, const QString& value, const QImage& image, bool isLeaf, const QString& symbolXml, const QString& symbolMilitaryName, int symbolPointCount )
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
                             Q_ARG( int, symbolPointCount ) );
  return item;
}
