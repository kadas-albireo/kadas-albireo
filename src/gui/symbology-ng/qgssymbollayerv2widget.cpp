/***************************************************************************
    qgssymbollayerv2widget.cpp - symbol layer widgets

    ---------------------
    begin                : November 2009
    copyright            : (C) 2009 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgssymbollayerv2widget.h"

#include "qgslinesymbollayerv2.h"
#include "qgsmarkersymbollayerv2.h"
#include "qgsfillsymbollayerv2.h"

#include "characterwidget.h"
#include "qgsdashspacedialog.h"
#include "qgsdatadefinedsymboldialog.h"
#include "qgssymbolv2selectordialog.h"
#include "qgssvgcache.h"
#include "qgssymbollayerv2utils.h"

#include "qgsstylev2.h" //for symbol selector dialog

#include "qgsapplication.h"

#include "qgslogger.h"

#include <QAbstractButton>
#include <QColorDialog>
#include <QCursor>
#include <QDir>
#include <QFileDialog>
#include <QPainter>
#include <QSettings>
#include <QStandardItemModel>
#include <QSvgRenderer>

QString QgsSymbolLayerV2Widget::dataDefinedPropertyLabel( const QString &entryName )
{
  QString label = entryName;
  if ( entryName == "size" )
  {
    label = tr( "Size" );
    QgsMarkerSymbolLayerV2 * layer = dynamic_cast<QgsMarkerSymbolLayerV2 *>( symbolLayer() );
    if ( layer )
    {
      switch ( layer->scaleMethod() )
      {
        case QgsSymbolV2::ScaleArea:
          label += " (" + tr( "area" ) + ")";
          break;
        case QgsSymbolV2::ScaleDiameter:
          label += " (" + tr( "diameter" ) + ")";
          break;
      }
    }
  }
  return label;
}

QgsSimpleLineSymbolLayerV2Widget::QgsSimpleLineSymbolLayerV2Widget( const QgsVectorLayer* vl, QWidget* parent )
    : QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = NULL;

  setupUi( this );

  if ( vl && vl->geometryType() != QGis::Polygon )
  {
    //draw inside polygon checkbox only makes sense for polygon layers
    mDrawInsideCheckBox->hide();
  }

  connect( spinWidth, SIGNAL( valueChanged( double ) ), this, SLOT( penWidthChanged() ) );
  connect( btnChangeColor, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( colorChanged( const QColor& ) ) );
  connect( cboPenStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( penStyleChanged() ) );
  connect( spinOffset, SIGNAL( valueChanged( double ) ), this, SLOT( offsetChanged() ) );
  connect( cboCapStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( penStyleChanged() ) );
  connect( cboJoinStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( penStyleChanged() ) );
  updatePatternIcon();

}

void QgsSimpleLineSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( !layer || layer->layerType() != "SimpleLine" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsSimpleLineSymbolLayerV2*>( layer );

  // set units
  mPenWidthUnitComboBox->blockSignals( true );
  mPenWidthUnitComboBox->setCurrentIndex( mLayer->widthUnit() );
  mPenWidthUnitComboBox->blockSignals( false );
  mOffsetUnitComboBox->blockSignals( true );
  mOffsetUnitComboBox->setCurrentIndex( mLayer->offsetUnit() );
  mOffsetUnitComboBox->blockSignals( false );
  mDashPatternUnitComboBox->blockSignals( true );
  mDashPatternUnitComboBox->setCurrentIndex( mLayer->customDashPatternUnit() );
  mDashPatternUnitComboBox->blockSignals( false );

  // set values
  spinWidth->setValue( mLayer->width() );
  btnChangeColor->setColor( mLayer->color() );
  btnChangeColor->setColorDialogOptions( QColorDialog::ShowAlphaChannel );
  spinOffset->setValue( mLayer->offset() );
  cboPenStyle->blockSignals( true );
  cboJoinStyle->blockSignals( true );
  cboCapStyle->blockSignals( true );
  cboPenStyle->setPenStyle( mLayer->penStyle() );
  cboJoinStyle->setPenJoinStyle( mLayer->penJoinStyle() );
  cboCapStyle->setPenCapStyle( mLayer->penCapStyle() );
  cboPenStyle->blockSignals( false );
  cboJoinStyle->blockSignals( false );
  cboCapStyle->blockSignals( false );

  //use a custom dash pattern?
  bool useCustomDashPattern = mLayer->useCustomDashPattern();
  mChangePatternButton->setEnabled( useCustomDashPattern );
  label_3->setEnabled( !useCustomDashPattern );
  cboPenStyle->setEnabled( !useCustomDashPattern );
  mCustomCheckBox->blockSignals( true );
  mCustomCheckBox->setCheckState( useCustomDashPattern ? Qt::Checked : Qt::Unchecked );
  mCustomCheckBox->blockSignals( false );

  //draw inside polygon?
  bool drawInsidePolygon = mLayer->drawInsidePolygon();
  mDrawInsideCheckBox->blockSignals( true );
  mDrawInsideCheckBox->setCheckState( drawInsidePolygon ? Qt::Checked : Qt::Unchecked );
  mDrawInsideCheckBox->blockSignals( false );

  updatePatternIcon();
}

QgsSymbolLayerV2* QgsSimpleLineSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsSimpleLineSymbolLayerV2Widget::penWidthChanged()
{
  mLayer->setWidth( spinWidth->value() );
  updatePatternIcon();
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::colorChanged( const QColor& color )
{
  mLayer->setColor( color );
  updatePatternIcon();
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::penStyleChanged()
{
  mLayer->setPenStyle( cboPenStyle->penStyle() );
  mLayer->setPenJoinStyle( cboJoinStyle->penJoinStyle() );
  mLayer->setPenCapStyle( cboCapStyle->penCapStyle() );
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::offsetChanged()
{
  mLayer->setOffset( spinOffset->value() );
  updatePatternIcon();
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::on_mCustomCheckBox_stateChanged( int state )
{
  bool checked = ( state == Qt::Checked );
  mChangePatternButton->setEnabled( checked );
  label_3->setEnabled( !checked );
  cboPenStyle->setEnabled( !checked );

  mLayer->setUseCustomDashPattern( checked );
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::on_mChangePatternButton_clicked()
{
  QgsDashSpaceDialog d( mLayer->customDashVector() );
  if ( d.exec() == QDialog::Accepted )
  {
    mLayer->setCustomDashVector( d.dashDotVector() );
    updatePatternIcon();
    emit changed();
  }
}

void QgsSimpleLineSymbolLayerV2Widget::on_mPenWidthUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setWidthUnit(( QgsSymbolV2::OutputUnit )index );
  }
}

void QgsSimpleLineSymbolLayerV2Widget::on_mOffsetUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOffsetUnit(( QgsSymbolV2::OutputUnit )index );
  }
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::on_mDashPatternUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setCustomDashPatternUnit(( QgsSymbolV2::OutputUnit )index );
  }
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::on_mDrawInsideCheckBox_stateChanged( int state )
{
  bool checked = ( state == Qt::Checked );
  mLayer->setDrawInsidePolygon( checked );
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "color", tr( "Color" ), mLayer->dataDefinedPropertyString( "color" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "width", tr( "Pen width" ), mLayer->dataDefinedPropertyString( "width" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "offset", tr( "Offset" ), mLayer->dataDefinedPropertyString( "offset" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "customdash", tr( "Dash pattern" ), mLayer->dataDefinedPropertyString( "customdash" ), "<dash>;<space>" );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "joinstyle", tr( "Join style" ), mLayer->dataDefinedPropertyString( "joinstyle" ), "'bevel'|'miter'|'round'" );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "capstyle", tr( "Cap style" ),  mLayer->dataDefinedPropertyString( "capstyle" ), "'square'|'flat'|'round'" );
  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );

  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}

void QgsSimpleLineSymbolLayerV2Widget::updatePatternIcon()
{
  if ( !mLayer )
  {
    return;
  }
  QgsSimpleLineSymbolLayerV2* layerCopy = dynamic_cast<QgsSimpleLineSymbolLayerV2*>( mLayer->clone() );
  if ( !layerCopy )
  {
    return;
  }
  layerCopy->setUseCustomDashPattern( true );
  QIcon buttonIcon = QgsSymbolLayerV2Utils::symbolLayerPreviewIcon( layerCopy, QgsSymbolV2::MM, mChangePatternButton->iconSize() );
  mChangePatternButton->setIcon( buttonIcon );
  delete layerCopy;
}


///////////


QgsSimpleMarkerSymbolLayerV2Widget::QgsSimpleMarkerSymbolLayerV2Widget( const QgsVectorLayer* vl, QWidget* parent )
    : QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = NULL;

  setupUi( this );

  QSize size = lstNames->iconSize();
  QStringList names;
  names << "circle" << "rectangle" << "diamond" << "pentagon" << "cross" << "cross2" << "triangle"
  << "equilateral_triangle" << "star" << "regular_star" << "arrow" << "line" << "arrowhead" << "filled_arrowhead";
  double markerSize = DEFAULT_POINT_SIZE * 2;
  for ( int i = 0; i < names.count(); ++i )
  {
    QgsSimpleMarkerSymbolLayerV2* lyr = new QgsSimpleMarkerSymbolLayerV2( names[i], QColor( 200, 200, 200 ), QColor( 0, 0, 0 ), markerSize );
    QIcon icon = QgsSymbolLayerV2Utils::symbolLayerPreviewIcon( lyr, QgsSymbolV2::MM, size );
    QListWidgetItem* item = new QListWidgetItem( icon, QString(), lstNames );
    item->setData( Qt::UserRole, names[i] );
    delete lyr;
  }

  connect( lstNames, SIGNAL( currentRowChanged( int ) ), this, SLOT( setName() ) );
  connect( btnChangeColorBorder, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( setColorBorder( const QColor& ) ) );
  connect( btnChangeColorFill, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( setColorFill( const QColor& ) ) );
  connect( spinSize, SIGNAL( valueChanged( double ) ), this, SLOT( setSize() ) );
  connect( spinAngle, SIGNAL( valueChanged( double ) ), this, SLOT( setAngle() ) );
  connect( spinOffsetX, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
  connect( spinOffsetY, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
}

void QgsSimpleMarkerSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "SimpleMarker" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsSimpleMarkerSymbolLayerV2*>( layer );

  // set values
  QString name = mLayer->name();
  for ( int i = 0; i < lstNames->count(); ++i )
  {
    if ( lstNames->item( i )->data( Qt::UserRole ).toString() == name )
    {
      lstNames->setCurrentRow( i );
      break;
    }
  }
  btnChangeColorBorder->setColor( mLayer->borderColor() );
  btnChangeColorBorder->setColorDialogOptions( QColorDialog::ShowAlphaChannel );
  btnChangeColorFill->setColor( mLayer->color() );
  btnChangeColorFill->setColorDialogOptions( QColorDialog::ShowAlphaChannel );
  spinSize->setValue( mLayer->size() );
  spinAngle->setValue( mLayer->angle() );
  mOutlineStyleComboBox->setPenStyle( mLayer->outlineStyle() );
  mOutlineWidthSpinBox->setValue( mLayer->outlineWidth() );

  // without blocking signals the value gets changed because of slot setOffset()
  spinOffsetX->blockSignals( true );
  spinOffsetX->setValue( mLayer->offset().x() );
  spinOffsetX->blockSignals( false );
  spinOffsetY->blockSignals( true );
  spinOffsetY->setValue( mLayer->offset().y() );
  spinOffsetY->blockSignals( false );

  mSizeUnitComboBox->blockSignals( true );
  mSizeUnitComboBox->setCurrentIndex( mLayer->sizeUnit() );
  mSizeUnitComboBox->blockSignals( false );
  mOffsetUnitComboBox->blockSignals( true );
  mOffsetUnitComboBox->setCurrentIndex( mLayer->offsetUnit() );
  mOffsetUnitComboBox->blockSignals( false );
  mOutlineWidthUnitComboBox->blockSignals( true );
  mOutlineWidthUnitComboBox->setCurrentIndex( mLayer->outlineWidthUnit() );
  mOutlineWidthUnitComboBox->blockSignals( false );

  //anchor points
  mHorizontalAnchorComboBox->blockSignals( true );
  mVerticalAnchorComboBox->blockSignals( true );
  mHorizontalAnchorComboBox->setCurrentIndex( mLayer->horizontalAnchorPoint() );
  mVerticalAnchorComboBox->setCurrentIndex( mLayer->verticalAnchorPoint() );
  mHorizontalAnchorComboBox->blockSignals( false );
  mVerticalAnchorComboBox->blockSignals( false );
}

QgsSymbolLayerV2* QgsSimpleMarkerSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsSimpleMarkerSymbolLayerV2Widget::setName()
{
  mLayer->setName( lstNames->currentItem()->data( Qt::UserRole ).toString() );
  emit changed();
}

void QgsSimpleMarkerSymbolLayerV2Widget::setColorBorder( const QColor& color )
{
  mLayer->setBorderColor( color );
  emit changed();
}

void QgsSimpleMarkerSymbolLayerV2Widget::setColorFill( const QColor& color )
{
  mLayer->setColor( color );
  emit changed();
}

void QgsSimpleMarkerSymbolLayerV2Widget::setSize()
{
  mLayer->setSize( spinSize->value() );
  emit changed();
}

void QgsSimpleMarkerSymbolLayerV2Widget::setAngle()
{
  mLayer->setAngle( spinAngle->value() );
  emit changed();
}

void QgsSimpleMarkerSymbolLayerV2Widget::setOffset()
{
  mLayer->setOffset( QPointF( spinOffsetX->value(), spinOffsetY->value() ) );
  emit changed();
}

void QgsSimpleMarkerSymbolLayerV2Widget::on_mOutlineStyleComboBox_currentIndexChanged( int index )
{
  Q_UNUSED( index );

  if ( mLayer )
  {
    mLayer->setOutlineStyle( mOutlineStyleComboBox->penStyle() );
    emit changed();
  }
}

void QgsSimpleMarkerSymbolLayerV2Widget::on_mOutlineWidthSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setOutlineWidth( d );
    emit changed();
  }
}

void QgsSimpleMarkerSymbolLayerV2Widget::on_mSizeUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setSizeUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsSimpleMarkerSymbolLayerV2Widget::on_mOffsetUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOffsetUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsSimpleMarkerSymbolLayerV2Widget::on_mOutlineWidthUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOutlineWidthUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsSimpleMarkerSymbolLayerV2Widget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "name", tr( "Name" ), mLayer->dataDefinedPropertyString( "name" ),
      "'square'|'rectangle'|'diamond'|'pentagon'\n|'triangle'|'equilateral_triangle'|'star'\n|'regular_star'|'arrow'|'filled_arrowhead'|'circle'\n|'cross'|'x'|'cross2'|'line'|'arrowhead'" );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "color", tr( "Fill color" ), mLayer->dataDefinedPropertyString( "color" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "color_border", tr( "Border color" ), mLayer->dataDefinedPropertyString( "color_border" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "outline_width", tr( "Outline width" ), mLayer->dataDefinedPropertyString( "outline_width" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "size", dataDefinedPropertyLabel( "size" ), mLayer->dataDefinedPropertyString( "size" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "angle", tr( "Angle" ), mLayer->dataDefinedPropertyString( "angle" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "offset", tr( "Offset" ), mLayer->dataDefinedPropertyString( "offset" ),
      QgsDataDefinedSymbolDialog::offsetHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "horizontal_anchor_point", tr( "Horizontal anchor point" ), mLayer->dataDefinedPropertyString( "horizontal_anchor_point" ),
      QgsDataDefinedSymbolDialog::horizontalAnchorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "vertical_anchor_point", tr( "Vertical anchor point" ), mLayer->dataDefinedPropertyString( "vertical_anchor_point" ),
      QgsDataDefinedSymbolDialog::verticalAnchorHelpText() );
  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );
  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}

void QgsSimpleMarkerSymbolLayerV2Widget::on_mHorizontalAnchorComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setHorizontalAnchorPoint(( QgsMarkerSymbolLayerV2::HorizontalAnchorPoint ) index );
    emit changed();
  }
}

void QgsSimpleMarkerSymbolLayerV2Widget::on_mVerticalAnchorComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setVerticalAnchorPoint(( QgsMarkerSymbolLayerV2::VerticalAnchorPoint ) index );
    emit changed();
  }
}


///////////

QgsSimpleFillSymbolLayerV2Widget::QgsSimpleFillSymbolLayerV2Widget( const QgsVectorLayer* vl, QWidget* parent )
    : QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = NULL;

  setupUi( this );

  connect( btnChangeColor, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( setColor( const QColor& ) ) );
  connect( cboFillStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setBrushStyle() ) );
  connect( btnChangeBorderColor, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( setBorderColor( const QColor& ) ) );
  connect( spinBorderWidth, SIGNAL( valueChanged( double ) ), this, SLOT( borderWidthChanged() ) );
  connect( cboBorderStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( borderStyleChanged() ) );
  connect( spinOffsetX, SIGNAL( valueChanged( double ) ), this, SLOT( offsetChanged() ) );
  connect( spinOffsetY, SIGNAL( valueChanged( double ) ), this, SLOT( offsetChanged() ) );
}

void QgsSimpleFillSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "SimpleFill" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsSimpleFillSymbolLayerV2*>( layer );

  // set values
  btnChangeColor->setColor( mLayer->color() );
  btnChangeColor->setColorDialogOptions( QColorDialog::ShowAlphaChannel );
  cboFillStyle->setBrushStyle( mLayer->brushStyle() );
  btnChangeBorderColor->setColor( mLayer->borderColor() );
  btnChangeBorderColor->setColorDialogOptions( QColorDialog::ShowAlphaChannel );
  cboBorderStyle->setPenStyle( mLayer->borderStyle() );
  spinBorderWidth->setValue( mLayer->borderWidth() );
  spinOffsetX->blockSignals( true );
  spinOffsetX->setValue( mLayer->offset().x() );
  spinOffsetX->blockSignals( false );
  spinOffsetY->blockSignals( true );
  spinOffsetY->setValue( mLayer->offset().y() );
  spinOffsetY->blockSignals( false );

  mBorderWidthUnitComboBox->blockSignals( true );
  mBorderWidthUnitComboBox->setCurrentIndex( mLayer->borderWidthUnit() );
  mBorderWidthUnitComboBox->blockSignals( false );
  mOffsetUnitComboBox->blockSignals( true );
  mOffsetUnitComboBox->setCurrentIndex( mLayer->offsetUnit() );
  mOffsetUnitComboBox->blockSignals( false );
}

QgsSymbolLayerV2* QgsSimpleFillSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsSimpleFillSymbolLayerV2Widget::setColor( const QColor& color )
{
  mLayer->setColor( color );
  emit changed();
}

void QgsSimpleFillSymbolLayerV2Widget::setBorderColor( const QColor& color )
{
  mLayer->setBorderColor( color );
  emit changed();
}

void QgsSimpleFillSymbolLayerV2Widget::setBrushStyle()
{
  mLayer->setBrushStyle( cboFillStyle->brushStyle() );
  emit changed();
}

void QgsSimpleFillSymbolLayerV2Widget::borderWidthChanged()
{
  mLayer->setBorderWidth( spinBorderWidth->value() );
  emit changed();
}

void QgsSimpleFillSymbolLayerV2Widget::borderStyleChanged()
{
  mLayer->setBorderStyle( cboBorderStyle->penStyle() );
  emit changed();
}

void QgsSimpleFillSymbolLayerV2Widget::offsetChanged()
{
  mLayer->setOffset( QPointF( spinOffsetX->value(), spinOffsetY->value() ) );
  emit changed();
}

void QgsSimpleFillSymbolLayerV2Widget::on_mBorderWidthUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setBorderWidthUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsSimpleFillSymbolLayerV2Widget::on_mOffsetUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOffsetUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsSimpleFillSymbolLayerV2Widget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "color", tr( "Color" ), mLayer->dataDefinedPropertyString( "color" ), QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "color_border", tr( "Border color" ), mLayer->dataDefinedPropertyString( "color_border" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "width_border", tr( "Border width" ), mLayer->dataDefinedPropertyString( "width_border" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );
  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}

///////////

QgsGradientFillSymbolLayerV2Widget::QgsGradientFillSymbolLayerV2Widget( const QgsVectorLayer* vl, QWidget* parent )
    : QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = NULL;

  setupUi( this );

  cboGradientColorRamp->setShowGradientOnly( true );
  cboGradientColorRamp->populate( QgsStyleV2::defaultStyle() );

  connect( btnChangeColor, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( setColor( const QColor& ) ) );
  connect( btnChangeColor2, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( setColor2( const QColor& ) ) );
  connect( cboGradientColorRamp, SIGNAL( currentIndexChanged( int ) ) , this, SLOT( applyColorRamp() ) );
  connect( cboGradientType, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setGradientType( int ) ) );
  connect( cboCoordinateMode, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setCoordinateMode( int ) ) );
  connect( cboGradientSpread, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setGradientSpread( int ) ) );
  connect( radioTwoColor, SIGNAL( toggled( bool ) ), this, SLOT( colorModeChanged() ) );
  connect( spinOffsetX, SIGNAL( valueChanged( double ) ), this, SLOT( offsetChanged() ) );
  connect( spinOffsetY, SIGNAL( valueChanged( double ) ), this, SLOT( offsetChanged() ) );
  connect( spinRefPoint1X, SIGNAL( valueChanged( double ) ), this, SLOT( referencePointChanged() ) );
  connect( spinRefPoint1Y, SIGNAL( valueChanged( double ) ), this, SLOT( referencePointChanged() ) );
  connect( checkRefPoint1Centroid, SIGNAL( toggled( bool ) ), this, SLOT( referencePointChanged() ) );
  connect( spinRefPoint2X, SIGNAL( valueChanged( double ) ), this, SLOT( referencePointChanged() ) );
  connect( spinRefPoint2Y, SIGNAL( valueChanged( double ) ), this, SLOT( referencePointChanged() ) );
  connect( checkRefPoint2Centroid, SIGNAL( toggled( bool ) ), this, SLOT( referencePointChanged() ) );
}

void QgsGradientFillSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "GradientFill" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsGradientFillSymbolLayerV2*>( layer );

  // set values
  btnChangeColor->setColor( mLayer->color() );
  btnChangeColor->setColorDialogOptions( QColorDialog::ShowAlphaChannel );
  btnChangeColor2->setColor( mLayer->color2() );
  btnChangeColor2->setColorDialogOptions( QColorDialog::ShowAlphaChannel );

  if ( mLayer->gradientColorType() == QgsGradientFillSymbolLayerV2::SimpleTwoColor )
  {
    radioTwoColor->setChecked( true );
    cboGradientColorRamp->setEnabled( false );
  }
  else
  {
    radioColorRamp->setChecked( true );
    btnChangeColor->setEnabled( false );
    btnChangeColor2->setEnabled( false );
  }

  // set source color ramp
  if ( mLayer->colorRamp() )
  {
    cboGradientColorRamp->blockSignals( true );
    cboGradientColorRamp->setSourceColorRamp( mLayer->colorRamp() );
    cboGradientColorRamp->blockSignals( false );
  }

  cboGradientType->blockSignals( true );
  switch ( mLayer->gradientType() )
  {
    case QgsGradientFillSymbolLayerV2::Linear:
      cboGradientType->setCurrentIndex( 0 );
      break;
    case QgsGradientFillSymbolLayerV2::Radial:
      cboGradientType->setCurrentIndex( 1 );
      break;
    case QgsGradientFillSymbolLayerV2::Conical:
      cboGradientType->setCurrentIndex( 2 );
      break;
  }
  cboGradientType->blockSignals( false );

  cboCoordinateMode->blockSignals( true );
  switch ( mLayer->coordinateMode() )
  {
    case QgsGradientFillSymbolLayerV2::Viewport:
      cboCoordinateMode->setCurrentIndex( 1 );
      checkRefPoint1Centroid->setEnabled( false );
      checkRefPoint2Centroid->setEnabled( false );
      break;
    case QgsGradientFillSymbolLayerV2::Feature:
    default:
      cboCoordinateMode->setCurrentIndex( 0 );
      break;
  }
  cboCoordinateMode->blockSignals( false );

  cboGradientSpread->blockSignals( true );
  switch ( mLayer->gradientSpread() )
  {
    case QgsGradientFillSymbolLayerV2::Pad:
      cboGradientSpread->setCurrentIndex( 0 );
      break;
    case QgsGradientFillSymbolLayerV2::Repeat:
      cboGradientSpread->setCurrentIndex( 1 );
      break;
    case QgsGradientFillSymbolLayerV2::Reflect:
      cboGradientSpread->setCurrentIndex( 2 );
      break;
  }
  cboGradientSpread->blockSignals( false );

  spinRefPoint1X->blockSignals( true );
  spinRefPoint1X->setValue( mLayer->referencePoint1().x() );
  spinRefPoint1X->blockSignals( false );
  spinRefPoint1Y->blockSignals( true );
  spinRefPoint1Y->setValue( mLayer->referencePoint1().y() );
  spinRefPoint1Y->blockSignals( false );
  checkRefPoint1Centroid->blockSignals( true );
  checkRefPoint1Centroid->setChecked( mLayer->referencePoint1IsCentroid() );
  if ( mLayer->referencePoint1IsCentroid() )
  {
    spinRefPoint1X->setEnabled( false );
    spinRefPoint1Y->setEnabled( false );
  }
  checkRefPoint1Centroid->blockSignals( false );
  spinRefPoint2X->blockSignals( true );
  spinRefPoint2X->setValue( mLayer->referencePoint2().x() );
  spinRefPoint2X->blockSignals( false );
  spinRefPoint2Y->blockSignals( true );
  spinRefPoint2Y->setValue( mLayer->referencePoint2().y() );
  spinRefPoint2Y->blockSignals( false );
  checkRefPoint2Centroid->blockSignals( true );
  checkRefPoint2Centroid->setChecked( mLayer->referencePoint2IsCentroid() );
  if ( mLayer->referencePoint2IsCentroid() )
  {
    spinRefPoint2X->setEnabled( false );
    spinRefPoint2Y->setEnabled( false );
  }
  checkRefPoint2Centroid->blockSignals( false );

  spinOffsetX->blockSignals( true );
  spinOffsetX->setValue( mLayer->offset().x() );
  spinOffsetX->blockSignals( false );
  spinOffsetY->blockSignals( true );
  spinOffsetY->setValue( mLayer->offset().y() );
  spinOffsetY->blockSignals( false );
  mSpinAngle->blockSignals( true );
  mSpinAngle->setValue( mLayer->angle() );
  mSpinAngle->blockSignals( false );

  mOffsetUnitComboBox->blockSignals( true );
  mOffsetUnitComboBox->setCurrentIndex( mLayer->offsetUnit() );
  mOffsetUnitComboBox->blockSignals( false );
}

QgsSymbolLayerV2* QgsGradientFillSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsGradientFillSymbolLayerV2Widget::setColor( const QColor& color )
{
  mLayer->setColor( color );
  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::setColor2( const QColor& color )
{
  mLayer->setColor2( color );
  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::colorModeChanged()
{
  if ( radioTwoColor->isChecked() )
  {
    mLayer->setGradientColorType( QgsGradientFillSymbolLayerV2::SimpleTwoColor );
  }
  else
  {
    mLayer->setGradientColorType( QgsGradientFillSymbolLayerV2::ColorRamp );
  }
  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::applyColorRamp()
{
  QgsVectorColorRampV2* ramp = cboGradientColorRamp->currentColorRamp();
  if ( ramp == NULL )
    return;

  mLayer->setColorRamp( ramp );
  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::setGradientType( int index )
{
  switch ( index )
  {
    case 0:
      mLayer->setGradientType( QgsGradientFillSymbolLayerV2::Linear );
      //set sensible default reference points
      spinRefPoint1X->setValue( 0.5 );
      spinRefPoint1Y->setValue( 0 );
      spinRefPoint2X->setValue( 0.5 );
      spinRefPoint2Y->setValue( 1 );
      break;
    case 1:
      mLayer->setGradientType( QgsGradientFillSymbolLayerV2::Radial );
      //set sensible default reference points
      spinRefPoint1X->setValue( 0 );
      spinRefPoint1Y->setValue( 0 );
      spinRefPoint2X->setValue( 1 );
      spinRefPoint2Y->setValue( 1 );
      break;
    case 2:
      mLayer->setGradientType( QgsGradientFillSymbolLayerV2::Conical );
      spinRefPoint1X->setValue( 0.5 );
      spinRefPoint1Y->setValue( 0.5 );
      spinRefPoint2X->setValue( 1 );
      spinRefPoint2Y->setValue( 1 );
      break;
  }
  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::setCoordinateMode( int index )
{

  switch ( index )
  {
    case 0:
      //feature coordinate mode
      mLayer->setCoordinateMode( QgsGradientFillSymbolLayerV2::Feature );
      //allow choice of centroid reference positions
      checkRefPoint1Centroid->setEnabled( true );
      checkRefPoint2Centroid->setEnabled( true );
      break;
    case 1:
      //viewport coordinate mode
      mLayer->setCoordinateMode( QgsGradientFillSymbolLayerV2::Viewport );
      //disable choice of centroid reference positions
      checkRefPoint1Centroid->setChecked( Qt::Unchecked );
      checkRefPoint1Centroid->setEnabled( false );
      checkRefPoint2Centroid->setChecked( Qt::Unchecked );
      checkRefPoint2Centroid->setEnabled( false );
      break;
  }

  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::setGradientSpread( int index )
{
  switch ( index )
  {
    case 0:
      mLayer->setGradientSpread( QgsGradientFillSymbolLayerV2::Pad );
      break;
    case 1:
      mLayer->setGradientSpread( QgsGradientFillSymbolLayerV2::Repeat );
      break;
    case 2:
      mLayer->setGradientSpread( QgsGradientFillSymbolLayerV2::Reflect );
      break;
  }

  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::offsetChanged()
{
  mLayer->setOffset( QPointF( spinOffsetX->value(), spinOffsetY->value() ) );
  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::referencePointChanged()
{
  mLayer->setReferencePoint1( QPointF( spinRefPoint1X->value(), spinRefPoint1Y->value() ) );
  mLayer->setReferencePoint1IsCentroid( checkRefPoint1Centroid->isChecked() );
  mLayer->setReferencePoint2( QPointF( spinRefPoint2X->value(), spinRefPoint2Y->value() ) );
  mLayer->setReferencePoint2IsCentroid( checkRefPoint2Centroid->isChecked() );
  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::on_mSpinAngle_valueChanged( double value )
{
  mLayer->setAngle( value );
  emit changed();
}

void QgsGradientFillSymbolLayerV2Widget::on_mOffsetUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOffsetUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsGradientFillSymbolLayerV2Widget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "color", tr( "Color" ), mLayer->dataDefinedPropertyString( "color" ), QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "color2", tr( "Color 2" ), mLayer->dataDefinedPropertyString( "color2" ), QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "angle", tr( "Angle" ), mLayer->dataDefinedPropertyString( "angle" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "gradient_type", tr( "Gradient type" ), mLayer->dataDefinedPropertyString( "gradient_type" ), QgsDataDefinedSymbolDialog::gradientTypeHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "coordinate_mode", tr( "Coordinate mode" ), mLayer->dataDefinedPropertyString( "coordinate_mode" ), QgsDataDefinedSymbolDialog::gradientCoordModeHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "spread", tr( "Spread" ), mLayer->dataDefinedPropertyString( "spread" ),
      QgsDataDefinedSymbolDialog::gradientSpreadHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "reference1_x", tr( "Reference Point 1 (x)" ), mLayer->dataDefinedPropertyString( "reference1_x" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "reference1_y", tr( "Reference Point 1 (y)" ), mLayer->dataDefinedPropertyString( "reference1_y" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "reference1_iscentroid", tr( "Reference Point 1 (is centroid)" ), mLayer->dataDefinedPropertyString( "reference1_iscentroid" ),
      QgsDataDefinedSymbolDialog::boolHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "reference2_x", tr( "Reference Point 2 (x)" ), mLayer->dataDefinedPropertyString( "reference2_x" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "reference2_y", tr( "Reference Point 2 (y)" ), mLayer->dataDefinedPropertyString( "reference2_y" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "reference2_iscentroid", tr( "Reference Point 2 (is centroid)" ), mLayer->dataDefinedPropertyString( "reference2_iscentroid" ),
      QgsDataDefinedSymbolDialog::boolHelpText() );

  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );
  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}

///////////

QgsMarkerLineSymbolLayerV2Widget::QgsMarkerLineSymbolLayerV2Widget( const QgsVectorLayer* vl, QWidget* parent )
    : QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = NULL;

  setupUi( this );

  connect( spinInterval, SIGNAL( valueChanged( double ) ), this, SLOT( setInterval( double ) ) );
  connect( chkRotateMarker, SIGNAL( clicked() ), this, SLOT( setRotate() ) );
  connect( spinOffset, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
  connect( radInterval, SIGNAL( clicked() ), this, SLOT( setPlacement() ) );
  connect( radVertex, SIGNAL( clicked() ), this, SLOT( setPlacement() ) );
  connect( radVertexLast, SIGNAL( clicked() ), this, SLOT( setPlacement() ) );
  connect( radVertexFirst, SIGNAL( clicked() ), this, SLOT( setPlacement() ) );
  connect( radCentralPoint, SIGNAL( clicked() ), this, SLOT( setPlacement() ) );
}

void QgsMarkerLineSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "MarkerLine" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsMarkerLineSymbolLayerV2*>( layer );

  // set values
  spinInterval->setValue( mLayer->interval() );
  chkRotateMarker->setChecked( mLayer->rotateMarker() );
  spinOffset->setValue( mLayer->offset() );
  if ( mLayer->placement() == QgsMarkerLineSymbolLayerV2::Interval )
    radInterval->setChecked( true );
  else if ( mLayer->placement() == QgsMarkerLineSymbolLayerV2::Vertex )
    radVertex->setChecked( true );
  else if ( mLayer->placement() == QgsMarkerLineSymbolLayerV2::LastVertex )
    radVertexLast->setChecked( true );
  else if ( mLayer->placement() == QgsMarkerLineSymbolLayerV2::CentralPoint )
    radCentralPoint->setChecked( true );
  else
    radVertexFirst->setChecked( true );

  // set units
  mIntervalUnitComboBox->blockSignals( true );
  mIntervalUnitComboBox->setCurrentIndex( mLayer->intervalUnit() );
  mIntervalUnitComboBox->blockSignals( false );
  mOffsetUnitComboBox->blockSignals( true );
  mOffsetUnitComboBox->setCurrentIndex( mLayer->offsetUnit() );
  mOffsetUnitComboBox->blockSignals( false );

  setPlacement(); // update gui
}

QgsSymbolLayerV2* QgsMarkerLineSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsMarkerLineSymbolLayerV2Widget::setInterval( double val )
{
  mLayer->setInterval( val );
  emit changed();
}

void QgsMarkerLineSymbolLayerV2Widget::setRotate()
{
  mLayer->setRotateMarker( chkRotateMarker->isChecked() );
  emit changed();
}

void QgsMarkerLineSymbolLayerV2Widget::setOffset()
{
  mLayer->setOffset( spinOffset->value() );
  emit changed();
}

void QgsMarkerLineSymbolLayerV2Widget::setPlacement()
{
  bool interval = radInterval->isChecked();
  spinInterval->setEnabled( interval );
  //mLayer->setPlacement( interval ? QgsMarkerLineSymbolLayerV2::Interval : QgsMarkerLineSymbolLayerV2::Vertex );
  if ( radInterval->isChecked() )
    mLayer->setPlacement( QgsMarkerLineSymbolLayerV2::Interval );
  else if ( radVertex->isChecked() )
    mLayer->setPlacement( QgsMarkerLineSymbolLayerV2::Vertex );
  else if ( radVertexLast->isChecked() )
    mLayer->setPlacement( QgsMarkerLineSymbolLayerV2::LastVertex );
  else if ( radVertexFirst->isChecked() )
    mLayer->setPlacement( QgsMarkerLineSymbolLayerV2::FirstVertex );
  else
    mLayer->setPlacement( QgsMarkerLineSymbolLayerV2::CentralPoint );

  emit changed();
}

void QgsMarkerLineSymbolLayerV2Widget::on_mIntervalUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setIntervalUnit(( QgsSymbolV2::OutputUnit ) index );
  }
  emit changed();
}

void QgsMarkerLineSymbolLayerV2Widget::on_mOffsetUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOffsetUnit(( QgsSymbolV2::OutputUnit ) index );
  }
  emit changed();
}

void QgsMarkerLineSymbolLayerV2Widget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "interval", tr( "Interval" ), mLayer->dataDefinedPropertyString( "interval" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "offset", tr( "Line offset" ), mLayer->dataDefinedPropertyString( "offset" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "placement", tr( "Placement" ), mLayer->dataDefinedPropertyString( "placement" ),
      tr( "'vertex'|'lastvertex'|'firstvertex'|'centerpoint'" ) );
  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );
  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}

///////////


QgsSvgMarkerSymbolLayerV2Widget::QgsSvgMarkerSymbolLayerV2Widget( const QgsVectorLayer* vl, QWidget* parent )
    : QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = NULL;

  setupUi( this );
  viewGroups->setHeaderHidden( true );

  populateList();

  connect( viewImages->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( setName( const QModelIndex& ) ) );
  connect( viewGroups->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( populateIcons( const QModelIndex& ) ) );
  connect( spinSize, SIGNAL( valueChanged( double ) ), this, SLOT( setSize() ) );
  connect( spinAngle, SIGNAL( valueChanged( double ) ), this, SLOT( setAngle() ) );
  connect( spinOffsetX, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
  connect( spinOffsetY, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
}

#include <QTime>
#include <QAbstractListModel>
#include <QPixmapCache>
#include <QStyle>

class QgsSvgListModel : public QAbstractListModel
{
  public:
    QgsSvgListModel( QObject* parent ) : QAbstractListModel( parent )
    {
      mSvgFiles = QgsSymbolLayerV2Utils::listSvgFiles();
    }

    // Constructor to create model for icons in a specific path
    QgsSvgListModel( QObject* parent, QString path ) : QAbstractListModel( parent )
    {
      mSvgFiles = QgsSymbolLayerV2Utils::listSvgFilesAt( path );
    }

    int rowCount( const QModelIndex & parent = QModelIndex() ) const
    {
      Q_UNUSED( parent );
      return mSvgFiles.count();
    }

    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const
    {
      QString entry = mSvgFiles.at( index.row() );

      if ( role == Qt::DecorationRole ) // icon
      {
        QPixmap pixmap;
        if ( !QPixmapCache::find( entry, pixmap ) )
        {
          // render SVG file
          QColor fill, outline;
          double outlineWidth;
          bool fillParam, outlineParam, outlineWidthParam;
          QgsSvgCache::instance()->containsParams( entry, fillParam, fill, outlineParam, outline, outlineWidthParam, outlineWidth );

          bool fitsInCache; // should always fit in cache at these sizes (i.e. under 559 px ^ 2, or half cache size)
          const QImage& img = QgsSvgCache::instance()->svgAsImage( entry, 30.0, fill, outline, outlineWidth, 3.5 /*appr. 88 dpi*/, 1.0, fitsInCache );
          pixmap = QPixmap::fromImage( img );
          QPixmapCache::insert( entry, pixmap );
        }

        return pixmap;
      }
      else if ( role == Qt::UserRole || role == Qt::ToolTipRole )
      {
        return entry;
      }

      return QVariant();
    }

  protected:
    QStringList mSvgFiles;
};

