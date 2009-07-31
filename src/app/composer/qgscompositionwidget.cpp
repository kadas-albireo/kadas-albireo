/***************************************************************************
                              qgscompositionwidget.cpp
                             --------------------------
    begin                : June 11 2008
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

#include "qgscompositionwidget.h"
#include "qgscomposition.h"
#include <QColorDialog>
#include <QWidget>
#include <QPrinter> //for screen resolution

QgsCompositionWidget::QgsCompositionWidget( QWidget* parent, QgsComposition* c ): QWidget( parent ), mComposition( c )
{
  setupUi( this );
  createPaperEntries();

  //unit
  mPaperUnitsComboBox->addItem( tr( "mm" ) );
  mPaperUnitsComboBox->addItem( tr( "inch" ) );

  //orientation
  mPaperOrientationComboBox->blockSignals( true );
  mPaperOrientationComboBox->insertItem( 0, tr( "Landscape" ) );
  mPaperOrientationComboBox->insertItem( 1, tr( "Portrait" ) );
  mPaperOrientationComboBox->blockSignals( false );
  mPaperOrientationComboBox->setCurrentIndex( 0 );

  //read with/height from composition and find suitable entries to display
  displayCompositionWidthHeight();

  if ( mComposition )
  {
    //read printout resolution from composition
    mResolutionLineEdit->setText( QString::number( mComposition->printResolution() ) );

    //print as raster
    if ( mComposition->printAsRaster() )
    {
      mPrintAsRasterCheckBox->setCheckState( Qt::Checked );
    }
    else
    {
      mPrintAsRasterCheckBox->setCheckState( Qt::Unchecked );
    }

    //snap grid
    if ( mComposition->snapToGridEnabled() )
    {
      mSnapToGridCheckBox->setCheckState( Qt::Checked );
    }
    else
    {
      mSnapToGridCheckBox->setCheckState( Qt::Unchecked );
    }
    mResolutionSpinBox->setValue( mComposition->snapGridResolution() );
    mOffsetXSpinBox->setValue( mComposition->snapGridOffsetX() );
    mOffsetYSpinBox->setValue( mComposition->snapGridOffsetY() );


    //grid pen width
    mPenWidthSpinBox->blockSignals( true );
    mPenWidthSpinBox->setValue( mComposition->gridPen().widthF() );
    mPenWidthSpinBox->blockSignals( false );

    //grid pen color
    mGridColorButton->blockSignals( true );
    mGridColorButton->setColor( mComposition->gridPen().color() );
    mGridColorButton->blockSignals( false );

    mGridStyleComboBox->blockSignals( true );
    mGridStyleComboBox->insertItem( 0, tr( "Solid" ) );
    mGridStyleComboBox->insertItem( 1, tr( "Dots" ) );
    mGridStyleComboBox->insertItem( 2, tr( "Crosses" ) );

    QgsComposition::GridStyle snapGridStyle = mComposition->gridStyle();
    if ( snapGridStyle == QgsComposition::Solid )
    {
      mGridStyleComboBox->setCurrentIndex( 0 );
    }
    else if ( snapGridStyle == QgsComposition::Dots )
    {
      mGridStyleComboBox->setCurrentIndex( 1 );
    }
    else
    {
      mGridStyleComboBox->setCurrentIndex( 2 );
    }
    mGridStyleComboBox->blockSignals( false );
  }
}

QgsCompositionWidget::QgsCompositionWidget(): QWidget( 0 ), mComposition( 0 )
{
  setupUi( this );
}

QgsCompositionWidget::~QgsCompositionWidget()
{

}

void QgsCompositionWidget::createPaperEntries()
{
  QList<QgsCompositionPaper> formats;

  formats
  << QgsCompositionPaper( tr( "A5 (148x210 mm)" ), 148, 210 )
  << QgsCompositionPaper( tr( "A4 (210x297 mm)" ), 210, 297 )
  << QgsCompositionPaper( tr( "A3 (297x420 mm)" ), 297, 420 )
  << QgsCompositionPaper( tr( "A2 (420x594 mm)" ), 420, 594 )
  << QgsCompositionPaper( tr( "A1 (594x841 mm)" ), 594, 841 )
  << QgsCompositionPaper( tr( "A0 (841x1189 mm)" ), 841, 1189 )
  << QgsCompositionPaper( tr( "B5 (176 x 250 mm)" ), 176, 250 )
  << QgsCompositionPaper( tr( "B4 (250 x 353 mm)" ), 250, 353 )
  << QgsCompositionPaper( tr( "B3 (353 x 500 mm)" ), 353, 500 )
  << QgsCompositionPaper( tr( "B2 (500 x 707 mm)" ), 500, 707 )
  << QgsCompositionPaper( tr( "B1 (707 x 1000 mm)" ), 707, 1000 )
  << QgsCompositionPaper( tr( "B0 (1000 x 1414 mm)" ), 1000, 1414 )
  << QgsCompositionPaper( tr( "Legal (8.5x14 inches)" ), 216, 356 )
  << QgsCompositionPaper( tr( "ANSI A (Letter; 8.5x11 inches)" ),  216, 279 )
  << QgsCompositionPaper( tr( "ANSI B (Tabloid; 11x17 inches)" ),  279, 432 )
  << QgsCompositionPaper( tr( "ANSI C (17x22 inches)" ),  432, 559 )
  << QgsCompositionPaper( tr( "ANSI D (22x34 inches)" ),  559, 864 )
  << QgsCompositionPaper( tr( "ANSI E (34x44 inches)" ),  864, 1118 )
  << QgsCompositionPaper( tr( "Arch A (9x12 inches)" ),  229, 305 )
  << QgsCompositionPaper( tr( "Arch B (12x18 inches)" ), 305, 457 )
  << QgsCompositionPaper( tr( "Arch C (18x24 inches)" ), 457, 610 )
  << QgsCompositionPaper( tr( "Arch D (24x36 inches)" ), 610, 914 )
  << QgsCompositionPaper( tr( "Arch E (36x48 inches)" ), 914, 1219 )
  << QgsCompositionPaper( tr( "Arch E1 (30x42 inches)" ), 762, 1067 )
  ;

  mPaperSizeComboBox->blockSignals( true );
  mPaperSizeComboBox->addItem( tr( "Custom" ) );

  for ( QList<QgsCompositionPaper>::const_iterator it = formats.begin(); it != formats.end(); it++ )
  {
    mPaperSizeComboBox->addItem( it->mName );
    mPaperMap.insert( it->mName, *it );
  }

  mPaperSizeComboBox->blockSignals( false );
}

void QgsCompositionWidget::on_mPaperSizeComboBox_currentIndexChanged( const QString& text )
{
  if ( mPaperSizeComboBox->currentText() == tr( "Custom" ) )
  {
    mPaperWidthLineEdit->setEnabled( true );
    mPaperHeightLineEdit->setEnabled( true );
    mPaperUnitsComboBox->setEnabled( true );
  }
  else
  {
    mPaperWidthLineEdit->setEnabled( false );
    mPaperHeightLineEdit->setEnabled( false );
    mPaperUnitsComboBox->setEnabled( false );
  }
  applyCurrentPaperSettings();
}

void QgsCompositionWidget::on_mPaperOrientationComboBox_currentIndexChanged( const QString& text )
{
  if ( mPaperSizeComboBox->currentText() == tr( "Custom" ) )
  {
    adjustOrientation();
    applyWidthHeight();
  }
  else
  {
    adjustOrientation();
    applyCurrentPaperSettings();
  }
}

void QgsCompositionWidget::on_mPaperUnitsComboBox_currentIndexChanged( const QString& text )
{
  double width = size( mPaperWidthLineEdit );
  double height = size( mPaperHeightLineEdit );

  if ( mPaperUnitsComboBox->currentIndex() == 0 )
  {
    // mm, value was inch
    width *= 25.4;
    height *= 25.4;
  }
  else
  {
    // inch, values was mm,
    width /= 25.4;
    height /= 25.4;
  }

  setSize( mPaperWidthLineEdit, width );
  setSize( mPaperHeightLineEdit, height );

  if ( mPaperSizeComboBox->currentText() == tr( "Custom" ) )
  {
    adjustOrientation();
    applyWidthHeight();
  }
  else
  {
    adjustOrientation();
    applyCurrentPaperSettings();
  }
}

void QgsCompositionWidget::adjustOrientation()
{
  double width = size( mPaperWidthLineEdit );
  double height = size( mPaperHeightLineEdit );

  if ( width < 0 || height < 0 )
  {
    return;
  }

  if ( height > width ) //change values such that width > height
  {
    double tmp = width;
    width = height;
    height = tmp;
  }

  bool lineEditsEnabled = mPaperWidthLineEdit->isEnabled();

  mPaperWidthLineEdit->setEnabled( true );
  mPaperHeightLineEdit->setEnabled( true );
  if ( mPaperOrientationComboBox->currentText() == tr( "Landscape" ) )
  {
    setSize( mPaperWidthLineEdit, width );
    setSize( mPaperHeightLineEdit, height );
  }
  else
  {
    setSize( mPaperWidthLineEdit, height );
    setSize( mPaperHeightLineEdit, width );
  }
  mPaperWidthLineEdit->setEnabled( lineEditsEnabled );
  mPaperHeightLineEdit->setEnabled( lineEditsEnabled );
}

void QgsCompositionWidget::setSize( QLineEdit *le, double v )
{
  if ( mPaperUnitsComboBox->currentIndex() == 0 )
  {
    // mm
    le->setText( QString::number( v ) );
  }
  else
  {
    // inch (show width in inch)
    le->setText( QString::number( v / 25.4 ) );
  }
}

double QgsCompositionWidget::size( QLineEdit *le )
{
  bool conversionSuccess;

  double size = le->text().toDouble( &conversionSuccess );
  if ( !conversionSuccess )
    return -1.0;

  if ( mPaperUnitsComboBox->currentIndex() == 0 )
  {
    // mm
    return size;
  }
  else
  {
    // inch return in mm
    return size * 25.4;
  }
}

void QgsCompositionWidget::applyCurrentPaperSettings()
{
  if ( mComposition )
  {
    //find entry in mPaper map to set width and height
    QMap<QString, QgsCompositionPaper>::iterator it = mPaperMap.find( mPaperSizeComboBox->currentText() );
    if ( it == mPaperMap.end() )
    {
      return;
    }

    mPaperWidthLineEdit->setEnabled( true );
    mPaperHeightLineEdit->setEnabled( true );
    setSize( mPaperWidthLineEdit, it->mWidth );
    setSize( mPaperHeightLineEdit, it->mHeight );
    mPaperWidthLineEdit->setEnabled( false );
    mPaperHeightLineEdit->setEnabled( false );

    adjustOrientation();
    applyWidthHeight();
  }
}

void QgsCompositionWidget::applyWidthHeight()
{
  double width = size( mPaperWidthLineEdit );
  double height = size( mPaperHeightLineEdit );

  if ( width < 0 || height < 0 )
    return;

  mComposition->setPaperSize( width, height );
}

void QgsCompositionWidget::on_mPaperWidthLineEdit_editingFinished()
{
  applyWidthHeight();
}

void QgsCompositionWidget::on_mPaperHeightLineEdit_editingFinished()
{
  applyWidthHeight();
}

void QgsCompositionWidget::displayCompositionWidthHeight()
{
  if ( !mComposition )
  {
    return;
  }

  //block all signals to avoid infinite recursion
  mPaperSizeComboBox->blockSignals( true );
  mPaperUnitsComboBox->blockSignals( true );
  mPaperWidthLineEdit->blockSignals( true );
  mPaperHeightLineEdit->blockSignals( true );
  mPaperOrientationComboBox->blockSignals( true );
  mResolutionLineEdit->blockSignals( true );

  double paperWidth = mComposition->paperWidth();
  setSize( mPaperWidthLineEdit, paperWidth );

  double paperHeight = mComposition->paperHeight();
  setSize( mPaperHeightLineEdit, paperHeight );

  //set orientation
  if ( paperWidth > paperHeight )
  {
    mPaperOrientationComboBox->setCurrentIndex( mPaperOrientationComboBox->findText( tr( "Landscape" ) ) );
  }
  else
  {
    mPaperOrientationComboBox->setCurrentIndex( mPaperOrientationComboBox->findText( tr( "Portrait" ) ) );
  }

  //set paper name
  bool found = false;
  QMap<QString, QgsCompositionPaper>::const_iterator paper_it = mPaperMap.constBegin();
  for ( ; paper_it != mPaperMap.constEnd(); ++paper_it )
  {
    QgsCompositionPaper currentPaper = paper_it.value();

    //consider width and height values may be exchanged
    if (( currentPaper.mWidth == paperWidth && currentPaper.mHeight == paperHeight )
        || ( currentPaper.mWidth == paperHeight && currentPaper.mHeight == paperWidth ) )
    {
      mPaperSizeComboBox->setCurrentIndex( mPaperSizeComboBox->findText( paper_it.key() ) );
      found = true;
    }
  }

  if ( !found )
  {
    //custom
    mPaperSizeComboBox->setCurrentIndex( 0 );
  }

  mPaperSizeComboBox->blockSignals( false );
  mPaperUnitsComboBox->blockSignals( false );
  mPaperWidthLineEdit->blockSignals( false );
  mPaperHeightLineEdit->blockSignals( false );
  mPaperOrientationComboBox->blockSignals( false );
  mResolutionLineEdit->blockSignals( false );
}

void QgsCompositionWidget::displaySnapingSettings()
{
  if ( !mComposition )
  {
    return;
  }

  mSnapToGridCheckBox->blockSignals( true );
  mResolutionSpinBox->blockSignals( true );
  mOffsetXSpinBox->blockSignals( true );
  mOffsetYSpinBox->blockSignals( true );

  if ( mComposition->snapToGridEnabled() )
  {
    mSnapToGridCheckBox->setCheckState( Qt::Checked );
  }
  else
  {
    mSnapToGridCheckBox->setCheckState( Qt::Unchecked );
  }

  mResolutionSpinBox->setValue( mComposition->snapGridResolution() );
  mOffsetXSpinBox->setValue( mComposition->snapGridOffsetX() );
  mOffsetYSpinBox->setValue( mComposition->snapGridOffsetY() );

  mSnapToGridCheckBox->blockSignals( false );
  mResolutionSpinBox->blockSignals( false );
  mOffsetXSpinBox->blockSignals( false );
  mOffsetYSpinBox->blockSignals( false );
}

void QgsCompositionWidget::on_mResolutionLineEdit_textChanged( const QString& text )
{
  bool conversionOk;
  int resolution = text.toInt( &conversionOk );
  if ( conversionOk && mComposition )
  {
    mComposition->setPrintResolution( resolution );
  }
  else if ( mComposition )
  {
    //set screen resolution per default
    QPrinter resolutionInfo( QPrinter::ScreenResolution );
    mComposition->setPrintResolution( resolutionInfo.resolution() );
  }
}

void QgsCompositionWidget::on_mPrintAsRasterCheckBox_stateChanged( int state )
{
  if ( !mComposition )
  {
    return;
  }

  if ( state == Qt::Checked )
  {
    mComposition->setPrintAsRaster( true );
  }
  else
  {
    mComposition->setPrintAsRaster( false );
  }
}

void QgsCompositionWidget::on_mSnapToGridCheckBox_stateChanged( int state )
{
  if ( mComposition )
  {
    if ( state == Qt::Checked )
    {
      mComposition->setSnapToGridEnabled( true );
    }
    else
    {
      mComposition->setSnapToGridEnabled( false );
    }
  }
}

void QgsCompositionWidget::on_mResolutionSpinBox_valueChanged( double d )
{
  if ( mComposition )
  {
    mComposition->setSnapGridResolution( d );
  }
}

void QgsCompositionWidget::on_mOffsetXSpinBox_valueChanged( double d )
{
  if ( mComposition )
  {
    mComposition->setSnapGridOffsetX( d );
  }
}

void QgsCompositionWidget::on_mOffsetYSpinBox_valueChanged( double d )
{
  if ( mComposition )
  {
    mComposition->setSnapGridOffsetY( d );
  }
}

void QgsCompositionWidget::on_mGridColorButton_clicked()
{
  QColor newColor = QColorDialog::getColor( mGridColorButton->color() );
  if ( !newColor.isValid() )
  {
    return ; //dialog canceled by user
  }
  mGridColorButton->setColor( newColor );

  if ( mComposition )
  {
    QPen pen = mComposition->gridPen();
    pen.setColor( newColor );
    mComposition->setGridPen( pen );
  }
}

void QgsCompositionWidget::on_mGridStyleComboBox_currentIndexChanged( const QString& text )
{
  if ( mComposition )
  {
    if ( mGridStyleComboBox->currentText() == tr( "Solid" ) )
    {
      mComposition->setGridStyle( QgsComposition::Solid );
    }
    else if ( mGridStyleComboBox->currentText() == tr( "Dots" ) )
    {
      mComposition->setGridStyle( QgsComposition::Dots );
    }
    else if ( mGridStyleComboBox->currentText() == tr( "Crosses" ) )
    {
      mComposition->setGridStyle( QgsComposition::Crosses );
    }
  }
}

void QgsCompositionWidget::on_mPenWidthSpinBox_valueChanged( double d )
{
  if ( mComposition )
  {
    QPen pen = mComposition->gridPen();
    pen.setWidthF( d );
    mComposition->setGridPen( pen );
  }
}
