/***************************************************************************
                         qgssinglesymbolrenderer.h  -  description
                             -------------------
    begin                : Oct 2003
    copyright            : (C) 2003 by Marco Hugentobler
    email                : mhugent@geo.unizh.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id: qgssinglesymbolrenderer.h 5371 2006-04-25 01:52:13Z wonder $ */

#ifndef QGSSINGLESYMBOLRENDERER_H
#define QGSSINGLESYMBOLRENDERER_H

#include "qgsrenderer.h"


/**Render class to display all the features with a single QgsSymbol*/
class CORE_EXPORT QgsSingleSymbolRenderer: public QgsRenderer
{
 public:
    QgsSingleSymbolRenderer(QGis::VectorType type);
    QgsSingleSymbolRenderer(const QgsSingleSymbolRenderer& other);
    QgsSingleSymbolRenderer& operator=(const QgsSingleSymbolRenderer& other);
    virtual ~QgsSingleSymbolRenderer();
    /**Replaces the current mSymbol by sy*/
    void addSymbol(QgsSymbol* sy);
    /*Returns a pointer to mSymbol*/
    const QgsSymbol* symbol() const;
    /**Renders an OGRFeature*/
    void renderFeature(QPainter* p, QgsFeature& f, QImage* img, double* scalefactor, bool selected, double widthScale = 1.);
    /**Reads the renderer configuration from an XML file
     @param rnode the DOM node to read 
     @param vl the vector layer which will be associated with the renderer*/
    virtual void readXML(const QDomNode& rnode, QgsVectorLayer& vl);
    /**Writes the contents of the renderer to a configuration file*/
    /*virtual void writeXML(std::ostream& xml);*/
    /**Writes the contents of the renderer to a configuration file
     @ return true in case of success*/
    virtual bool writeXML( QDomNode & layer_node, QDomDocument & document ) const;
    /**Returns false, no attributes neede for single symbol*/
    bool needsAttributes() const;
    /**Returns an empty list, since no classification attributes are used*/
    QgsAttributeList classificationAttributes() const;
    /**Returns the renderers name*/
    virtual QString name() const;
    /**Returns a list containing mSymbol*/
    const std::list<QgsSymbol*> symbols() const;
    /**Returns a deep copy of this renderer*/
    QgsRenderer* clone() const;
 protected:
    /**Object containing symbology information*/
    QgsSymbol* mSymbol;
};

inline const QgsSymbol* QgsSingleSymbolRenderer::symbol() const
{
    return mSymbol;
}

inline bool QgsSingleSymbolRenderer::needsAttributes() const
{
  return false;
}

#endif