class QgsSvgGroupsModel : public QStandardItemModel
{
  public:
    QgsSvgGroupsModel( QObject* parent ) : QStandardItemModel( parent )
    {
      QStringList svgPaths = QgsApplication::svgPaths();
      QStandardItem *parentItem = invisibleRootItem();

      for ( int i = 0; i < svgPaths.size(); i++ )
      {
        QDir dir( svgPaths[i] );
        QStandardItem *baseGroup;

        if ( dir.path().contains( QgsApplication::pkgDataPath() ) )
        {
          baseGroup = new QStandardItem( QString( "App Symbols" ) );
        }
        else if ( dir.path().contains( QgsApplication::qgisSettingsDirPath() ) )
        {
          baseGroup = new QStandardItem( QString( "User Symbols" ) );
        }
        else
        {
          baseGroup = new QStandardItem( dir.dirName() );
        }
        baseGroup->setData( QVariant( svgPaths[i] ) );
        baseGroup->setEditable( false );
        baseGroup->setCheckable( false );
        baseGroup->setIcon( QgsApplication::style()->standardIcon( QStyle::SP_DirIcon ) );
        baseGroup->setToolTip( dir.path() );
        parentItem->appendRow( baseGroup );
        createTree( baseGroup );
        QgsDebugMsg( QString( "SVG base path %1: %2" ).arg( i ).arg( baseGroup->data().toString() ) );
      }
    }
  private:
    void createTree( QStandardItem* &parentGroup )
    {
      QDir parentDir( parentGroup->data().toString() );
      foreach ( QString item, parentDir.entryList( QDir::Dirs | QDir::NoDotAndDotDot ) )
      {
        QStandardItem* group = new QStandardItem( item );
        group->setData( QVariant( parentDir.path() + "/" + item ) );
        group->setEditable( false );
        group->setCheckable( false );
        group->setToolTip( parentDir.path() + "/" + item );
        group->setIcon( QgsApplication::style()->standardIcon( QStyle::SP_DirIcon ) );
        parentGroup->appendRow( group );
        createTree( group );
      }
    }
};

