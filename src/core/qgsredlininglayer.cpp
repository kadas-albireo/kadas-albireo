/***************************************************************************
    qgsredlininglayer.h - Redlining support
     --------------------------------------
    Date                 : Sep 2015
    Copyright            : (C) 2015 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsredlininglayer.h"
#include "qgsgeometry.h"
#include "qgsredliningrendererv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgsvectordataprovider.h"

#include <QUuid>

QgsRedliningLayer::QgsRedliningLayer( const QString& name , const QString &crs ) : QgsVectorLayer(
      QString( "mixed?crs=%1&memoryid=%2" ).arg( crs ).arg( QUuid::createUuid().toString() ),
      name,
      "memory" )
{
  mLayerType = QgsMapLayer::RedliningLayer;

  setFeatureFormSuppress( QgsVectorLayer::SuppressOn );
  dataProvider()->addAttributes( QList<QgsField>()
                                 << QgsField( "size", QVariant::Double, "real", 2, 2 )
                                 << QgsField( "outline", QVariant::String, "string", 15 )
                                 << QgsField( "fill", QVariant::String, "string", 15 )
                                 << QgsField( "outline_style", QVariant::Int, "integer", 1 )
                                 << QgsField( "fill_style", QVariant::Int, "integer", 1 )
                                 << QgsField( "text", QVariant::String, "string", 128 )
                                 << QgsField( "text_x", QVariant::Double, "double", 20, 15 )
                                 << QgsField( "text_y", QVariant::Double, "double", 20, 15 )
                                 << QgsField( "flags", QVariant::String, "string", 64 )
                                 << QgsField( "tooltip", QVariant::String, "string", 128 ) );
  setRendererV2( new QgsRedliningRendererV2 );
  updateFields();

  setCustomProperty( "labeling", "pal" );
  setCustomProperty( "labeling/enabled", true );
  setCustomProperty( "labeling/fieldName", "text" );
  setCustomProperty( "labeling/dataDefined/PositionX", "1~~0~~~~text_x" );
  setCustomProperty( "labeling/dataDefined/PositionY", "1~~0~~~~text_y" );
  setCustomProperty( "labeling/dataDefined/Color", "1~~0~~~~fill" );
  setCustomProperty( "labeling/dataDefined/Size", "1~~1~~8 + \"size\"~~" );
  setCustomProperty( "labeling/dataDefined/Bold", "1~~1~~regexp_substr(\"flags\",'bold=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Italic", "1~~1~~regexp_substr(\"flags\",'italic=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Family", "1~~1~~regexp_substr(\"flags\",'family=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Rotation", "1~~1~~regexp_substr(\"flags\",'rotation=([^,]+)')~~" );
  setDisplayField( "tooltip" );
}

bool QgsRedliningLayer::addShape( QgsGeometry *geometry, const QColor &outline, const QColor &fill, int outlineSize, Qt::PenStyle outlineStyle, Qt::BrushStyle fillStyle, const QString& flags, const QString& tooltip )
{
  QgsFeature f( pendingFields() );
  f.setGeometry( geometry );
  f.setAttribute( "size", outlineSize );
  f.setAttribute( "outline", QgsSymbolLayerV2Utils::encodeColor( outline ) );
  f.setAttribute( "fill", QgsSymbolLayerV2Utils::encodeColor( fill ) );
  f.setAttribute( "outline_style", QgsSymbolLayerV2Utils::encodePenStyle( outlineStyle ) );
  f.setAttribute( "fill_style" , QgsSymbolLayerV2Utils::encodeBrushStyle( fillStyle ) );
  f.setAttribute( "flags", flags );
  f.setAttribute( "tooltip", tooltip );
  return dataProvider()->addFeatures( QgsFeatureList() << f );
}

bool QgsRedliningLayer::addText( const QString &text, const QgsPointV2& pos, const QColor& color, const QFont& font, const QString& tooltip, double rotation )
{
  while ( rotation <= -180 ) rotation += 360.;
  while ( rotation > 180 ) rotation -= 360.;
  QgsFeature f( pendingFields() );
  f.setGeometry( new QgsGeometry( pos.clone() ) );
  f.setAttribute( "text", text );
  f.setAttribute( "text_x", pos.x() );
  f.setAttribute( "text_y", pos.y() );
  f.setAttribute( "size", font.pixelSize() );
  f.setAttribute( "outline", QgsSymbolLayerV2Utils::encodeColor( color ) );
  f.setAttribute( "flags", QString( "family=%1,italic=%2,bold=%3,rotation=%4" ).arg( font.family() ).arg( font.italic() ).arg( font.bold() ).arg( rotation ) );
  f.setAttribute( "tooltip", tooltip );
  return dataProvider()->addFeatures( QgsFeatureList() << f );
}

void QgsRedliningLayer::read( const QDomElement& redliningElem )
{
  QgsFeatureList features;
  QDomNodeList nodes = redliningElem.childNodes();
  for ( int iNode = 0, nNodes = nodes.size(); iNode < nNodes; ++iNode )
  {
    QDomElement redliningItemElem = nodes.at( iNode ).toElement();
    if ( redliningItemElem.nodeName() == "RedliningItem" )
    {
      QgsFeature feature( dataProvider()->fields() );
      feature.setAttribute( "size", redliningItemElem.attribute( "size", "1" ) );
      feature.setAttribute( "outline", redliningItemElem.attribute( "outline", "255,0,0,255" ) );
      feature.setAttribute( "fill", redliningItemElem.attribute( "fill", "0,0,255,255" ) );
      feature.setAttribute( "outline_style", redliningElem.attribute( "outline_style", "1" ) );
      feature.setAttribute( "fill_style", redliningElem.attribute( "fill_style", "1" ) );
      feature.setAttribute( "text", redliningItemElem.attribute( "text", "" ) );
      feature.setAttribute( "text_x", redliningItemElem.attribute( "text_x", "" ) );
      feature.setAttribute( "text_y", redliningItemElem.attribute( "text_y", "" ) );
      feature.setAttribute( "flags", redliningItemElem.attribute( "flags", "" ) );
      feature.setAttribute( "tooltip", redliningItemElem.attribute( "tooltip" ) );
      feature.setGeometry( QgsGeometry::fromWkt( redliningItemElem.attribute( "geometry", "" ) ) );
      features.append( feature );
    }
  }
  dataProvider()->addFeatures( features );
  updateFields();
  emit layerModified();
}

void QgsRedliningLayer::write( QDomElement &redliningElem )
{
  QgsFeatureIterator it = getFeatures();
  QgsFeature feature;
  while ( it.nextFeature( feature ) )
  {
    QDomElement redliningItemElem = redliningElem.ownerDocument().createElement( "RedliningItem" );
    redliningItemElem.setAttribute( "size", feature.attribute( "size" ).toString() );
    redliningItemElem.setAttribute( "outline", feature.attribute( "outline" ).toString() );
    redliningItemElem.setAttribute( "fill", feature.attribute( "fill" ).toString() );
    redliningItemElem.setAttribute( "outline_style", feature.attribute( "outline_style" ).toString() );
    redliningItemElem.setAttribute( "fill_style", feature.attribute( "fill_style" ).toString() );
    redliningItemElem.setAttribute( "text", feature.attribute( "text" ).toString() );
    redliningItemElem.setAttribute( "text_x", feature.attribute( "text_x" ).toString() );
    redliningItemElem.setAttribute( "text_y", feature.attribute( "text_y" ).toString() );
    redliningItemElem.setAttribute( "flags", feature.attribute( "flags" ).toString() );
    redliningItemElem.setAttribute( "geometry", feature.geometry()->exportToWkt() );
    redliningItemElem.setAttribute( "tooltip", feature.attribute( "tooltip" ).toString() );
    redliningElem.appendChild( redliningItemElem );
  }
}

void QgsRedliningLayer::pasteFeatures( const QList<QgsFeature> &features )
{
  foreach ( QgsFeature feature, features )
  {
    if ( feature.attribute( "size" ).isNull() )
      feature.setAttribute( "size", 1 );
    if ( feature.attribute( "outline" ).isNull() )
      feature.setAttribute( "outline", QgsSymbolLayerV2Utils::encodeColor( Qt::red ) );
    if ( feature.attribute( "fill" ).isNull() )
      feature.setAttribute( "fill", QgsSymbolLayerV2Utils::encodeColor( Qt::blue ) );
    if ( feature.attribute( "outline_style" ).isNull() )
      feature.setAttribute( "outline_style", QgsSymbolLayerV2Utils::encodePenStyle( Qt::SolidLine ) );
    if ( feature.attribute( "fill_style" ).isNull() )
      feature.setAttribute( "fill_style" , QgsSymbolLayerV2Utils::encodeBrushStyle( Qt::SolidPattern ) );

    dataProvider()->addFeatures( QgsFeatureList() << feature );
  }
}
