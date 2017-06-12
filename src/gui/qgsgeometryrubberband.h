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

#include "qgsabstractgeometryv2.h"
#include "qgsmapcanvasitem.h"
#include "qgsdistancearea.h"
#include <QBrush>
#include <QPen>


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
      ICON_FULL_BOX,
      /**
       * A triangle is used to highlight points (△)
       */
      ICON_TRIANGLE,
      /**
       * A full triangle is used to highlight points (▲)
       */
      ICON_FULL_TRIANGLE
    };

    enum MeasurementMode
    {
      MEASURE_NONE,
      MEASURE_LINE,
      MEASURE_LINE_AND_SEGMENTS,
      MEASURE_ANGLE,
      MEASURE_AZIMUTH,
      MEASURE_POLYGON,
      MEASURE_RECTANGLE,
      MEASURE_CIRCLE
    };

    enum AngleUnit
    {
      ANGLE_DEGREES,
      ANGLE_RADIANS,
      ANGLE_GRADIANS,
      ANGLE_MIL
    };
    QgsGeometryRubberBand( QgsMapCanvas* mapCanvas, QGis::GeometryType geomType = QGis::Line );
    ~QgsGeometryRubberBand();

    /** Sets geometry (takes ownership). Geometry is expected to be in map coordinates */
    void setGeometry( QgsAbstractGeometryV2* geom, const QList<QgsVertexId>& hiddenNodes = QList<QgsVertexId>() );
    const QgsAbstractGeometryV2* geometry() { return mGeometry; }

    /** Sets the translation offset (offset in map coordinates for drawing geometry) */
    void setTranslationOffset( double dx, double dy );
    void translationOffset( double& dx, double& dy ) { dx = mTranslationOffset[0]; dy = mTranslationOffset[1]; }

    void moveVertex( const QgsVertexId& id, const QgsPointV2& newPos );

    void setFillColor( const QColor& c );
    QColor fillColor() const;
    void setOutlineColor( const QColor& c );
    QColor outlineColor() const;
    void setOutlineWidth( int width );
    int outlineWidth() const;
    void setLineStyle( Qt::PenStyle penStyle );
    Qt::PenStyle lineStyle() const;
    void setBrushStyle( Qt::BrushStyle brushStyle );
    Qt::BrushStyle brushStyle() const;
    void setIconType( IconType iconType ) { mIconType = iconType; }
    void setIconSize( int iconSize ) { mIconSize = iconSize; }
    void setIconFillColor( const QColor& c );
    void setIconOutlineColor( const QColor& c );
    void setIconOutlineWidth( int width );
    void setIconLineStyle( Qt::PenStyle penStyle );
    void setIconBrushStyle( Qt::BrushStyle brushStyle );
    void setMeasurementMode( MeasurementMode measurementMode, QGis::UnitType displayUnits, AngleUnit angleUnit = ANGLE_DEGREES );

    // Custom measurement handler for cases where default measurement method does not work
    class Measurer
    {
      public:
        virtual ~Measurer() {}
        struct Measurement
        {
          enum Type {Length, Area, Angle} type;
          QString label;
          double value;
        };
        virtual QList<Measurement> measure( MeasurementMode measurementMode, int part, const QgsAbstractGeometryV2* geometry, QList<double>& partMeasurements ) = 0;
    };
    // Ownership transferred
    void setMeasurer( Measurer* measurer ) { mMeasurer = measurer; }

    QString getTotalMeasurement() const;

  protected:
    virtual void paint( QPainter* painter );

  private:
    QgsAbstractGeometryV2* mGeometry;
    QList<QgsVertexId> mHiddenNodes;
    double mTranslationOffset[2];
    QBrush mBrush;
    QPen mPen;
    int mIconSize;
    IconType mIconType;
    QBrush mIconBrush;
    QPen mIconPen;
    QGis::GeometryType mGeometryType;
    QgsDistanceArea mDa;
    MeasurementMode mMeasurementMode;
    QGis::UnitType mDisplayUnits;
    AngleUnit mAngleUnit;
    QList<double> mPartMeasurements;
    QList<QGraphicsTextItem*> mMeasurementLabels;
    Measurer* mMeasurer;

    void drawVertex( QPainter* p, double x, double y );
    QgsRectangle rubberBandRectangle() const;
    void measureGeometry( QgsAbstractGeometryV2* geometry , int part );
    QString formatMeasurement( double value, bool isArea ) const;
    QString formatAngle( double value ) const;
    void addMeasurements( const QStringList& measurements, const QgsPointV2 &mapPos );

  private slots:
    void configureDistanceArea();
    void redrawMeasurements();
};

#endif // QGSGEOMETRYRUBBERBAND_H