void QgsSvgMarkerSymbolLayerV2Widget::populateList()
{
  QgsSvgGroupsModel* g = new QgsSvgGroupsModel( viewGroups );
  viewGroups->setModel( g );
  // Set the tree expanded at the first level
  int rows = g->rowCount( g->indexFromItem( g->invisibleRootItem() ) );
  for ( int i = 0; i < rows; i++ )
  {
    viewGroups->setExpanded( g->indexFromItem( g->item( i ) ), true );
  }

  // Initally load the icons in the List view without any grouping
  QgsSvgListModel* m = new QgsSvgListModel( viewImages );
  viewImages->setModel( m );
}

void QgsSvgMarkerSymbolLayerV2Widget::populateIcons( const QModelIndex& idx )
{
  QString path = idx.data( Qt::UserRole + 1 ).toString();

  QgsSvgListModel* m = new QgsSvgListModel( viewImages, path );
  viewImages->setModel( m );

  connect( viewImages->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( setName( const QModelIndex& ) ) );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::setGuiForSvg( const QgsSvgMarkerSymbolLayerV2* layer )
{
  if ( !layer )
  {
    return;
  }

  //activate gui for svg parameters only if supported by the svg file
  bool hasFillParam, hasOutlineParam, hasOutlineWidthParam;
  QColor defaultFill, defaultOutline;
  double defaultOutlineWidth;
  QgsSvgCache::instance()->containsParams( layer->path(), hasFillParam, defaultFill, hasOutlineParam, defaultOutline, hasOutlineWidthParam, defaultOutlineWidth );
  mChangeColorButton->setEnabled( hasFillParam );
  mChangeBorderColorButton->setEnabled( hasOutlineParam );
  mBorderWidthSpinBox->setEnabled( hasOutlineWidthParam );

  if ( hasFillParam )
  {
    if ( layer->fillColor().isValid() )
    {
      mChangeColorButton->setColor( layer->fillColor() );
    }
    else
    {
      mChangeColorButton->setColor( defaultFill );
    }
  }
  if ( hasOutlineParam )
  {
    if ( layer->outlineColor().isValid() )
    {
      mChangeBorderColorButton->setColor( layer->outlineColor() );
    }
    else
    {
      mChangeBorderColorButton->setColor( defaultOutline );
    }
  }

  mFileLineEdit->blockSignals( true );
  mFileLineEdit->setText( layer->path() );
  mFileLineEdit->blockSignals( false );

  mBorderWidthSpinBox->blockSignals( true );
  mBorderWidthSpinBox->setValue( layer->outlineWidth() );
  mBorderWidthSpinBox->blockSignals( false );
}


void QgsSvgMarkerSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( !layer )
  {
    return;
  }

  if ( layer->layerType() != "SvgMarker" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsSvgMarkerSymbolLayerV2*>( layer );

  // set values

  QAbstractItemModel* m = viewImages->model();
  QItemSelectionModel* selModel = viewImages->selectionModel();
  for ( int i = 0; i < m->rowCount(); i++ )
  {
    QModelIndex idx( m->index( i, 0 ) );
    if ( m->data( idx ).toString() == mLayer->path() )
    {
      selModel->select( idx, QItemSelectionModel::SelectCurrent );
      selModel->setCurrentIndex( idx, QItemSelectionModel::SelectCurrent );
      setName( idx );
      break;
    }
  }

  spinSize->setValue( mLayer->size() );
  spinAngle->setValue( mLayer->angle() );

  // without blocking signals the value gets changed because of slot setOffset()
  spinOffsetX->blockSignals( true );
  spinOffsetX->setValue( mLayer->offset().x() );
  spinOffsetX->blockSignals( false );
  spinOffsetY->blockSignals( true );
  spinOffsetY->setValue( mLayer->offset().y() );
  spinOffsetY->blockSignals( false );

  mSizeUnitComboBox->blockSignals( true );
  mSizeUnitComboBox->setCurrentIndex( mLayer->sizeUnit() );
  mSizeUnitComboBox->blockSignals( false );
  mBorderWidthUnitComboBox->blockSignals( true );
  mBorderWidthUnitComboBox->setCurrentIndex( mLayer->outlineWidthUnit() );
  mBorderWidthUnitComboBox->blockSignals( false );
  mOffsetUnitComboBox->blockSignals( true );
  mOffsetUnitComboBox->setCurrentIndex( mLayer->offsetUnit() );
  mOffsetUnitComboBox->blockSignals( false );

  //anchor points
  mHorizontalAnchorComboBox->blockSignals( true );
  mVerticalAnchorComboBox->blockSignals( true );
  mHorizontalAnchorComboBox->setCurrentIndex( mLayer->horizontalAnchorPoint() );
  mVerticalAnchorComboBox->setCurrentIndex( mLayer->verticalAnchorPoint() );
  mHorizontalAnchorComboBox->blockSignals( false );
  mVerticalAnchorComboBox->blockSignals( false );

  setGuiForSvg( mLayer );
}

QgsSymbolLayerV2* QgsSvgMarkerSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsSvgMarkerSymbolLayerV2Widget::setName( const QModelIndex& idx )
{
  QString name = idx.data( Qt::UserRole ).toString();
  mLayer->setPath( name );
  mFileLineEdit->setText( name );

  setGuiForSvg( mLayer );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::setSize()
{
  mLayer->setSize( spinSize->value() );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::setAngle()
{
  mLayer->setAngle( spinAngle->value() );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::setOffset()
{
  mLayer->setOffset( QPointF( spinOffsetX->value(), spinOffsetY->value() ) );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mFileToolButton_clicked()
{
  QSettings s;
  QString file = QFileDialog::getOpenFileName( 0,
                 tr( "Select SVG file" ),
                 s.value( "/UI/lastSVGMarkerDir" ).toString(),
                 tr( "SVG files" ) + " (*.svg)" );
  QFileInfo fi( file );
  if ( file.isEmpty() || !fi.exists() )
  {
    return;
  }
  mFileLineEdit->setText( file );
  mLayer->setPath( file );
  s.setValue( "/UI/lastSVGMarkerDir", fi.absolutePath() );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mFileLineEdit_textEdited( const QString& text )
{
  if ( !QFileInfo( text ).exists() )
  {
    return;
  }
  mLayer->setPath( text );
  setGuiForSvg( mLayer );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mFileLineEdit_editingFinished()
{
  if ( !QFileInfo( mFileLineEdit->text() ).exists() )
  {
    QUrl url( mFileLineEdit->text() );
    if ( !url.isValid() )
    {
      return;
    }
  }

  QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
  mLayer->setPath( mFileLineEdit->text() );
  QApplication::restoreOverrideCursor();

  setGuiForSvg( mLayer );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mChangeColorButton_colorChanged( const QColor& color )
{
  if ( !mLayer )
  {
    return;
  }

  mLayer->setFillColor( color );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mChangeBorderColorButton_colorChanged( const QColor& color )
{
  if ( !mLayer )
  {
    return;
  }

  mLayer->setOutlineColor( color );
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mBorderWidthSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setOutlineWidth( d );
    emit changed();
  }
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mSizeUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setSizeUnit(( QgsSymbolV2::OutputUnit ) index );
  }
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mBorderWidthUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOutlineWidthUnit(( QgsSymbolV2::OutputUnit ) index );
  }
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mOffsetUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOffsetUnit(( QgsSymbolV2::OutputUnit ) index );
  }
  emit changed();
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "size", dataDefinedPropertyLabel( "size" ), mLayer->dataDefinedPropertyString( "size" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "outline-width", tr( "Border width" ), mLayer->dataDefinedPropertyString( "outline-width" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "angle", tr( "Angle" ), mLayer->dataDefinedPropertyString( "angle" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "offset", tr( "Offset" ), mLayer->dataDefinedPropertyString( "offset" ),
      QgsDataDefinedSymbolDialog::offsetHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "name", tr( "SVG file" ), mLayer->dataDefinedPropertyString( "name" ),
      QgsDataDefinedSymbolDialog::fileNameHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "fill", tr( "Color" ), mLayer->dataDefinedPropertyString( "fill" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "outline", tr( "Border color" ), mLayer->dataDefinedPropertyString( "outline" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "horizontal_anchor_point", tr( "Horizontal anchor point" ), mLayer->dataDefinedPropertyString( "horizontal_anchor_point" ),
      QgsDataDefinedSymbolDialog::horizontalAnchorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "vertical_anchor_point", tr( "Vertical anchor point" ), mLayer->dataDefinedPropertyString( "vertical_anchor_point" ),
      QgsDataDefinedSymbolDialog::verticalAnchorHelpText() );
  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );
  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mHorizontalAnchorComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setHorizontalAnchorPoint( QgsMarkerSymbolLayerV2::HorizontalAnchorPoint( index ) );
    emit changed();
  }
}

void QgsSvgMarkerSymbolLayerV2Widget::on_mVerticalAnchorComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setVerticalAnchorPoint( QgsMarkerSymbolLayerV2::VerticalAnchorPoint( index ) );
    emit changed();
  }
}

/////////////

#include <QFileDialog>

QgsSVGFillSymbolLayerWidget::QgsSVGFillSymbolLayerWidget( const QgsVectorLayer* vl, QWidget* parent ): QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = 0;
  setupUi( this );
  mSvgTreeView->setHeaderHidden( true );
  insertIcons();

  connect( mSvgListView->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( setFile( const QModelIndex& ) ) );
  connect( mSvgTreeView->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( populateIcons( const QModelIndex& ) ) );
}

void QgsSVGFillSymbolLayerWidget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( !layer )
  {
    return;
  }

  if ( layer->layerType() != "SVGFill" )
  {
    return;
  }

  mLayer = dynamic_cast<QgsSVGFillSymbolLayer*>( layer );
  if ( mLayer )
  {
    double width = mLayer->patternWidth();
    mTextureWidthSpinBox->setValue( width );
    mSVGLineEdit->setText( mLayer->svgFilePath() );
    mRotationSpinBox->setValue( mLayer->angle() );
    mTextureWidthUnitComboBox->blockSignals( true );
    mTextureWidthUnitComboBox->setCurrentIndex( mLayer->patternWidthUnit() );
    mTextureWidthUnitComboBox->blockSignals( false );
    mSvgOutlineWidthUnitComboBox->blockSignals( true );
    mSvgOutlineWidthUnitComboBox->setCurrentIndex( mLayer->svgOutlineWidthUnit() );
    mSvgOutlineWidthUnitComboBox->blockSignals( false );
  }
  updateParamGui();
}

QgsSymbolLayerV2* QgsSVGFillSymbolLayerWidget::symbolLayer()
{
  return mLayer;
}

void QgsSVGFillSymbolLayerWidget::on_mBrowseToolButton_clicked()
{
  QString filePath = QFileDialog::getOpenFileName( 0, tr( "Select svg texture file" ) );
  if ( !filePath.isNull() )
  {
    mSVGLineEdit->setText( filePath );
    emit changed();
  }
}

void QgsSVGFillSymbolLayerWidget::on_mTextureWidthSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setPatternWidth( d );
    emit changed();
  }
}

void QgsSVGFillSymbolLayerWidget::on_mSVGLineEdit_textEdited( const QString & text )
{
  if ( !mLayer )
  {
    return;
  }

  QFileInfo fi( text );
  if ( !fi.exists() )
  {
    return;
  }
  mLayer->setSvgFilePath( text );
  updateParamGui();
  emit changed();
}

void QgsSVGFillSymbolLayerWidget::on_mSVGLineEdit_editingFinished()
{
  if ( !mLayer )
  {
    return;
  }

  QFileInfo fi( mSVGLineEdit->text() );
  if ( !fi.exists() )
  {
    QUrl url( mSVGLineEdit->text() );
    if ( !url.isValid() )
    {
      return;
    }
  }

  QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
  mLayer->setSvgFilePath( mSVGLineEdit->text() );
  QApplication::restoreOverrideCursor();

  updateParamGui();
  emit changed();
}

void QgsSVGFillSymbolLayerWidget::setFile( const QModelIndex& item )
{
  QString file = item.data( Qt::UserRole ).toString();
  mLayer->setSvgFilePath( file );
  mSVGLineEdit->setText( file );

  updateParamGui();
  emit changed();
}

void QgsSVGFillSymbolLayerWidget::insertIcons()
{
  QgsSvgGroupsModel* g = new QgsSvgGroupsModel( mSvgTreeView );
  mSvgTreeView->setModel( g );
  // Set the tree expanded at the first level
  int rows = g->rowCount( g->indexFromItem( g->invisibleRootItem() ) );
  for ( int i = 0; i < rows; i++ )
  {
    mSvgTreeView->setExpanded( g->indexFromItem( g->item( i ) ), true );
  }

  QgsSvgListModel* m = new QgsSvgListModel( mSvgListView );
  mSvgListView->setModel( m );
}

void QgsSVGFillSymbolLayerWidget::populateIcons( const QModelIndex& idx )
{
  QString path = idx.data( Qt::UserRole + 1 ).toString();

  QgsSvgListModel* m = new QgsSvgListModel( mSvgListView, path );
  mSvgListView->setModel( m );

  connect( mSvgListView->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( setFile( const QModelIndex& ) ) );
  emit changed();
}


void QgsSVGFillSymbolLayerWidget::on_mRotationSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setAngle( d );
    emit changed();
  }
}

