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
/* $Id$ */
#include "qgsapplication.h"
#include "qgsoptions.h"
#include "qgis.h"
#include "qgisapp.h"
#include "qgsgenericprojectionselector.h"
#include "qgscoordinatereferencesystem.h"

#include <QFileDialog>
#include <QSettings>
#include <QColorDialog>
#include <QLocale>

#include <cassert>
#include <sqlite3.h>
#include "qgslogger.h"
#define ELLIPS_FLAT "NONE"
#define ELLIPS_FLAT_DESC "None / Planimetric"

/**
 * \class QgsOptions - Set user options and preferences
 * Constructor
 */
QgsOptions::QgsOptions( QWidget *parent, Qt::WFlags fl ) :
    QDialog( parent, fl )
{
  setupUi( this );
  connect( cmbTheme, SIGNAL( activated( const QString& ) ), this, SLOT( themeChanged( const QString& ) ) );
  connect( cmbTheme, SIGNAL( highlighted( const QString& ) ), this, SLOT( themeChanged( const QString& ) ) );
  connect( cmbTheme, SIGNAL( textChanged( const QString& ) ), this, SLOT( themeChanged( const QString& ) ) );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  connect( this, SIGNAL( accepted() ), this, SLOT( saveOptions() ) );

  // read the current browser and set it
  QSettings settings;
  QgsDebugMsg( QString( "Standard Identify radius setting: %1" ).arg( QGis::DEFAULT_IDENTIFY_RADIUS ) );
  double identifyValue = settings.value( "/Map/identifyRadius", QGis::DEFAULT_IDENTIFY_RADIUS ).toDouble();
  QgsDebugMsg( QString( "Standard Identify radius setting read from settings file: %1" ).arg( identifyValue ) );
  spinBoxIdentifyValue->setValue( identifyValue );

  //Web proxy settings
  grpProxy->setChecked( settings.value( "proxy/proxyEnabled", "0" ).toBool() );
  leProxyHost->setText( settings.value( "proxy/proxyHost", "" ).toString() );
  leProxyPort->setText( settings.value( "proxy/proxyPort", "" ).toString() );
  leProxyUser->setText( settings.value( "proxy/proxyUser", "" ).toString() );
  leProxyPassword->setText( settings.value( "proxy/proxyPassword", "" ).toString() );
  // set the current theme
  cmbTheme->setItemText( cmbTheme->currentIndex(), settings.value( "/Themes" ).toString() );

  // set the attribute table behaviour
  cmbAttrTableBehaviour->clear();
  cmbAttrTableBehaviour->addItem( tr( "Show all features" ) );
  cmbAttrTableBehaviour->addItem( tr( "Show selected features" ) );
  cmbAttrTableBehaviour->addItem( tr( "Show features in current canvas" ) );
  cmbAttrTableBehaviour->setCurrentIndex( settings.value( "/qgis/attributeTableBehaviour", 0 ).toInt() );

  // set the display update threshold
  spinBoxUpdateThreshold->setValue( settings.value( "/Map/updateThreshold" ).toInt() );
  //set the default projection behaviour radio buttongs
  if ( settings.value( "/Projections/defaultBehaviour" ).toString() == "prompt" )
  {
    radPromptForProjection->setChecked( true );
  }
  else if ( settings.value( "/Projections/defaultBehaviour" ).toString() == "useProject" )
  {
    radUseProjectProjection->setChecked( true );
  }
  else //useGlobal
  {
    radUseGlobalProjection->setChecked( true );
  }

  txtGlobalWkt->setText( settings.value( "/Projections/defaultProjectionString", GEOPROJ4 ).toString() );

  // populate combo box with ellipsoids
  getEllipsoidList();
  QString myEllipsoidId = settings.value( "/qgis/measure/ellipsoid", "WGS84" ).toString();
  cmbEllipsoid->setItemText( cmbEllipsoid->currentIndex(), getEllipsoidName( myEllipsoidId ) );
  // add the themes to the combo box on the option dialog
  QDir myThemeDir( QgsApplication::pkgDataPath() + "/themes/" );
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

  //set the state of the checkboxes
  chkAntiAliasing->setChecked( settings.value( "/qgis/enable_anti_aliasing", false ).toBool() );

  // Slightly awkard here at the settings value is true to use QImage,
  // but the checkbox is true to use QPixmap
  chkUseQPixmap->setChecked( !( settings.value( "/qgis/use_qimage_to_render", true ).toBool() ) );
  chkAddedVisibility->setChecked( settings.value( "/qgis/new_layers_visible", true ).toBool() );
  cbxLegendClassifiers->setChecked( settings.value( "/qgis/showLegendClassifiers", false ).toBool() );
  cbxHideSplash->setChecked( settings.value( "/qgis/hideSplash", false ).toBool() );
  cbxAttributeTableDocked->setChecked( settings.value( "/qgis/dockAttributeTable", false ).toBool() );

  //set the colour for selections
  int myRed = settings.value( "/qgis/default_selection_color_red", 255 ).toInt();
  int myGreen = settings.value( "/qgis/default_selection_color_green", 255 ).toInt();
  int myBlue = settings.value( "/qgis/default_selection_color_blue", 0 ).toInt();
  pbnSelectionColour->setColor( QColor( myRed, myGreen, myBlue ) );

  //set the default color for canvas background
  myRed = settings.value( "/qgis/default_canvas_color_red", 255 ).toInt();
  myGreen = settings.value( "/qgis/default_canvas_color_green", 255 ).toInt();
  myBlue = settings.value( "/qgis/default_canvas_color_blue", 255 ).toInt();
  pbnCanvasColor->setColor( QColor( myRed, myGreen, myBlue ) );

  // set the default color for the measure tool
  myRed = settings.value( "/qgis/default_measure_color_red", 180 ).toInt();
  myGreen = settings.value( "/qgis/default_measure_color_green", 180 ).toInt();
  myBlue = settings.value( "/qgis/default_measure_color_blue", 180 ).toInt();
  pbnMeasureColour->setColor( QColor( myRed, myGreen, myBlue ) );

  capitaliseCheckBox->setChecked( settings.value( "qgis/capitaliseLayerName", QVariant( false ) ).toBool() );

  chbAskToSaveProjectChanges->setChecked( settings.value( "qgis/askToSaveProjectChanges", QVariant( true ) ).toBool() );
  chbWarnOldProjectVersion->setChecked( settings.value( "/qgis/warnOldProjectVersion", QVariant( true ) ).toBool() );

  cmbWheelAction->setCurrentIndex( settings.value( "/qgis/wheel_action", 0 ).toInt() );
  spinZoomFactor->setValue( settings.value( "/qgis/zoom_factor", 2 ).toDouble() );

  cbxSplitterRedraw->setChecked( settings.value( "/qgis/splitterRedraw", QVariant( true ) ).toBool() );

  //
  // Locale settings
  //
  QString mySystemLocale = QLocale::system().name();
  lblSystemLocale->setText( tr( "Detected active locale on your system: " ) + mySystemLocale );
  QString myUserLocale = settings.value( "locale/userLocale", "" ).toString();
  QStringList myI18nList = i18nList();
  cboLocale->addItems( myI18nList );
  if ( myI18nList.contains( myUserLocale ) )
  {
    cboLocale->setItemText( cboLocale->currentIndex(), myUserLocale );
  }
  bool myLocaleOverrideFlag = settings.value( "locale/overrideFlag", false ).toBool();
  grpLocale->setChecked( myLocaleOverrideFlag );

  //set elements in digitizing tab
  mLineWidthSpinBox->setValue( settings.value( "/qgis/digitizing/line_width", 1 ).toInt() );
  QColor digitizingColor;
  myRed = settings.value( "/qgis/digitizing/line_color_red", 255 ).toInt();
  myGreen = settings.value( "/qgis/digitizing/line_color_green", 0 ).toInt();
  myBlue = settings.value( "/qgis/digitizing/line_color_blue", 0 ).toInt();
  mLineColourToolButton->setColor( QColor( myRed, myGreen, myBlue ) );

  //default snap mode
  mDefaultSnapModeComboBox->insertItem( 0, tr( "to vertex" ), "to vertex" );
  mDefaultSnapModeComboBox->insertItem( 1, tr( "to segment" ), "to segment" );
  mDefaultSnapModeComboBox->insertItem( 2, tr( "to vertex and segment" ), "to vertex and segment" );
  QString defaultSnapString = settings.value( "/qgis/digitizing/default_snap_mode", "to vertex" ).toString();
  mDefaultSnapModeComboBox->setCurrentIndex( mDefaultSnapModeComboBox->findData( defaultSnapString ) );
  mDefaultSnappingToleranceSpinBox->setValue( settings.value( "/qgis/digitizing/default_snapping_tolerance", 0 ).toDouble() );
  mSearchRadiusVertexEditSpinBox->setValue( settings.value( "/qgis/digitizing/search_radius_vertex_edit", 10 ).toDouble() );

  //vertex marker
  mMarkerStyleComboBox->addItem( tr( "Semi transparent circle" ) );
  mMarkerStyleComboBox->addItem( tr( "Cross" ) );

  QString markerStyle = settings.value( "/qgis/digitizing/marker_style", "SemiTransparentCircle" ).toString();
  if ( markerStyle == "SemiTransparentCircle" )
  {
    mMarkerStyleComboBox->setCurrentIndex( mMarkerStyleComboBox->findText( tr( "Semi transparent circle" ) ) );
  }
  else if ( markerStyle == "Cross" )
  {
    mMarkerStyleComboBox->setCurrentIndex( mMarkerStyleComboBox->findText( tr( "Cross" ) ) );
  }

#ifdef Q_WS_MAC //MH: disable incremental update on Mac for now to avoid problems with resizing 
  groupBox_5->setEnabled( false );
#endif //Q_WS_MAC
}

