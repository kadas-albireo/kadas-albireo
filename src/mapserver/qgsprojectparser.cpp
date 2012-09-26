/***************************************************************************
                              qgsprojectparser.cpp
                              --------------------
  begin                : May 27, 2010
  copyright            : (C) 2010 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsprojectparser.h"
#include "qgsconfigcache.h"
#include "qgscrscache.h"
#include "qgsdatasourceuri.h"
#include "qgsmslayercache.h"
#include "qgslogger.h"
#include "qgsmapserviceexception.h"
#include "qgsrasterlayer.h"
#include "qgsrenderer.h"
#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"

#include "qgscomposition.h"
#include "qgscomposerarrow.h"
#include "qgscomposerattributetable.h"
#include "qgscomposerlabel.h"
#include "qgscomposerlegend.h"
#include "qgscomposermap.h"
#include "qgscomposerpicture.h"
#include "qgscomposerscalebar.h"
#include "qgscomposershape.h"

#include "QFileInfo"
#include "QTextStream"


QgsProjectParser::QgsProjectParser( QDomDocument* xmlDoc, const QString& filePath ): QgsConfigParser(), mXMLDoc( xmlDoc ), mProjectPath( filePath )
{
  mOutputUnits = QgsMapRenderer::Millimeters;
  setLegendParametersFromProject();
  setSelectionColor();
  setMaxWidthHeight();

  //accelerate search for layers and groups
  if ( mXMLDoc )
  {
    QDomNodeList layerNodeList = mXMLDoc->elementsByTagName( "maplayer" );
    QDomElement currentElement;
    int nNodes = layerNodeList.size();
    for ( int i = 0; i < nNodes; ++i )
    {
      currentElement = layerNodeList.at( i ).toElement();
      mProjectLayerElements.push_back( currentElement );
      mProjectLayerElementsByName.insert( layerName( currentElement ), currentElement );
      mProjectLayerElementsById.insert( layerId( currentElement ), currentElement );
    }

    QDomElement legendElement = mXMLDoc->documentElement().firstChildElement( "legend" );
    if ( !legendElement.isNull() )
    {
      QDomNodeList groupNodeList = legendElement.elementsByTagName( "legendgroup" );
      for ( int i = 0; i < groupNodeList.size(); ++i )
      {
        mLegendGroupElements.push_back( groupNodeList.at( i ).toElement() );
      }
    }
  }
}

QgsProjectParser::QgsProjectParser(): mXMLDoc( 0 )
{
}

QgsProjectParser::~QgsProjectParser()
{
  delete mXMLDoc;
}

int QgsProjectParser::numberOfLayers() const
{
  return mProjectLayerElements.size();
}

void QgsProjectParser::layersAndStylesCapabilities( QDomElement& parentElement, QDomDocument& doc ) const
{
  QStringList nonIdentifiableLayers = identifyDisabledLayers();
  QMap<QString, QgsMapLayer *> layerMap;

  foreach ( const QDomElement &elem, mProjectLayerElements )
  {
    QgsMapLayer *layer = createLayerFromElement( elem );
    if ( layer )
    {
      QgsDebugMsg( QString( "add layer %1 to map" ).arg( layer->id() ) );
      layerMap.insert( layer->id(), layer );
    }
#if QGSMSDEBUG
    else
    {
      QString buf;
      QTextStream s( &buf );
      elem.save( s, 0 );
      QgsDebugMsg( QString( "layer %1 not found" ).arg( buf ) );
    }
#endif
  }

  //According to the WMS spec, there can be only one toplevel layer.
  //So we create an artificial one here to be in accordance with the schema
  QString projTitle = projectTitle();
  QDomElement layerParentElem = doc.createElement( "Layer" );
  layerParentElem.setAttribute( "queryable", "1" );
  QDomElement layerParentNameElem = doc.createElement( "Name" );
  QDomText layerParentNameText = doc.createTextNode( projTitle );
  layerParentNameElem.appendChild( layerParentNameText );
  layerParentElem.appendChild( layerParentNameElem );
  QDomElement layerParentTitleElem = doc.createElement( "Title" );
  QDomText layerParentTitleText = doc.createTextNode( projTitle );
  layerParentTitleElem.appendChild( layerParentTitleText );
  layerParentElem.appendChild( layerParentTitleElem );

  QDomElement legendElem = mXMLDoc->documentElement().firstChildElement( "legend" );

  addLayers( doc, layerParentElem, legendElem, layerMap, nonIdentifiableLayers );

  parentElement.appendChild( layerParentElem );
  combineExtentAndCrsOfGroupChildren( layerParentElem, doc );
}

void QgsProjectParser::featureTypeList( QDomElement& parentElement, QDomDocument& doc ) const
{
  QStringList wfsLayersId = wfsLayers();

  if ( mProjectLayerElements.size() < 1 )
  {
    return;
  }

  QMap<QString, QgsMapLayer *> layerMap;

  foreach ( const QDomElement &elem, mProjectLayerElements )
  {
    QString type = elem.attribute( "type" );
    if ( type == "vector" )
    {
      //QgsMapLayer *layer = createLayerFromElement( *layerIt );
      QgsMapLayer *layer = createLayerFromElement( elem );
      if ( layer && wfsLayersId.contains( layer->id() ) )
      {
        QgsDebugMsg( QString( "add layer %1 to map" ).arg( layer->id() ) );
        layerMap.insert( layer->id(), layer );

        QDomElement layerElem = doc.createElement( "FeatureType" );
        QDomElement nameElem = doc.createElement( "Name" );
        //We use the layer name even though it might not be unique.
        //Because the id sometimes contains user/pw information and the name is more descriptive
        QDomText nameText = doc.createTextNode( layer->name() );
        nameElem.appendChild( nameText );
        layerElem.appendChild( nameElem );

        QDomElement titleElem = doc.createElement( "Title" );
        QString titleName = layer->title();
        if ( titleName.isEmpty() )
        {
          titleName = layer->name();
        }
        QDomText titleText = doc.createTextNode( titleName );
        titleElem.appendChild( titleText );
        layerElem.appendChild( titleElem );

        QDomElement abstractElem = doc.createElement( "Abstract" );
        QString abstractName = layer->abstract();
        if ( abstractName.isEmpty() )
        {
          abstractName = "";
        }
        QDomText abstractText = doc.createTextNode( abstractName );
        abstractElem.appendChild( abstractText );
        layerElem.appendChild( abstractElem );

        //appendExGeographicBoundingBox( layerElem, doc, layer->extent(), layer->crs() );

        QDomElement srsElem = doc.createElement( "SRS" );
        QDomText srsText = doc.createTextNode( layer->crs().authid() );
        srsElem.appendChild( srsText );
        layerElem.appendChild( srsElem );

        QgsRectangle layerExtent = layer->extent();
        QDomElement bBoxElement = doc.createElement( "LatLongBoundingBox" );
        bBoxElement.setAttribute( "minx", QString::number( layerExtent.xMinimum() ) );
        bBoxElement.setAttribute( "miny", QString::number( layerExtent.yMinimum() ) );
        bBoxElement.setAttribute( "maxx", QString::number( layerExtent.xMaximum() ) );
        bBoxElement.setAttribute( "maxy", QString::number( layerExtent.yMaximum() ) );
        layerElem.appendChild( bBoxElement );

        parentElement.appendChild( layerElem );
      }
    }
  }
  return;
}

void QgsProjectParser::describeFeatureType( const QString& aTypeName, QDomElement& parentElement, QDomDocument& doc ) const
{
  if ( mProjectLayerElements.size() < 1 )
  {
    return;
  }

  QStringList wfsLayersId = wfsLayers();
  QMap< QString, QMap< int, QString > > aliasInfo = layerAliasInfo();
  QMap< QString, QSet<QString> > hiddenAttrs = hiddenAttributes();

  foreach ( const QDomElement &elem, mProjectLayerElements )
  {
    QString type = elem.attribute( "type" );
    if ( type == "vector" )
    {
      QgsMapLayer *mLayer = createLayerFromElement( elem );
      QgsVectorLayer* layer = dynamic_cast<QgsVectorLayer*>( mLayer );
      if ( layer && wfsLayersId.contains( layer->id() ) && ( aTypeName == "" || layer->name() == aTypeName ) )
      {
        //do a select with searchRect and go through all the features
        QgsVectorDataProvider* provider = layer->dataProvider();
        if ( !provider )
        {
          continue;
        }

        //is there alias info for this vector layer?
        QMap< int, QString > layerAliasInfo;
        QMap< QString, QMap< int, QString > >::const_iterator aliasIt = aliasInfo.find( mLayer->id() );
        if ( aliasIt != aliasInfo.constEnd() )
        {
          layerAliasInfo = aliasIt.value();
        }

        //hidden attributes for this layer
        QSet<QString> layerHiddenAttributes;
        QMap< QString, QSet<QString> >::const_iterator hiddenIt = hiddenAttrs.find( mLayer->id() );
        if ( hiddenIt != hiddenAttrs.constEnd() )
        {
          layerHiddenAttributes = hiddenIt.value();
        }

        QString typeName = layer->name();
        typeName = typeName.replace( QString( " " ), QString( "_" ) );

        //xsd:element
        QDomElement elementElem = doc.createElement( "element"/*xsd:element*/ );
        elementElem.setAttribute( "name", typeName );
        elementElem.setAttribute( "type", "qgs:" + typeName + "Type" );
        elementElem.setAttribute( "substitutionGroup", "gml:_Feature" );
        parentElement.appendChild( elementElem );

        //xsd:complexType
        QDomElement complexTypeElem = doc.createElement( "complexType"/*xsd:complexType*/ );
        complexTypeElem.setAttribute( "name", typeName + "Type" );
        parentElement.appendChild( complexTypeElem );

        //xsd:complexType
        QDomElement complexContentElem = doc.createElement( "complexContent"/*xsd:complexContent*/ );
        complexTypeElem.appendChild( complexContentElem );

        //xsd:extension
        QDomElement extensionElem = doc.createElement( "extension"/*xsd:extension*/ );
        extensionElem.setAttribute( "base", "gml:AbstractFeatureType" );
        complexContentElem.appendChild( extensionElem );

        //xsd:sequence
        QDomElement sequenceElem = doc.createElement( "sequence"/*xsd:sequence*/ );
        extensionElem.appendChild( sequenceElem );

        //xsd:element
        QDomElement geomElem = doc.createElement( "element"/*xsd:element*/ );
        geomElem.setAttribute( "name", "geometry" );
        QGis::WkbType wkbType = layer->wkbType();
        switch ( wkbType )
        {
          case QGis::WKBPoint25D:
          case QGis::WKBPoint:
            geomElem.setAttribute( "type", "gml:PointPropertyType" );
            break;
          case QGis::WKBLineString25D:
          case QGis::WKBLineString:
            geomElem.setAttribute( "type", "gml:LineStringPropertyType" );
            break;
          case QGis::WKBPolygon25D:
          case QGis::WKBPolygon:
            geomElem.setAttribute( "type", "gml:PolygonPropertyType" );
            break;
          case QGis::WKBMultiPoint25D:
          case QGis::WKBMultiPoint:
            geomElem.setAttribute( "type", "gml:MultiPointPropertyType" );
            break;
          case QGis::WKBMultiLineString25D:
          case QGis::WKBMultiLineString:
            geomElem.setAttribute( "type", "gml:MultiLineStringPropertyType" );
            break;
          case QGis::WKBMultiPolygon25D:
          case QGis::WKBMultiPolygon:
            geomElem.setAttribute( "type", "gml:MultiPolygonPropertyType" );
            break;
          default:
            geomElem.setAttribute( "type", "gml:GeometryPropertyType" );
            break;
        }
        geomElem.setAttribute( "minOccurs", "0" );
        geomElem.setAttribute( "maxOccurs", "1" );
        sequenceElem.appendChild( geomElem );

        const QgsFieldMap& fields = provider->fields();
        for ( QgsFieldMap::const_iterator it = fields.begin(); it != fields.end(); ++it )
        {

          QString attributeName = it.value().name();
          //skip attribute if it has edit type 'hidden'
          if ( layerHiddenAttributes.contains( attributeName ) )
          {
            continue;
          }

          //xsd:element
          QDomElement geomElem = doc.createElement( "element"/*xsd:element*/ );
          geomElem.setAttribute( "name", attributeName );
          if ( it.value().type() == 2 )
            geomElem.setAttribute( "type", "integer" );
          else if ( it.value().type() == 6 )
            geomElem.setAttribute( "type", "double" );
          else
            geomElem.setAttribute( "type", "string" );

          sequenceElem.appendChild( geomElem );

          //check if the attribute name should be replaced with an alias
          QMap<int, QString>::const_iterator aliasIt = layerAliasInfo.find( it.key() );
          if ( aliasIt != layerAliasInfo.constEnd() )
          {
            geomElem.setAttribute( "alias", aliasIt.value() );
          }
        }
      }
    }
  }
  return;
}

