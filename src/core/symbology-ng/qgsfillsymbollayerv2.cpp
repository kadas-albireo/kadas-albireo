/***************************************************************************
    qgsfillsymbollayerv2.cpp
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
#include "qgsfillsymbollayerv2.h"
#include "qgsmarkersymbollayerv2.h"
#include "qgssymbollayerv2utils.h"

#include "qgsexpression.h"
#include "qgsrendercontext.h"
#include "qgsproject.h"
#include "qgssvgcache.h"
#include "qgslogger.h"

#include <QPainter>
#include <QFile>
#include <QSvgRenderer>
#include <QDomDocument>
#include <QDomElement>

QgsSimpleFillSymbolLayerV2::QgsSimpleFillSymbolLayerV2( QColor color, Qt::BrushStyle style, QColor borderColor, Qt::PenStyle borderStyle, double borderWidth )
    : mBrushStyle( style ), mBorderColor( borderColor ), mBorderStyle( borderStyle ), mBorderWidth( borderWidth ), mBorderWidthUnit( QgsSymbolV2::MM ),
    mOffsetUnit( QgsSymbolV2::MM ), mColorExpression( 0 ), mColorBorderExpression( 0 ), mWidthBorderExpression( 0 )
{
  mColor = color;
}

void QgsSimpleFillSymbolLayerV2::setOutputUnit( QgsSymbolV2::OutputUnit unit )
{
  mBorderWidthUnit = unit;
  mOffsetUnit = unit;
}

QgsSymbolV2::OutputUnit QgsSimpleFillSymbolLayerV2::outputUnit() const
{
  QgsSymbolV2::OutputUnit unit = mBorderWidthUnit;
  if ( mOffsetUnit != unit )
  {
    return QgsSymbolV2::Mixed;
  }
  return unit;
}

const QgsExpression* QgsSimpleFillSymbolLayerV2::dataDefinedProperty( const QString& property ) const
{
  if ( property == "color" )
  {
    return mColorExpression;
  }
  else if ( property == "color_border" )
  {
    return mColorBorderExpression;
  }
  else if ( property == "width_border" )
  {
    return mWidthBorderExpression;
  }
  return 0;
}

QString QgsSimpleFillSymbolLayerV2::dataDefinedPropertyString( const QString& property ) const
{
  const QgsExpression* ex  = dataDefinedProperty( property );
  return ex ? ex->dump() : QString();
}

void QgsSimpleFillSymbolLayerV2::setDataDefinedProperty( const QString& property, const QString& expressionString )
{
  if ( property == "color" )
  {
    delete mColorExpression; mColorExpression = new QgsExpression( expressionString );
  }
  else if ( property == "color_border" )
  {
    delete mColorBorderExpression; mColorBorderExpression = new QgsExpression( expressionString );
  }
  else if ( property == "width_border" )
  {
    delete mWidthBorderExpression; mWidthBorderExpression = new QgsExpression( expressionString );
  }
}

void QgsSimpleFillSymbolLayerV2::removeDataDefinedProperty( const QString& property )
{
  if ( property == "color" )
  {
    delete mColorExpression; mColorExpression = 0;
  }
  else if ( property == "color_border" )
  {
    delete mColorBorderExpression; mColorBorderExpression = 0;
  }
  else if ( property == "width_border" )
  {
    delete mWidthBorderExpression; mWidthBorderExpression = 0;
  }
}

void QgsSimpleFillSymbolLayerV2::removeDataDefinedProperties()
{
  delete mColorExpression; mColorExpression = 0;
  delete mColorBorderExpression; mColorBorderExpression = 0;
  delete mWidthBorderExpression; mWidthBorderExpression = 0;
}

QSet<QString> QgsSimpleFillSymbolLayerV2::usedAttributes() const
{
  QSet<QString> attributes;

  //add data defined attributes
  QStringList columns;
  if ( mColorExpression )
    columns.append( mColorExpression->referencedColumns() );
  if ( mColorBorderExpression )
    columns.append( mColorBorderExpression->referencedColumns() );
  if ( mWidthBorderExpression )
    columns.append( mWidthBorderExpression->referencedColumns() );

  QStringList::const_iterator it = columns.constBegin();
  for ( ; it != columns.constEnd(); ++it )
  {
    attributes.insert( *it );
  }
  return attributes;
}

void QgsSimpleFillSymbolLayerV2::prepareExpressions( const QgsVectorLayer* vl )
{
  if ( !vl )
  {
    return;
  }

  const QgsFields& fields = vl->pendingFields();
  if ( mColorExpression )
    mColorExpression->prepare( fields );
  if ( mColorBorderExpression )
    mColorBorderExpression->prepare( fields );
  if ( mWidthBorderExpression )
    mWidthBorderExpression->prepare( fields );
}

void QgsSimpleFillSymbolLayerV2::applyDataDefinedSymbology( QgsSymbolV2RenderContext& context, QBrush& brush, QPen& pen, QPen& selPen )
{
  if ( mColorExpression )
  {
    brush.setColor( QgsSymbolLayerV2Utils::decodeColor( mColorExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toString() ) );
  }
  if ( mColorBorderExpression )
  {
    pen.setColor( QgsSymbolLayerV2Utils::decodeColor( mColorBorderExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toString() ) );
  }
  if ( mWidthBorderExpression )
  {
    double width = mWidthBorderExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
    width *= QgsSymbolLayerV2Utils::lineWidthScaleFactor( context.renderContext(), mBorderWidthUnit );
    pen.setWidthF( width );
    selPen.setWidthF( width );
  }
}


QgsSymbolLayerV2* QgsSimpleFillSymbolLayerV2::create( const QgsStringMap& props )
{
  QColor color = DEFAULT_SIMPLEFILL_COLOR;
  Qt::BrushStyle style = DEFAULT_SIMPLEFILL_STYLE;
  QColor borderColor = DEFAULT_SIMPLEFILL_BORDERCOLOR;
  Qt::PenStyle borderStyle = DEFAULT_SIMPLEFILL_BORDERSTYLE;
  double borderWidth = DEFAULT_SIMPLEFILL_BORDERWIDTH;
  QPointF offset;

  if ( props.contains( "color" ) )
    color = QgsSymbolLayerV2Utils::decodeColor( props["color"] );
  if ( props.contains( "style" ) )
    style = QgsSymbolLayerV2Utils::decodeBrushStyle( props["style"] );
  if ( props.contains( "color_border" ) )
    borderColor = QgsSymbolLayerV2Utils::decodeColor( props["color_border"] );
  if ( props.contains( "style_border" ) )
    borderStyle = QgsSymbolLayerV2Utils::decodePenStyle( props["style_border"] );
  if ( props.contains( "width_border" ) )
    borderWidth = props["width_border"].toDouble();
  if ( props.contains( "offset" ) )
    offset = QgsSymbolLayerV2Utils::decodePoint( props["offset"] );

  QgsSimpleFillSymbolLayerV2* sl = new QgsSimpleFillSymbolLayerV2( color, style, borderColor, borderStyle, borderWidth );
  sl->setOffset( offset );
  if ( props.contains( "border_width_unit" ) )
    sl->setBorderWidthUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( props["border_width_unit"] ) );
  if ( props.contains( "offset_unit" ) )
    sl->setOffsetUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( props["offset_unit"] ) );

  if ( props.contains( "color_expression" ) )
  {
    sl->setDataDefinedProperty( "color", props["color_expression"] );
  }
  if ( props.contains( "color_border_expression" ) )
  {
    sl->setDataDefinedProperty( "color_border", props["color_border_expression"] );
  }
  if ( props.contains( "width_border_expression" ) )
  {
    sl->setDataDefinedProperty( "width_border", props["width_border_expression"] );
  }
  return sl;
}


QString QgsSimpleFillSymbolLayerV2::layerType() const
{
  return "SimpleFill";
}

void QgsSimpleFillSymbolLayerV2::startRender( QgsSymbolV2RenderContext& context )
{
  QColor fillColor = mColor;
  fillColor.setAlphaF( context.alpha() * mColor.alphaF() );
  mBrush = QBrush( fillColor, mBrushStyle );

  // scale brush content for printout
  double rasterScaleFactor = context.renderContext().rasterScaleFactor();
  if ( rasterScaleFactor != 1.0 )
  {
    mBrush.setMatrix( QMatrix().scale( 1.0 / rasterScaleFactor, 1.0 / rasterScaleFactor ) );
  }

  QColor selColor = context.selectionColor();
  QColor selPenColor = selColor == mColor ? selColor : mBorderColor;
  if ( ! selectionIsOpaque ) selColor.setAlphaF( context.alpha() );
  mSelBrush = QBrush( selColor );
  // N.B. unless a "selection line color" is implemented in addition to the "selection color" option
  // this would mean symbols with "no fill" look the same whether or not they are selected
  if ( selectFillStyle )
    mSelBrush.setStyle( mBrushStyle );

  QColor borderColor = mBorderColor;
  borderColor.setAlphaF( context.alpha() * mBorderColor.alphaF() );
  mPen = QPen( borderColor );
  mSelPen = QPen( selPenColor );
  mPen.setStyle( mBorderStyle );
  mPen.setWidthF( mBorderWidth * QgsSymbolLayerV2Utils::lineWidthScaleFactor( context.renderContext(), mBorderWidthUnit ) );
  prepareExpressions( context.layer() );
}

void QgsSimpleFillSymbolLayerV2::stopRender( QgsSymbolV2RenderContext& context )
{
  Q_UNUSED( context );
}

void QgsSimpleFillSymbolLayerV2::renderPolygon( const QPolygonF& points, QList<QPolygonF>* rings, QgsSymbolV2RenderContext& context )
{
  QPainter* p = context.renderContext().painter();
  if ( !p )
  {
    return;
  }

  applyDataDefinedSymbology( context, mBrush, mPen, mSelPen );

  p->setBrush( context.selected() ? mSelBrush : mBrush );
  p->setPen( mPen );
  p->setPen( context.selected() ? mSelPen : mPen );

  QPointF offset;
  if ( !mOffset.isNull() )
  {
    offset.setX( mOffset.x() * QgsSymbolLayerV2Utils::lineWidthScaleFactor( context.renderContext(), mOffsetUnit ) );
    offset.setY( mOffset.y() * QgsSymbolLayerV2Utils::lineWidthScaleFactor( context.renderContext(), mOffsetUnit ) );
    p->translate( offset );
  }

  _renderPolygon( p, points, rings );

  if ( !mOffset.isNull() )
  {
    p->translate( -offset );
  }
}

QgsStringMap QgsSimpleFillSymbolLayerV2::properties() const
{
  QgsStringMap map;
  map["color"] = QgsSymbolLayerV2Utils::encodeColor( mColor );
  map["style"] = QgsSymbolLayerV2Utils::encodeBrushStyle( mBrushStyle );
  map["color_border"] = QgsSymbolLayerV2Utils::encodeColor( mBorderColor );
  map["style_border"] = QgsSymbolLayerV2Utils::encodePenStyle( mBorderStyle );
  map["width_border"] = QString::number( mBorderWidth );
  map["border_width_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mBorderWidthUnit );
  map["offset"] = QgsSymbolLayerV2Utils::encodePoint( mOffset );
  map["offset_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mOffsetUnit );
  if ( mColorExpression )
  {
    map["color_expression"] = mColorExpression->dump();
  }
  if ( mColorBorderExpression )
  {
    map["color_border_expression"] = mColorBorderExpression->dump();
  }
  if ( mWidthBorderExpression )
  {
    map["width_border_expression"] = mWidthBorderExpression->dump();
  }
  return map;
}

QgsSymbolLayerV2* QgsSimpleFillSymbolLayerV2::clone() const
{
  QgsSimpleFillSymbolLayerV2* sl = new QgsSimpleFillSymbolLayerV2( mColor, mBrushStyle, mBorderColor, mBorderStyle, mBorderWidth );
  sl->setOffset( mOffset );
  sl->setOffsetUnit( mOffsetUnit );
  sl->setBorderWidthUnit( mBorderWidthUnit );

  if ( mColorExpression )
  {
    sl->setDataDefinedProperty( "color", mColorExpression->dump() );
  }
  if ( mColorBorderExpression )
  {
    sl->setDataDefinedProperty( "color_border", mColorBorderExpression->dump() );
  }
  if ( mWidthBorderExpression )
  {
    sl->setDataDefinedProperty( "width_border", mWidthBorderExpression->dump() );
  }
  return sl;
}

void QgsSimpleFillSymbolLayerV2::toSld( QDomDocument &doc, QDomElement &element, QgsStringMap props ) const
{
  if ( mBrushStyle == Qt::NoBrush && mBorderStyle == Qt::NoPen )
    return;

  QDomElement symbolizerElem = doc.createElement( "se:PolygonSymbolizer" );
  if ( !props.value( "uom", "" ).isEmpty() )
    symbolizerElem.setAttribute( "uom", props.value( "uom", "" ) );
  element.appendChild( symbolizerElem );

  // <Geometry>
  QgsSymbolLayerV2Utils::createGeometryElement( doc, symbolizerElem, props.value( "geom", "" ) );

  if ( mBrushStyle != Qt::NoBrush )
  {
    // <Fill>
    QDomElement fillElem = doc.createElement( "se:Fill" );
    symbolizerElem.appendChild( fillElem );
    QgsSymbolLayerV2Utils::fillToSld( doc, fillElem, mBrushStyle, mColor );
  }

  if ( mBorderStyle != Qt::NoPen )
  {
    // <Stroke>
    QDomElement strokeElem = doc.createElement( "se:Stroke" );
    symbolizerElem.appendChild( strokeElem );
    QgsSymbolLayerV2Utils::lineToSld( doc, strokeElem, mBorderStyle, mBorderColor, mBorderWidth );
  }

  // <se:Displacement>
  QgsSymbolLayerV2Utils::createDisplacementElement( doc, symbolizerElem, mOffset );
}

QString QgsSimpleFillSymbolLayerV2::ogrFeatureStyle( double mmScaleFactor, double mapUnitScaleFactor ) const
{
  //brush
  QString symbolStyle;
  symbolStyle.append( QgsSymbolLayerV2Utils::ogrFeatureStyleBrush( mColor ) );
  symbolStyle.append( ";" );
  //pen
  symbolStyle.append( QgsSymbolLayerV2Utils::ogrFeatureStylePen( mBorderWidth, mmScaleFactor, mapUnitScaleFactor, mBorderColor ) );
  return symbolStyle;
}

QgsSymbolLayerV2* QgsSimpleFillSymbolLayerV2::createFromSld( QDomElement &element )
{
  QgsDebugMsg( "Entered." );

  QColor color, borderColor;
  Qt::BrushStyle fillStyle;
  Qt::PenStyle borderStyle;
  double borderWidth;

  QDomElement fillElem = element.firstChildElement( "Fill" );
  QgsSymbolLayerV2Utils::fillFromSld( fillElem, fillStyle, color );

  QDomElement strokeElem = element.firstChildElement( "Stroke" );
  QgsSymbolLayerV2Utils::lineFromSld( strokeElem, borderStyle, borderColor, borderWidth );

  QPointF offset;
  QgsSymbolLayerV2Utils::displacementFromSldElement( element, offset );

  QgsSimpleFillSymbolLayerV2* sl = new QgsSimpleFillSymbolLayerV2( color, fillStyle, borderColor, borderStyle, borderWidth );
  sl->setOffset( offset );
  return sl;
}


//QgsImageFillSymbolLayer

QgsImageFillSymbolLayer::QgsImageFillSymbolLayer(): mOutlineWidth( 0.0 ), mOutlineWidthUnit( QgsSymbolV2::MM ), mOutline( 0 )
{
  setSubSymbol( new QgsLineSymbolV2() );
}

QgsImageFillSymbolLayer::~QgsImageFillSymbolLayer()
{
}

void QgsImageFillSymbolLayer::renderPolygon( const QPolygonF& points, QList<QPolygonF>* rings, QgsSymbolV2RenderContext& context )
{
  QPainter* p = context.renderContext().painter();
  if ( !p )
  {
    return;
  }

  applyDataDefinedSettings( context );

  p->setPen( QPen( Qt::NoPen ) );
  if ( context.selected() )
  {
    QColor selColor = context.selectionColor();
    // Alister - this doesn't seem to work here
    //if ( ! selectionIsOpaque )
    //  selColor.setAlphaF( context.alpha() );
    p->setBrush( QBrush( selColor ) );
    _renderPolygon( p, points, rings );
  }

  if ( doubleNear( mAngle, 0.0 ) )
  {
    p->setBrush( mBrush );
  }
  else
  {
    QTransform t = mBrush.transform();
    t.rotate( mAngle );
    QBrush rotatedBrush = mBrush;
    rotatedBrush.setTransform( t );
    p->setBrush( rotatedBrush );
  }
  _renderPolygon( p, points, rings );
  if ( mOutline )
  {
    mOutline->renderPolyline( points, context.feature(), context.renderContext(), -1, selectFillBorder && context.selected() );
    if ( rings )
    {
      QList<QPolygonF>::const_iterator ringIt = rings->constBegin();
      for ( ; ringIt != rings->constEnd(); ++ringIt )
      {
        mOutline->renderPolyline( *ringIt, context.feature(), context.renderContext(), -1, selectFillBorder && context.selected() );
      }
    }
  }
}

bool QgsImageFillSymbolLayer::setSubSymbol( QgsSymbolV2* symbol )
{
  if ( !symbol ) //unset current outline
  {
    delete mOutline;
    mOutline = 0;
    return true;
  }

  if ( symbol->type() != QgsSymbolV2::Line )
  {
    delete symbol;
    return false;
  }

  QgsLineSymbolV2* lineSymbol = dynamic_cast<QgsLineSymbolV2*>( symbol );
  if ( lineSymbol )
  {
    delete mOutline;
    mOutline = lineSymbol;
    return true;
  }

  delete symbol;
  return false;
}

//QgsSVGFillSymbolLayer

QgsSVGFillSymbolLayer::QgsSVGFillSymbolLayer( const QString& svgFilePath, double width, double angle ): QgsImageFillSymbolLayer(), mPatternWidth( width ),
    mPatternWidthUnit( QgsSymbolV2::MM ), mSvgOutlineWidthUnit( QgsSymbolV2::MM ), mWidthExpression( 0 ), mSvgFileExpression( 0 ), mAngleExpression( 0 ),
    mFillColorExpression( 0 ), mOutlineColorExpression( 0 ), mOutlineWidthExpression( 0 )
{
  setSvgFilePath( svgFilePath );
  mOutlineWidth = 0.3;
  mAngle = angle;
  setDefaultSvgParams();
  mSvgPattern = 0;
}

QgsSVGFillSymbolLayer::QgsSVGFillSymbolLayer( const QByteArray& svgData, double width, double angle ): QgsImageFillSymbolLayer(), mPatternWidth( width ),
    mSvgData( svgData ), mWidthExpression( 0 ), mSvgFileExpression( 0 ), mAngleExpression( 0 ),
    mFillColorExpression( 0 ), mOutlineColorExpression( 0 ), mOutlineWidthExpression( 0 )
{
  storeViewBox();
  mOutlineWidth = 0.3;
  mAngle = angle;
  setSubSymbol( new QgsLineSymbolV2() );
  setDefaultSvgParams();
  mSvgPattern = 0;
}

QgsSVGFillSymbolLayer::~QgsSVGFillSymbolLayer()
{
  delete mOutline;
  delete mSvgPattern;
}

void QgsSVGFillSymbolLayer::setOutputUnit( QgsSymbolV2::OutputUnit unit )
{
  mPatternWidthUnit = unit;
  mSvgOutlineWidthUnit = unit;
  mOutlineWidthUnit = unit;
}

QgsSymbolV2::OutputUnit QgsSVGFillSymbolLayer::outputUnit() const
{
  QgsSymbolV2::OutputUnit unit = mPatternWidthUnit;
  if ( mSvgOutlineWidthUnit != unit || mOutlineWidthUnit != unit )
  {
    return QgsSymbolV2::Mixed;
  }
  return unit;
}

void QgsSVGFillSymbolLayer::setSvgFilePath( const QString& svgPath )
{
  mSvgData = QgsSvgCache::instance()->getImageData( svgPath );
  storeViewBox();

  mSvgFilePath = svgPath;
  setDefaultSvgParams();
}

QgsSymbolLayerV2* QgsSVGFillSymbolLayer::create( const QgsStringMap& properties )
{
  QByteArray data;
  double width = 20;
  QString svgFilePath;
  double angle = 0.0;

  if ( properties.contains( "width" ) )
  {
    width = properties["width"].toDouble();
  }
  if ( properties.contains( "svgFile" ) )
  {
    QString svgName = properties["svgFile"];
    QString savePath = QgsSymbolLayerV2Utils::symbolNameToPath( svgName );
    svgFilePath = ( savePath.isEmpty() ? svgName : savePath );
  }
  if ( properties.contains( "angle" ) )
  {
    angle = properties["angle"].toDouble();
  }

  QgsSVGFillSymbolLayer* symbolLayer = 0;
  if ( !svgFilePath.isEmpty() )
  {
    symbolLayer = new QgsSVGFillSymbolLayer( svgFilePath, width, angle );
  }
  else
  {
    if ( properties.contains( "data" ) )
    {
      data = QByteArray::fromHex( properties["data"].toLocal8Bit() );
    }
    symbolLayer = new QgsSVGFillSymbolLayer( data, width, angle );
  }

  //svg parameters
  if ( properties.contains( "svgFillColor" ) )
  {
    symbolLayer->setSvgFillColor( QColor( properties["svgFillColor"] ) );
  }
  if ( properties.contains( "svgOutlineColor" ) )
  {
    symbolLayer->setSvgOutlineColor( QColor( properties["svgOutlineColor"] ) );
  }
  if ( properties.contains( "svgOutlineWidth" ) )
  {
    symbolLayer->setSvgOutlineWidth( properties["svgOutlineWidth"].toDouble() );
  }

  //units
  if ( properties.contains( "pattern_width_unit" ) )
  {
    symbolLayer->setPatternWidthUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["pattern_width_unit"] ) );
  }
  if ( properties.contains( "svg_outline_width_unit" ) )
  {
    symbolLayer->setSvgOutlineWidthUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["svg_outline_width_unit"] ) );
  }
  if ( properties.contains( "outline_width_unit" ) )
  {
    symbolLayer->setOutlineWidthUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["outline_width_unit"] ) );
  }

  if ( properties.contains( "width_expression" ) )
  {
    symbolLayer->setDataDefinedProperty( "width", properties["width_expression"] );
  }
  if ( properties.contains( "svgFile_expression" ) )
  {
    symbolLayer->setDataDefinedProperty( "svgFile", properties["svgFile_expression"] );
  }
  if ( properties.contains( "angle_expression" ) )
  {
    symbolLayer->setDataDefinedProperty( "angle", properties["angle_expression"] );
  }
  if ( properties.contains( "svgFillColor_expression" ) )
  {
    symbolLayer->setDataDefinedProperty( "svgFillColor", properties["svgFillColor_expression"] );
  }
  if ( properties.contains( "svgOutlineColor_expression" ) )
  {
    symbolLayer->setDataDefinedProperty( "svgOutlineColor", properties["svgOutlineColor_expression"] );
  }
  if ( properties.contains( "svgOutlineWidth_expression" ) )
  {
    symbolLayer->setDataDefinedProperty( "svgOutlineWidth", properties["svgOutlineWidth_expression"] );
  }

  return symbolLayer;
}

QString QgsSVGFillSymbolLayer::layerType() const
{
  return "SVGFill";
}

void QgsSVGFillSymbolLayer::applyPattern( QBrush& brush, const QString& svgFilePath, double patternWidth, QgsSymbolV2::OutputUnit patternWidthUnit,
    const QColor& svgFillColor, const QColor& svgOutlineColor, double svgOutlineWidth,
    QgsSymbolV2::OutputUnit svgOutlineWidthUnit, const QgsSymbolV2RenderContext& context )
{
  if ( mSvgViewBox.isNull() )
  {
    return;
  }

  delete mSvgPattern;
  mSvgPattern = 0;
  double size = patternWidth * QgsSymbolLayerV2Utils::pixelSizeScaleFactor( context.renderContext(), patternWidthUnit );

  if (( int )size < 1.0 || 10000.0 < size )
  {
    mSvgPattern = new QImage();
    brush.setTextureImage( *mSvgPattern );
  }
  else
  {
    bool fitsInCache = true;
    double outlineWidth = svgOutlineWidth * QgsSymbolLayerV2Utils::lineWidthScaleFactor( context.renderContext(), svgOutlineWidthUnit );
    const QImage& patternImage = QgsSvgCache::instance()->svgAsImage( svgFilePath, size, svgFillColor, svgOutlineColor, outlineWidth,
                                 context.renderContext().scaleFactor(), context.renderContext().rasterScaleFactor(), fitsInCache );
    if ( !fitsInCache )
    {
      const QPicture& patternPict = QgsSvgCache::instance()->svgAsPicture( svgFilePath, size, svgFillColor, svgOutlineColor, outlineWidth,
                                    context.renderContext().scaleFactor(), 1.0 );
      double hwRatio = 1.0;
      if ( patternPict.width() > 0 )
      {
        hwRatio = ( double )patternPict.height() / ( double )patternPict.width();
      }
      mSvgPattern = new QImage(( int )size, ( int )( size * hwRatio ), QImage::Format_ARGB32_Premultiplied );
      mSvgPattern->fill( 0 ); // transparent background

      QPainter p( mSvgPattern );
      p.drawPicture( QPointF( size / 2, size * hwRatio / 2 ), patternPict );
    }

    QTransform brushTransform;
    brushTransform.scale( 1.0 / context.renderContext().rasterScaleFactor(), 1.0 / context.renderContext().rasterScaleFactor() );
    if ( !doubleNear( context.alpha(), 1.0 ) )
    {
      QImage transparentImage = fitsInCache ? patternImage.copy() : mSvgPattern->copy();
      QgsSymbolLayerV2Utils::multiplyImageOpacity( &transparentImage, context.alpha() );
      brush.setTextureImage( transparentImage );
    }
    else
    {
      brush.setTextureImage( fitsInCache ? patternImage : *mSvgPattern );
    }
    brush.setTransform( brushTransform );
  }
}

void QgsSVGFillSymbolLayer::startRender( QgsSymbolV2RenderContext& context )
{

  applyPattern( mBrush, mSvgFilePath, mPatternWidth, mPatternWidthUnit, mSvgFillColor, mSvgOutlineColor, mSvgOutlineWidth, mSvgOutlineWidthUnit, context );

#if 0
  if ( mSvgViewBox.isNull() )
  {
    return;
  }

  delete mSvgPattern;
  mSvgPattern = 0;
  double size = mPatternWidth * QgsSymbolLayerV2Utils::pixelSizeScaleFactor( context.renderContext(), mPatternWidthUnit );

  //don't render pattern if symbol size is below one or above 10,000 pixels
  if (( int )size < 1.0 || 10000.0 < size )
  {
    mSvgPattern = new QImage();
    mBrush.setTextureImage( *mSvgPattern );
  }
  else
  {
    double outlineWidth = mSvgOutlineWidth * QgsSymbolLayerV2Utils::lineWidthScaleFactor( context.renderContext(), mSvgOutlineWidthUnit );
    bool fitsInCache = true;

    const QImage& patternImage = QgsSvgCache::instance()->svgAsImage( mSvgFilePath, size, mSvgFillColor, mSvgOutlineColor, outlineWidth,
                                 context.renderContext().scaleFactor(), context.renderContext().rasterScaleFactor(), fitsInCache );

    if ( !fitsInCache )
    {
      const QPicture& patternPict = QgsSvgCache::instance()->svgAsPicture( mSvgFilePath, size, mSvgFillColor, mSvgOutlineColor, outlineWidth,
                                    context.renderContext().scaleFactor(), 1.0 );
      double hwRatio = 1.0;
      if ( patternPict.width() > 0 )
      {
        hwRatio = ( double )patternPict.height() / ( double )patternPict.width();
      }
      mSvgPattern = new QImage(( int )size, ( int )( size * hwRatio ), QImage::Format_ARGB32_Premultiplied );
      mSvgPattern->fill( 0 ); // transparent background

      QPainter p( mSvgPattern );
      p.drawPicture( QPointF( size / 2, size * hwRatio / 2 ), patternPict );
    }

    QTransform brushTransform;
    brushTransform.scale( 1.0 / context.renderContext().rasterScaleFactor(), 1.0 / context.renderContext().rasterScaleFactor() );
    if ( !doubleNear( context.alpha(), 1.0 ) )
    {
      QImage transparentImage = fitsInCache ? patternImage.copy() : mSvgPattern->copy();
      QgsSymbolLayerV2Utils::multiplyImageOpacity( &transparentImage, context.alpha() );
      mBrush.setTextureImage( transparentImage );
    }
    else
    {
      mBrush.setTextureImage( fitsInCache ? patternImage : *mSvgPattern );
    }
    mBrush.setTransform( brushTransform );
  }
#endif //0

  if ( mOutline )
  {
    mOutline->startRender( context.renderContext() );
  }

  prepareExpressions( context.layer() );
}

void QgsSVGFillSymbolLayer::stopRender( QgsSymbolV2RenderContext& context )
{
  if ( mOutline )
  {
    mOutline->stopRender( context.renderContext() );
  }
}

QgsStringMap QgsSVGFillSymbolLayer::properties() const
{
  QgsStringMap map;
  if ( !mSvgFilePath.isEmpty() )
  {
    map.insert( "svgFile", QgsSymbolLayerV2Utils::symbolPathToName( mSvgFilePath ) );
  }
  else
  {
    map.insert( "data", QString( mSvgData.toHex() ) );
  }

  map.insert( "width", QString::number( mPatternWidth ) );
  map.insert( "angle", QString::number( mAngle ) );

  //svg parameters
  map.insert( "svgFillColor", mSvgFillColor.name() );
  map.insert( "svgOutlineColor", mSvgOutlineColor.name() );
  map.insert( "svgOutlineWidth", QString::number( mSvgOutlineWidth ) );

  //units
  map["pattern_width_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mPatternWidthUnit );
  map["svg_outline_width_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mSvgOutlineWidthUnit );
  map["outline_width_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mOutlineWidthUnit );

  //data defined properties
  if ( mWidthExpression )
  {
    map["width_expression"] = mWidthExpression->dump();
  }
  if ( mSvgFileExpression )
  {
    map["svgFile_expression"] = mSvgFileExpression->dump();
  }
  if ( mAngleExpression )
  {
    map["angle_expression"] = mAngleExpression->dump();
  }
  if ( mFillColorExpression )
  {
    map["svgFillColor_expression"] = mFillColorExpression->dump();
  }
  if ( mOutlineColorExpression )
  {
    map["svgOutlineColor_expression"] = mOutlineColorExpression->dump();
  }
  if ( mOutlineWidthExpression )
  {
    map["svgOutlineWidth_expression"] = mOutlineWidthExpression->dump();
  }

  return map;
}

QgsSymbolLayerV2* QgsSVGFillSymbolLayer::clone() const
{
  QgsSVGFillSymbolLayer* clonedLayer = 0;
  if ( !mSvgFilePath.isEmpty() )
  {
    clonedLayer = new QgsSVGFillSymbolLayer( mSvgFilePath, mPatternWidth, mAngle );
    clonedLayer->setSvgFillColor( mSvgFillColor );
    clonedLayer->setSvgOutlineColor( mSvgOutlineColor );
    clonedLayer->setSvgOutlineWidth( mSvgOutlineWidth );
  }
  else
  {
    clonedLayer = new QgsSVGFillSymbolLayer( mSvgData, mPatternWidth, mAngle );
  }

  clonedLayer->setPatternWidthUnit( mPatternWidthUnit );
  clonedLayer->setSvgOutlineWidthUnit( mSvgOutlineWidthUnit );
  clonedLayer->setOutlineWidthUnit( mOutlineWidthUnit );

  if ( mOutline )
  {
    clonedLayer->setSubSymbol( mOutline->clone() );
  }

  //data defined properties
  if ( mWidthExpression )
  {
    clonedLayer->setDataDefinedProperty( "width", mWidthExpression->dump() );
  }
  if ( mSvgFileExpression )
  {
    clonedLayer->setDataDefinedProperty( "svgFile", mSvgFileExpression->dump() );
  }
  if ( mAngleExpression )
  {
    clonedLayer->setDataDefinedProperty( "angle", mAngleExpression->dump() );
  }
  if ( mFillColorExpression )
  {
    clonedLayer->setDataDefinedProperty( "svgFillColor", mFillColorExpression->dump() );
  }
  if ( mOutlineColorExpression )
  {
    clonedLayer->setDataDefinedProperty( "svgOutlineColor", mOutlineColorExpression->dump() );
  }
  if ( mOutlineWidthExpression )
  {
    clonedLayer->setDataDefinedProperty( "svgOutlineWidth", mOutlineWidthExpression->dump() );
  }

  return clonedLayer;
}

void QgsSVGFillSymbolLayer::toSld( QDomDocument &doc, QDomElement &element, QgsStringMap props ) const
{
  QDomElement symbolizerElem = doc.createElement( "se:PolygonSymbolizer" );
  if ( !props.value( "uom", "" ).isEmpty() )
    symbolizerElem.setAttribute( "uom", props.value( "uom", "" ) );
  element.appendChild( symbolizerElem );

  QgsSymbolLayerV2Utils::createGeometryElement( doc, symbolizerElem, props.value( "geom", "" ) );

  QDomElement fillElem = doc.createElement( "se:Fill" );
  symbolizerElem.appendChild( fillElem );

  QDomElement graphicFillElem = doc.createElement( "se:GraphicFill" );
  fillElem.appendChild( graphicFillElem );

  QDomElement graphicElem = doc.createElement( "se:Graphic" );
  graphicFillElem.appendChild( graphicElem );

  if ( !mSvgFilePath.isEmpty() )
  {
    QgsSymbolLayerV2Utils::externalGraphicToSld( doc, graphicElem, mSvgFilePath, "image/svg+xml", mSvgFillColor, mPatternWidth );
  }
  else
  {
    // TODO: create svg from data
    // <se:InlineContent>
    symbolizerElem.appendChild( doc.createComment( "SVG from data not implemented yet" ) );
  }

  if ( mSvgOutlineColor.isValid() || mSvgOutlineWidth >= 0 )
  {
    QgsSymbolLayerV2Utils::lineToSld( doc, graphicElem, Qt::SolidLine, mSvgOutlineColor, mSvgOutlineWidth );
  }

  // <Rotation>
  QString angleFunc;
  bool ok;
  double angle = props.value( "angle", "0" ).toDouble( &ok );
  if ( !ok )
  {
    angleFunc = QString( "%1 + %2" ).arg( props.value( "angle", "0" ) ).arg( mAngle );
  }
  else if ( angle + mAngle != 0 )
  {
    angleFunc = QString::number( angle + mAngle );
  }
  QgsSymbolLayerV2Utils::createRotationElement( doc, graphicElem, angleFunc );

  if ( mOutline )
  {
    // the outline sub symbol should be stored within the Stroke element,
    // but it will be stored in a separated LineSymbolizer because it could
    // have more than one layer
    mOutline->toSld( doc, element, props );
  }
}

QgsSymbolLayerV2* QgsSVGFillSymbolLayer::createFromSld( QDomElement &element )
{
  QgsDebugMsg( "Entered." );

  QString path, mimeType;
  QColor fillColor, borderColor;
  Qt::PenStyle penStyle;
  double size, borderWidth;

  QDomElement fillElem = element.firstChildElement( "Fill" );
  if ( fillElem.isNull() )
    return NULL;

  QDomElement graphicFillElem = fillElem.firstChildElement( "GraphicFill" );
  if ( graphicFillElem.isNull() )
    return NULL;

  QDomElement graphicElem = graphicFillElem.firstChildElement( "Graphic" );
  if ( graphicElem.isNull() )
    return NULL;

  if ( !QgsSymbolLayerV2Utils::externalGraphicFromSld( graphicElem, path, mimeType, fillColor, size ) )
    return NULL;

  if ( mimeType != "image/svg+xml" )
    return NULL;

  QgsSymbolLayerV2Utils::lineFromSld( graphicElem, penStyle, borderColor, borderWidth );

  double angle = 0.0;
  QString angleFunc;
  if ( QgsSymbolLayerV2Utils::rotationFromSldElement( graphicElem, angleFunc ) )
  {
    bool ok;
    double d = angleFunc.toDouble( &ok );
    if ( ok )
      angle = d;
  }

  QgsSVGFillSymbolLayer* sl = new QgsSVGFillSymbolLayer( path, size, angle );
  sl->setSvgFillColor( fillColor );
  sl->setSvgOutlineColor( borderColor );
  sl->setSvgOutlineWidth( borderWidth );

  // try to get the outline
  QDomElement strokeElem = element.firstChildElement( "Stroke" );
  if ( !strokeElem.isNull() )
  {
    QgsSymbolLayerV2 *l = QgsSymbolLayerV2Utils::createLineLayerFromSld( strokeElem );
    if ( l )
    {
      QgsSymbolLayerV2List layers;
      layers.append( l );
      sl->setSubSymbol( new QgsLineSymbolV2( layers ) );
    }
  }

  return sl;
}

const QgsExpression* QgsSVGFillSymbolLayer::dataDefinedProperty( const QString& property ) const
{
  if ( property == "width" )
  {
    return mWidthExpression;
  }
  else if ( property == "svgFile" )
  {
    return mSvgFileExpression;
  }
  else if ( property == "angle" )
  {
    return mAngleExpression;
  }
  else if ( property == "svgFillColor" )
  {
    return mFillColorExpression;
  }
  else if ( property == "svgOutlineColor" )
  {
    return mOutlineColorExpression;
  }
  else if ( property == "svgOutlineWidth" )
  {
    return mOutlineWidthExpression;
  }
  return 0;
}

QString QgsSVGFillSymbolLayer::dataDefinedPropertyString( const QString& property ) const
{
  const QgsExpression* ex = dataDefinedProperty( property );
  return ex ? ex->dump() : QString();
}

void QgsSVGFillSymbolLayer::setDataDefinedProperty( const QString& property, const QString& expressionString )
{
  if ( property == "width" )
  {
    delete mWidthExpression; mWidthExpression = new QgsExpression( expressionString );
  }
  else if ( property == "svgFile" )
  {
    delete  mSvgFileExpression; mSvgFileExpression = new QgsExpression( expressionString );
  }
  else if ( property == "angle" )
  {
    delete mAngleExpression; mAngleExpression = new QgsExpression( expressionString );
  }
  else if ( property == "svgFillColor" )
  {
    delete mFillColorExpression; mFillColorExpression = new QgsExpression( expressionString );
  }
  else if ( property == "svgOutlineColor" )
  {
    delete mOutlineColorExpression; mOutlineColorExpression = new QgsExpression( expressionString );
  }
  else if ( property == "svgOutlineWidth" )
  {
    delete mOutlineWidthExpression; mOutlineWidthExpression = new QgsExpression( expressionString );
  }
}

void QgsSVGFillSymbolLayer::removeDataDefinedProperty( const QString& property )
{
  if ( property == "width" )
  {
    delete mWidthExpression; mWidthExpression = 0;
  }
  else if ( property == "svgFile" )
  {
    delete  mSvgFileExpression; mSvgFileExpression = 0;
  }
  else if ( property == "angle" )
  {
    delete mAngleExpression; mAngleExpression = 0;
  }
  else if ( property == "svgFillColor" )
  {
    delete mFillColorExpression; mFillColorExpression = 0;
  }
  else if ( property == "svgOutlineColor" )
  {
    delete mOutlineColorExpression; mOutlineColorExpression = 0;
  }
  else if ( property == "svgOutlineWidth" )
  {
    delete mOutlineWidthExpression; mOutlineWidthExpression = 0;
  }
}

void QgsSVGFillSymbolLayer::removeDataDefinedProperties()
{
  delete mWidthExpression; mWidthExpression = 0;
  delete mSvgFileExpression; mSvgFileExpression = 0;
  delete mAngleExpression; mAngleExpression = 0;
  delete mFillColorExpression; mFillColorExpression = 0;
  delete mOutlineColorExpression; mOutlineColorExpression = 0;
  delete mOutlineWidthExpression; mOutlineWidthExpression = 0;
}

QSet<QString> QgsSVGFillSymbolLayer::usedAttributes() const
{
  QSet<QString> attributes;

  //add data defined attributes
  QStringList columns;
  if ( mWidthExpression )
    columns.append( mWidthExpression->referencedColumns() );
  if ( mSvgFileExpression )
    columns.append( mSvgFileExpression->referencedColumns() );
  if ( mAngleExpression )
    columns.append( mAngleExpression->referencedColumns() );
  if ( mFillColorExpression )
    columns.append( mFillColorExpression->referencedColumns() );
  if ( mOutlineColorExpression )
    columns.append( mOutlineColorExpression->referencedColumns() );
  if ( mOutlineWidthExpression )
    columns.append( mOutlineWidthExpression->referencedColumns() );

  QStringList::const_iterator it = columns.constBegin();
  for ( ; it != columns.constEnd(); ++it )
  {
    attributes.insert( *it );
  }
  return attributes;
}

void QgsSVGFillSymbolLayer::applyDataDefinedSettings( const QgsSymbolV2RenderContext& context )
{
  if ( !mWidthExpression && !mSvgFileExpression && !mFillColorExpression && !mOutlineColorExpression && !mOutlineWidthExpression )
  {
    return; //no data defined settings
  }

  double width = mPatternWidth;
  if ( mWidthExpression )
  {
    width = mWidthExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  QString svgFile = mSvgFilePath;
  if ( mSvgFileExpression )
  {
    svgFile = mSvgFileExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toString();
  }
  QColor svgFillColor = mSvgFillColor;
  if ( mFillColorExpression )
  {
    svgFillColor = QgsSymbolLayerV2Utils::decodeColor( mFillColorExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toString() );
  }
  QColor svgOutlineColor = mSvgOutlineColor;
  if ( mOutlineColorExpression )
  {
    svgOutlineColor = QgsSymbolLayerV2Utils::decodeColor( mOutlineColorExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toString() );
  }
  double outlineWidth = mSvgOutlineWidth;
  if ( mOutlineWidthExpression )
  {
    outlineWidth = mOutlineWidthExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  applyPattern( mBrush, svgFile, width, mPatternWidthUnit, svgFillColor, svgOutlineColor, outlineWidth,
                mSvgOutlineWidthUnit, context );

}

void QgsSVGFillSymbolLayer::storeViewBox()
{
  if ( !mSvgData.isEmpty() )
  {
    QSvgRenderer r( mSvgData );
    if ( r.isValid() )
    {
      mSvgViewBox = r.viewBoxF();
      return;
    }
  }

  mSvgViewBox = QRectF();
  return;
}

void QgsSVGFillSymbolLayer::setDefaultSvgParams()
{
  //default values
  mSvgFillColor = QColor( 0, 0, 0 );
  mSvgOutlineColor = QColor( 0, 0, 0 );
  mSvgOutlineWidth = 0.3;

  if ( mSvgFilePath.isEmpty() )
  {
    return;
  }

  bool hasFillParam, hasOutlineParam, hasOutlineWidthParam;
  QColor defaultFillColor, defaultOutlineColor;
  double defaultOutlineWidth;
  QgsSvgCache::instance()->containsParams( mSvgFilePath, hasFillParam, defaultFillColor, hasOutlineParam, defaultOutlineColor, hasOutlineWidthParam,
      defaultOutlineWidth );

  if ( hasFillParam )
  {
    mSvgFillColor = defaultFillColor;
  }
  if ( hasOutlineParam )
  {
    mSvgOutlineColor = defaultOutlineColor;
  }
  if ( hasOutlineWidthParam )
  {
    mSvgOutlineWidth = defaultOutlineWidth;
  }
}

void QgsSVGFillSymbolLayer::prepareExpressions( const QgsVectorLayer* vl )
{
  if ( !vl )
  {
    return;
  }

  const QgsFields& fields = vl->pendingFields();
  if ( mWidthExpression )
    mWidthExpression->prepare( fields );
  if ( mSvgFileExpression )
    mSvgFileExpression->prepare( fields );
  if ( mAngleExpression )
    mAngleExpression->prepare( fields );
  if ( mFillColorExpression )
    mFillColorExpression->prepare( fields );
  if ( mOutlineColorExpression )
    mOutlineColorExpression->prepare( fields );
  if ( mOutlineWidthExpression )
    mOutlineWidthExpression->prepare( fields );
}

QgsLinePatternFillSymbolLayer::QgsLinePatternFillSymbolLayer(): QgsImageFillSymbolLayer(), mDistanceUnit( QgsSymbolV2::MM ), mLineWidthUnit( QgsSymbolV2::MM ),
    mOffsetUnit( QgsSymbolV2::MM ), mLineAngleExpression( 0 ), mDistanceExpression( 0 ), mLineWidthExpression( 0 ), mColorExpression( 0 )
{
}

QgsLinePatternFillSymbolLayer::~QgsLinePatternFillSymbolLayer()
{
}

void QgsLinePatternFillSymbolLayer::setOutputUnit( QgsSymbolV2::OutputUnit unit )
{
  mDistanceUnit = unit;
  mLineWidthUnit = unit;
  mOffsetUnit = unit;
}

QgsSymbolV2::OutputUnit QgsLinePatternFillSymbolLayer::outputUnit() const
{
  QgsSymbolV2::OutputUnit unit = mDistanceUnit;
  if ( mLineWidthUnit != unit || mOffsetUnit != unit )
  {
    return QgsSymbolV2::Mixed;
  }
  return unit;
}

QgsSymbolLayerV2* QgsLinePatternFillSymbolLayer::create( const QgsStringMap& properties )
{
  QgsLinePatternFillSymbolLayer* patternLayer = new QgsLinePatternFillSymbolLayer();

  //default values
  double lineAngle = 45;
  double distance = 5;
  double lineWidth = 0.5;
  QColor color( Qt::black );
  double offset = 0.0;

  if ( properties.contains( "lineangle" ) )
  {
    lineAngle = properties["lineangle"].toDouble();
  }
  patternLayer->setLineAngle( lineAngle );

  if ( properties.contains( "distance" ) )
  {
    distance = properties["distance"].toDouble();
  }
  patternLayer->setDistance( distance );

  if ( properties.contains( "linewidth" ) )
  {
    lineWidth = properties["linewidth"].toDouble();
  }
  patternLayer->setLineWidth( lineWidth );

  if ( properties.contains( "color" ) )
  {
    color = QgsSymbolLayerV2Utils::decodeColor( properties["color"] );
  }
  patternLayer->setColor( color );

  if ( properties.contains( "offset" ) )
  {
    offset = properties["offset"].toDouble();
  }
  patternLayer->setOffset( offset );


  if ( properties.contains( "distance_unit" ) )
  {
    patternLayer->setDistanceUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["distance_unit"] ) );
  }
  if ( properties.contains( "line_width_unit" ) )
  {
    patternLayer->setLineWidthUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["line_width_unit"] ) );
  }
  if ( properties.contains( "offset_unit" ) )
  {
    patternLayer->setOffsetUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["offset_unit"] ) );
  }

  //data defined properties
  if ( properties.contains( "lineangle_expression" ) )
  {
    patternLayer->setDataDefinedProperty( "lineangle", properties["lineangle_expression"] );
  }
  if ( properties.contains( "distance_expression" ) )
  {
    patternLayer->setDataDefinedProperty( "distance", properties["distance_expression"] );
  }
  if ( properties.contains( "linewidth_expression" ) )
  {
    patternLayer->setDataDefinedProperty( "linewidth", properties["linewidth_expression"] );
  }
  if ( properties.contains( "color_expression" ) )
  {
    patternLayer->setDataDefinedProperty( "color", properties["color_expression"] );
  }
  return patternLayer;
}

QString QgsLinePatternFillSymbolLayer::layerType() const
{
  return "LinePatternFill";
}

void QgsLinePatternFillSymbolLayer::applyPattern( const QgsSymbolV2RenderContext& context, QBrush& brush, double lineAngle, double distance,
    double lineWidth, const QColor& color )
{
  const QgsRenderContext& ctx = context.renderContext();
  double outlinePixelWidth = lineWidth * QgsSymbolLayerV2Utils::pixelSizeScaleFactor( ctx,  mLineWidthUnit );
  double outputPixelDist = distance * QgsSymbolLayerV2Utils::pixelSizeScaleFactor( ctx, mDistanceUnit );
  double outputPixelOffset = mOffset * QgsSymbolLayerV2Utils::pixelSizeScaleFactor( ctx,  mOffsetUnit );

  //create image
  int height, width;
  if ( doubleNear( lineAngle, 0 ) || doubleNear( lineAngle, 360 ) || doubleNear( lineAngle, 90 ) || doubleNear( lineAngle, 180 ) || doubleNear( lineAngle, 270 ) )
  {
    height = outputPixelDist;
    width = height; //width can be set to arbitrary value
  }
  else
  {
    height = qAbs( outputPixelDist / cos( lineAngle * M_PI / 180 ) ); //keep perpendicular distance between lines constant
    width = qAbs( height / tan( lineAngle * M_PI / 180 ) );
  }

  //depending on the angle, we might need to render into a larger image and use a subset of it
  int dx = 0;
  int dy = 0;

  if ( width > 10000 || height > 10000 ) //protect symbol layer from eating too much memory
  {
    QImage img;
    mBrush.setTextureImage( img );
    return;
  }

  QImage patternImage( width, height, QImage::Format_ARGB32 );
  patternImage.fill( 0 );
  QPainter p( &patternImage );

  p.setRenderHint( QPainter::Antialiasing, true );
  QPen pen( color );
  pen.setWidthF( outlinePixelWidth );
  pen.setCapStyle( Qt::FlatCap );
  p.setPen( pen );

  QPoint p1, p2, p3, p4, p5, p6;
  if ( doubleNear( lineAngle, 0.0 ) || doubleNear( lineAngle, 360.0 ) || doubleNear( lineAngle, 180.0 ) )
  {
    p1 = QPoint( 0, height );
    p2 = QPoint( width, height );
    p3 = QPoint( 0, 0 );
    p4 = QPoint( width, 0 );
    p5 = QPoint( 0, 2 * height );
    p6 = QPoint( width, 2 * height );
  }
  else if ( doubleNear( lineAngle, 90.0 ) || doubleNear( lineAngle, 270.0 ) )
  {
    p1 = QPoint( 0, height );
    p2 = QPoint( 0, 0 );
    p3 = QPoint( width, height );
    p4 = QPoint( width, 0 );
    p5 = QPoint( -width, height );
    p6 = QPoint( -width, 0 );
  }
  else if (( lineAngle > 0 && lineAngle < 90 ) || ( lineAngle > 180 && lineAngle < 270 ) )
  {
    dx = outputPixelDist * cos(( 90 - lineAngle ) * M_PI / 180.0 );
    dy = outputPixelDist * sin(( 90 - lineAngle ) * M_PI / 180.0 );
    p1 = QPoint( 0, height );
    p2 = QPoint( width, 0 );
    p3 = QPoint( -dx, height - dy );
    p4 = QPoint( width - dx, -dy ); //p4 = QPoint( p3.x() + width, p3.y() - height );
    p5 = QPoint( dx, height + dy );
    p6 = QPoint( width + dx, dy ); //p6 = QPoint( p5.x() + width, p5.y() - height );
  }
  else if (( lineAngle < 180 ) || ( lineAngle > 270 && lineAngle < 360 ) )
  {
    dy = outputPixelDist * cos(( 180 - lineAngle ) * M_PI / 180 );
    dx = outputPixelDist * sin(( 180 - lineAngle ) * M_PI / 180 );
    p1 = QPoint( width, height );
    p2 = QPoint( 0, 0 );
    p5 = QPoint( width + dx, height - dy );
    p6 = QPoint( p5.x() - width, p5.y() - height ); //p6 = QPoint( dx, -dy );
    p3 = QPoint( width - dx, height + dy );
    p4 = QPoint( p3.x() - width, p3.y() - height ); //p4 = QPoint( -dx, dy );
  }

  if ( !doubleNear( mOffset, 0.0 ) ) //shift everything
  {
    QPointF tempPt;
    tempPt = QgsSymbolLayerV2Utils::pointOnLineWithDistance( p1, p3, outputPixelDist + outputPixelOffset );
    p3 = QPoint( tempPt.x(), tempPt.y() );
    tempPt = QgsSymbolLayerV2Utils::pointOnLineWithDistance( p2, p4, outputPixelDist + outputPixelOffset );
    p4 = QPoint( tempPt.x(), tempPt.y() );
    tempPt = QgsSymbolLayerV2Utils::pointOnLineWithDistance( p1, p5, outputPixelDist - outputPixelOffset );
    p5 = QPoint( tempPt.x(), tempPt.y() );
    tempPt = QgsSymbolLayerV2Utils::pointOnLineWithDistance( p2, p6, outputPixelDist - outputPixelOffset );
    p6 = QPoint( tempPt.x(), tempPt.y() );

    //update p1, p2 last
    tempPt = QgsSymbolLayerV2Utils::pointOnLineWithDistance( p1, p3, outputPixelOffset ).toPoint();
    p1 = QPoint( tempPt.x(), tempPt.y() );
    tempPt = QgsSymbolLayerV2Utils::pointOnLineWithDistance( p2, p4, outputPixelOffset ).toPoint();
    p2 = QPoint( tempPt.x(), tempPt.y() );;
  }

  p.drawLine( p1, p2 );
  p.drawLine( p3, p4 );
  p.drawLine( p5, p6 );
  p.end();

  //set image to mBrush
  if ( !doubleNear( context.alpha(), 1.0 ) )
  {
    QImage transparentImage = patternImage.copy();
    QgsSymbolLayerV2Utils::multiplyImageOpacity( &transparentImage, context.alpha() );
    brush.setTextureImage( transparentImage );
  }
  else
  {
    brush.setTextureImage( patternImage );
  }

  QTransform brushTransform;
  brushTransform.scale( 1.0 / context.renderContext().rasterScaleFactor(), 1.0 / context.renderContext().rasterScaleFactor() );
  brush.setTransform( brushTransform );
}

void QgsLinePatternFillSymbolLayer::startRender( QgsSymbolV2RenderContext& context )
{
  applyPattern( context, mBrush, mLineAngle, mDistance, mLineWidth, mColor );

  if ( mOutline )
  {
    mOutline->startRender( context.renderContext() );
  }

  prepareExpressions( context.layer() );
}

void QgsLinePatternFillSymbolLayer::stopRender( QgsSymbolV2RenderContext & )
{
}

QgsStringMap QgsLinePatternFillSymbolLayer::properties() const
{
  QgsStringMap map;
  map.insert( "lineangle", QString::number( mLineAngle ) );
  map.insert( "distance", QString::number( mDistance ) );
  map.insert( "linewidth", QString::number( mLineWidth ) );
  map.insert( "color", QgsSymbolLayerV2Utils::encodeColor( mColor ) );
  map.insert( "offset", QString::number( mOffset ) );
  map.insert( "distance_unit", QgsSymbolLayerV2Utils::encodeOutputUnit( mDistanceUnit ) );
  map.insert( "line_width_unit", QgsSymbolLayerV2Utils::encodeOutputUnit( mLineWidthUnit ) );
  map.insert( "offset_unit", QgsSymbolLayerV2Utils::encodeOutputUnit( mOffsetUnit ) );

  //data defined properties
  if ( mLineAngleExpression )
  {
    map.insert( "lineangle_expression", mLineAngleExpression->dump() );
  }
  if ( mDistanceExpression )
  {
    map.insert( "distance_expression", mDistanceExpression->dump() );
  }
  if ( mLineWidthExpression )
  {
    map.insert( "linewidth_expression", mLineWidthExpression->dump() );
  }
  if ( mColorExpression )
  {
    map.insert( "color_expression", mColorExpression->dump() );
  }

  return map;
}

QgsSymbolLayerV2* QgsLinePatternFillSymbolLayer::clone() const
{
  QgsLinePatternFillSymbolLayer* clonedLayer = static_cast<QgsLinePatternFillSymbolLayer*>( QgsLinePatternFillSymbolLayer::create( properties() ) );
  if ( mOutline )
  {
    clonedLayer->setSubSymbol( mOutline->clone() );
  }
  clonedLayer->setDistanceUnit( mDistanceUnit );
  clonedLayer->setLineWidthUnit( mLineWidthUnit );
  clonedLayer->setOffsetUnit( mOffsetUnit );

  if ( mLineAngleExpression )
  {
    clonedLayer->setDataDefinedProperty( "lineangle", mLineAngleExpression->dump() );
  }
  if ( mDistanceExpression )
  {
    clonedLayer->setDataDefinedProperty( "distance", mDistanceExpression->dump() );
  }
  if ( mLineWidthExpression )
  {
    clonedLayer->setDataDefinedProperty( "linewidth", mLineWidthExpression->dump() );
  }
  if ( mColorExpression )
  {
    clonedLayer->setDataDefinedProperty( "color", mColorExpression->dump() );
  }

  return clonedLayer;
}

void QgsLinePatternFillSymbolLayer::toSld( QDomDocument &doc, QDomElement &element, QgsStringMap props ) const
{
  QDomElement symbolizerElem = doc.createElement( "se:PolygonSymbolizer" );
  if ( !props.value( "uom", "" ).isEmpty() )
    symbolizerElem.setAttribute( "uom", props.value( "uom", "" ) );
  element.appendChild( symbolizerElem );

  // <Geometry>
  QgsSymbolLayerV2Utils::createGeometryElement( doc, symbolizerElem, props.value( "geom", "" ) );

  QDomElement fillElem = doc.createElement( "se:Fill" );
  symbolizerElem.appendChild( fillElem );

  QDomElement graphicFillElem = doc.createElement( "se:GraphicFill" );
  fillElem.appendChild( graphicFillElem );

  QDomElement graphicElem = doc.createElement( "se:Graphic" );
  graphicFillElem.appendChild( graphicElem );

  QgsSymbolLayerV2Utils::wellKnownMarkerToSld( doc, graphicElem, "horline", QColor(), mColor, mLineWidth, mDistance );

  // <Rotation>
  QString angleFunc;
  bool ok;
  double angle = props.value( "angle", "0" ).toDouble( &ok );
  if ( !ok )
  {
    angleFunc = QString( "%1 + %2" ).arg( props.value( "angle", "0" ) ).arg( mLineAngle );
  }
  else if ( angle + mLineAngle != 0 )
  {
    angleFunc = QString::number( angle + mLineAngle );
  }
  QgsSymbolLayerV2Utils::createRotationElement( doc, graphicElem, angleFunc );

  // <se:Displacement>
  QPointF lineOffset( sin( mLineAngle ) * mOffset, cos( mLineAngle ) * mOffset );
  QgsSymbolLayerV2Utils::createDisplacementElement( doc, graphicElem, lineOffset );

  if ( mOutline )
  {
    // the outline sub symbol should be stored within the Stroke element,
    // but it will be stored in a separated LineSymbolizer because it could
    // have more than one layer
    mOutline->toSld( doc, element, props );
  }
}

QString QgsLinePatternFillSymbolLayer::ogrFeatureStyleWidth( double widthScaleFactor ) const
{
  QString featureStyle;
  featureStyle.append( "Brush(" );
  featureStyle.append( QString( "fc:%1" ).arg( mColor.name() ) );
  featureStyle.append( QString( ",bc:%1" ).arg( "#00000000" ) ); //transparent background
  featureStyle.append( ",id:\"ogr-brush-2\"" );
  featureStyle.append( QString( ",a:%1" ).arg( mLineAngle ) );
  featureStyle.append( QString( ",s:%1" ).arg( mLineWidth * widthScaleFactor ) );
  featureStyle.append( ",dx:0mm" );
  featureStyle.append( QString( ",dy:%1mm" ).arg( mDistance * widthScaleFactor ) );
  featureStyle.append( ")" );
  return featureStyle;
}

const QgsExpression* QgsLinePatternFillSymbolLayer::dataDefinedProperty( const QString& property ) const
{
  if ( property == "lineangle" )
  {
    return mLineAngleExpression;
  }
  else if ( property == "distance" )
  {
    return mDistanceExpression;
  }
  else if ( property == "linewidth" )
  {
    return mLineWidthExpression;
  }
  else if ( property == "color" )
  {
    return mColorExpression;
  }
  return 0;
}

QString QgsLinePatternFillSymbolLayer::dataDefinedPropertyString( const QString& property ) const
{
  const QgsExpression* ex = dataDefinedProperty( property );
  return ex ? ex->dump() : QString();
}

void QgsLinePatternFillSymbolLayer::setDataDefinedProperty( const QString& property, const QString& expressionString )
{
  if ( property == "lineangle" )
  {
    delete mLineAngleExpression; mLineAngleExpression = new QgsExpression( expressionString );
  }
  else if ( property == "distance" )
  {
    delete mDistanceExpression; mDistanceExpression = new QgsExpression( expressionString );
  }
  else if ( property == "linewidth" )
  {
    delete mLineWidthExpression; mLineWidthExpression = new QgsExpression( expressionString );
  }
  else if ( property == "color" )
  {
    delete mColorExpression; mColorExpression = new QgsExpression( expressionString );
  }
}

void QgsLinePatternFillSymbolLayer::removeDataDefinedProperty( const QString& property )
{
  if ( property == "lineangle" )
  {
    delete mLineAngleExpression; mLineAngleExpression = 0;
  }
  else if ( property == "distance" )
  {
    delete mDistanceExpression; mDistanceExpression = 0;
  }
  else if ( property == "linewidth" )
  {
    delete mLineWidthExpression; mLineWidthExpression = 0;
  }
  else if ( property == "color" )
  {
    delete mColorExpression; mColorExpression = 0;
  }
}

void QgsLinePatternFillSymbolLayer::removeDataDefinedProperties()
{
  delete mLineAngleExpression; mLineAngleExpression = 0;
  delete mDistanceExpression; mDistanceExpression = 0;
  delete mLineWidthExpression; mLineWidthExpression = 0;
  delete mColorExpression; mColorExpression = 0;
}

QSet<QString> QgsLinePatternFillSymbolLayer::usedAttributes() const
{
  QSet<QString> attributes;

  //add data defined attributes
  QStringList columns;
  if ( mLineAngleExpression )
    columns.append( mLineAngleExpression->referencedColumns() );
  if ( mDistanceExpression )
    columns.append( mDistanceExpression->referencedColumns() );
  if ( mLineWidthExpression )
    columns.append( mLineWidthExpression->referencedColumns() );
  if ( mColorExpression )
    columns.append( mColorExpression->referencedColumns() );

  QStringList::const_iterator it = columns.constBegin();
  for ( ; it != columns.constEnd(); ++it )
  {
    attributes.insert( *it );
  }
  return attributes;
}

void QgsLinePatternFillSymbolLayer::applyDataDefinedSettings( const QgsSymbolV2RenderContext& context )
{
  if ( !mLineAngleExpression && !mDistanceExpression && !mLineWidthExpression && !mColorExpression )
  {
    return; //no data defined settings
  }

  double lineAngle = mLineAngle;
  if ( mLineAngleExpression )
  {
    lineAngle = mLineAngleExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  double distance = mDistance;
  if ( mDistanceExpression )
  {
    distance = mDistanceExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  double lineWidth = mLineWidth;
  if ( mLineWidthExpression )
  {
    lineWidth = mLineWidthExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  QColor color = mColor;
  if ( mColorExpression )
  {
    color = QgsSymbolLayerV2Utils::decodeColor( mColorExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toString() );
  }
  applyPattern( context, mBrush, lineAngle, distance, lineWidth, color );
}

void QgsLinePatternFillSymbolLayer::prepareExpressions( const QgsVectorLayer* vl )
{
  if ( !vl )
  {
    return;
  }

  const QgsFields& fields = vl->pendingFields();
  if ( mLineAngleExpression )
    mLineAngleExpression->prepare( fields );
  if ( mDistanceExpression )
    mDistanceExpression->prepare( fields );
  if ( mLineWidthExpression )
    mLineWidthExpression->prepare( fields );
  if ( mColorExpression )
    mColorExpression->prepare( fields );
}

QgsSymbolLayerV2* QgsLinePatternFillSymbolLayer::createFromSld( QDomElement &element )
{
  QgsDebugMsg( "Entered." );

  QString name;
  QColor fillColor, lineColor;
  double size, lineWidth;

  QDomElement fillElem = element.firstChildElement( "Fill" );
  if ( fillElem.isNull() )
    return NULL;

  QDomElement graphicFillElem = fillElem.firstChildElement( "GraphicFill" );
  if ( graphicFillElem.isNull() )
    return NULL;

  QDomElement graphicElem = graphicFillElem.firstChildElement( "Graphic" );
  if ( graphicElem.isNull() )
    return NULL;

  if ( !QgsSymbolLayerV2Utils::wellKnownMarkerFromSld( graphicElem, name, fillColor, lineColor, lineWidth, size ) )
    return NULL;

  if ( name != "horline" )
    return NULL;

  double angle = 0.0;
  QString angleFunc;
  if ( QgsSymbolLayerV2Utils::rotationFromSldElement( graphicElem, angleFunc ) )
  {
    bool ok;
    double d = angleFunc.toDouble( &ok );
    if ( ok )
      angle = d;
  }

  double offset = 0.0;
  QPointF vectOffset;
  if ( QgsSymbolLayerV2Utils::displacementFromSldElement( graphicElem, vectOffset ) )
  {
    offset = sqrt( pow( vectOffset.x(), 2 ) + pow( vectOffset.y(), 2 ) );
  }

  QgsLinePatternFillSymbolLayer* sl = new QgsLinePatternFillSymbolLayer();
  sl->setColor( lineColor );
  sl->setLineWidth( lineWidth );
  sl->setLineAngle( angle );
  sl->setOffset( offset );
  sl->setDistance( size );

  // try to get the outline
  QDomElement strokeElem = element.firstChildElement( "Stroke" );
  if ( !strokeElem.isNull() )
  {
    QgsSymbolLayerV2 *l = QgsSymbolLayerV2Utils::createLineLayerFromSld( strokeElem );
    if ( l )
    {
      QgsSymbolLayerV2List layers;
      layers.append( l );
      sl->setSubSymbol( new QgsLineSymbolV2( layers ) );
    }
  }

  return sl;
}

////////////////////////

QgsPointPatternFillSymbolLayer::QgsPointPatternFillSymbolLayer(): QgsImageFillSymbolLayer(), mMarkerSymbol( 0 ), mDistanceX( 15 ),
    mDistanceXUnit( QgsSymbolV2::MM ), mDistanceY( 15 ), mDistanceYUnit( QgsSymbolV2::MM ), mDisplacementX( 0 ), mDisplacementXUnit( QgsSymbolV2::MM ),
    mDisplacementY( 0 ), mDisplacementYUnit( QgsSymbolV2::MM ), mDistanceXExpression( 0 ), mDistanceYExpression( 0 ),
    mDisplacementXExpression( 0 ), mDisplacementYExpression( 0 )
{
  mDistanceX = 15;
  mDistanceY = 15;
  mDisplacementX = 0;
  mDisplacementY = 0;
  setSubSymbol( new QgsMarkerSymbolV2() );
  QgsImageFillSymbolLayer::setSubSymbol( 0 ); //no outline
}

QgsPointPatternFillSymbolLayer::~QgsPointPatternFillSymbolLayer()
{
}

void QgsPointPatternFillSymbolLayer::setOutputUnit( QgsSymbolV2::OutputUnit unit )
{
  mDistanceXUnit = unit;
  mDistanceYUnit = unit;
  mDisplacementXUnit = unit;
  mDisplacementYUnit = unit;
}

QgsSymbolV2::OutputUnit QgsPointPatternFillSymbolLayer::outputUnit() const
{
  QgsSymbolV2::OutputUnit unit = mDistanceXUnit;
  if ( mDistanceYUnit != unit || mDisplacementXUnit != unit || mDisplacementYUnit != unit )
  {
    return QgsSymbolV2::Mixed;
  }
  return unit;
}

QgsSymbolLayerV2* QgsPointPatternFillSymbolLayer::create( const QgsStringMap& properties )
{
  QgsPointPatternFillSymbolLayer* layer = new QgsPointPatternFillSymbolLayer();
  if ( properties.contains( "distance_x" ) )
  {
    layer->setDistanceX( properties["distance_x"].toDouble() );
  }
  if ( properties.contains( "distance_y" ) )
  {
    layer->setDistanceY( properties["distance_y"].toDouble() );
  }
  if ( properties.contains( "displacement_x" ) )
  {
    layer->setDisplacementX( properties["displacement_x"].toDouble() );
  }
  if ( properties.contains( "displacement_y" ) )
  {
    layer->setDisplacementY( properties["displacement_y"].toDouble() );
  }

  if ( properties.contains( "distance_x_unit" ) )
  {
    layer->setDistanceXUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["distance_x_unit"] ) );
  }
  if ( properties.contains( "distance_y_unit" ) )
  {
    layer->setDistanceYUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["distance_y_unit"] ) );
  }
  if ( properties.contains( "displacement_x_unit" ) )
  {
    layer->setDisplacementXUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["displacement_x_unit"] ) );
  }
  if ( properties.contains( "displacement_y_unit" ) )
  {
    layer->setDisplacementYUnit( QgsSymbolLayerV2Utils::decodeOutputUnit( properties["displacement_y_unit"] ) );
  }

  //data defined properties
  if ( properties.contains( "distance_x_expression" ) )
  {
    layer->setDataDefinedProperty( "distance_x", properties["distance_x_expression"] );
  }
  if ( properties.contains( "distance_y_expression" ) )
  {
    layer->setDataDefinedProperty( "distance_y", properties["distance_y_expression"] );
  }
  if ( properties.contains( "displacement_x_expression" ) )
  {
    layer->setDataDefinedProperty( "displacement_x", properties["displacement_x_expression"] );
  }
  if ( properties.contains( "displacement_y_expression" ) )
  {
    layer->setDataDefinedProperty( "displacement_y", properties["displacement_y_expression"] );
  }
  return layer;
}

QString QgsPointPatternFillSymbolLayer::layerType() const
{
  return "PointPatternFill";
}

void QgsPointPatternFillSymbolLayer::applyPattern( const QgsSymbolV2RenderContext& context, QBrush& brush, double distanceX, double distanceY,
    double displacementX, double displacementY )
{
  //render 3 rows and columns in one go to easily incorporate displacement
  const QgsRenderContext& ctx = context.renderContext();
  double width = distanceX * QgsSymbolLayerV2Utils::pixelSizeScaleFactor( ctx, mDistanceXUnit ) * 2.0;
  double height = distanceY * QgsSymbolLayerV2Utils::pixelSizeScaleFactor( ctx, mDistanceYUnit ) * 2.0;

  if ( width > 10000 || height > 10000 ) //protect symbol layer from eating too much memory
  {
    QImage img;
    brush.setTextureImage( img );
    return;
  }

  QImage patternImage( width, height, QImage::Format_ARGB32 );
  patternImage.fill( 0 );

  if ( mMarkerSymbol )
  {
    QPainter p( &patternImage );

    //marker rendering needs context for drawing on patternImage
    QgsRenderContext pointRenderContext;
    pointRenderContext.setPainter( &p );
    pointRenderContext.setRasterScaleFactor( 1.0 );
    pointRenderContext.setScaleFactor( context.renderContext().scaleFactor() * context.renderContext().rasterScaleFactor() );
    QgsMapToPixel mtp( context.renderContext().mapToPixel().mapUnitsPerPixel() / context.renderContext().rasterScaleFactor() );
    pointRenderContext.setMapToPixel( mtp );
    pointRenderContext.setForceVectorOutput( false );

    mMarkerSymbol->startRender( pointRenderContext );

    //render corner points
    mMarkerSymbol->renderPoint( QPointF( 0, 0 ), context.feature(), pointRenderContext );
    mMarkerSymbol->renderPoint( QPointF( width, 0 ), context.feature(), pointRenderContext );
    mMarkerSymbol->renderPoint( QPointF( 0, height ), context.feature(), pointRenderContext );
    mMarkerSymbol->renderPoint( QPointF( width, height ), context.feature(), pointRenderContext );

    //render displaced points
    double displacementPixelX = displacementX * QgsSymbolLayerV2Utils::lineWidthScaleFactor( ctx, mDisplacementXUnit );
    double displacementPixelY = displacementY * QgsSymbolLayerV2Utils::lineWidthScaleFactor( ctx, mDisplacementYUnit );
    mMarkerSymbol->renderPoint( QPointF( width / 2.0, -displacementPixelY ), context.feature(), pointRenderContext );
    mMarkerSymbol->renderPoint( QPointF( displacementPixelX, height / 2.0 ), context.feature(), pointRenderContext );
    mMarkerSymbol->renderPoint( QPointF( width / 2.0 + displacementPixelX, height / 2.0 - displacementPixelY ), context.feature(), pointRenderContext );
    mMarkerSymbol->renderPoint( QPointF( width + displacementPixelX, height / 2.0 ), context.feature(), pointRenderContext );
    mMarkerSymbol->renderPoint( QPointF( width / 2.0, height - displacementPixelY ), context.feature(), pointRenderContext );

    mMarkerSymbol->stopRender( pointRenderContext );
  }

  if ( !doubleNear( context.alpha(), 1.0 ) )
  {
    QImage transparentImage = patternImage.copy();
    QgsSymbolLayerV2Utils::multiplyImageOpacity( &transparentImage, context.alpha() );
    brush.setTextureImage( transparentImage );
  }
  else
  {
    brush.setTextureImage( patternImage );
  }
  QTransform brushTransform;
  brushTransform.scale( 1.0 / context.renderContext().rasterScaleFactor(), 1.0 / context.renderContext().rasterScaleFactor() );
  brush.setTransform( brushTransform );
}

void QgsPointPatternFillSymbolLayer::startRender( QgsSymbolV2RenderContext& context )
{
  applyPattern( context, mBrush, mDistanceX, mDistanceY, mDisplacementX, mDisplacementY );

  if ( mOutline )
  {
    mOutline->startRender( context.renderContext() );
  }
}

void QgsPointPatternFillSymbolLayer::stopRender( QgsSymbolV2RenderContext& context )
{
  if ( mOutline )
  {
    mOutline->stopRender( context.renderContext() );
  }
}

QgsStringMap QgsPointPatternFillSymbolLayer::properties() const
{
  QgsStringMap propertyMap;
  propertyMap["distance_x"] = QString::number( mDistanceX );
  propertyMap["distance_y"] = QString::number( mDistanceY );
  propertyMap["displacement_x"] = QString::number( mDisplacementX );
  propertyMap["displacement_y"] = QString::number( mDisplacementY );
  propertyMap["distance_x_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mDistanceXUnit );
  propertyMap["distance_y_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mDistanceYUnit );
  propertyMap["displacement_x_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mDisplacementXUnit );
  propertyMap["displacement_y_unit"] = QgsSymbolLayerV2Utils::encodeOutputUnit( mDisplacementYUnit );

  //data defined properties
  if ( mDistanceXExpression )
  {
    propertyMap["distance_x_expression"] = mDistanceXExpression->dump();
  }
  if ( mDistanceYExpression )
  {
    propertyMap["distance_y_expression"] = mDistanceYExpression->dump();
  }
  if ( mDisplacementXExpression )
  {
    propertyMap["displacement_x_expression"] = mDisplacementXExpression->dump();
  }
  if ( mDisplacementYExpression )
  {
    propertyMap["displacement_y_expression"] = mDisplacementYExpression->dump();
  }
  return propertyMap;
}

QgsSymbolLayerV2* QgsPointPatternFillSymbolLayer::clone() const
{
  QgsPointPatternFillSymbolLayer* clonedLayer = static_cast<QgsPointPatternFillSymbolLayer*>( QgsPointPatternFillSymbolLayer::create( properties() ) );
  if ( mMarkerSymbol )
  {
    clonedLayer->setSubSymbol( mMarkerSymbol->clone() );
  }
  clonedLayer->setDistanceXUnit( mDistanceXUnit );
  clonedLayer->setDistanceYUnit( mDistanceYUnit );
  clonedLayer->setDisplacementXUnit( mDisplacementXUnit );
  clonedLayer->setDisplacementYUnit( mDisplacementYUnit );
  return clonedLayer;
}

void QgsPointPatternFillSymbolLayer::toSld( QDomDocument &doc, QDomElement &element, QgsStringMap props ) const
{
  for ( int i = 0; i < mMarkerSymbol->symbolLayerCount(); i++ )
  {
    QDomElement symbolizerElem = doc.createElement( "se:PolygonSymbolizer" );
    if ( !props.value( "uom", "" ).isEmpty() )
      symbolizerElem.setAttribute( "uom", props.value( "uom", "" ) );
    element.appendChild( symbolizerElem );

    // <Geometry>
    QgsSymbolLayerV2Utils::createGeometryElement( doc, symbolizerElem, props.value( "geom", "" ) );

    QDomElement fillElem = doc.createElement( "se:Fill" );
    symbolizerElem.appendChild( fillElem );

    QDomElement graphicFillElem = doc.createElement( "se:GraphicFill" );
    fillElem.appendChild( graphicFillElem );

    // store distanceX, distanceY, displacementX, displacementY in a <VendorOption>
    QString dist =  QgsSymbolLayerV2Utils::encodePoint( QPointF( mDistanceX, mDistanceY ) );
    QDomElement distanceElem = QgsSymbolLayerV2Utils::createVendorOptionElement( doc, "distance", dist );
    symbolizerElem.appendChild( distanceElem );

    QgsSymbolLayerV2 *layer = mMarkerSymbol->symbolLayer( i );
    QgsMarkerSymbolLayerV2 *markerLayer = static_cast<QgsMarkerSymbolLayerV2 *>( layer );
    if ( !markerLayer )
    {
      QString errorMsg = QString( "MarkerSymbolLayerV2 expected, %1 found. Skip it." ).arg( layer->layerType() );
      graphicFillElem.appendChild( doc.createComment( errorMsg ) );
    }
    else
    {
      markerLayer->writeSldMarker( doc, graphicFillElem, props );
    }
  }
}

QgsSymbolLayerV2* QgsPointPatternFillSymbolLayer::createFromSld( QDomElement &element )
{
  Q_UNUSED( element );
  return NULL;
}

bool QgsPointPatternFillSymbolLayer::setSubSymbol( QgsSymbolV2* symbol )
{
  if ( !symbol )
  {
    return false;
  }

  if ( symbol->type() == QgsSymbolV2::Marker )
  {
    QgsMarkerSymbolV2* markerSymbol = static_cast<QgsMarkerSymbolV2*>( symbol );
    delete mMarkerSymbol;
    mMarkerSymbol = markerSymbol;
  }
  return true;
}

const QgsExpression* QgsPointPatternFillSymbolLayer::dataDefinedProperty( const QString& property ) const
{
  if ( property == "distance_x" )
  {
    return mDistanceXExpression;
  }
  else if ( property == "distance_x" )
  {
    return mDistanceYExpression;
  }
  else if ( property == "displacement_x" )
  {
    return mDisplacementXExpression;
  }
  else if ( property == "displacement_y" )
  {
    return mDisplacementYExpression;
  }
  return 0;
}

QString QgsPointPatternFillSymbolLayer::dataDefinedPropertyString( const QString& property ) const
{
  const QgsExpression* ex = dataDefinedProperty( property );
  return ex ? ex->dump() : QString();
}

void QgsPointPatternFillSymbolLayer::setDataDefinedProperty( const QString& property, const QString& expressionString )
{
  if ( property == "distance_x" )
  {
    delete mDistanceXExpression; mDistanceXExpression = new QgsExpression( expressionString );
  }
  else if ( property == "distance_x" )
  {
    delete mDistanceYExpression; mDistanceYExpression = new QgsExpression( expressionString );
  }
  else if ( property == "displacement_x" )
  {
    delete mDisplacementXExpression; mDisplacementXExpression = new QgsExpression( expressionString );
  }
  else if ( property == "displacement_y" )
  {
    delete mDisplacementYExpression; mDisplacementYExpression = new QgsExpression( expressionString );
  }
}

void QgsPointPatternFillSymbolLayer::removeDataDefinedProperty( const QString& property )
{
  if ( property == "distance_x" )
  {
    delete mDistanceXExpression; mDistanceXExpression = 0;
  }
  else if ( property == "distance_x" )
  {
    delete mDistanceYExpression; mDistanceYExpression = 0;
  }
  else if ( property == "displacement_x" )
  {
    delete mDisplacementXExpression; mDisplacementXExpression = 0;
  }
  else if ( property == "displacement_y" )
  {
    delete mDisplacementYExpression; mDisplacementYExpression = 0;
  }
}

void QgsPointPatternFillSymbolLayer::removeDataDefinedProperties()
{
  delete mDistanceXExpression; mDistanceXExpression = 0;
  delete mDistanceYExpression; mDistanceYExpression = 0;
  delete mDisplacementXExpression; mDisplacementXExpression = 0;
  delete mDisplacementYExpression; mDisplacementYExpression = 0;
}

QSet<QString> QgsPointPatternFillSymbolLayer::usedAttributes() const
{
  QSet<QString> attributes;

  //add data defined attributes
  QStringList columns;
  if ( mDistanceXExpression )
    columns.append( mDistanceXExpression->referencedColumns() );
  if ( mDistanceYExpression )
    columns.append( mDistanceYExpression->referencedColumns() );
  if ( mDisplacementXExpression )
    columns.append( mDisplacementXExpression->referencedColumns() );
  if ( mDisplacementYExpression )
    columns.append( mDisplacementYExpression->referencedColumns() );

  QStringList::const_iterator it = columns.constBegin();
  for ( ; it != columns.constEnd(); ++it )
  {
    attributes.insert( *it );
  }
  return attributes;
}

void QgsPointPatternFillSymbolLayer::applyDataDefinedSettings( const QgsSymbolV2RenderContext& context )
{
  if ( !mDistanceXExpression && !mDistanceYExpression && !mDisplacementXExpression && !mDisplacementYExpression )
  {
    return;
  }

  double distanceX = mDistanceX;
  if ( mDistanceXExpression )
  {
    distanceX = mDistanceXExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  double distanceY = mDistanceY;
  if ( mDistanceYExpression )
  {
    distanceY = mDistanceYExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  double displacementX = mDisplacementX;
  if ( mDisplacementXExpression )
  {
    displacementX = mDisplacementXExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  double displacementY = mDisplacementY;
  if ( mDisplacementYExpression )
  {
    displacementY = mDisplacementYExpression->evaluate( const_cast<QgsFeature*>( context.feature() ) ).toDouble();
  }
  applyPattern( context, mBrush, distanceX, distanceY, displacementX, displacementY );
}

void QgsPointPatternFillSymbolLayer::prepareExpressions( const QgsVectorLayer* vl )
{
  if ( !vl )
  {
    return;
  }

  const QgsFields& fields = vl->pendingFields();
  if ( mDistanceXExpression )
    mDistanceXExpression->prepare( fields );
  if ( mDistanceYExpression )
    mDistanceYExpression->prepare( fields );
  if ( mDisplacementXExpression )
    mDisplacementXExpression->prepare( fields );
  if ( mDisplacementYExpression )
    mDisplacementYExpression->prepare( fields );
}

//////////////


QgsCentroidFillSymbolLayerV2::QgsCentroidFillSymbolLayerV2()
{
  mMarker = NULL;
  setSubSymbol( new QgsMarkerSymbolV2() );
}

QgsCentroidFillSymbolLayerV2::~QgsCentroidFillSymbolLayerV2()
{
  delete mMarker;
}

QgsSymbolLayerV2* QgsCentroidFillSymbolLayerV2::create( const QgsStringMap& /*properties*/ )
{
  return new QgsCentroidFillSymbolLayerV2();
}

