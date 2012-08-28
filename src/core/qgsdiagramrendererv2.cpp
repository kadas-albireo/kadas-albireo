/***************************************************************************
    qgsdiagramrendererv2.cpp
    ---------------------
    begin                : March 2011
    copyright            : (C) 2011 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsdiagramrendererv2.h"
#include "qgsdiagram.h"
#include "qgsrendercontext.h"
#include <QDomElement>
#include <QPainter>


void QgsDiagramLayerSettings::readXML( const QDomElement& elem )
{
  placement = ( Placement )elem.attribute( "placement" ).toInt();
  placementFlags = ( LinePlacementFlags )elem.attribute( "linePlacementFlags" ).toInt();
  priority = elem.attribute( "priority" ).toInt();
  obstacle = elem.attribute( "obstacle" ).toInt();
  dist = elem.attribute( "dist" ).toDouble();
  xPosColumn = elem.attribute( "xPosColumn" ).toInt();
  yPosColumn = elem.attribute( "yPosColumn" ).toInt();
}

void QgsDiagramLayerSettings::writeXML( QDomElement& layerElem, QDomDocument& doc ) const
{
  QDomElement diagramLayerElem = doc.createElement( "DiagramLayerSettings" );
  diagramLayerElem.setAttribute( "placement", placement );
  diagramLayerElem.setAttribute( "linePlacementFlags", placementFlags );
  diagramLayerElem.setAttribute( "priority", priority );
  diagramLayerElem.setAttribute( "obstacle", obstacle );
  diagramLayerElem.setAttribute( "dist", QString::number( dist ) );
  diagramLayerElem.setAttribute( "xPosColumn", xPosColumn );
  diagramLayerElem.setAttribute( "yPosColumn", yPosColumn );
  layerElem.appendChild( diagramLayerElem );
}

void QgsDiagramSettings::readXML( const QDomElement& elem )
{
  font.fromString( elem.attribute( "font" ) );
  backgroundColor.setNamedColor( elem.attribute( "backgroundColor" ) );
  backgroundColor.setAlpha( elem.attribute( "backgroundAlpha" ).toInt() );
  size.setWidth( elem.attribute( "width" ).toDouble() );
  size.setHeight( elem.attribute( "height" ).toDouble() );
  penColor.setNamedColor( elem.attribute( "penColor" ) );
  penWidth = elem.attribute( "penWidth" ).toDouble();
  minScaleDenominator = elem.attribute( "minScaleDenominator", "-1" ).toDouble();
  maxScaleDenominator = elem.attribute( "maxScaleDenominator", "-1" ).toDouble();

  //mm vs map units
  if ( elem.attribute( "sizeType" ) == "MM" )
  {
    sizeType = MM;
  }
  else
  {
    sizeType = MapUnits;
  }

  //colors
  categoryColors.clear();
  QStringList colorList = elem.attribute( "colors" ).split( "/" );
  QStringList::const_iterator colorIt = colorList.constBegin();
  for ( ; colorIt != colorList.constEnd(); ++colorIt )
  {
    categoryColors.append( QColor( *colorIt ) );
  }

  //attribute indices
  categoryIndices.clear();
  QStringList catList = elem.attribute( "categories" ).split( "/" );
  QStringList::const_iterator catIt = catList.constBegin();
  for ( ; catIt != catList.constEnd(); ++catIt )
  {
    categoryIndices.append( catIt->toInt() );
  }
}

void QgsDiagramSettings::writeXML( QDomElement& rendererElem, QDomDocument& doc ) const
{
  QDomElement categoryElem = doc.createElement( "DiagramCategory" );
  categoryElem.setAttribute( "font", font.toString() );
  categoryElem.setAttribute( "backgroundColor", backgroundColor.name() );
  categoryElem.setAttribute( "backgroundAlpha", backgroundColor.alpha() );
  categoryElem.setAttribute( "width", QString::number( size.width() ) );
  categoryElem.setAttribute( "height", QString::number( size.height() ) );
  categoryElem.setAttribute( "penColor", penColor.name() );
  categoryElem.setAttribute( "penWidth", QString::number( penWidth ) );
  categoryElem.setAttribute( "minScaleDenominator", QString::number( minScaleDenominator ) );
  categoryElem.setAttribute( "maxScaleDenominator", QString::number( maxScaleDenominator ) );
  if ( sizeType == MM )
  {
    categoryElem.setAttribute( "sizeType", "MM" );
  }
  else
  {
    categoryElem.setAttribute( "sizeType", "MapUnits" );
  }

  QString colors;
  for ( int i = 0; i < categoryColors.size(); ++i )
  {
    if ( i > 0 )
    {
      colors.append( "/" );
    }
    colors.append( categoryColors.at( i ).name() );
  }
  categoryElem.setAttribute( "colors", colors );

  QString categories;
  for ( int i = 0; i < categoryIndices.size(); ++i )
  {
    if ( i > 0 )
    {
      categories.append( "/" );
    }
    categories.append( QString::number( categoryIndices.at( i ) ) );
  }
  categoryElem.setAttribute( "categories", categories );

  rendererElem.appendChild( categoryElem );
}

QgsDiagramRendererV2::QgsDiagramRendererV2(): mDiagram( 0 )
{
}

QgsDiagramRendererV2::~QgsDiagramRendererV2()
{
  delete mDiagram;
}

void QgsDiagramRendererV2::setDiagram( QgsDiagram* d )
{
  delete mDiagram;
  mDiagram = d;
}

void QgsDiagramRendererV2::renderDiagram( const QgsAttributeMap& att, QgsRenderContext& c, const QPointF& pos )
{
  if ( !mDiagram )
  {
    return;
  }

  QgsDiagramSettings s;
  if ( !diagramSettings( att, c, s ) )
  {
    return;
  }

  mDiagram->renderDiagram( att, c, s, pos );
}

QSizeF QgsDiagramRendererV2::sizeMapUnits( const QgsAttributeMap& attributes, const QgsRenderContext& c )
{
  QgsDiagramSettings s;
  if ( !diagramSettings( attributes, c, s ) )
  {
    return QSizeF();
  }

  QSizeF size = diagramSize( attributes, c );
  if ( s.sizeType == QgsDiagramSettings::MM )
  {
    convertSizeToMapUnits( size, c );
  }
  return size;
}

void QgsDiagramRendererV2::convertSizeToMapUnits( QSizeF& size, const QgsRenderContext& context ) const
{
  if ( !size.isValid() )
  {
    return;
  }

  double pixelToMap = context.scaleFactor() * context.mapToPixel().mapUnitsPerPixel();
  size.rwidth() *= pixelToMap;
  size.rheight() *= pixelToMap;
}

int QgsDiagramRendererV2::dpiPaintDevice( const QPainter* painter )
{
  if ( painter )
  {
    QPaintDevice* device = painter->device();
    if ( device )
    {
      return device->logicalDpiX();
    }
  }
  return -1;
}

void QgsDiagramRendererV2::_readXML( const QDomElement& elem )
{
  delete mDiagram;
  QString diagramType = elem.attribute( "diagramType" );
  if ( diagramType == "Pie" )
  {
    mDiagram = new QgsPieDiagram();
  }
  else if ( diagramType == "Text" )
  {
    mDiagram = new QgsTextDiagram();
  }
  else
  {
    mDiagram = 0;
  }
}

void QgsDiagramRendererV2::_writeXML( QDomElement& rendererElem, QDomDocument& doc ) const
{
  Q_UNUSED( doc );
  if ( mDiagram )
  {
    rendererElem.setAttribute( "diagramType", mDiagram->diagramName() );
  }
}

QgsSingleCategoryDiagramRenderer::QgsSingleCategoryDiagramRenderer(): QgsDiagramRendererV2()
{
}

QgsSingleCategoryDiagramRenderer::~QgsSingleCategoryDiagramRenderer()
{
}

bool QgsSingleCategoryDiagramRenderer::diagramSettings( const QgsAttributeMap&, const QgsRenderContext& c, QgsDiagramSettings& s )
{
  Q_UNUSED( c );
  s = mSettings;
  return true;
}

QList<QgsDiagramSettings> QgsSingleCategoryDiagramRenderer::diagramSettings() const
{
  QList<QgsDiagramSettings> settingsList;
  settingsList.push_back( mSettings );
  return settingsList;
}

void QgsSingleCategoryDiagramRenderer::readXML( const QDomElement& elem )
{
  QDomElement categoryElem = elem.firstChildElement( "DiagramCategory" );
  if ( categoryElem.isNull() )
  {
    return;
  }

  mSettings.readXML( categoryElem );
  _readXML( elem );
}

void QgsSingleCategoryDiagramRenderer::writeXML( QDomElement& layerElem, QDomDocument& doc ) const
{
  QDomElement rendererElem = doc.createElement( "SingleCategoryDiagramRenderer" );
  mSettings.writeXML( rendererElem, doc );
  _writeXML( rendererElem, doc );
  layerElem.appendChild( rendererElem );
}


QgsLinearlyInterpolatedDiagramRenderer::QgsLinearlyInterpolatedDiagramRenderer(): QgsDiagramRendererV2()
{
}

QgsLinearlyInterpolatedDiagramRenderer::~QgsLinearlyInterpolatedDiagramRenderer()
{
}

QList<QgsDiagramSettings> QgsLinearlyInterpolatedDiagramRenderer::diagramSettings() const
{
  QList<QgsDiagramSettings> settingsList;
  settingsList.push_back( mSettings );
  return settingsList;
}

bool QgsLinearlyInterpolatedDiagramRenderer::diagramSettings( const QgsAttributeMap& attributes, const QgsRenderContext& c, QgsDiagramSettings& s )
{
  s = mSettings;
  s.size = diagramSize( attributes, c );
  return true;
}

QList<int> QgsLinearlyInterpolatedDiagramRenderer::diagramAttributes() const
{
  QList<int> attributes = mSettings.categoryIndices;
  if ( !attributes.contains( mClassificationAttribute ) )
  {
    attributes.push_back( mClassificationAttribute );
  }
  return attributes;
}

QSizeF QgsLinearlyInterpolatedDiagramRenderer::diagramSize( const QgsAttributeMap& attributes, const QgsRenderContext& c )
{
  Q_UNUSED( c );
  QgsAttributeMap::const_iterator attIt = attributes.find( mClassificationAttribute );
  if ( attIt == attributes.constEnd() )
  {
    return QSizeF(); //zero size if attribute is missing
  }
  double value = attIt.value().toDouble();

  //interpolate size
  double ratio = ( value - mLowerValue ) / ( mUpperValue - mLowerValue );
  return QSizeF( mUpperSize.width() * ratio + mLowerSize.width() * ( 1 - ratio ),
                 mUpperSize.height() * ratio + mLowerSize.height() * ( 1 - ratio ) );
}

void QgsLinearlyInterpolatedDiagramRenderer::readXML( const QDomElement& elem )
{
  mLowerValue = elem.attribute( "lowerValue" ).toDouble();
  mUpperValue = elem.attribute( "upperValue" ).toDouble();
  mLowerSize.setWidth( elem.attribute( "lowerWidth" ).toDouble() );
  mLowerSize.setHeight( elem.attribute( "lowerHeight" ).toDouble() );
  mUpperSize.setWidth( elem.attribute( "upperWidth" ).toDouble() );
  mUpperSize.setHeight( elem.attribute( "upperHeight" ).toDouble() );
  mClassificationAttribute = elem.attribute( "classificationAttribute" ).toInt();
  QDomElement settingsElem = elem.firstChildElement( "DiagramCategory" );
  if ( !settingsElem.isNull() )
  {
    mSettings.readXML( settingsElem );
  }
  _readXML( elem );
}

void QgsLinearlyInterpolatedDiagramRenderer::writeXML( QDomElement& layerElem, QDomDocument& doc ) const
{
  QDomElement rendererElem = doc.createElement( "LinearlyInterpolatedDiagramRenderer" );
  rendererElem.setAttribute( "lowerValue", QString::number( mLowerValue ) );
  rendererElem.setAttribute( "upperValue", QString::number( mUpperValue ) );
  rendererElem.setAttribute( "lowerWidth", QString::number( mLowerSize.width() ) );
  rendererElem.setAttribute( "lowerHeight", QString::number( mLowerSize.height() ) );
  rendererElem.setAttribute( "upperWidth", QString::number( mUpperSize.width() ) );
  rendererElem.setAttribute( "upperHeight", QString::number( mUpperSize.height() ) );
  rendererElem.setAttribute( "classificationAttribute", mClassificationAttribute );
  mSettings.writeXML( rendererElem, doc );
  _writeXML( rendererElem, doc );
  layerElem.appendChild( rendererElem );
}
