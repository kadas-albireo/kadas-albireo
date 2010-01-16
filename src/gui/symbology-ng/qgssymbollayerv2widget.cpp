
#include "qgssymbollayerv2widget.h"

#include "qgslinesymbollayerv2.h"
#include "qgsmarkersymbollayerv2.h"
#include "qgsfillsymbollayerv2.h"

#include "qgssymbolv2propertiesdialog.h"

#include "qgsapplication.h"

#include <QAbstractButton>
#include <QColorDialog>
#include <QDir>
#include <QPainter>
#include <QStandardItemModel>
#include <QSvgRenderer>



QgsSimpleLineSymbolLayerV2Widget::QgsSimpleLineSymbolLayerV2Widget( QWidget* parent )
    : QgsSymbolLayerV2Widget( parent )
{
  mLayer = NULL;

  setupUi( this );

  connect( spinWidth, SIGNAL( valueChanged( double ) ), this, SLOT( penWidthChanged() ) );
  connect( btnChangeColor, SIGNAL( clicked() ), this, SLOT( colorChanged() ) );
  connect( cboPenStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( penStyleChanged() ) );
  connect( spinOffset, SIGNAL( valueChanged( double ) ), this, SLOT( offsetChanged() ) );
  connect( cboCapStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( penStyleChanged() ) );
  connect( cboJoinStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( penStyleChanged() ) );

}

void QgsSimpleLineSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "SimpleLine" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsSimpleLineSymbolLayerV2*>( layer );

  // set values
  spinWidth->setValue( mLayer->width() );
  btnChangeColor->setColor( mLayer->color() );
  cboPenStyle->setPenStyle( mLayer->penStyle() );
  spinOffset->setValue( mLayer->offset() );
  cboJoinStyle->setPenJoinStyle( mLayer->penJoinStyle() );
  cboCapStyle->setPenCapStyle( mLayer->penCapStyle() );
}

QgsSymbolLayerV2* QgsSimpleLineSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsSimpleLineSymbolLayerV2Widget::penWidthChanged()
{
  mLayer->setWidth( spinWidth->value() );
  emit changed();
}

void QgsSimpleLineSymbolLayerV2Widget::colorChanged()
{
  QColor color = QColorDialog::getColor( mLayer->color(), this );
  if ( !color.isValid() )
    return;
  mLayer->setColor( color );
  btnChangeColor->setColor( mLayer->color() );
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
  emit changed();
}


///////////