QString QgsCentroidFillSymbolLayerV2::layerType() const
{
  return "CentroidFill";
}

void QgsCentroidFillSymbolLayerV2::setColor( const QColor& color )
{
  mMarker->setColor( color );
  mColor = color;
}

void QgsCentroidFillSymbolLayerV2::startRender( QgsSymbolV2RenderContext& context )
{
  mMarker->setAlpha( context.alpha() );
  mMarker->setOutputUnit( context.outputUnit() );

  mMarker->startRender( context.renderContext() );
}

void QgsCentroidFillSymbolLayerV2::stopRender( QgsSymbolV2RenderContext& context )
{
  mMarker->stopRender( context.renderContext() );
}

void QgsCentroidFillSymbolLayerV2::renderPolygon( const QPolygonF& points, QList<QPolygonF>* rings, QgsSymbolV2RenderContext& context )
{
  Q_UNUSED( rings );

  // calculate centroid
  double cx = 0, cy = 0;
  double area, sum = 0;
  for ( int i = points.count() - 1, j = 0; j < points.count(); i = j++ )
  {
    const QPointF& p1 = points[i];
    const QPointF& p2 = points[j];
    area = p1.x() * p2.y() - p1.y() * p2.x();
    sum += area;
    cx += ( p1.x() + p2.x() ) * area;
    cy += ( p1.y() + p2.y() ) * area;
  }
  sum *= 3.0;
  cx /= sum;
  cy /= sum;

  mMarker->renderPoint( QPointF( cx, cy ), context.feature(), context.renderContext(), -1, context.selected() );
}

