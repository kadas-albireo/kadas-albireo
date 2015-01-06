/***************************************************************************
                         qgsnewvectorlayerdialog.cpp  -  description
                             -------------------
    begin                : October 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsnewvectorlayerdialog.h"
#include "qgsapplication.h"
#include "qgis.h"
#include "qgslogger.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsproviderregistry.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorfilewriter.h"

#include <QPushButton>
#include <QComboBox>
#include <QLibrary>
#include <QSettings>
#include <QFileDialog>


QgsNewVectorLayerDialog::QgsNewVectorLayerDialog( QWidget *parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
  setupUi( this );

  QSettings settings;
  restoreGeometry( settings.value( "/Windows/NewVectorLayer/geometry" ).toByteArray() );

  mAddAttributeButton->setIcon( QgsApplication::getThemeIcon( "/mActionNewAttribute.png" ) );
  mRemoveAttributeButton->setIcon( QgsApplication::getThemeIcon( "/mActionDeleteAttribute.png" ) );
  mTypeBox->addItem( tr( "Text data" ), "String" );
  mTypeBox->addItem( tr( "Whole number" ), "Integer" );
  mTypeBox->addItem( tr( "Decimal number" ), "Real" );
  mTypeBox->addItem( tr( "Date" ), "Date" );

  mWidth->setValidator( new QIntValidator( 1, 255, this ) );
  mPrecision->setValidator( new QIntValidator( 0, 15, this ) );

  mPointRadioButton->setChecked( true );
  mFileFormatComboBox->addItem( tr( "ESRI Shapefile" ), "ESRI Shapefile" );
#if 0
  // Disabled until provider properly supports editing the created file formats
  mFileFormatComboBox->addItem( tr( "Comma Separated Value" ), "Comma Separated Value" );
  mFileFormatComboBox->addItem( tr( "GML" ), "GML" );
  mFileFormatComboBox->addItem( tr( "Mapinfo File" ), "Mapinfo File" );
#endif
  if ( mFileFormatComboBox->count() == 1 )
  {
    mFileFormatComboBox->setVisible( false );
    mFileFormatLabel->setVisible( false );
  }

  mFileFormatComboBox->setCurrentIndex( 0 );

  mFileEncoding->addItems( QgsVectorDataProvider::availableEncodings() );

  // Use default encoding if none supplied
  QString enc = QSettings().value( "/UI/encoding", "System" ).toString();

  // The specified decoding is added if not existing alread, and then set current.
  // This should select it.
  int encindex = mFileEncoding->findText( enc );
  if ( encindex < 0 )
  {
    mFileEncoding->insertItem( 0, enc );
    encindex = 0;
  }
  mFileEncoding->setCurrentIndex( encindex );

  mOkButton = buttonBox->button( QDialogButtonBox::Ok );

  mAttributeView->addTopLevelItem( new QTreeWidgetItem( QStringList() << "id" << "Integer" << "10" << "" ) );

  QgsCoordinateReferenceSystem defaultCrs;
  defaultCrs.createFromOgcWmsCrs( settings.value( "/Projections/layerDefaultCrs", GEO_EPSG_CRS_AUTHID ).toString() );
  defaultCrs.validate();
  mCrsSelector->setCrs( defaultCrs );

  connect( mNameEdit, SIGNAL( textChanged( QString ) ), this, SLOT( nameChanged( QString ) ) );
  connect( mAttributeView, SIGNAL( itemSelectionChanged() ), this, SLOT( selectionChanged() ) );

  mAddAttributeButton->setEnabled( false );
  mRemoveAttributeButton->setEnabled( false );
}

QgsNewVectorLayerDialog::~QgsNewVectorLayerDialog()
{
  QSettings settings;
  settings.setValue( "/Windows/NewVectorLayer/geometry", saveGeometry() );
}

void QgsNewVectorLayerDialog::on_mFileFormatComboBox_currentIndexChanged( int index )
{
  Q_UNUSED( index );
  if ( mFileFormatComboBox->currentText() == tr( "ESRI Shapefile" ) )
    mNameEdit->setMaxLength( 10 );
  else
    mNameEdit->setMaxLength( 32767 );
}

void QgsNewVectorLayerDialog::on_mTypeBox_currentIndexChanged( int index )
{
  // FIXME: sync with providers/ogr/qgsogrprovider.cpp
  switch ( index )
  {
    case 0: // Text data
      if ( mWidth->text().toInt() < 1 || mWidth->text().toInt() > 255 )
        mWidth->setText( "80" );
      mPrecision->setEnabled( false );
      mWidth->setValidator( new QIntValidator( 1, 255, this ) );
      break;

    case 1: // Whole number
      if ( mWidth->text().toInt() < 1 || mWidth->text().toInt() > 10 )
        mWidth->setText( "10" );
      mPrecision->setEnabled( false );
      mWidth->setValidator( new QIntValidator( 1, 10, this ) );
      break;

    case 2: // Decimal number
      if ( mWidth->text().toInt() < 1 || mWidth->text().toInt() > 20 )
        mWidth->setText( "20" );
      mPrecision->setEnabled( true );
      mWidth->setValidator( new QIntValidator( 1, 20, this ) );
      break;

    default:
      QgsDebugMsg( "unexpected index" );
      break;
  }
}

QGis::WkbType QgsNewVectorLayerDialog::selectedType() const
{
  if ( mPointRadioButton->isChecked() )
  {
    return QGis::WKBPoint;
  }
  else if ( mLineRadioButton->isChecked() )
  {
    return QGis::WKBLineString;
  }
  else if ( mPolygonRadioButton->isChecked() )
  {
    return QGis::WKBPolygon;
  }
  return QGis::WKBUnknown;
}

int QgsNewVectorLayerDialog::selectedCrsId() const
{
  return mCrsSelector->crs().srsid();
}

void QgsNewVectorLayerDialog::on_mAddAttributeButton_clicked()
{
  QString myName = mNameEdit->text();
  QString myWidth = mWidth->text();
  QString myPrecision = mPrecision->isEnabled() ? mPrecision->text() : "";
  //use userrole to avoid translated type string
  QString myType = mTypeBox->itemData( mTypeBox->currentIndex(), Qt::UserRole ).toString();
  mAttributeView->addTopLevelItem( new QTreeWidgetItem( QStringList() << myName << myType << myWidth << myPrecision ) );
  if ( mAttributeView->topLevelItemCount() > 0 )
  {
    mOkButton->setEnabled( true );
  }
  mNameEdit->clear();
}

void QgsNewVectorLayerDialog::on_mRemoveAttributeButton_clicked()
{
  delete mAttributeView->currentItem();
  if ( mAttributeView->topLevelItemCount() == 0 )
  {
    mOkButton->setEnabled( false );
  }
}

void QgsNewVectorLayerDialog::attributes( QList< QPair<QString, QString> >& at ) const
{
  QTreeWidgetItemIterator it( mAttributeView );
  while ( *it )
  {
    QTreeWidgetItem *item = *it;
    QString type = QString( "%1;%2;%3" ).arg( item->text( 1 ) ).arg( item->text( 2 ) ).arg( item->text( 3 ) );
    at.push_back( qMakePair( item->text( 0 ), type ) );
    QgsDebugMsg( QString( "appending %1//%2" ).arg( item->text( 0 ) ).arg( type ) );
    ++it;
  }
}

QString QgsNewVectorLayerDialog::selectedFileFormat() const
{
  //use userrole to avoid translated type string
  QString myType = mFileFormatComboBox->itemData( mFileFormatComboBox->currentIndex(), Qt::UserRole ).toString();
  return myType;
}

QString QgsNewVectorLayerDialog::selectedFileEncoding() const
{
  return mFileEncoding->currentText();
}

void QgsNewVectorLayerDialog::nameChanged( QString name )
{
  mAddAttributeButton->setDisabled( name.isEmpty() || mAttributeView->findItems( name, Qt::MatchExactly ).size() > 0 );
}

void QgsNewVectorLayerDialog::selectionChanged()
{
  mRemoveAttributeButton->setDisabled( mAttributeView->selectedItems().size() == 0 );
}


// this is static
QString QgsNewVectorLayerDialog::runAndCreateLayer( QWidget* parent, QString* pEnc )
{
  QgsNewVectorLayerDialog geomDialog( parent );
  if ( geomDialog.exec() == QDialog::Rejected )
  {
    return "";
  }

  QGis::WkbType geometrytype = geomDialog.selectedType();
  QString fileformat = geomDialog.selectedFileFormat();
  QString enc = geomDialog.selectedFileEncoding();
  int crsId = geomDialog.selectedCrsId();
  QgsDebugMsg( QString( "New file format will be: %1" ).arg( fileformat ) );

  QList< QPair<QString, QString> > attributes;
  geomDialog.attributes( attributes );

  QSettings settings;
  QString lastUsedDir = settings.value( "/UI/lastVectorFileFilterDir", "." ).toString();
  QString filterString = QgsVectorFileWriter::filterForDriver( fileformat );
  QString fileName = QFileDialog::getSaveFileName( 0, tr( "Save layer as..." ), lastUsedDir, filterString );
  if ( fileName.isNull() )
  {
    return "";
  }

  if ( fileformat == "ESRI Shapefile" && !fileName.endsWith( ".shp", Qt::CaseInsensitive ) )
    fileName += ".shp";

  settings.setValue( "/UI/lastVectorFileFilterDir", QFileInfo( fileName ).absolutePath() );
  settings.setValue( "/UI/encoding", enc );

  //try to create the new layer with OGRProvider instead of QgsVectorFileWriter
  QgsProviderRegistry * pReg = QgsProviderRegistry::instance();
  QString ogrlib = pReg->library( "ogr" );
  // load the data provider
  QLibrary* myLib = new QLibrary( ogrlib );
  bool loaded = myLib->load();
  if ( loaded )
  {
    QgsDebugMsg( "ogr provider loaded" );

    typedef bool ( *createEmptyDataSourceProc )( const QString&, const QString&, const QString&, QGis::WkbType,
        const QList< QPair<QString, QString> >&, const QgsCoordinateReferenceSystem * );
    createEmptyDataSourceProc createEmptyDataSource = ( createEmptyDataSourceProc ) cast_to_fptr( myLib->resolve( "createEmptyDataSource" ) );
    if ( createEmptyDataSource )
    {
      if ( geometrytype != QGis::WKBUnknown )
      {
        QgsCoordinateReferenceSystem srs( crsId, QgsCoordinateReferenceSystem::InternalCrsId );
        if ( !createEmptyDataSource( fileName, fileformat, enc, geometrytype, attributes, &srs ) )
        {
          return QString::null;
        }
      }
      else
      {
        QgsDebugMsg( "geometry type not recognised" );
        return QString::null;
      }
    }
    else
    {
      QgsDebugMsg( "Resolving newEmptyDataSource(...) failed" );
      return QString::null;
    }
  }

  if ( pEnc )
    *pEnc = enc;

  return fileName;
}
