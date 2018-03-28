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
#include "qgscrscache.h"
#include "qgsgeometry.h"
#include "qgsfeaturestore.h"
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
  mValid = true;
  mPriority = 10;

  setFeatureFormSuppress( QgsVectorLayer::SuppressOn );
  dataProvider()->addAttributes( QList<QgsField>()
                                 << QgsField( "size", QVariant::Double, "real", 2, 2 )
                                 << QgsField( "outline", QVariant::String, "string", 15 )
                                 << QgsField( "fill", QVariant::String, "string", 15 )
                                 << QgsField( "outline_style", QVariant::Int, "integer", 1 )
                                 << QgsField( "fill_style", QVariant::Int, "integer", 1 )
                                 << QgsField( "text", QVariant::String, "string", 128 )
                                 << QgsField( "flags", QVariant::String, "string", 64 )
                                 << QgsField( "tooltip", QVariant::String, "string", 128 )
                                 << QgsField( "attributes", QVariant::String, "string", 8192 ) );
  setRendererV2( new QgsRedliningRendererV2 );
  updateFields();

  setCustomProperty( "labeling", "pal" );
  setCustomProperty( "labeling/enabled", true );
  setCustomProperty( "labeling/displayAll", true );
  setCustomProperty( "labeling/fieldName", "text" );
  setCustomProperty( "labeling/dataDefined/Color", "1~~0~~~~fill" );
  setCustomProperty( "labeling/dataDefined/Size", "1~~1~~regexp_substr(\"flags\",'fontSize=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Bold", "1~~1~~regexp_substr(\"flags\",'bold=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Italic", "1~~1~~regexp_substr(\"flags\",'italic=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Family", "1~~1~~regexp_substr(\"flags\",'family=([^,]+)')~~" );
  setCustomProperty( "labeling/dataDefined/Rotation", "1~~1~~regexp_substr(\"flags\",'rotation=([^,]+)')~~" );
  setDisplayField( "tooltip" );
  connect( this, SIGNAL( layerTransparencyChanged( int ) ), this, SLOT( changeTextTransparency( int ) ) );
}

bool QgsRedliningLayer::addShape( QgsGeometry *geometry, const QColor &outline, const QColor &fill, int outlineSize, Qt::PenStyle outlineStyle, Qt::BrushStyle fillStyle, const QString& flags, const QString& tooltip , const QString &text, const QString& attributes )
{
  QFont font;
  QgsFeature f( pendingFields() );
  f.setGeometry( geometry );
  f.setAttribute( "size", outlineSize );
  f.setAttribute( "text", text );
  f.setAttribute( "outline", QgsSymbolLayerV2Utils::encodeColor( outline ) );
  f.setAttribute( "fill", QgsSymbolLayerV2Utils::encodeColor( fill ) );
  f.setAttribute( "outline_style", QgsSymbolLayerV2Utils::encodePenStyle( outlineStyle ) );
  f.setAttribute( "fill_style" , QgsSymbolLayerV2Utils::encodeBrushStyle( fillStyle ) );
  QMap<QString, QString> flagsMap = deserializeFlags( flags );
  if ( !text.isEmpty() )
  {
    flagsMap["family"] = flagsMap.value( "family", font.family() );
    flagsMap["italic"] = flagsMap.value( "italic", QString( "%1" ).arg( font.italic() ) );
    flagsMap["bold"] = flagsMap.value( "bold", QString( "%1" ).arg( font.bold() ) );
    flagsMap["fontSize"] = flagsMap.value( "fontSize", QString::number( font.pointSize() ) );
  }
  flagsMap["rotation"] = flagsMap.value( "rotation", "0" );
  f.setAttribute( "flags", serializeFlags( flagsMap ) );
  f.setAttribute( "tooltip", tooltip );
  f.setAttribute( "attributes", attributes );
  return dataProvider()->addFeatures( QgsFeatureList() << f );
}

