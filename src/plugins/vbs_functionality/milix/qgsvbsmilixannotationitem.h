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

class QgsCoordinateTransform;
class QgsVBSCoordinateDisplayer;
class QgsVBSMilixItem;

class QgsVBSMilixAnnotationItem : public QgsAnnotationItem
{
    QGS_ANNOTATION_ITEM( QgsVBSMilixAnnotationItem, "MilixAnnotationItem" )

  public:
    QgsVBSMilixAnnotationItem( QgsMapCanvas* canvas );
    QgsVBSMilixAnnotationItem* clone( QgsMapCanvas */*canvas*/ ) override { return 0; /* Unsupported */ }
    QgsVBSMilixItem* createMilixItem() const;
    void setSymbolXml( const QString& symbolXml, const QString &symbolMilitaryName, bool isMultiPoint );
    void setMapPosition( const QgsPoint &pos, const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) override;
    void appendPoint( const QPoint &newPoint );
    void movePoint( int index, const QPoint &newPos );
    QList<QPoint> screenPoints() const;
    int absolutePointIdx( int regularIdx ) const;
    void setGraphic( VBSMilixClient::NPointSymbolGraphic& result, bool updatePoints );
    void finalize() { mFinalized = true; }

    void paint( QPainter* painter ) override;
    void writeXML( QDomDocument& /*doc*/ ) const override {}
    void readXML( const QDomDocument& /*doc*/, const QDomElement& /*itemElem*/ ) override {}

    int moveActionForPosition( const QPointF& pos ) const override;
    void handleMoveAction( int moveAction, const QPointF &newPos, const QPointF &oldPos ) override;
    Qt::CursorShape cursorShapeForAction( int moveAction ) const override;

    void showContextMenu( const QPoint &screenPos );
    bool hitTest( const QPoint& screenPos ) const override;

  private:
    QString mSymbolXml;
    QString mSymbolMilitaryName;
    QPixmap mGraphic;
    QList<QgsPoint> mAdditionalPoints;
    QPoint mRenderOffset;
    QList<int> mControlPoints;
    bool mIsMultiPoint;
    bool mFinalized;

    void _showItemEditor() override;
    void updateSymbol( bool updatePoints );
};

#endif // QGSVBSMILIXANNOTATIONITEM_H
