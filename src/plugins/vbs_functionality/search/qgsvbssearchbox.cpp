/***************************************************************************
 *  qgsvbssearchbox.cpp                                                    *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
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
#include "qgsvbssearchbox.h"
#include "qgsmapcanvas.h"
#include "qgscoordinatetransform.h"
#include "qgsvbssearchprovider.h"
#include "qgsvbscoordinatesearchprovider.h"
#include "qgsvbslocationsearchprovider.h"
#include "qgsrubberband.h"
#include <QHeaderView>
#include <QKeyEvent>
#include <QShortcut>
#include <QStyle>


const int QgsVBSSearchBox::sEntryTypeRole = Qt::UserRole;
const int QgsVBSSearchBox::sCatNameRole = Qt::UserRole + 1;
const int QgsVBSSearchBox::sCatCountRole = Qt::UserRole + 2;
const int QgsVBSSearchBox::sResultDataRole = Qt::UserRole + 3;


QgsVBSSearchBox::QgsVBSSearchBox( QgisInterface *iface, QWidget *parent )
    : QLineEdit( parent ), mIface( iface )
{
  mNumRunningProviders = 0;
  mRubberBand = 0;

  mPopup.setColumnCount( 1 );
  mPopup.setEditTriggers( QTreeWidget::NoEditTriggers );
  mPopup.setFocusPolicy( Qt::NoFocus );
  mPopup.setFocusProxy( parent );
  mPopup.setFrameStyle( QFrame::Box );
  mPopup.setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  mPopup.setMouseTracking( true );
  mPopup.setRootIsDecorated( false );
  mPopup.setSelectionBehavior( QTreeWidget::SelectRows );
  mPopup.setUniformRowHeights( true );
  mPopup.setWindowFlags( Qt::Popup );
  mPopup.header()->hide();
  mPopup.installEventFilter( this );

  mTimer.setSingleShot( true );
  mTimer.setInterval( 500 );

  m_searchButton.setParent( this );
  m_searchButton.setIcon( QIcon( ":/vbsfunctionality/icons/search.svg" ) );
  m_searchButton.setIconSize( QSize( 16, 16 ) );
  m_searchButton.setCursor( Qt::PointingHandCursor );
  m_searchButton.setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  m_searchButton.setToolTip( tr( "Search" ) );

  m_clearButton.setParent( this );
  m_clearButton.setIcon( QIcon( ":/vbsfunctionality/icons/clear.svg" ) );
  m_clearButton.setIconSize( QSize( 16, 16 ) );
  m_clearButton.setCursor( Qt::PointingHandCursor );
  m_clearButton.setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  m_clearButton.setToolTip( tr( "Clear" ) );
  m_clearButton.setVisible( false );

  installEventFilter( this );
  connect( this, SIGNAL( textEdited( QString ) ), &mTimer, SLOT( start() ) );
  connect( &m_searchButton, SIGNAL( clicked() ), this, SLOT( startSearch() ) );
  connect( this, SIGNAL( returnPressed() ), this, SLOT( startSearch() ) );
  connect( &mTimer, SIGNAL( timeout() ), this, SLOT( startSearch() ) );
  connect( &mPopup, SIGNAL( itemSelectionChanged() ), this, SLOT( resultSelected() ) );
  connect( &mPopup, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( resultActivated() ) );
  connect( this, SIGNAL( textEdited( QString ) ), this, SLOT( setSearchIcon() ) );
  connect( &m_clearButton, SIGNAL( clicked() ), this, SLOT( clearSearch() ) );

  int frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  setStyleSheet( QString( "QLineEdit { padding-right: %1px; } " ).arg( m_searchButton.sizeHint().width() + frameWidth + 5 ) );
  QSize msz = minimumSizeHint();
  setMinimumSize( std::max( msz.width(), m_searchButton.sizeHint().height() + frameWidth * 2 + 2 ),
                  std::max( msz.height(), m_searchButton.sizeHint().height() + frameWidth * 2 + 2 ) );
  setPlaceholderText( tr( "Search" ) );

  addSearchProvider( new QgsVBSCoordinateSearchProvider(mIface) );
  addSearchProvider( new QgsVBSLocationSearchProvider(mIface) );
}

QgsVBSSearchBox::~QgsVBSSearchBox()
{
  qDeleteAll( mSearchProviders );
}


void QgsVBSSearchBox::addSearchProvider( QgsVBSSearchProvider* provider )
{
  mSearchProviders.append( provider );
  connect( provider, SIGNAL( searchFinished() ), this, SLOT( searchProviderFinished() ) );
  connect( provider, SIGNAL( searchResultFound( QgsVBSSearchProvider::SearchResult ) ), this, SLOT( searchResultFound( QgsVBSSearchProvider::SearchResult ) ) );
}

void QgsVBSSearchBox::removeSearchProvider( QgsVBSSearchProvider* provider )
{
  mSearchProviders.removeAll( provider );
  disconnect( provider, SIGNAL( searchFinished() ), this, SLOT( searchProviderFinished() ) );
  disconnect( provider, SIGNAL( searchResultFound( QgsVBSSearchProvider::SearchResult ) ), this, SLOT( searchResultFound( QgsVBSSearchProvider::SearchResult ) ) );
}

bool QgsVBSSearchBox::eventFilter( QObject* obj, QEvent* ev )
{
  if ( obj == this )
  {
    if ( ev->type() == QEvent::KeyPress && static_cast<QKeyEvent*>( ev )->key() == Qt::Key_Escape )
    {
      setText( "" );
      clearFocus();
      return true;
    }
  }
  else if ( obj == &mPopup )
  {
    if ( ev->type() == QEvent::MouseButtonPress )
    {
      mPopup.hide();
      setFocus();
      return true;
    }
    else if ( ev->type() == QEvent::KeyPress )
    {
      int key = static_cast<QKeyEvent*>( ev )->key();
      if ( key == Qt::Key_Up || key == Qt::Key_Down ||
           key == Qt::Key_Home || key == Qt::Key_End ||
           key == Qt::Key_PageUp || key == Qt::Key_PageDown )
      {
        return false;
      }
      else if ( key == Qt::Key_Enter || key == Qt::Key_Return )
      {
        resultActivated();
      }
      else if ( key == Qt::Key_Escape )
      {
        setFocus();
        mPopup.hide();
      }
      else
      {
        setFocus();
        event( ev );
      }
      return true;
    }
  }
  return false;
}

void QgsVBSSearchBox::resizeEvent( QResizeEvent * )
{
  int frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  QSize sz = m_searchButton.sizeHint();
  m_searchButton.move(( rect().right() - frameWidth - sz.width() - 4 ),
                      ( rect().bottom() + 1 - sz.height() ) / 2 );
  sz = m_clearButton.sizeHint();
  m_clearButton.move(( rect().right() - frameWidth - sz.width() - 4 ),
                     ( rect().bottom() + 1 - sz.height() ) / 2 );
}

void QgsVBSSearchBox::setSearchIcon()
{
  m_clearButton.setVisible( false );
  m_searchButton.setVisible( true );
}

void QgsVBSSearchBox::startSearch()
{
  mTimer.stop();

  if ( mNumRunningProviders > 0 )
  {
  for ( QgsVBSSearchProvider* provider : mSearchProviders )
    {
      provider->cancelSearch();
    }
  }

  QString searchtext = text();
  if ( searchtext.size() < 3 )
  {
    return;
  }

  m_searchButton.setVisible( false );
  m_clearButton.setVisible( true );
  mIface->mapCanvas()->scene()->removeItem( mRubberBand );
  delete mRubberBand;
  mRubberBand = 0;

  mPopup.setUpdatesEnabled( false );
  mPopup.clear();

  mNumRunningProviders = mSearchProviders.count();

for ( QgsVBSSearchProvider* provider : mSearchProviders )
  {
    provider->startSearch( searchtext );
  }
}

void QgsVBSSearchBox::clearSearch()
{
for ( QgsVBSSearchProvider* provider : mSearchProviders )
  {
    provider->cancelSearch();
  }
  clear();
  setSearchIcon();
  mIface->mapCanvas()->scene()->removeItem( mRubberBand );
  delete mRubberBand;
  mRubberBand = 0;
}

void QgsVBSSearchBox::searchProviderFinished()
{
  --mNumRunningProviders;
  if ( mNumRunningProviders == 0 )
  {
    mPopup.setUpdatesEnabled( true );
    int w = std::max( width(), mPopup.width() );
    mPopup.resize( w, 200 );
    mPopup.move( mapToGlobal( QPoint( 0, height() ) ) );
    mPopup.setFocus();
    mPopup.expandAll();
    mPopup.show();
  }
}

void QgsVBSSearchBox::searchResultFound( QgsVBSSearchProvider::SearchResult result )
{
  // Search category item
  QTreeWidgetItem* categoryItem = 0;
  QTreeWidgetItem* root = mPopup.invisibleRootItem();
  for ( int i = 0, n = root->childCount(); i < n; ++i )
  {
    if ( root->child( i )->data( 0, sCatNameRole ).toString() == result.category )
    {
      categoryItem = root->child( i );
    }
  }

  // If category does not exist, create it
  if ( !categoryItem )
  {
    int pos = 0;
    for ( int i = 0, n = root->childCount(); i < n; ++i )
    {
      if ( result.category.compare( root->child( i )->data( 0, sCatNameRole ).toString() ) < 0 )
      {
        break;
      }
      ++pos;
    }
    categoryItem = new QTreeWidgetItem();
    categoryItem->setData( 0, sEntryTypeRole, EntryTypeCategory );
    categoryItem->setData( 0, sCatNameRole, result.category );
    categoryItem->setData( 0, sCatCountRole, 0 );
    categoryItem->setFlags( Qt::ItemIsEnabled );
    QFont font = categoryItem->font( 0 );
    font.setBold( true );
    categoryItem->setFont( 0, font );
    root->insertChild( pos, categoryItem );
  }

  // Insert new result
  QTreeWidgetItem* resultItem = new QTreeWidgetItem();
  resultItem->setData( 0, Qt::DisplayRole, result.text );
  resultItem->setData( 0, sEntryTypeRole, EntryTypeResult );
  resultItem->setData( 0, sResultDataRole, QVariant::fromValue( result ) );
  resultItem->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );

  categoryItem->addChild( resultItem );
  int categoryCount = categoryItem->data( 0, sCatCountRole ).toInt() + 1;
  categoryItem->setData( 0, sCatCountRole, categoryCount );
  categoryItem->setData( 0, Qt::DisplayRole, QString( "%1 (%2)" ).arg( categoryItem->data( 0, sCatNameRole ).toString() ).arg( categoryCount ) );
}

void QgsVBSSearchBox::resultSelected()
{
  // Nothing ATM
}

void QgsVBSSearchBox::resultActivated()
{
  if ( mPopup.isVisible() && mPopup.currentItem() )
  {
    QTreeWidgetItem* item = mPopup.currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
    {
      return;
    }

    QgsVBSSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<QgsVBSSearchProvider::SearchResult>();
    QgsRectangle zoomExtent;
    if ( result.bbox.isEmpty() )
    {
      zoomExtent = mIface->mapCanvas()->mapSettings().computeExtentForScale( result.pos, result.zoomScale, result.crs );
      mRubberBand = new QgsRubberBand( mIface->mapCanvas(), QGis::Point );
      mRubberBand->setColor( Qt::yellow );
      mRubberBand->setBorderColor( Qt::red );
      mRubberBand->setIcon( QgsRubberBand::ICON_CIRCLE );
      mRubberBand->setIconSize( 15 );
      mRubberBand->setWidth( 4 );
      QgsCoordinateTransform t( result.crs, mIface->mapCanvas()->mapSettings().destinationCrs() );
      mRubberBand->setToGeometry( QgsGeometry::fromPoint( t.transform( result.pos ) ), 0 );
    }
    else
    {
      QgsCoordinateTransform t( result.crs, mIface->mapCanvas()->mapSettings().destinationCrs() );
      zoomExtent = t.transform( result.bbox );
    }
    mIface->mapCanvas()->setExtent( zoomExtent );
    mIface->mapCanvas()->refresh();
    mPopup.hide();
  }
}
