#include "qgskmlexportdialog.h"
#include "qgsmaplayerregistry.h"
#include "qgspluginlayer.h"
#include "qgsvectorlayer.h"
#include "qgsrasterlayer.h"
#include <QFileDialog>
#include <QSettings>

QgsKMLExportDialog::QgsKMLExportDialog( const QStringList layerIds, QWidget * parent, Qt::WindowFlags f ): QDialog( parent, f ), mLayerIds( layerIds )
{
  setupUi( this );
  insertAvailableLayers();
  mFormatComboBox->setCurrentIndex( 1 );
  on_mFormatComboBox_currentIndexChanged( mFormatComboBox->currentText() );
}

QgsKMLExportDialog::QgsKMLExportDialog(): QDialog()
{
}

QgsKMLExportDialog::~QgsKMLExportDialog()
{
}

QString QgsKMLExportDialog::saveFile() const
{
  //Add file ending if not already there...
  QString fileName = mFileLineEdit->text();
  if ( exportFormat() == QgsKMLExportDialog::KML && !fileName.endsWith( ".kml", Qt::CaseInsensitive ) )
  {
    fileName.append( ".kml" );
  }
  else if ( exportFormat() == QgsKMLExportDialog::KMZ && !fileName.endsWith( ".kmz", Qt::CaseInsensitive ) ) //KMZ
  {
    fileName.append( ".kmz" );
  }
  return fileName;
}

QList<QgsMapLayer*> QgsKMLExportDialog::selectedLayers() const
{
  QList<QgsMapLayer*> layerList;

  int nItems = mLayerListWidget->count();
  for ( int i = 0; i < nItems; ++i )
  {
    QListWidgetItem* item = mLayerListWidget->item( i );
    if ( item && ( item->flags() & Qt::ItemIsEnabled ) && item->checkState() == Qt::Checked )
    {
      QString id = item->data( Qt::UserRole ).toString();
      QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( id );
      if ( layer )
      {
        layerList.prepend( layer );
      }
    }
  }
  return layerList;
}

bool QgsKMLExportDialog::exportAnnotations() const
{
  return mAnnotationsCheckBox->isChecked();
}

void QgsKMLExportDialog::insertAvailableLayers()
{
  mLayerListWidget->clear();
  QStringList::const_iterator it = mLayerIds.constBegin();
  for ( ; it != mLayerIds.constEnd(); ++it )
  {
    QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( *it );
    QgsVectorLayer* vLayer = dynamic_cast<QgsVectorLayer*>( layer );
    QgsRasterLayer* rLayer = dynamic_cast<QgsRasterLayer*>( layer );
    QgsPluginLayer* pLayer = dynamic_cast<QgsPluginLayer*>( layer );
    if ( !vLayer && !rLayer && ( !pLayer || pLayer->pluginLayerType() != "MilX_Layer" ) )
    {
      continue;
    }
    Qt::CheckState checked = rLayer && rLayer->source().contains( "url=http" ) ? Qt::Unchecked : Qt::Checked;

    QListWidgetItem* item = new QListWidgetItem( layer->name() );
    item->setCheckState( checked );
    item->setData( Qt::UserRole, *it );
    mLayerListWidget->addItem( item );
  }
}

void QgsKMLExportDialog::on_mFileSelectionButton_clicked()
{
  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString filter = exportFormat() == QgsKMLExportDialog::KML ? "KML (*.kml);;" : "KMZ (*.kmz);;";
  QString fileName = QFileDialog::getSaveFileName( 0, tr( "Save KML file" ), lastDir, filter );
  if ( !fileName.isEmpty() )
  {
    QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( fileName ).absolutePath() );
    if ( exportFormat() == QgsKMLExportDialog::KML && !fileName.endsWith( ".kml", Qt::CaseInsensitive ) )
    {
      fileName.append( ".kml" );
    }
    else if ( exportFormat() == QgsKMLExportDialog::KMZ && !fileName.endsWith( ".kmz", Qt::CaseInsensitive ) ) //KMZ
    {
      fileName.append( ".kmz" );
    }
    mFileLineEdit->setText( fileName );
  }
}

void QgsKMLExportDialog::on_mFormatComboBox_currentIndexChanged( const QString& text )
{
  if ( text == "KML" )
  {
    deactivateNonVectorLayers();
    mAnnotationsCheckBox->setChecked( false );
    mAnnotationsCheckBox->setEnabled( false );
  }
  else
  {
    activateAllLayers();
    mAnnotationsCheckBox->setEnabled( true );
  }
}

QgsKMLExportDialog::ExportFormat QgsKMLExportDialog::exportFormat() const
{
  if ( QString::compare( mFormatComboBox->currentText(), "KMZ", Qt::CaseInsensitive ) == 0 )
  {
    return QgsKMLExportDialog::KMZ;
  }
  else
  {
    return QgsKMLExportDialog::KML;
  }
}

void QgsKMLExportDialog::deactivateNonVectorLayers()
{
  int rowCount = mLayerListWidget->count();
  for ( int i = 0; i < rowCount; ++i )
  {
    QString layerId = mLayerListWidget->item( i )->data( Qt::UserRole ).toString();
    QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerId );
    if ( layer && layer->type() != QgsMapLayer::VectorLayer )
    {
      mLayerListWidget->item( i )->setFlags( Qt::NoItemFlags );
      mLayerListWidget->item( i )->setCheckState( Qt::Unchecked );
    }
  }
}

void QgsKMLExportDialog::activateAllLayers()
{
  int rowCount = mLayerListWidget->count();
  for ( int i = 0; i < rowCount; ++i )
  {
    mLayerListWidget->item( i )->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
  }
}
