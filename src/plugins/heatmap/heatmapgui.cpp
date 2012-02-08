/***************************************************************************
 *   Copyright (C) 2003 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
// qgis includes
#include "qgis.h"
#include "heatmapgui.h"
#include "qgscontexthelp.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsvectorlayer.h"

// GDAL includes
#include "gdal_priv.h"
#include "cpl_string.h"
#include "cpl_conv.h"

//qt includes
#include <QComboBox>
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

//standard includes

HeatmapGui::HeatmapGui( QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl )
{
  setupUi( this );

  // Adding point layers to the mInputVectorCombo
  foreach( QgsMapLayer *l, QgsMapLayerRegistry::instance()->mapLayers() )
  {
    QgsVectorLayer *vl = qobject_cast<QgsVectorLayer *>( l );
    if ( !vl || vl->geometryType() != QGis::Point )
      continue;

    mInputVectorCombo->addItem( vl->name(), vl->id() );
  }

  // Adding GDAL drivers with CREATE to the mFormatCombo
  int myTiffIndex = -1;
  int myIndex = -1;
  GDALAllRegister();
  int nDrivers = GDALGetDriverCount();
  for ( int i = 0; i < nDrivers; i += 1 )
  {
    GDALDriver* nthDriver = GetGDALDriverManager()->GetDriver( i );
    char** driverMetadata = nthDriver->GetMetadata();
    if ( CSLFetchBoolean( driverMetadata, GDAL_DCAP_CREATE, false ) )
    {
      ++myIndex;
      QString myLongName = nthDriver->GetMetadataItem( GDAL_DMD_LONGNAME );
      // Add LongName text, shortname variant; GetDescription actually gets the shortname
      mFormatCombo->addItem( myLongName, QVariant( nthDriver->GetDescription() ) );
      // Add the drivers and their extensions to a map for filename correction
      mExtensionMap.insert( nthDriver->GetDescription(), nthDriver->GetMetadataItem( GDAL_DMD_EXTENSION ) );
      if ( myLongName == "GeoTIFF" )
      {
        myTiffIndex = myIndex;
      }
    }
  }
  mFormatCombo->setCurrentIndex( myTiffIndex );

  //finally set right the ok button
  enableOrDisableOkButton();
}

HeatmapGui::~HeatmapGui()
{
}

void HeatmapGui::on_mButtonBox_accepted()
{
  // Variables to be emitted with the createRaster signal
  int bufferDistance;
  float decayRatio;
  QString outputFileName;
  QString outputFormat;

  QString dummyText;

  // The input vector layer
  QString myLayerId = mInputVectorCombo->itemData( mInputVectorCombo->currentIndex() ).toString();

  QgsVectorLayer* inputLayer = qobject_cast<QgsVectorLayer *>( QgsMapLayerRegistry::instance()->mapLayer( myLayerId ) );
  if ( !inputLayer )
  {
    QMessageBox::information( 0, tr( "Layer not found" ), tr( "Layer %1 not found." ).arg( myLayerId ) );
    return;
  }

  // The buffer distance
  dummyText = mBufferLineEdit->text();
  bufferDistance = dummyText.toInt();
  if ( bufferDistance == 0 )
  {
    QMessageBox::information( 0, tr( "Invalid buffer value" ), tr( "Buffer distance cannot be zero. Please enter a valid value." ) );
    return;
  }
  // The decay ratio
  dummyText = mDecayLineEdit->text();
  decayRatio = dummyText.toFloat();

  // The output filename
  outputFileName = mOutputRasterLineEdit->text();
  QFileInfo myFileInfo( outputFileName );
  if ( outputFileName.isEmpty() || !myFileInfo.dir().exists() )
  {
    QMessageBox::information( 0, tr( "Invalid output filename" ), tr( "Please enter a valid output file path and name." ) );
    return;
  }

  // The output format
  outputFormat = mFormatCombo->itemData( mFormatCombo->currentIndex() ).toString();

  // append the file format if the suffix is empty
  QString suffix = myFileInfo.suffix();
  if ( suffix.isEmpty() )
  {
    QMap<QString, QString>::const_iterator it = mExtensionMap.find( outputFormat );
    if ( it != mExtensionMap.end() && it.key() == outputFormat )
    {
      // making sure that there is really a extension value available
      // Some drivers donot seem to have any extension at all
      if ( it.value() != NULL || it.value() != "" )
      {
        outputFileName.append( "." );
        outputFileName.append( it.value() );
      }
    }
  }

  emit createRaster( inputLayer, bufferDistance, decayRatio, outputFileName, outputFormat );

  //and finally
  accept();
}

void HeatmapGui::on_mButtonBox_rejected()
{
  reject();
}

void HeatmapGui::on_mButtonBox_helpRequested()
{
  QgsContextHelp::run( metaObject()->className() );
}

void HeatmapGui::on_mBrowseButton_clicked()
{
  QSettings s;
  QString lastDir = s.value( "/Heatmap/lastOutputDir", "" ).toString();

  QString outputFilename = QFileDialog::getSaveFileName( 0, tr( "Save Heatmap as:" ), lastDir );
  if ( !outputFilename.isEmpty() )
  {
    mOutputRasterLineEdit->setText( outputFilename );
    QFileInfo outputFileInfo( outputFilename );
    QDir outputDir = outputFileInfo.absoluteDir();
    if ( outputDir.exists() )
    {
      s.setValue( "/Heatmap/lastOutputDir", outputFileInfo.absolutePath() );
    }
  }

  enableOrDisableOkButton();
}

void HeatmapGui::on_mOutputRasterLineEdit_editingFinished()
{
  enableOrDisableOkButton();
}

void HeatmapGui::enableOrDisableOkButton()
{
  bool enabled = true;
  QString filename = mOutputRasterLineEdit->text();
  QFileInfo theFileInfo( filename );
  if ( filename.isEmpty() || !theFileInfo.dir().exists() || ( mInputVectorCombo->count() == 0 ) )
  {
    enabled = false;
  }
  mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( enabled );
}
