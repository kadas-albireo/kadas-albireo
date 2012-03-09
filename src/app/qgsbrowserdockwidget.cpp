#include "qgsbrowserdockwidget.h"

#include <QHeaderView>
#include <QTreeView>
#include <QMenu>
#include <QSettings>
#include <QToolButton>

#include "qgsbrowsermodel.h"
#include "qgsdataitem.h"
#include "qgslogger.h"
#include "qgsmaplayerregistry.h"
#include "qgsrasterlayer.h"
#include "qgsvectorlayer.h"
#include "qgisapp.h"

#include <QDragEnterEvent>
/**
Utility class for correct drag&drop handling.

We want to allow user to drag layers to qgis window. At the same time we do not
accept drops of the items on our view - but if we ignore the drag enter action
then qgis application consumes the drag events and it is possible to drop the
items on the tree view although the drop is actually managed by qgis app.
 */
class QgsBrowserTreeView : public QTreeView
{
  public:
    QgsBrowserTreeView( QWidget* parent ) : QTreeView( parent )
    {
      setDragDropMode( QTreeView::DragDrop ); // sets also acceptDrops + dragEnabled
      setSelectionMode( QAbstractItemView::ExtendedSelection );
      setContextMenuPolicy( Qt::CustomContextMenu );
      setHeaderHidden( true );
      setDropIndicatorShown( true );
    }

    void dragEnterEvent( QDragEnterEvent* e )
    {
      // accept drag enter so that our widget will not get ignored
      // and drag events will not get passed to QgisApp
      e->accept();
    }
    void dragMoveEvent( QDragMoveEvent* e )
    {
      // do not accept drops above/below items
      /*if ( dropIndicatorPosition() != QAbstractItemView::OnItem )
      {
        QgsDebugMsg("drag not on item");
        e->ignore();
        return;
      }*/

      QTreeView::dragMoveEvent( e );

      if ( !e->provides( "application/x-vnd.qgis.qgis.uri" ) )
      {
        e->ignore();
        return;
      }
    }
};

QgsBrowserDockWidget::QgsBrowserDockWidget( QWidget * parent ) :
    QDockWidget( parent ), mModel( NULL )
{
  setWindowTitle( tr( "Browser" ) );

  mBrowserView = new QgsBrowserTreeView( this );

  QToolButton* refreshButton = new QToolButton( this );
  refreshButton->setIcon( QgisApp::instance()->getThemeIcon( "mActionDraw.png" ) );
  // remove this to save space
  refreshButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
  refreshButton->setText( tr( "Refresh" ) );
  refreshButton->setToolTip( tr( "Refresh" ) );
  refreshButton->setAutoRaise( true );
  connect( refreshButton, SIGNAL( clicked() ), this, SLOT( refresh() ) );

  QToolButton* addLayersButton = new QToolButton( this );
  addLayersButton->setIcon( QgisApp::instance()->getThemeIcon( "mActionAddLayer.png" ) );
  // remove this to save space
  addLayersButton->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );
  addLayersButton->setText( tr( "Add Selection" ) );
  addLayersButton->setToolTip( tr( "Add Selected Layers" ) );
  addLayersButton->setAutoRaise( true );
  connect( addLayersButton, SIGNAL( clicked() ), this, SLOT( addSelectedLayers() ) );

  QVBoxLayout* layout = new QVBoxLayout();
  QHBoxLayout* hlayout = new QHBoxLayout();
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setSpacing( 0 );
  hlayout->setContentsMargins( 0, 0, 0, 0 );
  hlayout->setSpacing( 5 );
  hlayout->setAlignment( Qt::AlignLeft );

  hlayout->addWidget( refreshButton );
  hlayout->addWidget( addLayersButton );
  layout->addLayout( hlayout );
  layout->addWidget( mBrowserView );

  QWidget* innerWidget = new QWidget( this );
  innerWidget->setLayout( layout );
  setWidget( innerWidget );

  connect( mBrowserView, SIGNAL( customContextMenuRequested( const QPoint & ) ), this, SLOT( showContextMenu( const QPoint & ) ) );
  //connect( mBrowserView, SIGNAL( clicked( const QModelIndex& ) ), this, SLOT( itemClicked( const QModelIndex& ) ) );
  connect( mBrowserView, SIGNAL( doubleClicked( const QModelIndex& ) ), this, SLOT( itemClicked( const QModelIndex& ) ) );

}

void QgsBrowserDockWidget::showEvent( QShowEvent * e )
{
  // delayed initialization of the model
  if ( mModel == NULL )
  {
    mModel = new QgsBrowserModel( mBrowserView );
    mBrowserView->setModel( mModel );

    // provide a horizontal scroll bar instead of using ellipse (...) for longer items
    mBrowserView->setTextElideMode( Qt::ElideNone );
    mBrowserView->header()->setResizeMode( 0, QHeaderView::ResizeToContents );
    mBrowserView->header()->setStretchLastSection( false );
  }

  QDockWidget::showEvent( e );
}


void QgsBrowserDockWidget::itemClicked( const QModelIndex& index )
{
  QgsDataItem *dataItem = mModel->dataItem( index );

  if ( dataItem != NULL && dataItem->type() == QgsDataItem::Layer )
  {
    QgsLayerItem *layerItem = qobject_cast<QgsLayerItem*>( dataItem );
    if ( layerItem != NULL )
      addLayer( layerItem );
  }
}

