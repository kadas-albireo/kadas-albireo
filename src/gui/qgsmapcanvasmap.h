/***************************************************************************
    qgsmapcanvasmap.h  -  draws the map in map canvas
    ----------------------
    begin                : February 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPCANVASMAP_H
#define QGSMAPCANVASMAP_H

#include <QGraphicsRectItem>
#include <QPixmap>
#include <QTimer>

#include <qgis.h>
#include <qgsmaptopixel.h>

#include <qgsmapcanvasitem.h>

class QgsMapSettings;
class QgsMapCanvas;

/** \ingroup gui
 * A rectangular graphics item representing the map on the canvas.
 */
class GUI_EXPORT QgsMapCanvasMap : public QgsMapCanvasItem  // public QObject, public QGraphicsRectItem
{
  public:

    //! constructor
    QgsMapCanvasMap( QgsMapCanvas* canvas );

    ~QgsMapCanvasMap();

    //! @note added in 2.1
    void setContent( const QImage& image, const QgsRectangle& rect );

    //! @note added in 2.1
    QImage contentImage() const { return mImage; }

    virtual void paint( QPainter * painter );

    //! @deprecated in 2.1 - does nothing. Kept for API compatibility
    Q_DECL_DEPRECATED void refresh() {}

    //! @deprecated in 2.1 - does nothing. Kept for API compatibility
    Q_DECL_DEPRECATED void resize( QSize size ) { Q_UNUSED( size ); }

    //! @deprecated in 2.1 - does nothing. Kept for API compatibility
    Q_DECL_DEPRECATED void enableAntiAliasing( bool flag );

    //! @deprecated in 2.1 - does nothing. Kept for API compatibility
    Q_DECL_DEPRECATED void render() {}

    //! @deprecated in 2.1 - does nothing. Kept for API compatibility
    Q_DECL_DEPRECATED void setBackgroundColor( const QColor& color ) { Q_UNUSED( color ); }

    //! @deprecated in 2.1 - not called by QgsMapCanvas anymore
    Q_DECL_DEPRECATED void setPanningOffset( const QPoint& point ) { Q_UNUSED( point ); }

    //! @deprecated in 2.1
    Q_DECL_DEPRECATED QPaintDevice& paintDevice();

    //! @deprecated in 2.1 - does nothing. Kept for API compatibility
    Q_DECL_DEPRECATED void updateContents() {}

  private:

    QImage mImage;
};

#endif