void QgsSVGFillSymbolLayerWidget::updateParamGui()
{
  //activate gui for svg parameters only if supported by the svg file
  bool hasFillParam, hasOutlineParam, hasOutlineWidthParam;
  QColor defaultFill, defaultOutline;
  double defaultOutlineWidth;
  QgsSvgCache::instance()->containsParams( mSVGLineEdit->text(), hasFillParam, defaultFill, hasOutlineParam, defaultOutline, hasOutlineWidthParam, defaultOutlineWidth );
  if ( hasFillParam )
    mChangeColorButton->setColor( defaultFill );
  mChangeColorButton->setEnabled( hasFillParam );
  if ( hasOutlineParam )
    mChangeBorderColorButton->setColor( defaultOutline );
  mChangeBorderColorButton->setEnabled( hasOutlineParam );
  mBorderWidthSpinBox->setEnabled( hasOutlineWidthParam );
}

void QgsSVGFillSymbolLayerWidget::on_mChangeColorButton_colorChanged( const QColor& color )
{
  if ( !mLayer )
  {
    return;
  }

  mLayer->setSvgFillColor( color );
  emit changed();
}

void QgsSVGFillSymbolLayerWidget::on_mChangeBorderColorButton_colorChanged( const QColor& color )
{
  if ( !mLayer )
  {
    return;
  }

  mLayer->setSvgOutlineColor( color );
  emit changed();
}

