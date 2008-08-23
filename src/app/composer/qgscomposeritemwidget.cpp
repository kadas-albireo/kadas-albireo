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
#include <QColorDialog>

QgsComposerItemWidget::QgsComposerItemWidget( QWidget* parent, QgsComposerItem* item ): QWidget( parent ), mItem( item )
{
  setupUi( this );
  setValuesForGuiElements();
}

QgsComposerItemWidget::QgsComposerItemWidget(): QWidget( 0 ), mItem( 0 )
{

}

QgsComposerItemWidget::~QgsComposerItemWidget()
{

}

//slots
void QgsComposerItemWidget::on_mFrameColorButton_clicked()
{
  if ( !mItem )
  {
    return;
  }

  QColor newFrameColor = QColorDialog::getColor( mItem->pen().color(), 0 );
  if ( !newFrameColor.isValid() )
  {
    return; //dialog canceled
  }

  QPen thePen;
  thePen.setColor( newFrameColor );
  thePen.setWidthF( mOutlineWidthSpinBox->value() );

  mItem->setPen( thePen );
  mItem->update();
}

void QgsComposerItemWidget::on_mBackgroundColorButton_clicked()
{
  if ( !mItem )
  {
    return;
  }

  QColor newBackgroundColor = QColorDialog::getColor( mItem->brush().color(), 0 );
  if ( !newBackgroundColor.isValid() )
  {
    return; //dialog canceled
  }

  newBackgroundColor.setAlpha( mOpacitySlider->value() );
  mItem->setBrush( QBrush( QColor( newBackgroundColor ), Qt::SolidPattern ) );
  mItem->update();
}

void QgsComposerItemWidget::on_mOpacitySlider_sliderReleased()
{
  if ( !mItem )
  {
    return;
  }
  int value = mOpacitySlider->value();

  QBrush itemBrush = mItem->brush();
  QColor brushColor = itemBrush.color();
  brushColor.setAlpha( value );
  mItem->setBrush( QBrush( brushColor ) );
  mItem->update();
}

void QgsComposerItemWidget::on_mOutlineWidthSpinBox_valueChanged( double d )
{
  if ( !mItem )
  {
    return;
  }

  QPen itemPen = mItem->pen();
  itemPen.setWidthF( d );
  mItem->setPen( itemPen );
}

void QgsComposerItemWidget::on_mFrameCheckBox_stateChanged( int state )
{
  if ( !mItem )
  {
    return;
  }

  if ( state == Qt::Checked )
  {
    mItem->setFrame( true );
  }
  else
  {
    mItem->setFrame( false );
  }
  mItem->update();
}

void QgsComposerItemWidget::setValuesForGuiElements()
{
  if ( !mItem )
  {
    return;
  }

  mOpacitySlider->blockSignals( true );
  mOutlineWidthSpinBox->blockSignals( true );
  mFrameCheckBox->blockSignals( true );

  mOpacitySlider->setValue( mItem->brush().color().alpha() );
  mOutlineWidthSpinBox->setValue( mItem->pen().widthF() );
  if ( mItem->frame() )
  {
    mFrameCheckBox->setCheckState( Qt::Checked );
  }
  else
  {
    mFrameCheckBox->setCheckState( Qt::Unchecked );
  }

  mOpacitySlider->blockSignals( false );
  mOutlineWidthSpinBox->blockSignals( false );
  mFrameCheckBox->blockSignals( false );

}