void QgsBrowserDockWidget::showContextMenu( const QPoint & pt )
{
  QModelIndex idx = mBrowserView->indexAt( pt );
  QgsDataItem* item = mModel->dataItem( idx );
  if ( !item )
    return;

  QMenu* menu = new QMenu( this );

  if ( item->type() == QgsDataItem::Directory )
  {
    QSettings settings;
    QStringList favDirs = settings.value( "/browser/favourites" ).toStringList();
    bool inFavDirs = favDirs.contains( item->path() );

    if ( item->parent() != NULL && !inFavDirs )
    {
      // only non-root directories can be added as favourites
      menu->addAction( tr( "Add as a favourite" ), this, SLOT( addFavourite() ) );
    }
    else if ( inFavDirs )
    {
      // only favourites can be removed
      menu->addAction( tr( "Remove favourite" ), this, SLOT( removeFavourite() ) );
    }
  }

  else if ( item->type() == QgsDataItem::Layer )
  {
    menu->addAction( tr( "Add Layer" ), this, SLOT( itemClicked( idx ) ) );
    menu->addAction( tr( "Add Selected Layers" ), this, SLOT( addSelectedLayers() ) );
  }

  QList<QAction*> actions = item->actions();
  if ( !actions.isEmpty() )
  {
    if ( !menu->actions().isEmpty() )
      menu->addSeparator();
    // add action to the menu
    menu->addActions( actions );
  }

  if ( menu->actions().count() == 0 )
  {
    delete menu;
    return;
  }

  menu->popup( mBrowserView->mapToGlobal( pt ) );
}

void QgsBrowserDockWidget::addFavourite()
{
  QgsDataItem* item = mModel->dataItem( mBrowserView->currentIndex() );
  if ( !item )
    return;
  if ( item->type() != QgsDataItem::Directory )
    return;

  QString newFavDir = item->path();

  QSettings settings;
  QStringList favDirs = settings.value( "/browser/favourites" ).toStringList();
  favDirs.append( newFavDir );
  settings.setValue( "/browser/favourites", favDirs );

  // reload the browser model so that the newly added favourite directory is shown
  mModel->reload();
}

void QgsBrowserDockWidget::removeFavourite()
{
  QgsDataItem* item = mModel->dataItem( mBrowserView->currentIndex() );
  if ( !item )
    return;
  if ( item->type() != QgsDataItem::Directory )
    return;

  QString favDir  = item->path();

  QSettings settings;
  QStringList favDirs = settings.value( "/browser/favourites" ).toStringList();
  favDirs.removeAll( favDir );
  settings.setValue( "/browser/favourites", favDirs );

  // reload the browser model so that the favourite directory is not shown anymore
  mModel->reload();
}

void QgsBrowserDockWidget::refresh()
{
  QApplication::setOverrideCursor( Qt::WaitCursor );
  refreshModel( QModelIndex() );
  QApplication::restoreOverrideCursor();
}

void QgsBrowserDockWidget::refreshModel( const QModelIndex& index )
{
  QgsDebugMsg( "Entered" );
  if ( index.isValid() )
  {
    QgsDataItem *item = mModel->dataItem( index );
    if ( item )
    {
      QgsDebugMsg( "path = " + item->path() );
    }
    else
    {
      QgsDebugMsg( "invalid item" );
    }
  }

  mModel->refresh( index );

  for ( int i = 0 ; i < mModel->rowCount( index ); i++ )
  {
    QModelIndex idx = mModel->index( i, 0, index );
    if ( mBrowserView->isExpanded( idx ) || !mModel->hasChildren( idx ) )
    {
      refreshModel( idx );
    }
  }
}

void QgsBrowserDockWidget::addLayer( QgsLayerItem *layerItem )
{
  if ( layerItem == NULL )
    return;

  QString uri = layerItem->uri();
  if ( uri.isEmpty() )
    return;

  QgsMapLayer::LayerType type = layerItem->mapLayerType();
  QString providerKey = layerItem->providerKey();

  QgsDebugMsg( providerKey + " : " + uri );
  if ( type == QgsMapLayer::VectorLayer )
  {
    QgisApp::instance()->addVectorLayer( uri, layerItem->name(), providerKey );
  }
  if ( type == QgsMapLayer::RasterLayer )
  {
    // This should go to WMS provider
    QStringList URIParts = uri.split( "|" );
    QString rasterLayerPath = URIParts.at( 0 );
    QStringList layers;
    QStringList styles;
    QString format;
    QString crs;
    for ( int i = 1 ; i < URIParts.size(); i++ )
    {
      QString part = URIParts.at( i );
      int pos = part.indexOf( "=" );
      QString field = part.left( pos );
      QString value = part.mid( pos + 1 );

      if ( field == "layers" )
        layers = value.split( "," );
      if ( field == "styles" )
        styles = value.split( "," );
      if ( field == "format" )
        format = value;
      if ( field == "crs" )
        crs = value;
    }
    QgsDebugMsg( "rasterLayerPath = " + rasterLayerPath );
    QgsDebugMsg( "layers = " + layers.join( " " ) );

    QgisApp::instance()->addRasterLayer( rasterLayerPath, layerItem->name(), providerKey, layers, styles, format, crs );
  }
}

void QgsBrowserDockWidget::addSelectedLayers()
{
  QApplication::setOverrideCursor( Qt::WaitCursor );

  // get a sorted list of selected indexes
  QModelIndexList list = mBrowserView->selectionModel()->selectedIndexes();
  qSort( list );

  // add items in reverse order so they are in correct order in the layers dock
  for ( int i = list.size() - 1; i >= 0; i-- )
  {
    QModelIndex index = list[i];
    QgsDataItem *dataItem = mModel->dataItem( index );
    if ( dataItem && dataItem->type() == QgsDataItem::Layer )
    {
      QgsLayerItem *layerItem = qobject_cast<QgsLayerItem*>( dataItem );
      if ( layerItem )
        addLayer( layerItem );
    }
  }

  QApplication::restoreOverrideCursor();
}