void QgsSVGFillSymbolLayerWidget::on_mBorderWidthSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setSvgOutlineWidth( d );
    emit changed();
  }
}

void QgsSVGFillSymbolLayerWidget::on_mTextureWidthUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setPatternWidthUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsSVGFillSymbolLayerWidget::on_mSvgOutlineWidthUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setSvgOutlineWidthUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsSVGFillSymbolLayerWidget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "width", tr( "Texture width" ), mLayer->dataDefinedPropertyString( "width" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "svgFile", tr( "SVG file" ), mLayer->dataDefinedPropertyString( "svgFile" ),
      QgsDataDefinedSymbolDialog::fileNameHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "angle", tr( "Rotation" ), mLayer->dataDefinedPropertyString( "angle" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "svgFillColor", tr( "Color" ), mLayer->dataDefinedPropertyString( "svgFillColor" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "svgOutlineColor", tr( "Border color" ), mLayer->dataDefinedPropertyString( "svgOutlineColor" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "svgOutlineWidth", tr( "Border width" ), mLayer->dataDefinedPropertyString( "svgOutlineWidth" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );
  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}

/////////////

QgsLinePatternFillSymbolLayerWidget::QgsLinePatternFillSymbolLayerWidget( const QgsVectorLayer* vl, QWidget* parent ):
    QgsSymbolLayerV2Widget( parent, vl ), mLayer( 0 )
{
  setupUi( this );
}

void QgsLinePatternFillSymbolLayerWidget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "LinePatternFill" )
  {
    return;
  }

  QgsLinePatternFillSymbolLayer* patternLayer = static_cast<QgsLinePatternFillSymbolLayer*>( layer );
  if ( patternLayer )
  {
    mLayer = patternLayer;
    mAngleSpinBox->setValue( mLayer->lineAngle() );
    mDistanceSpinBox->setValue( mLayer->distance() );
    mOffsetSpinBox->setValue( mLayer->offset() );

    //units
    mDistanceUnitComboBox->blockSignals( true );
    mDistanceUnitComboBox->setCurrentIndex( mLayer->distanceUnit() );
    mDistanceUnitComboBox->blockSignals( false );
    mOffsetUnitComboBox->blockSignals( true );
    mOffsetUnitComboBox->setCurrentIndex( mLayer->offsetUnit() );
    mOffsetUnitComboBox->blockSignals( false );
  }
}

QgsSymbolLayerV2* QgsLinePatternFillSymbolLayerWidget::symbolLayer()
{
  return mLayer;
}

void QgsLinePatternFillSymbolLayerWidget::on_mAngleSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setLineAngle( d );
    emit changed();
  }
}