void QgsProjectParser::addLayers( QDomDocument &doc,
                                  QDomElement &parentElem,
                                  const QDomElement &legendElem,
                                  const QMap<QString, QgsMapLayer *> &layerMap,
                                  const QStringList &nonIdentifiableLayers ) const
{
  QDomNodeList legendChildren = legendElem.childNodes();
  for ( int i = 0; i < legendChildren.size(); ++i )
  {
    QDomElement currentChildElem = legendChildren.at( i ).toElement();

    QDomElement layerElem = doc.createElement( "Layer" );

    if ( currentChildElem.tagName() == "legendgroup" )
    {
      layerElem.setAttribute( "queryable", "1" );
      QString name = currentChildElem.attribute( "name" );
      QDomElement nameElem = doc.createElement( "Name" );
      QDomText nameText = doc.createTextNode( name );
      nameElem.appendChild( nameText );
      layerElem.appendChild( nameElem );

      QDomElement titleElem = doc.createElement( "Title" );
      QDomText titleText = doc.createTextNode( name );
      titleElem.appendChild( titleText );
      layerElem.appendChild( titleElem );

      if ( currentChildElem.attribute( "embedded" ) == "1" )
      {
        //add layers from other project files and embed into this group
        QString project = convertToAbsolutePath( currentChildElem.attribute( "project" ) );
        QgsDebugMsg( QString( "Project path: %1" ).arg( project ) );
        QString embeddedGroupName = currentChildElem.attribute( "name" );
        QgsProjectParser* p = dynamic_cast<QgsProjectParser*>( QgsConfigCache::instance()->searchConfiguration( project ) );
        QList<QDomElement> embeddedGroupElements = p->mLegendGroupElements;
        if ( p )
        {
          QStringList pIdDisabled = p->identifyDisabledLayers();

          QDomElement embeddedGroupElem;
          foreach ( const QDomElement &elem, embeddedGroupElements )
          {
            if ( elem.attribute( "name" ) == embeddedGroupName )
            {
              embeddedGroupElem = elem;
              break;
            }
          }

          QMap<QString, QgsMapLayer *> pLayerMap;
          QList<QDomElement> embeddedProjectLayerElements = p->mProjectLayerElements;
          foreach ( const QDomElement &elem, embeddedProjectLayerElements )
          {
            pLayerMap.insert( layerId( elem ), p->createLayerFromElement( elem ) );
          }

          p->addLayers( doc, layerElem, embeddedGroupElem, pLayerMap, pIdDisabled );
        }
      }
      else //normal (not embedded) legend group
      {
        addLayers( doc, layerElem, currentChildElem, layerMap, nonIdentifiableLayers );
      }

      // combine bounding boxes of children (groups/layers)
      combineExtentAndCrsOfGroupChildren( layerElem, doc );
    }
    else if ( currentChildElem.tagName() == "legendlayer" )
    {
      QString id = layerIdFromLegendLayer( currentChildElem );

      if ( !layerMap.contains( id ) )
      {
        QgsDebugMsg( QString( "layer %1 not found in map - layer cache to small?" ).arg( id ) );
        continue;
      }

      QgsMapLayer *currentLayer = layerMap[ id ];
      if ( !currentLayer )
      {
        QgsDebugMsg( QString( "layer %1 not found" ).arg( id ) );
        continue;
      }

      if ( nonIdentifiableLayers.contains( currentLayer->id() ) )
      {
        layerElem.setAttribute( "queryable", "0" );
      }
      else
      {
        layerElem.setAttribute( "queryable", "1" );
      }

      QDomElement nameElem = doc.createElement( "Name" );
      //We use the layer name even though it might not be unique.
      //Because the id sometimes contains user/pw information and the name is more descriptive
      QDomText nameText = doc.createTextNode( currentLayer->name() );
      nameElem.appendChild( nameText );
      layerElem.appendChild( nameElem );

      QDomElement titleElem = doc.createElement( "Title" );
      QString titleName = currentLayer->title();
      if ( titleName.isEmpty() )
      {
        titleName = currentLayer->name();
      }
      QDomText titleText = doc.createTextNode( titleName );
      titleElem.appendChild( titleText );
      layerElem.appendChild( titleElem );

      QString abstract = currentLayer->abstract();
      if ( !abstract.isEmpty() )
      {
        QDomElement abstractElem = doc.createElement( "Abstract" );
        QDomText abstractText = doc.createTextNode( abstract );
        abstractElem.appendChild( abstractText );
        layerElem.appendChild( abstractElem );
      }

      //CRS
      QStringList crsList = createCRSListForLayer( currentLayer );
      appendCRSElementsToLayer( layerElem, doc, crsList );

      //Ex_GeographicBoundingBox
      appendLayerBoundingBoxes( layerElem, doc, currentLayer->extent(), currentLayer->crs() );

      //only one default style in project file mode
      QDomElement styleElem = doc.createElement( "Style" );
      QDomElement styleNameElem = doc.createElement( "Name" );
      QDomText styleNameText = doc.createTextNode( "default" );
      styleNameElem.appendChild( styleNameText );
      QDomElement styleTitleElem = doc.createElement( "Title" );
      QDomText styleTitleText = doc.createTextNode( "default" );
      styleTitleElem.appendChild( styleTitleText );
      styleElem.appendChild( styleNameElem );
      styleElem.appendChild( styleTitleElem );
      layerElem.appendChild( styleElem );
    }
    else
    {
      QgsDebugMsg( "unexpected child element" );
      continue;
    }

    parentElem.appendChild( layerElem );
  }
}

