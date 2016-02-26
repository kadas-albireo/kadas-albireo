#include "qgskmlexportdialog.h"
#include "qgsmaplayerregistry.h"
#include "qgsvectorlayer.h"
#include <QFileDialog>

QgsKMLExportDialog::QgsKMLExportDialog( const QStringList layerIds, QWidget * parent, Qt::WindowFlags f ): QDialog( parent, f ), mLayerIds( layerIds )
{
  setupUi( this );
  insertAvailableLayers();
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
    if ( item && item->checkState() == Qt::Checked )
    {
      QString id = item->data( Qt::UserRole ).toString();
      QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( id );
      if ( layer )
      {
        layerList.append( layer );
      }
    }
  }
  return layerList;
}

bool QgsKMLExportDialog::visibleExtentOnly() const
{
  return mMapExtentCheckBox->isChecked();
}

void QgsKMLExportDialog::insertAvailableLayers()
{
  mLayerListWidget->clear();
  QStringList::const_iterator it = mLayerIds.constBegin();
  for ( ; it != mLayerIds.constEnd(); ++it )
  {
    QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( *it );
    if ( !layer /*|| !dynamic_cast<QgsVectorLayer*>( layer )*/ )
    {
      continue;
    }

    QListWidgetItem* item = new QListWidgetItem( layer->name() );
    item->setCheckState( Qt::Checked );
    item->setData( Qt::UserRole, *it );
    mLayerListWidget->addItem( item );
  }
}

void QgsKMLExportDialog::on_mFileSelectionButton_clicked()
{
  QString fileName = QFileDialog::getSaveFileName( 0, tr( "Save KML file" ) );
  if ( !fileName.isEmpty() )
  {
    mFileLineEdit->setText( fileName );
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
