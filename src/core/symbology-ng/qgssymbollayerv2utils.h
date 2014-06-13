/***************************************************************************
 qgssymbollayerv2utils.h
 ---------------------
 begin                : November 2009
 copyright            : (C) 2009 by Martin Dobias
 email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSSYMBOLLAYERV2UTILS_H
#define QGSSYMBOLLAYERV2UTILS_H

#include <QMap>
#include <Qt>
#include <QtCore>
#include <QFont>
#include <QColor>
#include "qgssymbolv2.h"
#include "qgis.h"
#include "qgsmapunitscale.h"

class QgsExpression;
class QgsSymbolLayerV2;
class QgsVectorColorRampV2;

typedef QMap<QString, QString> QgsStringMap;
typedef QMap<QString, QgsSymbolV2* > QgsSymbolV2Map;

class QDomDocument;
class QDomElement;
class QIcon;
class QPixmap;
class QPointF;
class QSize;

class CORE_EXPORT QgsSymbolLayerV2Utils
{
  public:

    static QString encodeColor( QColor color );
    static QColor decodeColor( QString str );

    static QString encodeSldAlpha( int alpha );
    static int decodeSldAlpha( QString str );

    static QString encodeSldFontStyle( QFont::Style style );
    static QFont::Style decodeSldFontStyle( QString str );

    static QString encodeSldFontWeight( int weight );
    static int decodeSldFontWeight( QString str );

    static QString encodePenStyle( Qt::PenStyle style );
    static Qt::PenStyle decodePenStyle( QString str );

    static QString encodePenJoinStyle( Qt::PenJoinStyle style );
    static Qt::PenJoinStyle decodePenJoinStyle( QString str );

    static QString encodePenCapStyle( Qt::PenCapStyle style );
    static Qt::PenCapStyle decodePenCapStyle( QString str );

    static QString encodeSldLineJoinStyle( Qt::PenJoinStyle style );
    static Qt::PenJoinStyle decodeSldLineJoinStyle( QString str );

    static QString encodeSldLineCapStyle( Qt::PenCapStyle style );
    static Qt::PenCapStyle decodeSldLineCapStyle( QString str );

    static QString encodeBrushStyle( Qt::BrushStyle style );
    static Qt::BrushStyle decodeBrushStyle( QString str );

    static QString encodeSldBrushStyle( Qt::BrushStyle style );
    static Qt::BrushStyle decodeSldBrushStyle( QString str );

    static QString encodePoint( QPointF point );
    static QPointF decodePoint( QString str );

    static QString encodeMapUnitScale( const QgsMapUnitScale& mapUnitScale );
    static QgsMapUnitScale decodeMapUnitScale( const QString& str );

    static QString encodeRealVector( const QVector<qreal>& v );
    static QVector<qreal> decodeRealVector( const QString& s );

    static QString encodeSldRealVector( const QVector<qreal>& v );
    static QVector<qreal> decodeSldRealVector( const QString& s );

    static QString encodeOutputUnit( QgsSymbolV2::OutputUnit unit );
    static QgsSymbolV2::OutputUnit decodeOutputUnit( QString str );

    static QString encodeSldUom( QgsSymbolV2::OutputUnit unit, double *scaleFactor );
    static QgsSymbolV2::OutputUnit decodeSldUom( QString str, double *scaleFactor );

    static QString encodeScaleMethod( QgsSymbolV2::ScaleMethod scaleMethod );
    static QgsSymbolV2::ScaleMethod decodeScaleMethod( QString str );

    static QIcon symbolPreviewIcon( QgsSymbolV2* symbol, QSize size );
    static QIcon symbolLayerPreviewIcon( QgsSymbolLayerV2* layer, QgsSymbolV2::OutputUnit u, QSize size, const QgsMapUnitScale& scale = QgsMapUnitScale() );
    static QIcon colorRampPreviewIcon( QgsVectorColorRampV2* ramp, QSize size );

    static void drawStippledBackround( QPainter* painter, QRect rect );

    static QPixmap symbolPreviewPixmap( QgsSymbolV2* symbol, QSize size );
    static QPixmap colorRampPreviewPixmap( QgsVectorColorRampV2* ramp, QSize size );

    /**Returns the maximum estimated bleed for the symbol */
    static double estimateMaxSymbolBleed( QgsSymbolV2* symbol );

    static QgsSymbolV2* loadSymbol( QDomElement& element );
    static QgsSymbolLayerV2* loadSymbolLayer( QDomElement& element );
    static QDomElement saveSymbol( QString symbolName, QgsSymbolV2* symbol, QDomDocument& doc );

    static bool createSymbolLayerV2ListFromSld( QDomElement& element, QGis::GeometryType geomType, QgsSymbolLayerV2List &layers );

    static QgsSymbolLayerV2* createFillLayerFromSld( QDomElement &element );
    static QgsSymbolLayerV2* createLineLayerFromSld( QDomElement &element );
    static QgsSymbolLayerV2* createMarkerLayerFromSld( QDomElement &element );

    static bool convertPolygonSymbolizerToPointMarker( QDomElement &element, QgsSymbolLayerV2List &layerList );
    static bool hasExternalGraphic( QDomElement &element );
    static bool hasWellKnownMark( QDomElement &element );

    static bool needFontMarker( QDomElement &element );
    static bool needSvgMarker( QDomElement &element );
    static bool needEllipseMarker( QDomElement &element );
    static bool needMarkerLine( QDomElement &element );
    static bool needLinePatternFill( QDomElement &element );
    static bool needPointPatternFill( QDomElement &element );
    static bool needSvgFill( QDomElement &element );

    static void fillToSld( QDomDocument &doc, QDomElement &element,
                           Qt::BrushStyle brushStyle, QColor color = QColor() );
    static bool fillFromSld( QDomElement &element,
                             Qt::BrushStyle &brushStyle, QColor &color );

    //! @note not available in python bindings
    static void lineToSld( QDomDocument &doc, QDomElement &element,
                           Qt::PenStyle penStyle, QColor color, double width = -1,
                           const Qt::PenJoinStyle *penJoinStyle = 0, const Qt::PenCapStyle *penCapStyle = 0,
                           const QVector<qreal> *customDashPattern = 0, double dashOffset = 0.0 );
    static bool lineFromSld( QDomElement &element,
                             Qt::PenStyle &penStyle, QColor &color, double &width,
                             Qt::PenJoinStyle *penJoinStyle = 0, Qt::PenCapStyle *penCapStyle = 0,
                             QVector<qreal> *customDashPattern = 0, double *dashOffset = 0 );

    static void externalGraphicToSld( QDomDocument &doc, QDomElement &element,
                                      QString path, QString mime,
                                      QColor color, double size = -1 );
    static bool externalGraphicFromSld( QDomElement &element,
                                        QString &path, QString &mime,
                                        QColor &color, double &size );

    /** @deprecated Use wellKnownMarkerToSld( QDomDocument &doc, QDomElement &element, QString name, QColor color, QColor borderColor, Qt::PenStyle borderStyle, double borderWidth, double size ) instead */
    Q_DECL_DEPRECATED static void wellKnownMarkerToSld( QDomDocument &doc, QDomElement &element,
        QString name, QColor color, QColor borderColor = QColor(),
        double borderWidth = -1, double size = -1 );
    static void wellKnownMarkerToSld( QDomDocument &doc, QDomElement &element,
                                      QString name, QColor color, QColor borderColor, Qt::PenStyle borderStyle,
                                      double borderWidth = -1, double size = -1 );
    /** @deprecated Use wellKnownMarkerFromSld( QDomElement &element, QString &name, QColor &color, QColor &borderColor, Qt::PenStyle &borderStyle, double &borderWidth, double &size ) instead */
    Q_DECL_DEPRECATED static bool wellKnownMarkerFromSld( QDomElement &element,
        QString &name, QColor &color, QColor &borderColor,
        double &borderWidth, double &size );
    static bool wellKnownMarkerFromSld( QDomElement &element,
                                        QString &name, QColor &color, QColor &borderColor, Qt::PenStyle &borderStyle,
                                        double &borderWidth, double &size );

    static void externalMarkerToSld( QDomDocument &doc, QDomElement &element,
                                     QString path, QString format, int *markIndex = 0,
                                     QColor color = QColor(), double size = -1 );
    static bool externalMarkerFromSld( QDomElement &element,
                                       QString &path, QString &format, int &markIndex,
                                       QColor &color, double &size );


    static void labelTextToSld( QDomDocument &doc, QDomElement &element, QString label,
                                QFont font, QColor color = QColor(), double size = -1 );

    /**Create ogr feature style string for pen */
    static QString ogrFeatureStylePen( double width, double mmScaleFactor, double mapUnitsScaleFactor, const QColor& c,
                                       Qt::PenJoinStyle joinStyle = Qt::MiterJoin,
                                       Qt::PenCapStyle capStyle = Qt::FlatCap,
                                       double offset = 0.0,
                                       const QVector<qreal>* dashPattern = 0 );
    /**Create ogr feature style string for brush
     @param fillColr fill color*/
    static QString ogrFeatureStyleBrush( const QColor& fillColr );

    static void createRotationElement( QDomDocument &doc, QDomElement &element, QString rotationFunc );
    static bool rotationFromSldElement( QDomElement &element, QString &rotationFunc );

    static void createOpacityElement( QDomDocument &doc, QDomElement &element, QString alphaFunc );
    static bool opacityFromSldElement( QDomElement &element, QString &alphaFunc );

    static void createDisplacementElement( QDomDocument &doc, QDomElement &element, QPointF offset );
    static bool displacementFromSldElement( QDomElement &element, QPointF &offset );

    static void createOnlineResourceElement( QDomDocument &doc, QDomElement &element, QString path, QString format );
    static bool onlineResourceFromSldElement( QDomElement &element, QString &path, QString &format );

    static void createGeometryElement( QDomDocument &doc, QDomElement &element, QString geomFunc );
    static bool geometryFromSldElement( QDomElement &element, QString &geomFunc );

    static bool createFunctionElement( QDomDocument &doc, QDomElement &element, QString function );
    static bool functionFromSldElement( QDomElement &element, QString &function );

    static QDomElement createSvgParameterElement( QDomDocument &doc, QString name, QString value );
    static QgsStringMap getSvgParameterList( QDomElement &element );

    static QDomElement createVendorOptionElement( QDomDocument &doc, QString name, QString value );
    static QgsStringMap getVendorOptionList( QDomElement &element );

    static QgsStringMap parseProperties( QDomElement& element );
    static void saveProperties( QgsStringMap props, QDomDocument& doc, QDomElement& element );

    static QgsSymbolV2Map loadSymbols( QDomElement& element );
    static QDomElement saveSymbols( QgsSymbolV2Map& symbols, QString tagName, QDomDocument& doc );

    static void clearSymbolMap( QgsSymbolV2Map& symbols );

    static QgsVectorColorRampV2* loadColorRamp( QDomElement& element );
    static QDomElement saveColorRamp( QString name, QgsVectorColorRampV2* ramp, QDomDocument& doc );

    /**
     * Attempts to parse a string as a color using a variety of common formats, including hex
     * codes, rgb and rgba strings.
     * @param colorStr string representing the color
     * @returns parsed color
     * @note added in 2.3
     */
    static QColor parseColor( QString colorStr );

    /**
     * Attempts to parse a string as a color using a variety of common formats, including hex
     * codes, rgb and rgba strings.
     * @param colorStr string representing the color
     * @param containsAlpha if colorStr contains an explicit alpha value then containsAlpha will be set to true
     * @returns parsed color
     * @note added in 2.3
     */
    static QColor parseColorWithAlpha( const QString colorStr, bool &containsAlpha );

    /**Returns the line width scale factor depending on the unit and the paint device*/
    static double lineWidthScaleFactor( const QgsRenderContext& c, QgsSymbolV2::OutputUnit u, const QgsMapUnitScale& scale = QgsMapUnitScale() );
    /**Returns scale factor painter units -> pixel dimensions*/
    static double pixelSizeScaleFactor( const QgsRenderContext& c, QgsSymbolV2::OutputUnit u, const QgsMapUnitScale& scale = QgsMapUnitScale() );
    /**Creates a render context for a pixel based device*/
    static QgsRenderContext createRenderContext( QPainter* p );

    /**Multiplies opacity of image pixel values with a (global) transparency value*/
    static void multiplyImageOpacity( QImage* image, qreal alpha );

    /** Blurs an image in place, e.g. creating Qt-independent drop shadows
     * @note added in 1.9
     */
    static void blurImageInPlace( QImage& image, const QRect& rect, int radius, bool alphaOnly );

    /** Converts a QColor into a premultiplied ARGB QColor value using a specified alpha value
     * @note added in 2.3
     */
    static void premultiplyColor( QColor& rgb, int alpha );

    /**Sorts the passed list in requested order*/
    static void sortVariantList( QList<QVariant>& list, Qt::SortOrder order );
    /**Returns a point on the line from startPoint to directionPoint that is a certain distance away from the starting point*/
    static QPointF pointOnLineWithDistance( const QPointF& startPoint, const QPointF& directionPoint, double distance );

    //! Return a list of all available svg files
    static QStringList listSvgFiles();

    //! Return a list of svg files at the specified directory
    static QStringList listSvgFilesAt( QString directory );

    /** Get symbol's path from its name.
     *  If the name is not absolute path the file is searched in SVG paths specified
     *  in settings svg/searchPathsForSVG.
     */
    static QString symbolNameToPath( QString name );

    //! Get symbols's name from its path
    static QString symbolPathToName( QString path );

    //! Calculate the centroid point of a QPolygonF
    static QPointF polygonCentroid( const QPolygonF& points );

    //! Calculate a point within of a QPolygonF
    static QPointF polygonPointOnSurface( const QPolygonF& points );

    //! Calculate whether a point is within of a QPolygonF
    static bool pointInPolygon( const QPolygonF &points, const QPointF &point );

    /** Return a new valid expression instance for given field or expression string.
     * If the input is not a valid expression, it is assumed that it is a field name and gets properly quoted.
     * If the string is empty, returns null pointer.
     * This is useful when accepting input which could be either a non-quoted field name or expression.
     * @note added in 2.2
     */
    static QgsExpression* fieldOrExpressionToExpression( const QString& fieldOrExpression );

    /** Return a field name if the whole expression is just a name of the field .
     *  Returns full expression string if the expression is more complex than just one field.
     *  Using just expression->expression() method may return quoted field name, but that is not
     *  wanted for saving (due to backward compatibility) or display in GUI.
     * @note added in 2.2
     */
    static QString fieldOrExpressionFromExpression( QgsExpression* expression );
};

class QPolygonF;

//! @deprecated since 2.4 - calculate line shifted by a specified distance
QList<QPolygonF> offsetLine( QPolygonF polyline, double dist );
//! calculate geometry shifted by a specified distance
QList<QPolygonF> offsetLine( QPolygonF polyline, double dist, QGis::GeometryType geometryType );

#endif