void QgsProjectParser::combineExtentAndCrsOfGroupChildren( QDomElement& groupElem, QDomDocument& doc ) const
{
  QgsRectangle combinedBBox;
  QSet<QString> combinedCRSSet;
  bool firstBBox = true;
  bool firstCRSSet = true;

  QDomNodeList layerChildren = groupElem.childNodes();
  for ( int j = 0; j < layerChildren.size(); ++j )
  {
    QDomElement childElem = layerChildren.at( j ).toElement();

    if ( childElem.tagName() != "Layer" )
      continue;

    QgsRectangle bbox = layerBoundingBoxInProjectCRS( childElem, doc );
    if ( !bbox.isEmpty() )
    {
      if ( firstBBox )
      {
        combinedBBox = bbox;
        firstBBox = false;
      }
      else
      {
        combinedBBox.combineExtentWith( &bbox );
      }
    }

    //combine crs set
    QSet<QString> crsSet;
    if ( crsSetForLayer( childElem, crsSet ) )
    {
      if ( firstCRSSet )
      {
        combinedCRSSet = crsSet;
        firstCRSSet = false;
      }
      else
      {
        combinedCRSSet.intersect( crsSet );
      }
    }
  }

  appendCRSElementsToLayer( groupElem, doc, combinedCRSSet.toList() );

  const QgsCoordinateReferenceSystem& groupCRS = projectCRS();
  appendLayerBoundingBoxes( groupElem, doc, combinedBBox, groupCRS );
}

QList<QgsMapLayer*> QgsProjectParser::mapLayerFromStyle( const QString& lName, const QString& styleName, bool useCache ) const
{
  Q_UNUSED( styleName );
  QList<QgsMapLayer*> layerList;

  if ( !mXMLDoc )
  {
    return layerList;
  }

  //does lName refer to a leaf layer
  QHash< QString, QDomElement >::const_iterator layerElemIt = mProjectLayerElementsByName.find( lName );
  if ( layerElemIt != mProjectLayerElementsByName.constEnd() )
  {
    QgsMapLayer* layer = createLayerFromElement( layerElemIt.value(), useCache );
    if ( layer )
    {
      layerList.push_back( layer );
      return layerList;
    }
  }

  //group or project name
  QDomElement groupElement;
  if ( lName == projectTitle() )
  {
    groupElement = mXMLDoc->documentElement().firstChildElement( "legend" );
  }
  else
  {
    QList<QDomElement>::const_iterator groupIt = mLegendGroupElements.constBegin();
    for ( ; groupIt != mLegendGroupElements.constEnd(); ++groupIt )
    {
      if ( groupIt->attribute( "name" ) == lName )
      {
        groupElement = *groupIt;
        break;
      }
    }
  }

  if ( !groupElement.isNull() )
  {
    //embedded group has no children in this project file
    if ( groupElement.attribute( "embedded" ) == "1" )
    {
      addLayersFromGroup( groupElement, layerList, useCache );
      return layerList;
    }

    //group element found, iterate children and call addLayersFromGroup / addLayerFromLegendLayer for each
    QDomNodeList childList = groupElement.childNodes();
    for ( uint i = 0; i < childList.length(); ++i )
    {
      QDomElement childElem = childList.at( i ).toElement();
      if ( childElem.tagName() == "legendgroup" )
      {
        addLayersFromGroup( childElem, layerList, useCache );
      }
      else if ( childElem.tagName() == "legendlayer" )
      {
        addLayerFromLegendLayer( childElem, layerList, useCache );
      }
    }
    return layerList;
  }

  //still not found. Check if it is a single embedded layer (embedded layers are not contained in mProjectLayerElementsByName)
  QDomElement legendElement = mXMLDoc->documentElement().firstChildElement( "legend" );
  QDomNodeList legendLayerList = legendElement.elementsByTagName( "legendlayer" );
  for ( int i = 0; i < legendLayerList.size(); ++i )
  {
    QDomElement legendLayerElem = legendLayerList.at( i ).toElement();
    if ( legendLayerElem.attribute( "name" ) == lName )
    {
      addLayerFromLegendLayer( legendLayerElem, layerList, useCache );
    }
  }

  //Still not found. Probably it is a layer or a subgroup in an embedded group
  //go through all groups
  //check if they are embedded
  //if yes, request leaf layers and groups from project parser
  QList<QDomElement>::const_iterator legendIt = mLegendGroupElements.constBegin();
  for ( ; legendIt != mLegendGroupElements.constEnd(); ++legendIt )
  {
    if ( legendIt->attribute( "embedded" ) == "1" )
    {
      QString project = convertToAbsolutePath( legendIt->attribute( "project" ) );
      QgsProjectParser* p = dynamic_cast<QgsProjectParser*>( QgsConfigCache::instance()->searchConfiguration( project ) );
      if ( p )
      {
        const QHash< QString, QDomElement >& pLayerByName = p->mProjectLayerElementsByName;
        QHash< QString, QDomElement >::const_iterator pLayerNameIt = pLayerByName.find( lName );
        if ( pLayerNameIt != pLayerByName.constEnd() )
        {
          layerList.push_back( p->createLayerFromElement( pLayerNameIt.value(), useCache ) );
          break;
        }

        QList<QDomElement>::const_iterator pLegendGroupIt = p->mLegendGroupElements.constBegin();
        for ( ; pLegendGroupIt != p->mLegendGroupElements.constEnd(); ++pLegendGroupIt )
        {
          if ( pLegendGroupIt->attribute( "name" ) == lName )
          {
            p->addLayersFromGroup( *pLegendGroupIt, layerList, useCache );
            break;
          }
        }
      }
    }
  }

  return layerList;
}

