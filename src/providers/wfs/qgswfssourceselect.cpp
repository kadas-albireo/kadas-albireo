/***************************************************************************
                              qgswfssourceselect.h
                              --------------------
  begin                : Jun 02, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgswfssourceselect.h"
#include "qgswfscapabilities.h"
#include "qgswfsprovider.h"
#include "qgsexpressionbuilderdialog.h"
#include "qgsowsconnection.h"

#include <QMessageBox>
#include <QEventLoop>


QgsWFSSourceSelect::QgsWFSSourceSelect( QWidget* parent, Qt::WindowFlags fl, bool embeddedMode )
    : QgsSourceSelectDialog( "WFS", QgsSourceSelectDialog::FeatureService, parent, fl )
{
  if ( embeddedMode )
  {
    buttonBox->button( QDialogButtonBox::Close )->hide();
  }
}

bool QgsWFSSourceSelect::connectToService( const QgsOWSConnection &connection )
{
  QgsWFSCapabilities capabilities( connection.uri().encodedUri() );

  QEventLoop loop;
  connect( &capabilities, SIGNAL( gotCapabilities() ), &loop, SLOT( quit() ) );
  capabilities.requestCapabilities();
  loop.exec( QEventLoop::ExcludeUserInputEvents );

  QgsWFSCapabilities::ErrorCode err = capabilities.errorCode();
  if ( err != QgsWFSCapabilities::NoError )
  {
    QString title;
    switch ( err )
    {
      case QgsWFSCapabilities::NetworkError: title = tr( "Network Error" ); break;
      case QgsWFSCapabilities::XmlError:     title = tr( "Capabilities document is not valid" ); break;
      case QgsWFSCapabilities::ServerExceptionError: title = tr( "Server Exception" ); break;
      default: tr( "Error" ); break;
    }
    // handle errors
    QMessageBox::critical( 0, title, capabilities.errorMessage() );
    return false;
  }

  QgsWFSCapabilities::GetCapabilities caps = capabilities.capabilities();
  foreach ( QgsWFSCapabilities::FeatureType featureType, caps.featureTypes )
  {
    // insert the typenames, titles and abstracts into the tree view
    QStandardItem* titleItem = new QStandardItem( featureType.title );
    QStandardItem* nameItem = new QStandardItem( featureType.name );
    QStandardItem* abstractItem = new QStandardItem( featureType.abstract );
    abstractItem->setToolTip( "<font color=black>" + featureType.abstract  + "</font>" );
    abstractItem->setTextAlignment( Qt::AlignLeft | Qt::AlignTop );
    QStandardItem* cachedItem = new QStandardItem();
    QStandardItem* filterItem = new QStandardItem();
    cachedItem->setCheckable( true );
    cachedItem->setCheckState( Qt::Checked );

    mModel->appendRow( QList<QStandardItem*>() << titleItem << nameItem << abstractItem << cachedItem << filterItem );

    mAvailableCRS[featureType.name] = featureType.crslist;
  }
  return true;
}

void QgsWFSSourceSelect::buildQuery( const QgsOWSConnection &connection, const QModelIndex& index )
{
  if ( !index.isValid() )
  {
    return;
  }
  QModelIndex filterIndex = index.sibling( index.row(), 4 );
  QString typeName = index.sibling( index.row(), 1 ).data().toString();

  //get available fields for wfs layer
  QgsWFSProvider p( "" );  //bypasses most provider instantiation logic
  QgsWFSCapabilities conn( connection.uri().encodedUri() );
  QString uri = conn.uriDescribeFeatureType( typeName );

  QgsFields fields;
  QString geometryAttribute;
  QGis::WkbType geomType;
  if ( p.describeFeatureType( uri, geometryAttribute, fields, geomType ) != 0 )
  {
    return;
  }

  //show expression builder
  QgsExpressionBuilderDialog d( 0, filterIndex.data().toString() );

  //add available attributes to expression builder
  QgsExpressionBuilderWidget* w = d.expressionBuilder();
  w->loadFieldNames( fields );

  if ( d.exec() == QDialog::Accepted )
  {
    QgsDebugMsg( "Expression text = " + w->expressionText() );
    mModelProxy->setData( filterIndex, QVariant( w->expressionText() ) );
  }
}

QString QgsWFSSourceSelect::getLayerURI( const QgsOWSConnection& connection,
    const QString &/*layerTitle*/, const QString& layerName,
    const QString& crs,
    const QString& filter,
    const QgsRectangle& bBox ) const
{
  QgsWFSCapabilities conn( connection.uri().encodedUri() );
  return conn.uriGetFeature( layerName, crs, filter, bBox );
}