//! Destructor
QgsOptions::~QgsOptions() {}

void QgsOptions::on_pbnSelectionColour_clicked()
{
  QColor color = QColorDialog::getColor( pbnSelectionColour->color(), this );
  if ( color.isValid() )
  {
    pbnSelectionColour->setColor( color );
  }
}

void QgsOptions::on_pbnCanvasColor_clicked()
{
  QColor color = QColorDialog::getColor( pbnCanvasColor->color(), this );
  if ( color.isValid() )
  {
    pbnCanvasColor->setColor( color );
  }
}

void QgsOptions::on_pbnMeasureColour_clicked()
{
  QColor color = QColorDialog::getColor( pbnMeasureColour->color(), this );
  if ( color.isValid() )
  {
    pbnMeasureColour->setColor( color );
  }
}

void QgsOptions::on_mLineColourToolButton_clicked()
{
  QColor color = QColorDialog::getColor( mLineColourToolButton->color(), this );
  if ( color.isValid() )
  {
    mLineColourToolButton->setColor( color );
  }
}

void QgsOptions::themeChanged( const QString &newThemeName )
{
  // Slot to change the theme as user scrolls through the choices
  QString newt = newThemeName;
  QgisApp::instance()->setTheme( newt );
}

QString QgsOptions::theme()
{
  // returns the current theme (as selected in the cmbTheme combo box)
  return cmbTheme->currentText();
}