void QgsProjectParser::addLayersFromGroup( const QDomElement& legendGroupElem, QList<QgsMapLayer*>& layerList, bool useCache ) const
{
  if ( legendGroupElem.attribute( "embedded" ) == "1" ) //embedded group
  {
    //get project parser
    //get group elements from project parser, find the group
    //iterate over layers and add them (embedding in embedded groups does not work)
    QString groupName = legendGroupElem.attribute( "name" );
    QString project = convertToAbsolutePath( legendGroupElem.attribute( "project" ) );
    QgsProjectParser* p = dynamic_cast<QgsProjectParser*>( QgsConfigCache::instance()->searchConfiguration( project ) );
    if ( !p )
    {
      return;
    }

    QList<QDomElement>  pLegendGroupElems = p->mLegendGroupElements;
    QList<QDomElement>::const_iterator pGroupIt = pLegendGroupElems.constBegin();
    for ( ; pGroupIt != pLegendGroupElems.constEnd(); ++pGroupIt )
    {
      if ( pGroupIt->attribute( "name" ) == groupName )
      {
        p->addLayersFromGroup( *pGroupIt, layerList, useCache );
        return;
      }
    }
  }
  else //normal group
  {
    QDomNodeList groupElemChildren = legendGroupElem.childNodes();
    for ( int i = 0; i < groupElemChildren.size(); ++i )
    {
      QDomElement elem = groupElemChildren.at( i ).toElement();
      if ( elem.tagName() == "legendgroup" )
      {
        addLayersFromGroup( elem, layerList, useCache );
      }
      else if ( elem.tagName() == "legendlayer" )
      {
        addLayerFromLegendLayer( elem, layerList, useCache );
      }
    }
  }
}

void QgsProjectParser::addLayerFromLegendLayer( const QDomElement& legendLayerElem, QList<QgsMapLayer*>& layerList, bool useCache ) const
{
  //get layer id
  //search dom element for <maplayer> element
  //call createLayerFromElement()

  QString id = legendLayerElem.firstChild().firstChild().toElement().attribute( "layerid" );
  QHash< QString, QDomElement >::const_iterator layerIt = mProjectLayerElementsById.find( id );
  if ( layerIt != mProjectLayerElementsById.constEnd() )
  {
    QgsMapLayer* layer = createLayerFromElement( layerIt.value(), useCache );
    if ( layer )
    {
      layerList.append( layer );
    }
  }
}

int QgsProjectParser::layersAndStyles( QStringList& layers, QStringList& styles ) const
{
  layers.clear();
  styles.clear();

  QList<QDomElement>::const_iterator elemIt = mProjectLayerElements.constBegin();

  QString currentLayerName;

  for ( ; elemIt != mProjectLayerElements.constEnd(); ++elemIt )
  {
    currentLayerName = layerName( *elemIt );
    if ( !currentLayerName.isNull() )
    {
      layers << currentLayerName;
      styles << "default";
    }
  }
  return 0;
}

QDomDocument QgsProjectParser::getStyle( const QString& styleName, const QString& layerName ) const
{
  Q_UNUSED( styleName );
  Q_UNUSED( layerName );
  return QDomDocument();
}

QgsMapRenderer::OutputUnits QgsProjectParser::outputUnits() const
{
  return QgsMapRenderer::Millimeters;
}

QStringList QgsProjectParser::identifyDisabledLayers() const
{
  QStringList disabledList;
  if ( !mXMLDoc )
  {
    return disabledList;
  }

  QDomElement qgisElem = mXMLDoc->documentElement();
  if ( qgisElem.isNull() )
  {
    return disabledList;
  }
  QDomElement propertiesElem = qgisElem.firstChildElement( "properties" );
  if ( propertiesElem.isNull() )
  {
    return disabledList;
  }
  QDomElement identifyElem = propertiesElem.firstChildElement( "Identify" );
  if ( identifyElem.isNull() )
  {
    return disabledList;
  }
  QDomElement disabledLayersElem = identifyElem.firstChildElement( "disabledLayers" );
  if ( disabledLayersElem.isNull() )
  {
    return disabledList;
  }
  QDomNodeList valueList = disabledLayersElem.elementsByTagName( "value" );
  for ( int i = 0; i < valueList.size(); ++i )
  {
    disabledList << valueList.at( i ).toElement().text();
  }
  return disabledList;
}

QStringList QgsProjectParser::wfsLayers() const
{
  QStringList wfsList;
  if ( !mXMLDoc )
  {
    return wfsList;
  }

  QDomElement qgisElem = mXMLDoc->documentElement();
  if ( qgisElem.isNull() )
  {
    return wfsList;
  }
  QDomElement propertiesElem = qgisElem.firstChildElement( "properties" );
  if ( propertiesElem.isNull() )
  {
    return wfsList;
  }
  QDomElement wfsLayersElem = propertiesElem.firstChildElement( "WFSLayers" );
  if ( wfsLayersElem.isNull() )
  {
    return wfsList;
  }
  QDomNodeList valueList = wfsLayersElem.elementsByTagName( "value" );
  for ( int i = 0; i < valueList.size(); ++i )
  {
    wfsList << valueList.at( i ).toElement().text();
  }
  return wfsList;
}

QStringList QgsProjectParser::supportedOutputCrsList() const
{
  QStringList crsList;
  if ( !mXMLDoc )
  {
    return crsList;
  }

  QDomElement qgisElem = mXMLDoc->documentElement();
  if ( qgisElem.isNull() )
  {
    return crsList;
  }
  QDomElement propertiesElem = qgisElem.firstChildElement( "properties" );
  if ( propertiesElem.isNull() )
  {
    return crsList;
  }
  QDomElement wmsCrsElem = propertiesElem.firstChildElement( "WMSCrsList" );
  if ( !wmsCrsElem.isNull() )
  {
    QDomNodeList valueList = wmsCrsElem.elementsByTagName( "value" );
    for ( int i = 0; i < valueList.size(); ++i )
    {
      crsList.append( valueList.at( i ).toElement().text() );
    }
  }
  else
  {
    QDomElement wmsEpsgElem = propertiesElem.firstChildElement( "WMSEpsgList" );
    if ( wmsEpsgElem.isNull() )
    {
      return crsList;
    }
    QDomNodeList valueList = wmsEpsgElem.elementsByTagName( "value" );
    bool conversionOk;
    for ( int i = 0; i < valueList.size(); ++i )
    {
      int epsgNr = valueList.at( i ).toElement().text().toInt( &conversionOk );
      if ( conversionOk )
      {
        crsList.append( QString( "EPSG:%1" ).arg( epsgNr ) );
      }
    }
  }

  return crsList;
}

