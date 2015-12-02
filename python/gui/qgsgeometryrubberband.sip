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

    QgsGeometryRubberBand( QgsMapCanvas* mapCanvas, QGis::GeometryType geomType = QGis::Line );
    ~QgsGeometryRubberBand();

    /** Sets geometry (takes ownership). Geometry is expected to be in map coordinates */
    void setGeometry( QgsAbstractGeometryV2* geom );
    const QgsAbstractGeometryV2* geometry();

    /** Sets the translation offset (offset in map coordinates for drawing geometry) */
    void setTranslationOffset( double dx, double dy );
    void translationOffset( double& dx, double& dy );

    void moveVertex( const QgsVertexId& id, const QgsPointV2& newPos );

    void setFillColor( const QColor& c );
    void setOutlineColor( const QColor& c );
    void setOutlineWidth( int width );
    void setLineStyle( Qt::PenStyle penStyle );
    void setBrushStyle( Qt::BrushStyle brushStyle );
    void setIconType( IconType iconType );
    void setMeasurementMode( MeasurementMode measurementMode, QGis::UnitType displayUnits );


  protected:
    void paint( QPainter* painter );
};