void QgsOptions::saveOptions()
{
  QSettings settings;
  //Web proxy settings
  settings.setValue( "proxy/proxyEnabled", grpProxy->isChecked() );
  settings.setValue( "proxy/proxyHost", leProxyHost->text() );
  settings.setValue( "proxy/proxyPort", leProxyPort->text() );
  settings.setValue( "proxy/proxyUser", leProxyUser->text() );
  settings.setValue( "proxy/proxyPassword", leProxyPassword->text() );
  //general settings
  settings.setValue( "/Map/identifyRadius", spinBoxIdentifyValue->value() );
  settings.setValue( "/qgis/showLegendClassifiers", cbxLegendClassifiers->isChecked() );
  settings.setValue( "/qgis/hideSplash", cbxHideSplash->isChecked() );
  settings.setValue( "/qgis/dockAttributeTable", cbxAttributeTableDocked->isChecked() );
  settings.setValue( "/qgis/attributeTableBehaviour", cmbAttrTableBehaviour->currentIndex() );
  settings.setValue( "/qgis/new_layers_visible", chkAddedVisibility->isChecked() );
  settings.setValue( "/qgis/enable_anti_aliasing", chkAntiAliasing->isChecked() );
  settings.setValue( "/qgis/use_qimage_to_render", !( chkUseQPixmap->isChecked() ) );
  settings.setValue( "qgis/capitaliseLayerName", capitaliseCheckBox->isChecked() );
  settings.setValue( "qgis/askToSaveProjectChanges", chbAskToSaveProjectChanges->isChecked() );
  settings.setValue( "qgis/warnOldProjectVersion", chbWarnOldProjectVersion->isChecked() );

  if ( cmbTheme->currentText().length() == 0 )
  {
    settings.setValue( "/Themes", "default" );
  }
  else
  {
    settings.setValue( "/Themes", cmbTheme->currentText() );
  }
  settings.setValue( "/Map/updateThreshold", spinBoxUpdateThreshold->value() );
  //check behaviour so default projection when new layer is added with no
  //projection defined...
  if ( radPromptForProjection->isChecked() )
  {
    //
    settings.setValue( "/Projections/defaultBehaviour", "prompt" );
  }
  else if ( radUseProjectProjection->isChecked() )
  {
    //
    settings.setValue( "/Projections/defaultBehaviour", "useProject" );
  }
  else //assumes radUseGlobalProjection is checked
  {
    //
    settings.setValue( "/Projections/defaultBehaviour", "useGlobal" );
  }

  settings.setValue( "/Projections/defaultProjectionString", txtGlobalWkt->toPlainText() );

  settings.setValue( "/qgis/measure/ellipsoid", getEllipsoidAcronym( cmbEllipsoid->currentText() ) );

  //set the colour for selections
  QColor myColor = pbnSelectionColour->color();
  settings.setValue( "/qgis/default_selection_color_red", myColor.red() );
  settings.setValue( "/qgis/default_selection_color_green", myColor.green() );
  settings.setValue( "/qgis/default_selection_color_blue", myColor.blue() );

  //set the default color for canvas background
  myColor = pbnCanvasColor->color();
  settings.setValue( "/qgis/default_canvas_color_red", myColor.red() );
  settings.setValue( "/qgis/default_canvas_color_green", myColor.green() );
  settings.setValue( "/qgis/default_canvas_color_blue", myColor.blue() );

  //set the default color for the measure tool
  myColor = pbnMeasureColour->color();
  settings.setValue( "/qgis/default_measure_color_red", myColor.red() );
  settings.setValue( "/qgis/default_measure_color_green", myColor.green() );
  settings.setValue( "/qgis/default_measure_color_blue", myColor.blue() );

  settings.setValue( "/qgis/wheel_action", cmbWheelAction->currentIndex() );
  settings.setValue( "/qgis/zoom_factor", spinZoomFactor->value() );

  settings.setValue( "/qgis/splitterRedraw", cbxSplitterRedraw->isChecked() );

  //digitizing
  settings.setValue( "/qgis/digitizing/line_width", mLineWidthSpinBox->value() );
  QColor digitizingColor = mLineColourToolButton->color();
  settings.setValue( "/qgis/digitizing/line_color_red", digitizingColor.red() );
  settings.setValue( "/qgis/digitizing/line_color_green", digitizingColor.green() );
  settings.setValue( "/qgis/digitizing/line_color_blue", digitizingColor.blue() );

  //default snap mode
  QString defaultSnapModeString = mDefaultSnapModeComboBox->itemData( mDefaultSnapModeComboBox->currentIndex() ).toString();
  settings.setValue( "/qgis/digitizing/default_snap_mode", defaultSnapModeString );
  settings.setValue( "/qgis/digitizing/default_snapping_tolerance", mDefaultSnappingToleranceSpinBox->value() );
  settings.setValue( "/qgis/digitizing/search_radius_vertex_edit", mSearchRadiusVertexEditSpinBox->value() );

  QString markerComboText = mMarkerStyleComboBox->currentText();
  if ( markerComboText == tr( "Semi transparent circle" ) )
  {
    settings.setValue( "/qgis/digitizing/marker_style", "SemiTransparentCircle" );
  }
  else if ( markerComboText == tr( "Cross" ) )
  {
    settings.setValue( "/qgis/digitizing/marker_style", "Cross" );
  }

  //
  // Locale settings
  //
  settings.setValue( "locale/userLocale", cboLocale->currentText() );
  settings.setValue( "locale/overrideFlag", grpLocale->isChecked() );
}


