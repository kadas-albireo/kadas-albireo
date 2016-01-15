/***************************************************************************
 *  qgssearchbox.cpp                                                    *
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
#include "qgssearchbox.h"
#include "qgscoordinatetransform.h"
#include "qgscrscache.h"
#include "qgsgeometryrubberband.h"
#include "qgsmapcanvas.h"
#include "qgsmaptooldrawshape.h"
#include "qgsproject.h"
#include "qgssearchprovider.h"
#include "qgscoordinatesearchprovider.h"
#include "qgslocationsearchprovider.h"
#include "qgslocaldatasearchprovider.h"
#include "qgsremotedatasearchprovider.h"
#include "qgsworldlocationsearchprovider.h"
#include "qgsrubberband.h"
#include <QCheckBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QShortcut>
#include <QStyle>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QImageReader>


const int QgsSearchBox::sEntryTypeRole = Qt::UserRole;
const int QgsSearchBox::sCatNameRole = Qt::UserRole + 1;
const int QgsSearchBox::sCatPrecedenceRole = Qt::UserRole + 2;
const int QgsSearchBox::sCatCountRole = Qt::UserRole + 3;
const int QgsSearchBox::sResultDataRole = Qt::UserRole + 4;

// Overridden to make event() public
class QgsSearchBox::LineEdit : public QLineEdit
{
  public:
    LineEdit( QWidget* parent ) : QLineEdit( parent ) {}
    bool event( QEvent *e ) override { return QLineEdit::event( e ); }
};


// Overridden to make event() public
class QgsSearchBox::TreeWidget: public QTreeWidget
{
  public:
    TreeWidget( QWidget* parent ) : QTreeWidget( parent ) {}
    bool event( QEvent *e ) override { return QTreeWidget::event( e ); }
};


QgsSearchBox::QgsSearchBox( QWidget *parent )
    : QWidget( parent )
{ }

void QgsSearchBox::init( QgsMapCanvas *canvas )
{
  mMapCanvas = canvas;
  mNumRunningProviders = 0;
  mRubberBand = 0;
  mFilterTool = 0;

  mSearchBox = new LineEdit( this );
  mSearchBox->setObjectName( "searchBox" );
  mSearchBox->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );

  mTreeWidget = new TreeWidget( mSearchBox );
  mTreeWidget->setWindowFlags( Qt::Popup );
  mTreeWidget->setFocusPolicy( Qt::NoFocus );
  mTreeWidget->setFrameStyle( QFrame::Box );
  mTreeWidget->setRootIsDecorated( false );
  mTreeWidget->setColumnCount( 1 );
  mTreeWidget->setEditTriggers( QTreeWidget::NoEditTriggers );
  mTreeWidget->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  mTreeWidget->setMouseTracking( true );
  mTreeWidget->setUniformRowHeights( true );
  mTreeWidget->header()->hide();
  mTreeWidget->hide();

  mTimer.setSingleShot( true );
  mTimer.setInterval( 500 );

  mSearchButton = new QToolButton( mSearchBox );
  mSearchButton->setIcon( QIcon( ":/images/themes/default/search.svg" ) );
  mSearchButton->setIconSize( QSize( 16, 16 ) );
  mSearchButton->setCursor( Qt::PointingHandCursor );
  mSearchButton->setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  mSearchButton->setToolTip( tr( "Search" ) );

  mClearButton = new QToolButton( mSearchBox );
  mClearButton->setIcon( QIcon( ":/images/themes/default/clear.svg" ) );
  mClearButton->setIconSize( QSize( 16, 16 ) );
  mClearButton->setCursor( Qt::PointingHandCursor );
  mClearButton->setStyleSheet( "QToolButton { border: none; padding: 0px; }" );
  mClearButton->setToolTip( tr( "Clear" ) );
  mClearButton->setVisible( false );
  mClearButton->installEventFilter( this );

  QMenu* filterMenu = new QMenu( mSearchBox );
  QActionGroup* filterActionGroup = new QActionGroup( filterMenu );
  QAction* noFilterAction = new QAction( QIcon( ":/images/themes/default/search_filter_none.svg" ), tr( "No filter" ), filterMenu );
  filterActionGroup->addAction( noFilterAction );
  connect( noFilterAction, SIGNAL( triggered( bool ) ), this, SLOT( clearFilter() ) );

  QAction* circleFilterAction = new QAction( QIcon( ":/images/themes/default/search_filter_circle.svg" ), tr( "Filter by radius" ), filterMenu );
  circleFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterCircle ) ) );
  filterActionGroup->addAction( circleFilterAction );
  connect( circleFilterAction, SIGNAL( triggered( bool ) ), this, SLOT( setFilterTool() ) );

  QAction* rectangleFilterAction = new QAction( QIcon( ":/images/themes/default/search_filter_rect.svg" ), tr( "Filter by rectangle" ), filterMenu );
  rectangleFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterRect ) ) );
  filterActionGroup->addAction( rectangleFilterAction );
  connect( rectangleFilterAction, SIGNAL( triggered( bool ) ), this, SLOT( setFilterTool() ) );

  QAction* polygonFilterAction = new QAction( QIcon( ":/images/themes/default/search_filter_poly.svg" ), tr( "Filter by polygon" ), filterMenu );
  polygonFilterAction->setData( QVariant::fromValue( static_cast<int>( FilterPoly ) ) );
  filterActionGroup->addAction( polygonFilterAction );
  connect( polygonFilterAction, SIGNAL( triggered( bool ) ), this, SLOT( setFilterTool() ) );

  filterMenu->addActions( QList<QAction*>() << noFilterAction << circleFilterAction << rectangleFilterAction << polygonFilterAction );

  mFilterButton = new QToolButton( this );
  mFilterButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::MinimumExpanding );
  mFilterButton->setObjectName( "searchFilter" );
  mFilterButton->setDefaultAction( noFilterAction );
  mFilterButton->setIconSize( QSize( 16, 16 ) );
  mFilterButton->setPopupMode( QToolButton::InstantPopup );
  mFilterButton->setCursor( Qt::PointingHandCursor );
  mFilterButton->setToolTip( tr( "Select Filter" ) );
  mFilterButton->setMenu( filterMenu );
  connect( filterMenu, SIGNAL( triggered( QAction* ) ), mFilterButton, SLOT( setDefaultAction( QAction* ) ) );

  setLayout( new QHBoxLayout );
  layout()->addWidget( mSearchBox );
  layout()->addWidget( mFilterButton );
  layout()->setContentsMargins( 0, 5, 0, 5 );
  layout()->setSpacing( 0 );

  connect( mSearchBox, SIGNAL( textEdited( QString ) ), this, SLOT( textChanged() ) );
  connect( mSearchButton, SIGNAL( clicked() ), this, SLOT( startSearch() ) );
  connect( &mTimer, SIGNAL( timeout() ), this, SLOT( startSearch() ) );
  connect( mTreeWidget, SIGNAL( itemSelectionChanged() ), this, SLOT( resultSelected() ) );
  connect( mTreeWidget, SIGNAL( itemClicked( QTreeWidgetItem*, int ) ), this, SLOT( resultActivated() ) );
  connect( mTreeWidget, SIGNAL( itemActivated( QTreeWidgetItem*, int ) ), this, SLOT( resultActivated() ) );
#pragma message( "warning: TODO -> app?" )
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( clearSearch() ) );

  int frameWidth = mSearchBox->style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
  mSearchBox->setStyleSheet( QString( "QLineEdit { padding-right: %1px; } " ).arg( mSearchButton->sizeHint().width() + frameWidth + 5 ) );
  QSize msz = mSearchBox->minimumSizeHint();
  mSearchBox->setMinimumSize( std::max( msz.width(), mSearchButton->sizeHint().height() + frameWidth * 2 + 2 ),
                              std::max( msz.height(), mSearchButton->sizeHint().height() + frameWidth * 2 + 2 ) );
  mSearchBox->setPlaceholderText( tr( "Search for Places, Coordinates, Adresses, ..." ) );

  qRegisterMetaType<QgsSearchProvider::SearchResult>( "QgsSearchProvider::SearchResult" );
  addSearchProvider( new QgsCoordinateSearchProvider( mMapCanvas ) );
  addSearchProvider( new QgsLocationSearchProvider( mMapCanvas ) );
  addSearchProvider( new QgsLocalDataSearchProvider( mMapCanvas ) );
  addSearchProvider( new QgsRemoteDataSearchProvider( mMapCanvas ) );
  addSearchProvider( new QgsWorldLocationSearchProvider( mMapCanvas ) );

  mSearchBox->installEventFilter( this );
  mTreeWidget->installEventFilter( this );
}

QgsSearchBox::~QgsSearchBox()
{
  qDeleteAll( mSearchProviders );
}

void QgsSearchBox::addSearchProvider( QgsSearchProvider* provider )
{
  mSearchProviders.append( provider );
  connect( provider, SIGNAL( searchFinished() ), this, SLOT( searchProviderFinished() ) );
  connect( provider, SIGNAL( searchResultFound( QgsSearchProvider::SearchResult ) ), this, SLOT( searchResultFound( QgsSearchProvider::SearchResult ) ) );
}

void QgsSearchBox::removeSearchProvider( QgsSearchProvider* provider )
{
  mSearchProviders.removeAll( provider );
  disconnect( provider, SIGNAL( searchFinished() ), this, SLOT( searchProviderFinished() ) );
  disconnect( provider, SIGNAL( searchResultFound( QgsSearchProvider::SearchResult ) ), this, SLOT( searchResultFound( QgsSearchProvider::SearchResult ) ) );
}

bool QgsSearchBox::eventFilter( QObject* obj, QEvent* ev )
{
  if ( obj == mSearchBox && ev->type() == QEvent::FocusIn )
  {
    mTreeWidget->resize( mSearchBox->width(), 200 );
    mTreeWidget->move( mSearchBox->mapToGlobal( QPoint( 0, mSearchBox->height() ) ) );
    mTreeWidget->show();
    if ( !mClearButton->isVisible() )
      resultSelected();
    if ( mFilterTool )
      mFilterTool->getRubberBand()->setVisible( true );
    return true;
  }
  else if ( obj == mSearchBox && ev->type() == QEvent::MouseButtonPress )
  {
    mSearchBox->selectAll();
    return true;
  }
  else if ( obj == mSearchBox && ev->type() == QEvent::Resize )
  {
    int frameWidth = mSearchBox->style()->pixelMetric( QStyle::PM_DefaultFrameWidth );
    QRect r = mSearchBox->rect();
    QSize sz = mSearchButton->sizeHint();
    mSearchButton->move(( r.right() - frameWidth - sz.width() - 4 ),
                        ( r.bottom() + 1 - sz.height() ) / 2 );
    sz = mClearButton->sizeHint();
    mClearButton->move(( r.right() - frameWidth - sz.width() - 4 ),
                       ( r.bottom() + 1 - sz.height() ) / 2 );
    return true;
  }
  else if ( obj == mClearButton && ev->type() == QEvent::MouseButtonPress )
  {
    clearSearch();
    return true;
  }
  else if ( obj == mTreeWidget && ev->type() == QEvent::Close )
  {
    cancelSearch();
    mSearchBox->clearFocus();
    if ( mFilterTool )
      mFilterTool->getRubberBand()->setVisible( false );
    return true;
  }
  else if ( obj == mTreeWidget && ev->type() == QEvent::MouseButtonPress )
  {
    mTreeWidget->close();
    return true;
  }
  else if ( obj == mTreeWidget && ev->type() == QEvent::KeyPress )
  {
    int key = static_cast<QKeyEvent*>( ev )->key();
    if ( key == Qt::Key_Escape )
    {
      mTreeWidget->close();
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
    }
    else if ( key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_PageUp || key == Qt::Key_PageDown )
      return mTreeWidget->event( ev );
    else
      return mSearchBox->event( ev );
  }
  else
  {
    return QWidget::eventFilter( obj, ev );
  }
  return false;
}

void QgsSearchBox::textChanged()
{
  mSearchButton->setVisible( true );
  mClearButton->setVisible( false );
  cancelSearch();
  mTimer.start();
}

void QgsSearchBox::startSearch()
{
  mTimer.stop();

  mTreeWidget->blockSignals( true );
  mTreeWidget->clear();
  mTreeWidget->blockSignals( false );

  QString searchtext = mSearchBox->text();
  if ( searchtext.size() < 3 )
    return;

  mNumRunningProviders = mSearchProviders.count();

  QgsSearchProvider::SearchRegion searchRegion;
  if ( mFilterTool )
  {
    QgsPolygon poly = QgsGeometry( mFilterTool->getRubberBand()->geometry()->clone() ).asPolygon();
    if ( !poly.isEmpty() )
    {
      searchRegion.polygon = poly.front();
      searchRegion.crs = mMapCanvas->mapSettings().destinationCrs().authid();
    }
  }

  foreach ( QgsSearchProvider* provider, mSearchProviders )
    provider->startSearch( searchtext, searchRegion );
}

void QgsSearchBox::clearSearch()
{
  mSearchBox->clear();
  mSearchButton->setVisible( true );
  mClearButton->setVisible( false );
  mMapCanvas->scene()->removeItem( mRubberBand );
  delete mRubberBand;
  mRubberBand = 0;
  mTreeWidget->close();
  mTreeWidget->blockSignals( true );
  mTreeWidget->clear();
  mTreeWidget->blockSignals( false );
}

void QgsSearchBox::searchProviderFinished()
{
  --mNumRunningProviders;
}

void QgsSearchBox::searchResultFound( QgsSearchProvider::SearchResult result )
{
  // Search category item
  QTreeWidgetItem* categoryItem = 0;
  QTreeWidgetItem* root = mTreeWidget->invisibleRootItem();
  for ( int i = 0, n = root->childCount(); i < n; ++i )
  {
    if ( root->child( i )->data( 0, sCatNameRole ).toString() == result.category )
      categoryItem = root->child( i );
  }

  // If category does not exist, create it
  if ( !categoryItem )
  {
    int pos = 0;
    for ( int i = 0, n = root->childCount(); i < n; ++i )
    {
      if ( result.categoryPrecedence < root->child( i )->data( 0, sCatPrecedenceRole ).toInt() )
      {
        break;
      }
      ++pos;
    }
    categoryItem = new QTreeWidgetItem();
    categoryItem->setData( 0, sEntryTypeRole, EntryTypeCategory );
    categoryItem->setData( 0, sCatNameRole, result.category );
    categoryItem->setData( 0, sCatPrecedenceRole, result.categoryPrecedence );
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

void QgsSearchBox::resultSelected()
{
  if ( mTreeWidget->currentItem() )
  {
    QTreeWidgetItem* item = mTreeWidget->currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
      return;

    QgsSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<QgsSearchProvider::SearchResult>();
    if ( !mRubberBand )
      createRubberBand();
    const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( result.crs, mMapCanvas->mapSettings().destinationCrs().authid() );
    mRubberBand->setToGeometry( QgsGeometry::fromPoint( t->transform( result.pos ) ), 0 );
    mSearchBox->blockSignals( true );
    mSearchBox->setText( result.text );
    mSearchBox->blockSignals( false );
    mSearchButton->setVisible( true );
    mClearButton->setVisible( false );
  }
}

void QgsSearchBox::resultActivated()
{
  if ( mTreeWidget->currentItem() )
  {
    QTreeWidgetItem* item = mTreeWidget->currentItem();
    if ( item->data( 0, sEntryTypeRole ) != EntryTypeResult )
      return;

    QgsSearchProvider::SearchResult result = item->data( 0, sResultDataRole ).value<QgsSearchProvider::SearchResult>();
    QgsRectangle zoomExtent;
    if ( result.bbox.isEmpty() )
    {
      zoomExtent = mMapCanvas->mapSettings().computeExtentForScale( result.pos, result.zoomScale, QgsCRSCache::instance()->crsByAuthId( result.crs ) );
      if ( !mRubberBand )
        createRubberBand();
      const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( result.crs, mMapCanvas->mapSettings().destinationCrs().authid() );
      mRubberBand->setToGeometry( QgsGeometry::fromPoint( t->transform( result.pos ) ), 0 );
    }
    else
    {
      const QgsCoordinateTransform* t = QgsCoordinateTransformCache::instance()->transform( result.crs, mMapCanvas->mapSettings().destinationCrs().authid() );
      zoomExtent = t->transform( result.bbox );
    }
    mMapCanvas->setExtent( zoomExtent );
    mMapCanvas->refresh();
    mSearchBox->blockSignals( true );
    mSearchBox->setText( result.text );
    mSearchBox->blockSignals( false );
    mSearchButton->setVisible( false );
    mClearButton->setVisible( true );
    mTreeWidget->close();
  }
}

void QgsSearchBox::createRubberBand()
{
  mRubberBand = new QgsRubberBand( mMapCanvas, QGis::Point );
  QSize imgSize = QImageReader( ":/images/themes/default/pin_blue.svg" ).size();
  mRubberBand->setSvgIcon( ":/images/themes/default/pin_blue.svg", QPoint( -imgSize.width() / 2., -imgSize.height() ) );
}

void QgsSearchBox::cancelSearch()
{
  foreach ( QgsSearchProvider* provider, mSearchProviders )
  {
    provider->cancelSearch();
  }
  // If the clear button is visible, the rubberband marks an activated search
  // result, which can be cleared by pressing the clear button
  if ( mRubberBand && !mClearButton->isVisible() )
  {
    mMapCanvas->scene()->removeItem( mRubberBand );
    delete mRubberBand;
    mRubberBand = 0;
  }
}

void QgsSearchBox::clearFilter()
{
  if ( mFilterTool != 0 )
  {
    delete mFilterTool;
    mFilterTool = 0;
    // Trigger a new search since the filter changed
    startSearch();
  }
}

void QgsSearchBox::setFilterTool()
{
  QAction* action = qobject_cast<QAction*>( QObject::sender() );
  FilterType filterType = static_cast<FilterType>( action->data().toInt() );
  delete mFilterTool;
  switch ( filterType )
  {
    case FilterRect:
      mFilterTool = new QgsMapToolDrawRectangle( mMapCanvas ); break;
    case FilterPoly:
      mFilterTool = new QgsMapToolDrawPolyLine( mMapCanvas, true ); break;
    case FilterCircle:
      mFilterTool = new QgsMapToolDrawCircle( mMapCanvas ); break;
  }
  mMapCanvas->setMapTool( mFilterTool );
  action->setCheckable( true );
  action->setChecked( true );
  connect( mFilterTool, SIGNAL( finished() ), this, SLOT( filterToolFinished() ) );
}

void QgsSearchBox::filterToolFinished()
{
  mFilterButton->defaultAction()->setChecked( false );
  mFilterButton->defaultAction()->setCheckable( false );
  if ( mFilterTool )
  {
    mSearchBox->setFocus();
    mSearchBox->selectAll();
    // Trigger a new search since the filter changed
    startSearch();
  }
  else
  {
    mFilterButton->setDefaultAction( mFilterButton->menu()->actions().first() );
    clearFilter();
  }
}