bool QgsRedliningLayer::addText( const QString &text, const QgsPointV2& pos, const QColor& color, const QFont& font, const QString& tooltip, double rotation, int markerSize )
{
  while ( rotation <= -180 ) rotation += 360.;
  while ( rotation > 180 ) rotation -= 360.;
  QgsFeature f( pendingFields() );
  f.setGeometry( new QgsGeometry( pos.clone() ) );
  f.setAttribute( "text", text );
  f.setAttribute( "size", markerSize );
  f.setAttribute( "fill", QgsSymbolLayerV2Utils::encodeColor( color ) );
  f.setAttribute( "outline", QgsSymbolLayerV2Utils::encodeColor( color ) );
  f.setAttribute( "flags", QString( "family=%1,italic=%2,bold=%3,rotation=%4,fontSize=%5" ).arg( font.family() ).arg( font.italic() ).arg( font.bold() ).arg( rotation ).arg( font.pointSize() ) );
  f.setAttribute( "tooltip", tooltip );
  return dataProvider()->addFeatures( QgsFeatureList() << f );
}

QgsFeatureList QgsRedliningLayer::pasteFeatures( const QgsFeatureStore& featureStore, QgsPoint* targetPos )
{
  const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( featureStore.crs().authid(), crs().authid() );
  QgsFeatureList newFeatures;
  QgsPoint oldCenter;
  foreach ( const QgsFeature& feature, featureStore.features() )
  {
    QString flags = feature.attribute( "flags" ).toString();
    if ( flags.isEmpty() )
    {
      if ( feature.geometry()->type() == QGis::Point )
      {
        flags = "shape=point,symbol=circle";
      }
      else if ( feature.geometry()->type() == QGis::Line )
      {
        flags = "shape=line";
      }
      else if ( feature.geometry()->type() == QGis::Polygon )
      {
        flags = "shape=polygon";
      }
      else
      {
        continue;
      }
    }
    QgsFeature newFeature( dataProvider()->fields() );
    newFeature.setGeometry( QgsGeometry( feature.geometry()->geometry()->transformed( *ct ) ) );
    oldCenter += newFeature.geometry()->boundingBox().center();

    newFeature.setAttributes( feature.attributes() );

    QMap<QString, QVariant> attribs;
    attribs["flags"] = flags;
    attribs["size"] = 1;
    attribs["outline"] = QgsSymbolLayerV2Utils::encodeColor( Qt::black );
    attribs["fill"] = QgsSymbolLayerV2Utils::encodeColor( Qt::yellow );
    attribs["outline_style"] = QgsSymbolLayerV2Utils::encodePenStyle( Qt::SolidLine );
    attribs["fill_style"] = QgsSymbolLayerV2Utils::encodeBrushStyle( Qt::SolidPattern );

    foreach ( const QString& key, attribs.keys() )
    {
      QVariant srcValue = feature.attribute( key );
      newFeature.setAttribute( key, srcValue.isNull() ? attribs[key] : srcValue );
    }
    newFeatures.append( newFeature );
  }
  if ( targetPos )
  {
    int n = newFeatures.size();
    oldCenter = QgsPoint( oldCenter.x() / n, oldCenter.y() / n );
    QgsVector delta = *targetPos - oldCenter;
    for ( int i = 0; i < n; ++i )
    {
      newFeatures[i].geometry()->translate( delta.x(), delta.y() );
    }
  }

  dataProvider()->addFeatures( newFeatures );
  return newFeatures;
}

QgsFeatureId QgsRedliningLayer::addFeature( QgsFeature& f )
{
  QgsFeatureList features = QgsFeatureList() << f;
  if ( dataProvider()->addFeatures( features ) )
  {
    updateExtents();
    emit featureAdded( features.front().id() );
  }
  return features.front().id();
}

void QgsRedliningLayer::deleteFeature( QgsFeatureId fid )
{
  if ( dataProvider()->deleteFeatures( QgsFeatureIds() << fid ) )
  {
    updateExtents();
    emit featureDeleted( fid );
  }
}

void QgsRedliningLayer::changeGeometry( QgsFeatureId fid, const QgsGeometry& geom )
{
  QgsGeometryMap geomMap;
  geomMap[fid] = geom;
  dataProvider()->changeGeometryValues( geomMap );
  updateExtents();
  emit geometryChanged( fid, geom );
}

void QgsRedliningLayer::changeAttributes( QgsFeatureId fid, const QgsAttributeMap& attribs )
{
  QgsChangedAttributesMap changedAttribs;
  changedAttribs[fid] = attribs;
  dataProvider()->changeAttributeValues( changedAttribs );
  updateExtents();
  foreach ( int key, attribs.keys() )
  {
    emit attributeValueChanged( fid, key, attribs[key] );
  }
}


