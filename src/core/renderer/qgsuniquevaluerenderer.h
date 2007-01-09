/***************************************************************************
                         qgsuniquevaluerenderer.h  -  description
                             -------------------
    begin                : July 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id: qgsuniquevaluerenderer.h 5371 2006-04-25 01:52:13Z wonder $ */
#ifndef QGSUNIQUEVALUERENDERER_H
#define QGSUNIQUEVALUERENDERER_H

#include "qgsrenderer.h"
#include <map>

class CORE_EXPORT QgsUniqueValueRenderer: public QgsRenderer
{
 public:
    QgsUniqueValueRenderer(QGis::VectorType type);
    QgsUniqueValueRenderer(const QgsUniqueValueRenderer& other);
    QgsUniqueValueRenderer& operator=(const QgsUniqueValueRenderer& other);
    virtual ~QgsUniqueValueRenderer();
    void renderFeature(QPainter* p, QgsFeature& f,QImage* img, double* scalefactor, bool selected, double widthScale = 1.);
    /**Reads the renderer configuration from an XML file
     @param rnode the DOM node to read 
     @param vl the vector layer which will be associated with the renderer*/
    void readXML(const QDomNode& rnode, QgsVectorLayer& vl);
    /**Writes the contents of the renderer to a configuration file
     @ return true in case of success*/
    virtual bool writeXML( QDomNode & layer_node, QDomDocument & document ) const;
    /** Returns true, if attribute values are used by the renderer and false otherwise*/
    bool needsAttributes() const;
    /**Returns a list with indexes of classification attributes*/
    QgsAttributeList classificationAttributes() const;
    /**Returns the renderers name*/
    QString name() const;
    /**Inserts an entry into mEntries. The render items have to be created with the new operator and are automatically destroyed if not needed anymore*/
    void insertValue(QString name, QgsSymbol* symbol);
    /**Removes all entries from mEntries*/
    void clearValues();
    /**Sets the Field index used for classification*/
    void setClassificationField(int field);
    /**Returns the index of the classification field*/
    int classificationField();
    /**Return symbology items*/
    const std::list<QgsSymbol*> symbols() const;
    QgsRenderer* clone() const;
 protected:
    /**Field index used for classification*/
    int mClassificationField;
    /**Symbols for the unique values*/
    std::map<QString, QgsSymbol*> mSymbols;
};

inline bool QgsUniqueValueRenderer::needsAttributes() const
{
    return true;
}

#endif
