/***************************************************************************
                         qgscomposeritemwidget.cpp
                         -------------------------
    begin                : August 2008
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

#include "qgscomposeritemwidget.h"
#include "qgscomposeritem.h"
#include "qgscomposermap.h"
#include "qgscomposition.h"
#include "qgspoint.h"
#include <QColorDialog>
#include <QPen>

QgsComposerItemWidget::QgsComposerItemWidget( QWidget* parent, QgsComposerItem* item ): QWidget( parent ), mItem( item )
{

  setupUi( this );

  //make button exclusive
  QButtonGroup* buttonGroup = new QButtonGroup( this );
  buttonGroup->addButton( mUpperLeftCheckBox );
  buttonGroup->addButton( mUpperMiddleCheckBox );
  buttonGroup->addButton( mUpperRightCheckBox );
  buttonGroup->addButton( mMiddleLeftCheckBox );
  buttonGroup->addButton( mMiddleCheckBox );
  buttonGroup->addButton( mMiddleRightCheckBox );
  buttonGroup->addButton( mLowerLeftCheckBox );
  buttonGroup->addButton( mLowerMiddleCheckBox );
  buttonGroup->addButton( mLowerRightCheckBox );
  buttonGroup->setExclusive( true );

  mXLineEdit->setValidator( new QDoubleValidator( 0 ) );
  mYLineEdit->setValidator( new QDoubleValidator( 0 ) );
  mWidthLineEdit->setValidator( new QDoubleValidator( 0 ) );
  mHeightLineEdit->setValidator( new QDoubleValidator( 0 ) );

  setValuesForGuiElements();
  connect( mItem->composition(), SIGNAL( paperSizeChanged() ), this, SLOT( setValuesForGuiPositionElements() ) );
  connect( mItem, SIGNAL( sizeChanged() ), this, SLOT( setValuesForGuiPositionElements() ) );
  connect( mItem, SIGNAL( itemChanged() ), this, SLOT( setValuesForGuiNonPositionElements() ) );

  connect( mTransparencySlider, SIGNAL( valueChanged( int ) ), mTransparencySpnBx, SLOT( setValue( int ) ) );
  connect( mTransparencySpnBx, SIGNAL( valueChanged( int ) ), mTransparencySlider, SLOT( setValue( int ) ) );
}

QgsComposerItemWidget::QgsComposerItemWidget(): QWidget( 0 ), mItem( 0 )
{

}

QgsComposerItemWidget::~QgsComposerItemWidget()
{

}

void QgsComposerItemWidget::showBackgroundGroup( bool showGroup )
{
  mBackgroundGroupBox->setVisible( showGroup );
}

void QgsComposerItemWidget::showFrameGroup( bool showGroup )
{
  mFrameGroupBox->setVisible( showGroup );
}

//slots
void QgsComposerItemWidget::on_mFrameColorButton_clicked()
{
  if ( !mItem )
  {
    return;
  }
}

void QgsComposerItemWidget::on_mFrameColorButton_colorChanged( const QColor& newFrameColor )
{
  if ( !mItem )
  {
    return;
  }
  mItem->beginCommand( tr( "Frame color changed" ) );
  QPen thePen = mItem->pen();
  thePen.setColor( newFrameColor );

  mItem->setPen( thePen );
  mItem->update();
  mItem->endCommand();
}

void QgsComposerItemWidget::on_mBackgroundColorButton_clicked()
{
  if ( !mItem )
  {
    return;
  }
}

void QgsComposerItemWidget::on_mBackgroundColorButton_colorChanged( const QColor& newBackgroundColor )
{
  if ( !mItem )
  {
    return;
  }
  mItem->beginCommand( tr( "Background color changed" ) );
  mItem->setBackgroundColor( newBackgroundColor );

  //if the item is a composer map, we need to regenerate the map image
  //because it usually is cached
  QgsComposerMap* cm = dynamic_cast<QgsComposerMap *>( mItem );
  if ( cm )
  {
    cm->cache();
  }
  mItem->update();
  mItem->endCommand();
}

void QgsComposerItemWidget::changeItemPosition()
{
  mItem->beginCommand( tr( "Item position changed" ) );

  bool convXSuccess, convYSuccess;
  double x = mXLineEdit->text().toDouble( &convXSuccess );
  double y = mYLineEdit->text().toDouble( &convYSuccess );

  bool convSuccessWidth, convSuccessHeight;
  double width = mWidthLineEdit->text().toDouble( &convSuccessWidth );
  double height = mHeightLineEdit->text().toDouble( &convSuccessHeight );

  if ( !convXSuccess || !convYSuccess || !convSuccessWidth || !convSuccessHeight )
  {
    return;
  }

  mItem->setItemPosition( x, y, width, height, positionMode(), false, mPageSpinBox->value() );

  mItem->update();
  mItem->endCommand();
}

QgsComposerItem::ItemPositionMode QgsComposerItemWidget::positionMode() const
{
  if ( mUpperLeftCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::UpperLeft;
  }
  else if ( mUpperMiddleCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::UpperMiddle;
  }
  else if ( mUpperRightCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::UpperRight;
  }
  else if ( mMiddleLeftCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::MiddleLeft;
  }
  else if ( mMiddleCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::Middle;
  }
  else if ( mMiddleRightCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::MiddleRight;
  }
  else if ( mLowerLeftCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::LowerLeft;
  }
  else if ( mLowerMiddleCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::LowerMiddle;
  }
  else if ( mLowerRightCheckBox->checkState() == Qt::Checked )
  {
    return QgsComposerItem::LowerRight;
  }
  return QgsComposerItem::UpperLeft;
}

void QgsComposerItemWidget::on_mOutlineWidthSpinBox_valueChanged( double d )
{
  if ( !mItem )
  {
    return;
  }

  mItem->beginCommand( tr( "Item outline width" ), QgsComposerMergeCommand::ItemOutlineWidth );
  mItem->setFrameOutlineWidth( d );
  mItem->endCommand();
}

void QgsComposerItemWidget::on_mFrameJoinStyleCombo_currentIndexChanged( int index )
{
  Q_UNUSED( index );
  if ( !mItem )
  {
    return;
  }

  mItem->beginCommand( tr( "Item frame join style" ) );
  mItem->setFrameJoinStyle( mFrameJoinStyleCombo->penJoinStyle() );
  mItem->endCommand();
}

void QgsComposerItemWidget::on_mFrameGroupBox_toggled( bool state )
{
  if ( !mItem )
  {
    return;
  }

  mItem->beginCommand( tr( "Item frame toggled" ) );
  mItem->setFrameEnabled( state );
  mItem->update();
  mItem->endCommand();
}

void QgsComposerItemWidget::on_mBackgroundGroupBox_toggled( bool state )
{
  if ( !mItem )
  {
    return;
  }

  mItem->beginCommand( tr( "Item background toggled" ) );
  mItem->setBackgroundEnabled( state );

  //if the item is a composer map, we need to regenerate the map image
  //because it usually is cached
  QgsComposerMap* cm = dynamic_cast<QgsComposerMap *>( mItem );
  if ( cm )
  {
    cm->cache();
  }

  mItem->update();
  mItem->endCommand();
}


void QgsComposerItemWidget::setValuesForGuiPositionElements()
{
  if ( !mItem )
  {
    return;
  }

  mXLineEdit->blockSignals( true );
  mYLineEdit->blockSignals( true );
  mWidthLineEdit->blockSignals( true );
  mHeightLineEdit->blockSignals( true );
  mUpperLeftCheckBox->blockSignals( true );
  mUpperMiddleCheckBox->blockSignals( true );
  mUpperRightCheckBox->blockSignals( true );
  mMiddleLeftCheckBox->blockSignals( true );
  mMiddleCheckBox->blockSignals( true );
  mMiddleRightCheckBox->blockSignals( true );
  mLowerLeftCheckBox->blockSignals( true );
  mLowerMiddleCheckBox->blockSignals( true );
  mLowerRightCheckBox->blockSignals( true );
  mPageSpinBox->blockSignals( true );

  QPointF pos = mItem->pagePos();

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::UpperLeft )
  {
    mUpperLeftCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() ) );
    mYLineEdit->setText( QString::number( pos.y() ) );
  }

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::UpperMiddle )
  {
    mUpperMiddleCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() + mItem->rect().width() / 2.0 ) );
    mYLineEdit->setText( QString::number( pos.y() ) );
  }

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::UpperRight )
  {
    mUpperRightCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() + mItem->rect().width() ) );
    mYLineEdit->setText( QString::number( pos.y() ) );
  }

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::MiddleLeft )
  {
    mMiddleLeftCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() ) );
    mYLineEdit->setText( QString::number( pos.y() + mItem->rect().height() / 2.0 ) );
  }

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::Middle )
  {
    mMiddleCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() + mItem->rect().width() / 2.0 ) );
    mYLineEdit->setText( QString::number( pos.y() + mItem->rect().height() / 2.0 ) );
  }

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::MiddleRight )
  {
    mMiddleRightCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() + mItem->rect().width() ) );
    mYLineEdit->setText( QString::number( pos.y() + mItem->rect().height() / 2.0 ) );
  }

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::LowerLeft )
  {
    mLowerLeftCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() ) );
    mYLineEdit->setText( QString::number( pos.y() + mItem->rect().height() ) );
  }

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::LowerMiddle )
  {
    mLowerMiddleCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() + mItem->rect().width() / 2.0 ) );
    mYLineEdit->setText( QString::number( pos.y() + mItem->rect().height() ) );
  }

  if ( mItem->lastUsedPositionMode() == QgsComposerItem::LowerRight )
  {
    mLowerRightCheckBox->setChecked( true );
    mXLineEdit->setText( QString::number( pos.x() + mItem->rect().width() ) );
    mYLineEdit->setText( QString::number( pos.y() + mItem->rect().height() ) );
  }

  mWidthLineEdit->setText( QString::number( mItem->rect().width() ) );
  mHeightLineEdit->setText( QString::number( mItem->rect().height() ) );
  mPageSpinBox->setValue( mItem->page() );


  mXLineEdit->blockSignals( false );
  mYLineEdit->blockSignals( false );
  mWidthLineEdit->blockSignals( false );
  mHeightLineEdit->blockSignals( false );
  mUpperLeftCheckBox->blockSignals( false );
  mUpperMiddleCheckBox->blockSignals( false );
  mUpperRightCheckBox->blockSignals( false );
  mMiddleLeftCheckBox->blockSignals( false );
  mMiddleCheckBox->blockSignals( false );
  mMiddleRightCheckBox->blockSignals( false );
  mLowerLeftCheckBox->blockSignals( false );
  mLowerMiddleCheckBox->blockSignals( false );
  mLowerRightCheckBox->blockSignals( false );
  mPageSpinBox->blockSignals( false );
}

void QgsComposerItemWidget::setValuesForGuiNonPositionElements()
{
  if ( !mItem )
  {
    return;
  }

  mOutlineWidthSpinBox->blockSignals( true );
  mFrameGroupBox->blockSignals( true );
  mBackgroundGroupBox->blockSignals( true );
  mItemIdLineEdit->blockSignals( true );
  mBlendModeCombo->blockSignals( true );
  mTransparencySlider->blockSignals( true );
  mTransparencySpnBx->blockSignals( true );
  mFrameColorButton->blockSignals( true );
  mFrameJoinStyleCombo->blockSignals( true );
  mBackgroundColorButton->blockSignals( true );
  mItemRotationSpinBox->blockSignals( true );

  mBackgroundColorButton->setColor( mItem->brush().color() );
  mFrameColorButton->setColor( mItem->pen().color() );
  mOutlineWidthSpinBox->setValue( mItem->frameOutlineWidth() );
  mFrameJoinStyleCombo->setPenJoinStyle( mItem->frameJoinStyle() );
  mItemIdLineEdit->setText( mItem->id() );
  mFrameGroupBox->setChecked( mItem->hasFrame() );
  mBackgroundGroupBox->setChecked( mItem->hasBackground() );
  mBlendModeCombo->setBlendMode( mItem->blendMode() );
  mTransparencySlider->setValue( mItem->transparency() );
  mTransparencySpnBx->setValue( mItem->transparency() );
  mItemRotationSpinBox->setValue( mItem->itemRotation() );

  mBackgroundColorButton->blockSignals( false );
  mFrameColorButton->blockSignals( false );
  mFrameJoinStyleCombo->blockSignals( false );
  mOutlineWidthSpinBox->blockSignals( false );
  mFrameGroupBox->blockSignals( false );
  mBackgroundGroupBox->blockSignals( false );
  mItemIdLineEdit->blockSignals( false );
  mBlendModeCombo->blockSignals( false );
  mTransparencySlider->blockSignals( false );
  mTransparencySpnBx->blockSignals( false );
  mItemRotationSpinBox->blockSignals( false );
}

void QgsComposerItemWidget::setValuesForGuiElements()
{
  if ( !mItem )
  {
    return;
  }

  mBackgroundColorButton->setColorDialogTitle( tr( "Select background color" ) );
  mBackgroundColorButton->setColorDialogOptions( QColorDialog::ShowAlphaChannel );
  mFrameColorButton->setColorDialogTitle( tr( "Select frame color" ) );
  mFrameColorButton->setColorDialogOptions( QColorDialog::ShowAlphaChannel );

  setValuesForGuiPositionElements();
  setValuesForGuiNonPositionElements();
}

void QgsComposerItemWidget::on_mBlendModeCombo_currentIndexChanged( int index )
{
  Q_UNUSED( index );
  if ( mItem )
  {
    mItem->beginCommand( tr( "Item blend mode changed" ) );
    mItem->setBlendMode( mBlendModeCombo->blendMode() );
    mItem->endCommand();
  }
}

void QgsComposerItemWidget::on_mTransparencySlider_valueChanged( int value )
{
  if ( mItem )
  {
    mItem->beginCommand( tr( "Item transparency changed" ), QgsComposerMergeCommand::ItemTransparency );
    mItem->setTransparency( value );
    mItem->endCommand();
  }
}

void QgsComposerItemWidget::on_mItemIdLineEdit_editingFinished()
{
  if ( mItem )
  {
    mItem->beginCommand( tr( "Item id changed" ), QgsComposerMergeCommand::ComposerLabelSetId );
    mItem->setId( mItemIdLineEdit->text() );
    mItemIdLineEdit->setText( mItem->id() );
    mItem->endCommand();
  }
}

void QgsComposerItemWidget::on_mUpperLeftCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x(), mItem->pos().y(), QgsComposerItem::UpperLeft );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mUpperMiddleCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x() + mItem->rect().width() / 2.0,
                            mItem->pos().y(), QgsComposerItem::UpperMiddle );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mUpperRightCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x() + mItem->rect().width(),
                            mItem->pos().y(), QgsComposerItem::UpperRight );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mMiddleLeftCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x(),
                            mItem->pos().y() + mItem->rect().height() / 2.0, QgsComposerItem::MiddleLeft );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mMiddleCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x() + mItem->rect().width() / 2.0,
                            mItem->pos().y() + mItem->rect().height() / 2.0, QgsComposerItem::Middle );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mMiddleRightCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x() + mItem->rect().width(),
                            mItem->pos().y() + mItem->rect().height() / 2.0, QgsComposerItem::MiddleRight );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mLowerLeftCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x(),
                            mItem->pos().y() + mItem->rect().height(), QgsComposerItem::LowerLeft );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mLowerMiddleCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x() + mItem->rect().width() / 2.0,
                            mItem->pos().y() + mItem->rect().height(), QgsComposerItem::LowerMiddle );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mLowerRightCheckBox_stateChanged( int state )
{
  if ( state != Qt::Checked )
    return;
  if ( mItem )
  {
    mItem->setItemPosition( mItem->pos().x() + mItem->rect().width(),
                            mItem->pos().y() + mItem->rect().height(), QgsComposerItem::LowerRight );
  }
  setValuesForGuiPositionElements();
}

void QgsComposerItemWidget::on_mItemRotationSpinBox_valueChanged( double val )
{
  if ( mItem )
  {
    mItem->beginCommand( tr( "Item rotation changed" ), QgsComposerMergeCommand::ItemRotation );
    mItem->setItemRotation( val, true );
    mItem->update();
    mItem->endCommand();
  }
}
