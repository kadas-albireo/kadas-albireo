/***************************************************************************
                              qgsvbsmilixannotationitem.h
                              ---------------------------
  begin                : Oct 01, 2015
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

#ifndef QGSVBSMILIXANNOTATIONITEM_H
#define QGSVBSMILIXANNOTATIONITEM_H

#include "qgsannotationitem.h"
#include "Client/VBSMilixClient.hpp"

class QgsVBSCoordinateDisplayer;

class QgsVBSMilixAnnotationItem : public QgsAnnotationItem
{
    QGS_ANNOTATION_ITEM( QgsVBSMilixAnnotationItem, "MilixAnnotationItem" )
  public:

    QgsVBSMilixAnnotationItem( QgsMapCanvas* canvas );
    void setSymbolXml( const QString& symbolXml , bool hasVariablePointCount );
    const QString& symbolXml() const { return mSymbolXml; }
    void setMapPosition( const QgsPoint &pos, const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) override;
    void appendPoint( const QPoint &newPoint );
    void movePoint( int index, const QPoint &newPos );
    QList<QPoint> points() const;
    const QList<int>& controlPoints() const { return mControlPoints; }
    int absolutePointIdx( int regularIdx ) const;
    bool isNPoint() const { return !mAdditionalPoints.isEmpty(); }
    void setGraphic( VBSMilixClient::NPointSymbolGraphic& result, bool updatePoints );
    void finalize() { mFinalized = true; }

    void writeXML( QDomDocument& doc ) const override;
    void readXML( const QDomDocument& doc, const QDomElement& itemElem ) override;
    void paint( QPainter* painter ) override;

    int moveActionForPosition( const QPointF& pos ) const override;
    void handleMoveAction( int moveAction, const QPointF &newPos, const QPointF &oldPos ) override;
    Qt::CursorShape cursorShapeForAction( int moveAction ) const override;

    void showContextMenu( const QPoint &screenPos );

  private:
    QString mSymbolXml;
    QPixmap mGraphic;
    QList<QgsPoint> mAdditionalPoints;
    QPoint mOffset;
    QList<int> mControlPoints;
    bool mHasVariablePointCount;
    bool mFinalized;

    void _showItemEditor() override;
    void updateSymbol();
};

#endif // QGSVBSMILIXANNOTATIONITEM_H
