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
#include "qgsvbslocaldatasearchprovider.h"
#include "qgsvbsremotedatasearchprovider.h"
#include "qgsrubberband.h"
#include <QCheckBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QShortcut>
#include <QStyle>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QImageReader>


const int QgsVBSSearchBox::sEntryTypeRole = Qt::UserRole;
const int QgsVBSSearchBox::sCatNameRole = Qt::UserRole + 1;
const int QgsVBSSearchBox::sCatCountRole = Qt::UserRole + 2;
const int QgsVBSSearchBox::sResultDataRole = Qt::UserRole + 3;


class QgsVBSSearchBox::PopupFrame : public QFrame
{
  public:
    PopupFrame( QgsVBSSearchBox* parent ) : QFrame( parent, Qt::Popup )
    {
      setFrameStyle( QFrame::Box );
      setStyleSheet( "PopupFrame{ background-color: white;}" );
      setFocusPolicy( Qt::NoFocus );
      hide();
    }

  private:
    void closeEvent( QCloseEvent* ) override
    {
      qobject_cast<QgsVBSSearchBox*>( parentWidget() )->cancelSearch();
      parentWidget()->clearFocus();
    }
};

class QgsVBSSearchBox::TreeWidget : public QTreeWidget
{
  public:
    TreeWidget( QWidget* parent = 0 ) : QTreeWidget( parent ) {}
    bool event( QEvent *e ) {  return QTreeWidget::event( e ); }
};


