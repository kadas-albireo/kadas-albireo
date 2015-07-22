/***************************************************************************
                              qgsoverlayitem.h
                              ----------------
  begin                : July 21, 2015
  copyright            : (C) 2015 by Sandro Mani
  email                : smani at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSOVERLAYITEM_H
#define QGSOVERLAYITEM_H

#include "qgsmapcanvasitem.h"
#include "qgscoordinatereferencesystem.h"

class QgsOverlayItem : public QObject, public QgsMapCanvasItem
{
    Q_OBJECT
  public:
    QgsOverlayItem( QgsMapCanvas* mapCanvas );
    ~QgsOverlayItem();

    /** Set the position of the item to a fixed geographical position
     * @param pos The geographical position
     * @param crs The crs of the point
     */
    void setGeoPosition( const QgsPoint& pos, const QgsCoordinateReferenceSystem& crs );

    /** Sets the position of the item to a fixed canvas position
     * @param pos The position in canvas pixel coordinates
     */
    void setCanvasPosition( const QgsPoint& pos );

    /**
     * @return The bounding box of the item in canvas coordinates
     */
    QRectF boundingRect() const override { return mBoundingRect; }

    void writeXML( QDomDocument& doc, QDomElement& parentItem ) const;
    void readXML( const QDomElement& parentItem );

  protected:
    QgsCoordinateReferenceSystem mPosCrs;
    QgsPoint mPos;
    QPointF mCanvasPos;
    QRectF mBoundingRect;

    QPointF getCanvasCoordinates() const;
    virtual void updateBoundingRect() = 0;

  private slots:
    void updateCanvasPosition();
};

#endif // QGSOVERLAYITEM_H
