#include "qgsdatadefinedsymboldialog.h"
#include "qgsexpressionbuilderdialog.h"
#include "qgsvectorlayer.h"
#include "qgslogger.h"
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

QgsDataDefinedSymbolDialog::QgsDataDefinedSymbolDialog( const QList< DataDefinedSymbolEntry >& entries, const QgsVectorLayer* vl, QWidget * parent, Qt::WindowFlags f ): QDialog( parent, f ), mVectorLayer( vl )
{
  setupUi( this );

  QgsFields attributeFields;
  if ( mVectorLayer )
  {
    attributeFields = mVectorLayer->pendingFields();
  }

  mTableWidget->setRowCount( entries.size() );

  int i = 0;
  QList< DataDefinedSymbolEntry >::const_iterator entryIt = entries.constBegin();
  for ( ; entryIt != entries.constEnd(); ++entryIt )
  {
    //check box
    QCheckBox* cb = new QCheckBox( this );
    cb->setChecked( !entryIt->initialValue.isEmpty() );
    mTableWidget->setCellWidget( i, 0, cb );
    mTableWidget->setColumnWidth( 0, cb->width() );


    //property name
    QTableWidgetItem* propertyItem = new QTableWidgetItem( entryIt->title );
    propertyItem->setData( Qt::UserRole, entryIt->property );
    mTableWidget->setItem( i, 1, propertyItem );

    //attribute list
    QString expressionString = entryIt->initialValue;
    QComboBox* attributeComboBox = new QComboBox( this );
    attributeComboBox->addItem( QString() );
    for ( int j = 0; j < attributeFields.count(); ++j )
    {
      attributeComboBox->addItem( attributeFields.at( j ).name() );
    }

    int attrComboIndex = comboIndexForExpressionString( expressionString, attributeComboBox );
    if ( attrComboIndex >= 0 )
    {
      attributeComboBox->setCurrentIndex( attrComboIndex );
    }
    else
    {
      attributeComboBox->setItemText( 0, expressionString );
    }

    mTableWidget->setCellWidget( i, 2, attributeComboBox );

    //expression button
    QPushButton* expressionButton = new QPushButton( "...", this );
    QObject::connect( expressionButton, SIGNAL( clicked() ), this, SLOT( expressionButtonClicked() ) );
    mTableWidget->setCellWidget( i, 3, expressionButton );

    //help text
    QTableWidgetItem* helpItem = new QTableWidgetItem( entryIt->helpText );
    mTableWidget->setItem( i, 4, helpItem );

    ++i;
  }
}

QgsDataDefinedSymbolDialog::~QgsDataDefinedSymbolDialog()
{

}

QMap< QString, QString > QgsDataDefinedSymbolDialog::dataDefinedProperties() const
{
  QMap< QString, QString > propertyMap;
  int rowCount = mTableWidget->rowCount();
  for ( int i = 0; i < rowCount; ++i )
  {
    //property
    QString propertyKey = mTableWidget->item( i, 1 )->data( Qt::UserRole ).toString();
    //checked?
    bool checked = false;
    QCheckBox* cb = qobject_cast<QCheckBox*>( mTableWidget->cellWidget( i, 0 ) );
    if ( cb )
    {
      checked = cb->isChecked();
    }
    QString expressionString;
    if ( checked )
    {
      QComboBox* comboBox = qobject_cast<QComboBox*>( mTableWidget->cellWidget( i, 2 ) );
      expressionString = comboBox->currentText();
      if ( comboBox->currentIndex() > 0 )
      {
        expressionString.prepend( "\"" ).append( "\"" );
      }
    }
    propertyMap.insert( propertyKey, expressionString );
  }
  return propertyMap;
}

void QgsDataDefinedSymbolDialog::expressionButtonClicked()
{
  QgsDebugMsg( "Expression button clicked" );

  //find out row
  QObject* senderObj = sender();
  int row = 0;
  for ( ; row < mTableWidget->rowCount(); ++row )
  {
    if ( senderObj == mTableWidget->cellWidget( row, 3 ) )
    {
      break;
    }
  }

  QComboBox* attributeCombo = qobject_cast<QComboBox*>( mTableWidget->cellWidget( row, 2 ) );
  if ( !attributeCombo )
  {
    return;
  }

  QString previousText = attributeCombo->itemText( attributeCombo->currentIndex() );
  if ( attributeCombo->currentIndex() > 0 )
  {
    previousText.prepend( "\"" ).append( "\"" );
  }

  QgsExpressionBuilderDialog d( const_cast<QgsVectorLayer*>( mVectorLayer ), previousText );
  if ( d.exec() == QDialog::Accepted )
  {
    QString expressionString = d.expressionText();
    int comboIndex = comboIndexForExpressionString( d.expressionText(), attributeCombo );

    if ( comboIndex == -1 )
    {
      attributeCombo->setItemText( 0, d.expressionText() );
      attributeCombo->setCurrentIndex( 0 );
    }
    else
    {
      if ( comboIndex != 0 )
      {
        attributeCombo->setItemText( 0, QString() );
      }
      attributeCombo->setCurrentIndex( comboIndex );
    }
  }
}

int QgsDataDefinedSymbolDialog::comboIndexForExpressionString( const QString& expr, const QComboBox* cb )
{
  QString attributeString = expr.trimmed();
  int comboIndex = cb->findText( attributeString );
  if ( comboIndex == -1 )
  {
    attributeString.remove( 0, 1 ).chop( 1 );
    comboIndex = cb->findText( attributeString );
  }
  return comboIndex;
}

QString QgsDataDefinedSymbolDialog::doubleHelpText()
{
  return tr( "double" );
}

QString QgsDataDefinedSymbolDialog::colorHelpText()
{
  return tr( "'<red>,<green>,<blue>,<alpha>'" );
}

QString QgsDataDefinedSymbolDialog::offsetHelpText()
{
  return "'<x>,<y>'";
}

QString QgsDataDefinedSymbolDialog::fileNameHelpText()
{
  return tr( "'<filename>'" );
}

QString QgsDataDefinedSymbolDialog::horizontalAnchorHelpText()
{
  return tr( "'left'|'center'|'right'" );
}

QString QgsDataDefinedSymbolDialog::verticalAnchorHelpText()
{
  return tr( "'top'|'center'|'bottom'" );
}

QString QgsDataDefinedSymbolDialog::gradientTypeHelpText()
{
  return tr( "'linear'|'radial'|'conical'" );
}

QString QgsDataDefinedSymbolDialog::gradientCoordModeHelpText()
{
  return tr( "'feature'|'viewport'" );
}

QString QgsDataDefinedSymbolDialog::gradientSpreadHelpText()
{
  return tr( "'pad'|'repeat'|'reflect'" );
}

QString QgsDataDefinedSymbolDialog::boolHelpText()
{
  return tr( "0 (false)|1 (true)" );
}