QgsVBSSearchBox::QgsVBSSearchBox( QgisInterface *iface, QWidget *parent )
    : QLineEdit( parent ), mIface( iface )
{
  mNumRunningProviders = 0;
  mRubberBand = 0;

  installEventFilter( this );

  mPopup = new PopupFrame( this );
  mPopup->setLayout( new QVBoxLayout );
  mPopup->layout()->setContentsMargins( 1, 1, 1, 1 );
  mPopup->layout()->setSpacing( 1 );
  mPopup->installEventFilter( this );

  mTreeWidget = new TreeWidget( mPopup );
  mTreeWidget->setFocusPolicy( Qt::NoFocus );
  mTreeWidget->setFrameStyle( QFrame::NoFrame );
  mTreeWidget->setRootIsDecorated( false );
  mTreeWidget->setColumnCount( 1 );
  mTreeWidget->setEditTriggers( QTreeWidget::NoEditTriggers );
  mTreeWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  mTreeWidget->setMouseTracking( true );
  mTreeWidget->setUniformRowHeights( true );
  mTreeWidget->header()->hide();
  mTreeWidget->installEventFilter( this );
  mPopup->layout()->addWidget( mTreeWidget );

  QFrame* line = new QFrame( mPopup );
  line->setFrameShape( QFrame::HLine );
  line->setFrameShadow( QFrame::Sunken );
  mPopup->layout()->addWidget( line );

  mVisibleOnlyCheckBox = new QCheckBox( tr( "Limit to visible area" ), mPopup );
  mVisibleOnlyCheckBox->setFocusPolicy( Qt::NoFocus );
  mPopup->layout()->addWidget( mVisibleOnlyCheckBox );

  mTimer.setSingleShot( true );
  mTimer.setInterval( 500 );

  mSearchButton = new QToolButton( this );
  mSearchButton->setIcon( QIcon( ":/vbsfunctionality/icons/search.svg" ) );
  mSearchButton->setIconSize( QSize( 16, 16 ) );
  mSearchButton->setCursor( Qt::PointingHandCursor );
  mSearchButton->setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  mSearchButton->setToolTip( tr( "Search" ) );

  mClearButton = new QToolButton( this );
  mClearButton->setIcon( QIcon( ":/vbsfunctionality/icons/clear.svg" ) );
  mClearButton->setIconSize( QSize( 16, 16 ) );
  mClearButton->setCursor( Qt::PointingHandCursor );
  mClearButton->setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  mClearButton->setToolTip( tr( "Clear" ) );
  mClearButton->setVisible( false );
  mClearButton->installEventFilter( this );

  connect( this, SIGNAL( textEdited( QString ) ), this, SLOT( textChanged() ) );
  connect( mSearchButton, SIGNAL( clicked() ), this, SLOT( startSearch() ) );
  connect( mVisibleOnlyCheckBox, SIGNAL( toggled( bool ) ), this, SLOT( startSearch() ) );
  connect( &mTimer, SIGNAL( timeout() ), this, SLOT( startSearch() ) );
  connect( mTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( resultSelected() ) );
  connect( mTreeWidget, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( resultActivated() ) );
  connect( mTreeWidget, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT( resultActivated() ) );
  connect( mIface, SIGNAL( newProjectCreated() ), this, SLOT( clearSearch() ) );

  int frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  setStyleSheet( QString( "QLineEdit { padding-right: %1px; } " ).arg( mSearchButton->sizeHint().width() + frameWidth + 5 ) );
  QSize msz = minimumSizeHint();
  setMinimumSize( std::max( msz.width(), mSearchButton->sizeHint().height() + frameWidth * 2 + 2 ),
                  std::max( msz.height(), mSearchButton->sizeHint().height() + frameWidth * 2 + 2 ) );
  setPlaceholderText( tr( "Search" ) );

  qRegisterMetaType<QgsVBSSearchProvider::SearchResult>( "QgsVBSSearchProvider::SearchResult" );
  addSearchProvider( new QgsVBSCoordinateSearchProvider( mIface ) );
  addSearchProvider( new QgsVBSLocationSearchProvider( mIface ) );
  addSearchProvider( new QgsVBSLocalDataSearchProvider( mIface ) );
  addSearchProvider( new QgsVBSRemoteDataSearchProvider( mIface ) );
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
  if ( obj == mClearButton && ev->type() == QEvent::MouseButtonPress )
  {
    clearSearch();
    return true;
  }
  else if ( obj == this && ev->type() == QEvent::MouseButtonPress )
  {
    selectAll();
    return true;
  }
  else if ( ev->type() == QEvent::KeyPress )
  {
    int key = static_cast<QKeyEvent*>( ev )->key();
    if ( key == Qt::Key_Escape )
    {
      mPopup->close();
      return true;
    }
    else if ( key == Qt::Key_Enter || key == Qt::Key_Return )
    {
      if ( mTimer.isActive() )
      {
        // Search text was changed
        startSearch();
        return true;
      }
      else
      {
        return mTreeWidget->event( ev );
      }
    }
    if ( key != Qt::Key_Up && key != Qt::Key_Down &&
         key != Qt::Key_PageUp && key != Qt::Key_PageDown )
    {
      return event( ev );
    }
    else
    {
      return mTreeWidget->event( ev );
    }
  }
  return false;
}

void QgsVBSSearchBox::resizeEvent( QResizeEvent * )
{
  int frameWidth = style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  QSize sz = mSearchButton->sizeHint();
  mSearchButton->move(( rect().right() - frameWidth - sz.width() - 4 ),
                      ( rect().bottom() + 1 - sz.height() ) / 2 );
  sz = mClearButton->sizeHint();
  mClearButton->move(( rect().right() - frameWidth - sz.width() - 4 ),
                     ( rect().bottom() + 1 - sz.height() ) / 2 );
}

void QgsVBSSearchBox::focusInEvent( QFocusEvent * )
{
  mPopup->resize( width(), 200 );
  mPopup->move( mapToGlobal( QPoint( 0, height() ) ) );
  mPopup->show();
  if ( !mClearButton->isVisible() )
    resultSelected();
  selectAll();
}

void QgsVBSSearchBox::textChanged()
{
  mSearchButton->setVisible( true );
  mClearButton->setVisible( false );
  cancelSearch();
  mTimer.start();
}

