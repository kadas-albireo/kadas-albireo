/***************************************************************************
      qgsclipboard.h  -  QGIS internal clipboard for storage of features
      ------------------------------------------------------------------
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

#ifndef QGSCLIPBOARD_H
#define QGSCLIPBOARD_H

#include <QObject>
#include "qgsfeaturestore.h"

class QMimeData;
class QgsPoint;

class GUI_EXPORT QgsPasteHandler
{
  public:
    virtual ~QgsPasteHandler() {}
    virtual void paste( const QString& mimeData, const QByteArray& data, const QgsPoint* mapPos ) = 0;
};

/*
 * Constants used to describe copy-paste MIME types
 */
#define QGSCLIPBOARD_STYLE_MIME "application/qgis.style"
#define QGSCLIPBOARD_FEATURESTORE_MIME "application/qgis.featurestore"

class GUI_EXPORT QgsClipboard : public QObject
{
    Q_OBJECT
  public:
    QgsClipboard( QObject* parent = 0 );

    // Returns whether there is any data in the clipboard
    bool isEmpty() const;

    // Queries whether the clipboard has specified format.
    bool hasFormat( const QString& format ) const;

    // Sets the clipboard contents
    void setMimeData( QMimeData* mimeData );

    // Retreives the clipboard contents
    const QMimeData* mimeData();

    // Utility function for storing features in clipboard
    void setStoredFeatures( const QgsFeatureStore& featureStore );

    // Utility function for getting features from clipboard
    const QgsFeatureStore& getStoredFeatures() const;

  signals:
    void dataChanged();

  private:
    QgsFeatureStore mFeatureStore;

  private slots:
    void onDataChanged();
};

#endif
