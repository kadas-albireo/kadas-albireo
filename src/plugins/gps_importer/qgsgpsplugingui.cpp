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
#include "qgsgpsplugingui.h"
#include "qgsgpsdevicedialog.h"
#include "qgsmaplayer.h"
#include "qgsdataprovider.h"
#include "qgscontexthelp.h"
#include "qgslogger.h"

//qt includes
#include <QFileDialog>
#include <QSettings>

//standard includes
#include <cassert>
#include <cstdlib>
#include <iostream>


QgsGPSPluginGui::QgsGPSPluginGui( const BabelMap& importers,
                                  std::map<QString, QgsGPSDevice*>& devices,
                                  std::vector<QgsVectorLayer*> gpxMapLayers,
                                  QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl ), mGPXLayers( gpxMapLayers ),
    mImporters( importers ), mDevices( devices )
{
  setupUi( this );
  populatePortComboBoxes();
  populateULLayerComboBox();
  populateIMPBabelFormats();
  populateLoadDialog();
  populateULDialog();
  populateDLDialog();
  populateIMPDialog();
  populateCONVDialog();

  connect( pbULEditDevices, SIGNAL( clicked() ), this, SLOT( openDeviceEditor() ) );
  connect( pbDLEditDevices, SIGNAL( clicked() ), this, SLOT( openDeviceEditor() ) );

  // make sure that the OK button is enabled only when it makes sense to
  // click it
  pbnOK = buttonBox->button( QDialogButtonBox::Ok );
  pbnOK->setEnabled( false );
  connect( leGPXFile, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( leIMPInput, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( leIMPOutput, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( leIMPLayer, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( leCONVInput, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( leCONVOutput, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( leCONVLayer, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( leDLOutput, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( leDLBasename, SIGNAL( textChanged( const QString& ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( cmbULLayer, SIGNAL( textChanged( QString ) ),
           this, SLOT( enableRelevantControls() ) );
  connect( tabWidget, SIGNAL( currentChanged( int ) ),
           this, SLOT( enableRelevantControls() ) );

  // drag and drop filter
  leGPXFile->setSuffixFilter( "gpx" );
}

QgsGPSPluginGui::~QgsGPSPluginGui()
{
}

void QgsGPSPluginGui::on_buttonBox_accepted()
{

  // what should we do?
  switch ( tabWidget->currentIndex() )
  {
      // add a GPX layer?
    case 0:
      emit loadGPXFile( leGPXFile->text(), cbGPXWaypoints->isChecked(),
                        cbGPXRoutes->isChecked(), cbGPXTracks->isChecked() );
      break;

      // or import other file?
    case 1:
    {
      const QString& typeString( cmbIMPFeature->currentText() );
      emit importGPSFile( leIMPInput->text(),
                          mImporters.find( mImpFormat )->second,
                          typeString == tr( "Waypoints" ), typeString == tr( "Routes" ),
                          typeString == tr( "Tracks" ), leIMPOutput->text(),
                          leIMPLayer->text() );
      break;
    }
    // or download GPS data from a device?
    case 2:
    {
      int featureType = cmbDLFeatureType->currentIndex();

      QString fileName = leDLOutput->text();
      if ( fileName.right( 4 ) != ".gpx" )
      {
        fileName += ".gpx";
      }

      emit downloadFromGPS( cmbDLDevice->currentText(), cmbDLPort->currentText(),
                            featureType == 0, featureType == 1, featureType == 2,
                            fileName, leDLBasename->text() );
      break;
    }
    // or upload GPS data to a device?
    case 3:
    {
      emit uploadToGPS( mGPXLayers[cmbULLayer->currentIndex()],
                        cmbULDevice->currentText(), cmbULPort->currentText() );
      break;
    }
    // or convert between waypoints/tracks=
    case 4:
    {
      int convertType = cmbCONVType->currentIndex();
      emit convertGPSFile( leCONVInput->text(),
                           convertType,
                           leCONVOutput->text(),
                           leCONVLayer->text() );
      break;
    }
  }
  // The slots that are called above will emit closeGui() when successful.
  // If not successful, the user will get another shot without starting from scratch
  // accept();
}


void QgsGPSPluginGui::on_pbnDLOutput_clicked()
{
  QString myFileNameQString =
    QFileDialog::getSaveFileName( this, //parent dialog
                                  tr( "Choose a file name to save under" ),
                                  ".", //initial dir
                                  tr( "GPS eXchange format (*.gpx)" ) );
  if ( !myFileNameQString.isEmpty() )
    leDLOutput->setText( myFileNameQString );
}


void QgsGPSPluginGui::enableRelevantControls()
{
  // load GPX
  if ( tabWidget->currentIndex() == 0 )
  {
    if (( leGPXFile->text() == "" ) )
    {
      pbnOK->setEnabled( false );
      cbGPXWaypoints->setEnabled( false );
      cbGPXRoutes->setEnabled( false );
      cbGPXTracks->setEnabled( false );
      cbGPXWaypoints->setChecked( false );
      cbGPXRoutes->setChecked( false );
      cbGPXTracks->setChecked( false );
    }
    else
    {
      pbnOK->setEnabled( true );
      cbGPXWaypoints->setEnabled( true );
      cbGPXWaypoints->setChecked( true );
      cbGPXRoutes->setEnabled( true );
      cbGPXTracks->setEnabled( true );
      cbGPXRoutes->setChecked( true );
      cbGPXTracks->setChecked( true );
    }
  }

  // import other file
  else if ( tabWidget->currentIndex() == 1 )
  {

    if (( leIMPInput->text() == "" ) || ( leIMPOutput->text() == "" ) ||
        ( leIMPLayer->text() == "" ) )
      pbnOK->setEnabled( false );
    else
      pbnOK->setEnabled( true );
  }

  // download from device
  else if ( tabWidget->currentIndex() == 2 )
  {
    if ( cmbDLDevice->currentText() == "" || leDLBasename->text() == "" ||
         leDLOutput->text() == "" )
      pbnOK->setEnabled( false );
    else
      pbnOK->setEnabled( true );
  }

  // upload to device
  else if ( tabWidget->currentIndex() == 3 )
  {
    if ( cmbULDevice->currentText() == "" || cmbULLayer->currentText() == "" )
      pbnOK->setEnabled( false );
    else
      pbnOK->setEnabled( true );
  }

  // convert between waypoint/routes
  else if ( tabWidget->currentIndex() == 4 )
  {

    if (( leCONVInput->text() == "" ) || ( leCONVOutput->text() == "" ) ||
        ( leCONVLayer->text() == "" ) )
      pbnOK->setEnabled( false );
    else
      pbnOK->setEnabled( true );
  }
}


void QgsGPSPluginGui::on_buttonBox_rejected()
{
  reject();
}


void QgsGPSPluginGui::on_pbnGPXSelectFile_clicked()
{
  QgsLogger::debug( " Gps File Importer::pbnGPXSelectFile_clicked() " );
  QString myFileTypeQString;
  QString myFilterString = tr( "GPS eXchange format (*.gpx)" );
  QSettings settings;
  QString dir = settings.value( "/Plugin-GPS/gpxdirectory" ).toString();
  if ( dir.isEmpty() )
    dir = ".";
  QString myFileNameQString = QFileDialog::getOpenFileName(
                                this, //parent dialog
                                tr( "Select GPX file" ), //caption
                                dir, //initial dir
                                myFilterString, //filters to select
                                &myFileTypeQString ); //the pointer to store selected filter
  QgsLogger::debug( "Selected filetype filter is : " + myFileTypeQString );
  if ( !myFileNameQString.isEmpty() )
    leGPXFile->setText( myFileNameQString );
}


void QgsGPSPluginGui::on_pbnIMPInput_clicked()
{
  QString myFileType;
  QString myFileName = QFileDialog::getOpenFileName(
                         this, //parent dialog
                         tr( "Select file and format to import" ), //caption
                         ".", //initial dir
                         mBabelFilter,
                         &myFileType ); //the pointer to store selected filter
  if ( !myFileName.isEmpty() )
  {
    mImpFormat = myFileType.left( myFileType.length() - 6 );
    std::map<QString, QgsBabelFormat*>::const_iterator iter;
    iter = mImporters.find( mImpFormat );
    if ( iter == mImporters.end() )
    {
      QgsLogger::warning( "Unknown file format selected: " +
                          myFileType.left( myFileType.length() - 6 ) );
    }
    else
    {
      QgsLogger::debug( iter->first + " selected" );
      leIMPInput->setText( myFileName );
      cmbIMPFeature->clear();
      if ( iter->second->supportsWaypoints() )
        cmbIMPFeature->addItem( tr( "Waypoints" ) );
      if ( iter->second->supportsRoutes() )
        cmbIMPFeature->addItem( tr( "Routes" ) );
      if ( iter->second->supportsTracks() )
        cmbIMPFeature->addItem( tr( "Tracks" ) );
    }
  }
}


void QgsGPSPluginGui::on_pbnIMPOutput_clicked()
{
  QString myFileNameQString =
    QFileDialog::getSaveFileName( this, //parent dialog
                                  tr( "Choose a file name to save under" ),
                                  ".", //initial dir
                                  tr( "GPS eXchange format (*.gpx)" ) );
  if ( !myFileNameQString.isEmpty() )
    leIMPOutput->setText( myFileNameQString );
}

void QgsGPSPluginGui::on_pbnRefresh_clicked()
{
  populatePortComboBoxes();
}

void QgsGPSPluginGui::populatePortComboBoxes()
{

  cmbDLPort->clear();
#ifdef linux
  // look for linux serial devices
  QString linuxDev( "/dev/ttyS%1" );
  for ( int i = 0; i < 10; ++i )
  {
    if ( QFileInfo( linuxDev.arg( i ) ).exists() )
    {
      cmbDLPort->addItem( linuxDev.arg( i ) );
      cmbULPort->addItem( linuxDev.arg( i ) );
    }
    else
      break;
  }

  // and the ttyUSB* devices (serial USB adaptor)
  linuxDev = "/dev/ttyUSB%1";
  for ( int i = 0; i < 10; ++i )
  {
    if ( QFileInfo( linuxDev.arg( i ) ).exists() )
    {
      cmbDLPort->addItem( linuxDev.arg( i ) );
      cmbULPort->addItem( linuxDev.arg( i ) );
    }
    else
      break;
  }

  cmbDLPort->addItem( "usb:" );
  cmbULPort->addItem( "usb:" );
#endif

#ifdef __FreeBSD__ // freebsd
  // and freebsd devices (untested)
  QString freebsdDev( "/dev/cuaa%1" );
  for ( int i = 0; i < 10; ++i )
  {
    if ( QFileInfo( freebsdDev.arg( i ) ).exists() )
    {
      cmbDLPort->addItem( freebsdDev.arg( i ) );
      cmbULPort->addItem( freebsdDev.arg( i ) );
    }
    else
      break;
  }

  // and the ucom devices (serial USB adaptors)
  freebsdDev = "/dev/ucom%1";
  for ( int i = 0; i < 10; ++i )
  {
    if ( QFileInfo( freebsdDev.arg( i ) ).exists() )
    {
      cmbDLPort->addItem( freebsdDev.arg( i ) );
      cmbULPort->addItem( freebsdDev.arg( i ) );
    }
    else
      break;
  }

#endif

#ifdef sparc
  // and solaris devices (also untested)
  QString solarisDev( "/dev/cua/%1" );
  for ( int i = 'a'; i < 'k'; ++i )
  {
    if ( QFileInfo( solarisDev.arg( char( i ) ) ).exists() )
    {
      cmbDLPort->addItem( solarisDev.arg( char( i ) ) );
      cmbULPort->addItem( solarisDev.arg( char( i ) ) );
    }
    else
      break;
  }
#endif

#ifdef WIN32
  cmbULPort->addItem( "com1" );
  cmbULPort->addItem( "com2" );
  cmbULPort->addItem( "com3" );
  cmbULPort->addItem( "com4" );
  cmbULPort->addItem( "usb:" );
  cmbDLPort->addItem( "com1" );
  cmbDLPort->addItem( "com2" );
  cmbDLPort->addItem( "com3" );
  cmbDLPort->addItem( "com4" );
  cmbDLPort->addItem( "usb:" );
#endif

  // OSX, OpenBSD, NetBSD etc? Anyone?

  // remember the last ports used
  QSettings settings;
  QString lastDLPort = settings.value( "/Plugin-GPS/lastdlport", "" ).toString();
  QString lastULPort = settings.value( "/Plugin-GPS/lastulport", "" ).toString();
  for ( int i = 0; i < cmbDLPort->count(); ++i )
  {
    if ( cmbDLPort->itemText( i ) == lastDLPort )
    {
      cmbDLPort->setCurrentIndex( i );
      break;
    }
  }
  for ( int i = 0; i < cmbULPort->count(); ++i )
  {
    if ( cmbULPort->itemText( i ) == lastULPort )
    {
      cmbULPort->setCurrentIndex( i );
      break;
    }
  }
}


void QgsGPSPluginGui::populateULLayerComboBox()
{
  for ( std::vector<QgsVectorLayer*>::size_type i = 0; i < mGPXLayers.size(); ++i )
    cmbULLayer->addItem( mGPXLayers[i]->name() );
}


void QgsGPSPluginGui::populateIMPBabelFormats()
{
  mBabelFilter = "";
  cmbULDevice->clear();
  cmbDLDevice->clear();
  QSettings settings;
  QString lastDLDevice = settings.value( "/Plugin-GPS/lastdldevice", "" ).toString();
  QString lastULDevice = settings.value( "/Plugin-GPS/lastuldevice", "" ).toString();
  BabelMap::const_iterator iter;
  for ( iter = mImporters.begin(); iter != mImporters.end(); ++iter )
    mBabelFilter.append( iter->first ).append( " (*.*);;" );
  mBabelFilter.chop( 2 ); // Remove the trailing ;;, which otherwise leads to an empty filetype
  int u = -1, d = -1;
  std::map<QString, QgsGPSDevice*>::const_iterator iter2;
  for ( iter2 = mDevices.begin(); iter2 != mDevices.end(); ++iter2 )
  {
    cmbULDevice->addItem( iter2->first );
    if ( iter2->first == lastULDevice )
      u = cmbULDevice->count() - 1;
    cmbDLDevice->addItem( iter2->first );
    if ( iter2->first == lastDLDevice )
      d = cmbDLDevice->count() - 1;
  }
  if ( u != -1 )
    cmbULDevice->setCurrentIndex( u );
  if ( d != -1 )
    cmbDLDevice->setCurrentIndex( d );
}

void QgsGPSPluginGui::populateLoadDialog()
{

  QString format = QString( "<p>%1</p><p>%2</p>" );

  QString sentence1 = tr( "GPX is the %1, which is used to store information about waypoints, routes, and tracks." ).
                      arg( QString( "<a href=http://www.topografix.com/gpx.asp>%1</a>" ).arg( tr( "GPS eXchange file format" ) ) );
  QString sentence2 = tr( "Select a GPX file and then select the feature types that you want to load." );

  QString text = format.arg( sentence1 ).arg( sentence2 );

  teLoadDescription->setHtml( text );
  QgsDebugMsg( text );
}

void QgsGPSPluginGui::populateDLDialog()
{

  QString format = QString( "<p>%1 %2 %3<p>%4 %5</p>" );

  QString sentence1 = tr( "This tool will help you download data from a GPS device." );
  QString sentence2 = tr( "Choose your GPS device, the port it is connected to, the feature type you want to download, a name for your new layer, and the GPX file where you want to store the data." );
  QString sentence3 = tr( "If your device isn't listed, or if you want to change some settings, you can also edit the devices." );
  QString sentence4 = tr( "This tool uses the program GPSBabel (%1) to transfer the data." )
                      .arg( "<a href=\"http://www.gpsbabel.org\">http://www.gpsbabel.org</a>" );

  QString sentence5 = tr( "This requires that you have GPSBabel installed where QGIS can find it." );

  QString text = format.arg( sentence1 ).arg( sentence2 ).arg( sentence3 ).arg( sentence4 ).arg( sentence5 );

  teDLDescription->setHtml( text );
  QgsDebugMsg( text );
}

void QgsGPSPluginGui::populateULDialog()
{

  QString format = QString( "<p>%1 %2 %3<p>%4 %5</p>" );

  QString sentence1 = tr( "This tool will help you upload data from a GPX layer to a GPS device." );
  QString sentence2 = tr( "Choose the layer you want to upload, the device you want to upload it to, and the port your device is connected to." );
  QString sentence3 = tr( "If your device isn't listed, or if you want to change some settings, you can also edit the devices." );
  QString sentence4 = tr( "This tool uses the program GPSBabel (%1) to transfer the data." )
                      .arg( "<a href=\"http://www.gpsbabel.org\">http://www.gpsbabel.org</a>" );

  QString sentence5 = tr( "This requires that you have GPSBabel installed where QGIS can find it." );

  QString text = format.arg( sentence1 ).arg( sentence2 ).arg( sentence3 ).arg( sentence4 ).arg( sentence5 );

  teULDescription->setHtml( text );
  QgsDebugMsg( text );
}

void QgsGPSPluginGui::populateIMPDialog()
{

  QString format = QString( "<p>%1 %2<p><p>%3 %4</p>" );

  QString sentence1 = tr( "QGIS can only load GPX files by itself, but many other formats can be converted to GPX using GPSBabel (%1)." )
                      .arg( "<a href=\"http://www.gpsbabel.org\">http://www.gpsbabel.org</a>" );
  QString sentence2 = tr( "This requires that you have GPSBabel installed where QGIS can find it." );
  QString sentence3 = tr( "Select a GPS file format and the file that you want to import, the feature type that you want to use, a GPX file name that you want to save the converted file as, and a name for the new layer." );
  QString sentence4 = tr( "All file formats can not store waypoints, routes, and tracks, so some feature types may be disabled for some file formats." );

  QString text = format.arg( sentence1 ).arg( sentence2 ).arg( sentence3 ).arg( sentence4 );

  teIMPDescription->setHtml( text );
  QgsDebugMsg( text );
}


void QgsGPSPluginGui::populateCONVDialog()
{
  cmbCONVType->addItem( tr( "Routes" ) + " -> " + tr( "Waypoints" ) );
  cmbCONVType->addItem( tr( "Waypoints" ) + " -> " + tr( "Routes" ) );

  QString format = QString( "<html><body><p>%1 %2<p>%3</body></html>" );

  QString sentence1 = tr( "QGIS can perform conversions of GPX files, by using GPSBabel (%1) to perform the conversions." )
                      .arg( "<a href=\"http://www.gpsbabel.org\">http://www.gpsbabel.org</a>" );
  QString sentence2 = tr( "This requires that you have GPSBabel installed where QGIS can find it." );
  QString sentence3 = tr( "Select a GPX input file name, the type of conversion you want to perform, a GPX file name that you want to save the converted file as, and a name for the new layer created from the result." );

  QString text = format.arg( sentence1 ).arg( sentence2 ).arg( sentence3 );

  teCONVDescription->setHtml( text );
  QgsDebugMsg( text );
}

void QgsGPSPluginGui::on_pbnCONVInput_clicked()
{
  QString myFileTypeQString;
  QString myFilterString = tr( "GPS eXchange format (*.gpx)" );
  QSettings settings;
  QString dir = settings.value( "/Plugin-GPS/gpxdirectory" ).toString();
  if ( dir.isEmpty() )
    dir = ".";
  QString myFileNameQString = QFileDialog::getOpenFileName(
                                this, //parent dialog
                                tr( "Select GPX file" ), //caption
                                dir, //initial dir
                                myFilterString, //filters to select
                                &myFileTypeQString ); //the pointer to store selected filter
  if ( !myFileNameQString.isEmpty() )
    leCONVInput->setText( myFileNameQString );
}

void QgsGPSPluginGui::on_pbnCONVOutput_clicked()
{
  QString myFileNameQString =
    QFileDialog::getSaveFileName( this, //parent dialog
                                  tr( "Choose a file name to save under" ),
                                  ".", //initial dir
                                  tr( "GPS eXchange format (*.gpx)" ) );
  if ( !myFileNameQString.isEmpty() )
    leCONVOutput->setText( myFileNameQString );
}

void QgsGPSPluginGui::openDeviceEditor()
{
  QgsGPSDeviceDialog* dlg = new QgsGPSDeviceDialog( mDevices );
  dlg->show();
  connect( dlg, SIGNAL( devicesChanged() ), this, SLOT( devicesUpdated() ) );
}


void QgsGPSPluginGui::devicesUpdated()
{
  populateIMPBabelFormats();
}


void QgsGPSPluginGui::on_buttonBox_helpRequested()
{
  QgsContextHelp::run( context_id );
}
