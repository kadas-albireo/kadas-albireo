/***************************************************************************
    qgsvbsvectoridentify.cpp
    ------------------------
    begin                : March 2018
    copyright            : (C) 2018 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "qgisapp.h"
#include "qgscrscache.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgsgeometryrubberband.h"
#include "qgsmapcanvas.h"
#include "qgsvbsvectoridentify.h"
#include "qgsvectorlayer.h"


QgsVBSVectorIdentifyResultDialog::QgsVBSVectorIdentifyResultDialog( const QgsVectorLayer* layer, const QgsFeature& feature, QgisApp *app = nullptr )
    : QDialog( app )
{
  setWindowTitle( tr( "Identify results" ) );
  setAttribute( Qt::WA_DeleteOnClose );
  setLayout( new QVBoxLayout() );
  resize( 480, 480 );

  QTreeWidget* treeWidget = new QTreeWidget( this );
  treeWidget->setColumnCount( 2 );
  treeWidget->header()->setStretchLastSection( true );
  treeWidget->header()->resizeSection( 0, 200 );
  treeWidget->setHeaderLabels( QStringList() << tr( "Attribute" ) << tr( "Value" ) );
  treeWidget->setDropIndicatorShown( false );
  treeWidget->setVerticalScrollMode( QTreeWidget::ScrollPerPixel );
  layout()->addWidget( treeWidget );
  QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Close, Qt::Horizontal, this );
  connect( bbox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( bbox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  layout()->addWidget( bbox );

  if ( layer->type() == QgsMapLayer::RedliningLayer )
  {
    QStringList attributes = feature.attribute( "attributes" ).toString().split( QRegExp( "&(?!amp;)" ) );
  for ( const QString& attribute : attributes )
    {
      QStringList pair = attribute.split( "=" );
      treeWidget->invisibleRootItem()->addChild( new QTreeWidgetItem( pair ) );
    }
  }
  else
  {
    QgsAttributes attributes = feature.attributes();
    for ( int i = 0, n = attributes.size(); i < n; ++i )
    {
      QString attribName = layer->attributeDisplayName( i );
      QStringList pair = QStringList() << attribName << attributes.at( i ).toString();
      treeWidget->invisibleRootItem()->addChild( new QTreeWidgetItem( pair ) );
    }
  }

  treeWidget->expandAll();

  QgsMapCanvas* canvas = app->mapCanvas();

  QgsAbstractGeometryV2* geometry = feature.geometry()->geometry()->clone();
  geometry->transform( *QgsCoordinateTransformCache::instance()->transform( layer->crs().authid(), canvas->mapSettings().destinationCrs().authid() ) );
  mRubberBand = new QgsGeometryRubberBand( canvas, feature.geometry()->type() );
  mRubberBand->setGeometry( geometry );
  mRubberBand->setIconType( QgsGeometryRubberBand::ICON_NONE );
}

QgsVBSVectorIdentifyResultDialog::~QgsVBSVectorIdentifyResultDialog()
{
  delete mRubberBand.data();
}
