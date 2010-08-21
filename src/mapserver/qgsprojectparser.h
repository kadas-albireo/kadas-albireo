/***************************************************************************
                              qgsprojectparser.h
                              ------------------
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

#ifndef QGSPROJECTPARSER_H
#define QGSPROJECTPARSER_H

#include "qgsconfigparser.h"
#include <QList>
#include <QPair>

//Information about relationship between groups and layers
//key: group name (or null strings for single layers without groups)
//value: containter with layer ids contained in the group
typedef QPair< QString, QList<QString> > GroupLayerInfo;

/**QGIS mapserver configuration parser for project files (.qgs)*/
class QgsProjectParser: public QgsConfigParser
{
  public:
    /**Constructor. Takes ownership of xml document*/
    QgsProjectParser( QDomDocument* xmlDoc );
    virtual ~QgsProjectParser();

    /**Adds layer and style specific capabilities elements to the parent node. This includes the individual layers and styles, their description, native CRS, bounding boxes, etc.*/
    virtual void layersAndStylesCapabilities(QDomElement& parentElement, QDomDocument& doc) const;

    /**Returns one or possibly several maplayers for a given layer name and style. If no layers/style are found, an empty list is returned*/
    virtual QList<QgsMapLayer*> mapLayerFromStyle(const QString& lName, const QString& styleName, bool allowCaching = true) const;

    /**Fills a layer and a style list. The two list have the same number of entries and the style and the layer at a position belong together (similar to the HTTP parameters 'Layers' and 'Styles'. Returns 0 in case of success*/
    virtual int layersAndStyles(QStringList& layers, QStringList& styles) const;

    /**Returns the xml fragment of a style*/
    virtual QDomDocument getStyle(const QString& styleName, const QString& layerName) const;

    /**Returns if output are MM or PIXEL*/
    virtual QgsMapRenderer::OutputUnits outputUnits() const;

    /**Adds an external GML dataset. The class takes ownership and deletes all the documents in the destructor*/
    void addExternalGMLData(const QString& layerName, QDomDocument* gmlDoc);

    /**Returns an ID-list of layers which are not queryable (comes from <properties> -> <Identify> -> <disabledLayers in the project file*/
    virtual QStringList identifyDisabledLayers() const;

    /**Returns a set of supported epsg codes for the capabilities document. The list comes from the property <WMSEpsgList> in the project file.
       An empty set means that all possible CRS should be advertised (which could result in very long capabilities documents)
       Example:
       <properties>
          ....
          <WMSEpsgList type="QStringList">
            <value>21781</value>
            <value>4326</value>
          </WMSEpsgList>
      </properties>
    */
    virtual QSet<int> supportedOutputCrsSet() const;

    /**Returns information about vector layer aliases. First key is the layer id, (second) key is the field id, value the alias.
       Default implementation returns an empty map*/
    virtual QMap< QString, QMap< int, QString > > layerAliasInfo() const;

    /**Returns a stringlist containing the names of the attributes with hidden edit types*/
    virtual QMap< QString, QSet<QString> > hiddenAttributes() const;

    /**Returns map rectangle for the project file*/
    QgsRectangle mapRectangle() const;

    /**Returns epsg number of the project crs (or -1 in case of error)*/
    int mapEpsg() const;

    /**Return project title*/
    QString projectTitle() const;

private:
    /**Content of project file*/
    QDomDocument* mXMLDoc;

    /**Get all layers of the project (ordered same as in the project file)*/
    QList<QDomElement> projectLayerElements() const;
    /**Get all layers of the project, accessible by layer id*/
    QMap< QString, QDomElement > projectLayerElementsById() const;
    /**Get all layers of the project, accessible by layer name*/
    QMap< QString, QDomElement > projectLayerElementsByName() const;
    /**Creates a maplayer object from <maplayer> element. The layer cash owns the maplayer, so don't delete it
    @return the maplayer or 0 in case of error*/
    QgsMapLayer* createLayerFromElement( const QDomElement& elem) const;
    /**Returns the text of the <id> element for a layer element
    @return id or a null string in case of error*/
    QString layerId( const QDomElement& layerElem ) const;
    /**Returns the text of the <layername> element for a layer element
    @return id or a null string in case of error*/
    QString layerName( const QDomElement& layerElem ) const;
    /**Sets legend parameters according to the first <ComposerLegend> element in the project file*/
    void setLegendParametersFromProject();
    /**Returns the groups / toplevel layers and the maplayer ids contained in it*/
    QList< GroupLayerInfo > groupLayerRelationshipFromProject() const;
    /**Returns the layer id under a <legendlayer> tag in the QGIS projectfile*/
    QString layerIdFromLegendLayer( const QDomElement& legendLayer ) const;
};

#endif // QGSPROJECTPARSER_H