void QgsLinePatternFillSymbolLayerWidget::on_mDistanceSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setDistance( d );
    emit changed();
  }
}

void QgsLinePatternFillSymbolLayerWidget::on_mOffsetSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setOffset( d );
    emit changed();
  }
}

void QgsLinePatternFillSymbolLayerWidget::on_mDistanceUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setDistanceUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsLinePatternFillSymbolLayerWidget::on_mOffsetUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOffsetUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsLinePatternFillSymbolLayerWidget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "lineangle",  tr( "Angle" ), mLayer->dataDefinedPropertyString( "lineangle" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "distance", tr( "Distance" ), mLayer->dataDefinedPropertyString( "distance" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "linewidth", tr( "Line width" ), mLayer->dataDefinedPropertyString( "linewidth" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "color", tr( "Color" ), mLayer->dataDefinedPropertyString( "color" ),
      QgsDataDefinedSymbolDialog::colorHelpText() );
  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );
  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}


/////////////

QgsPointPatternFillSymbolLayerWidget::QgsPointPatternFillSymbolLayerWidget( const QgsVectorLayer* vl, QWidget* parent ):
    QgsSymbolLayerV2Widget( parent, vl ), mLayer( 0 )
{
  setupUi( this );
}


void QgsPointPatternFillSymbolLayerWidget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( !layer || layer->layerType() != "PointPatternFill" )
  {
    return;
  }

  mLayer = static_cast<QgsPointPatternFillSymbolLayer*>( layer );
  mHorizontalDistanceSpinBox->setValue( mLayer->distanceX() );
  mVerticalDistanceSpinBox->setValue( mLayer->distanceY() );
  mHorizontalDisplacementSpinBox->setValue( mLayer->displacementX() );
  mVerticalDisplacementSpinBox->setValue( mLayer->displacementY() );

  mHorizontalDistanceUnitComboBox->blockSignals( true );
  mHorizontalDistanceUnitComboBox->setCurrentIndex( mLayer->distanceXUnit() );
  mHorizontalDistanceUnitComboBox->blockSignals( false );
  mVerticalDistanceUnitComboBox->blockSignals( true );
  mVerticalDistanceUnitComboBox->setCurrentIndex( mLayer->distanceYUnit() );
  mVerticalDistanceUnitComboBox->blockSignals( false );
  mHorizontalDisplacementUnitComboBox->blockSignals( true );
  mHorizontalDisplacementUnitComboBox->setCurrentIndex( mLayer->displacementXUnit() );
  mHorizontalDisplacementUnitComboBox->blockSignals( false );
  mVerticalDisplacementUnitComboBox->blockSignals( true );
  mVerticalDisplacementUnitComboBox->setCurrentIndex( mLayer->displacementYUnit() );
  mVerticalDisplacementUnitComboBox->blockSignals( false );
}