QgsStringMap QgsCentroidFillSymbolLayerV2::properties() const
{
  return QgsStringMap();
}

QgsSymbolLayerV2* QgsCentroidFillSymbolLayerV2::clone() const
{
  QgsCentroidFillSymbolLayerV2* x = new QgsCentroidFillSymbolLayerV2();
  x->setSubSymbol( mMarker->clone() );
  return x;
}

void QgsCentroidFillSymbolLayerV2::toSld( QDomDocument &doc, QDomElement &element, QgsStringMap props ) const
{
  // SLD 1.0 specs says: "if a line, polygon, or raster geometry is
  // used with PointSymbolizer, then the semantic is to use the centroid
  // of the geometry, or any similar representative point.
  mMarker->toSld( doc, element, props );
}

QgsSymbolLayerV2* QgsCentroidFillSymbolLayerV2::createFromSld( QDomElement &element )
{
  QgsDebugMsg( "Entered." );

  QgsSymbolLayerV2 *l = QgsSymbolLayerV2Utils::createMarkerLayerFromSld( element );
  if ( !l )
    return NULL;

  QgsSymbolLayerV2List layers;
  layers.append( l );
  QgsMarkerSymbolV2 *marker = new QgsMarkerSymbolV2( layers );

  QgsCentroidFillSymbolLayerV2* x = new QgsCentroidFillSymbolLayerV2();
  x->setSubSymbol( marker );
  return x;
}


QgsSymbolV2* QgsCentroidFillSymbolLayerV2::subSymbol()
{
  return mMarker;
}

bool QgsCentroidFillSymbolLayerV2::setSubSymbol( QgsSymbolV2* symbol )
{
  if ( symbol == NULL || symbol->type() != QgsSymbolV2::Marker )
  {
    delete symbol;
    return false;
  }

  delete mMarker;
  mMarker = static_cast<QgsMarkerSymbolV2*>( symbol );
  mColor = mMarker->color();
  return true;
}