void QgsVBSSearchBox::startSearch()
{
  mTimer.stop();

  mTreeWidget->blockSignals( true );
  mTreeWidget->clear();
  mTreeWidget->blockSignals( false );

  QString searchtext = text();
  if ( searchtext.size() < 3 )
  {
    return;
  }

  mNumRunningProviders = mSearchProviders.count();

  QgsVBSSearchProvider::SearchRegion searchRegion;
  if ( mVisibleOnlyCheckBox->isChecked() )
  {
    searchRegion.crs = mIface->mapCanvas()->mapSettings().destinationCrs();
    searchRegion.rect = mIface->mapCanvas()->mapSettings().visibleExtent();
  }

  foreach ( QgsVBSSearchProvider* provider, mSearchProviders )
  {
    provider->startSearch( searchtext, searchRegion );
  }
}

void QgsVBSSearchBox::clearSearch()
{
  clear();
  mIface->mapCanvas()->scene()->removeItem( mRubberBand );
  delete mRubberBand;
  mRubberBand = 0;
  mPopup->close();
  mTreeWidget->blockSignals( true );
  mTreeWidget->clear();
  mTreeWidget->blockSignals( false );
}

void QgsVBSSearchBox::searchProviderFinished()
{
  --mNumRunningProviders;
//  if ( mNumRunningProviders == 0 )
//  {
//    mTreeWidget->setUpdatesEnabled( true );
//    mTreeWidget->expandAll();
//  }
}

void QgsVBSSearchBox::searchResultFound( QgsVBSSearchProvider::SearchResult result )
{
  // Search category item
  QTreeWidgetItem* categoryItem = 0;
  QTreeWidgetItem* root = mTreeWidget->invisibleRootItem();
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
    categoryItem->setExpanded( true );
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
  if ( mTreeWidget->currentItem() )
  {
    QTreeWidgetItem* item = mTreeWidget->currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
    {
      return;
    }
    QgsVBSSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<QgsVBSSearchProvider::SearchResult>();
    if ( !mRubberBand )
      createRubberBand();
    QgsCoordinateTransform t( result.crs, mIface->mapCanvas()->mapSettings().destinationCrs() );
    mRubberBand->setToGeometry( QgsGeometry::fromPoint( t.transform( result.pos ) ), 0 );
    blockSignals( true );
    setText( result.text );
    blockSignals( false );
    mSearchButton->setVisible( true );
    mClearButton->setVisible( false );
  }
}

void QgsVBSSearchBox::resultActivated()
{
  if ( mTreeWidget->currentItem() )
  {
    QTreeWidgetItem* item = mTreeWidget->currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
    {
      return;
    }

    QgsVBSSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<QgsVBSSearchProvider::SearchResult>();
    QgsRectangle zoomExtent;
    if ( result.bbox.isEmpty() )
    {
      zoomExtent = mIface->mapCanvas()->mapSettings().computeExtentForScale( result.pos, result.zoomScale, result.crs );
      if ( !mRubberBand )
        createRubberBand();
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
    blockSignals( true );
    setText( result.text );
    blockSignals( false );
    mSearchButton->setVisible( false );
    mClearButton->setVisible( true );
    mPopup->close();
  }
}

void QgsVBSSearchBox::createRubberBand()
{
  mRubberBand = new QgsRubberBand( mIface->mapCanvas(), QGis::Point );
  QSize imgSize = QImageReader( ":/vbsfunctionality/icons/pin_blue.svg" ).size();
  mRubberBand->setSvgIcon( ":/vbsfunctionality/icons/pin_blue.svg", QPoint( -imgSize.width() / 2., -imgSize.height() ) );
}

void QgsVBSSearchBox::cancelSearch()
{
  foreach ( QgsVBSSearchProvider* provider, mSearchProviders )
  {
    provider->cancelSearch();
  }
  // If the clear button is visible, the rubberband marks an activated search
  // result, which can be cleared by pressing the clear button
  if ( mRubberBand && !mClearButton->isVisible() )
  {
    mIface->mapCanvas()->scene()->removeItem( mRubberBand );
    delete mRubberBand;
    mRubberBand = 0;
  }
}