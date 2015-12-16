/***************************************************************************
                         qgsgeometryrubberband.h
                         -----------------------
    begin                : December 2014
    copyright            : (C) 2014 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGEOMETRYRUBBERBAND_H
#define QGSGEOMETRYRUBBERBAND_H

#include "qgsmapcanvasitem.h"
#include "qgsdistancearea.h"
#include <QBrush>
#include <QPen>


class QgsAbstractGeometryV2;
class QgsDistanceArea;
class QgsPointV2;
struct QgsVertexId;

class GUI_EXPORT QgsGeometryRubberBand: public QObject, public QgsMapCanvasItem
{
    Q_OBJECT
  public:
    enum IconType
    {
      /**
      * No icon is used
      */
      ICON_NONE,
      /**
       * A cross is used to highlight points (+)
       */
      ICON_CROSS,
      /**
       * A cross is used to highlight points (x)
       */
      ICON_X,
      /**
       * A box is used to highlight points (□)
       */
      ICON_BOX,
      /**
       * A circle is used to highlight points (○)
       */
      ICON_CIRCLE,
      /**
       * A full box is used to highlight points (■)
       */
      ICON_FULL_BOX
    };

    enum MeasurementMode
    {
      MEASURE_NONE,
      MEASURE_LINE,
      MEASURE_LINE_AND_SEGMENTS,
      MEASURE_POLYGON,
      MEASURE_RECTANGLE,
      MEASURE_CIRCLE
    };

    QgsGeometryRubberBand( QgsMapCanvas* mapCanvas, QGis::GeometryType geomType = QGis::Line );
    ~QgsGeometryRubberBand();

    /** Sets geometry (takes ownership). Geometry is expected to be in map coordinates */
    void setGeometry( QgsAbstractGeometryV2* geom );
    const QgsAbstractGeometryV2* geometry() { return mGeometry; }

    /** Sets the translation offset (offset in map coordinates for drawing geometry) */
    void setTranslationOffset( double dx, double dy );
    void translationOffset( double& dx, double& dy ) { dx = mTranslationOffset[0]; dy = mTranslationOffset[1]; }

    void moveVertex( const QgsVertexId& id, const QgsPointV2& newPos );

    void setFillColor( const QColor& c );
    void setOutlineColor( const QColor& c );
    void setOutlineWidth( int width );
    void setLineStyle( Qt::PenStyle penStyle );
    void setBrushStyle( Qt::BrushStyle brushStyle );
    void setIconType( IconType iconType ) { mIconType = iconType; }
    void setMeasurementMode( MeasurementMode measurementMode, QGis::UnitType displayUnits );

  protected:
    virtual void paint( QPainter* painter );

  private:
    QgsAbstractGeometryV2* mGeometry;
    double mTranslationOffset[2];
    QBrush mBrush;
    QPen mPen;
    int mIconSize;
    IconType mIconType;
    QGis::GeometryType mGeometryType;
    QgsDistanceArea mDa;
    MeasurementMode mMeasurementMode;
    QGis::UnitType mDisplayUnits;
    QList<QGraphicsTextItem*> mMeasurementLabels;

    void drawVertex( QPainter* p, double x, double y );
    QgsRectangle rubberBandRectangle() const;
    void measureGeometry( QgsAbstractGeometryV2* geometry );
    QString formatMeasurement( double value, bool isArea ) const;
    void addMeasurements( const QStringList& measurements, const QgsPointV2 &mapPos );

  private slots:
    void redrawMeasurements();
};

#endif // QGSGEOMETRYRUBBERBAND_H