void QgsOptions::on_pbnSelectProjection_clicked()
{
  QSettings settings;
  QgsGenericProjectionSelector * mySelector = new QgsGenericProjectionSelector( this );

  //find out srs id of current proj4 string
  QgsCoordinateReferenceSystem refSys;
  if ( refSys.createFromProj4( txtGlobalWkt->toPlainText() ) )
  {
    mySelector->setSelectedCrsId( refSys.srsid() );
  }

  if ( mySelector->exec() )
  {
    //! @todo changes this control name in gui to txtGlobalProjString
    txtGlobalWkt->setText( mySelector->selectedProj4String() );
    QgsDebugMsg( QString( "------ Global Default Projection Selection set to ----------\n%1" ).arg( txtGlobalWkt->toPlainText() ) );
  }
  else
  {
    QgsDebugMsg( "------ Global Default Projection Selection change cancelled ----------" );
    QApplication::restoreOverrideCursor();
  }

}

void QgsOptions::on_chkAntiAliasing_stateChanged()
{
  // We can't have the anti-aliasing turned on when QPixmap is being
  // used (we we can. but it then doesn't do anti-aliasing, and this
  // will confuse people).
  if ( chkAntiAliasing->isChecked() )
    chkUseQPixmap->setChecked( false );

}

void QgsOptions::on_chkUseQPixmap_stateChanged()
{
  // We can't have the anti-aliasing turned on when QPixmap is being
  // used (we we can. but it then doesn't do anti-aliasing, and this
  // will confuse people).
  if ( chkUseQPixmap->isChecked() )
    chkAntiAliasing->setChecked( false );

}

