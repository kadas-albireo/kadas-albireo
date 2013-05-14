/***************************************************************************
 *   Copyright (C) 2004 by Gary Sherman                                    *
 *   sherman at mrcc.com                                                   *
 *                                                                         *
 *   GUI for loading a delimited text file as a layer in QGIS              *
 *   This plugin works in conjuction with the delimited text data          *
 *   provider plugin                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include "qgsdelimitedtextsourceselect.h"

#include "qgisinterface.h"
#include "qgscontexthelp.h"
#include "qgslogger.h"
#include "qgsvectordataprovider.h"
#include "qgsdelimitedtextprovider.h"
#include "qgsdelimitedtextfile.h"

#include <QButtonGroup>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegExp>
#include <QSettings>
#include <QTextStream>
#include <QTextCodec>
#include <QUrl>

const int MAX_SAMPLE_LENGTH = 200;

QgsDelimitedTextSourceSelect::QgsDelimitedTextSourceSelect( QWidget * parent, Qt::WFlags fl, bool embedded ):
    QDialog( parent, fl ),
    mFile( new QgsDelimitedTextFile() ),
    mExampleRowCount( 20 ),
    mBadRowCount( 0 ),
    mPluginKey( "/Plugin-DelimitedText" ),
    mLastFileType( "" )
{

  setupUi( this );

  QSettings settings;
  restoreGeometry( settings.value( mPluginKey + "/geometry" ).toByteArray() );

  if ( embedded )
  {
    buttonBox->button( QDialogButtonBox::Cancel )->hide();
    buttonBox->button( QDialogButtonBox::Ok )->hide();
  }

  bgFileFormat=new QButtonGroup(this);
  bgFileFormat->addButton(delimiterCSV,swFileFormat->indexOf( swpCSVOptions));
  bgFileFormat->addButton(delimiterChars,swFileFormat->indexOf( swpDelimOptions));
  bgFileFormat->addButton(delimiterRegexp,swFileFormat->indexOf( swpRegexpOptions));

  bgGeomType=new QButtonGroup(this);
  bgGeomType->addButton(geomTypeXY,swGeomType->indexOf( swpGeomXY));
  bgGeomType->addButton(geomTypeWKT,swGeomType->indexOf( swpGeomWKT));
  bgGeomType->addButton(geomTypeNone,swGeomType->indexOf( swpGeomNone));

  connect( bgFileFormat, SIGNAL(buttonClicked(int)), swFileFormat, SLOT(setCurrentIndex(int)));
  connect( bgGeomType, SIGNAL(buttonClicked(int)),swGeomType,SLOT(setCurrentIndex(int)));

  cmbEncoding->clear();
  cmbEncoding->addItems( QgsVectorDataProvider::availableEncodings() );
  cmbEncoding->setCurrentIndex( cmbEncoding->findText( "UTF-8" ) );

  loadSettings();
  updateFieldsAndEnable();

  connect( txtFilePath, SIGNAL( textChanged( QString ) ), this, SLOT( updateFileName() ) );
  connect( txtLayerName, SIGNAL( textChanged( QString ) ), this, SLOT( enableAccept() ) );
  connect( cmbEncoding, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );

  connect( delimiterCSV, SIGNAL( toggled( bool ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( delimiterChars, SIGNAL( toggled( bool ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( delimiterRegexp, SIGNAL( toggled( bool ) ), this, SLOT( updateFieldsAndEnable() ) );

  connect( cbxDelimComma, SIGNAL( stateChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( cbxDelimSpace, SIGNAL( stateChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( cbxDelimTab, SIGNAL( stateChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( cbxDelimSemicolon, SIGNAL( stateChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( cbxDelimColon, SIGNAL( stateChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );

  connect( txtDelimiterOther, SIGNAL( textChanged( QString ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( txtQuoteChars, SIGNAL( textChanged( QString ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( txtEscapeChars, SIGNAL( textChanged( QString ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( txtDelimiterRegexp, SIGNAL( textChanged( QString ) ), this, SLOT( updateFieldsAndEnable() ) );

  connect( rowCounter, SIGNAL( valueChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( cbxUseHeader, SIGNAL( stateChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( cbxSkipEmptyFields, SIGNAL( stateChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( cbxTrimFields, SIGNAL( stateChanged( int ) ), this, SLOT( updateFieldsAndEnable() ) );

  connect( cbxPointIsComma, SIGNAL( toggled( bool ) ), this, SLOT( updateFieldsAndEnable() ) );
  connect( cbxXyDms, SIGNAL( toggled( bool ) ), this, SLOT( updateFieldsAndEnable() ) );
}

QgsDelimitedTextSourceSelect::~QgsDelimitedTextSourceSelect()
{
  QSettings settings;
  settings.setValue( mPluginKey + "/geometry", saveGeometry() );
  delete mFile;
}

void QgsDelimitedTextSourceSelect::on_btnBrowseForFile_clicked()
{
  getOpenFileName();
}

void QgsDelimitedTextSourceSelect::on_buttonBox_accepted()
{
  // The following conditions should not be hit! OK will not be enabled...
  if ( txtLayerName->text().isEmpty() )
  {
    QMessageBox::warning( this, tr( "No layer name" ), tr( "Please enter a layer name before adding the layer to the map" ) );
    txtLayerName->setFocus();
    return;
  }
  if ( delimiterChars->isChecked() )
  {
    if ( selectedChars().size() == 0 )
    {
      QMessageBox::warning( this, tr( "No delimiters set" ), tr( "Please one or more characters to use as the delimiter, or choose a different delimiter type" ) );
      txtDelimiterOther->setFocus();
      return;
    }
  }
  if ( delimiterRegexp->isChecked() )
  {
    QRegExp re( txtDelimiterRegexp->text() );
    if ( ! re.isValid() )
    {
      QMessageBox::warning( this, tr( "Invalid regular expression" ), tr( "Please enter a valid regular expression as the delimiter, or choose a different delimiter type" ) );
      txtDelimiterRegexp->setFocus();
      return;
    }
  }
  if ( ! mFile->isValid() )
  {
    QMessageBox::warning( this, tr( "Invalid delimited text file" ), tr( "Please enter a valid file and delimiter" ) );
    return;
  }

  //Build the delimited text URI from the user provided information

  QUrl url = mFile->url();

  if ( cbxPointIsComma->isChecked() )
  {
    url.addQueryItem( "decimalPoint", "," );
  }
  if ( cbxXyDms->isChecked() )
  {
    url.addQueryItem( "xyDms", "yes" );
  }

  if ( geomTypeXY->isChecked() )
  {
    if ( !cmbXField->currentText().isEmpty() && !cmbYField->currentText().isEmpty() )
    {
      QString field = cmbXField->currentText();
      url.addQueryItem( "xField", field );
      field = cmbYField->currentText();
      url.addQueryItem( "yField", field );
    }
  }
  else if ( geomTypeWKT->isChecked() )
  {
    if ( ! cmbWktField->currentText().isEmpty() )
    {
      QString field = cmbWktField->currentText();
      url.addQueryItem( "wktField", field );
    }
    if ( cmbGeometryType->currentIndex() > 0 )
    {
      url.addQueryItem( "geomType", cmbGeometryType->currentText() );
    }
  }
  else
  {
    url.addQueryItem( "geomType", "none" );
  }

  if( ! geomTypeNone->isChecked()) url.addQueryItem( "spatialIndex", cbxSpatialIndex->isChecked() ? "yes" : "no" );
  url.addQueryItem( "subsetIndex", cbxSubsetIndex->isChecked() ? "yes" : "no" );
  url.addQueryItem( "watchFile", cbxWatchFile->isChecked() ? "yes" : "no" );

  // store the settings
  saveSettings();
  saveSettingsForFile( txtFilePath->text() );


  // add the layer to the map
  emit addVectorLayer( QString::fromAscii( url.toEncoded() ), txtLayerName->text(), "delimitedtext" );

  accept();
}

void QgsDelimitedTextSourceSelect::on_buttonBox_rejected()
{
  reject();
}

QString QgsDelimitedTextSourceSelect::selectedChars()
{
  QString chars = "";
  if ( cbxDelimComma->isChecked() )
    chars.append( "," );
  if ( cbxDelimSpace->isChecked() )
    chars.append( " " );
  if ( cbxDelimTab->isChecked() )
    chars.append( "\t" );
  if ( cbxDelimSemicolon->isChecked() )
    chars.append( ";" );
  if ( cbxDelimColon->isChecked() )
    chars.append( ":" );
  chars = QgsDelimitedTextFile::encodeChars( chars );
  chars.append( txtDelimiterOther->text() );
  return chars;
}
void QgsDelimitedTextSourceSelect::setSelectedChars( QString delimiters )
{
  QString chars = QgsDelimitedTextFile::decodeChars( delimiters );
  cbxDelimComma->setChecked( chars.contains( "," ) );
  cbxDelimSpace->setChecked( chars.contains( " " ) );
  cbxDelimTab->setChecked( chars.contains( "\t" ) );
  cbxDelimColon->setChecked( chars.contains( ":" ) );
  cbxDelimSemicolon->setChecked( chars.contains( ";" ) );
  chars = chars.remove( QRegExp( "[ ,:;\t]" ) );
  chars = QgsDelimitedTextFile::encodeChars( chars );
  txtDelimiterOther->setText( chars );
}

void QgsDelimitedTextSourceSelect::loadSettings( QString subkey, bool loadGeomSettings )
{
  QSettings settings;

  // at startup, fetch the last used delimiter and directory from
  // settings
  QString key = mPluginKey;
  if ( ! subkey.isEmpty() ) key.append( "/" ).append( subkey );

  // and how to use the delimiter
  QString delimiterType = settings.value( key + "/delimiterType", "" ).toString();
  if ( delimiterType == "chars" )
  {
    delimiterChars->setChecked( true );
  }
  else if ( delimiterType == "regexp" )
  {
    delimiterRegexp->setChecked( true );
  }
  else if ( delimiterType == "csv" )
  {
    delimiterCSV->setChecked( true );
  }
  swFileFormat->setCurrentIndex( bgFileFormat->checkedId() );

  QString encoding = settings.value( key + "/encoding", "" ).toString();
  if ( ! encoding.isEmpty() ) cmbEncoding->setCurrentIndex( cmbEncoding->findText( encoding ) );
  QString delimiters = settings.value( key + "/delimiters", "" ).toString();
  if ( ! delimiters.isEmpty() ) setSelectedChars( delimiters );

  txtQuoteChars->setText( settings.value( key + "/quoteChars", "\"" ).toString() );
  txtEscapeChars->setText( settings.value( key + "/escapeChars", "\"" ).toString() );

  QString regexp = settings.value( key + "/delimiterRegexp", "" ).toString();
  if ( ! regexp.isEmpty() ) txtDelimiterRegexp->setText( regexp );

  rowCounter->setValue( settings.value( key + "/startFrom", 0 ).toInt() );
  cbxUseHeader->setChecked( settings.value( key + "/useHeader", "true" ) != "false" );
  cbxTrimFields->setChecked( settings.value( key + "/trimFields", "false" ) == "true" );
  cbxSkipEmptyFields->setChecked( settings.value( key + "/skipEmptyFields", "false" ) == "true" );
  cbxPointIsComma->setChecked( settings.value( key + "/decimalPoint", "." ).toString().contains( "," ) );
  cbxSubsetIndex->setChecked( settings.value( key + "/subsetIndex", "false" ) == "true" );
  cbxSpatialIndex->setChecked( settings.value( key + "/spatialIndex", "false" ) == "true" );
  cbxWatchFile->setChecked( settings.value( key + "/watchFile", "false" ) == "true" );

  if ( loadGeomSettings )
  {
    QString geomColumnType = settings.value( key + "/geomColumnType", "xy" ).toString();
    if ( geomColumnType == "xy" ) geomTypeXY->setChecked( true );
    else if ( geomColumnType == "wkt" ) geomTypeWKT->setChecked( true );
    else geomTypeNone->setChecked( true );
    cbxXyDms->setChecked( settings.value( key + "/xyDms", "false" ) == "true" );
    swGeomType->setCurrentIndex( bgGeomType->checkedId() );
  }

}

void QgsDelimitedTextSourceSelect::saveSettings( QString subkey, bool saveGeomSettings )
{
  QSettings settings;
  QString key = mPluginKey;
  if ( ! subkey.isEmpty() ) key.append( "/" ).append( subkey );
  settings.setValue( key + "/encoding", cmbEncoding->currentText() );
  settings.setValue( key + "/geometry", saveGeometry() );

  if ( delimiterCSV->isChecked() )
    settings.setValue( key + "/delimiterType", "csv" );
  else if ( delimiterChars->isChecked() )
    settings.setValue( key + "/delimiterType", "chars" );
  else
    settings.setValue( key + "/delimiterType", "regexp" );
  settings.setValue( key + "/delimiters", selectedChars() );
  settings.setValue( key + "/quoteChars", txtQuoteChars->text() );
  settings.setValue( key + "/escapeChars", txtEscapeChars->text() );
  settings.setValue( key + "/delimiterRegexp", txtDelimiterRegexp->text() );
  settings.setValue( key + "/startFrom", rowCounter->value() );
  settings.setValue( key + "/useHeader", cbxUseHeader->isChecked() ? "true" : "false" );
  settings.setValue( key + "/trimFields", cbxTrimFields->isChecked() ? "true" : "false" );
  settings.setValue( key + "/skipEmptyFields", cbxSkipEmptyFields->isChecked() ? "true" : "false" );
  settings.setValue( key + "/decimalPoint", cbxPointIsComma->isChecked() ? "," : "." );
  settings.setValue( key + "/subsetIndex", cbxSubsetIndex->isChecked() ? "true" : "false");
  settings.setValue( key + "/spatialIndex", cbxSpatialIndex->isChecked() ? "true" : "false");
  settings.setValue( key + "/watchFile", cbxWatchFile->isChecked() ? "true" : "false");
  if ( saveGeomSettings )
  {
    QString geomColumnType = "none";
    if ( geomTypeXY->isChecked() ) geomColumnType = "xy";
    if ( geomTypeWKT->isChecked() ) geomColumnType = "wkt";
    settings.setValue( key + "/geomColumnType", geomColumnType );
    settings.setValue( key + "/xyDms", cbxXyDms->isChecked() ? "true" : "false" );
  }

}

void QgsDelimitedTextSourceSelect::loadSettingsForFile( QString filename )
{
  if ( filename.isEmpty() ) return;
  QFileInfo fi( filename );
  QString filetype = fi.suffix();
  // Don't expect to change settings if not changing file type
  if ( filetype != mLastFileType ) loadSettings( fi.suffix(), true );
  mLastFileType = filetype;
}

void QgsDelimitedTextSourceSelect::saveSettingsForFile( QString filename )
{
  if ( filename.isEmpty() ) return;
  QFileInfo fi( filename );
  saveSettings( fi.suffix(), true );
}


bool QgsDelimitedTextSourceSelect::loadDelimitedFileDefinition()
{
  mFile->setFileName( txtFilePath->text() );
  mFile->setEncoding( cmbEncoding->currentText() );
  if ( delimiterChars->isChecked() )
  {
    mFile->setTypeCSV( selectedChars(), txtQuoteChars->text(), txtEscapeChars->text() );
  }
  else if ( delimiterRegexp->isChecked() )
  {
    mFile->setTypeRegexp( txtDelimiterRegexp->text() );
  }
  else
  {
    mFile->setTypeCSV();
  }
  mFile->setSkipLines( rowCounter->value() );
  mFile->setUseHeader( cbxUseHeader->isChecked() );
  mFile->setDiscardEmptyFields( cbxSkipEmptyFields->isChecked() );
  mFile->setTrimFields( cbxTrimFields->isChecked() );
  return mFile->isValid();
}


void QgsDelimitedTextSourceSelect::updateFieldLists()
{
  // Update the x and y field dropdown boxes
  QgsDebugMsg( "Updating field lists" );

  disconnect( cmbXField, SIGNAL( currentIndexChanged( int ) ), this, SLOT( enableAccept() ) );
  disconnect( cmbYField, SIGNAL( currentIndexChanged( int ) ), this, SLOT( enableAccept() ) );
  disconnect( cmbWktField, SIGNAL( currentIndexChanged( int ) ), this, SLOT( enableAccept() ) );
  disconnect( geomTypeXY, SIGNAL( toggled( bool ) ), this, SLOT( enableAccept() ) );
  disconnect( geomTypeWKT, SIGNAL( toggled( bool ) ), this, SLOT( enableAccept() ) );
  disconnect( geomTypeNone, SIGNAL( toggled( bool ) ), this, SLOT( enableAccept() ) );

  QString columnX = cmbXField->currentText();
  QString columnY = cmbYField->currentText();
  QString columnWkt = cmbWktField->currentText();

  // clear the field lists
  cmbXField->clear();
  cmbYField->clear();
  cmbWktField->clear();

  // clear the sample text box
  tblSample->clear();
  tblSample->setColumnCount( 0 );
  tblSample->setRowCount( 0 );

  if ( ! loadDelimitedFileDefinition() )
    return;

  // Put a sample set of records into the sample box.  Also while scanning assess suitability of
  // fields for use as coordinate and WKT fields


  QList<bool> isValidCoordinate;
  QList<bool> isValidWkt;
  QList<bool> isEmpty;
  int counter = 0;
  mBadRowCount = 0;
  QStringList values;
  QRegExp wktre( "^\\s*(?:MULTI)?(?:POINT|LINESTRING|POLYGON)\\s*Z?\\s*M?\\(", Qt::CaseInsensitive );

  while ( counter < mExampleRowCount )
  {
    QgsDelimitedTextFile::Status status = mFile->nextRecord( values );
    if ( status == QgsDelimitedTextFile::RecordEOF ) break;
    if ( status != QgsDelimitedTextFile::RecordOk ) { mBadRowCount++; continue; }
    counter++;

    // Look at count of non-blank fields

    int nv = values.size();
    while ( nv > 0 && values[nv-1].isEmpty() ) nv--;

    if ( isEmpty.size() < nv )
    {
      while ( isEmpty.size() < nv )
      {
        isEmpty.append( true );
        isValidCoordinate.append( false );
        isValidWkt.append( false );
      }
      tblSample->setColumnCount( nv );
    }

    tblSample->setRowCount( counter );

    bool xyDms = cbxXyDms->isChecked();

    for ( int i = 0; i < tblSample->columnCount(); i++ )
    {
      QString value = i < nv ? values[i] : "";
      if ( value.length() > MAX_SAMPLE_LENGTH ) value = value.mid( 0, MAX_SAMPLE_LENGTH ) + "...";
      QTableWidgetItem *item = new QTableWidgetItem( value );
      tblSample->setItem( counter - 1, i, item );
      if ( ! value.isEmpty() )
      {
        if ( isEmpty[i] )
        {
          isEmpty[i] = false;
          isValidCoordinate[i] = true;
          isValidWkt[i] = true;
        }
        if ( isValidCoordinate[i] )
        {
          bool ok = true;
          if ( cbxPointIsComma->isChecked() )
          {
            value.replace( ",", "." );
          }
          if ( xyDms )
          {
            ok = QgsDelimitedTextProvider::CrdDmsRegexp.indexIn( value ) == 0;
          }
          else
          {
            value.toDouble( &ok );
          }
          isValidCoordinate[i] = ok;
        }
        if ( isValidWkt[i] )
        {
          value.remove( QgsDelimitedTextProvider::WktPrefixRegexp );
          isValidWkt[i] = value.contains( wktre );
        }
      }
    }
  }

  QStringList fieldList = mFile->fieldNames();

  if ( isEmpty.size() < fieldList.size() )
  {
    while ( isEmpty.size() < fieldList.size() )
    {
      isEmpty.append( true );
      isValidCoordinate.append( false );
      isValidWkt.append( false );
    }
    tblSample->setColumnCount( fieldList.size() );
  }

  tblSample->setHorizontalHeaderLabels( fieldList );
  tblSample->resizeColumnsToContents();
  tblSample->resizeRowsToContents();

  // We don't know anything about a text based field other
  // than its name. All fields are assumed to be text
  // As we ignore blank fields we need to map original index
  // of selected fields to index in combo box.

  int fieldNo = 0;
  for ( int i = 0; i < fieldList.size(); i++ )
  {
    QString field = fieldList[i];
    // skip empty field names
    if ( field.isEmpty() ) continue;
    cmbXField->addItem( field );
    cmbYField->addItem( field );
    cmbWktField->addItem( field );
    fieldNo++;
  }

  // Try resetting current values for column names

  cmbWktField->setCurrentIndex( cmbWktField->findText( columnWkt ) );
  cmbXField->setCurrentIndex( cmbXField->findText( columnX ) );
  cmbYField->setCurrentIndex( cmbYField->findText( columnY ) );

  // Now try setting optional X,Y fields - will only reset the fields if
  // not already set.

  trySetXYField( fieldList, isValidCoordinate, "longitude", "latitude" );
  trySetXYField( fieldList, isValidCoordinate, "lon", "lat" );
  trySetXYField( fieldList, isValidCoordinate, "east", "north" );
  trySetXYField( fieldList, isValidCoordinate, "x", "y" );
  trySetXYField( fieldList, isValidCoordinate, "e", "n" );

  // And also a WKT field if there is one

  if ( cmbWktField->currentIndex() < 0 )
  {
    for ( int i = 0; i < fieldList.size(); i++ )
    {
      if ( ! isValidWkt[i] ) continue;
      int index = cmbWktField->findText( fieldList[i] );
      if ( index >= 0 )
      {
        cmbWktField->setCurrentIndex( index );
        break;
      }
    }
  }

  bool haveFields = fieldNo > 0;

  bool isXY = cmbWktField->currentIndex() < 0 ||
              ( geomTypeXY->isChecked() &&
                ( cmbXField->currentIndex() >= 0 && cmbYField->currentIndex() >= 0 ) );
  geomTypeXY->setChecked( isXY );
  geomTypeWKT->setChecked( ! isXY );
  swGeomType->setCurrentIndex( bgGeomType->checkedId() );

  if ( haveFields )
  {
    connect( cmbXField, SIGNAL( currentIndexChanged( int ) ), this, SLOT( enableAccept() ) );
    connect( cmbYField, SIGNAL( currentIndexChanged( int ) ), this, SLOT( enableAccept() ) );
    connect( cmbWktField, SIGNAL( currentIndexChanged( int ) ), this, SLOT( enableAccept() ) );
    connect( geomTypeXY, SIGNAL( toggled( bool ) ), this, SLOT( enableAccept() ) );
    connect( geomTypeWKT, SIGNAL( toggled( bool ) ), this, SLOT( enableAccept() ) );
    connect( geomTypeNone, SIGNAL( toggled( bool ) ), this, SLOT( enableAccept() ) );
  }

}

bool QgsDelimitedTextSourceSelect::trySetXYField( QStringList &fields, QList<bool> &isValidNumber, QString xname, QString yname )
{
  // If fields already set, then nothing to do
  if ( cmbXField->currentIndex() >= 0 && cmbYField->currentIndex() >= 0 ) return true;

  // Try and find a valid field name matching the x field
  int indexX = -1;
  int indexY = -1;

  for ( int i = 0; i < fields.size(); i++ )
  {
    // Only interested in number fields containing the xname string
    // that are in the X combo box
    if ( ! isValidNumber[i] ) continue;
    if ( ! fields[i].contains( xname, Qt::CaseInsensitive ) ) continue;
    indexX = cmbXField->findText( fields[i] );
    if ( indexX < 0 ) continue;

    // Now look for potential y fields, like xname with x replaced with y
    QString xfield( fields[i] );
    int from = 0;
    while ( true )
    {
      int pos = xfield.indexOf( xname, from, Qt::CaseInsensitive );
      if ( pos < 0 ) break;
      from = pos + 1;
      QString yfield = xfield.mid( 0, pos ) + yname + xfield.mid( pos + xname.size() );
      if ( ! fields.contains( yfield, Qt::CaseInsensitive ) ) continue;
      for ( int iy = 0; iy < fields.size(); iy++ )
      {
        if ( ! isValidNumber[iy] ) continue;
        if ( iy == i ) continue;
        if ( fields[iy].compare( yfield, Qt::CaseInsensitive ) == 0 )
        {
          indexY = cmbYField->findText( fields[iy] );
          break;
        }
      }
      if ( indexY >= 0 ) break;
    }
    if ( indexY >= 0 ) break;
  }
  if ( indexY >= 0 )
  {
    cmbXField->setCurrentIndex( indexX );
    cmbYField->setCurrentIndex( indexY );
  }
  return indexY >= 0;
}

void QgsDelimitedTextSourceSelect::getOpenFileName()
{
  // Get a file to process, starting at the current directory
  // Set inital dir to last used
  QSettings settings;
  QString selectedFilter = settings.value( mPluginKey + "/file_filter", "" ).toString();

  QString s = QFileDialog::getOpenFileName(
                this,
                tr( "Choose a delimited text file to open" ),
                settings.value( mPluginKey + "/text_path", "./" ).toString(),
                tr( "Text files" ) + " (*.txt *.csv *.dat *.wkt);;"
                + tr( "All files" ) + " (* *.*)",
                &selectedFilter
              );
  // set path
  if ( s.isNull() ) return;
  settings.setValue( mPluginKey + "/file_filter", selectedFilter );
  txtFilePath->setText( s );
}

void QgsDelimitedTextSourceSelect::updateFileName()
{
  // put a default layer name in the text entry
  QString filename = txtFilePath->text();
  QFileInfo finfo( filename );
  if ( finfo.exists() )
  {
    QSettings settings;
    settings.setValue( mPluginKey + "/text_path", finfo.path() );
  }
  txtLayerName->setText( finfo.completeBaseName() );
  loadSettingsForFile( filename );
  updateFieldsAndEnable();
}

void QgsDelimitedTextSourceSelect::updateFieldsAndEnable()
{
  updateFieldLists();
  enableAccept();
}

bool QgsDelimitedTextSourceSelect::validate()
{
  // Check that input data is valid - provide a status message if not..

  QString message( "" );
  bool enabled = false;

  if ( txtFilePath->text().trimmed().isEmpty() )
  {
    message = tr( "Please select an input file" );
  }
  else if ( ! QFileInfo( txtFilePath->text() ).exists() )
  {
    message = tr( "File %1 does not exist" ).arg( txtFilePath->text() );
  }
  else if ( txtLayerName->text().isEmpty() )
  {
    message = tr( "Please enter a layer name" );
  }
  else if ( delimiterChars->isChecked() && selectedChars().size() == 0 )
  {
    message = tr( "At least one delimiter character must be specified" );
  }

  if ( message.isEmpty() && delimiterRegexp->isChecked() )
  {
    QRegExp re( txtDelimiterRegexp->text() );
    if ( ! re.isValid() )
    {
      message = tr( "Regular expression is not valid" );
    }
    else if ( re.pattern().startsWith( "^" ) && re.captureCount() == 0 )
    {
      message = tr( "^.. expression needs capture groups" );
    }
    lblRegexpError->setText( message );
  }
  if ( ! message.isEmpty() )
  {
    // continue...
  }
  // Hopefully won't hit this none-specific message, but just in case ...
  else if ( ! mFile->isValid() )
  {
    message = tr( "Definition of filename and delimiters is not valid" );
  }
  // Assume that the sample table will have been populated if data was found
  else if ( tblSample->rowCount() == 0 )
  {
    message = tr( "No data found in file" );
    if ( mBadRowCount > 0 )
    {
      message = message + " (" + tr( "%1 badly formatted records discarded" ).arg( mBadRowCount ) + ")";
    }
  }
  else if ( geomTypeXY->isChecked() && ( cmbXField->currentText().isEmpty()  || cmbYField->currentText().isEmpty() ) )
  {
    message = tr( "X and Y field names must be selected" );
  }
  else if ( geomTypeXY->isChecked() && ( cmbXField->currentText() == cmbYField->currentText() ) )
  {
    message = tr( "X and Y field names cannot be the same" );
  }
  else if ( geomTypeWKT->isChecked() && cmbWktField->currentText().isEmpty() )
  {
    message = tr( "The WKT field name must be selected" );
  }
  else
  {
    enabled = true;
    if ( mBadRowCount > 0 )
    {
      message = tr( "%1 badly formatted records discarded from sample data" ).arg( mBadRowCount );
    }

  }
  lblStatus->setText( message );
  return enabled;
}

void QgsDelimitedTextSourceSelect::enableAccept()
{
  bool enabled = validate();
  buttonBox->button( QDialogButtonBox::Ok )->setEnabled( enabled );
}