QgsSymbolLayerV2* QgsPointPatternFillSymbolLayerWidget::symbolLayer()
{
  return mLayer;
}

void QgsPointPatternFillSymbolLayerWidget::on_mHorizontalDistanceSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setDistanceX( d );
    emit changed();
  }
}

void QgsPointPatternFillSymbolLayerWidget::on_mVerticalDistanceSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setDistanceY( d );
    emit changed();
  }
}

void QgsPointPatternFillSymbolLayerWidget::on_mHorizontalDisplacementSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setDisplacementX( d );
    emit changed();
  }
}

void QgsPointPatternFillSymbolLayerWidget::on_mVerticalDisplacementSpinBox_valueChanged( double d )
{
  if ( mLayer )
  {
    mLayer->setDisplacementY( d );
    emit changed();
  }
}

void QgsPointPatternFillSymbolLayerWidget::on_mHorizontalDistanceUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setDistanceXUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsPointPatternFillSymbolLayerWidget::on_mVerticalDistanceUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setDistanceYUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsPointPatternFillSymbolLayerWidget::on_mHorizontalDisplacementUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setDisplacementXUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsPointPatternFillSymbolLayerWidget::on_mVerticalDisplacementUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setDisplacementYUnit(( QgsSymbolV2::OutputUnit ) index );
    emit changed();
  }
}