bool QgsProjectParser::featureInfoWithWktGeometry() const
{
  if ( !mXMLDoc )
  {
    return false;
  }

  QDomElement qgisElem = mXMLDoc->documentElement();
  if ( qgisElem.isNull() )
  {
    return false;
  }
  QDomElement propertiesElem = qgisElem.firstChildElement( "properties" );
  if ( propertiesElem.isNull() )
  {
    return false;
  }
  QDomElement wktElem = propertiesElem.firstChildElement( "WMSAddWktGeometry" );
  if ( wktElem.isNull() )
  {
    return false;
  }

  return ( wktElem.text().compare( "true", Qt::CaseInsensitive ) == 0 );
}

QMap< QString, QMap< int, QString > > QgsProjectParser::layerAliasInfo() const
{
  QMap< QString, QMap< int, QString > > resultMap;

  QList<QDomElement>::const_iterator layerIt = mProjectLayerElements.constBegin();
  for ( ; layerIt != mProjectLayerElements.constEnd(); ++layerIt )
  {
    QDomNodeList aNodeList = layerIt->elementsByTagName( "aliases" );
    if ( aNodeList.size() > 0 )
    {
      QMap<int, QString> aliasMap;
      QDomNodeList aliasNodeList = aNodeList.at( 0 ).toElement().elementsByTagName( "alias" );
      for ( int i = 0; i < aliasNodeList.size(); ++i )
      {
        QDomElement aliasElem = aliasNodeList.at( i ).toElement();
        aliasMap.insert( aliasElem.attribute( "index" ).toInt(), aliasElem.attribute( "name" ) );
      }
      resultMap.insert( layerId( *layerIt ) , aliasMap );
    }
  }

  return resultMap;
}

QMap< QString, QSet<QString> > QgsProjectParser::hiddenAttributes() const
{
  QMap< QString, QSet<QString> > resultMap;
  QList<QDomElement>::const_iterator layerIt = mProjectLayerElements.constBegin();
  for ( ; layerIt != mProjectLayerElements.constEnd(); ++layerIt )
  {
    QDomNodeList editTypesList = layerIt->elementsByTagName( "edittypes" );
    if ( editTypesList.size() > 0 )
    {
      QSet< QString > hiddenAttributes;
      QDomElement editTypesElem = editTypesList.at( 0 ).toElement();
      QDomNodeList editTypeList = editTypesElem.elementsByTagName( "edittype" );
      for ( int i = 0; i < editTypeList.size(); ++i )
      {
        QDomElement editTypeElem = editTypeList.at( i ).toElement();
        if ( editTypeElem.attribute( "type" ).toInt() == QgsVectorLayer::Hidden )
        {
          hiddenAttributes.insert( editTypeElem.attribute( "name" ) );
        }
      }

      if ( hiddenAttributes.size() > 0 )
      {
        resultMap.insert( layerId( *layerIt ), hiddenAttributes );
      }
    }
  }

  return resultMap;
}

QgsRectangle QgsProjectParser::mapRectangle() const
{
  if ( !mXMLDoc )
  {
    return QgsRectangle();
  }

  QDomElement qgisElem = mXMLDoc->documentElement();
  if ( qgisElem.isNull() )
  {
    return QgsRectangle();
  }

  QDomElement propertiesElem = qgisElem.firstChildElement( "properties" );
  if ( propertiesElem.isNull() )
  {
    return QgsRectangle();
  }

  QDomElement extentElem = propertiesElem.firstChildElement( "WMSExtent" );
  if ( extentElem.isNull() )
  {
    return QgsRectangle();
  }

  QDomNodeList valueNodeList = extentElem.elementsByTagName( "value" );
  if ( valueNodeList.size() < 4 )
  {
    return QgsRectangle();
  }

  //order of value elements must be xmin, ymin, xmax, ymax
  double xmin = valueNodeList.at( 0 ).toElement().text().toDouble();
  double ymin = valueNodeList.at( 1 ).toElement().text().toDouble();
  double xmax = valueNodeList.at( 2 ).toElement().text().toDouble();
  double ymax = valueNodeList.at( 3 ).toElement().text().toDouble();
  return QgsRectangle( xmin, ymin, xmax, ymax );
}

QString QgsProjectParser::mapAuthid() const
{
  if ( !mXMLDoc )
  {
    return QString::null;
  }

  QDomElement qgisElem = mXMLDoc->documentElement();
  if ( qgisElem.isNull() )
  {
    return QString::null;
  }

  QDomElement mapCanvasElem = qgisElem.firstChildElement( "mapcanvas" );
  if ( mapCanvasElem.isNull() )
  {
    return QString::null;
  }

  QDomElement srsElem = mapCanvasElem.firstChildElement( "destinationsrs" );
  if ( srsElem.isNull() )
  {
    return QString::null;
  }

  QDomNodeList authIdNodes = srsElem.elementsByTagName( "authid" );
  if ( authIdNodes.size() < 1 )
  {
    return QString::null;
  }

  return authIdNodes.at( 0 ).toElement().text();
}

QString QgsProjectParser::projectTitle() const
{
  if ( !mXMLDoc )
  {
    return QString();
  }

  QDomElement qgisElem = mXMLDoc->documentElement();
  if ( qgisElem.isNull() )
  {
    return QString();
  }

  QDomElement titleElem = qgisElem.firstChildElement( "title" );
  if ( !titleElem.isNull() )
  {
    QString title = titleElem.text();
    if ( !title.isEmpty() )
    {
      return title;
    }
  }

  //no title element or not project title set. Use project filename without extension
  QFileInfo projectFileInfo( mProjectPath );
  return projectFileInfo.baseName();
}

QgsMapLayer* QgsProjectParser::createLayerFromElement( const QDomElement& elem, bool useCache ) const
{
  if ( elem.isNull() || !mXMLDoc )
  {
    return 0;
  }

  QDomElement dataSourceElem = elem.firstChildElement( "datasource" );
  QString uri = dataSourceElem.text();
  QString absoluteUri;
  if ( !dataSourceElem.isNull() )
  {
    //convert relative pathes to absolute ones if necessary
    if ( uri.startsWith( "dbname" ) ) //database
    {
      QgsDataSourceURI dsUri( uri );
      if ( dsUri.host().isEmpty() ) //only convert path for file based databases
      {
        QString dbnameUri = dsUri.database();
        QString dbNameUriAbsolute = convertToAbsolutePath( dbnameUri );
        if ( dbnameUri != dbNameUriAbsolute )
        {
          dsUri.setDatabase( dbNameUriAbsolute );
          absoluteUri = dsUri.uri();
          QDomText absoluteTextNode = mXMLDoc->createTextNode( absoluteUri );
          dataSourceElem.replaceChild( absoluteTextNode, dataSourceElem.firstChild() );
        }
      }
    }
    else //file based data source
    {
      absoluteUri = convertToAbsolutePath( uri );
      if ( uri != absoluteUri )
      {
        QDomText absoluteTextNode = mXMLDoc->createTextNode( absoluteUri );
        dataSourceElem.replaceChild( absoluteTextNode, dataSourceElem.firstChild() );
      }
    }
  }

  QString id = layerId( elem );
  QgsMapLayer* layer = 0;
  if ( useCache )
  {
    layer = QgsMSLayerCache::instance()->searchLayer( absoluteUri, id );
  }

  if ( layer )
  {
    return layer;
  }

  QString type = elem.attribute( "type" );
  if ( type == "vector" )
  {
    layer = new QgsVectorLayer();
  }
  else if ( type == "raster" )
  {
    layer = new QgsRasterLayer();
  }
  else if ( elem.attribute( "embedded" ) == "1" ) //layer is embedded from another project file
  {
    QString project = convertToAbsolutePath( elem.attribute( "project" ) );
    QgsDebugMsg( QString( "Project path: %1" ).arg( project ) );
    QgsProjectParser* otherConfig = dynamic_cast<QgsProjectParser*>( QgsConfigCache::instance()->searchConfiguration( project ) );
    if ( !otherConfig )
    {
      return 0;
    }

    QHash< QString, QDomElement >::const_iterator layerIt = otherConfig->mProjectLayerElementsById.find( elem.attribute( "id" ) );
    if ( layerIt == otherConfig->mProjectLayerElementsById.constEnd() )
    {
      return 0;
    }
    return otherConfig->createLayerFromElement( layerIt.value() );
  }

  if ( layer )
  {
    layer->readXML( const_cast<QDomElement&>( elem ) ); //should be changed to const in QgsMapLayer
    layer->setLayerName( layerName( elem ) );
    if ( useCache )
    {
      QgsMSLayerCache::instance()->insertLayer( absoluteUri, id, layer, mProjectPath );
    }
    else
    {
      mLayersToRemove.push_back( layer );
    }
  }
  return layer;
}

