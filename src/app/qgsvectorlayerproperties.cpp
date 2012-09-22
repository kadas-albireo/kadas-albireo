/***************************************************************************
                          qgsdlgvectorlayerproperties.cpp
                   Unified property dialog for vector layers
                             -------------------
    begin                : 2004-01-28
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <memory>
#include <limits>

#include "qgisapp.h"
#include "qgsaddjoindialog.h"
#include "qgsapplication.h"
#include "qgsattributeactiondialog.h"
#include "qgsapplydialog.h"
#include "qgscontexthelp.h"
#include "qgscontinuouscolordialog.h"
#include "qgscoordinatetransform.h"
#include "qgsdiagramproperties.h"
#include "qgsdiagramrendererv2.h"
#include "qgsfieldcalculator.h"
#include "qgsgraduatedsymboldialog.h"
#include "qgslabeldialog.h"
#include "qgslabelinggui.h"
#include "qgslabel.h"
#include "qgsgenericprojectionselector.h"
#include "qgslogger.h"
#include "qgsmaplayerregistry.h"
#include "qgspluginmetadata.h"
#include "qgspluginregistry.h"
#include "qgsproject.h"
#include "qgssinglesymboldialog.h"
#include "qgsuniquevaluedialog.h"
#include "qgsvectorlayer.h"
#include "qgsvectorlayerproperties.h"
#include "qgsconfig.h"
#include "qgsvectordataprovider.h"
#include "qgsvectoroverlayplugin.h"
#include "qgsquerybuilder.h"
#include "qgsdatasourceuri.h"

#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDialog>
#include <QSettings>
#include <QComboBox>
#include <QCheckBox>
#include <QHeaderView>
#include <QColorDialog>

#include "qgsrendererv2propertiesdialog.h"
#include "qgsstylev2.h"
#include "qgssymbologyv2conversion.h"

QgsVectorLayerProperties::QgsVectorLayerProperties(
  QgsVectorLayer *lyr,
  QWidget * parent,
  Qt::WFlags fl
)
    : QDialog( parent, fl )
    , layer( lyr )
    , mMetadataFilled( false )
    , mRendererDialog( 0 )
{
  setupUi( this );
  setupEditTypes();

  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  connect( buttonBox->button( QDialogButtonBox::Apply ), SIGNAL( clicked() ), this, SLOT( apply() ) );
  connect( this, SIGNAL( accepted() ), this, SLOT( apply() ) );
  connect( mAddAttributeButton, SIGNAL( clicked() ), this, SLOT( addAttribute() ) );
  connect( mDeleteAttributeButton, SIGNAL( clicked() ), this, SLOT( deleteAttribute() ) );

  connect( mToggleEditingButton, SIGNAL( clicked() ), this, SLOT( toggleEditing() ) );
  connect( this, SIGNAL( toggleEditing( QgsMapLayer* ) ),
           QgisApp::instance(), SLOT( toggleEditing( QgsMapLayer* ) ) );

  connect( layer, SIGNAL( editingStarted() ), this, SLOT( editingToggled() ) );
  connect( layer, SIGNAL( editingStopped() ), this, SLOT( editingToggled() ) );
  connect( layer, SIGNAL( attributeAdded( int ) ), this, SLOT( attributeAdded( int ) ) );
  connect( layer, SIGNAL( attributeDeleted( int ) ), this, SLOT( attributeDeleted( int ) ) );

  connect( insertFieldButton, SIGNAL( clicked() ), this, SLOT( insertField() ) );
  connect( insertExpressionButton, SIGNAL( clicked() ), this, SLOT( insertExpression() ) );

  mAddAttributeButton->setIcon( QgsApplication::getThemeIcon( "/mActionNewAttribute.png" ) );
  mDeleteAttributeButton->setIcon( QgsApplication::getThemeIcon( "/mActionDeleteAttribute.png" ) );
  mToggleEditingButton->setIcon( QgsApplication::getThemeIcon( "/mActionToggleEditing.png" ) );
  mCalculateFieldButton->setIcon( QgsApplication::getThemeIcon( "/mActionCalculateField.png" ) );

  connect( btnUseNewSymbology, SIGNAL( clicked() ), this, SLOT( useNewSymbology() ) );

  QVBoxLayout *layout = new QVBoxLayout( labelingFrame );
  layout->setMargin( 0 );
  labelingDialog = new QgsLabelingGui( QgisApp::instance()->palLabeling(), layer, QgisApp::instance()->mapCanvas(), labelingFrame );
  layout->addWidget( labelingDialog );
  labelingFrame->setLayout( layout );

  // Create the Label dialog tab
  layout = new QVBoxLayout( labelOptionsFrame );
  layout->setMargin( 0 );
  labelDialog = new QgsLabelDialog( layer->label(), labelOptionsFrame );
  layout->addWidget( labelDialog );
  labelOptionsFrame->setLayout( layout );
  connect( labelDialog, SIGNAL( labelSourceSet() ), this, SLOT( setLabelCheckBox() ) );

  // Create the Actions dialog tab
  QVBoxLayout *actionLayout = new QVBoxLayout( actionOptionsFrame );
  actionLayout->setMargin( 0 );
  const QgsFieldMap &fields = layer->pendingFields();
  actionDialog = new QgsAttributeActionDialog( layer->actions(), fields, actionOptionsFrame );
  actionLayout->addWidget( actionDialog );

  // Create the menu for the save style button to choose the output format
  mSaveAsMenu = new QMenu( pbnSaveStyleAs );
  mSaveAsMenu->addAction( tr( "QGIS Layer Style File" ) );
  mSaveAsMenu->addAction( tr( "SLD File" ) );
  QObject::connect( mSaveAsMenu, SIGNAL( triggered( QAction * ) ), this, SLOT( saveStyleAsMenuTriggered( QAction * ) ) );

  reset();

  if ( layer->dataProvider() )//enable spatial index button group if supported by provider
  {
    int capabilities = layer->dataProvider()->capabilities();
    if ( !( capabilities&QgsVectorDataProvider::CreateSpatialIndex ) )
    {
      pbnIndex->setEnabled( false );
    }

    if ( capabilities & QgsVectorDataProvider::SetEncoding )
    {
      cboProviderEncoding->addItems( QgsVectorDataProvider::availableEncodings() );
      QString enc = layer->dataProvider()->encoding();
      int encindex = cboProviderEncoding->findText( enc );
      if ( encindex < 0 )
      {
        cboProviderEncoding->insertItem( 0, enc );
        encindex = 0;
      }
      cboProviderEncoding->setCurrentIndex( encindex );
    }
    else
    {
      // currently only encoding can be set in this group, so hide it completely
      grpProviderOptions->hide();
    }
  }

  updateButtons();

  leSpatialRefSys->setText( layer->crs().authid() + " - " + layer->crs().description() );
  leSpatialRefSys->setCursorPosition( 0 );

  leEditForm->setText( layer->editForm() );
  leEditFormInit->setText( layer->editFormInit() );

  connect( sliderTransparency, SIGNAL( valueChanged( int ) ), this, SLOT( sliderTransparency_valueChanged( int ) ) );

  //insert existing join info
  const QList< QgsVectorJoinInfo >& joins = layer->vectorJoins();
  for ( int i = 0; i < joins.size(); ++i )
  {
    addJoinToTreeWidget( joins[i] );
  }

  diagramPropertiesDialog = new QgsDiagramProperties( layer, mDiagramFrame );
  mDiagramFrame->setLayout( new QVBoxLayout( mDiagramFrame ) );
  mDiagramFrame->layout()->addWidget( diagramPropertiesDialog );

  //for each overlay plugin create a new tab
  int position;
  QList<QgsVectorOverlayPlugin*> overlayPluginList = overlayPlugins();
  QList<QgsVectorOverlayPlugin*>::const_iterator it = overlayPluginList.constBegin();

  for ( ; it != overlayPluginList.constEnd(); ++it )
  {
    QgsApplyDialog* d = ( *it )->dialog( lyr );
    position = tabWidget->insertTab( tabWidget->count(), qobject_cast<QDialog*>( d ), QgsApplication::getThemeIcon( "propertyicons/diagram.png" ), tr( "Overlay" ) );
    tabWidget->setCurrentIndex( position ); //ugly, but otherwise the properties dialog is a mess
    mOverlayDialogs.push_back( d );
  }

  //layer title and abstract
  if ( layer )
  {
    mLayerTitleLineEdit->setText( layer->title() );
    mLayerAbstractTextEdit->setPlainText( layer->abstract() );
  }

  tabWidget->setCurrentIndex( 0 );

  QSettings settings;
  restoreGeometry( settings.value( "/Windows/VectorLayerProperties/geometry" ).toByteArray() );
  tabWidget->setCurrentIndex( settings.value( "/Windows/VectorLayerProperties/row" ).toInt() );

  setWindowTitle( tr( "Layer Properties - %1" ).arg( layer->name() ) );
} // QgsVectorLayerProperties ctor


QgsVectorLayerProperties::~QgsVectorLayerProperties()
{
  disconnect( labelDialog, SIGNAL( labelSourceSet() ), this, SLOT( setLabelCheckBox() ) );

  QSettings settings;
  settings.setValue( "/Windows/VectorLayerProperties/geometry", saveGeometry() );
  settings.setValue( "/Windows/VectorLayerProperties/row", tabWidget->currentIndex() );
}

void QgsVectorLayerProperties::loadRows()
{
  QObject::disconnect( tblAttributes, SIGNAL( cellChanged( int, int ) ), this, SLOT( on_tblAttributes_cellChanged( int, int ) ) );
  const QgsFieldMap &fields = layer->pendingFields();

  tblAttributes->clear();

  tblAttributes->setColumnCount( attrColCount );
  tblAttributes->setRowCount( fields.size() );
  tblAttributes->setHorizontalHeaderItem( attrIdCol, new QTableWidgetItem( tr( "Id" ) ) );
  tblAttributes->setHorizontalHeaderItem( attrNameCol, new QTableWidgetItem( tr( "Name" ) ) );
  tblAttributes->setHorizontalHeaderItem( attrTypeCol, new QTableWidgetItem( tr( "Type" ) ) );
  tblAttributes->setHorizontalHeaderItem( attrLengthCol, new QTableWidgetItem( tr( "Length" ) ) );
  tblAttributes->setHorizontalHeaderItem( attrPrecCol, new QTableWidgetItem( tr( "Precision" ) ) );
  tblAttributes->setHorizontalHeaderItem( attrCommentCol, new QTableWidgetItem( tr( "Comment" ) ) );
  tblAttributes->setHorizontalHeaderItem( attrEditTypeCol, new QTableWidgetItem( tr( "Edit widget" ) ) );
  tblAttributes->setHorizontalHeaderItem( attrAliasCol, new QTableWidgetItem( tr( "Alias" ) ) );

  tblAttributes->horizontalHeader()->setResizeMode( 1, QHeaderView::Stretch );
  tblAttributes->horizontalHeader()->setResizeMode( 7, QHeaderView::Stretch );
  tblAttributes->setSelectionBehavior( QAbstractItemView::SelectRows );
  tblAttributes->setSelectionMode( QAbstractItemView::ExtendedSelection );
  tblAttributes->verticalHeader()->hide();

  int row = 0;
  for ( QgsFieldMap::const_iterator it = fields.begin(); it != fields.end(); it++, row++ )
    setRow( row, it.key(), it.value() );

  tblAttributes->resizeColumnsToContents();
  QObject::connect( tblAttributes, SIGNAL( cellChanged( int, int ) ), this, SLOT( on_tblAttributes_cellChanged( int, int ) ) );
}

void QgsVectorLayerProperties::setRow( int row, int idx, const QgsField &field )
{
  tblAttributes->setItem( row, attrIdCol, new QTableWidgetItem( QString::number( idx ) ) );
  tblAttributes->setItem( row, attrNameCol, new QTableWidgetItem( field.name() ) );
  tblAttributes->setItem( row, attrTypeCol, new QTableWidgetItem( field.typeName() ) );
  tblAttributes->setItem( row, attrLengthCol, new QTableWidgetItem( QString::number( field.length() ) ) );
  tblAttributes->setItem( row, attrPrecCol, new QTableWidgetItem( QString::number( field.precision() ) ) );
  tblAttributes->setItem( row, attrCommentCol, new QTableWidgetItem( field.comment() ) );

  for ( int i = 0; i < attrEditTypeCol; i++ )
    tblAttributes->item( row, i )->setFlags( tblAttributes->item( row, i )->flags() & ~Qt::ItemIsEditable );

  QPushButton *pb = new QPushButton( editTypeButtonText( layer->editType( idx ) ) );
  tblAttributes->setCellWidget( row, attrEditTypeCol, pb );
  connect( pb, SIGNAL( pressed() ), this, SLOT( attributeTypeDialog( ) ) );
  mButtonMap.insert( idx, pb );

  //set the alias for the attribute
  tblAttributes->setItem( row, attrAliasCol, new QTableWidgetItem( layer->attributeAlias( idx ) ) );
}

void QgsVectorLayerProperties::attributeTypeDialog( )
{
  QPushButton *pb = qobject_cast<QPushButton *>( sender() );
  if ( !pb )
    return;

  int index = mButtonMap.key( pb, -1 );
  if ( index == -1 )
    return;

  QgsAttributeTypeDialog attributeTypeDialog( layer );

  attributeTypeDialog.setValueMap( mValueMaps.value( index, layer->valueMap( index ) ) );
  attributeTypeDialog.setRange( mRanges.value( index, layer->range( index ) ) );
  attributeTypeDialog.setValueRelation( mValueRelationData.value( index, layer->valueRelation( index ) ) );

  QPair<QString, QString> checkStates = mCheckedStates.value( index, layer->checkedState( index ) );
  attributeTypeDialog.setCheckedState( checkStates.first, checkStates.second );

  attributeTypeDialog.setIndex( index, mEditTypeMap.value( index, layer->editType( index ) ) );

  if ( !attributeTypeDialog.exec() )
    return;

  QgsVectorLayer::EditType editType = attributeTypeDialog.editType();

  mEditTypeMap.insert( index, editType );

  QString buttonText;
  switch ( editType )
  {
    case QgsVectorLayer::ValueMap:
      mValueMaps.insert( index, attributeTypeDialog.valueMap() );
      break;
    case QgsVectorLayer::EditRange:
    case QgsVectorLayer::SliderRange:
    case QgsVectorLayer::DialRange:
      mRanges.insert( index, attributeTypeDialog.rangeData() );
      break;
    case QgsVectorLayer::CheckBox:
      mCheckedStates.insert( index, attributeTypeDialog.checkedState() );
      break;
    case QgsVectorLayer::ValueRelation:
      mValueRelationData.insert( index, attributeTypeDialog.valueRelationData() );
      break;
    case QgsVectorLayer::LineEdit:
    case QgsVectorLayer::TextEdit:
    case QgsVectorLayer::UniqueValues:
    case QgsVectorLayer::UniqueValuesEditable:
    case QgsVectorLayer::Classification:
    case QgsVectorLayer::FileName:
    case QgsVectorLayer::Enumeration:
    case QgsVectorLayer::Immutable:
    case QgsVectorLayer::Hidden:
    case QgsVectorLayer::Calendar:
    case QgsVectorLayer::UuidGenerator:
      break;
  }

  pb->setText( editTypeButtonText( editType ) );
}


void QgsVectorLayerProperties::toggleEditing()
{
  emit toggleEditing( layer );

  pbnQueryBuilder->setEnabled( layer && layer->dataProvider() && layer->dataProvider()->supportsSubsetString() &&
                               !layer->isEditable() && layer->vectorJoins().size() < 1 );
  if ( layer->isEditable() )
  {
    pbnQueryBuilder->setToolTip( tr( "Stop editing mode to enable this." ) );
  }
}

void QgsVectorLayerProperties::attributeAdded( int idx )
{
  const QgsFieldMap &fields = layer->pendingFields();
  int row = tblAttributes->rowCount();
  tblAttributes->insertRow( row );
  setRow( row, idx, fields[idx] );
  tblAttributes->setCurrentCell( row, idx );
}


void QgsVectorLayerProperties::attributeDeleted( int idx )
{
  for ( int i = 0; i < tblAttributes->rowCount(); i++ )
  {
    if ( tblAttributes->item( i, 0 )->text().toInt() == idx )
    {
      tblAttributes->removeRow( i );
      break;
    }
  }
}

void QgsVectorLayerProperties::addAttribute()
{
  QgsAddAttrDialog dialog( layer, this );
  if ( dialog.exec() == QDialog::Accepted )
  {
    layer->beginEditCommand( "Attribute added" );
    if ( !addAttribute( dialog.field() ) )
    {
      layer->destroyEditCommand();
      QMessageBox::information( this, tr( "Name conflict" ), tr( "The attribute could not be inserted. The name already exists in the table." ) );
    }
    else
    {
      layer->endEditCommand();
    }
  }
}

bool QgsVectorLayerProperties::addAttribute( const QgsField &field )
{
  QgsDebugMsg( "inserting attribute " + field.name() + " of type " + field.typeName() );
  layer->beginEditCommand( tr( "Added attribute" ) );
  if ( layer->addAttribute( field ) )
  {
    layer->endEditCommand();
    return true;
  }
  else
  {
    layer->destroyEditCommand();
    return false;
  }
}

void QgsVectorLayerProperties::deleteAttribute()
{
  QList<QTableWidgetItem*> items = tblAttributes->selectedItems();
  QList<int> idxs;

  for ( QList<QTableWidgetItem*>::const_iterator it = items.begin(); it != items.end(); it++ )
  {
    if (( *it )->column() == 0 )
      idxs << ( *it )->text().toInt();
  }
  for ( QList<int>::const_iterator it = idxs.begin(); it != idxs.end(); it++ )
  {
    layer->beginEditCommand( tr( "Deleted attribute" ) );
    layer->deleteAttribute( *it );
    layer->endEditCommand();
  }
}

void QgsVectorLayerProperties::editingToggled()
{
  if ( !layer->isEditable() )
    loadRows();

  updateButtons();
}

void QgsVectorLayerProperties::updateButtons()
{
  if ( layer->isEditable() )
  {
    int cap = layer->dataProvider()->capabilities();
    mAddAttributeButton->setEnabled( cap & QgsVectorDataProvider::AddAttributes );
    mDeleteAttributeButton->setEnabled( cap & QgsVectorDataProvider::DeleteAttributes );
    mCalculateFieldButton->setEnabled( cap & ( QgsVectorDataProvider::ChangeAttributeValues | QgsVectorDataProvider::AddAttributes ) );
    mToggleEditingButton->setChecked( true );
  }
  else
  {
    mAddAttributeButton->setEnabled( false );
    mDeleteAttributeButton->setEnabled( false );
    mToggleEditingButton->setChecked( false );
    mToggleEditingButton->setEnabled( false );
    mCalculateFieldButton->setEnabled( false );
  }
}

void QgsVectorLayerProperties::sliderTransparency_valueChanged( int theValue )
{
  //set the transparency percentage label to a suitable value
  int myInt = static_cast < int >(( theValue / 255.0 ) * 100 );  //255.0 to prevent integer division
  lblTransparencyPercent->setText( tr( "Transparency: %1%" ).arg( myInt ) );
}//sliderTransparency_valueChanged

void QgsVectorLayerProperties::setLabelCheckBox()
{
  labelCheckBox->setCheckState( Qt::Checked );
}

void QgsVectorLayerProperties::alterLayerDialog( const QString & dialogString )
{

  widgetStackRenderers->removeWidget( mRendererDialog );
  delete mRendererDialog;
  mRendererDialog = 0;
  if ( dialogString == tr( "Single Symbol" ) )
  {
    mRendererDialog = new QgsSingleSymbolDialog( layer );
  }
  else if ( dialogString == tr( "Graduated Symbol" ) )
  {
    mRendererDialog = new QgsGraduatedSymbolDialog( layer );
  }
  else if ( dialogString == tr( "Continuous Color" ) )
  {
    mRendererDialog = new QgsContinuousColorDialog( layer );
  }
  else if ( dialogString == tr( "Unique Value" ) )
  {
    mRendererDialog = new QgsUniqueValueDialog( layer );
  }
  widgetStackRenderers->addWidget( mRendererDialog );
  widgetStackRenderers->setCurrentWidget( mRendererDialog );
}

void QgsVectorLayerProperties::setLegendType( QString type )
{
  legendtypecombobox->setItemText( legendtypecombobox->currentIndex(), type );
}

void QgsVectorLayerProperties::insertField()
{
  // Convert the selected field to an expression and
  // insert it into the action at the cursor position

  if ( !fieldComboBox->currentText().isNull() )
  {
    QString field = "[% \"";
    field += fieldComboBox->currentText();
    field += "\" %]";
    htmlMapTip->insertPlainText( field );
  }
}

void QgsVectorLayerProperties::insertExpression()
{
  QString selText = htmlMapTip->textCursor().selectedText();

  // edit the selected expression if there's one
  if ( selText.startsWith( "[%" ) && selText.endsWith( "%]" ) )
    selText = selText.mid( 2, selText.size() - 4 );

  // display the expression builder
  QgsExpressionBuilderDialog dlg( layer , selText, this );
  dlg.setWindowTitle( tr( "Insert expression" ) );
  if ( dlg.exec() == QDialog::Accepted )
  {
    QString expression =  dlg.expressionBuilder()->expressionText();
    //Only add the expression if the user has entered some text.
    if ( !expression.isEmpty() )
    {
      htmlMapTip->insertPlainText( "[%" + expression + "%]" );
    }
  }
}

void QgsVectorLayerProperties::setDisplayField( QString name )
{
  int idx = displayFieldComboBox->findText( name );
  if ( idx == -1 )
  {
    htmlRadio->setChecked( true );
    htmlMapTip->setPlainText( name );
  }
  else
  {
    fieldComboRadio->setChecked( true );
    displayFieldComboBox->setCurrentIndex( idx );
  }
}

//! @note in raster props, this method is called sync()
void QgsVectorLayerProperties::reset( void )
{
  QObject::disconnect( tblAttributes, SIGNAL( cellChanged( int, int ) ), this, SLOT( on_tblAttributes_cellChanged( int, int ) ) );

  // populate the general information
  txtDisplayName->setText( layer->name() );
  pbnQueryBuilder->setWhatsThis( tr( "This button opens the query "
                                     "builder and allows you to create a subset of features to display on "
                                     "the map canvas rather than displaying all features in the layer" ) );
  txtSubsetSQL->setWhatsThis( tr( "The query used to limit the features in the "
                                  "layer is shown here. To enter or modify the query, click on the Query Builder button" ) );

  //see if we are dealing with a pg layer here
  grpSubset->setEnabled( true );
  txtSubsetSQL->setText( layer->subsetString() );
  // if the user is allowed to type an adhoc query, the app will crash if the query
  // is bad. For this reason, the sql box is disabled and the query must be built
  // using the query builder, either by typing it in by hand or using the buttons, etc
  // on the builder. If the ability to enter a query directly into the box is required,
  // a mechanism to check it must be implemented.
  txtSubsetSQL->setEnabled( false );
  pbnQueryBuilder->setEnabled( layer && layer->dataProvider() && layer->dataProvider()->supportsSubsetString() &&
                               !layer->isEditable() && layer->vectorJoins().size() < 1 );
  if ( layer->isEditable() )
  {
    pbnQueryBuilder->setToolTip( tr( "Stop editing mode to enable this." ) );
  }

  //get field list for display field combo
  const QgsFieldMap& myFields = layer->pendingFields();
  for ( QgsFieldMap::const_iterator it = myFields.begin(); it != myFields.end(); ++it )
  {
    displayFieldComboBox->addItem( it->name() );
    fieldComboBox->addItem( it->name() );
  }

  setDisplayField( layer-> displayField() );

  // set up the scale based layer visibility stuff....
  chkUseScaleDependentRendering->setChecked( layer->hasScaleBasedVisibility() );
  bool projectScales = QgsProject::instance()->readBoolEntry( "Scales", "/useProjectScales" );
  if ( projectScales )
  {
    QStringList scalesList = QgsProject::instance()->readListEntry( "Scales", "/ScalesList" );
    cbMinimumScale->updateScales( scalesList );
    cbMaximumScale->updateScales( scalesList );
  }
  cbMinimumScale->setScale( 1.0 / layer->minimumScale() );
  cbMaximumScale->setScale( 1.0 / layer->maximumScale() );

  // symbology initialization
  if ( legendtypecombobox->count() == 0 )
  {
    legendtypecombobox->addItem( tr( "Single Symbol" ) );
    if ( myFields.size() > 0 )
    {
      legendtypecombobox->addItem( tr( "Graduated Symbol" ) );
      legendtypecombobox->addItem( tr( "Continuous Color" ) );
      legendtypecombobox->addItem( tr( "Unique Value" ) );
    }
  }

  // load appropriate symbology page (V1 or V2)
  updateSymbologyPage();

  // reset fields in label dialog
  layer->label()->setFields( layer->pendingFields() );

  actionDialog->init();
  labelDialog->init();
  labelCheckBox->setChecked( layer->hasLabelsEnabled() );
  labelOptionsFrame->setEnabled( layer->hasLabelsEnabled() );
  //set the transparency slider
  sliderTransparency->setValue( 255 - layer->getTransparency() );
  //update the transparency percentage label
  sliderTransparency_valueChanged( 255 - layer->getTransparency() );

  loadRows();
  QObject::connect( tblAttributes, SIGNAL( cellChanged( int, int ) ), this, SLOT( on_tblAttributes_cellChanged( int, int ) ) );
  QObject::connect( labelCheckBox, SIGNAL( clicked( bool ) ), this, SLOT( enableLabelOptions( bool ) ) );
} // reset()


//
// methods reimplemented from qt designer base class
//

QMap< QgsVectorLayer::EditType, QString > QgsVectorLayerProperties::editTypeMap;

void QgsVectorLayerProperties::setupEditTypes()
{
  if ( !editTypeMap.isEmpty() )
    return;

  editTypeMap.insert( QgsVectorLayer::LineEdit, tr( "Line edit" ) );
  editTypeMap.insert( QgsVectorLayer::UniqueValues, tr( "Unique values" ) );
  editTypeMap.insert( QgsVectorLayer::UniqueValuesEditable, tr( "Unique values editable" ) );
  editTypeMap.insert( QgsVectorLayer::Classification, tr( "Classification" ) );
  editTypeMap.insert( QgsVectorLayer::ValueMap, tr( "Value map" ) );
  editTypeMap.insert( QgsVectorLayer::EditRange, tr( "Edit range" ) );
  editTypeMap.insert( QgsVectorLayer::SliderRange, tr( "Slider range" ) );
  editTypeMap.insert( QgsVectorLayer::DialRange, tr( "Dial range" ) );
  editTypeMap.insert( QgsVectorLayer::FileName, tr( "File name" ) );
  editTypeMap.insert( QgsVectorLayer::Enumeration, tr( "Enumeration" ) );
  editTypeMap.insert( QgsVectorLayer::Immutable, tr( "Immutable" ) );
  editTypeMap.insert( QgsVectorLayer::Hidden, tr( "Hidden" ) );
  editTypeMap.insert( QgsVectorLayer::CheckBox, tr( "Checkbox" ) );
  editTypeMap.insert( QgsVectorLayer::TextEdit, tr( "Text edit" ) );
  editTypeMap.insert( QgsVectorLayer::Calendar, tr( "Calendar" ) );
  editTypeMap.insert( QgsVectorLayer::ValueRelation, tr( "Value relation" ) );
  editTypeMap.insert( QgsVectorLayer::UuidGenerator, tr( "UUID generator" ) );
}

QString QgsVectorLayerProperties::editTypeButtonText( QgsVectorLayer::EditType type )
{
  return editTypeMap[ type ];
}

QgsVectorLayer::EditType QgsVectorLayerProperties::editTypeFromButtonText( QString text )
{
  return editTypeMap.key( text );
}

void QgsVectorLayerProperties::apply()
{
  labelingDialog->apply();

  //
  // Set up sql subset query if applicable
  //
  grpSubset->setEnabled( true );

  if ( txtSubsetSQL->toPlainText() != layer->subsetString() )
  {
    // set the subset sql for the layer
    layer->setSubsetString( txtSubsetSQL->toPlainText() );
    mMetadataFilled = false;
  }

  // set up the scale based layer visibility stuff....
  layer->toggleScaleBasedVisibility( chkUseScaleDependentRendering->isChecked() );
  layer->setMinimumScale( 1.0 / cbMinimumScale->scale() );
  layer->setMaximumScale( 1.0 / cbMaximumScale->scale() );

  // provider-specific options
  if ( layer->dataProvider() )
  {
    if ( layer->dataProvider()->capabilities() & QgsVectorDataProvider::SetEncoding )
    {
      layer->dataProvider()->setEncoding( cboProviderEncoding->currentText() );
    }
  }

  // update the display field
  if ( htmlRadio->isChecked() )
  {
    layer->setDisplayField( htmlMapTip->toPlainText() );
  }

  if ( fieldComboRadio->isChecked() )
  {
    layer->setDisplayField( displayFieldComboBox->currentText() );
  }

  layer->setEditForm( leEditForm->text() );
  layer->setEditFormInit( leEditFormInit->text() );

  actionDialog->apply();

  labelDialog->apply();
  layer->enableLabels( labelCheckBox->isChecked() );
  layer->setLayerName( displayName() );

  for ( int i = 0; i < tblAttributes->rowCount(); i++ )
  {
    int idx = tblAttributes->item( i, attrIdCol )->text().toInt();

    QPushButton *pb = qobject_cast<QPushButton *>( tblAttributes->cellWidget( i, attrEditTypeCol ) );
    if ( !pb )
      continue;

    QgsVectorLayer::EditType editType = editTypeFromButtonText( pb->text() );
    layer->setEditType( idx, editType );

    switch ( editType )
    {
      case QgsVectorLayer::ValueMap:
        if ( mValueMaps.contains( idx ) )
        {
          QMap<QString, QVariant> &map = layer->valueMap( idx );
          map.clear();
          map = mValueMaps[idx];
        }
        break;

      case QgsVectorLayer::EditRange:
      case QgsVectorLayer::SliderRange:
      case QgsVectorLayer::DialRange:
        if ( mRanges.contains( idx ) )
        {
          layer->range( idx ) = mRanges[idx];
        }
        break;

      case QgsVectorLayer::CheckBox:
        if ( mCheckedStates.contains( idx ) )
        {
          layer->setCheckedState( idx, mCheckedStates[idx].first, mCheckedStates[idx].second );
        }
        break;

      case QgsVectorLayer::ValueRelation:
        if ( mValueRelationData.contains( idx ) )
        {
          layer->valueRelation( idx ) = mValueRelationData[idx];
        }
        break;

      case QgsVectorLayer::LineEdit:
      case QgsVectorLayer::UniqueValues:
      case QgsVectorLayer::UniqueValuesEditable:
      case QgsVectorLayer::Classification:
      case QgsVectorLayer::FileName:
      case QgsVectorLayer::Enumeration:
      case QgsVectorLayer::Immutable:
      case QgsVectorLayer::Hidden:
      case QgsVectorLayer::TextEdit:
      case QgsVectorLayer::Calendar:
      case QgsVectorLayer::UuidGenerator:
        break;
    }
  }

  if ( layer->isUsingRendererV2() )
  {
    QgsRendererV2PropertiesDialog* dlg =
      static_cast<QgsRendererV2PropertiesDialog*>( widgetStackRenderers->currentWidget() );
    dlg->apply();
  }
  else
  {
    QgsSingleSymbolDialog *sdialog =
      qobject_cast < QgsSingleSymbolDialog * >( widgetStackRenderers->currentWidget() );
    QgsGraduatedSymbolDialog *gdialog =
      qobject_cast < QgsGraduatedSymbolDialog * >( widgetStackRenderers->currentWidget() );
    QgsContinuousColorDialog *cdialog =
      qobject_cast < QgsContinuousColorDialog * >( widgetStackRenderers->currentWidget() );
    QgsUniqueValueDialog* udialog =
      qobject_cast< QgsUniqueValueDialog * >( widgetStackRenderers->currentWidget() );

    if ( sdialog )
    {
      sdialog->apply();
    }
    else if ( gdialog )
    {
      gdialog->apply();
    }
    else if ( cdialog )
    {
      cdialog->apply();
    }
    else if ( udialog )
    {
      udialog->apply();
    }
    layer->setTransparency( static_cast < unsigned int >( 255 - sliderTransparency->value() ) );

  }

  //apply diagram settings
  diagramPropertiesDialog->apply();

  //apply overlay dialogs
  for ( QList<QgsApplyDialog*>::iterator it = mOverlayDialogs.begin(); it != mOverlayDialogs.end(); ++it )
  {
    ( *it )->apply();
  }

  //layer title and abstract
  layer->setTitle( mLayerTitleLineEdit->text() );
  layer->setAbstract( mLayerAbstractTextEdit->toPlainText() );

  // update symbology
  emit refreshLegend( layer->id(), false );

  //no need to delete the old one, maplayer will do it if needed
  layer->setCacheImage( 0 );

  layer->triggerRepaint();
  // notify the project we've made a change
  QgsProject::instance()->dirty( true );

}

void QgsVectorLayerProperties::on_pbnQueryBuilder_clicked()
{
  // launch the query builder
  QgsQueryBuilder *qb = new QgsQueryBuilder( layer, this );

  // Set the sql in the query builder to the same in the prop dialog
  // (in case the user has already changed it)
  qb->setSql( txtSubsetSQL->toPlainText() );
  // Open the query builder
  if ( qb->exec() )
  {
    // if the sql is changed, update it in the prop subset text box
    txtSubsetSQL->setText( qb->sql() );
    //TODO If the sql is changed in the prop dialog, the layer extent should be recalculated

    // The datasource for the layer needs to be updated with the new sql since this gets
    // saved to the project file. This should happen at the map layer level...

  }
  // delete the query builder object
  delete qb;
}

void QgsVectorLayerProperties::on_pbnIndex_clicked()
{
  QgsVectorDataProvider* pr = layer->dataProvider();
  if ( pr )
  {
    setCursor( Qt::WaitCursor );
    bool errval = pr->createSpatialIndex();
    setCursor( Qt::ArrowCursor );
    if ( errval )
    {
      QMessageBox::information( this, tr( "Spatial Index" ), tr( "Creation of spatial index successful" ) );
    }
    else
    {
      // TODO: Remind the user to use OGR >= 1.2.6 and Shapefile
      QMessageBox::information( this, tr( "Spatial Index" ), tr( "Creation of spatial index failed" ) );
    }
  }
}

QString QgsVectorLayerProperties::metadata()
{
  return layer->metadata();
}



void QgsVectorLayerProperties::on_pbnChangeSpatialRefSys_clicked()
{
  QgsGenericProjectionSelector * mySelector = new QgsGenericProjectionSelector( this );
  mySelector->setMessage();
  mySelector->setSelectedCrsId( layer->crs().srsid() );
  if ( mySelector->exec() )
  {
    QgsCoordinateReferenceSystem srs( mySelector->selectedCrsId(), QgsCoordinateReferenceSystem::InternalCrsId );
    layer->setCrs( srs );
  }
  else
  {
    QApplication::restoreOverrideCursor();
  }
  delete mySelector;

  leSpatialRefSys->setText( layer->crs().authid() + " - " + layer->crs().description() );
  leSpatialRefSys->setCursorPosition( 0 );
}

void QgsVectorLayerProperties::on_pbnLoadDefaultStyle_clicked()
{
  bool defaultLoadedFlag = false;
  QString myMessage = layer->loadDefaultStyle( defaultLoadedFlag );
  //reset if the default style was loaded ok only
  if ( defaultLoadedFlag )
  {
    // all worked ok so no need to inform user
    reset();
  }
  else
  {
    //something went wrong - let them know why
    QMessageBox::information( this, tr( "Default Style" ), myMessage );
  }
}

void QgsVectorLayerProperties::on_pbnSaveDefaultStyle_clicked()
{
  apply(); // make sure the qml to save is uptodate

  // a flag passed by reference
  bool defaultSavedFlag = false;
  // after calling this the above flag will be set true for success
  // or false if the save operation failed
  QString myMessage = layer->saveDefaultStyle( defaultSavedFlag );
  if ( !defaultSavedFlag )
  {
    //only raise the message if something went wrong
    QMessageBox::information( this, tr( "Default Style" ), myMessage );
  }
}


void QgsVectorLayerProperties::on_pbnLoadStyle_clicked()
{
  QSettings myQSettings;  // where we keep last used filter in persistent state
  QString myLastUsedDir = myQSettings.value( "style/lastStyleDir", "." ).toString();

  QString myFileName = QFileDialog::getOpenFileName( this, tr( "Load layer properties from style file" ), myLastUsedDir,
                       tr( "QGIS Layer Style File" ) + " (*.qml);;" + tr( "SLD File" ) + " (*.sld)" );
  if ( myFileName.isNull() )
  {
    return;
  }

  QString myMessage;
  bool defaultLoadedFlag = false;

  if ( myFileName.endsWith( ".sld", Qt::CaseInsensitive ) )
  {
    // load from SLD
    myMessage = layer->loadSldStyle( myFileName, defaultLoadedFlag );
  }
  else
  {
    myMessage = layer->loadNamedStyle( myFileName, defaultLoadedFlag );
  }
  //reset if the default style was loaded ok only
  if ( defaultLoadedFlag )
  {
    reset();
  }
  else
  {
    //let the user know what went wrong
    QMessageBox::information( this, tr( "Load Style" ), myMessage );
  }

  QFileInfo myFI( myFileName );
  QString myPath = myFI.path();
  myQSettings.setValue( "style/lastStyleDir", myPath );
}


void QgsVectorLayerProperties::on_pbnSaveStyleAs_clicked()
{
  saveStyleAs( QML );
}

void QgsVectorLayerProperties::saveStyleAsMenuTriggered( QAction *action )
{
  QMenu *menu = qobject_cast<QMenu *>( sender() );
  if ( !menu )
    return;

  int index = mSaveAsMenu->actions().indexOf( action );
  if ( index < 0 )
    return;

  saveStyleAs(( StyleType ) index );
}

void QgsVectorLayerProperties::saveStyleAs( StyleType styleType )
{
  QSettings myQSettings;  // where we keep last used filter in persistent state
  QString myLastUsedDir = myQSettings.value( "style/lastStyleDir", "." ).toString();

  QString format, extension;
  if ( styleType == SLD )
  {
    format = tr( "SLD File" ) + " (*.sld)";
    extension = ".sld";
  }
  else
  {
    format =  tr( "QGIS Layer Style File" ) + " (*.qml)";
    extension = ".qml";
  }

  QString myOutputFileName = QFileDialog::getSaveFileName( this, tr( "Save layer properties as style file" ),
                             myLastUsedDir, format );
  if ( myOutputFileName.isNull() ) //dialog canceled
  {
    return;
  }

  apply(); // make sure the style to save is uptodate

  QString myMessage;
  bool defaultLoadedFlag = false;

  //ensure the user never omitted the extension from the file name
  if ( !myOutputFileName.endsWith( extension, Qt::CaseInsensitive ) )
  {
    myOutputFileName += extension;
  }

  if ( styleType == SLD )
  {
    // convert to SLD
    myMessage = layer->saveSldStyle( myOutputFileName, defaultLoadedFlag );
  }
  else
  {
    myMessage = layer->saveNamedStyle( myOutputFileName, defaultLoadedFlag );
  }

  //reset if the default style was loaded ok only
  if ( defaultLoadedFlag )
  {
    reset();
  }
  else
  {
    //let the user know what went wrong
    QMessageBox::information( this, tr( "Saved Style" ), myMessage );
  }

  QFileInfo myFI( myOutputFileName );
  QString myPath = myFI.path();
  // Persist last used dir
  myQSettings.setValue( "style/lastStyleDir", myPath );
}

void QgsVectorLayerProperties::on_tblAttributes_cellChanged( int row, int column )
{
  if ( column == attrAliasCol && layer ) //only consider attribute aliases in this function
  {
    int idx = tblAttributes->item( row, attrIdCol )->text().toInt();

    const QgsFieldMap &fields = layer->pendingFields();

    if ( !fields.contains( idx ) )
    {
      return; // index must be wrong
    }

    QTableWidgetItem *aliasItem = tblAttributes->item( row, column );
    if ( aliasItem )
    {
      layer->addAttributeAlias( idx, aliasItem->text() );
    }
  }
}

void QgsVectorLayerProperties::on_mCalculateFieldButton_clicked()
{
  if ( !layer )
  {
    return;
  }

  QgsFieldCalculator calc( layer );
  calc.exec();
}

void QgsVectorLayerProperties::on_pbnSelectEditForm_clicked()
{
  QSettings myQSettings;
  QString lastUsedDir = myQSettings.value( "style/lastUIDir", "." ).toString();
  QString uifilename = QFileDialog::getOpenFileName( this, tr( "Select edit form" ), lastUsedDir, tr( "UI file" )  + " (*.ui)" );

  if ( uifilename.isNull() )
    return;

  leEditForm->setText( uifilename );
}

QList<QgsVectorOverlayPlugin*> QgsVectorLayerProperties::overlayPlugins() const
{
  QList<QgsVectorOverlayPlugin*> pluginList;

  QgisPlugin* thePlugin = 0;
  QgsVectorOverlayPlugin* theOverlayPlugin = 0;

  QList<QgsPluginMetadata*> pluginData = QgsPluginRegistry::instance()->pluginData();
  for ( QList<QgsPluginMetadata*>::iterator it = pluginData.begin(); it != pluginData.end(); ++it )
  {
    if ( *it )
    {
      thePlugin = ( *it )->plugin();
      if ( thePlugin && thePlugin->type() == QgisPlugin::VECTOR_OVERLAY )
      {
        theOverlayPlugin = dynamic_cast<QgsVectorOverlayPlugin *>( thePlugin );
        if ( theOverlayPlugin )
        {
          pluginList.push_back( theOverlayPlugin );
        }
      }
    }
  }

  return pluginList;
}

void QgsVectorLayerProperties::setUsingNewSymbology( bool useNewSymbology )
{
  if ( useNewSymbology )
  {
    QgsSymbologyV2Conversion::rendererV1toV2( layer );
  }
  else
  {
    QgsSymbologyV2Conversion::rendererV2toV1( layer );
  }

  // update GUI!
  updateSymbologyPage();
}

void QgsVectorLayerProperties::on_mButtonAddJoin_clicked()
{
  QgsAddJoinDialog d( layer );
  if ( d.exec() == QDialog::Accepted )
  {
    QgsVectorJoinInfo info;
    info.targetField = d.targetField();
    info.joinLayerId = d.joinedLayerId();
    info.joinField = d.joinField();
    info.memoryCache = d.cacheInMemory();
    if ( layer )
    {
      //create attribute index if possible
      if ( d.createAttributeIndex() )
      {
        QgsVectorLayer* joinLayer = qobject_cast<QgsVectorLayer*>( QgsMapLayerRegistry::instance()->mapLayer( info.joinLayerId ) );
        if ( joinLayer )
        {
          joinLayer->dataProvider()->createAttributeIndex( info.joinField );
        }
      }

      layer->addJoin( info );
      loadRows(); //update attribute tab
      addJoinToTreeWidget( info );
      pbnQueryBuilder->setEnabled( layer && layer->dataProvider() && layer->dataProvider()->supportsSubsetString() &&
                                   !layer->isEditable() && layer->vectorJoins().size() < 1 );
    }
  }
}

void QgsVectorLayerProperties::addJoinToTreeWidget( const QgsVectorJoinInfo& join )
{
  QTreeWidgetItem* joinItem = new QTreeWidgetItem();

  QgsVectorLayer* joinLayer = qobject_cast<QgsVectorLayer*>( QgsMapLayerRegistry::instance()->mapLayer( join.joinLayerId ) );
  if ( !joinLayer )
  {
    return;
  }

  joinItem->setText( 0, joinLayer->name() );
  joinItem->setData( 0, Qt::UserRole, join.joinLayerId );
  QString joinFieldName = joinLayer->pendingFields().value( join.joinField ).name();
  QString targetFieldName = layer->pendingFields().value( join.targetField ).name();
  joinItem->setText( 1, joinFieldName );
  joinItem->setData( 1, Qt::UserRole, join.joinField );
  joinItem->setText( 2, targetFieldName );
  joinItem->setData( 2, Qt::UserRole, join.targetField );

  mJoinTreeWidget->addTopLevelItem( joinItem );
}

void QgsVectorLayerProperties::on_mButtonRemoveJoin_clicked()
{
  QTreeWidgetItem* currentJoinItem = mJoinTreeWidget->currentItem();
  if ( !layer || !currentJoinItem )
  {
    return;
  }

  layer->removeJoin( currentJoinItem->data( 0, Qt::UserRole ).toString() );
  loadRows();
  mJoinTreeWidget->takeTopLevelItem( mJoinTreeWidget->indexOfTopLevelItem( currentJoinItem ) );
  pbnQueryBuilder->setEnabled( layer && layer->dataProvider() && layer->dataProvider()->supportsSubsetString() &&
                               !layer->isEditable() && layer->vectorJoins().size() < 1 );
}


void QgsVectorLayerProperties::useNewSymbology()
{
  int res = QMessageBox::question( this, tr( "Symbology" ),
                                   tr( "Do you wish to use the new symbology implementation for this layer?" ),
                                   QMessageBox::Yes | QMessageBox::No );

  if ( res != QMessageBox::Yes )
    return;

  setUsingNewSymbology( true );
}

void QgsVectorLayerProperties::updateSymbologyPage()
{

  //find out the type of renderer in the vectorlayer, create a dialog with these settings and add it to the form
  delete mRendererDialog;
  mRendererDialog = 0;

  bool v2 = layer->isUsingRendererV2();

  // hide unused widgets
  legendtypecombobox->setVisible( !v2 );
  legendtypelabel->setVisible( !v2 );
  lblTransparencyPercent->setVisible( !v2 );
  sliderTransparency->setVisible( !v2 );
  btnUseNewSymbology->setVisible( !v2 );

  if ( v2 )
  {
    mRendererDialog = new QgsRendererV2PropertiesDialog( layer, QgsStyleV2::defaultStyle(), true );

    connect( mRendererDialog, SIGNAL( useNewSymbology( bool ) ), this, SLOT( setUsingNewSymbology( bool ) ) );

    // display the menu to choose the output format (fix #5136)
    pbnSaveStyleAs->setText( tr( "Save Style" ) );
    pbnSaveStyleAs->setMenu( mSaveAsMenu );
    QObject::disconnect( pbnSaveStyleAs, SIGNAL( clicked() ), this, SLOT( on_pbnSaveStyleAs_clicked() ) );
  }
  else if ( layer->renderer() )
  {
    QString rtype = layer->renderer()->name();
    if ( rtype == "Single Symbol" )
    {
      mRendererDialog = new QgsSingleSymbolDialog( layer );
      legendtypecombobox->setCurrentIndex( 0 );
    }
    else if ( rtype == "Graduated Symbol" )
    {
      mRendererDialog = new QgsGraduatedSymbolDialog( layer );
      legendtypecombobox->setCurrentIndex( 1 );
    }
    else if ( rtype == "Continuous Color" )
    {
      mRendererDialog = new QgsContinuousColorDialog( layer );
      legendtypecombobox->setCurrentIndex( 2 );
    }
    else if ( rtype == "Unique Value" )
    {
      mRendererDialog = new QgsUniqueValueDialog( layer );
      legendtypecombobox->setCurrentIndex( 3 );
    }

    QObject::connect( legendtypecombobox, SIGNAL( activated( const QString & ) ), this,
                      SLOT( alterLayerDialog( const QString & ) ) );

    pbnSaveStyleAs->setText( tr( "Save Style..." ) );
    pbnSaveStyleAs->setMenu( NULL );
    QObject::connect( pbnSaveStyleAs, SIGNAL( clicked() ), this, SLOT( on_pbnSaveStyleAs_clicked() ) );
  }
  else
  {
    if ( tabWidget->currentIndex() == 0 )
    {
      tabWidget->setCurrentIndex( 1 );
    }

    tabWidget->setTabEnabled( 0, true ); // hide symbology item
  }

  if ( mRendererDialog )
  {
    widgetStackRenderers->addWidget( mRendererDialog );
    widgetStackRenderers->setCurrentWidget( mRendererDialog );
  }
}

void QgsVectorLayerProperties::on_pbnUpdateExtents_clicked()
{
  layer->updateExtents();
  mMetadataFilled = false;
}

void QgsVectorLayerProperties::on_tabWidget_currentChanged( int index )
{
  if ( index != 6 || mMetadataFilled )
    return;

  //set the metadata contents (which can be expensive)
  QString myStyle = QgsApplication::reportStyleSheet();
  teMetadata->clear();
  teMetadata->document()->setDefaultStyleSheet( myStyle );
  teMetadata->setHtml( metadata() );
  mMetadataFilled = true;
}

void QgsVectorLayerProperties::enableLabelOptions( bool theFlag )
{
  labelOptionsFrame->setEnabled( theFlag );
}