// Return state of the visibility flag for newly added layers. If

bool QgsOptions::newVisible()
{
  return chkAddedVisibility->isChecked();
}

void QgsOptions::getEllipsoidList()
{
  // (copied from qgscustomprojectiondialog.cpp)

  //
  // Populate the ellipsoid combo
  //
  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;


  cmbEllipsoid->addItem( ELLIPS_FLAT_DESC );
  //check the db is available
  myResult = sqlite3_open( QgsApplication::qgisUserDbFilePath().toUtf8().data(), &myDatabase );
  if ( myResult )
  {
    QgsDebugMsg( QString( "Can't open database: %1" ).arg( sqlite3_errmsg( myDatabase ) ) );
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert( myResult == 0 );
  }

  // Set up the query to retrieve the projection information needed to populate the ELLIPSOID list
  QString mySql = "select * from tbl_ellipsoid order by name";
  myResult = sqlite3_prepare( myDatabase, mySql.toUtf8(), mySql.length(), &myPreparedStatement, &myTail );
  // XXX Need to free memory from the error msg if one is set
  if ( myResult == SQLITE_OK )
  {
    while ( sqlite3_step( myPreparedStatement ) == SQLITE_ROW )
    {
      cmbEllipsoid->addItem(( const char * )sqlite3_column_text( myPreparedStatement, 1 ) );
    }
  }
  // close the sqlite3 statement
  sqlite3_finalize( myPreparedStatement );
  sqlite3_close( myDatabase );
}