void QgsPointPatternFillSymbolLayerWidget::on_mDataDefinedPropertiesButton_clicked()
{
  if ( !mLayer )
  {
    return;
  }

  QList< QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry > dataDefinedProperties;
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "distance_x", tr( "Horizontal distance" ), mLayer->dataDefinedPropertyString( "distance_x" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "distance_y", tr( "Vertical distance" ), mLayer->dataDefinedPropertyString( "distance_y" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "displacement_x", tr( "Horizontal displacement" ), mLayer->dataDefinedPropertyString( "displacement_x" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  dataDefinedProperties << QgsDataDefinedSymbolDialog::DataDefinedSymbolEntry( "displacement_y", tr( "Vertical displacement" ), mLayer->dataDefinedPropertyString( "displacement_y" ),
      QgsDataDefinedSymbolDialog::doubleHelpText() );
  QgsDataDefinedSymbolDialog d( dataDefinedProperties, mVectorLayer );
  if ( d.exec() == QDialog::Accepted )
  {
    //empty all existing properties first
    mLayer->removeDataDefinedProperties();

    QMap<QString, QString> properties = d.dataDefinedProperties();
    QMap<QString, QString>::const_iterator it = properties.constBegin();
    for ( ; it != properties.constEnd(); ++it )
    {
      if ( !it.value().isEmpty() )
      {
        mLayer->setDataDefinedProperty( it.key(), it.value() );
      }
    }
    emit changed();
  }
}

/////////////

QgsFontMarkerSymbolLayerV2Widget::QgsFontMarkerSymbolLayerV2Widget( const QgsVectorLayer* vl, QWidget* parent )
    : QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = NULL;

  setupUi( this );
  widgetChar = new CharacterWidget;
  scrollArea->setWidget( widgetChar );

  connect( cboFont, SIGNAL( currentFontChanged( const QFont & ) ), this, SLOT( setFontFamily( const QFont& ) ) );
  connect( spinSize, SIGNAL( valueChanged( double ) ), this, SLOT( setSize( double ) ) );
  connect( btnColor, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( setColor( const QColor& ) ) );
  connect( spinAngle, SIGNAL( valueChanged( double ) ), this, SLOT( setAngle( double ) ) );
  connect( spinOffsetX, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
  connect( spinOffsetY, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
  connect( widgetChar, SIGNAL( characterSelected( const QChar & ) ), this, SLOT( setCharacter( const QChar & ) ) );
}


void QgsFontMarkerSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "FontMarker" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsFontMarkerSymbolLayerV2*>( layer );

  // set values
  cboFont->setCurrentFont( QFont( mLayer->fontFamily() ) );
  spinSize->setValue( mLayer->size() );
  btnColor->setColor( mLayer->color() );
  btnColor->setColorDialogOptions( QColorDialog::ShowAlphaChannel );
  spinAngle->setValue( mLayer->angle() );

  //block
  spinOffsetX->blockSignals( true );
  spinOffsetX->setValue( mLayer->offset().x() );
  spinOffsetX->blockSignals( false );
  spinOffsetY->blockSignals( true );
  spinOffsetY->setValue( mLayer->offset().y() );
  spinOffsetY->blockSignals( false );

  mSizeUnitComboBox->blockSignals( true );
  mSizeUnitComboBox->setCurrentIndex( mLayer->sizeUnit() );
  mSizeUnitComboBox->blockSignals( false );

  mOffsetUnitComboBox->blockSignals( true );
  mOffsetUnitComboBox->setCurrentIndex( mLayer->offsetUnit() );
  mOffsetUnitComboBox->blockSignals( false );

  //anchor points
  mHorizontalAnchorComboBox->blockSignals( true );
  mVerticalAnchorComboBox->blockSignals( true );
  mHorizontalAnchorComboBox->setCurrentIndex( mLayer->horizontalAnchorPoint() );
  mVerticalAnchorComboBox->setCurrentIndex( mLayer->verticalAnchorPoint() );
  mHorizontalAnchorComboBox->blockSignals( false );
  mVerticalAnchorComboBox->blockSignals( false );
}

QgsSymbolLayerV2* QgsFontMarkerSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsFontMarkerSymbolLayerV2Widget::setFontFamily( const QFont& font )
{
  mLayer->setFontFamily( font.family() );
  widgetChar->updateFont( font );
  emit changed();
}

void QgsFontMarkerSymbolLayerV2Widget::setColor( const QColor& color )
{
  mLayer->setColor( color );
  emit changed();
}

void QgsFontMarkerSymbolLayerV2Widget::setSize( double size )
{
  mLayer->setSize( size );
  //widgetChar->updateSize(size);
  emit changed();
}

void QgsFontMarkerSymbolLayerV2Widget::setAngle( double angle )
{
  mLayer->setAngle( angle );
  emit changed();
}

void QgsFontMarkerSymbolLayerV2Widget::setCharacter( const QChar& chr )
{
  mLayer->setCharacter( chr );
  emit changed();
}

void QgsFontMarkerSymbolLayerV2Widget::setOffset()
{
  mLayer->setOffset( QPointF( spinOffsetX->value(), spinOffsetY->value() ) );
  emit changed();
}

void QgsFontMarkerSymbolLayerV2Widget::on_mSizeUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setSizeUnit(( QgsSymbolV2::OutputUnit ) index );
  }
  emit changed();
}

void QgsFontMarkerSymbolLayerV2Widget::on_mOffsetUnitComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setOffsetUnit(( QgsSymbolV2::OutputUnit ) index );
  }
  emit changed();
}

void QgsFontMarkerSymbolLayerV2Widget::on_mHorizontalAnchorComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setHorizontalAnchorPoint( QgsMarkerSymbolLayerV2::HorizontalAnchorPoint( index ) );
    emit changed();
  }
}

void QgsFontMarkerSymbolLayerV2Widget::on_mVerticalAnchorComboBox_currentIndexChanged( int index )
{
  if ( mLayer )
  {
    mLayer->setVerticalAnchorPoint( QgsMarkerSymbolLayerV2::VerticalAnchorPoint( index ) );
    emit changed();
  }
}


///////////////


QgsCentroidFillSymbolLayerV2Widget::QgsCentroidFillSymbolLayerV2Widget( const QgsVectorLayer* vl, QWidget* parent )
    : QgsSymbolLayerV2Widget( parent, vl )
{
  mLayer = NULL;

  setupUi( this );
}

void QgsCentroidFillSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "CentroidFill" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsCentroidFillSymbolLayerV2*>( layer );
}

QgsSymbolLayerV2* QgsCentroidFillSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

