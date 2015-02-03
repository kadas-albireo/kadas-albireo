/***************************************************************************
                          qgsoptions.cpp
                    Set user options and preferences
                             -------------------
    begin                : May 28, 2004
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
#include "qgsapplication.h"
#include "qgsdistancearea.h"
#include "qgsoptions.h"
#include "qgis.h"
#include "qgisapp.h"
#include "qgisappstylesheet.h"
#include "qgshighlight.h"
#include "qgsmapcanvas.h"
#include "qgsmaprenderer.h"
#include "qgsgenericprojectionselector.h"
#include "qgscoordinatereferencesystem.h"
#include "qgstolerance.h"
#include "qgsscaleutils.h"
#include "qgsnetworkaccessmanager.h"
#include "qgsproject.h"

#include "qgsattributetablefiltermodel.h"
#include "qgsrasterformatsaveoptionswidget.h"
#include "qgsrasterpyramidsoptionswidget.h"
#include "qgsdialog.h"
#include "qgscomposer.h"
#include "qgscolorschemeregistry.h"
#include "qgssymbollayerv2utils.h"
#include "qgscolordialog.h"

#include <QInputDialog>
#include <QFileDialog>
#include <QSettings>
#include <QColorDialog>
#include <QLocale>
#include <QProcess>
#include <QToolBar>
#include <QScrollBar>
#include <QSize>
#include <QStyleFactory>
#include <QMessageBox>
#include <QNetworkDiskCache>

#include <limits>
#include <sqlite3.h>
#include "qgslogger.h"

#define CPL_SUPRESS_CPLUSPLUS
#include <gdal.h>
#include <geos_c.h>
#include <cpl_conv.h> // for setting gdal options

#include "qgsconfig.h"

/**
 * \class QgsOptions - Set user options and preferences
 * Constructor
 */
