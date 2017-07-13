class QgsGeometryRubberBand: public QgsMapCanvasItem
{
%TypeHeaderCode
#include <qgsgeometryrubberband.h>
%End
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
    void setGeometry( QgsAbstractGeometryV2* geom );
    const QgsAbstractGeometryV2* geometry() const;

    /** Returns whether the geometry contains the specified point */
    bool contains(const QgsPoint& p, double tol) const;

    /** Sets the translation offset (offset in map coordinates for drawing geometry) */
    void setTranslationOffset( double dx, double dy );
    void translationOffset( double& dx, double& dy );

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
    void setIconType( IconType iconType );
    IconType iconType() const;
    void setIconSize( int iconSize );
    int iconSize() const;
    void setIconFillColor( const QColor& c );
    const QColor& iconFillColor() const;
    void setIconOutlineColor( const QColor& c );
    QColor iconOutlineColor() const;
    void setIconOutlineWidth( int width );
    int iconOutlineWidth() const;
    void setIconLineStyle( Qt::PenStyle penStyle );
    Qt::PenStyle iconLineStyle() const;
    void setIconBrushStyle( Qt::BrushStyle brushStyle );
    Qt::BrushStyle iconBrushStyle() const;
    void setMeasurementMode( MeasurementMode measurementMode, QGis::UnitType displayUnits, AngleUnit angleUnit = ANGLE_DEGREES );


  protected:
    void paint( QPainter* painter );
};
