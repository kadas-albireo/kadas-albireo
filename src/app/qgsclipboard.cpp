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

#include <fstream>

#include <QApplication>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QClipboard>
#include <QSettings>
#include <QMimeData>

#include "qgsclipboard.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsgeometry.h"
#include "qgscoordinatereferencesystem.h"
#include "qgslogger.h"
#include "qgsvectorlayer.h"

QgsClipboard::QgsClipboard()
    : QObject()
    , mFeatureClipboard()
    , mFeatureFields()
    , mUseSystemClipboard( false )
{
  connect( QApplication::clipboard(), SIGNAL( dataChanged() ), this, SLOT( systemClipboardChanged() ) );
}

QgsClipboard::~QgsClipboard()
{
}

void QgsClipboard::replaceWithCopyOf( QgsVectorLayer *src )
{
  if ( !src )
    return;

  // Replace the QGis clipboard.
  mFeatureFields = src->pendingFields();
  mFeatureClipboard = src->selectedFeatures();
  mCRS = src->crs();

  QgsDebugMsg( "replaced QGis clipboard." );

  setSystemClipboard();
  mUseSystemClipboard = false;
  emit changed();
}

void QgsClipboard::replaceWithCopyOf( QgsFeatureStore & featureStore )
{
  QgsDebugMsg( QString( "features count = %1" ).arg( featureStore.features().size() ) );
  mFeatureFields = featureStore.fields();
  mFeatureClipboard = featureStore.features();
  mCRS = featureStore.crs();
  setSystemClipboard();
  mUseSystemClipboard = false;
  emit changed();
}

void QgsClipboard::setSystemClipboard()
{
  // Replace the system clipboard.
  QSettings settings;
  bool copyWKT = settings.value( "qgis/copyGeometryAsWKT", true ).toBool();

  QStringList textLines;
  QStringList textFields;

  // first do the field names
  if ( copyWKT )
  {
    textFields += "wkt_geom";
  }

  for ( int idx = 0; idx < mFeatureFields.count(); ++idx )
  {
    textFields += mFeatureFields[idx].name();
  }
  textLines += textFields.join( "\t" );
  textFields.clear();

  // then the field contents
  for ( QgsFeatureList::iterator it = mFeatureClipboard.begin(); it != mFeatureClipboard.end(); ++it )
  {
    const QgsAttributes& attributes = it->attributes();

    // TODO: Set up Paste Transformations to specify the order in which fields are added.
    if ( copyWKT )
    {
      if ( it->geometry() )
        textFields += it->geometry()->exportToWkt();
      else
      {
        textFields += settings.value( "qgis/nullValue", "NULL" ).toString();
      }
    }

    // QgsDebugMsg("about to traverse fields.");
    for ( int idx = 0; idx < attributes.count(); ++idx )
    {
      // QgsDebugMsg(QString("inspecting field '%1'.").arg(it2->toString()));
      textFields += attributes[idx].toString();
    }

    textLines += textFields.join( "\t" );
    textFields.clear();
  }

  QString textCopy = textLines.join( "\n" );

  QClipboard *cb = QApplication::clipboard();

  // Copy text into the clipboard

  // With qgis running under Linux, but with a Windows based X
  // server (Xwin32), ::Selection was necessary to get the data into
  // the Windows clipboard (which seems contrary to the Qt
  // docs). With a Linux X server, ::Clipboard was required.
  // The simple solution was to put the text into both clipboards.

#ifndef Q_OS_WIN
  cb->setText( textCopy, QClipboard::Selection );
#endif
  cb->setText( textCopy, QClipboard::Clipboard );

  QgsDebugMsg( QString( "replaced system clipboard with: %1." ).arg( textCopy ) );
}

QgsFeatureList QgsClipboard::copyOf( const QgsFields &fields )
{
  QgsDebugMsg( "returning clipboard." );
  if ( !mUseSystemClipboard )
    return mFeatureClipboard;

  QClipboard *cb = QApplication::clipboard();

#ifndef Q_OS_WIN
  QString text = cb->text( QClipboard::Selection );
#else
  QString text = cb->text( QClipboard::Clipboard );
#endif

  QStringList values = text.split( "\n" );
  if ( values.isEmpty() || text.isEmpty() )
    return mFeatureClipboard;

  QgsFeatureList features;
  foreach ( QString row, values )
  {
    // Assume that it's just WKT for now.
    QgsGeometry* geometry = QgsGeometry::fromWkt( row );
    if ( !geometry )
      continue;

    QgsFeature feature;
    if ( !fields.isEmpty() )
      feature.setFields( &fields, true );

    feature.setGeometry( geometry );
    features.append( feature );
  }

  if ( features.isEmpty() )
    return mFeatureClipboard;

  if ( !fields.isEmpty() )
    mFeatureFields = fields;

  return features;
}

void QgsClipboard::clear()
{
  mFeatureClipboard.clear();

  QgsDebugMsg( "cleared clipboard." );
  emit changed();
}

void QgsClipboard::insert( QgsFeature& feature )
{
  mFeatureClipboard.push_back( feature );

  QgsDebugMsg( "inserted " + feature.geometry()->exportToWkt() );
  mUseSystemClipboard = false;
  emit changed();
}

bool QgsClipboard::empty()
{
  QClipboard *cb = QApplication::clipboard();
#ifndef Q_OS_WIN
  QString text = cb->text( QClipboard::Selection );
#else
  QString text = cb->text( QClipboard::Clipboard );
#endif
  return text.isEmpty() && mFeatureClipboard.empty();
}

QgsFeatureList QgsClipboard::transformedCopyOf( QgsCoordinateReferenceSystem destCRS, const QgsFields &fields )
{
  QgsFeatureList featureList = copyOf( fields );
  QgsCoordinateTransform ct( crs(), destCRS );

  QgsDebugMsg( "transforming clipboard." );
  for ( QgsFeatureList::iterator iter = featureList.begin(); iter != featureList.end(); ++iter )
  {
    iter->geometry()->transform( ct );
  }

  return featureList;
}

QgsCoordinateReferenceSystem QgsClipboard::crs()
{
  return mCRS;
}

void QgsClipboard::setData( const QString& mimeType, const QByteArray& data, const QString* text )
{
  QMimeData *mdata = new QMimeData();
  mdata->setData( mimeType, data );
  if ( text )
  {
    mdata->setText( *text );
  }
  // Transfers ownership to the clipboard object
#ifndef Q_OS_WIN
  QApplication::clipboard()->setMimeData( mdata, QClipboard::Selection );
#endif
  QApplication::clipboard()->setMimeData( mdata, QClipboard::Clipboard );
}

void QgsClipboard::setData( const QString& mimeType, const QByteArray& data, const QString& text )
{
  setData( mimeType, data, &text );
}

void QgsClipboard::setData( const QString& mimeType, const QByteArray& data )
{
  setData( mimeType, data, 0 );
}

bool QgsClipboard::hasFormat( const QString& mimeType )
{
  return QApplication::clipboard()->mimeData()->hasFormat( mimeType );
}

QByteArray QgsClipboard::data( const QString& mimeType )
{
  return QApplication::clipboard()->mimeData()->data( mimeType );
}

void QgsClipboard::systemClipboardChanged()
{
  mUseSystemClipboard = true;
  emit changed();
}
