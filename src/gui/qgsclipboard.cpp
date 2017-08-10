/***************************************************************************
     qgsclipboard.cpp  -  QGIS internal clipboard for storage of features
     --------------------------------------------------------------------
    begin                : 20 May, 2005
    copyright            : (C) 2005 by Brendan Morley
    email                : morb at ozemail dot com dot au
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QApplication>
#include <QClipboard>
#include <QDataStream>
#include <QMimeData>
#include <QSettings>

#include "qgsclipboard.h"
#include "qgsfeaturestore.h"
#include "qgsgeometry.h"

QgsClipboard::QgsClipboard( QObject* parent )
    : QObject( parent )
{
  connect( QApplication::clipboard(), SIGNAL( dataChanged() ), this, SIGNAL( dataChanged() ) );
}

void QgsClipboard::setData( QMimeData* mimeData )
{
  QApplication::clipboard()->setMimeData( mimeData );
  mFeatureStore = QgsFeatureStore();
}

const QMimeData* QgsClipboard::data()
{
  return QApplication::clipboard()->mimeData();
}

bool QgsClipboard::hasFormat( const QString& format ) const
{
  if ( format == QGSCLIPBOARD_FEATURESTORE_MIME )
  {
    return !mFeatureStore.features().isEmpty();
  }
  const QMimeData* mimeData = QApplication::clipboard()->mimeData();
  return mimeData && mimeData->hasFormat( format );
}

void QgsClipboard::setStoredFeatures( const QgsFeatureStore &featureStore )
{
  mFeatureStore = featureStore;

  // Also store plaintext version
  QSettings settings;
  bool copyWKT = settings.value( "qgis/copyGeometryAsWKT", true ).toBool();

  QStringList textLines;
  QStringList textFields;

  // first column names
  if ( copyWKT )
  {
    textFields += "wkt_geom";
  }
  for ( int i = 0, n = featureStore.fields().count(); i < n; ++i )
  {
    textFields.append( featureStore.fields().at( i ).name() );
  }
  textLines.append( textFields.join( "\t" ) );
  textFields.clear();

  // then the field contents
  foreach ( const QgsFeature& feature, featureStore.features() )
  {
    if ( copyWKT )
    {
      if ( feature.geometry() )
        textFields.append( feature.geometry()->exportToWkt() );
      else
        textFields.append( settings.value( "qgis/nullValue", "NULL" ).toString() );
    }
    foreach ( const QVariant& attr, feature.attributes() )
    {
      textFields.append( attr.toString() );
    }
    textLines.append( textFields.join( "\t" ) );
    textFields.clear();
  }
  QMimeData* mimeData = new QMimeData();
  mimeData->setData( "text/plain", textLines.join( "\n" ).toLocal8Bit() );
  QApplication::clipboard()->setMimeData( mimeData );
}

const QgsFeatureStore &QgsClipboard::getStoredFeatures() const
{
  return mFeatureStore;
}
