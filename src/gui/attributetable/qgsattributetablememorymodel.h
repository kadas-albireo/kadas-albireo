/***************************************************************************
  QgsAttributeTableMemoryModel.h - Memory Model for attribute table
  -------------------
         date                 : Feb 2009
         copyright            : Vita Cizek
         email                : weetya (at) gmail.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSATTRIBUTETABLEMEMORYMODEL_H
#define QGSATTRIBUTETABLEMEMORYMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>
#include <QHash>

//QGIS Includes
#include "qgsfeature.h" //QgsAttributeMap
#include "qgsvectorlayer.h" //QgsAttributeList
#include "qgsattributetablemodel.h"
#include "qgsattributetableidcolumnpair.h"

class QgsAttributeTableMemoryModel : public QgsAttributeTableModel
{
    Q_OBJECT;

  public:
    /**
     * Constructor
     * @param theLayer layer pointer
     */
    QgsAttributeTableMemoryModel( QgsVectorLayer *theLayer );

    /**
     * Remove rows
     */
    bool removeRows( int row, int count, const QModelIndex &parent = QModelIndex() );

  protected slots:
    /**
     * Launched when a feature has been deleted
     * @param fid feature id
     */
    virtual void featureDeleted( QgsFeatureId fid );
    /**
     * Launched when a feature has been deleted
     * @param fid feature id
     */
    virtual void featureAdded( QgsFeatureId fid );
    /**
     * Launched when layer has been deleted
     */
    virtual void layerDeleted();

  private slots:
    /**
     * Launched when attribute value has been changed
     * @param fid feature id
     * @param idx attribute index
     * @param value new value
     */
    virtual void attributeValueChanged( QgsFeatureId fid, int idx, const QVariant &value );

  private:
    /**
     * load feature fid into mFeat
     * @param fid feature id
     * @return feature exists
     */
    virtual bool featureAtId( QgsFeatureId fid );

    /**
     * Loads the layer into the model
     */
    virtual void loadLayer();
};

#endif //QGSATTRIBUTETABLEMEMORYMODEL_H
