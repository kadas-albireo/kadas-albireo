/***************************************************************************
*    qgsglobeplugindialog.cpp - settings dialog for the globe plugin
*     --------------------------------------
*    Date                 : 11-Nov-2010
*    Copyright            : (C) 2010 by Marco Bernasocchi
*    Email                : marco at bernawebdesign.ch
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "qgsglobeplugindialog.h"
#include "globe_plugin.h"

#include <qgsapplication.h>
#include <qgsproject.h>
#include <qgslogger.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSettings>
#include <QTimer>

#include <osg/DisplaySettings>
#include <osgViewer/Viewer>
#include <osgEarth/Version>
#include <osgEarthUtil/EarthManipulator>

QgsGlobePluginDialog::QgsGlobePluginDialog( QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl )
{
  setupUi( this );

  comboBoxBaseLayer->addItem( "Readymap: NASA BlueMarble Imagery", "http://readymap.org/readymap/tiles/1.0.0/1/" );
  comboBoxBaseLayer->addItem( "Readymap: NASA BlueMarble with land removed, only ocean", "http://readymap.org/readymap/tiles/1.0.0/2/" );
  comboBoxBaseLayer->addItem( "Readymap: High resolution insets from various locations around the world Austin, TX; Kandahar, Afghanistan; Bagram, Afghanistan; Boston, MA; Washington, DC", "http://readymap.org/readymap/tiles/1.0.0/3/" );
  comboBoxBaseLayer->addItem( "Readymap: Global Land Cover Facility 15m Landsat", "http://readymap.org/readymap/tiles/1.0.0/6/" );
  comboBoxBaseLayer->addItem( "Readymap: NASA BlueMarble + Landsat + Ocean Masking Layer", "http://readymap.org/readymap/tiles/1.0.0/7/" );
  comboBoxBaseLayer->addItem( tr( "[Custom]" ) );

  comboBoxStereoMode->addItem( "OFF", -1 );
  comboBoxStereoMode->addItem( "ANAGLYPHIC", osg::DisplaySettings::ANAGLYPHIC );
  comboBoxStereoMode->addItem( "QUAD_BUFFER", osg::DisplaySettings::ANAGLYPHIC );
  comboBoxStereoMode->addItem( "HORIZONTAL_SPLIT", osg::DisplaySettings::HORIZONTAL_SPLIT );
  comboBoxStereoMode->addItem( "VERTICAL_SPLIT", osg::DisplaySettings::VERTICAL_SPLIT );

  elevationDatasourcesWidget->setColumnHidden( 1, true ); // Hide cache column for the moment


  lineEditAASamples->setValidator( new QIntValidator( lineEditAASamples ) );

  lineEditElevationLayerPath->setText( QDir::homePath() );

#if OSGEARTH_VERSION_LESS_THAN( 2, 5, 0 )
  mSpinBoxVerticalScale->setVisible( false );
#endif

  connect( checkBoxSkyAutoAmbient, SIGNAL( toggled( bool ) ), horizontalSliderMinAmbient, SLOT( setEnabled( bool ) ) );
  connect( buttonBox->button( QDialogButtonBox::Apply ), SIGNAL( clicked( bool ) ), this, SLOT( apply() ) );

  restoreSavedSettings();
}

void QgsGlobePluginDialog::restoreSavedSettings()
{
  QSettings settings;

  // Stereo settings
  comboBoxStereoMode->setCurrentIndex( comboBoxStereoMode->findText( settings.value( "/Plugin-Globe/stereoMode", "OFF" ).toString() ) );
  spinBoxStereoScreenDistance->setValue( settings.value( "/Plugin-Globe/spinBoxStereoScreenDistance",
                                         osg::DisplaySettings::instance()->getScreenDistance() ).toDouble() );
  spinBoxStereoScreenWidth->setValue( settings.value( "/Plugin-Globe/spinBoxStereoScreenWidth",
                                      osg::DisplaySettings::instance()->getScreenWidth() ).toDouble() );
  spinBoxStereoScreenHeight->setValue( settings.value( "/Plugin-Globe/spinBoxStereoScreenHeight",
                                       osg::DisplaySettings::instance()->getScreenHeight() ).toDouble() );
  spinBoxStereoEyeSeparation->setValue( settings.value( "/Plugin-Globe/spinBoxStereoEyeSeparation",
                                        osg::DisplaySettings::instance()->getEyeSeparation() ).toDouble() );
  spinBoxSplitStereoHorizontalSeparation->setValue( settings.value( "/Plugin-Globe/spinBoxSplitStereoHorizontalSeparation",
      osg::DisplaySettings::instance()->getSplitStereoHorizontalSeparation() ).toInt() );
  spinBoxSplitStereoVerticalSeparation->setValue( settings.value( "/Plugin-Globe/spinBoxSplitStereoVerticalSeparation",
      osg::DisplaySettings::instance()->getSplitStereoVerticalSeparation() ).toInt() );
  comboBoxSplitStereoHorizontalEyeMapping->setCurrentIndex( settings.value( "/Plugin-Globe/comboBoxSplitStereoHorizontalEyeMapping",
      osg::DisplaySettings::instance()->getSplitStereoHorizontalEyeMapping() ).toInt() );
  comboBoxSplitStereoVerticalEyeMapping->setCurrentIndex( settings.value( "/Plugin-Globe/comboBoxSplitStereoVerticalEyeMapping",
      osg::DisplaySettings::instance()->getSplitStereoVerticalEyeMapping() ).toInt() );

  // Navigation settings
  sliderScrollSensitivity->setValue( settings.value( "/Plugin-Globe/scrollSensitivity", 20 ).toInt() );
  checkBoxInvertScroll->setChecked( settings.value( "/Plugin-Globe/invertScrollWheel", 0 ).toInt() );

  // Video settings
  groupBoxAntiAliasing->setChecked( settings.value( "/Plugin-Globe/anti-aliasing", false ).toBool() );
  lineEditAASamples->setText( settings.value( "/Plugin-Globe/anti-aliasing-level", "" ).toString() );

  // Advanced
  checkBoxFrustumHighlighting->setChecked( settings.value( "/Plugin-Globe/frustum-highlighting", false ).toBool() );
  checkBoxFeatureIdentification->setChecked( settings.value( "/Plugin-Globe/feature-identification", false ).toBool() );
}

void QgsGlobePluginDialog::on_buttonBox_accepted()
{
  apply();
  accept();
}

void QgsGlobePluginDialog::on_buttonBox_rejected()
{
  restoreSavedSettings();
  readProjectSettings();
  reject();
}

void QgsGlobePluginDialog::apply()
{
  QSettings settings;

  // Stereo settings
  settings.setValue( "/Plugin-Globe/stereoMode", comboBoxStereoMode->currentText() );
  settings.setValue( "/Plugin-Globe/stereoScreenDistance", spinBoxStereoScreenDistance->value() );
  settings.setValue( "/Plugin-Globe/stereoScreenWidth", spinBoxStereoScreenWidth->value() );
  settings.setValue( "/Plugin-Globe/stereoScreenHeight", spinBoxStereoScreenHeight->value() );
  settings.setValue( "/Plugin-Globe/stereoEyeSeparation", spinBoxStereoEyeSeparation->value() );
  settings.setValue( "/Plugin-Globe/SplitStereoHorizontalSeparation", spinBoxSplitStereoHorizontalSeparation->value() );
  settings.setValue( "/Plugin-Globe/SplitStereoVerticalSeparation", spinBoxSplitStereoVerticalSeparation->value() );
  settings.setValue( "/Plugin-Globe/SplitStereoHorizontalEyeMapping", comboBoxSplitStereoHorizontalEyeMapping->currentIndex() );
  settings.setValue( "/Plugin-Globe/SplitStereoVerticalEyeMapping", comboBoxSplitStereoVerticalEyeMapping->currentIndex() );

  // Navigation settings
  settings.setValue( "/Plugin-Globe/scrollSensitivity", sliderScrollSensitivity->value() );
  settings.setValue( "/Plugin-Globe/invertScrollWheel", checkBoxInvertScroll->checkState() );

  // Video settings
  settings.setValue( "/Plugin-Globe/anti-aliasing", groupBoxAntiAliasing->isChecked() );
  settings.setValue( "/Plugin-Globe/anti-aliasing-level", lineEditAASamples->text() );

  // Advanced settings
  settings.setValue( "/Plugin-Globe/frustum-highlighting", checkBoxFrustumHighlighting->isChecked() );
  settings.setValue( "/Plugin-Globe/feature-identification", checkBoxFeatureIdentification->isChecked() );

  writeProjectSettings();

  // Apply stereo settings
  int stereoMode = comboBoxStereoMode->itemData( comboBoxStereoMode->currentIndex() ).toInt();
  if ( stereoMode == -1 )
  {
    osg::DisplaySettings::instance()->setStereo( false );
  }
  else
  {
    osg::DisplaySettings::instance()->setStereo( true );
    osg::DisplaySettings::instance()->setStereoMode(
      static_cast<osg::DisplaySettings::StereoMode>( stereoMode ) );
    osg::DisplaySettings::instance()->setEyeSeparation( spinBoxStereoEyeSeparation->value() );
    osg::DisplaySettings::instance()->setScreenDistance( spinBoxStereoScreenDistance->value() );
    osg::DisplaySettings::instance()->setScreenWidth( spinBoxStereoScreenWidth->value() );
    osg::DisplaySettings::instance()->setScreenHeight( spinBoxStereoScreenHeight->value() );
    osg::DisplaySettings::instance()->setSplitStereoVerticalSeparation(
      spinBoxSplitStereoVerticalSeparation->value() );
    osg::DisplaySettings::instance()->setSplitStereoVerticalEyeMapping(
      static_cast<osg::DisplaySettings::SplitStereoVerticalEyeMapping>(
        comboBoxSplitStereoVerticalEyeMapping->currentIndex() ) );
    osg::DisplaySettings::instance()->setSplitStereoHorizontalSeparation(
      spinBoxSplitStereoHorizontalSeparation->value() );
    osg::DisplaySettings::instance()->setSplitStereoHorizontalEyeMapping(
      static_cast<osg::DisplaySettings::SplitStereoHorizontalEyeMapping>(
        comboBoxSplitStereoHorizontalEyeMapping->currentIndex() ) );
  }

  emit settingsApplied();
}

void QgsGlobePluginDialog::readProjectSettings()
{
  // Elevation settings
  elevationDatasourcesWidget->setRowCount( 0 );
  foreach ( const ElevationDataSource& ds, getElevationDataSources() )
  {
    int row = elevationDatasourcesWidget->rowCount();
    elevationDatasourcesWidget->setRowCount( row + 1 );
    elevationDatasourcesWidget->setItem( row, 0, new QTableWidgetItem( ds.type ) );
    QTableWidgetItem* chkBoxItem = new QTableWidgetItem();
    //chkBoxItem->setCheckState( ds.cache ? Qt::Checked : Qt::Unchecked );
    elevationDatasourcesWidget->setItem( row, 1, chkBoxItem );
    elevationDatasourcesWidget->setItem( row, 2, new QTableWidgetItem( ds.uri ) );
  }

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
  mSpinBoxVerticalScale->setValue( QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/verticalScale", 1 ) );
#endif

  // Map settings
  mBaseLayerGroupBox->setChecked( QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/baseLayerEnabled/", true ) );
  lineEditBaseLayerURL->setText( QgsProject::instance()->readEntry( "Globe-Plugin", "/baseLayerURL/", "http://readymap.org/readymap/tiles/1.0.0/7/" ) );
  int idx = comboBoxBaseLayer->findData( lineEditBaseLayerURL->text() );
  comboBoxBaseLayer->setCurrentIndex( idx == -1 ? comboBoxBaseLayer->count() - 1 : idx );
  groupBoxSky->setChecked( QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/skyEnabled", true ) );
  dateTimeEditSky->setDateTime( QDateTime::fromString( QgsProject::instance()->readEntry( "Globe-Plugin", "/skyDateTime", QDateTime::currentDateTime().toString() ) ) );
  checkBoxSkyAutoAmbient->setChecked( QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/skyAutoAmbient", true ) );
  horizontalSliderMinAmbient->setValue( QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/skyMinAmbient", 30. ) );
}

void QgsGlobePluginDialog::writeProjectSettings()
{
  // Elevation settings
  QgsProject::instance()->removeEntry( "Globe-Plugin", "/elevationDatasources/" );
  for ( int row = 0, nRows = elevationDatasourcesWidget->rowCount(); row < nRows; ++row )
  {
    QString type  = elevationDatasourcesWidget->item( row, 0 )->text();
    QString uri   = elevationDatasourcesWidget->item( row, 2 )->text();
    bool    cache = elevationDatasourcesWidget->item( row, 1 )->checkState();
    QString key = QString( "/elevationDatasources/L%1" ).arg( row );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/type", type );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/uri", uri );
    QgsProject::instance()->writeEntry( "Globe-Plugin", key + "/cache", cache );
  }

#if OSGEARTH_VERSION_GREATER_OR_EQUAL( 2, 5, 0 )
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/verticalScale", mSpinBoxVerticalScale->value() );
#endif

  // Map settings
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/baseLayerEnabled/", mBaseLayerGroupBox->isChecked() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/baseLayerURL/", lineEditBaseLayerURL->text() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/skyEnabled/", groupBoxSky->isChecked() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/skyDateTime/", dateTimeEditSky->dateTime().toString() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/skyAutoAmbient/", checkBoxSkyAutoAmbient->isChecked() );
  QgsProject::instance()->writeEntry( "Globe-Plugin", "/skyMinAmbient/", horizontalSliderMinAmbient->value() );
}

/// ELEVATION /////////////////////////////////////////////////////////////////

QString QgsGlobePluginDialog::validateElevationResource( QString type, QString uri )
{
  if ( "Raster" == type )
  {
    QFileInfo file( uri );
    if ( file.isFile() && file.isReadable() )
      return QString::null;
    else
      return tr( "Invalid Path: The file is either unreadable or does not exist" );
  }
  else
  {
    QNetworkAccessManager networkMgr;
    QNetworkReply* reply = nullptr;
    QUrl url( uri );
    int timeout = 5000; // 5 sec

    while ( true )
    {
      QNetworkRequest req( url );
      req.setRawHeader( "User-Agent" , "Wget/1.13.4" );
      reply = networkMgr.get( req );
      QTimer timer;
      QEventLoop loop;
      QObject::connect( &timer, SIGNAL( timeout() ), &loop, SLOT( quit() ) );
      QObject::connect( reply, SIGNAL( finished() ), &loop, SLOT( quit() ) );
      timer.setSingleShot( true );
      timer.start( timeout );
      loop.exec();
      if ( reply->isRunning() )
      {
        reply->close();
        delete reply;
        return tr( "Error contacting server: timeout" );
      }

      QUrl redirectUrl = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
      if ( redirectUrl.isValid() && url != redirectUrl )
      {
        delete reply;
        url = redirectUrl;
      }
      else
      {
        break;
      }
    }

    if ( reply->error() == QNetworkReply::NoError )
      return QString::null;
    else
      return tr( "Error contacting server: %1" ).arg( reply->errorString() );
  }
}

void QgsGlobePluginDialog::on_comboBoxElevationLayerType_currentIndexChanged( QString type )
{
  lineEditElevationLayerPath->setEnabled( true );
  if ( "Raster" == type )
  {
    pushButtonElevationLayerBrowse->setEnabled( true );
    lineEditElevationLayerPath->setText( QDir::homePath() );
  }
  else if ( "Worldwind" == type )
  {
    pushButtonElevationLayerBrowse->setEnabled( false );
    lineEditElevationLayerPath->setText( "http://tileservice.worldwindcentral.com/getTile?bmng.topo.bathy.200401" );
    lineEditElevationLayerPath->setEnabled( false );
  }
  else if ( "TMS" == type )
  {
    pushButtonElevationLayerBrowse->setEnabled( false );
    lineEditElevationLayerPath->setText( "http://readymap.org/readymap/tiles/1.0.0/9/" );
  }
}

void QgsGlobePluginDialog::on_pushButtonElevationLayerBrowse_clicked()
{
  //see http://www.gdal.org/formats_list.html
  QString filter = tr( "GDAL files" ) + " (*.dem *.tif *.tiff *.jpg *.jpeg *.asc);;"
                   + tr( "DEM files" ) + " (*.dem);;"
                   + tr( "All files" ) + " (*.*)";
  QString newPath = QFileDialog::getOpenFileName( this, tr( "Open raster file" ), QDir::homePath(), filter );
  if ( ! newPath.isEmpty() )
  {
    lineEditElevationLayerPath->setText( newPath );
  }
}

void QgsGlobePluginDialog::on_pushButtonElevationLayerAdd_clicked()
{
  QString error = validateElevationResource( comboBoxElevationLayerType->currentText(), lineEditElevationLayerPath->text() );
  QString question = tr( "Do you want to add the datasource anyway?" );
  if ( error.isNull() || QMessageBox::warning( this, tr( "Warning" ), error + "\n" + question, QMessageBox::Ok, QMessageBox::Cancel ) == QMessageBox::Ok )
  {
    int row = elevationDatasourcesWidget->rowCount();
    //cache->setCheckState(( comboBoxElevationLayerType->currentText() == "Worldwind" ) ? Qt::Checked : Qt::Unchecked ); //worldwind_cache will be active
    elevationDatasourcesWidget->setRowCount( row + 1 );
    elevationDatasourcesWidget->setItem( row, 0, new QTableWidgetItem( comboBoxElevationLayerType->currentText() ) );
    elevationDatasourcesWidget->setItem( row, 1, new QTableWidgetItem() );
    elevationDatasourcesWidget->setItem( row, 2, new QTableWidgetItem( lineEditElevationLayerPath->text() ) );
    elevationDatasourcesWidget->setCurrentIndex( elevationDatasourcesWidget->model()->index( row, 0 ) );
  }
}

void QgsGlobePluginDialog::on_pushButtonElevationLayerRemove_clicked()
{
  elevationDatasourcesWidget->removeRow( elevationDatasourcesWidget->currentRow() );
}

void QgsGlobePluginDialog::on_pushButtonElevationLayerUp_clicked()
{
  moveRow( elevationDatasourcesWidget, -1 );
}

void QgsGlobePluginDialog::on_pushButtonElevationLayerDown_clicked()
{
  moveRow( elevationDatasourcesWidget, + 1 );
}

void QgsGlobePluginDialog::moveRow( QTableWidget* widget, int offset )
{
  if ( widget->currentItem() != 0 )
  {
    int srcRow = widget->currentItem()->row();
    int dstRow = srcRow + offset;
    if ( dstRow >= 0 && dstRow < widget->rowCount() )
    {
      for ( int col = 0, nCols = widget->columnCount(); col < nCols; ++col )
      {
        QTableWidgetItem* srcItem = widget->takeItem( srcRow, col );
        QTableWidgetItem* dstItem = widget->takeItem( dstRow, col );
        widget->setItem( dstRow, col, srcItem );
        widget->setItem( srcRow, col, dstItem );
      }
      widget->selectRow( dstRow );
    }
  }
}

QList<QgsGlobePluginDialog::ElevationDataSource> QgsGlobePluginDialog::getElevationDataSources() const
{
  int keysCount = QgsProject::instance()->subkeyList( "Globe-Plugin", "/elevationDatasources/" ).count();
  QList<ElevationDataSource> datasources;
  for ( int i = 0; i < keysCount; ++i )
  {
    QString key = QString( "/elevationDatasources/L%1" ).arg( i );
    ElevationDataSource datasource;
    datasource.type  = QgsProject::instance()->readEntry( "Globe-Plugin", key + "/type" );
    datasource.uri   = QgsProject::instance()->readEntry( "Globe-Plugin", key + "/uri" );
//  datasource.cache = QgsProject::instance()->readBoolEntry( "Globe-Plugin", key + "/cache" );
    datasources.append( datasource );
  }
  return datasources;
}

double QgsGlobePluginDialog::getVerticalScale() const
{
  return mSpinBoxVerticalScale->value();
}

/// MAP ///////////////////////////////////////////////////////////////////////

void QgsGlobePluginDialog::on_comboBoxBaseLayer_currentIndexChanged( int index )
{
  QVariant url = comboBoxBaseLayer->itemData( index );
  if ( url.isValid() )
  {
    lineEditBaseLayerURL->setEnabled( false );
    lineEditBaseLayerURL->setText( url.toString() );
  }
  else
  {
    lineEditBaseLayerURL->setEnabled( true );
  }
}

QString QgsGlobePluginDialog::getBaseLayerUrl() const
{
  return mBaseLayerGroupBox->isChecked() ? lineEditBaseLayerURL->text() : QString::null;
}

bool QgsGlobePluginDialog::getSkyEnabled() const
{
  return QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/skyEnabled", true );
}

QDateTime QgsGlobePluginDialog::getSkyDateTime() const
{
  return QDateTime::fromString( QgsProject::instance()->readEntry( "Globe-Plugin", "/skyDateTime", QDateTime::currentDateTime().toString() ) );
}

bool QgsGlobePluginDialog::getSkyAutoAmbience() const
{
  return QgsProject::instance()->readBoolEntry( "Globe-Plugin", "/skyAutoAmbient", true );
}

double QgsGlobePluginDialog::getSkyMinAmbient() const
{
  return QgsProject::instance()->readDoubleEntry( "Globe-Plugin", "/skyMinAmbient", 30. ) / 100.;
}

/// NAVIGATION ////////////////////////////////////////////////////////////////

float QgsGlobePluginDialog::getScrollSensitivity() const
{
  return sliderScrollSensitivity->value() / 10;
}

bool QgsGlobePluginDialog::getInvertScrollWheel() const
{
  return checkBoxInvertScroll->checkState();
}

/// ADVANCED //////////////////////////////////////////////////////////////////

bool QgsGlobePluginDialog::getFrustumHighlighting() const
{
  return checkBoxFrustumHighlighting->isChecked();
}

bool QgsGlobePluginDialog::getFeatureIdenification() const
{
  return checkBoxFeatureIdentification->isChecked();
}

/// STEREO ////////////////////////////////////////////////////////////////////
void QgsGlobePluginDialog::on_pushButtonStereoResetDefaults_clicked()
{
  //http://www.openscenegraph.org/projects/osg/wiki/Support/UserGuides/StereoSettings
  comboBoxStereoMode->setCurrentIndex( comboBoxStereoMode->findText( "OFF" ) );
  spinBoxStereoScreenDistance->setValue( 0.5 );
  spinBoxStereoScreenHeight->setValue( 0.26 );
  spinBoxStereoScreenWidth->setValue( 0.325 );
  spinBoxStereoEyeSeparation->setValue( 0.06 );
  spinBoxSplitStereoHorizontalSeparation->setValue( 42 );
  spinBoxSplitStereoVerticalSeparation->setValue( 42 );
  comboBoxSplitStereoHorizontalEyeMapping->setCurrentIndex( 0 );
  comboBoxSplitStereoVerticalEyeMapping->setCurrentIndex( 0 );
}

void QgsGlobePluginDialog::on_comboBoxStereoMode_currentIndexChanged( int index )
{
  //http://www.openscenegraph.org/projects/osg/wiki/Support/UserGuides/StereoSettings
  //http://www.openscenegraph.org/documentation/OpenSceneGraphReferenceDocs/a00181.html

  int stereoMode = comboBoxStereoMode->itemData( index ).toInt();
  bool stereoEnabled = stereoMode != -1;
  bool verticalSplit = stereoMode == osg::DisplaySettings::VERTICAL_SPLIT;
  bool horizontalSplit = stereoMode == osg::DisplaySettings::HORIZONTAL_SPLIT;

  spinBoxStereoScreenDistance->setEnabled( stereoEnabled );
  spinBoxStereoScreenHeight->setEnabled( stereoEnabled );
  spinBoxStereoScreenWidth->setEnabled( stereoEnabled );
  spinBoxStereoEyeSeparation->setEnabled( stereoEnabled );
  spinBoxSplitStereoHorizontalSeparation->setEnabled( stereoEnabled && horizontalSplit );
  comboBoxSplitStereoHorizontalEyeMapping->setEnabled( stereoEnabled && horizontalSplit );
  spinBoxSplitStereoVerticalSeparation->setEnabled( stereoEnabled && verticalSplit );
  comboBoxSplitStereoVerticalEyeMapping->setEnabled( stereoEnabled && verticalSplit );
}
