/***************************************************************************
 *  qgskmlexportdialog.h                                                   *
 *  -----------                                                            *
 *  begin                : October 2015                                    *
 *  copyright            : (C) 2015 by Marco Hugentobler / Sourcepole AG   *
 *  email                : marco@sourcepole.ch                             *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "layertree/qgslayertreegroup.h"
#include "layertree/qgslayertreelayer.h"
#include "qgskmlexportdialog.h"
#include "qgsmaplayerregistry.h"
#include "qgsproject.h"

#include <QPushButton>
#include <QFileDialog>
#include <QSettings>

QgsKMLExportDialog::QgsKMLExportDialog( const QList<QgsMapLayer*> &activeLayers, QWidget *parent, Qt::WindowFlags f )
    : QDialog( parent, f )
{
  setupUi( this );

  // Use layerTreeRoot to get layers ordered as in the layer tree
  foreach ( QgsLayerTreeLayer* layerTreeLayer, QgsProject::instance()->layerTreeRoot()->findLayers() )
  {
    QgsMapLayer* layer = layerTreeLayer->layer();
    if ( !layer )
      continue;
    QListWidgetItem* item = new QListWidgetItem( layer->name() );
    item->setCheckState( layer->source().contains( "url=http" ) || !activeLayers.contains( layer ) ? Qt::Unchecked : Qt::Checked );
    item->setData( Qt::UserRole, layer->id() );
    mLayerListWidget->addItem( item );
  }
  mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( false );

  connect( mFileSelectionButton, SIGNAL( clicked() ), this, SLOT( selectFile() ) );
}

void QgsKMLExportDialog::selectFile()
{
  QStringList filters;
  filters.append( tr( "KMZ File (*.kmz)" ) );
  filters.append( tr( "KML File (*.kml)" ) );

  QString lastDir = QSettings().value( "/UI/lastImportExportDir", "." ).toString();
  QString selectedFilter;

  QString filename = QFileDialog::getSaveFileName( 0, tr( "Select Output" ), lastDir, filters.join( ";;" ), &selectedFilter );
  if ( filename.isEmpty() )
  {
    return;
  }
  QSettings().setValue( "/UI/lastImportExportDir", QFileInfo( filename ).absolutePath() );
  if ( selectedFilter == filters[0] && !filename.endsWith( ".kmz", Qt::CaseInsensitive ) )
  {
    filename += ".kmz";
  }
  else if ( selectedFilter == filters[1] && !filename.endsWith( ".kml", Qt::CaseInsensitive ) )
  {
    filename += ".kml";
  }
  mFileLineEdit->setText( filename );
  mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( true );

  // Toggle sensitivity of layers depending on whether KML or KMZ is selected
  if ( selectedFilter == filters[0] )
  {
    // KMZ, enable all layers
    for ( int i = 0, n = mLayerListWidget->count(); i < n; ++i )
    {
      mLayerListWidget->item( i )->setFlags( Qt::ItemIsUserCheckable | Qt::ItemIsEnabled );
    }
    mAnnotationsCheckBox->setEnabled( true );
  }
  else if ( selectedFilter == filters[1] )
  {
    // KML, disable non-vector layers and annotations
    for ( int i = 0, n = mLayerListWidget->count(); i < n; ++i )
    {
      QString layerId = mLayerListWidget->item( i )->data( Qt::UserRole ).toString();
      QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerId );
      if ( layer && layer->type() != QgsMapLayer::VectorLayer )
      {
        mLayerListWidget->item( i )->setFlags( Qt::NoItemFlags );
        mLayerListWidget->item( i )->setCheckState( Qt::Unchecked );
      }
    }
    mAnnotationsCheckBox->setChecked( false );
    mAnnotationsCheckBox->setEnabled( false );
  }
}

QList<QgsMapLayer*> QgsKMLExportDialog::getSelectedLayers() const
{
  QList<QgsMapLayer*> layerList;
  for ( int i = 0, n = mLayerListWidget->count(); i < n; ++i )
  {
    QListWidgetItem* item = mLayerListWidget->item( i );
    if (( item->flags() & Qt::ItemIsEnabled ) && item->checkState() == Qt::Checked )
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