QString QgsProjectParser::layerId( const QDomElement& layerElem ) const
{
  if ( layerElem.isNull() )
  {
    return QString();
  }

  QDomElement idElem = layerElem.firstChildElement( "id" );
  if ( idElem.isNull() )
  {
    //embedded layer have id attribute instead of id child element
    return layerElem.attribute( "id" );
  }
  return idElem.text();
}

QString QgsProjectParser::layerName( const QDomElement& layerElem ) const
{
  if ( layerElem.isNull() )
  {
    return QString();
  }

  QDomElement nameElem = layerElem.firstChildElement( "layername" );
  if ( nameElem.isNull() )
  {
    return QString();
  }
  return nameElem.text().replace( "," , "%60" ); //commas are not allowed in layer names
}

void QgsProjectParser::setLegendParametersFromProject()
{
  if ( !mXMLDoc )
  {
    return;
  }

  QDomElement documentElem = mXMLDoc->documentElement();
  if ( documentElem.isNull() )
  {
    return;
  }

  QDomElement composerElem = documentElem.firstChildElement( "Composer" );
  if ( composerElem.isNull() )
  {
    return;
  }
  QDomElement composerLegendElem = composerElem.firstChildElement( "ComposerLegend" );
  if ( composerLegendElem.isNull() )
  {
    return;
  }

  mLegendBoxSpace = composerLegendElem.attribute( "boxSpace" ).toDouble();
  mLegendLayerSpace = composerLegendElem.attribute( "layerSpace" ).toDouble();
  mLegendSymbolSpace = composerLegendElem.attribute( "symbolSpace" ).toDouble();
  mLegendIconLabelSpace = composerLegendElem.attribute( "iconLabelSpace" ).toDouble();
  mLegendSymbolWidth = composerLegendElem.attribute( "symbolWidth" ).toDouble();
  mLegendSymbolHeight = composerLegendElem.attribute( "symbolHeight" ).toDouble();
  mLegendLayerFont.fromString( composerLegendElem.attribute( "layerFont" ) );
  mLegendItemFont.fromString( composerLegendElem.attribute( "itemFont" ) );
}

QList< GroupLayerInfo > QgsProjectParser::groupLayerRelationshipFromProject() const
{
  QList< GroupLayerInfo > resultList;
  if ( !mXMLDoc )
  {
    return resultList;
  }

  QDomElement documentElem = mXMLDoc->documentElement();
  if ( documentElem.isNull() )
  {
    return resultList;
  }
  QDomElement legendElem = documentElem.firstChildElement( "legend" );
  if ( legendElem.isNull() )
  {
    return resultList;
  }

  QDomNodeList legendChildren = legendElem.childNodes();
  QDomElement currentChildElem;
  for ( int i = 0; i < legendChildren.size(); ++i )
  {
    QList< QString > layerIdList;
    currentChildElem = legendChildren.at( i ).toElement();
    if ( currentChildElem.isNull() )
    {
      continue;
    }
    else if ( currentChildElem.tagName() == "legendlayer" )
    {
      layerIdList.push_back( layerIdFromLegendLayer( currentChildElem ) );
      resultList.push_back( qMakePair( QString(), layerIdList ) );
    }
    else if ( currentChildElem.tagName() == "legendgroup" )
    {
      QDomNodeList childLayerList = currentChildElem.elementsByTagName( "legendlayer" );
      QString groupName = currentChildElem.attribute( "name" ).replace( "," , "%60" );
      QString currentLayerId;

      for ( int j = 0; j < childLayerList.size(); ++j )
      {
        layerIdList.push_back( layerIdFromLegendLayer( childLayerList.at( j ).toElement() ) );
      }
      resultList.push_back( qMakePair( groupName, layerIdList ) );
    }
  }

  return resultList;
}

QString QgsProjectParser::layerIdFromLegendLayer( const QDomElement& legendLayer ) const
{
  if ( legendLayer.isNull() )
  {
    return QString();
  }

  QDomNodeList legendLayerFileList = legendLayer.elementsByTagName( "legendlayerfile" );
  if ( legendLayerFileList.size() < 1 )
  {
    return QString();
  }

  return legendLayerFileList.at( 0 ).toElement().attribute( "layerid" );
}

QgsComposition* QgsProjectParser::initComposition( const QString& composerTemplate, QgsMapRenderer* mapRenderer, QList< QgsComposerMap*>& mapList, QList< QgsComposerLabel* >& labelList ) const
{
  //Create composition from xml
  QDomElement composerElem = composerByName( composerTemplate );
  if ( composerElem.isNull() )
  {
    throw QgsMapServiceException( "Error", "Composer template not found" );
  }

  QDomElement compositionElem = composerElem.firstChildElement( "Composition" );
  if ( compositionElem.isNull() )
  {
    return 0;
  }

  QgsComposition* composition = new QgsComposition( mapRenderer ); //set resolution, paper size from composer element attributes
  if ( !composition->readXML( compositionElem, *mXMLDoc ) )
  {
    delete composition;
    return 0;
  }

  composition->addItemsFromXML( compositionElem, *mXMLDoc );
  composition->composerItems( mapList );
  composition->composerItems( labelList );

  QList< QgsComposerPicture* > pictureList;
  composition->composerItems( pictureList );
  QList< QgsComposerPicture* >::iterator picIt = pictureList.begin();
  for ( ; picIt != pictureList.end(); ++picIt )
  {
    ( *picIt )->setPictureFile( convertToAbsolutePath(( *picIt )->pictureFile() ) );
  }

  return composition;
}