QMap<QString, QString> QgsRedliningLayer::deserializeFlags( const QString& flagsStr )
{
  QMap<QString, QString> flagsMap;
  foreach ( const QString& flag, flagsStr.split( ",", QString::SkipEmptyParts ) )
  {
    int pos = flag.indexOf( "=" );
    flagsMap.insert( flag.left( pos ), pos >= 0 ? flag.mid( pos + 1 ) : QString() );
  }
  return flagsMap;
}

QString QgsRedliningLayer::serializeFlags( const QMap<QString, QString>& flagsMap )
{
  QString flagsStr;
  foreach ( const QString& key, flagsMap.keys() )
  {
    flagsStr += QString( "%1=%2," ).arg( key ).arg( flagsMap.value( key ) );
  }
  return flagsStr;
}

bool QgsRedliningLayer::readXml( const QDomNode& layer_node )
{
  QgsFeatureList features;
  QDomNodeList nodes = layer_node.toElement().childNodes();
  setLayerTransparency( layer_node.firstChildElement( "layerTransparency" ).text().toInt() );
  for ( int iNode = 0, nNodes = nodes.size(); iNode < nNodes; ++iNode )
  {
    QDomElement redliningItemElem = nodes.at( iNode ).toElement();
    if ( redliningItemElem.nodeName() == "RedliningItem" )
    {
      QgsFeature feature( dataProvider()->fields() );
      feature.setAttribute( "size", redliningItemElem.attribute( "size", "1" ) );
      feature.setAttribute( "outline", redliningItemElem.attribute( "outline", "255,0,0,255" ) );
      feature.setAttribute( "fill", redliningItemElem.attribute( "fill", "0,0,255,255" ) );
      feature.setAttribute( "outline_style", redliningItemElem.attribute( "outline_style", "1" ) );
      feature.setAttribute( "fill_style", redliningItemElem.attribute( "fill_style", "1" ) );
      feature.setAttribute( "text", redliningItemElem.attribute( "text", "" ) );
      feature.setAttribute( "flags", redliningItemElem.attribute( "flags", "" ) );
      feature.setAttribute( "tooltip", redliningItemElem.attribute( "tooltip" ) );
      feature.setAttribute( "attributes", redliningItemElem.attribute( "attributes", "" ) );
      feature.setGeometry( QgsGeometry::fromWkt( redliningItemElem.attribute( "geometry", "" ) ) );
      features.append( feature );
    }
  }
  dataProvider()->addFeatures( features );
  updateFields();
  emit layerModified();
  return true;
}

bool QgsRedliningLayer::writeXml( QDomNode & layer_node, QDomDocument & document )
{
  QDomElement layerElement = layer_node.toElement();
  layerElement.setAttribute( "type", "redlining" );

  QDomElement transparencyElem = document.createElement( "layerTransparency" );
  QDomText transparencyValue = document.createTextNode( QString::number( layerTransparency() ) );
  transparencyElem.appendChild( transparencyValue );
  layer_node.appendChild( transparencyElem );

  setLayerTransparency( layer_node.firstChildElement( "layerTransparency" ).text().toInt() );
  QgsFeatureIterator it = getFeatures();
  QgsFeature feature;
  while ( it.nextFeature( feature ) )
  {
    QDomElement redliningItemElem = document.createElement( "RedliningItem" );
    redliningItemElem.setAttribute( "size", feature.attribute( "size" ).toString() );
    redliningItemElem.setAttribute( "outline", feature.attribute( "outline" ).toString() );
    redliningItemElem.setAttribute( "fill", feature.attribute( "fill" ).toString() );
    redliningItemElem.setAttribute( "outline_style", feature.attribute( "outline_style" ).toString() );
    redliningItemElem.setAttribute( "fill_style", feature.attribute( "fill_style" ).toString() );
    redliningItemElem.setAttribute( "text", feature.attribute( "text" ).toString() );
    redliningItemElem.setAttribute( "flags", feature.attribute( "flags" ).toString() );
    redliningItemElem.setAttribute( "geometry", feature.geometry()->exportToWkt() );
    redliningItemElem.setAttribute( "tooltip", feature.attribute( "tooltip" ).toString() );
    redliningItemElem.setAttribute( "attributes", feature.attribute( "attributes" ).toString() );
    layer_node.appendChild( redliningItemElem );
  }
  return true;
}

void QgsRedliningLayer::changeTextTransparency( int transparency )
{
  setCustomProperty( "labeling/textTransp", transparency );
}