QgsOptions::QgsOptions( QWidget *parent, Qt::WindowFlags fl ) :
    QgsOptionsDialogBase( "Options", parent, fl )
{
  setupUi( this );

  // QgsOptionsDialogBase handles saving/restoring of geometry, splitter and current tab states,
  // switching vertical tabs between icon/text to icon-only modes (splitter collapsed to left),
  // and connecting QDialogButtonBox's accepted/rejected signals to dialog's accept/reject slots
  initOptionsBase( false );

  // stylesheet setup
  mStyleSheetBuilder = QgisApp::instance()->styleSheetBuilder();
  mStyleSheetNewOpts = mStyleSheetBuilder->defaultOptions();
  mStyleSheetOldOpts = QMap<QString, QVariant>( mStyleSheetNewOpts );

  connect( cmbTheme, SIGNAL( activated( const QString& ) ), this, SLOT( themeChanged( const QString& ) ) );
  connect( cmbTheme, SIGNAL( highlighted( const QString& ) ), this, SLOT( themeChanged( const QString& ) ) );
  connect( cmbTheme, SIGNAL( textChanged( const QString& ) ), this, SLOT( themeChanged( const QString& ) ) );

  connect( mFontFamilyRadioCustom, SIGNAL( toggled( bool ) ), mFontFamilyComboBox, SLOT( setEnabled( bool ) ) );

  connect( cmbIconSize, SIGNAL( activated( const QString& ) ), this, SLOT( iconSizeChanged( const QString& ) ) );
  connect( cmbIconSize, SIGNAL( highlighted( const QString& ) ), this, SLOT( iconSizeChanged( const QString& ) ) );
  connect( cmbIconSize, SIGNAL( textChanged( const QString& ) ), this, SLOT( iconSizeChanged( const QString& ) ) );

  connect( this, SIGNAL( accepted() ), this, SLOT( saveOptions() ) );
  connect( this, SIGNAL( rejected() ), this, SLOT( rejectOptions() ) );

  QStringList styles = QStyleFactory::keys();
  foreach ( QString style, styles )
  {
    cmbStyle->addItem( style );
  }

  mIdentifyHighlightColorButton->setColorDialogTitle( tr( "Identify highlight color" ) );
  mIdentifyHighlightColorButton->setAllowAlpha( true );
  mIdentifyHighlightColorButton->setContext( "gui" );
  mIdentifyHighlightColorButton->setDefaultColor( QGis::DEFAULT_HIGHLIGHT_COLOR );

  QSettings settings;

  double identifyValue = settings.value( "/Map/searchRadiusMM", QGis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
  QgsDebugMsg( QString( "Standard Identify radius setting read from settings file: %1" ).arg( identifyValue ) );
  if ( identifyValue <= 0.0 )
    identifyValue = QGis::DEFAULT_SEARCH_RADIUS_MM;
  spinBoxIdentifyValue->setMinimum( 0.0 );
  spinBoxIdentifyValue->setValue( identifyValue );
  QColor highlightColor = QColor( settings.value( "/Map/highlight/color", QGis::DEFAULT_HIGHLIGHT_COLOR.name() ).toString() );
  int highlightAlpha = settings.value( "/Map/highlight/colorAlpha", QGis::DEFAULT_HIGHLIGHT_COLOR.alpha() ).toInt();
  highlightColor.setAlpha( highlightAlpha );
  mIdentifyHighlightColorButton->setColor( highlightColor );
  double highlightBuffer = settings.value( "/Map/highlight/buffer", QGis::DEFAULT_HIGHLIGHT_BUFFER_MM ).toDouble();
  mIdentifyHighlightBufferSpinBox->setValue( highlightBuffer );
  double highlightMinWidth = settings.value( "/Map/highlight/minWidth", QGis::DEFAULT_HIGHLIGHT_MIN_WIDTH_MM ).toDouble();
  mIdentifyHighlightMinWidthSpinBox->setValue( highlightMinWidth );

  // custom environment variables
  bool useCustomVars = settings.value( "qgis/customEnvVarsUse", QVariant( false ) ).toBool();
  mCustomVariablesChkBx->setChecked( useCustomVars );
  if ( !useCustomVars )
  {
    mAddCustomVarBtn->setEnabled( false );
    mRemoveCustomVarBtn->setEnabled( false );
    mCustomVariablesTable->setEnabled( false );
  }
  QStringList customVarsList = settings.value( "qgis/customEnvVars", "" ).toStringList();
  foreach ( const QString &varStr, customVarsList )
  {
    int pos = varStr.indexOf( QLatin1Char( '|' ) );
    if ( pos == -1 )
      continue;
    QString varStrApply = varStr.left( pos );
    QString varStrNameValue = varStr.mid( pos + 1 );
    pos = varStrNameValue.indexOf( QLatin1Char( '=' ) );
    if ( pos == -1 )
      continue;
    QString varStrName = varStrNameValue.left( pos );
    QString varStrValue = varStrNameValue.mid( pos + 1 );

    addCustomEnvVarRow( varStrName, varStrValue, varStrApply );
  }
  QFontMetrics fmCustomVar( mCustomVariablesTable->horizontalHeader()->font() );
  int fmCustomVarH = fmCustomVar.height() + 8;
  mCustomVariablesTable->horizontalHeader()->setFixedHeight( fmCustomVarH );

  mCustomVariablesTable->setColumnWidth( 0, 120 );
  if ( mCustomVariablesTable->rowCount() > 0 )
  {
    mCustomVariablesTable->resizeColumnToContents( 1 );
  }
  else
  {
    mCustomVariablesTable->setColumnWidth( 1, 120 );
  }

  // current environment variables
  mCurrentVariablesTable->horizontalHeader()->setFixedHeight( fmCustomVarH );
  QMap<QString, QString> sysVarsMap = QgsApplication::systemEnvVars();
  QStringList currentVarsList = QProcess::systemEnvironment();

  foreach ( const QString &varStr, currentVarsList )
  {
    int pos = varStr.indexOf( QLatin1Char( '=' ) );
    if ( pos == -1 )
      continue;
    QStringList varStrItms;
    QString varStrName = varStr.left( pos );
    QString varStrValue = varStr.mid( pos + 1 );
    varStrItms << varStrName << varStrValue;

    // check if different than system variable
    QString sysVarVal = QString( "" );
    bool sysVarMissing = !sysVarsMap.contains( varStrName );
    if ( sysVarMissing )
      sysVarVal = tr( "not present" );

    if ( !sysVarMissing && sysVarsMap.value( varStrName ) != varStrValue )
      sysVarVal = sysVarsMap.value( varStrName );

    if ( !sysVarVal.isEmpty() )
      sysVarVal = tr( "System value: %1" ).arg( sysVarVal );

    int rowCnt = mCurrentVariablesTable->rowCount();
    mCurrentVariablesTable->insertRow( rowCnt );

    QFont fItm;
    for ( int i = 0; i < varStrItms.size(); ++i )
    {
      QTableWidgetItem* varNameItm = new QTableWidgetItem( varStrItms.at( i ) );
      varNameItm->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable
                            | Qt::ItemIsEditable | Qt::ItemIsDragEnabled );
      fItm = varNameItm->font();
      if ( !sysVarVal.isEmpty() )
      {
        fItm.setBold( true );
        varNameItm->setFont( fItm );
        varNameItm->setToolTip( sysVarVal );
      }
      mCurrentVariablesTable->setItem( rowCnt, i, varNameItm );
    }
    fItm.setBold( true );
    QFontMetrics fmRow( fItm );
    mCurrentVariablesTable->setRowHeight( rowCnt, fmRow.height() + 6 );
  }
  if ( mCurrentVariablesTable->rowCount() > 0 )
    mCurrentVariablesTable->resizeColumnToContents( 0 );

  //local directories to search when loading c++ plugins
  QString myPaths = settings.value( "plugins/searchPathsForPlugins", "" ).toString();
  if ( !myPaths.isEmpty() )
  {
    QStringList myPathList = myPaths.split( "|" );
    QStringList::const_iterator pathIt = myPathList.constBegin();
    for ( ; pathIt != myPathList.constEnd(); ++pathIt )
    {
      QListWidgetItem* newItem = new QListWidgetItem( mListPluginPaths );
      newItem->setText( *pathIt );
      newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
      mListPluginPaths->addItem( newItem );
    }
  }

  //local directories to search when looking for an SVG with a given basename
  myPaths = settings.value( "svg/searchPathsForSVG", "" ).toString();
  if ( !myPaths.isEmpty() )
  {
    QStringList myPathList = myPaths.split( "|" );
    QStringList::const_iterator pathIt = myPathList.constBegin();
    for ( ; pathIt != myPathList.constEnd(); ++pathIt )
    {
      QListWidgetItem* newItem = new QListWidgetItem( mListSVGPaths );
      newItem->setText( *pathIt );
      newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
      mListSVGPaths->addItem( newItem );
    }
  }

  //Network timeout
  mNetworkTimeoutSpinBox->setValue( settings.value( "/qgis/networkAndProxy/networkTimeout", "60000" ).toInt() );
  leUserAgent->setText( settings.value( "/qgis/networkAndProxy/userAgent", "Mozilla/5.0" ).toString() );

  // WMS/WMS-C tile expiry time
  mDefaultTileExpirySpinBox->setValue( settings.value( "/qgis/defaultTileExpiry", "24" ).toInt() );

  // WMS/WMS-C default max retry in case of tile request errors
  mDefaultTileMaxRetrySpinBox->setValue( settings.value( "/qgis/defaultTileMaxRetry", "3" ).toInt() );

  //Web proxy settings
  grpProxy->setChecked( settings.value( "proxy/proxyEnabled", "0" ).toBool() );
  leProxyHost->setText( settings.value( "proxy/proxyHost", "" ).toString() );
  leProxyPort->setText( settings.value( "proxy/proxyPort", "" ).toString() );
  leProxyUser->setText( settings.value( "proxy/proxyUser", "" ).toString() );
  leProxyPassword->setText( settings.value( "proxy/proxyPassword", "" ).toString() );

  //available proxy types
  mProxyTypeComboBox->insertItem( 0, "DefaultProxy" );
  mProxyTypeComboBox->insertItem( 1, "Socks5Proxy" );
  mProxyTypeComboBox->insertItem( 2, "HttpProxy" );
  mProxyTypeComboBox->insertItem( 3, "HttpCachingProxy" );
  mProxyTypeComboBox->insertItem( 4, "FtpCachingProxy" );
  QString settingProxyType = settings.value( "proxy/proxyType", "DefaultProxy" ).toString();
  mProxyTypeComboBox->setCurrentIndex( mProxyTypeComboBox->findText( settingProxyType ) );

  //URLs excluded not going through proxies
  QString proxyExcludedURLs = settings.value( "proxy/proxyExcludedUrls", "" ).toString();
  if ( !proxyExcludedURLs.isEmpty() )
  {
    QStringList splittedUrls = proxyExcludedURLs.split( "|" );
    QStringList::const_iterator urlIt = splittedUrls.constBegin();
    for ( ; urlIt != splittedUrls.constEnd(); ++urlIt )
    {
      QListWidgetItem* newItem = new QListWidgetItem( mExcludeUrlListWidget );
      newItem->setText( *urlIt );
      newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
      mExcludeUrlListWidget->addItem( newItem );
    }
  }

  // cache settings
  QNetworkDiskCache *cache = qobject_cast<QNetworkDiskCache*>( QgsNetworkAccessManager::instance()->cache() );
  if ( cache )
  {
    mCacheDirectory->setText( cache->cacheDirectory() );
    mCacheSize->setMinimum( 0 );
    mCacheSize->setMaximum( std::numeric_limits<int>::max() );
    mCacheSize->setSingleStep( 1024 );
    QgsDebugMsg( QString( "set cacheSize: %1" ).arg( cache->maximumCacheSize() ) );
    mCacheSize->setValue( cache->maximumCacheSize() / 1024 );
  }

  //wms search server
  leWmsSearch->setText( settings.value( "/qgis/WMSSearchUrl", "http://geopole.org/wms/search?search=%1&type=rss" ).toString() );

  // set the current theme
  cmbTheme->setItemText( cmbTheme->currentIndex(), settings.value( "/Themes" ).toString() );

  // set the attribute table default filter
  cmbAttrTableBehaviour->clear();
  cmbAttrTableBehaviour->addItem( tr( "Show all features" ), QgsAttributeTableFilterModel::ShowAll );
  cmbAttrTableBehaviour->addItem( tr( "Show selected features" ), QgsAttributeTableFilterModel::ShowSelected );
  cmbAttrTableBehaviour->addItem( tr( "Show features visible on map" ), QgsAttributeTableFilterModel::ShowVisible );
  cmbAttrTableBehaviour->setCurrentIndex( cmbAttrTableBehaviour->findData( settings.value( "/qgis/attributeTableBehaviour", QgsAttributeTableFilterModel::ShowAll ).toInt() ) );


  spinBoxAttrTableRowCache->setValue( settings.value( "/qgis/attributeTableRowCache", 10000 ).toInt() );
  spinBoxAttrTableRowCache->setSpecialValueText( tr( "All" ) );

  // set the prompt for raster sublayers
  // 0 = Always -> always ask (if there are existing sublayers)
  // 1 = If needed -> ask if layer has no bands, but has sublayers
  // 2 = Never -> never prompt, will not load anything
  // 3 = Load all -> never prompt, but load all sublayers
  cmbPromptRasterSublayers->clear();
  cmbPromptRasterSublayers->addItem( tr( "Always" ) );
  cmbPromptRasterSublayers->addItem( tr( "If needed" ) ); //this means, prompt if there are sublayers but no band in the main dataset
  cmbPromptRasterSublayers->addItem( tr( "Never" ) );
  cmbPromptRasterSublayers->addItem( tr( "Load all" ) );
  cmbPromptRasterSublayers->setCurrentIndex( settings.value( "/qgis/promptForRasterSublayers", 0 ).toInt() );

  // Scan for valid items in the browser dock
  cmbScanItemsInBrowser->clear();
  cmbScanItemsInBrowser->addItem( tr( "Check file contents" ), "contents" ); // 0
  cmbScanItemsInBrowser->addItem( tr( "Check extension" ), "extension" );    // 1
  int index = cmbScanItemsInBrowser->findData( settings.value( "/qgis/scanItemsInBrowser2", "" ) );
  if ( index == -1 ) index = 1;
  cmbScanItemsInBrowser->setCurrentIndex( index );

  // Scan for contents of compressed files (.zip) in browser dock
  cmbScanZipInBrowser->clear();
  cmbScanZipInBrowser->addItem( tr( "No" ), QVariant( "no" ) );
  // cmbScanZipInBrowser->addItem( tr( "Passthru" ) );     // 1 - removed
  cmbScanZipInBrowser->addItem( tr( "Basic scan" ), QVariant( "basic" ) );
  cmbScanZipInBrowser->addItem( tr( "Full scan" ), QVariant( "full" ) );
  index = cmbScanZipInBrowser->findData( settings.value( "/qgis/scanZipInBrowser2", "" ) );
  if ( index == -1 ) index = 1;
  cmbScanZipInBrowser->setCurrentIndex( index );

  // log rendering events, for userspace debugging
  mLogCanvasRefreshChkBx->setChecked( settings.value( "/Map/logCanvasRefreshEvent", false ).toBool() );

  //set the default projection behaviour radio buttongs
  if ( settings.value( "/Projections/defaultBehaviour", "prompt" ).toString() == "prompt" )
  {
    radPromptForProjection->setChecked( true );
  }
  else if ( settings.value( "/Projections/defaultBehaviour", "prompt" ).toString() == "useProject" )
  {
    radUseProjectProjection->setChecked( true );
  }
  else //useGlobal
  {
    radUseGlobalProjection->setChecked( true );
  }
  QString myLayerDefaultCrs = settings.value( "/Projections/layerDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
  mLayerDefaultCrs.createFromOgcWmsCrs( myLayerDefaultCrs );
  leLayerGlobalCrs->setCrs( mLayerDefaultCrs );

  //on the fly CRS transformation settings
  //it would be logical to have single settings value but originaly the radio buttons were checkboxes
  if ( settings.value( "/Projections/otfTransformAutoEnable", true ).toBool() )
  {
    radOtfAuto->setChecked( true );
  }
  else if ( settings.value( "/Projections/otfTransformEnabled", false ).toBool() )
  {
    radOtfTransform->setChecked( true );
  }
  else
  {
    radOtfNone->setChecked( true ); // default
  }

  QString myDefaultCrs = settings.value( "/Projections/projectDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString();
  mDefaultCrs.createFromOgcWmsCrs( myDefaultCrs );
  leProjectGlobalCrs->setCrs( mDefaultCrs );
  leProjectGlobalCrs->setOptionVisible( QgsProjectionSelectionWidget::DefaultCrs, false );

  //default datum transformations
  settings.beginGroup( "/Projections" );

  chkShowDatumTransformDialog->setChecked( settings.value( "showDatumTransformDialog", false ).toBool() );

  QStringList projectionKeys = settings.allKeys();

  //collect src and dest entries that belong together
  QMap< QPair< QString, QString >, QPair< int, int > > transforms;
  QStringList::const_iterator pkeyIt = projectionKeys.constBegin();
  for ( ; pkeyIt != projectionKeys.constEnd(); ++pkeyIt )
  {
    if ( pkeyIt->contains( "srcTransform" ) || pkeyIt->contains( "destTransform" ) )
    {
      QStringList split = pkeyIt->split( "/" );
      QString srcAuthId, destAuthId;
      if ( split.size() > 0 )
      {
        srcAuthId = split.at( 0 );
      }
      if ( split.size() > 1 )
      {
        destAuthId = split.at( 1 ).split( "_" ).at( 0 );
      }

      if ( pkeyIt->contains( "srcTransform" ) )
      {
        transforms[ qMakePair( srcAuthId, destAuthId )].first = settings.value( *pkeyIt ).toInt();
      }
      else if ( pkeyIt->contains( "destTransform" ) )
      {
        transforms[ qMakePair( srcAuthId, destAuthId )].second = settings.value( *pkeyIt ).toInt();
      }
    }
  }
  settings.endGroup();

  QMap< QPair< QString, QString >, QPair< int, int > >::const_iterator transformIt = transforms.constBegin();
  for ( ; transformIt != transforms.constEnd(); ++transformIt )
  {
    const QPair< int, int >& v = transformIt.value();
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText( 0, transformIt.key().first );
    item->setText( 1, transformIt.key().second );
    item->setText( 2, QString::number( v.first ) );
    item->setText( 3, QString::number( v.second ) );
    mDefaultDatumTransformTreeWidget->addTopLevelItem( item );
  }

  // Set the units for measuring
  QGis::UnitType myDisplayUnits = QGis::fromLiteral( settings.value( "/qgis/measure/displayunits", QGis::toLiteral( QGis::Meters ) ).toString() );
  if ( myDisplayUnits == QGis::Feet )
  {
    radFeet->setChecked( true );
  }
  else if ( myDisplayUnits == QGis::NauticalMiles )
  {
    radNautical->setChecked( true );
  }
  else if ( myDisplayUnits == QGis::Degrees )
  {
    radDegrees->setChecked( true );
  }
  else
  {
    radMeters->setChecked( true );
  }

  QButtonGroup* angleButtonGroup = new QButtonGroup( this );
  angleButtonGroup->addButton( mDegreesRadioButton );
  angleButtonGroup->addButton( mRadiansRadioButton );
  angleButtonGroup->addButton( mGonRadioButton );

  QString myAngleUnitsTxt = settings.value( "/qgis/measure/angleunits", "degrees" ).toString();
  if ( myAngleUnitsTxt == "gon" )
  {
    mGonRadioButton->setChecked( true );
  }
  else if ( myAngleUnitsTxt == "radians" )
  {
    mRadiansRadioButton->setChecked( true );
  }
  else //degrees
  {
    mDegreesRadioButton->setChecked( true );
  }

  // set decimal places of the measure tool
  int decimalPlaces = settings.value( "/qgis/measure/decimalplaces", "3" ).toInt();
  mDecimalPlacesSpinBox->setRange( 0, 12 );
  mDecimalPlacesSpinBox->setValue( decimalPlaces );

  // set if base unit of measure tool should be changed
  bool baseUnit = settings.value( "qgis/measure/keepbaseunit", false ).toBool();
  if ( baseUnit )
  {
    mKeepBaseUnitCheckBox->setChecked( true );
  }
  else
  {
    mKeepBaseUnitCheckBox->setChecked( false );
  }


  // add the themes to the combo box on the option dialog
  QDir myThemeDir( ":/images/themes/" );
  myThemeDir.setFilter( QDir::Dirs );
  QStringList myDirList = myThemeDir.entryList( QStringList( "*" ) );
  cmbTheme->clear();
  for ( int i = 0; i < myDirList.count(); i++ )
  {
    if ( myDirList[i] != "." && myDirList[i] != ".." )
    {
      cmbTheme->addItem( myDirList[i] );
    }
  }

  // set the theme combo
  cmbTheme->setCurrentIndex( cmbTheme->findText( settings.value( "/Themes", "default" ).toString() ) );
  cmbIconSize->setCurrentIndex( cmbIconSize->findText( settings.value( "/IconSize", QGIS_ICON_SIZE ).toString() ) );

  // set font size and family
  spinFontSize->blockSignals( true );
  mFontFamilyRadioQt->blockSignals( true );
  mFontFamilyRadioCustom->blockSignals( true );
  mFontFamilyComboBox->blockSignals( true );

  spinFontSize->setValue( mStyleSheetOldOpts.value( "fontPointSize" ).toInt() );
  QString fontFamily = mStyleSheetOldOpts.value( "fontFamily" ).toString();
  bool isQtDefault = ( fontFamily == mStyleSheetBuilder->defaultFont().family() );
  mFontFamilyRadioQt->setChecked( isQtDefault );
  mFontFamilyRadioCustom->setChecked( !isQtDefault );
  mFontFamilyComboBox->setEnabled( !isQtDefault );
  if ( !isQtDefault )
  {
    QFont *tempFont = new QFont( fontFamily );
    // is exact family match returned from system?
    if ( tempFont->family() == fontFamily )
    {
      mFontFamilyComboBox->setCurrentFont( *tempFont );
    }
    delete tempFont;
  }

  spinFontSize->blockSignals( false );
  mFontFamilyRadioQt->blockSignals( false );
  mFontFamilyRadioCustom->blockSignals( false );
  mFontFamilyComboBox->blockSignals( false );

  // custom group boxes
  mCustomGroupBoxChkBx->setChecked( mStyleSheetOldOpts.value( "groupBoxCustom" ).toBool() );
  mCustomSideBarSide->setChecked( mStyleSheetOldOpts.value( "sidebarStyle" ).toBool() );
  mBoldGroupBoxTitleChkBx->setChecked( mStyleSheetOldOpts.value( "groupBoxBoldTitle" ).toBool() );

  mMessageTimeoutSpnBx->setValue( settings.value( "/qgis/messageTimeout", 5 ).toInt() );

  QString name = QApplication::style()->objectName();
  cmbStyle->setCurrentIndex( cmbStyle->findText( name, Qt::MatchFixedString ) );

  mNativeColorDialogsChkBx->setChecked( settings.value( "/qgis/native_color_dialogs", false ).toBool() );
  mLiveColorDialogsChkBx->setChecked( settings.value( "/qgis/live_color_dialogs", false ).toBool() );

  //set the state of the checkboxes
  //Changed to default to true as of QGIS 1.7
  chkAntiAliasing->setChecked( settings.value( "/qgis/enable_anti_aliasing", true ).toBool() );
  chkUseRenderCaching->setChecked( settings.value( "/qgis/enable_render_caching", true ).toBool() );
  chkParallelRendering->setChecked( settings.value( "/qgis/parallel_rendering", false ).toBool() );
  spinMapUpdateInterval->setValue( settings.value( "/qgis/map_update_interval", 250 ).toInt() );
  chkMaxThreads->setChecked( QgsApplication::maxThreads() != -1 );
  spinMaxThreads->setEnabled( chkMaxThreads->isChecked() );
  spinMaxThreads->setRange( 1, QThread::idealThreadCount() );
  spinMaxThreads->setValue( QgsApplication::maxThreads() );

  // Default simplify drawing configuration
  mSimplifyDrawingGroupBox->setChecked( settings.value( "/qgis/simplifyDrawingHints", ( int )QgsVectorSimplifyMethod::GeometrySimplification ).toInt() != QgsVectorSimplifyMethod::NoSimplification );
  mSimplifyDrawingSpinBox->setValue( settings.value( "/qgis/simplifyDrawingTol", QGis::DEFAULT_MAPTOPIXEL_THRESHOLD ).toFloat() );
  mSimplifyDrawingAtProvider->setChecked( !settings.value( "/qgis/simplifyLocal", true ).toBool() );

  QStringList myScalesList = PROJECT_SCALES.split( "," );
  myScalesList.append( "1:1" );
  mSimplifyMaximumScaleComboBox->updateScales( myScalesList );
  mSimplifyMaximumScaleComboBox->setScale( 1.0 / settings.value( "/qgis/simplifyMaxScale", 1 ).toFloat() );

  // Slightly awkard here at the settings value is true to use QImage,
  // but the checkbox is true to use QPixmap
  chkAddedVisibility->setChecked( settings.value( "/qgis/new_layers_visible", true ).toBool() );
  cbxLegendClassifiers->setChecked( settings.value( "/qgis/showLegendClassifiers", false ).toBool() );
  mLegendLayersBoldChkBx->setChecked( settings.value( "/qgis/legendLayersBold", true ).toBool() );
  mLegendGroupsBoldChkBx->setChecked( settings.value( "/qgis/legendGroupsBold", false ).toBool() );
  cbxHideSplash->setChecked( settings.value( "/qgis/hideSplash", false ).toBool() );
  cbxShowTips->setChecked( settings.value( "/qgis/showTips", true ).toBool() );
  cbxAttributeTableDocked->setChecked( settings.value( "/qgis/dockAttributeTable", false ).toBool() );
  cbxSnappingOptionsDocked->setChecked( settings.value( "/qgis/dockSnapping", false ).toBool() );
  cbxAddPostgisDC->setChecked( settings.value( "/qgis/addPostgisDC", false ).toBool() );
  cbxAddOracleDC->setChecked( settings.value( "/qgis/addOracleDC", false ).toBool() );
  cbxCreateRasterLegendIcons->setChecked( settings.value( "/qgis/createRasterLegendIcons", false ).toBool() );
  cbxCopyWKTGeomFromTable->setChecked( settings.value( "/qgis/copyGeometryAsWKT", true ).toBool() );
  leNullValue->setText( settings.value( "qgis/nullValue", "NULL" ).toString() );
  cbxIgnoreShapeEncoding->setChecked( settings.value( "/qgis/ignoreShapeEncoding", true ).toBool() );
  cbxCanvasRotation->setChecked( QgsMapCanvas::rotationEnabled() );

  cmbLegendDoubleClickAction->setCurrentIndex( settings.value( "/qgis/legendDoubleClickAction", 0 ).toInt() );

  // WMS getLegendGraphic setting
  mLegendGraphicResolutionSpinBox->setValue( settings.value( "/qgis/defaultLegendGraphicResolution", 0 ).toInt() );

  //
  // Raster properties
  //
  spnRed->setValue( settings.value( "/Raster/defaultRedBand", 1 ).toInt() );
  spnGreen->setValue( settings.value( "/Raster/defaultGreenBand", 2 ).toInt() );
  spnBlue->setValue( settings.value( "/Raster/defaultBlueBand", 3 ).toInt() );

  initContrastEnhancement( cboxContrastEnhancementAlgorithmSingleBand, "singleBand", "StretchToMinimumMaximum" );
  initContrastEnhancement( cboxContrastEnhancementAlgorithmMultiBandSingleByte, "multiBandSingleByte", "NoEnhancement" );
  initContrastEnhancement( cboxContrastEnhancementAlgorithmMultiBandMultiByte, "multiBandMultiByte", "StretchToMinimumMaximum" );

  QString cumulativeCutText = tr( "Cumulative pixel count cut" );
  cboxContrastEnhancementLimits->addItem( tr( "Cumulative pixel count cut" ), "CumulativeCut" );
  cboxContrastEnhancementLimits->addItem( tr( "Minimum / maximum" ), "MinMax" );
  cboxContrastEnhancementLimits->addItem( tr( "Mean +/- standard deviation" ), "StdDev" );

  QString contrastEnchacementLimits = settings.value( "/Raster/defaultContrastEnhancementLimits", "CumulativeCut" ).toString();

  cboxContrastEnhancementLimits->setCurrentIndex( cboxContrastEnhancementLimits->findData( contrastEnchacementLimits ) );

  spnThreeBandStdDev->setValue( settings.value( "/Raster/defaultStandardDeviation", 2.0 ).toDouble() );

  mRasterCumulativeCutLowerDoubleSpinBox->setValue( 100.0 * settings.value( "/Raster/cumulativeCutLower", QString::number( QgsRasterLayer::CUMULATIVE_CUT_LOWER ) ).toDouble() );
  mRasterCumulativeCutUpperDoubleSpinBox->setValue( 100.0 * settings.value( "/Raster/cumulativeCutUpper", QString::number( QgsRasterLayer::CUMULATIVE_CUT_UPPER ) ).toDouble() );

  //set the color for selections
  int myRed = settings.value( "/qgis/default_selection_color_red", 255 ).toInt();
  int myGreen = settings.value( "/qgis/default_selection_color_green", 255 ).toInt();
  int myBlue = settings.value( "/qgis/default_selection_color_blue", 0 ).toInt();
  int myAlpha = settings.value( "/qgis/default_selection_color_alpha", 255 ).toInt();
  pbnSelectionColor->setColor( QColor( myRed, myGreen, myBlue, myAlpha ) );
  pbnSelectionColor->setColorDialogTitle( tr( "Set selection color" ) );
  pbnSelectionColor->setAllowAlpha( true );
  pbnSelectionColor->setContext( "gui" );
  pbnSelectionColor->setDefaultColor( QColor( 255, 255, 0, 255 ) );

  //set the default color for canvas background
  myRed = settings.value( "/qgis/default_canvas_color_red", 255 ).toInt();
  myGreen = settings.value( "/qgis/default_canvas_color_green", 255 ).toInt();
  myBlue = settings.value( "/qgis/default_canvas_color_blue", 255 ).toInt();
  pbnCanvasColor->setColor( QColor( myRed, myGreen, myBlue ) );
  pbnCanvasColor->setColorDialogTitle( tr( "Set canvas color" ) );
  pbnCanvasColor->setContext( "gui" );
  pbnCanvasColor->setDefaultColor( Qt::white );

  // set the default color for the measure tool
  myRed = settings.value( "/qgis/default_measure_color_red", 222 ).toInt();
  myGreen = settings.value( "/qgis/default_measure_color_green", 155 ).toInt();
  myBlue = settings.value( "/qgis/default_measure_color_blue", 67 ).toInt();
  pbnMeasureColor->setColor( QColor( myRed, myGreen, myBlue ) );
  pbnMeasureColor->setColorDialogTitle( tr( "Set measuring tool color" ) );
  pbnMeasureColor->setContext( "gui" );
  pbnMeasureColor->setDefaultColor( QColor( 222, 155, 67 ) );

  capitaliseCheckBox->setChecked( settings.value( "/qgis/capitaliseLayerName", QVariant( false ) ).toBool() );

  int projOpen = settings.value( "/qgis/projOpenAtLaunch", 0 ).toInt();
  mProjectOnLaunchCmbBx->setCurrentIndex( projOpen );
  mProjectOnLaunchLineEdit->setText( settings.value( "/qgis/projOpenAtLaunchPath" ).toString() );
  mProjectOnLaunchLineEdit->setEnabled( projOpen == 2 );
  mProjectOnLaunchPushBtn->setEnabled( projOpen == 2 );

  chbAskToSaveProjectChanges->setChecked( settings.value( "qgis/askToSaveProjectChanges", QVariant( true ) ).toBool() );
  mLayerDeleteConfirmationChkBx->setChecked( settings.value( "qgis/askToDeleteLayers", true ).toBool() );
  chbWarnOldProjectVersion->setChecked( settings.value( "/qgis/warnOldProjectVersion", QVariant( true ) ).toBool() );
  cmbEnableMacros->setCurrentIndex( settings.value( "/qgis/enableMacros", 1 ).toInt() );

  // templates
  cbxProjectDefaultNew->setChecked( settings.value( "/qgis/newProjectDefault", QVariant( false ) ).toBool() );
  QString templateDirName = settings.value( "/qgis/projectTemplateDir",
                            QgsApplication::qgisSettingsDirPath() + "project_templates" ).toString();
  // make dir if it doesn't exists - should just be called once
  QDir templateDir;
  if ( ! templateDir.exists( templateDirName ) )
  {
    templateDir.mkdir( templateDirName );
  }
  leTemplateFolder->setText( templateDirName );

  cmbWheelAction->setCurrentIndex( settings.value( "/qgis/wheel_action", 2 ).toInt() );
  spinZoomFactor->setValue( settings.value( "/qgis/zoom_factor", 2 ).toDouble() );

  // predefined scales for scale combobox
  myPaths = settings.value( "Map/scales", PROJECT_SCALES ).toString();
  if ( !myPaths.isEmpty() )
  {
    QStringList myScalesList = myPaths.split( "," );
    QStringList::const_iterator scaleIt = myScalesList.constBegin();
    for ( ; scaleIt != myScalesList.constEnd(); ++scaleIt )
    {
      QListWidgetItem* newItem = new QListWidgetItem( mListGlobalScales );
      newItem->setText( *scaleIt );
      newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
      mListGlobalScales->addItem( newItem );
    }
  }

  //
  // Color palette
  //
  connect( mButtonCopyColors, SIGNAL( clicked() ), mTreeCustomColors, SLOT( copyColors() ) );
  connect( mButtonRemoveColor, SIGNAL( clicked() ), mTreeCustomColors, SLOT( removeSelection() ) );
  connect( mButtonPasteColors, SIGNAL( clicked() ), mTreeCustomColors, SLOT( pasteColors() ) );

  //find custom color scheme from registry
  QList<QgsCustomColorScheme *> customSchemes;
  QgsColorSchemeRegistry::instance()->schemes( customSchemes );
  if ( customSchemes.length() > 0 )
  {
    mTreeCustomColors->setScheme( customSchemes.at( 0 ) );
  }

  //
  // Composer settings
  //

  //default composer font
  mComposerFontComboBox->blockSignals( true );

  QString composerFontFamily = settings.value( "/Composer/defaultFont" ).toString();

  QFont *tempComposerFont = new QFont( composerFontFamily );
  // is exact family match returned from system?
  if ( tempComposerFont->family() == composerFontFamily )
  {
    mComposerFontComboBox->setCurrentFont( *tempComposerFont );
  }
  delete tempComposerFont;

  mComposerFontComboBox->blockSignals( false );

  //default composer grid color
  int gridRed, gridGreen, gridBlue, gridAlpha;
  gridRed = settings.value( "/Composer/gridRed", 190 ).toInt();
  gridGreen = settings.value( "/Composer/gridGreen", 190 ).toInt();
  gridBlue = settings.value( "/Composer/gridBlue", 190 ).toInt();
  gridAlpha = settings.value( "/Composer/gridAlpha", 100 ).toInt();
  QColor gridColor = QColor( gridRed, gridGreen, gridBlue, gridAlpha );
  mGridColorButton->setColor( gridColor );
  mGridColorButton->setColorDialogTitle( tr( "Select grid color" ) );
  mGridColorButton->setAllowAlpha( true );
  mGridColorButton->setContext( "gui" );
  mGridColorButton->setDefaultColor( QColor( 190, 190, 190, 100 ) );

  //default composer grid style
  QString gridStyleString;
  gridStyleString = settings.value( "/Composer/gridStyle", "Dots" ).toString();
  mGridStyleComboBox->insertItem( 0, tr( "Solid" ) );
  mGridStyleComboBox->insertItem( 1, tr( "Dots" ) );
  mGridStyleComboBox->insertItem( 2, tr( "Crosses" ) );
  if ( gridStyleString == "Solid" )
  {
    mGridStyleComboBox->setCurrentIndex( 0 );
  }
  else if ( gridStyleString == "Crosses" )
  {
    mGridStyleComboBox->setCurrentIndex( 2 );
  }
  else
  {
    //default grid is dots
    mGridStyleComboBox->setCurrentIndex( 1 );
  }

  //grid and guide defaults
  mGridResolutionSpinBox->setValue( settings.value( "/Composer/defaultSnapGridResolution", 10.0 ).toDouble() );
  mSnapToleranceSpinBox->setValue( settings.value( "/Composer/defaultSnapTolerancePixels", 5 ).toInt() );
  mOffsetXSpinBox->setValue( settings.value( "/Composer/defaultSnapGridOffsetX", 0 ).toDouble() );
  mOffsetYSpinBox->setValue( settings.value( "/Composer/defaultSnapGridOffsetY", 0 ).toDouble() );

  //
  // Locale settings
  //
  QString mySystemLocale = QLocale::system().name();
  lblSystemLocale->setText( tr( "Detected active locale on your system: %1" ).arg( mySystemLocale ) );
  QString myUserLocale = settings.value( "locale/userLocale", "" ).toString();
  QStringList myI18nList = i18nList();
  foreach ( QString l, myI18nList )
  {
#if QT_VERSION >= 0x040800
    cboLocale->addItem( QIcon( QString( ":/images/flags/%1.png" ).arg( l ) ), QLocale( l ).nativeLanguageName(), l );
#else
    cboLocale->addItem( QIcon( QString( ":/images/flags/%1.png" ).arg( l ) ), l, l );
#endif
  }
  cboLocale->setCurrentIndex( cboLocale->findData( myUserLocale ) );
  bool myLocaleOverrideFlag = settings.value( "locale/overrideFlag", false ).toBool();
  grpLocale->setChecked( myLocaleOverrideFlag );

  //set elements in digitizing tab
  mLineWidthSpinBox->setValue( settings.value( "/qgis/digitizing/line_width", 1 ).toInt() );
  QColor digitizingColor;
  myRed = settings.value( "/qgis/digitizing/line_color_red", 255 ).toInt();
  myGreen = settings.value( "/qgis/digitizing/line_color_green", 0 ).toInt();
  myBlue = settings.value( "/qgis/digitizing/line_color_blue", 0 ).toInt();
  myAlpha = settings.value( "/qgis/digitizing/line_color_alpha", 200 ).toInt();
  mLineColorToolButton->setColor( QColor( myRed, myGreen, myBlue, myAlpha ) );
  mLineColorToolButton->setAllowAlpha( true );
  mLineColorToolButton->setContext( "gui" );
  mLineColorToolButton->setDefaultColor( QColor( 255, 0, 0, 200 ) );

  //default snap mode
  mDefaultSnapModeComboBox->insertItem( 0, tr( "To vertex" ), "to vertex" );
  mDefaultSnapModeComboBox->insertItem( 1, tr( "To segment" ), "to segment" );
  mDefaultSnapModeComboBox->insertItem( 2, tr( "To vertex and segment" ), "to vertex and segment" );
  mDefaultSnapModeComboBox->insertItem( 3, tr( "Off" ), "off" );
  QString defaultSnapString = settings.value( "/qgis/digitizing/default_snap_mode", "off" ).toString();
  mDefaultSnapModeComboBox->setCurrentIndex( mDefaultSnapModeComboBox->findData( defaultSnapString ) );
  mDefaultSnappingToleranceSpinBox->setValue( settings.value( "/qgis/digitizing/default_snapping_tolerance", 0 ).toDouble() );
  mSearchRadiusVertexEditSpinBox->setValue( settings.value( "/qgis/digitizing/search_radius_vertex_edit", 10 ).toDouble() );
  int defSnapUnits = settings.value( "/qgis/digitizing/default_snapping_tolerance_unit", QgsTolerance::ProjectUnits ).toInt();
  if ( defSnapUnits == QgsTolerance::ProjectUnits || defSnapUnits == QgsTolerance::LayerUnits )
  {
    index = mDefaultSnappingToleranceComboBox->findText( tr( "map units" ) );
  }
  else
  {
    index = mDefaultSnappingToleranceComboBox->findText( tr( "pixels" ) );
  }
  mDefaultSnappingToleranceComboBox->setCurrentIndex( index );
  int defRadiusUnits = settings.value( "/qgis/digitizing/search_radius_vertex_edit_unit", QgsTolerance::Pixels ).toInt();
  if ( defRadiusUnits == QgsTolerance::ProjectUnits || defRadiusUnits == QgsTolerance::LayerUnits )
  {
    index = mSearchRadiusVertexEditComboBox->findText( tr( "map units" ) );
  }
  else
  {
    index = mSearchRadiusVertexEditComboBox->findText( tr( "pixels" ) );
  }
  mSearchRadiusVertexEditComboBox->setCurrentIndex( index );

  //vertex marker
  mMarkersOnlyForSelectedCheckBox->setChecked( settings.value( "/qgis/digitizing/marker_only_for_selected", false ).toBool() );

  mMarkerStyleComboBox->addItem( tr( "Semi transparent circle" ) );
  mMarkerStyleComboBox->addItem( tr( "Cross" ) );
  mMarkerStyleComboBox->addItem( tr( "None" ) );

  mValidateGeometries->clear();
  mValidateGeometries->addItem( tr( "Off" ) );
  mValidateGeometries->addItem( tr( "QGIS" ) );
#if defined(GEOS_VERSION_MAJOR) && defined(GEOS_VERSION_MINOR) && \
    ( (GEOS_VERSION_MAJOR==3 && GEOS_VERSION_MINOR>=3) || GEOS_VERSION_MAJOR>3)
  mValidateGeometries->addItem( tr( "GEOS" ) );
#endif

  QString markerStyle = settings.value( "/qgis/digitizing/marker_style", "Cross" ).toString();
  if ( markerStyle == "SemiTransparentCircle" )
  {
    mMarkerStyleComboBox->setCurrentIndex( mMarkerStyleComboBox->findText( tr( "Semi transparent circle" ) ) );
  }
  else if ( markerStyle == "Cross" )
  {
    mMarkerStyleComboBox->setCurrentIndex( mMarkerStyleComboBox->findText( tr( "Cross" ) ) );
  }
  else if ( markerStyle == "None" )
  {
    mMarkerStyleComboBox->setCurrentIndex( mMarkerStyleComboBox->findText( tr( "None" ) ) );
  }
  mMarkerSizeSpinBox->setValue( settings.value( "/qgis/digitizing/marker_size", 3 ).toInt() );

  chkReuseLastValues->setChecked( settings.value( "/qgis/digitizing/reuseLastValues", false ).toBool() );
  chkDisableAttributeValuesDlg->setChecked( settings.value( "/qgis/digitizing/disable_enter_attribute_values_dialog", false ).toBool() );
  mValidateGeometries->setCurrentIndex( settings.value( "/qgis/digitizing/validate_geometries", 1 ).toInt() );

  mOffsetJoinStyleComboBox->addItem( tr( "Round" ), 0 );
  mOffsetJoinStyleComboBox->addItem( tr( "Mitre" ), 1 );
  mOffsetJoinStyleComboBox->addItem( tr( "Bevel" ), 2 );
  mOffsetJoinStyleComboBox->setCurrentIndex( settings.value( "/qgis/digitizing/offset_join_style", 0 ).toInt() );
  mOffsetQuadSegSpinBox->setValue( settings.value( "/qgis/digitizing/offset_quad_seg", 8 ).toInt() );
  mCurveOffsetMiterLimitComboBox->setValue( settings.value( "/qgis/digitizing/offset_miter_limit", 5.0 ).toDouble() );

  // load gdal driver list only when gdal tab is first opened
  mLoadedGdalDriverList = false;

  // restore window and widget geometry/state
  restoreOptionsBaseUi();
}

//! Destructor
QgsOptions::~QgsOptions()
{
}

void QgsOptions::setCurrentPage( QString pageWidgetName )
{
  //find the page with a matching widget name
  for ( int idx = 0; idx < mOptionsStackedWidget->count(); ++idx )
  {
    QWidget * currentPage = mOptionsStackedWidget->widget( idx );
    if ( currentPage->objectName() == pageWidgetName )
    {
      //found the page, set it as current
      mOptionsStackedWidget->setCurrentIndex( idx );
      return;
    }
  }
}

void QgsOptions::on_mProxyTypeComboBox_currentIndexChanged( int idx )
{
  frameManualProxy->setEnabled( idx != 0 );
}

void QgsOptions::on_cbxProjectDefaultNew_toggled( bool checked )
{
  if ( checked )
  {
    QString fileName = QgsApplication::qgisSettingsDirPath() + QString( "project_default.qgs" );
    if ( ! QFile::exists( fileName ) )
    {
      QMessageBox::information( 0, tr( "Save default project" ), tr( "You must set a default project" ) );
      cbxProjectDefaultNew->setChecked( false );
    }
  }
}

void QgsOptions::on_pbnProjectDefaultSetCurrent_clicked()
{
  QString fileName = QgsApplication::qgisSettingsDirPath() + QString( "project_default.qgs" );
  if ( QgsProject::instance()->write( QFileInfo( fileName ) ) )
  {
    QMessageBox::information( 0, tr( "Save default project" ), tr( "Current project saved as default" ) );
  }
  else
  {
    QMessageBox::critical( 0, tr( "Save default project" ), tr( "Error saving current project as default" ) );
  }
}

void QgsOptions::on_pbnProjectDefaultReset_clicked()
{
  QString fileName = QgsApplication::qgisSettingsDirPath() + QString( "project_default.qgs" );
  if ( QFile::exists( fileName ) )
  {
    QFile::remove( fileName );
  }
  cbxProjectDefaultNew->setChecked( false );
}

void QgsOptions::on_pbnTemplateFolderBrowse_pressed()
{
  QString newDir = QFileDialog::getExistingDirectory( 0, tr( "Choose a directory to store project template files" ),
                   leTemplateFolder->text() );
  if ( ! newDir.isNull() )
  {
    leTemplateFolder->setText( newDir );
  }
}

void QgsOptions::on_pbnTemplateFolderReset_pressed()
{
  leTemplateFolder->setText( QgsApplication::qgisSettingsDirPath() + QString( "project_templates" ) );
}

void QgsOptions::themeChanged( const QString &newThemeName )
{
  // Slot to change the theme as user scrolls through the choices
  QgisApp::instance()->setTheme( newThemeName );
}

void QgsOptions::iconSizeChanged( const QString &iconSize )
{
  QgisApp::instance()->setIconSizes( iconSize.toInt() );
}

void QgsOptions::on_mProjectOnLaunchCmbBx_currentIndexChanged( int indx )
{
  bool specific = ( indx == 2 );
  mProjectOnLaunchLineEdit->setEnabled( specific );
  mProjectOnLaunchPushBtn->setEnabled( specific );
}

void QgsOptions::on_mProjectOnLaunchPushBtn_pressed()
{
  // Retrieve last used project dir from persistent settings
  QSettings settings;
  QString lastUsedDir = settings.value( "/UI/lastProjectDir", "." ).toString();
  QString projPath = QFileDialog::getOpenFileName( this,
                     tr( "Choose project file to open at launch" ),
                     lastUsedDir,
                     tr( "QGIS files" ) + " (*.qgs *.QGS)" );
  if ( !projPath.isNull() )
  {
    mProjectOnLaunchLineEdit->setText( projPath );
  }
}

QString QgsOptions::theme()
{
  // returns the current theme (as selected in the cmbTheme combo box)
  return cmbTheme->currentText();
}

void QgsOptions::saveOptions()
{
  QSettings settings;

  // custom environment variables
  settings.setValue( "qgis/customEnvVarsUse", QVariant( mCustomVariablesChkBx->isChecked() ) );
  QStringList customVars;
  for ( int i = 0; i < mCustomVariablesTable->rowCount(); ++i )
  {
    if ( mCustomVariablesTable->item( i, 1 )->text().isEmpty() )
      continue;
    QComboBox* varApplyCmbBx = qobject_cast<QComboBox*>( mCustomVariablesTable->cellWidget( i, 0 ) );
    QString customVar = varApplyCmbBx->itemData( varApplyCmbBx->currentIndex() ).toString();
    customVar += "|";
    customVar += mCustomVariablesTable->item( i, 1 )->text();
    customVar += "=";
    customVar += mCustomVariablesTable->item( i, 2 )->text();
    customVars << customVar;
  }
  settings.setValue( "qgis/customEnvVars", QVariant( customVars ) );

  //search directories for user plugins
  QString myPaths;
  for ( int i = 0; i < mListPluginPaths->count(); ++i )
  {
    if ( i != 0 )
    {
      myPaths += "|";
    }
    myPaths += mListPluginPaths->item( i )->text();
  }
  settings.setValue( "plugins/searchPathsForPlugins", myPaths );

  //search directories for svgs
  myPaths.clear();
  for ( int i = 0; i < mListSVGPaths->count(); ++i )
  {
    if ( i != 0 )
    {
      myPaths += "|";
    }
    myPaths += mListSVGPaths->item( i )->text();
  }
  settings.setValue( "svg/searchPathsForSVG", myPaths );

  //Network timeout
  settings.setValue( "/qgis/networkAndProxy/networkTimeout", mNetworkTimeoutSpinBox->value() );
  settings.setValue( "/qgis/networkAndProxy/userAgent", leUserAgent->text() );

  // WMS/WMS-C tile expiry time
  settings.setValue( "/qgis/defaultTileExpiry", mDefaultTileExpirySpinBox->value() );

  // WMS/WMS-C default max retry in case of tile request errors
  settings.setValue( "/qgis/defaultTileMaxRetry", mDefaultTileMaxRetrySpinBox->value() );

  //Web proxy settings
  settings.setValue( "proxy/proxyEnabled", grpProxy->isChecked() );
  settings.setValue( "proxy/proxyHost", leProxyHost->text() );
  settings.setValue( "proxy/proxyPort", leProxyPort->text() );
  settings.setValue( "proxy/proxyUser", leProxyUser->text() );
  settings.setValue( "proxy/proxyPassword", leProxyPassword->text() );
  settings.setValue( "proxy/proxyType", mProxyTypeComboBox->currentText() );

  settings.setValue( "cache/directory", mCacheDirectory->text() );
  settings.setValue( "cache/size", QVariant::fromValue( mCacheSize->value()*1024L ) );

  //url to exclude from proxys
  QString proxyExcludeString;
  for ( int i = 0; i < mExcludeUrlListWidget->count(); ++i )
  {
    if ( i != 0 )
    {
      proxyExcludeString += "|";
    }
    proxyExcludeString += mExcludeUrlListWidget->item( i )->text();
  }
  settings.setValue( "proxy/proxyExcludedUrls", proxyExcludeString );

  QgisApp::instance()->namUpdate();

  //wms search url
  settings.setValue( "/qgis/WMSSearchUrl", leWmsSearch->text() );

  //general settings
  settings.setValue( "/Map/searchRadiusMM", spinBoxIdentifyValue->value() );
  settings.setValue( "/Map/highlight/color", mIdentifyHighlightColorButton->color().name() );
  settings.setValue( "/Map/highlight/colorAlpha", mIdentifyHighlightColorButton->color().alpha() );
  settings.setValue( "/Map/highlight/buffer", mIdentifyHighlightBufferSpinBox->value() );
  settings.setValue( "/Map/highlight/minWidth", mIdentifyHighlightMinWidthSpinBox->value() );

  bool showLegendClassifiers = settings.value( "/qgis/showLegendClassifiers", false ).toBool();
  settings.setValue( "/qgis/showLegendClassifiers", cbxLegendClassifiers->isChecked() );
  bool legendLayersBold = settings.value( "/qgis/legendLayersBold", true ).toBool();
  settings.setValue( "/qgis/legendLayersBold", mLegendLayersBoldChkBx->isChecked() );
  bool legendGroupsBold = settings.value( "/qgis/legendGroupsBold", false ).toBool();
  settings.setValue( "/qgis/legendGroupsBold", mLegendGroupsBoldChkBx->isChecked() );
  settings.setValue( "/qgis/hideSplash", cbxHideSplash->isChecked() );
  settings.setValue( "/qgis/showTips", cbxShowTips->isChecked() );
  settings.setValue( "/qgis/dockAttributeTable", cbxAttributeTableDocked->isChecked() );
  settings.setValue( "/qgis/attributeTableBehaviour", cmbAttrTableBehaviour->itemData( cmbAttrTableBehaviour->currentIndex() ) );
  settings.setValue( "/qgis/attributeTableRowCache", spinBoxAttrTableRowCache->value() );
  settings.setValue( "/qgis/promptForRasterSublayers", cmbPromptRasterSublayers->currentIndex() );
  settings.setValue( "/qgis/scanItemsInBrowser2",
                     cmbScanItemsInBrowser->itemData( cmbScanItemsInBrowser->currentIndex() ).toString() );
  settings.setValue( "/qgis/scanZipInBrowser2",
                     cmbScanZipInBrowser->itemData( cmbScanZipInBrowser->currentIndex() ).toString() );
  settings.setValue( "/qgis/ignoreShapeEncoding", cbxIgnoreShapeEncoding->isChecked() );
  settings.setValue( "/qgis/dockSnapping", cbxSnappingOptionsDocked->isChecked() );
  settings.setValue( "/qgis/addPostgisDC", cbxAddPostgisDC->isChecked() );
  settings.setValue( "/qgis/addOracleDC", cbxAddOracleDC->isChecked() );
  settings.setValue( "/qgis/defaultLegendGraphicResolution", mLegendGraphicResolutionSpinBox->value() );
  bool createRasterLegendIcons = settings.value( "/qgis/createRasterLegendIcons", false ).toBool();
  settings.setValue( "/qgis/createRasterLegendIcons", cbxCreateRasterLegendIcons->isChecked() );
  settings.setValue( "/qgis/copyGeometryAsWKT", cbxCopyWKTGeomFromTable->isChecked() );
  settings.setValue( "/qgis/new_layers_visible", chkAddedVisibility->isChecked() );
  settings.setValue( "/qgis/enable_anti_aliasing", chkAntiAliasing->isChecked() );
  settings.setValue( "/qgis/enable_render_caching", chkUseRenderCaching->isChecked() );
  settings.setValue( "/qgis/parallel_rendering", chkParallelRendering->isChecked() );
  int maxThreads = chkMaxThreads->isChecked() ? spinMaxThreads->value() : -1;
  QgsApplication::setMaxThreads( maxThreads );
  settings.setValue( "/qgis/max_threads", maxThreads );

  settings.setValue( "/qgis/map_update_interval", spinMapUpdateInterval->value() );
  settings.setValue( "/qgis/legendDoubleClickAction", cmbLegendDoubleClickAction->currentIndex() );
  bool legendLayersCapitalise = settings.value( "/qgis/capitaliseLayerName", false ).toBool();
  settings.setValue( "/qgis/capitaliseLayerName", capitaliseCheckBox->isChecked() );
  QgsMapCanvas::enableRotation( cbxCanvasRotation->isChecked() );

  // Default simplify drawing configuration
  QgsVectorSimplifyMethod::SimplifyHints simplifyHints = QgsVectorSimplifyMethod::NoSimplification;
  if ( mSimplifyDrawingGroupBox->isChecked() )
  {
    simplifyHints |= QgsVectorSimplifyMethod::GeometrySimplification;
    if ( mSimplifyDrawingSpinBox->value() > 1 ) simplifyHints |= QgsVectorSimplifyMethod::AntialiasingSimplification;
  }
  settings.setValue( "/qgis/simplifyDrawingHints", ( int ) simplifyHints );
  settings.setValue( "/qgis/simplifyDrawingTol", mSimplifyDrawingSpinBox->value() );
  settings.setValue( "/qgis/simplifyLocal", !mSimplifyDrawingAtProvider->isChecked() );
  settings.setValue( "/qgis/simplifyMaxScale", 1.0 / mSimplifyMaximumScaleComboBox->scale() );

  // project
  settings.setValue( "/qgis/projOpenAtLaunch", mProjectOnLaunchCmbBx->currentIndex() );
  settings.setValue( "/qgis/projOpenAtLaunchPath", mProjectOnLaunchLineEdit->text() );

  settings.setValue( "/qgis/askToSaveProjectChanges", chbAskToSaveProjectChanges->isChecked() );
  settings.setValue( "qgis/askToDeleteLayers", mLayerDeleteConfirmationChkBx->isChecked() );
  settings.setValue( "/qgis/warnOldProjectVersion", chbWarnOldProjectVersion->isChecked() );
  if (( settings.value( "/qgis/projectTemplateDir" ).toString() != leTemplateFolder->text() ) ||
      ( settings.value( "/qgis/newProjectDefault" ).toBool() != cbxProjectDefaultNew->isChecked() ) )
  {
    settings.setValue( "/qgis/newProjectDefault", cbxProjectDefaultNew->isChecked() );
    settings.setValue( "/qgis/projectTemplateDir", leTemplateFolder->text() );
    QgisApp::instance()->updateProjectFromTemplates();
  }
  settings.setValue( "/qgis/enableMacros", cmbEnableMacros->currentIndex() );

  settings.setValue( "/qgis/nullValue", leNullValue->text() );
  settings.setValue( "/qgis/style", cmbStyle->currentText() );

  if ( cmbTheme->currentText().length() == 0 )
  {
    settings.setValue( "/Themes", "default" );
  }
  else
  {
    settings.setValue( "/Themes", cmbTheme->currentText() );
  }

  settings.setValue( "/IconSize", cmbIconSize->currentText() );

  settings.setValue( "/qgis/messageTimeout", mMessageTimeoutSpnBx->value() );

  settings.setValue( "/qgis/native_color_dialogs", mNativeColorDialogsChkBx->isChecked() );
  settings.setValue( "/qgis/live_color_dialogs", mLiveColorDialogsChkBx->isChecked() );

  // rasters settings
  settings.setValue( "/Raster/defaultRedBand", spnRed->value() );
  settings.setValue( "/Raster/defaultGreenBand", spnGreen->value() );
  settings.setValue( "/Raster/defaultBlueBand", spnBlue->value() );

  saveContrastEnhancement( cboxContrastEnhancementAlgorithmSingleBand, "singleBand" );
  saveContrastEnhancement( cboxContrastEnhancementAlgorithmMultiBandSingleByte, "multiBandSingleByte" );
  saveContrastEnhancement( cboxContrastEnhancementAlgorithmMultiBandMultiByte, "multiBandMultiByte" );

  QString contrastEnhancementLimits = cboxContrastEnhancementLimits->itemData( cboxContrastEnhancementLimits->currentIndex() ).toString();
  settings.setValue( "/Raster/defaultContrastEnhancementLimits", contrastEnhancementLimits );

  settings.setValue( "/Raster/defaultStandardDeviation", spnThreeBandStdDev->value() );

  settings.setValue( "/Raster/cumulativeCutLower", mRasterCumulativeCutLowerDoubleSpinBox->value() / 100.0 );
  settings.setValue( "/Raster/cumulativeCutUpper", mRasterCumulativeCutUpperDoubleSpinBox->value() / 100.0 );

  // log rendering events, for userspace debugging
  settings.setValue( "/Map/logCanvasRefreshEvent", mLogCanvasRefreshChkBx->isChecked() );

  //check behaviour so default projection when new layer is added with no
  //projection defined...
  if ( radPromptForProjection->isChecked() )
  {
    settings.setValue( "/Projections/defaultBehaviour", "prompt" );
  }
  else if ( radUseProjectProjection->isChecked() )
  {
    settings.setValue( "/Projections/defaultBehaviour", "useProject" );
  }
  else //assumes radUseGlobalProjection is checked
  {
    settings.setValue( "/Projections/defaultBehaviour", "useGlobal" );
  }

  settings.setValue( "/Projections/layerDefaultCrs", mLayerDefaultCrs.authid() );

  // save 'on the fly' CRS transformation settings
  settings.setValue( "/Projections/otfTransformAutoEnable", radOtfAuto->isChecked() );
  settings.setValue( "/Projections/otfTransformEnabled", radOtfTransform->isChecked() );
  settings.setValue( "/Projections/projectDefaultCrs", mDefaultCrs.authid() );

  settings.setValue( "/Projections/showDatumTransformDialog", chkShowDatumTransformDialog->isChecked() );

  if ( radFeet->isChecked() )
  {
    settings.setValue( "/qgis/measure/displayunits", QGis::toLiteral( QGis::Feet ) );
  }
  else if ( radNautical->isChecked() )
  {
    settings.setValue( "/qgis/measure/displayunits", QGis::toLiteral( QGis::NauticalMiles ) );
  }
  else if ( radDegrees->isChecked() )
  {
    settings.setValue( "/qgis/measure/displayunits", QGis::toLiteral( QGis::Degrees ) );
  }
  else
  {
    settings.setValue( "/qgis/measure/displayunits", QGis::toLiteral( QGis::Meters ) );
  }

  QString angleUnitString = "degrees";
  if ( mRadiansRadioButton->isChecked() )
  {
    angleUnitString = "radians";
  }
  else if ( mGonRadioButton->isChecked() )
  {
    angleUnitString = "gon";
  }
  settings.setValue( "/qgis/measure/angleunits", angleUnitString );

  int decimalPlaces = mDecimalPlacesSpinBox->value();
  settings.setValue( "/qgis/measure/decimalplaces", decimalPlaces );

  bool baseUnit = mKeepBaseUnitCheckBox->isChecked();
  settings.setValue( "/qgis/measure/keepbaseunit", baseUnit );

  //set the color for selections
  QColor myColor = pbnSelectionColor->color();
  settings.setValue( "/qgis/default_selection_color_red", myColor.red() );
  settings.setValue( "/qgis/default_selection_color_green", myColor.green() );
  settings.setValue( "/qgis/default_selection_color_blue", myColor.blue() );
  settings.setValue( "/qgis/default_selection_color_alpha", myColor.alpha() );

  //set the default color for canvas background
  myColor = pbnCanvasColor->color();
  settings.setValue( "/qgis/default_canvas_color_red", myColor.red() );
  settings.setValue( "/qgis/default_canvas_color_green", myColor.green() );
  settings.setValue( "/qgis/default_canvas_color_blue", myColor.blue() );

  //set the default color for the measure tool
  myColor = pbnMeasureColor->color();
  settings.setValue( "/qgis/default_measure_color_red", myColor.red() );
  settings.setValue( "/qgis/default_measure_color_green", myColor.green() );
  settings.setValue( "/qgis/default_measure_color_blue", myColor.blue() );

  settings.setValue( "/qgis/wheel_action", cmbWheelAction->currentIndex() );
  settings.setValue( "/qgis/zoom_factor", spinZoomFactor->value() );

  //digitizing
  settings.setValue( "/qgis/digitizing/line_width", mLineWidthSpinBox->value() );
  QColor digitizingColor = mLineColorToolButton->color();
  settings.setValue( "/qgis/digitizing/line_color_red", digitizingColor.red() );
  settings.setValue( "/qgis/digitizing/line_color_green", digitizingColor.green() );
  settings.setValue( "/qgis/digitizing/line_color_blue", digitizingColor.blue() );
  settings.setValue( "/qgis/digitizing/line_color_alpha", digitizingColor.alpha() );

  //default snap mode
  QString defaultSnapModeString = mDefaultSnapModeComboBox->itemData( mDefaultSnapModeComboBox->currentIndex() ).toString();
  settings.setValue( "/qgis/digitizing/default_snap_mode", defaultSnapModeString );
  settings.setValue( "/qgis/digitizing/default_snapping_tolerance", mDefaultSnappingToleranceSpinBox->value() );
  settings.setValue( "/qgis/digitizing/search_radius_vertex_edit", mSearchRadiusVertexEditSpinBox->value() );
  settings.setValue( "/qgis/digitizing/default_snapping_tolerance_unit",
                     ( mDefaultSnappingToleranceComboBox->currentIndex() == 0 ? QgsTolerance::ProjectUnits : QgsTolerance::Pixels ) );
  settings.setValue( "/qgis/digitizing/search_radius_vertex_edit_unit",
                     ( mSearchRadiusVertexEditComboBox->currentIndex()  == 0 ? QgsTolerance::ProjectUnits : QgsTolerance::Pixels ) );

  settings.setValue( "/qgis/digitizing/marker_only_for_selected", mMarkersOnlyForSelectedCheckBox->isChecked() );

  QString markerComboText = mMarkerStyleComboBox->currentText();
  if ( markerComboText == tr( "Semi transparent circle" ) )
  {
    settings.setValue( "/qgis/digitizing/marker_style", "SemiTransparentCircle" );
  }
  else if ( markerComboText == tr( "Cross" ) )
  {
    settings.setValue( "/qgis/digitizing/marker_style", "Cross" );
  }
  else if ( markerComboText == tr( "None" ) )
  {
    settings.setValue( "/qgis/digitizing/marker_style", "None" );
  }
  settings.setValue( "/qgis/digitizing/marker_size", ( mMarkerSizeSpinBox->value() ) );

  settings.setValue( "/qgis/digitizing/reuseLastValues", chkReuseLastValues->isChecked() );
  settings.setValue( "/qgis/digitizing/disable_enter_attribute_values_dialog", chkDisableAttributeValuesDlg->isChecked() );
  settings.setValue( "/qgis/digitizing/validate_geometries", mValidateGeometries->currentIndex() );

  settings.setValue( "/qgis/digitizing/offset_join_style", mOffsetJoinStyleComboBox->itemData( mOffsetJoinStyleComboBox->currentIndex() ).toInt() );
  settings.setValue( "/qgis/digitizing/offset_quad_seg", mOffsetQuadSegSpinBox->value() );
  settings.setValue( "/qgis/digitizing/offset_miter_limit", mCurveOffsetMiterLimitComboBox->value() );

  // default scale list
  myPaths.clear();
  for ( int i = 0; i < mListGlobalScales->count(); ++i )
  {
    if ( i != 0 )
    {
      myPaths += ",";
    }
    myPaths += mListGlobalScales->item( i )->text();
  }
  settings.setValue( "Map/scales", myPaths );

  //
  // Color palette
  //
  if ( mTreeCustomColors->isDirty() )
  {
    mTreeCustomColors->saveColorsToScheme();
  }

  //
  // Composer settings
  //

  //default font
  QString composerFont = mComposerFontComboBox->currentFont().family();
  settings.setValue( "/Composer/defaultFont", composerFont );

  //grid color
  settings.setValue( "/Composer/gridRed", mGridColorButton->color().red() );
  settings.setValue( "/Composer/gridGreen", mGridColorButton->color().green() );
  settings.setValue( "/Composer/gridBlue", mGridColorButton->color().blue() );
  settings.setValue( "/Composer/gridAlpha", mGridColorButton->color().alpha() );

  //grid style
  if ( mGridStyleComboBox->currentText() == tr( "Solid" ) )
  {
    settings.setValue( "/Composer/gridStyle", "Solid" );
  }
  else if ( mGridStyleComboBox->currentText() == tr( "Dots" ) )
  {
    settings.setValue( "/Composer/gridStyle", "Dots" );
  }
  else if ( mGridStyleComboBox->currentText() == tr( "Crosses" ) )
  {
    settings.setValue( "/Composer/gridStyle", "Crosses" );
  }

  //grid and guide defaults
  settings.setValue( "/Composer/defaultSnapGridResolution", mGridResolutionSpinBox->value() );
  settings.setValue( "/Composer/defaultSnapTolerancePixels", mSnapToleranceSpinBox->value() );
  settings.setValue( "/Composer/defaultSnapGridOffsetX", mOffsetXSpinBox->value() );
  settings.setValue( "/Composer/defaultSnapGridOffsetY", mOffsetYSpinBox->value() );

  //
  // Locale settings
  //
  settings.setValue( "locale/userLocale", cboLocale->itemData( cboLocale->currentIndex() ).toString() );
  settings.setValue( "locale/overrideFlag", grpLocale->isChecked() );

  // Gdal skip driver list
  if ( mLoadedGdalDriverList )
    saveGdalDriverList();

  // refresh legend if any legend item's state is to be changed
  if ( legendLayersBold != mLegendLayersBoldChkBx->isChecked()
       || legendGroupsBold != mLegendGroupsBoldChkBx->isChecked()
       || legendLayersCapitalise != capitaliseCheckBox->isChecked() )
  {
    // TODO[MD] QgisApp::instance()->legend()->updateLegendItemStyles();
  }

  // refresh symbology for any legend items, only if needed
  if ( showLegendClassifiers != cbxLegendClassifiers->isChecked()
       || createRasterLegendIcons != cbxCreateRasterLegendIcons->isChecked() )
  {
    // TODO[MD] QgisApp::instance()->legend()->updateLegendItemSymbologies();
  }

  // save app stylesheet last (in case reset becomes necessary)
  if ( mStyleSheetNewOpts != mStyleSheetOldOpts )
  {
    mStyleSheetBuilder->saveToSettings( mStyleSheetNewOpts );
  }

  saveDefaultDatumTransformations();
}

void QgsOptions::rejectOptions()
{
  // don't reset stylesheet if we don't have to
  if ( mStyleSheetNewOpts != mStyleSheetOldOpts )
  {
    mStyleSheetBuilder->buildStyleSheet( mStyleSheetOldOpts );
  }
}

void QgsOptions::on_spinFontSize_valueChanged( int fontSize )
{
  mStyleSheetNewOpts.insert( "fontPointSize", QVariant( fontSize ) );
  mStyleSheetBuilder->buildStyleSheet( mStyleSheetNewOpts );
}

void QgsOptions::on_mFontFamilyRadioQt_released()
{
  if ( mStyleSheetNewOpts.value( "fontFamily" ).toString() != mStyleSheetBuilder->defaultFont().family() )
  {
    mStyleSheetNewOpts.insert( "fontFamily", QVariant( mStyleSheetBuilder->defaultFont().family() ) );
    mStyleSheetBuilder->buildStyleSheet( mStyleSheetNewOpts );
  }
}

void QgsOptions::on_mFontFamilyRadioCustom_released()
{
  if ( mFontFamilyComboBox->currentFont().family() != mStyleSheetBuilder->defaultFont().family() )
  {
    mStyleSheetNewOpts.insert( "fontFamily", QVariant( mFontFamilyComboBox->currentFont().family() ) );
    mStyleSheetBuilder->buildStyleSheet( mStyleSheetNewOpts );
  }
}

void QgsOptions::on_mFontFamilyComboBox_currentFontChanged( const QFont& font )
{
  if ( mFontFamilyRadioCustom->isChecked()
       && mStyleSheetNewOpts.value( "fontFamily" ).toString() != font.family() )
  {
    mStyleSheetNewOpts.insert( "fontFamily", QVariant( font.family() ) );
    mStyleSheetBuilder->buildStyleSheet( mStyleSheetNewOpts );
  }
}

void QgsOptions::on_mCustomGroupBoxChkBx_clicked( bool chkd )
{
  mStyleSheetNewOpts.insert( "groupBoxCustom", QVariant( chkd ) );
  mStyleSheetBuilder->buildStyleSheet( mStyleSheetNewOpts );
}

void QgsOptions::on_mCustomSideBarSide_clicked( bool chkd )
{
  mStyleSheetNewOpts.insert( "sidebarStyle", chkd );
  mStyleSheetBuilder->buildStyleSheet( mStyleSheetNewOpts );
}

void QgsOptions::on_mBoldGroupBoxTitleChkBx_clicked( bool chkd )
{
  mStyleSheetNewOpts.insert( "groupBoxBoldTitle", QVariant( chkd ) );
  mStyleSheetBuilder->buildStyleSheet( mStyleSheetNewOpts );
}

void QgsOptions::on_leProjectGlobalCrs_crsChanged( QgsCoordinateReferenceSystem crs )
{
  mDefaultCrs = crs;
}

void QgsOptions::on_leLayerGlobalCrs_crsChanged( QgsCoordinateReferenceSystem crs )
{
  mLayerDefaultCrs = crs;
}

void QgsOptions::on_lstGdalDrivers_itemDoubleClicked( QTreeWidgetItem * item, int column )
{
  Q_UNUSED( column );
  // edit driver if driver supports write
  if ( item && ( cmbEditCreateOptions->findText( item->text( 0 ) ) != -1 ) )
  {
    editGdalDriver( item->text( 0 ) );
  }
}

void QgsOptions::on_pbnEditCreateOptions_pressed()
{
  editGdalDriver( cmbEditCreateOptions->currentText() );
}

void QgsOptions::on_pbnEditPyramidsOptions_pressed()
{
  editGdalDriver( "_pyramids" );
}

void QgsOptions::editGdalDriver( const QString& driverName )
{
  if ( driverName.isEmpty() )
    return;

  QgsDialog dlg( this, 0, QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
  QVBoxLayout *layout = dlg.layout();
  QString title = tr( "Create Options - %1 Driver" ).arg( driverName );
  if ( driverName == "_pyramids" )
    title = tr( "Create Options - pyramids" );
  dlg.setWindowTitle( title );
  QLabel *label = new QLabel( title, &dlg );
  label->setAlignment( Qt::AlignHCenter );
  layout->addWidget( label );

  if ( driverName == "_pyramids" )
  {
    QgsRasterPyramidsOptionsWidget* optionsWidget =
      new QgsRasterPyramidsOptionsWidget( &dlg, "gdal" );
    layout->addWidget( optionsWidget );
    dlg.resize( 400, 400 );
    if ( dlg.exec() == QDialog::Accepted )
      optionsWidget->apply();
  }
  else
  {
    QgsRasterFormatSaveOptionsWidget* optionsWidget =
      new QgsRasterFormatSaveOptionsWidget( &dlg, driverName,
                                            QgsRasterFormatSaveOptionsWidget::Full, "gdal" );
    layout->addWidget( optionsWidget );
    if ( dlg.exec() == QDialog::Accepted )
      optionsWidget->apply();
  }

}

// Return state of the visibility flag for newly added layers. If

bool QgsOptions::newVisible()
{
  return chkAddedVisibility->isChecked();
}

QStringList QgsOptions::i18nList()
{
  QStringList myList;
  myList << "en_US"; //there is no qm file for this so we add it manually
  QString myI18nPath = QgsApplication::i18nPath();
  QDir myDir( myI18nPath, "qgis*.qm" );
  QStringList myFileList = myDir.entryList();
  QStringListIterator myIterator( myFileList );
  while ( myIterator.hasNext() )
  {
    QString myFileName = myIterator.next();

    // Ignore the 'en' translation file, already added as 'en_US'.
    if ( myFileName.compare( "qgis_en.qm" ) == 0 ) continue;

    myList << myFileName.replace( "qgis_", "" ).replace( ".qm", "" );
  }
  return myList;
}

void QgsOptions::on_mRestoreDefaultWindowStateBtn_clicked()
{
  // richard
  QSettings mySettings;
  if ( QMessageBox::warning( this, tr( "Restore UI defaults" ), tr( "Are you sure to reset the UI to default (needs restart)?" ), QMessageBox::Ok | QMessageBox::Cancel ) == QMessageBox::Cancel )
    return;
  mySettings.setValue( "/qgis/restoreDefaultWindowState", true );
}

void QgsOptions::on_mCustomVariablesChkBx_toggled( bool chkd )
{
  mAddCustomVarBtn->setEnabled( chkd );
  mRemoveCustomVarBtn->setEnabled( chkd );
  mCustomVariablesTable->setEnabled( chkd );
}

void QgsOptions::addCustomEnvVarRow( QString varName, QString varVal, QString varApply )
{
  int rowCnt = mCustomVariablesTable->rowCount();
  mCustomVariablesTable->insertRow( rowCnt );

  QComboBox* varApplyCmbBx = new QComboBox( this );
  varApplyCmbBx->addItem( tr( "Overwrite" ), QVariant( "overwrite" ) );
  varApplyCmbBx->addItem( tr( "If Undefined" ), QVariant( "undefined" ) );
  varApplyCmbBx->addItem( tr( "Unset" ), QVariant( "unset" ) );
  varApplyCmbBx->addItem( tr( "Prepend" ), QVariant( "prepend" ) );
  varApplyCmbBx->addItem( tr( "Append" ), QVariant( "append" ) );
  varApplyCmbBx->setCurrentIndex( varApply.isEmpty() ? 0 : varApplyCmbBx->findData( QVariant( varApply ) ) );

  QFont cbf = varApplyCmbBx->font();
  QFontMetrics cbfm = QFontMetrics( cbf );
  cbf.setPointSize( cbf.pointSize() - 2 );
  varApplyCmbBx->setFont( cbf );
  mCustomVariablesTable->setCellWidget( rowCnt, 0, varApplyCmbBx );

  Qt::ItemFlags itmFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable
                           | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;

  QTableWidgetItem* varNameItm = new QTableWidgetItem( varName );
  varNameItm->setFlags( itmFlags );
  mCustomVariablesTable->setItem( rowCnt, 1, varNameItm );

  QTableWidgetItem* varValueItm = new QTableWidgetItem( varVal );
  varNameItm->setFlags( itmFlags );
  mCustomVariablesTable->setItem( rowCnt, 2, varValueItm );

  mCustomVariablesTable->setRowHeight( rowCnt, cbfm.height() + 8 );
}

void QgsOptions::on_mAddCustomVarBtn_clicked()
{
  addCustomEnvVarRow( QString( "" ), QString( "" ) );
  mCustomVariablesTable->setFocus();
  mCustomVariablesTable->setCurrentCell( mCustomVariablesTable->rowCount() - 1, 1 );
  mCustomVariablesTable->edit( mCustomVariablesTable->currentIndex() );
}

void QgsOptions::on_mRemoveCustomVarBtn_clicked()
{
  mCustomVariablesTable->removeRow( mCustomVariablesTable->currentRow() );
}

void QgsOptions::on_mCurrentVariablesQGISChxBx_toggled( bool qgisSpecific )
{
  for ( int i = mCurrentVariablesTable->rowCount() - 1; i >= 0; --i )
  {
    if ( qgisSpecific )
    {
      QString itmTxt = mCurrentVariablesTable->item( i, 0 )->text();
      if ( !itmTxt.startsWith( "QGIS", Qt::CaseInsensitive ) )
        mCurrentVariablesTable->hideRow( i );
    }
    else
    {
      mCurrentVariablesTable->showRow( i );
    }
  }
  if ( mCurrentVariablesTable->rowCount() > 0 )
  {
    mCurrentVariablesTable->sortByColumn( 0, Qt::AscendingOrder );
    mCurrentVariablesTable->resizeColumnToContents( 0 );
  }
}

void QgsOptions::on_mBtnAddPluginPath_clicked()
{
  QString myDir = QFileDialog::getExistingDirectory(
                    this,
                    tr( "Choose a directory" ),
                    QDir::toNativeSeparators( QDir::homePath() ),
                    QFileDialog::ShowDirsOnly
                  );

  if ( ! myDir.isEmpty() )
  {
    QListWidgetItem* newItem = new QListWidgetItem( mListPluginPaths );
    newItem->setText( myDir );
    newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    mListPluginPaths->addItem( newItem );
    mListPluginPaths->setCurrentItem( newItem );
  }
}

void QgsOptions::on_mBtnRemovePluginPath_clicked()
{
  int currentRow = mListPluginPaths->currentRow();
  QListWidgetItem* itemToRemove = mListPluginPaths->takeItem( currentRow );
  delete itemToRemove;
}

void QgsOptions::on_mBtnAddSVGPath_clicked()
{
  QString myDir = QFileDialog::getExistingDirectory(
                    this,
                    tr( "Choose a directory" ),
                    QDir::toNativeSeparators( QDir::homePath() ),
                    QFileDialog::ShowDirsOnly
                  );

  if ( ! myDir.isEmpty() )
  {
    QListWidgetItem* newItem = new QListWidgetItem( mListSVGPaths );
    newItem->setText( myDir );
    newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    mListSVGPaths->addItem( newItem );
    mListSVGPaths->setCurrentItem( newItem );
  }
}

void QgsOptions::on_mBtnRemoveSVGPath_clicked()
{
  int currentRow = mListSVGPaths->currentRow();
  QListWidgetItem* itemToRemove = mListSVGPaths->takeItem( currentRow );
  delete itemToRemove;
}

void QgsOptions::on_mAddUrlPushButton_clicked()
{
  QListWidgetItem* newItem = new QListWidgetItem( mExcludeUrlListWidget );
  newItem->setText( "URL" );
  newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
  mExcludeUrlListWidget->addItem( newItem );
  mExcludeUrlListWidget->setCurrentItem( newItem );
}

void QgsOptions::on_mRemoveUrlPushButton_clicked()
{
  int currentRow = mExcludeUrlListWidget->currentRow();
  QListWidgetItem* itemToRemove = mExcludeUrlListWidget->takeItem( currentRow );
  delete itemToRemove;
}

void QgsOptions::on_mBrowseCacheDirectory_clicked()
{
  QString myDir = QFileDialog::getExistingDirectory(
                    this,
                    tr( "Choose a directory" ),
                    QDir::toNativeSeparators( mCacheDirectory->text() ),
                    QFileDialog::ShowDirsOnly
                  );

  if ( !myDir.isEmpty() )
  {
    mCacheDirectory->setText( QDir::toNativeSeparators( myDir ) );
  }

}

void QgsOptions::on_mClearCache_clicked()
{
  QgsNetworkAccessManager::instance()->cache()->clear();
}

void QgsOptions::on_mOptionsStackedWidget_currentChanged( int theIndx )
{
  Q_UNUSED( theIndx );
  // load gdal driver list when gdal tab is first opened
  if ( mOptionsStackedWidget->currentWidget()->objectName() == QString( "mOptionsPageGDAL" )
       && ! mLoadedGdalDriverList )
  {
    loadGdalDriverList();
  }
}

void QgsOptions::loadGdalDriverList()
{
  QStringList mySkippedDrivers = QgsApplication::skippedGdalDrivers();
  GDALDriverH myGdalDriver; // current driver
  QString myGdalDriverDescription;
  QStringList myDrivers;
  QStringList myGdalWriteDrivers;
  QMap<QString, QString> myDriversFlags, myDriversExt, myDriversLongName;

  // make sure we save list when accept()
  mLoadedGdalDriverList = true;

  // allow to retrieve metadata from all drivers, they will be skipped again when saving
  CPLSetConfigOption( "GDAL_SKIP", "" );
  GDALAllRegister();

  int myGdalDriverCount = GDALGetDriverCount();
  for ( int i = 0; i < myGdalDriverCount; ++i )
  {
    myGdalDriver = GDALGetDriver( i );

    Q_CHECK_PTR( myGdalDriver );

    if ( !myGdalDriver )
    {
      QgsLogger::warning( "unable to get driver " + QString::number( i ) );
      continue;
    }

    // in GDAL 2.0 vector and mixed drivers are returned by GDALGetDriver, so filter out non-raster drivers
    // TODO add same UI for vector drivers
#ifdef GDAL_COMPUTE_VERSION
#if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2,0,0)
    if ( QString( GDALGetMetadataItem( myGdalDriver, GDAL_DCAP_RASTER, NULL ) ) != "YES" )
      continue;
#endif
#endif

    myGdalDriverDescription = GDALGetDescription( myGdalDriver );
    myDrivers << myGdalDriverDescription;

    QgsDebugMsg( QString( "driver #%1 - %2" ).arg( i ).arg( myGdalDriverDescription ) );

    // get driver R/W flags, taken from GDALGeneralCmdLineProcessor()
    const char *pszRWFlag, *pszVirtualIO;
    if ( GDALGetMetadataItem( myGdalDriver, GDAL_DCAP_CREATE, NULL ) )
    {
      myGdalWriteDrivers << myGdalDriverDescription;
      pszRWFlag = "rw+";
    }
    else if ( GDALGetMetadataItem( myGdalDriver, GDAL_DCAP_CREATECOPY,
                                   NULL ) )
      pszRWFlag = "rw";
    else
      pszRWFlag = "ro";
    if ( GDALGetMetadataItem( myGdalDriver, GDAL_DCAP_VIRTUALIO, NULL ) )
      pszVirtualIO = "v";
    else
      pszVirtualIO = "";
    myDriversFlags[myGdalDriverDescription] = QString( "%1%2" ).arg( pszRWFlag ).arg( pszVirtualIO );

    // get driver extensions and long name
    // the gdal provider can override/add extensions but there is no interface to query this
    // aside from parsing QgsRasterLayer::buildSupportedRasterFileFilter()
    myDriversExt[myGdalDriverDescription] = QString( GDALGetMetadataItem( myGdalDriver, "DMD_EXTENSION", "" ) ).toLower();
    myDriversLongName[myGdalDriverDescription] = QString( GDALGetMetadataItem( myGdalDriver, "DMD_LONGNAME", "" ) );

  }
  // restore GDAL_SKIP just in case
  CPLSetConfigOption( "GDAL_SKIP", mySkippedDrivers.join( " " ).toUtf8() );

  myDrivers.removeDuplicates();
  // myDrivers.sort();
  // sort list case insensitive - no existing function for this!
  QMap<QString, QString> strMap;
  foreach ( QString str, myDrivers )
    strMap.insert( str.toLower(), str );
  myDrivers = strMap.values();

  foreach ( QString myName, myDrivers )
  {
    QTreeWidgetItem * mypItem = new QTreeWidgetItem( QStringList( myName ) );
    if ( mySkippedDrivers.contains( myName ) )
    {
      mypItem->setCheckState( 0, Qt::Unchecked );
    }
    else
    {
      mypItem->setCheckState( 0, Qt::Checked );
    }

    // add driver metadata
    mypItem->setText( 1, myDriversExt[myName] );
    QString myFlags = myDriversFlags[myName];
    mypItem->setText( 2, myFlags );
    mypItem->setText( 3, myDriversLongName[myName] );
    lstGdalDrivers->addTopLevelItem( mypItem );
  }
  // adjust column width
  for ( int i = 0; i < 4; i++ )
  {
    lstGdalDrivers->resizeColumnToContents( i );
    lstGdalDrivers->setColumnWidth( i, lstGdalDrivers->columnWidth( i ) + 5 );
  }

  // populate cmbEditCreateOptions with gdal write drivers - sorted, GTiff first
  strMap.clear();
  foreach ( QString str, myGdalWriteDrivers )
    strMap.insert( str.toLower(), str );
  myGdalWriteDrivers = strMap.values();
  myGdalWriteDrivers.removeAll( "Gtiff" );
  myGdalWriteDrivers.prepend( "GTiff" );
  cmbEditCreateOptions->clear();
  foreach ( QString myName, myGdalWriteDrivers )
  {
    cmbEditCreateOptions->addItem( myName );
  }

}

void QgsOptions::saveGdalDriverList()
{
  for ( int i = 0; i < lstGdalDrivers->topLevelItemCount(); i++ )
  {
    QTreeWidgetItem * mypItem = lstGdalDrivers->topLevelItem( i );
    if ( mypItem->checkState( 0 ) == Qt::Unchecked )
    {
      QgsApplication::skipGdalDriver( mypItem->text( 0 ) );
    }
    else
    {
      QgsApplication::restoreGdalDriver( mypItem->text( 0 ) );
    }
  }
  QSettings mySettings;
  mySettings.setValue( "gdal/skipList", QgsApplication::skippedGdalDrivers().join( " " ) );
}

void QgsOptions::on_pbnAddScale_clicked()
{
  int myScale = QInputDialog::getInt(
                  this,
                  tr( "Enter scale" ),
                  tr( "Scale denominator" ),
                  -1,
                  1
                );

  if ( myScale != -1 )
  {
    QListWidgetItem* newItem = new QListWidgetItem( mListGlobalScales );
    newItem->setText( QString( "1:%1" ).arg( myScale ) );
    newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    mListGlobalScales->addItem( newItem );
    mListGlobalScales->setCurrentItem( newItem );
  }
}

void QgsOptions::on_pbnRemoveScale_clicked()
{
  int currentRow = mListGlobalScales->currentRow();
  QListWidgetItem* itemToRemove = mListGlobalScales->takeItem( currentRow );
  delete itemToRemove;
}

void QgsOptions::on_pbnDefaultScaleValues_clicked()
{
  mListGlobalScales->clear();

  QStringList myScalesList = PROJECT_SCALES.split( "," );
  QStringList::const_iterator scaleIt = myScalesList.constBegin();
  for ( ; scaleIt != myScalesList.constEnd(); ++scaleIt )
  {
    QListWidgetItem* newItem = new QListWidgetItem( mListGlobalScales );
    newItem->setText( *scaleIt );
    newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    mListGlobalScales->addItem( newItem );
  }
}

void QgsOptions::on_pbnImportScales_clicked()
{
  QString fileName = QFileDialog::getOpenFileName( this, tr( "Load scales" ), ".",
                     tr( "XML files (*.xml *.XML)" ) );
  if ( fileName.isEmpty() )
  {
    return;
  }

  QString msg;
  QStringList myScales;
  if ( !QgsScaleUtils::loadScaleList( fileName, myScales, msg ) )
  {
    QgsDebugMsg( msg );
  }

  QStringList::const_iterator scaleIt = myScales.constBegin();
  for ( ; scaleIt != myScales.constEnd(); ++scaleIt )
  {
    QListWidgetItem* newItem = new QListWidgetItem( mListGlobalScales );
    newItem->setText( *scaleIt );
    newItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable );
    mListGlobalScales->addItem( newItem );
  }
}

void QgsOptions::on_pbnExportScales_clicked()
{
  QString fileName = QFileDialog::getSaveFileName( this, tr( "Save scales" ), ".",
                     tr( "XML files (*.xml *.XML)" ) );
  if ( fileName.isEmpty() )
  {
    return;
  }

  // ensure the user never ommited the extension from the file name
  if ( !fileName.toLower().endsWith( ".xml" ) )
  {
    fileName += ".xml";
  }

  QStringList myScales;
  for ( int i = 0; i < mListGlobalScales->count(); ++i )
  {
    myScales.append( mListGlobalScales->item( i )->text() );
  }

  QString msg;
  if ( !QgsScaleUtils::saveScaleList( fileName, myScales, msg ) )
  {
    QgsDebugMsg( msg );
  }
}

void QgsOptions::initContrastEnhancement( QComboBox *cbox, QString name, QString defaultVal )
{
  QSettings settings;

  //add items to the color enhanceContrast combo box
  cbox->addItem( tr( "No Stretch" ), "NoEnhancement" );
  cbox->addItem( tr( "Stretch To MinMax" ), "StretchToMinimumMaximum" );
  cbox->addItem( tr( "Stretch And Clip To MinMax" ), "StretchAndClipToMinimumMaximum" );
  cbox->addItem( tr( "Clip To MinMax" ), "ClipToMinimumMaximum" );

  QString contrastEnchacement = settings.value( "/Raster/defaultContrastEnhancementAlgorithm/" + name, defaultVal ).toString();
  cbox->setCurrentIndex( cbox->findData( contrastEnchacement ) );
}

void QgsOptions::saveContrastEnhancement( QComboBox *cbox, QString name )
{
  QSettings settings;
  QString value = cbox->itemData( cbox->currentIndex() ).toString();
  settings.setValue( "/Raster/defaultContrastEnhancementAlgorithm/" + name, value );
}

void QgsOptions::on_mRemoveDefaultTransformButton_clicked()
{
  QList<QTreeWidgetItem*> items = mDefaultDatumTransformTreeWidget->selectedItems();
  for ( int i = 0; i < items.size(); ++i )
  {
    int idx = mDefaultDatumTransformTreeWidget->indexOfTopLevelItem( items.at( i ) );
    if ( idx >= 0 )
    {
      delete mDefaultDatumTransformTreeWidget->takeTopLevelItem( idx );
    }
  }
}

void QgsOptions::on_mAddDefaultTransformButton_clicked()
{
  QTreeWidgetItem* item = new QTreeWidgetItem();
  item->setText( 0, "" );
  item->setText( 1, "" );
  item->setText( 2, "" );
  item->setText( 3, "" );
  item->setFlags( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable );
  mDefaultDatumTransformTreeWidget->addTopLevelItem( item );
}

void QgsOptions::saveDefaultDatumTransformations()
{
  QSettings s;
  s.beginGroup( "/Projections" );
  QStringList groupKeys = s.allKeys();
  QStringList::const_iterator groupKeyIt = groupKeys.constBegin();
  for ( ; groupKeyIt != groupKeys.constEnd(); ++groupKeyIt )
  {
    if ( groupKeyIt->contains( "srcTransform" ) || groupKeyIt->contains( "destTransform" ) )
    {
      s.remove( *groupKeyIt );
    }
  }

  int nDefaultTransforms = mDefaultDatumTransformTreeWidget->topLevelItemCount();
  for ( int i = 0; i < nDefaultTransforms; ++i )
  {
    QTreeWidgetItem* item = mDefaultDatumTransformTreeWidget->topLevelItem( i );
    QString srcAuthId = item->text( 0 );
    QString destAuthId = item->text( 1 );
    if ( srcAuthId.isEmpty() || destAuthId.isEmpty() )
    {
      continue;
    }

    bool conversionOk;
    int srcDatumTransform = item->text( 2 ).toInt( &conversionOk );
    if ( conversionOk )
    {
      s.setValue( srcAuthId + "//" + destAuthId + "_srcTransform", srcDatumTransform );
    }
    int destDatumTransform = item->text( 3 ).toInt( &conversionOk );
    if ( conversionOk )
    {
      s.setValue( srcAuthId + "//" + destAuthId + "_destTransform", destDatumTransform );
    }
  }

  s.endGroup();
}


void QgsOptions::on_mButtonAddColor_clicked()
{
  QColor newColor = QgsColorDialogV2::getColor( QColor(), this->parentWidget(), tr( "Select color" ), true );
  if ( !newColor.isValid() )
  {
    return;
  }
  activateWindow();

  mTreeCustomColors->addColor( newColor, QgsSymbolLayerV2Utils::colorToName( newColor ) );
}

void QgsOptions::on_mButtonImportColors_clicked()
{
  QSettings s;
  QString lastDir = s.value( "/UI/lastGplPaletteDir", "" ).toString();
  QString filePath = QFileDialog::getOpenFileName( this, tr( "Select palette file" ), lastDir, "GPL (*.gpl);;All files (*.*)" );
  activateWindow();
  if ( filePath.isEmpty() )
  {
    return;
  }

  //check if file exists
  QFileInfo fileInfo( filePath );
  if ( !fileInfo.exists() || !fileInfo.isReadable() )
  {
    QMessageBox::critical( 0, tr( "Invalid file" ), tr( "Error, file does not exist or is not readable" ) );
    return;
  }

  s.setValue( "/UI/lastGplPaletteDir", fileInfo.absolutePath() );
  QFile file( filePath );
  bool importOk = mTreeCustomColors->importColorsFromGpl( file );
  if ( !importOk )
  {
    QMessageBox::critical( 0, tr( "Invalid file" ), tr( "Error, no colors found in palette file" ) );
    return;
  }
}

void QgsOptions::on_mButtonExportColors_clicked()
{
  QSettings s;
  QString lastDir = s.value( "/UI/lastGplPaletteDir", "" ).toString();
  QString fileName = QFileDialog::getSaveFileName( this, tr( "Palette file" ), lastDir, "GPL (*.gpl)" );
  activateWindow();
  if ( fileName.isEmpty() )
  {
    return;
  }

  // ensure filename contains extension
  if ( !fileName.toLower().endsWith( ".gpl" ) )
  {
    fileName += ".gpl";
  }

  QFileInfo fileInfo( fileName );
  s.setValue( "/UI/lastGplPaletteDir", fileInfo.absolutePath() );

  QFile file( fileName );
  bool exportOk = mTreeCustomColors->exportColorsToGpl( file );
  if ( !exportOk )
  {
    QMessageBox::critical( 0, tr( "Error exporting" ), tr( "Error writing palette file" ) );
    return;
  }
}