void QgsProjectParser::printCapabilities( QDomElement& parentElement, QDomDocument& doc ) const
{
  if ( !mXMLDoc )
  {
    return;
  }

  QDomNodeList composerNodeList = mXMLDoc->elementsByTagName( "Composer" );
  if ( composerNodeList.size() < 1 )
  {
    return;
  }

  QDomElement composerTemplatesElem = doc.createElement( "ComposerTemplates" );
  composerTemplatesElem.setAttribute( "xmlns:wms", "http://www.opengis.net/wms" );
  composerTemplatesElem.setAttribute( "xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance" );
  composerTemplatesElem.setAttribute( "xsi:type", "wms:_ExtendedCapabilities" );

  for ( int i = 0; i < composerNodeList.size(); ++i )
  {
    QDomElement composerTemplateElem = doc.createElement( "ComposerTemplate" );
    QDomElement currentComposerElem = composerNodeList.at( i ).toElement();
    if ( currentComposerElem.isNull() )
    {
      continue;
    }

    composerTemplateElem.setAttribute( "name", currentComposerElem.attribute( "title" ) );

    //get paper width and hight in mm from composition
    QDomElement compositionElem = currentComposerElem.firstChildElement( "Composition" );
    if ( compositionElem.isNull() )
    {
      continue;
    }
    composerTemplateElem.setAttribute( "width", compositionElem.attribute( "paperWidth" ) );
    composerTemplateElem.setAttribute( "height", compositionElem.attribute( "paperHeight" ) );


    //add available composer maps and their size in mm
    QDomNodeList composerMapList = currentComposerElem.elementsByTagName( "ComposerMap" );
    for ( int j = 0; j < composerMapList.size(); ++j )
    {
      QDomElement cmap = composerMapList.at( j ).toElement();
      QDomElement citem = cmap.firstChildElement( "ComposerItem" );
      if ( citem.isNull() )
      {
        continue;
      }

      QDomElement composerMapElem = doc.createElement( "ComposerMap" );
      composerMapElem.setAttribute( "name", "map" + cmap.attribute( "id" ) );
      composerMapElem.setAttribute( "width", citem.attribute( "width" ) );
      composerMapElem.setAttribute( "height", citem.attribute( "height" ) );
      composerTemplateElem.appendChild( composerMapElem );
    }

    //add available composer labels
    QDomNodeList composerLabelList = currentComposerElem.elementsByTagName( "ComposerLabel" );
    for ( int j = 0; j < composerLabelList.size(); ++j )
    {
      QDomElement citem = composerLabelList.at( j ).firstChildElement( "ComposerItem" );
      QString id = citem.attribute( "id" );
      if ( id.isEmpty() ) //only export labels with ids for text replacement
      {
        continue;
      }
      QDomElement composerLabelElem = doc.createElement( "ComposerLabel" );
      composerLabelElem.setAttribute( "name", id );
      composerTemplateElem.appendChild( composerLabelElem );
    }

    composerTemplatesElem.appendChild( composerTemplateElem );
  }
  parentElement.appendChild( composerTemplatesElem );
}

QDomElement QgsProjectParser::composerByName( const QString& composerName ) const
{
  QDomElement composerElem;
  if ( !mXMLDoc )
  {
    return composerElem;
  }

  QDomNodeList composerNodeList = mXMLDoc->elementsByTagName( "Composer" );
  for ( int i = 0; i < composerNodeList.size(); ++i )
  {
    QDomElement currentComposerElem = composerNodeList.at( i ).toElement();
    if ( currentComposerElem.attribute( "title" ) == composerName )
    {
      return currentComposerElem;
    }
  }

  return composerElem;
}

void QgsProjectParser::serviceCapabilities( QDomElement& parentElement, QDomDocument& doc ) const
{
  QDomElement serviceElem = doc.createElement( "Service" );

  QDomElement propertiesElem = mXMLDoc->documentElement().firstChildElement( "properties" );
  if ( propertiesElem.isNull() )
  {
    QgsConfigParser::serviceCapabilities( parentElement, doc );
    return;
  }

  QDomElement serviceCapabilityElem = propertiesElem.firstChildElement( "WMSServiceCapabilities" );
  if ( serviceCapabilityElem.isNull() || serviceCapabilityElem.text().compare( "true", Qt::CaseInsensitive ) != 0 )
  {
    QgsConfigParser::serviceCapabilities( parentElement, doc );
    return;
  }

  //Service name is always WMS
  QDomElement wmsNameElem = doc.createElement( "Name" );
  QDomText wmsNameText = doc.createTextNode( "WMS" );
  wmsNameElem.appendChild( wmsNameText );
  serviceElem.appendChild( wmsNameElem );

  //WMS title
  QDomElement titleElem = propertiesElem.firstChildElement( "WMSServiceTitle" );
  if ( !titleElem.isNull() )
  {
    QDomElement wmsTitleElem = doc.createElement( "Title" );
    QDomText wmsTitleText = doc.createTextNode( titleElem.text() );
    wmsTitleElem.appendChild( wmsTitleText );
    serviceElem.appendChild( wmsTitleElem );
  }

  //WMS abstract
  QDomElement abstractElem = propertiesElem.firstChildElement( "WMSServiceAbstract" );
  if ( !abstractElem.isNull() )
  {
    QDomElement wmsAbstractElem = doc.createElement( "Abstract" );
    QDomText wmsAbstractText = doc.createTextNode( abstractElem.text() );
    wmsAbstractElem.appendChild( wmsAbstractText );
    serviceElem.appendChild( wmsAbstractElem );
  }

  //OnlineResource element is mandatory according to the WMS specification
  QDomElement wmsOnlineResourceElem = propertiesElem.firstChildElement( "WMSOnlineResource" );
  QDomElement onlineResourceElem = doc.createElement( "OnlineResource" );
  onlineResourceElem.setAttribute( "xmlns:xlink", "http://www.w3.org/1999/xlink" );
  onlineResourceElem.setAttribute( "xlink:type", "simple" );
  if ( !wmsOnlineResourceElem.isNull() )
  {
    onlineResourceElem.setAttribute( "xlink:href", wmsOnlineResourceElem.text() );
  }

  serviceElem.appendChild( onlineResourceElem );

  //Contact information
  QDomElement contactInfoElem = doc.createElement( "ContactInformation" );

  //Contact person primary
  QDomElement contactPersonPrimaryElem = doc.createElement( "ContactPersonPrimary" );

  //Contact person
  QDomElement contactPersonElem = propertiesElem.firstChildElement( "WMSContactPerson" );
  QString contactPersonString;
  if ( !contactPersonElem.isNull() )
  {
    contactPersonString = contactPersonElem.text();
  }
  QDomElement wmsContactPersonElem = doc.createElement( "ContactPerson" );
  QDomText contactPersonText = doc.createTextNode( contactPersonString );
  wmsContactPersonElem.appendChild( contactPersonText );
  contactPersonPrimaryElem.appendChild( wmsContactPersonElem );


  //Contact organisation
  QDomElement contactOrganizationElem = propertiesElem.firstChildElement( "WMSContactOrganization" );
  QString contactOrganizationString;
  if ( !contactOrganizationElem.isNull() )
  {
    contactOrganizationString = contactOrganizationElem.text();
  }
  QDomElement wmsContactOrganizationElem = doc.createElement( "ContactOrganization" );
  QDomText contactOrganizationText = doc.createTextNode( contactOrganizationString );
  wmsContactOrganizationElem.appendChild( contactOrganizationText );
  contactPersonPrimaryElem.appendChild( wmsContactOrganizationElem );
  contactInfoElem.appendChild( contactPersonPrimaryElem );

  //phone
  QDomElement phoneElem = propertiesElem.firstChildElement( "WMSContactPhone" );
  if ( !phoneElem.isNull() )
  {
    QDomElement wmsPhoneElem = doc.createElement( "ContactVoiceTelephone" );
    QDomText wmsPhoneText = doc.createTextNode( phoneElem.text() );
    wmsPhoneElem.appendChild( wmsPhoneText );
    contactInfoElem.appendChild( wmsPhoneElem );
  }

  //mail
  QDomElement mailElem = propertiesElem.firstChildElement( "WMSContactMail" );
  if ( !mailElem.isNull() )
  {
    QDomElement wmsMailElem = doc.createElement( "ContactElectronicMailAddress" );
    QDomText wmsMailText = doc.createTextNode( mailElem.text() );
    wmsMailElem.appendChild( wmsMailText );
    contactInfoElem.appendChild( wmsMailElem );
  }

  serviceElem.appendChild( contactInfoElem );

  //MaxWidth / MaxHeight for WMS 1.3
  QString version = doc.documentElement().attribute( "version" );
  if ( version != "1.1.1" )
  {
    if ( mMaxWidth != -1 )
    {
      QDomElement maxWidthElem = doc.createElement( "MaxWidth" );
      QDomText maxWidthText = doc.createTextNode( QString::number( mMaxWidth ) );
      maxWidthElem.appendChild( maxWidthText );
      serviceElem.appendChild( maxWidthElem );
    }
    if ( mMaxHeight != -1 )
    {
      QDomElement maxHeightElem = doc.createElement( "MaxHeight" );
      QDomText maxHeightText = doc.createTextNode( QString::number( mMaxHeight ) );
      maxHeightElem.appendChild( maxHeightText );
      serviceElem.appendChild( maxHeightElem );
    }
  }

  parentElement.appendChild( serviceElem );
}