QString QgsOptions::getEllipsoidAcronym( QString theEllipsoidName )
{
  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  QString       myName( ELLIPS_FLAT );
  //check the db is available
  myResult = sqlite3_open( QgsApplication::qgisUserDbFilePath().toUtf8().data(), &myDatabase );
  if ( myResult )
  {
    QgsDebugMsg( QString( "Can't open database: %1" ).arg( sqlite3_errmsg( myDatabase ) ) );
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert( myResult == 0 );
  }
  // Set up the query to retrieve the projection information needed to populate the ELLIPSOID list
  QString mySql = "select acronym from tbl_ellipsoid where name='" + theEllipsoidName + "'";
  myResult = sqlite3_prepare( myDatabase, mySql.toUtf8(), mySql.length(), &myPreparedStatement, &myTail );
  // XXX Need to free memory from the error msg if one is set
  if ( myResult == SQLITE_OK )
  {
    if ( sqlite3_step( myPreparedStatement ) == SQLITE_ROW )
      myName = QString(( const char * )sqlite3_column_text( myPreparedStatement, 0 ) );
  }
  // close the sqlite3 statement
  sqlite3_finalize( myPreparedStatement );
  sqlite3_close( myDatabase );
  return myName;

}

QString QgsOptions::getEllipsoidName( QString theEllipsoidAcronym )
{
  sqlite3      *myDatabase;
  const char   *myTail;
  sqlite3_stmt *myPreparedStatement;
  int           myResult;
  QString       myName( ELLIPS_FLAT_DESC );
  //check the db is available
  myResult = sqlite3_open( QgsApplication::qgisUserDbFilePath().toUtf8().data(), &myDatabase );
  if ( myResult )
  {
    QgsDebugMsg( QString( "Can't open database: %1" ).arg( sqlite3_errmsg( myDatabase ) ) );
    // XXX This will likely never happen since on open, sqlite creates the
    //     database if it does not exist.
    assert( myResult == 0 );
  }
  // Set up the query to retrieve the projection information needed to populate the ELLIPSOID list
  QString mySql = "select name from tbl_ellipsoid where acronym='" + theEllipsoidAcronym + "'";
  myResult = sqlite3_prepare( myDatabase, mySql.toUtf8(), mySql.length(), &myPreparedStatement, &myTail );
  // XXX Need to free memory from the error msg if one is set
  if ( myResult == SQLITE_OK )
  {
    if ( sqlite3_step( myPreparedStatement ) == SQLITE_ROW )
      myName = QString(( const char * )sqlite3_column_text( myPreparedStatement, 0 ) );
  }
  // close the sqlite3 statement
  sqlite3_finalize( myPreparedStatement );
  sqlite3_close( myDatabase );
  return myName;

}

QStringList QgsOptions::i18nList()
{
  QStringList myList;
  myList << "en_US"; //there is no qm file for this so we add it manually
  QString myI18nPath = QgsApplication::i18nPath();
  QDir myDir( myI18nPath, "*.qm" );
  QStringList myFileList = myDir.entryList();
  QStringListIterator myIterator( myFileList );
  while ( myIterator.hasNext() )
  {
    QString myFileName = myIterator.next();
    myList << myFileName.replace( "qgis_", "" ).replace( ".qm", "" );
  }
  return myList;
}