QgsSimpleMarkerSymbolLayerV2Widget::QgsSimpleMarkerSymbolLayerV2Widget( QWidget* parent )
    : QgsSymbolLayerV2Widget( parent )
{
  mLayer = NULL;

  setupUi( this );

  QSize size = lstNames->iconSize();
  QStringList names;
  names << "circle" << "rectangle" << "diamond" << "pentagon" << "cross" << "cross2" << "triangle"
  << "equilateral_triangle" << "star" << "regular_star" << "arrow";
  double markerSize = size.width() - 1; // keep some space around
  for ( int i = 0; i < names.count(); ++i )
  {
    QgsSimpleMarkerSymbolLayerV2* lyr = new QgsSimpleMarkerSymbolLayerV2( names[i], QColor( 200, 200, 200 ), QColor( 0, 0, 0 ), markerSize );
    QIcon icon = QgsSymbolLayerV2Utils::symbolLayerPreviewIcon( lyr, QgsSymbolV2::MM, size ); //todo: unit needs to be a parameter
    QListWidgetItem* item = new QListWidgetItem( icon, QString(), lstNames );
    item->setData( Qt::UserRole, names[i] );
    delete lyr;
  }

  connect( lstNames, SIGNAL( currentRowChanged( int ) ), this, SLOT( setName() ) );
  connect( btnChangeColorBorder, SIGNAL( clicked() ), this, SLOT( setColorBorder() ) );
  connect( btnChangeColorFill, SIGNAL( clicked() ), this, SLOT( setColorFill() ) );
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
  btnChangeColorFill->setColor( mLayer->color() );
  spinSize->setValue( mLayer->size() );
  spinAngle->setValue( mLayer->angle() );

  // without blocking signals the value gets changed because of slot setOffset()
  spinOffsetX->blockSignals( true );
  spinOffsetX->setValue( mLayer->offset().x() );
  spinOffsetX->blockSignals( false );
  spinOffsetY->blockSignals( true );
  spinOffsetY->setValue( mLayer->offset().y() );
  spinOffsetY->blockSignals( false );
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

void QgsSimpleMarkerSymbolLayerV2Widget::setColorBorder()
{
  QColor borderColor = QColorDialog::getColor( mLayer->borderColor(), this );
  if ( !borderColor.isValid() )
    return;
  mLayer->setBorderColor( borderColor );
  btnChangeColorBorder->setColor( mLayer->borderColor() );
  emit changed();
}

void QgsSimpleMarkerSymbolLayerV2Widget::setColorFill()
{
  QColor color = QColorDialog::getColor( mLayer->color(), this );
  if ( !color.isValid() )
    return;
  mLayer->setColor( color );
  btnChangeColorFill->setColor( mLayer->color() );
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


///////////

QgsSimpleFillSymbolLayerV2Widget::QgsSimpleFillSymbolLayerV2Widget( QWidget* parent )
    : QgsSymbolLayerV2Widget( parent )
{
  mLayer = NULL;

  setupUi( this );

  connect( btnChangeColor, SIGNAL( clicked() ), this, SLOT( setColor() ) );
  connect( cboFillStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( setBrushStyle() ) );
  connect( btnChangeBorderColor, SIGNAL( clicked() ), this, SLOT( setBorderColor() ) );
  connect( spinBorderWidth, SIGNAL( valueChanged( double ) ), this, SLOT( borderWidthChanged() ) );
  connect( cboBorderStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( borderStyleChanged() ) );
}

void QgsSimpleFillSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "SimpleFill" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsSimpleFillSymbolLayerV2*>( layer );

  // set values
  btnChangeColor->setColor( mLayer->color() );
  cboFillStyle->setBrushStyle( mLayer->brushStyle() );
  btnChangeBorderColor->setColor( mLayer->borderColor() );
  cboBorderStyle->setPenStyle( mLayer->borderStyle() );
  spinBorderWidth->setValue( mLayer->borderWidth() );
}

QgsSymbolLayerV2* QgsSimpleFillSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsSimpleFillSymbolLayerV2Widget::setColor()
{
  QColor color = QColorDialog::getColor( mLayer->color(), this );
  if ( !color.isValid() )
    return;
  mLayer->setColor( color );
  btnChangeColor->setColor( mLayer->color() );
  emit changed();
}

void QgsSimpleFillSymbolLayerV2Widget::setBorderColor()
{
  QColor color = QColorDialog::getColor( mLayer->borderColor(), this );
  if ( !color.isValid() )
    return;
  mLayer->setBorderColor( color );
  btnChangeBorderColor->setColor( mLayer->borderColor() );
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

///////////

QgsMarkerLineSymbolLayerV2Widget::QgsMarkerLineSymbolLayerV2Widget( QWidget* parent )
    : QgsSymbolLayerV2Widget( parent )
{
  mLayer = NULL;

  setupUi( this );

  connect( spinInterval, SIGNAL( valueChanged( double ) ), this, SLOT( setInterval( double ) ) );
  connect( btnChangeMarker, SIGNAL( clicked() ), this, SLOT( setMarker() ) );
  connect( chkRotateMarker, SIGNAL( clicked() ), this, SLOT( setRotate() ) );
  connect( spinOffset, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
}

void QgsMarkerLineSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "MarkerLine" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsMarkerLineSymbolLayerV2*>( layer );

  // set values
  spinInterval->setValue(( int ) mLayer->interval() );
  chkRotateMarker->setChecked( mLayer->rotateMarker() );
  spinOffset->setValue( mLayer->offset() );
  updateMarker();
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

void QgsMarkerLineSymbolLayerV2Widget::setMarker()
{
  QgsSymbolV2PropertiesDialog dlg( mLayer->subSymbol(), this );
  if ( dlg.exec() == 0 )
    return;
  updateMarker();

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


void QgsMarkerLineSymbolLayerV2Widget::updateMarker()
{
  QIcon icon = QgsSymbolLayerV2Utils::symbolPreviewIcon( mLayer->subSymbol(), btnChangeMarker->iconSize() );
  btnChangeMarker->setIcon( icon );
}


///////////


QgsSvgMarkerSymbolLayerV2Widget::QgsSvgMarkerSymbolLayerV2Widget( QWidget* parent )
    : QgsSymbolLayerV2Widget( parent )
{
  mLayer = NULL;

  setupUi( this );

  populateList();

  connect( viewImages->selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( setName( const QModelIndex& ) ) );
  connect( spinSize, SIGNAL( valueChanged( double ) ), this, SLOT( setSize() ) );
  connect( spinAngle, SIGNAL( valueChanged( double ) ), this, SLOT( setAngle() ) );
  connect( spinOffsetX, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
  connect( spinOffsetY, SIGNAL( valueChanged( double ) ), this, SLOT( setOffset() ) );
}

void QgsSvgMarkerSymbolLayerV2Widget::populateList()
{
  QStandardItemModel* m = new QStandardItemModel( viewImages );
  viewImages->setModel( m );

  QSvgRenderer renderer;
  QPainter painter;

  foreach( QString entry, QgsSvgMarkerSymbolLayerV2::listSvgFiles() )
  {
    // render SVG file
    renderer.load( entry );
    QPixmap pixmap( renderer.defaultSize() );
    pixmap.fill();
    painter.begin( &pixmap );
    renderer.render( &painter );
    painter.end();

    // add item
    QStandardItem* item = new QStandardItem( QIcon( pixmap ), QString() );
    item->setData( entry, Qt::UserRole );
    item->setToolTip( entry );
    m->appendRow( item );
  }

}


void QgsSvgMarkerSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "SvgMarker" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsSvgMarkerSymbolLayerV2*>( layer );

  // set values

  QStandardItemModel* m = static_cast<QStandardItemModel*>( viewImages->model() );
  for ( int i = 0; i < m->rowCount(); i++ )
  {
    QStandardItem* item = m->item( i, 0 );
    if ( item->data( Qt::UserRole ).toString() == mLayer->path() )
    {
      viewImages->selectionModel()->select( item->index(), QItemSelectionModel::SelectCurrent );
      viewImages->selectionModel()->setCurrentIndex( item->index(), QItemSelectionModel::SelectCurrent );
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
}

QgsSymbolLayerV2* QgsSvgMarkerSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsSvgMarkerSymbolLayerV2Widget::setName( const QModelIndex& idx )
{
  mLayer->setPath( idx.data( Qt::UserRole ).toString() );

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

///////////////

QgsLineDecorationSymbolLayerV2Widget::QgsLineDecorationSymbolLayerV2Widget( QWidget* parent )
    : QgsSymbolLayerV2Widget( parent )
{
  mLayer = NULL;

  setupUi( this );

  connect( btnChangeColor, SIGNAL( clicked() ), this, SLOT( colorChanged() ) );
}

void QgsLineDecorationSymbolLayerV2Widget::setSymbolLayer( QgsSymbolLayerV2* layer )
{
  if ( layer->layerType() != "LineDecoration" )
    return;

  // layer type is correct, we can do the cast
  mLayer = static_cast<QgsLineDecorationSymbolLayerV2*>( layer );

  // set values
  btnChangeColor->setColor( mLayer->color() );
}

QgsSymbolLayerV2* QgsLineDecorationSymbolLayerV2Widget::symbolLayer()
{
  return mLayer;
}

void QgsLineDecorationSymbolLayerV2Widget::colorChanged()
{
  QColor color = QColorDialog::getColor( mLayer->color(), this );
  if ( !color.isValid() )
    return;
  mLayer->setColor( color );
  btnChangeColor->setColor( mLayer->color() );
  emit changed();
}

/////////////

#include <QFileDialog>

QgsSVGFillSymbolLayerWidget::QgsSVGFillSymbolLayerWidget( QWidget* parent ): QgsSymbolLayerV2Widget( parent )
{
  mLayer = 0;
  setupUi( this );
  insertIcons();
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
  }
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

void QgsSVGFillSymbolLayerWidget::on_mSVGLineEdit_textChanged( const QString & text )
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
  emit changed();
}

void QgsSVGFillSymbolLayerWidget::on_mSvgListWidget_itemActivated( QListWidgetItem* item )
{
  mSVGLineEdit->setText( item->data( Qt::UserRole ).toString() );
}

void QgsSVGFillSymbolLayerWidget::insertIcons()
{
  mSvgListWidget->clear();

  QStringList svgFiles = QgsSvgMarkerSymbolLayerV2::listSvgFiles();
  QSvgRenderer renderer;
  QPainter painter;

  QStringList::const_iterator it = svgFiles.constBegin();
  for ( ; it != svgFiles.constEnd(); ++it )
  {
    renderer.load( *it );
    QPixmap pixmap( renderer.defaultSize() );
    pixmap.fill();
    painter.begin( &pixmap );
    renderer.render( &painter );
    painter.end();

    QListWidgetItem* item = new QListWidgetItem( mSvgListWidget );
    item->setData( Qt::UserRole, *it );
    item->setIcon( QIcon( pixmap ) );
    item->setToolTip( *it );
  }
}

void QgsSVGFillSymbolLayerWidget::on_mChangeOutlinePushButton_clicked()
{
  QgsSymbolV2PropertiesDialog dlg( mLayer->subSymbol(), this );
  if ( dlg.exec() == 0 )
  {
    return;
  }

  emit changed();
}