QString QgsProjectParser::serviceUrl() const
{
  QString url;

  if ( !mXMLDoc )
  {
    return url;
  }

  QDomElement propertiesElem = mXMLDoc->documentElement().firstChildElement( "properties" );
  if ( !propertiesElem.isNull() )
  {
    QDomElement wmsUrlElem = propertiesElem.firstChildElement( "WMSUrl" );
    if ( !wmsUrlElem.isNull() )
    {
      url = wmsUrlElem.text();
    }
  }
  return url;
}

QString QgsProjectParser::convertToAbsolutePath( const QString& file ) const
{
  if ( !file.startsWith( "./" ) && !file.startsWith( "../" ) )
  {
    return file;
  }

  QString srcPath = file;
  QString projPath = mProjectPath;

#if defined(Q_OS_WIN)
  srcPath.replace( "\\", "/" );
  projPath.replace( "\\", "/" );

  bool uncPath = projPath.startsWith( "//" );
#endif

  QStringList srcElems = file.split( "/", QString::SkipEmptyParts );
  QStringList projElems = mProjectPath.split( "/", QString::SkipEmptyParts );

#if defined(Q_OS_WIN)
  if ( uncPath )
  {
    projElems.insert( 0, "" );
    projElems.insert( 0, "" );
  }
#endif

  // remove project file element
  projElems.removeLast();

  // append source path elements
  projElems << srcElems;
  projElems.removeAll( "." );

  // resolve ..
  int pos;
  while (( pos = projElems.indexOf( ".." ) ) > 0 )
  {
    // remove preceding element and ..
    projElems.removeAt( pos - 1 );
    projElems.removeAt( pos - 1 );
  }

#if !defined(Q_OS_WIN)
  // make path absolute
  projElems.prepend( "" );
#endif

  return projElems.join( "/" );
}

void QgsProjectParser::setSelectionColor()
{
  int red = 255;
  int green = 255;
  int blue = 0;
  int alpha = 255;

  //overwrite default selection color with settings from the project
  if ( mXMLDoc )
  {
    QDomElement qgisElem = mXMLDoc->documentElement();
    if ( !qgisElem.isNull() )
    {
      QDomElement propertiesElem = qgisElem.firstChildElement( "properties" );
      if ( !propertiesElem.isNull() )
      {
        QDomElement guiElem = propertiesElem.firstChildElement( "Gui" );
        if ( !guiElem.isNull() )
        {
          QDomElement redElem = guiElem.firstChildElement( "SelectionColorRedPart" );
          if ( !redElem.isNull() )
          {
            red = redElem.text().toInt();
          }
          QDomElement greenElem = guiElem.firstChildElement( "SelectionColorGreenPart" );
          if ( !greenElem.isNull() )
          {
            green = greenElem.text().toInt();
          }
          QDomElement blueElem = guiElem.firstChildElement( "SelectionColorBluePart" );
          if ( !blueElem.isNull() )
          {
            blue = blueElem.text().toInt();
          }
          QDomElement alphaElem = guiElem.firstChildElement( "SelectionColorAlphaPart" );
          if ( !alphaElem.isNull() )
          {
            alpha = alphaElem.text().toInt();
          }
        }
      }
    }
  }

  mSelectionColor = QColor( red, green, blue, alpha );
}

void QgsProjectParser::setMaxWidthHeight()
{
  if ( mXMLDoc )
  {
    QDomElement qgisElem = mXMLDoc->documentElement();
    if ( !qgisElem.isNull() )
    {
      QDomElement propertiesElem = qgisElem.firstChildElement( "properties" );
      if ( !propertiesElem.isNull() )
      {
        QDomElement maxWidthElem = propertiesElem.firstChildElement( "WMSMaxWidth" );
        if ( !maxWidthElem.isNull() )
        {
          mMaxWidth = maxWidthElem.text().toInt();
        }
        QDomElement maxHeightElem = propertiesElem.firstChildElement( "WMSMaxHeight" );
        if ( !maxHeightElem.isNull() )
        {
          mMaxHeight = maxHeightElem.text().toInt();
        }
      }
    }
  }
}

const QgsCoordinateReferenceSystem& QgsProjectParser::projectCRS() const
{
  //mapcanvas->destinationsrs->spatialrefsys->authid
  if ( mXMLDoc )
  {
    QDomElement authIdElem = mXMLDoc->documentElement().firstChildElement( "mapcanvas" ).firstChildElement( "destinationsrs" ).
                             firstChildElement( "spatialrefsys" ).firstChildElement( "authid" );
    if ( !authIdElem.isNull() )
    {
      return QgsCRSCache::instance()->crsByAuthId( authIdElem.text() );
    }
  }
  return QgsCRSCache::instance()->crsByEpsgId( GEO_EPSG_CRS_ID );
}

QgsRectangle QgsProjectParser::layerBoundingBoxInProjectCRS( const QDomElement& layerElem, const QDomDocument &doc ) const
{
  QgsRectangle BBox;
  if ( layerElem.isNull() )
  {
    return BBox;
  }

  //read box coordinates and layer auth. id
  QDomElement boundingBoxElem = layerElem.firstChildElement( "BoundingBox" );
  if ( boundingBoxElem.isNull() )
  {
    return BBox;
  }

  double minx, miny, maxx, maxy;
  bool conversionOk;
  minx = boundingBoxElem.attribute( "minx" ).toDouble( &conversionOk );
  if ( !conversionOk )
  {
    return BBox;
  }
  miny = boundingBoxElem.attribute( "miny" ).toDouble( &conversionOk );
  if ( !conversionOk )
  {
    return BBox;
  }
  maxx = boundingBoxElem.attribute( "maxx" ).toDouble( &conversionOk );
  if ( !conversionOk )
  {
    return BBox;
  }
  maxy = boundingBoxElem.attribute( "maxy" ).toDouble( &conversionOk );
  if ( !conversionOk )
  {
    return BBox;
  }


  QString version = doc.documentElement().attribute( "version" );

  //create layer crs
  const QgsCoordinateReferenceSystem& layerCrs = QgsCRSCache::instance()->crsByAuthId( boundingBoxElem.attribute( version == "1.1.1" ? "SRS" : "CRS" ) );
  if ( !layerCrs.isValid() )
  {
    return BBox;
  }

  BBox.set( minx, miny, maxx, maxy );

  if ( version == "1.3.0" && layerCrs.axisInverted() )
  {
    BBox.invert();
  }

  //get project crs
  const QgsCoordinateReferenceSystem& projectCrs = projectCRS();
  QgsCoordinateTransform t( layerCrs, projectCrs );

  //transform
  BBox = t.transformBoundingBox( BBox );
  return BBox;
}
