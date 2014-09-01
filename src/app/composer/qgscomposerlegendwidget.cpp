/***************************************************************************
                         qgscomposerlegendwidget.cpp
                         ---------------------------
    begin                : July 2008
    copyright            : (C) 2008 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposerlegendwidget.h"
#include "qgscomposerlegend.h"
#include "qgscomposerlegenditem.h"
#include "qgscomposerlegenditemdialog.h"
#include "qgscomposerlegendlayersdialog.h"
#include "qgscomposeritemwidget.h"
#include "qgscomposermap.h"
#include "qgscomposition.h"
#include <QFontDialog>
#include <QColorDialog>
#include <QInputDialog>

#include "qgisapp.h"
#include "qgsapplication.h"
#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgslegendrenderer.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaprenderer.h"
#include "qgsproject.h"
#include "qgsvectorlayer.h"

#include <QMessageBox>



class QgsComposerLegendMenuProvider : public QObject, public QgsLayerTreeViewMenuProvider
{
  public:
    QgsComposerLegendMenuProvider( QgsLayerTreeView* view, QgsComposerLegendWidget* w ) : mView( view ), mWidget( w ) {}

    virtual QMenu* createContextMenu()
    {
      if ( !mView->currentNode() )
        return 0;

      QMenu* menu = new QMenu();

      QgsComposerLegendStyle::Style currentStyle = QgsLegendRenderer::nodeLegendStyle( mView->currentNode(), mView->layerTreeModel() );

      QList<QgsComposerLegendStyle::Style> lst;
      lst << QgsComposerLegendStyle::Hidden << QgsComposerLegendStyle::Group << QgsComposerLegendStyle::Subgroup;
      foreach ( QgsComposerLegendStyle::Style style, lst )
      {
        QAction* action = menu->addAction( QgsComposerLegendStyle::styleLabel( style ), mWidget, SLOT( setCurrentNodeStyleFromAction() ) );
        action->setCheckable( true );
        action->setChecked( currentStyle == style );
        action->setData( ( int ) style );
      }

      return menu;
    }

  protected:
    QgsLayerTreeView* mView;
    QgsComposerLegendWidget* mWidget;
};



QgsComposerLegendWidget::QgsComposerLegendWidget( QgsComposerLegend* legend ): QgsComposerItemBaseWidget( 0, legend ), mLegend( legend )
{
  setupUi( this );

  // setup icons
  mAddToolButton->setIcon( QIcon( QgsApplication::iconPath( "symbologyAdd.png" ) ) );
  mEditPushButton->setIcon( QIcon( QgsApplication::iconPath( "symbologyEdit.png" ) ) );
  mRemoveToolButton->setIcon( QIcon( QgsApplication::iconPath( "symbologyRemove.png" ) ) );
  mMoveUpToolButton->setIcon( QIcon( QgsApplication::iconPath( "symbologyUp.png" ) ) );
  mMoveDownToolButton->setIcon( QIcon( QgsApplication::iconPath( "symbologyDown.png" ) ) );
  mCountToolButton->setIcon( QIcon( QgsApplication::iconPath( "mActionSum.png" ) ) );

  mFontColorButton->setColorDialogTitle( tr( "Select font color" ) );
  mFontColorButton->setContext( "composer" );

  //add widget for item properties
  QgsComposerItemWidget* itemPropertiesWidget = new QgsComposerItemWidget( this, legend );
  mainLayout->addWidget( itemPropertiesWidget );

  mItemTreeView->setHeaderHidden( true );
  mItemTreeView->setModel( legend->modelV2() );
  mItemTreeView->setMenuProvider( new QgsComposerLegendMenuProvider( mItemTreeView, this ) );

  if ( legend )
  {
    connect( legend, SIGNAL( itemChanged() ), this, SLOT( setGuiElements() ) );
  }

  mWrapCharLineEdit->setText( legend->wrapChar() );

  setGuiElements();

  connect( mItemTreeView->selectionModel(), SIGNAL( currentChanged( const QModelIndex &, const QModelIndex & ) ),
           this, SLOT( selectedChanged( const QModelIndex &, const QModelIndex & ) ) );
}

QgsComposerLegendWidget::QgsComposerLegendWidget(): QgsComposerItemBaseWidget( 0, 0 ), mLegend( 0 )
{
  setupUi( this );
}

QgsComposerLegendWidget::~QgsComposerLegendWidget()
{

}

void QgsComposerLegendWidget::setGuiElements()
{
  if ( !mLegend )
  {
    return;
  }

  int alignment = mLegend->titleAlignment() == Qt::AlignLeft ? 0 : mLegend->titleAlignment() == Qt::AlignHCenter ? 1 : 2;

  blockAllSignals( true );
  mTitleLineEdit->setText( mLegend->title() );
  mTitleAlignCombo->setCurrentIndex( alignment );
  mColumnCountSpinBox->setValue( mLegend->columnCount() );
  mSplitLayerCheckBox->setChecked( mLegend->splitLayer() );
  mEqualColumnWidthCheckBox->setChecked( mLegend->equalColumnWidth() );
  mSymbolWidthSpinBox->setValue( mLegend->symbolWidth() );
  mSymbolHeightSpinBox->setValue( mLegend->symbolHeight() );
  mWmsLegendWidthSpinBox->setValue( mLegend->wmsLegendWidth() );
  mWmsLegendHeightSpinBox->setValue( mLegend->wmsLegendHeight() );
  mTitleSpaceBottomSpinBox->setValue( mLegend->style( QgsComposerLegendStyle::Title ).margin( QgsComposerLegendStyle::Bottom ) );
  mGroupSpaceSpinBox->setValue( mLegend->style( QgsComposerLegendStyle::Group ).margin( QgsComposerLegendStyle::Top ) );
  mLayerSpaceSpinBox->setValue( mLegend->style( QgsComposerLegendStyle::Subgroup ).margin( QgsComposerLegendStyle::Top ) );
  // We keep Symbol and SymbolLabel Top in sync for now
  mSymbolSpaceSpinBox->setValue( mLegend->style( QgsComposerLegendStyle::Symbol ).margin( QgsComposerLegendStyle::Top ) );
  mIconLabelSpaceSpinBox->setValue( mLegend->style( QgsComposerLegendStyle::SymbolLabel ).margin( QgsComposerLegendStyle::Left ) );
  mBoxSpaceSpinBox->setValue( mLegend->boxSpace() );
  mColumnSpaceSpinBox->setValue( mLegend->columnSpace() );
  mCheckBoxAutoUpdate->setChecked( mLegend->autoUpdateModel() );
  refreshMapComboBox();

  const QgsComposerMap* map = mLegend->composerMap();
  if ( map )
  {
    mMapComboBox->setCurrentIndex( mMapComboBox->findData( map->id() ) );
  }
  else
  {
    mMapComboBox->setCurrentIndex( mMapComboBox->findData( -1 ) );
  }
  mFontColorButton->setColor( mLegend->fontColor() );
  blockAllSignals( false );
}

void QgsComposerLegendWidget::on_mWrapCharLineEdit_textChanged( const QString &text )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Item wrapping changed" ) );
    mLegend->setWrapChar( text );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mTitleLineEdit_textChanged( const QString& text )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend title changed" ), QgsComposerMergeCommand::ComposerLegendText );
    mLegend->setTitle( text );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mTitleAlignCombo_currentIndexChanged( int index )
{
  if ( mLegend )
  {
    Qt::AlignmentFlag alignment = index == 0 ? Qt::AlignLeft : index == 1 ? Qt::AlignHCenter : Qt::AlignRight;
    mLegend->beginCommand( tr( "Legend title alignment changed" ) );
    mLegend->setTitleAlignment( alignment );
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mColumnCountSpinBox_valueChanged( int c )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend column count" ), QgsComposerMergeCommand::LegendColumnCount );
    mLegend->setColumnCount( c );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
  mSplitLayerCheckBox->setEnabled( c > 1 );
  mEqualColumnWidthCheckBox->setEnabled( c > 1 );
}

void QgsComposerLegendWidget::on_mSplitLayerCheckBox_toggled( bool checked )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend split layers" ), QgsComposerMergeCommand::LegendSplitLayer );
    mLegend->setSplitLayer( checked );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mEqualColumnWidthCheckBox_toggled( bool checked )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend equal column width" ), QgsComposerMergeCommand::LegendEqualColumnWidth );
    mLegend->setEqualColumnWidth( checked );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mSymbolWidthSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend symbol width" ), QgsComposerMergeCommand::LegendSymbolWidth );
    mLegend->setSymbolWidth( d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mSymbolHeightSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend symbol height" ), QgsComposerMergeCommand::LegendSymbolHeight );
    mLegend->setSymbolHeight( d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mWmsLegendWidthSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Wms Legend width" ), QgsComposerMergeCommand::LegendWmsLegendWidth );
    mLegend->setWmsLegendWidth( d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mWmsLegendHeightSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Wms Legend height" ), QgsComposerMergeCommand::LegendWmsLegendHeight );
    mLegend->setWmsLegendHeight( d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mTitleSpaceBottomSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend title space bottom" ), QgsComposerMergeCommand::LegendTitleSpaceBottom );
    mLegend->rstyle( QgsComposerLegendStyle::Title ).setMargin( QgsComposerLegendStyle::Bottom, d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mGroupSpaceSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend group space" ), QgsComposerMergeCommand::LegendGroupSpace );
    mLegend->rstyle( QgsComposerLegendStyle::Group ).setMargin( QgsComposerLegendStyle::Top, d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mLayerSpaceSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend layer space" ), QgsComposerMergeCommand::LegendLayerSpace );
    mLegend->rstyle( QgsComposerLegendStyle::Subgroup ).setMargin( QgsComposerLegendStyle::Top, d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mSymbolSpaceSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend symbol space" ), QgsComposerMergeCommand::LegendSymbolSpace );
    // We keep Symbol and SymbolLabel Top in sync for now
    mLegend->rstyle( QgsComposerLegendStyle::Symbol ).setMargin( QgsComposerLegendStyle::Top, d );
    mLegend->rstyle( QgsComposerLegendStyle::SymbolLabel ).setMargin( QgsComposerLegendStyle::Top, d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mIconLabelSpaceSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend icon label space" ), QgsComposerMergeCommand::LegendIconSymbolSpace );
    mLegend->rstyle( QgsComposerLegendStyle::SymbolLabel ).setMargin( QgsComposerLegendStyle::Left, d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mTitleFontButton_clicked()
{
  if ( mLegend )
  {
    bool ok;
#if defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
    // Native Mac dialog works only for Qt Carbon
    QFont newFont = QFontDialog::getFont( &ok, mLegend->style( QgsComposerLegendStyle::Title ).font(), 0, QString(), QFontDialog::DontUseNativeDialog );
#else
    QFont newFont = QFontDialog::getFont( &ok, mLegend->style( QgsComposerLegendStyle::Title ).font() );
#endif
    if ( ok )
    {
      mLegend->beginCommand( tr( "Title font changed" ) );
      mLegend->setStyleFont( QgsComposerLegendStyle::Title, newFont );
      mLegend->adjustBoxSize();
      mLegend->update();
      mLegend->endCommand();
    }
  }
}

void QgsComposerLegendWidget::on_mGroupFontButton_clicked()
{
  if ( mLegend )
  {
    bool ok;
#if defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
    // Native Mac dialog works only for Qt Carbon
    QFont newFont = QFontDialog::getFont( &ok, mLegend->style( QgsComposerLegendStyle::Group ).font(), 0, QString(), QFontDialog::DontUseNativeDialog );
#else
    QFont newFont = QFontDialog::getFont( &ok, mLegend->style( QgsComposerLegendStyle::Group ).font() );
#endif
    if ( ok )
    {
      mLegend->beginCommand( tr( "Legend group font changed" ) );
      mLegend->setStyleFont( QgsComposerLegendStyle::Group, newFont );
      mLegend->adjustBoxSize();
      mLegend->update();
      mLegend->endCommand();
    }
  }
}

void QgsComposerLegendWidget::on_mLayerFontButton_clicked()
{
  if ( mLegend )
  {
    bool ok;
#if defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
    // Native Mac dialog works only for Qt Carbon
    QFont newFont = QFontDialog::getFont( &ok, mLegend->style( QgsComposerLegendStyle::Subgroup ).font(), 0, QString(), QFontDialog::DontUseNativeDialog );
#else
    QFont newFont = QFontDialog::getFont( &ok, mLegend->style( QgsComposerLegendStyle::Subgroup ).font() );
#endif
    if ( ok )
    {
      mLegend->beginCommand( tr( "Legend layer font changed" ) );
      mLegend->setStyleFont( QgsComposerLegendStyle::Subgroup, newFont );
      mLegend->adjustBoxSize();
      mLegend->update();
      mLegend->endCommand();
    }
  }
}

void QgsComposerLegendWidget::on_mItemFontButton_clicked()
{
  if ( mLegend )
  {
    bool ok;
#if defined(Q_WS_MAC) && defined(QT_MAC_USE_COCOA)
    // Native Mac dialog works only for Qt Carbon
    QFont newFont = QFontDialog::getFont( &ok, mLegend->style( QgsComposerLegendStyle::SymbolLabel ).font(), 0, QString(), QFontDialog::DontUseNativeDialog );
#else
    QFont newFont = QFontDialog::getFont( &ok, mLegend->style( QgsComposerLegendStyle::SymbolLabel ).font() );
#endif
    if ( ok )
    {
      mLegend->beginCommand( tr( "Legend item font changed" ) );
      mLegend->setStyleFont( QgsComposerLegendStyle::SymbolLabel, newFont );
      mLegend->adjustBoxSize();
      mLegend->update();
      mLegend->endCommand();
    }
  }
}

void QgsComposerLegendWidget::on_mFontColorButton_colorChanged( const QColor& newFontColor )
{
  if ( !mLegend )
  {
    return;
  }

  mLegend->beginCommand( tr( "Legend font color changed" ) );
  mLegend->setFontColor( newFontColor );
  mLegend->update();
  mLegend->endCommand();
}

void QgsComposerLegendWidget::on_mBoxSpaceSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend box space" ), QgsComposerMergeCommand::LegendBoxSpace );
    mLegend->setBoxSpace( d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mColumnSpaceSpinBox_valueChanged( double d )
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend box space" ), QgsComposerMergeCommand::LegendColumnSpace );
    mLegend->setColumnSpace( d );
    mLegend->adjustBoxSize();
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::on_mMoveDownToolButton_clicked()
{
  if ( !mLegend )
  {
    return;
  }

  QStandardItemModel* itemModel = qobject_cast<QStandardItemModel *>( mItemTreeView->model() );
  if ( !itemModel )
  {
    return;
  }

  QModelIndex currentIndex = mItemTreeView->currentIndex();
  if ( !currentIndex.isValid() )
  {
    return;
  }

  //is there an older sibling?
  int row = currentIndex.row();
  QModelIndex youngerSibling = currentIndex.sibling( row + 1, 0 );

  if ( !youngerSibling.isValid() )
  {
    return;
  }

  mLegend->beginCommand( "Moved legend item down" );
  QModelIndex parentIndex = currentIndex.parent();
  QList<QStandardItem*> itemToMove;
  QList<QStandardItem*> youngerSiblingItem;

  if ( !parentIndex.isValid() ) //move toplevel (layer) item
  {
    youngerSiblingItem = itemModel->takeRow( row + 1 );
    itemToMove = itemModel->takeRow( row );
    itemModel->insertRow( row, youngerSiblingItem );
    itemModel->insertRow( row + 1, itemToMove );
  }
  else //move child (classification) item
  {
    QStandardItem* parentItem = itemModel->itemFromIndex( parentIndex );
    youngerSiblingItem = parentItem->takeRow( row + 1 );
    itemToMove = parentItem->takeRow( row );
    parentItem->insertRow( row, youngerSiblingItem );
    parentItem->insertRow( row + 1, itemToMove );
  }

  mItemTreeView->setCurrentIndex( itemModel->indexFromItem( itemToMove.at( 0 ) ) );
  mLegend->update();
  mLegend->endCommand();
}

void QgsComposerLegendWidget::on_mMoveUpToolButton_clicked()
{
  if ( !mLegend )
  {
    return;
  }

  QStandardItemModel* itemModel = qobject_cast<QStandardItemModel *>( mItemTreeView->model() );
  if ( !itemModel )
  {
    return;
  }

  QModelIndex currentIndex = mItemTreeView->currentIndex();
  if ( !currentIndex.isValid() )
  {
    return;
  }

  mLegend->beginCommand( "Moved legend item up" );
  //is there an older sibling?
  int row = currentIndex.row();
  QModelIndex olderSibling = currentIndex.sibling( row - 1, 0 );

  if ( !olderSibling.isValid() )
  {
    return;
  }

  QModelIndex parentIndex = currentIndex.parent();
  QList<QStandardItem*> itemToMove;
  QList<QStandardItem*> olderSiblingItem;

  if ( !parentIndex.isValid() ) //move toplevel item
  {
    itemToMove = itemModel->takeRow( row );
    olderSiblingItem = itemModel->takeRow( row - 1 );
    itemModel->insertRow( row - 1, itemToMove );
    itemModel->insertRow( row, olderSiblingItem );

  }
  else //move classification items
  {
    QStandardItem* parentItem = itemModel->itemFromIndex( parentIndex );
    itemToMove = parentItem->takeRow( row );
    olderSiblingItem = parentItem->takeRow( row - 1 );
    parentItem->insertRow( row - 1, itemToMove );
    parentItem->insertRow( row, olderSiblingItem );
  }

  mItemTreeView->setCurrentIndex( itemModel->indexFromItem( itemToMove.at( 0 ) ) );
  mLegend->update();
  mLegend->endCommand();
}

void QgsComposerLegendWidget::on_mCheckBoxAutoUpdate_stateChanged( int state )
{
  mLegend->setAutoUpdateModel( state == Qt::Checked );
}

void QgsComposerLegendWidget::on_mMapComboBox_currentIndexChanged( int index )
{
  if ( !mLegend )
  {
    return;
  }

  QVariant itemData = mMapComboBox->itemData( index );
  if ( itemData.type() == QVariant::Invalid )
  {
    return;
  }

  const QgsComposition* comp = mLegend->composition();
  if ( !comp )
  {
    return;
  }

  int mapNr = itemData.toInt();
  if ( mapNr < 0 )
  {
    mLegend->setComposerMap( 0 );
  }
  else
  {
    const QgsComposerMap* map = comp->getComposerMapById( mapNr );
    if ( map )
    {
      mLegend->beginCommand( tr( "Legend map changed" ) );
      mLegend->setComposerMap( map );
      mLegend->update();
      mLegend->endCommand();
    }
  }
}

void QgsComposerLegendWidget::on_mAddToolButton_clicked()
{
  if ( !mLegend )
  {
    return;
  }

  QgisApp* app = QgisApp::instance();
  if ( !app )
  {
    return;
  }

  QgsMapCanvas* canvas = app->mapCanvas();
  if ( canvas )
  {
    QList<QgsMapLayer*> layers = canvas->layers();

    QgsComposerLegendLayersDialog addDialog( layers );
    if ( addDialog.exec() == QDialog::Accepted )
    {
      QgsMapLayer* layer = addDialog.selectedLayer();
      if ( layer )
      {
        mLegend->beginCommand( "Legend item added" );
        mLegend->modelV2()->rootGroup()->addLayer( layer );
        mLegend->endCommand();
      }
    }
  }
}

void QgsComposerLegendWidget::on_mRemoveToolButton_clicked()
{
  if ( !mLegend )
  {
    return;
  }

  QItemSelectionModel* selectionModel = mItemTreeView->selectionModel();
  if ( !selectionModel )
  {
    return;
  }

  mLegend->beginCommand( "Legend item removed" );

  QList<QPersistentModelIndex> indexes;
  foreach ( const QModelIndex &index, selectionModel->selectedIndexes() )
    indexes << index;

  foreach ( const QPersistentModelIndex index, indexes )
  {
    mLegend->modelV2()->removeRow( index.row(), index.parent() );
  }

  mLegend->adjustBoxSize();
  mLegend->update();
  mLegend->endCommand();
}

void QgsComposerLegendWidget::on_mEditPushButton_clicked()
{
  if ( !mLegend )
  {
    return;
  }

  QgsLayerTreeNode* currentNode = mItemTreeView->currentNode();

  // TODO: support layer node, legend node

  if ( !QgsLayerTree::isGroup( currentNode ) )
    return;

  QString currentText = QgsLayerTree::toGroup( currentNode )->name();

  bool ok;
  QString newText = QInputDialog::getText( this, tr( "Legend item properties" ), tr( "Item text" ),
                                           QLineEdit::Normal, currentText, &ok );
  if ( !ok )
    return;

  mLegend->beginCommand( tr( "Legend item edited" ) );
  QgsLayerTree::toGroup( currentNode )->setName( newText );
  mLegend->adjustBoxSize();
  mLegend->update();
  mLegend->endCommand();
}

void QgsComposerLegendWidget::on_mUpdatePushButton_clicked()
{
  if ( !mLegend )
  {
    return;
  }

  //get current item
  QStandardItemModel* itemModel = qobject_cast<QStandardItemModel *>( mItemTreeView->model() );
  if ( !itemModel )
  {
    return;
  }

  //get current item
  QModelIndex currentIndex = mItemTreeView->currentIndex();
  if ( !currentIndex.isValid() )
  {
    return;
  }

  QStandardItem* currentItem = itemModel->itemFromIndex( currentIndex );
  if ( !currentItem )
  {
    return;
  }

  mLegend->beginCommand( tr( "Legend updated" ) );
  if ( mLegend->model() )
  {
    mLegend->model()->updateItem( currentItem );
  }
  mLegend->update();
  mLegend->adjustBoxSize();
  mLegend->endCommand();
}

void QgsComposerLegendWidget::on_mCountToolButton_clicked( bool checked )
{
  QgsDebugMsg( "Entered." );
  if ( !mLegend )
  {
    return;
  }

  //get current item
  QModelIndex currentIndex = mItemTreeView->currentIndex();
  if ( !currentIndex.isValid() )
  {
    return;
  }

  QgsLayerTreeNode* currentNode = mItemTreeView->currentNode();
  if ( !QgsLayerTree::isLayer( currentNode ) )
    return;

  mLegend->beginCommand( tr( "Legend updated" ) );
  currentNode->setCustomProperty( "showFeatureCount", checked ? 1 : 0 );
  mLegend->update();
  mLegend->adjustBoxSize();
  mLegend->endCommand();
}

void QgsComposerLegendWidget::on_mUpdateAllPushButton_clicked()
{
  updateLegend();
}

void QgsComposerLegendWidget::on_mAddGroupToolButton_clicked()
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend group added" ) );
    mLegend->modelV2()->rootGroup()->addGroup( tr( "Group" ) );
    mLegend->update();
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::updateLegend()
{
  if ( mLegend )
  {
    mLegend->beginCommand( tr( "Legend updated" ) );
    QgisApp* app = QgisApp::instance();
    if ( !app )
    {
      return;
    }

    //get layer id list
    QStringList layerIdList;
    const QgsComposerMap* linkedMap = mLegend->composerMap();
    if ( linkedMap && linkedMap->keepLayerSet() )
    {
      //if there is a linked map, and if that linked map has a locked layer set, we take the layerIdList from that ComposerMap
      layerIdList = linkedMap->layerSet();
    }
    else
    {
      //we take the layerIdList from the Canvas
      QgsMapCanvas* canvas = app->mapCanvas();
      if ( canvas )
      {
        layerIdList = canvas->mapSettings().layers();
      }
    }


    //and also group info
    mLegend->model()->setLayerSetAndGroups( QgsProject::instance()->layerTreeRoot() );
    mLegend->endCommand();
  }
}

void QgsComposerLegendWidget::blockAllSignals( bool b )
{
  mTitleLineEdit->blockSignals( b );
  mTitleAlignCombo->blockSignals( b );
  mItemTreeView->blockSignals( b );
  mCheckBoxAutoUpdate->blockSignals( b );
  mMapComboBox->blockSignals( b );
  mColumnCountSpinBox->blockSignals( b );
  mSplitLayerCheckBox->blockSignals( b );
  mEqualColumnWidthCheckBox->blockSignals( b );
  mSymbolWidthSpinBox->blockSignals( b );
  mSymbolHeightSpinBox->blockSignals( b );
  mGroupSpaceSpinBox->blockSignals( b );
  mLayerSpaceSpinBox->blockSignals( b );
  mSymbolSpaceSpinBox->blockSignals( b );
  mIconLabelSpaceSpinBox->blockSignals( b );
  mBoxSpaceSpinBox->blockSignals( b );
  mColumnSpaceSpinBox->blockSignals( b );
  mFontColorButton->blockSignals( b );
}

void QgsComposerLegendWidget::refreshMapComboBox()
{
  if ( !mLegend )
  {
    return;
  }

  const QgsComposition* composition = mLegend->composition();
  if ( !composition )
  {
    return;
  }

  //save current entry
  int currentMapId = mMapComboBox->itemData( mMapComboBox->currentIndex() ).toInt();
  mMapComboBox->clear();

  QList<const QgsComposerMap*> availableMaps = composition->composerMapItems();
  QList<const QgsComposerMap*>::const_iterator mapItemIt = availableMaps.constBegin();
  for ( ; mapItemIt != availableMaps.constEnd(); ++mapItemIt )
  {
    mMapComboBox->addItem( tr( "Map %1" ).arg(( *mapItemIt )->id() ), ( *mapItemIt )->id() );
  }
  mMapComboBox->addItem( tr( "None" ), -1 );

  //the former entry is not there anymore
  int entry = mMapComboBox->findData( currentMapId );
  if ( entry == -1 )
  {
  }
  else
  {
    mMapComboBox->setCurrentIndex( entry );
  }
}

void QgsComposerLegendWidget::showEvent( QShowEvent * event )
{
  refreshMapComboBox();
  QWidget::showEvent( event );
}

void QgsComposerLegendWidget::selectedChanged( const QModelIndex & current, const QModelIndex & previous )
{
  Q_UNUSED( current );
  Q_UNUSED( previous );
  QgsDebugMsg( "Entered" );

  mCountToolButton->setChecked( false );
  mCountToolButton->setEnabled( false );

  QgsLayerTreeNode* currentNode = mItemTreeView->currentNode();
  if ( !QgsLayerTree::isLayer( currentNode ) )
    return;

  QgsLayerTreeLayer* currentLayerNode = QgsLayerTree::toLayer( currentNode );
  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer*>( currentLayerNode->layer() );
  if ( !vl )
    return;

  mCountToolButton->setChecked( currentNode->customProperty( "showFeatureCount", 0 ).toInt() );
  mCountToolButton->setEnabled( true );
}

void QgsComposerLegendWidget::setCurrentNodeStyleFromAction()
{
  QAction* a = qobject_cast<QAction*>( sender() );
  if ( !a || !mItemTreeView->currentNode() )
    return;

  QgsLegendRenderer::setNodeLegendStyle( mItemTreeView->currentNode(), ( QgsComposerLegendStyle::Style ) a->data().toInt() );
  mLegend->update();
}
