/***************************************************************************
  QgsAttributeTableModel.h - Models for attribute table
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

#ifndef QGSATTRIBUTETABLEMODEL_H
#define QGSATTRIBUTETABLEMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>
#include <QHash>
#include <QQueue>

#include "qgsfeature.h" // QgsAttributeMap
#include "qgsvectorlayer.h" // QgsAttributeList
#include "qgsattributetableidcolumnpair.h"

class QgsMapCanvas;

class GUI_EXPORT QgsAttributeTableModel: public QAbstractTableModel
{
    Q_OBJECT

  public:
    /**
     * Constructor
     * @param canvas map canvas pointer
     * @param theLayer layer pointer
     * @param parent parent pointer
     */
    QgsAttributeTableModel( QgsMapCanvas *canvas, QgsVectorLayer *theLayer, QObject *parent = 0 );
    /**
     * Returns the number of rows
     * @param parent parent index
     */
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
    /**
     * Returns the number of columns
     * @param parent parent index
     */
    int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    /**
     * Returns header data
     * @param section required section
     * @param orientation horizontal or vertical orientation
     * @param role data role
     */
    QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
    /**
     * Returns data on the given index
     * @param index model index
     * @param role data role
     */
    virtual QVariant data( const QModelIndex &index, int role ) const;
    /**
     * Updates data on given index
     * @param index model index
     * @param value new data value
     * @param role data role
     */
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
    /**
     * Returns item flags for the index
     * @param index model index
     */
    Qt::ItemFlags flags( const QModelIndex &index ) const;

    /**
     * Reloads the model data between indices
     * @param index1 start index
     * @param index2 end index
     */
    void reload( const QModelIndex &index1, const QModelIndex &index2 );
    /**
     * Remove rows
     */
    bool removeRows( int row, int count, const QModelIndex &parent = QModelIndex() );
    /**
     * Resets the model
     */
    void resetModel();
    /**
     * Layout has been changed
     */
    void changeLayout();
    /**
     * Layout will be changed
     */
    void incomingChangeLayout();
    /**
     * Maps feature id to table row
     * @param id feature id
     */
    int idToRow( QgsFeatureId id ) const;
    /**
     * get field index from column
     */
    int fieldIdx( int col ) const;
    /**
     * get column from field index
     */
    int fieldCol( int idx ) const;
    /**
     * Maps row to feature id
     * @param row row number
     */
    QgsFeatureId rowToId( int row ) const;
    /**
     * Sorts the model
     * @param column column to sort by
     * @param order sorting order
     */
    virtual void sort( int column, Qt::SortOrder order = Qt::AscendingOrder );
    /**
     * Swaps two rows
     * @param a first row
     * @param b second row
     */
    void swapRows( QgsFeatureId a, QgsFeatureId b );

    /**
     * Returns layer pointer
     */
    QgsVectorLayer* layer() const { return mLayer; }

    /** Execute an action */
    void executeAction( int action, const QModelIndex &idx ) const;

    /** return feature attributes at given index */
    QgsFeature feature( const QModelIndex &idx ) const;

  signals:
    /**
     * Model has been changed
     */
    void modelChanged();

    void progress( int i, bool &cancel );
    void finished();

  public slots:
    void extentsChanged();

  private slots:
    /**
     * Launched when attribute has been added
     * @param idx attribute index
     */
    virtual void attributeAdded( int idx );
    /**
     * Launched when attribute has been deleted
     * @param idx attribute index
     */
    virtual void attributeDeleted( int idx );

  protected slots:
    /**
     * Launched when attribute value has been changed
     * @param fid feature id
     * @param idx attribute index
     * @param value new value
     */
    virtual void attributeValueChanged( QgsFeatureId fid, int idx, const QVariant &value );
    /**
     * Launched when a feature has been deleted
     * @param fid feature id
     */
    virtual void featureDeleted( QgsFeatureId fid );
    /**
     * Launched when a feature has been added
     * @param fid feature id
     * @param inOperation guard insertion with beginInsertRows() / endInsertRows()
     */
    virtual void featureAdded( QgsFeatureId fid, bool inOperation = true );

    /**
     * Launched when layer has been deleted
     */
    virtual void layerDeleted();

  protected:
    QgsMapCanvas *mCanvas;
    QgsVectorLayer *mLayer;
    int mFieldCount;

    mutable QgsFeature mFeat;
    mutable QHash<QgsFeatureId, QgsFeature> mFeatureMap;

    QgsAttributeList mAttributes;
    QMap< int, const QMap<QString, QVariant> * > mValueMaps;

    QList<QgsAttributeTableIdColumnPair> mSortList;
    QHash<QgsFeatureId, int> mIdRowMap;
    QHash<int, QgsFeatureId> mRowIdMap;

    //! useful when showing only features from a particular extent
    QgsRectangle mCurrentExtent;

    /**
     * Initializes id <-> row maps
     */
    void initIdMaps();

    /**
      * Gets mFieldCount, mAttributes and mValueMaps
      */
    virtual void loadAttributes();

  public:
    /**
     * Loads the layer into the model
     */
    virtual void loadLayer();

  private:
    mutable QQueue<QgsFeatureId> mFeatureQueue;

    /**
     * load feature fid into mFeat
     * @param fid feature id
     * @return feature exists
     */
    virtual bool featureAtId( QgsFeatureId fid ) const;
};


#endif
