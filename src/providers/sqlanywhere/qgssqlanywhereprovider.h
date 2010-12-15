/***************************************************************************
    qgssqlanywhereprovider.h - QGIS data provider for SQL Anywhere DBMS
    ------------------------
    begin                : Dec 2010
    copyright            : (C) 2010 by iAnywhere Solutions, Inc.
    author               : David DeHaan
    email                : ddehaan at sybase dot com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSSQLANYWHEREPROVIDER_H
#define QGSSQLANYWHEREPROVIDER_H

#include "qgsdatasourceuri.h"
#include "qgsvectordataprovider.h"
#include "qgsrectangle.h"
#include <list>
#include <queue>
#include <fstream>
#include <set>
#include <QMutex>

class QgsFeature;
class QgsField;

#include "sqlanyconnection.h"
#include "sqlanystatement.h"

/**
  \class QgsSqlAnywhereProvider
  \brief Data provider for SQL Anywhere layers.

  This provider implements the interface defined in the QgsDataProvider
  class to provide access to spatial data residing in a SQL Anywhere database.
  */
class QgsSqlAnywhereProvider: public QgsVectorDataProvider
{
    Q_OBJECT

  public:
    /**
     * Constructor of the vector provider
     * @param uri  uniform resource locator (URI) for a dataset
     */
    QgsSqlAnywhereProvider( QString const &uri = "" );

    //! Destructor
    virtual ~ QgsSqlAnywhereProvider();

    /**
     * Returns the permanent storage type for this layer as a friendly name.
     */
    virtual QString storageType() const;

    /** Get the QgsCoordinateReferenceSystem for this layer */
    QgsCoordinateReferenceSystem crs() { return mCrs; }

    /** Gets the feature at the given feature ID
      *  @param featureId of feature to retrieve
      *  @param feature which will receive data from the provider
      *  @param fetchGeometry true if the feature geometry should be fetched
      *  @param fetchAttributes list of attributes which should be fetched
      */
    virtual bool featureAtId( int featureId,
                              QgsFeature & feature, bool fetchGeometry = true, QgsAttributeList fetchAttributes = QgsAttributeList() );

    /** Accessor for sql where clause used to limit dataset */
    virtual QString subsetString();

    /** mutator for sql where clause used to limit dataset size */
    virtual bool setSubsetString( QString theSQL );
    virtual bool supportsSubsetString() { return true; }

    /** Select features based on a bounding rectangle. Features can be retrieved with calls to nextFeature.
     *  @param fetchAttributes list of attributes which should be fetched
     *  @param rect spatial filter
     *  @param fetchGeometry true if the feature geometry should be fetched
     *  @param useIntersect true if an accurate intersection test should be used,
     *                     false if a test based on bounding box is sufficient
     */
    virtual void select( QgsAttributeList fetchAttributes = QgsAttributeList(),
                         QgsRectangle rect = QgsRectangle(), bool fetchGeometry = true, bool useIntersect = false );

    /**
     * Get the next feature resulting from a select operation.
     * @param feature feature which will receive data from the provider
     * @return true when there was a feature to fetch, false when end was hit
     */
    virtual bool nextFeature( QgsFeature & feature );

    /** Get the feature type. This corresponds to
     * WKBPoint,
     * WKBLineString,
     * WKBPolygon,
     * WKBMultiPoint,
     * WKBMultiLineString or
     * WKBMultiPolygon
     * as defined in qgis.h
     */
    QGis::WkbType geometryType() const { return mGeomType; }

    /**
     * return the number of layers for the current data source
     */
    size_t layerCount() const { return 1; }

    /**
     * Get the number of features in the layer
     */
    long featureCount() const { return mNumberFeatures; }

    /**
     * Return the extent for this data layer
    */
    virtual QgsRectangle extent();

    /**
     * Get the number of fields in the layer
     */
    uint fieldCount() const { return mAttributeFields.size(); }

    /**
      * Get the field information for the layer
      * @return vector of QgsField objects
      */
    const QgsFieldMap & fields() const { return mAttributeFields; }

    /**
     * Restart reading features from previous select operation
     */
    void rewind();

    /** Returns the minimum value of an attribute
     *  @param index the index of the attribute */
    QVariant minimumValue( int index ) { return minmaxValue( index, "MIN" ); }

    /** Returns the maximum value of an attribute
     *  @param index the index of the attribute */
    QVariant maximumValue( int index ) { return minmaxValue( index, "MIN" ); }

    /** Return the unique values of an attribute
     *  @param index the index of the attribute
     *  @param values reference to the list of unique values
     *  @param limit maximum number of values (added in 1.4) */
    virtual void uniqueValues( int index, QList < QVariant > &uniqueValues, int limit = -1 );

    /**Returns true if layer is valid
    */
    bool isValid() { return mValid; }

    /**Returns the default value for field specified by @c fieldId */
    QVariant defaultValue( int fieldId ) { return mAttributeDefaults[fieldId]; }

    /**Adds a list of features
      @return true in case of success and false in case of failure*/
    bool addFeatures( QgsFeatureList & flist );

    /**Deletes a list of features
      @param id list of feature ids
      @return true in case of success and false in case of failure*/
    bool deleteFeatures( const QgsFeatureIds & id );

    /**Adds new attributes
      @param name map with attribute name as key and type as value
      @return true in case of success and false in case of failure*/
    bool addAttributes( const QList<QgsField> &attributes );

    /**Deletes existing attributes
      @param ids of the attributes to delete
      @return true in case of success and false in case of failure*/
    bool deleteAttributes( const QgsAttributeIds & ids );

    /**Changes attribute values of existing features
      @param attr_map a map containing the new attributes. The integer is the feature id,
      the first QString is the attribute name and the second one is the new attribute value
      @return true in case of success and false in case of failure*/
    bool changeAttributeValues( const QgsChangedAttributesMap & attr_map );

    /**
       Changes geometries of existing features
       @param geometry_map   A std::map containing the feature IDs to change the geometries of.
                             the second map parameter being the new geometries themselves
       @return               true in case of success and false in case of failure
     */
    bool changeGeometryValues( QgsGeometryMap & geometry_map );

    /**Returns a bitmask containing the supported capabilities*/
    int capabilities() const { return mCapabilities; }

    /** return a provider name
    */
    QString name() const;

    /** return description
    * Return a terse string describing what the provider is.
    */
    QString description() const;


  private:
    /** Returns the minimum value of an attribute
     *  @param index the index of the attribute */
    QVariant minmaxValue( int index, const QString minmax );

    /**
     * Sets mNumberFeatures
     */
    void countFeatures();
    /**
     * loads fields to member mAttributeFields and mAttributeDefaults
     */
    bool loadFields();
    /**
     * retrieves attribute's default value from database schema
     */
    QString getDefaultValue( QString attrName );
    /**
     * Populates QgsVectorDataProvider::mNativeTypes
     */
    void setNativeTypes();
    /**
     * Sets mKeyColumn and mKeyConstrained
     */
    bool findKeyColumn();
    /**
     * Sets mIsTable, mTableId, mIsComputed, mGeomType, mSrid
     */
    bool checkLayerType();
    /**
     * sets mCrs, mSrsExtent
     */
    bool checkSrs();
    /**
     * Sets mCapabilities
     */
    bool checkPermissions();
    bool testDMLPermission( QString sql );
    bool testDeletePermission();
    bool testInsertPermission();
    bool testUpdateGeomPermission();
    bool testUpdateOtherPermission();
    bool testUpdateColumn( QString colname );
    bool testAlterTable();
    bool testMeasuredOr3D();

    /**
     * retrieves specified field from mAttributeFields
     */
    const QgsField field( int index ) const;

    /**
     * Ensures that a database connections is live,
     * connecting or reconnecting if necessary.
     *
     * Note that a reconnection automatically invalidates
     *   the cursors mStmt and mIdStmt.
     */
    bool ensureConnRO();
    bool ensureConnRW();

    /**
    * internal utility functions used to handle common database tasks
    */
    void    closeDb();
    void    closeCursors() { closeConnROCursors(); }
    void    closeConnROCursors();
    void    closeConnRO();
    void    closeConnRW();
    QString quotedIdentifier( QString id ) const;
    QString quotedValue( QString value ) const;
    QString getWhereClause() const { return mSubsetString.isEmpty() ? "1=1 " : "( " + mSubsetString + ") "; }
    bool    hasUniqueData( QString colName );
    QString makeSelectSql( QString whereClause ) const;
    bool    nextFeature( QgsFeature & feature, SqlAnyStatement *stmt );
    QString geomSampleSet();
    QString geomColIdent() const { return quotedIdentifier( mGeometryColumn ) + mGeometryProjStr; }

    /**
     * static internal utility functions
     */
    static QGis::WkbType lookupWkbType( QString type );
    static void showMessageBox( const QString& title, const QString& text );
    static void showMessageBox( const QString& title, const QStringList& text );
    static void reportError( const QString& title, sacapi_i32 code, const char *errbuf );
    static void reportError( const QString& title, SqlAnyStatement *stmt );
    static void reportError( const QString& title, sacapi_i32 code, QString msg );


  private:
    /**
     * information needed to establish connection
     */
    QString mConnectInfo;
    /**
     * map of field index number to field type
     */
    QgsFieldMap mAttributeFields;
    /**
     * map of field index number to field default value
     */
    QMap<int, QString> mAttributeDefaults;
    /**
       * Flag indicating if the layer data source is a SQL Anywhere layer
       */
    bool mValid;
    /**
     * Use estimated metadata when determining table counts,
     * geometry type, and extent.
     */
    bool mUseEstimatedMetadata;
    /**
     * Name of the schema
     */
    QString mSchemaName;
    /**
     * Name of the table with no schema
     */
    QString mTableName;
    /**
     * Internal oid of the table/view
     */
    unsigned int mTableId;
    /**
     * Flag indicating whether this is a table (as compared to a view)
     */
    bool mIsTable;
    /**
     * Name of the table in format "schema"."table"
     */
    QString mQuotedTableName;
    /**
     * Name of the key column used to access the table
     */
    QString mKeyColumn;
    /**
     * Flag indicating whether schema constraint enforces mKeyColumn is unique
     */
    bool mKeyConstrained;
    /**
     * Name of the geometry column in the table
     */
    QString mGeometryColumn;
    QString mGeometryProjStr;
    /**
     * Geometry type
     */
    QGis::WkbType mGeomType;
    /**
     * Flag indicating whether mGeometryColumn originates from a
     *   computed expression
     */
    bool mIsComputed;

    /**
     * Bitmap of capabilities supported for this layer.
     * See QgsVectorDataProvider::Capability
     */
    int mCapabilities;

    /**
     * String used to define a subset of the layer
     */
    QString mSubsetString;
    /**
       * Spatial reference id of the layer
       */
    int mSrid;
    /**
      * Definition of the reference system
     */
    QgsCoordinateReferenceSystem mCrs;
    /**
     * SRS boundaries (used to threshold rectangle queries)
     */
    QgsRectangle mSrsExtent;

    /**
     * Rectangle that contains the extent (bounding box) of the layer
     */
    QgsRectangle mLayerExtent;

    /**
     * Number of features in the layer
     */
    long mNumberFeatures;

    /**
     * Statement handle for fetching of features by bounding rectangle
     */
    SqlAnyStatement *mStmt;
    QgsAttributeList mStmtAttributesToFetch;
    bool  mStmtFetchGeom;
    QgsRectangle mStmtRect;
    bool  mStmtUseIntersect;

    /**
     * Statement handle for fetching of features by ID
     */
    SqlAnyStatement *mIdStmt;
    QgsAttributeList mIdStmtAttributesToFetch;
    bool  mIdStmtFetchGeom;

    /**
     * Read-only connection to SQL Anywhere database
     */
    SqlAnyConnection   *mConnRO;
    /**
     * Read-write connection to SQL Anywhere database
     */
    SqlAnyConnection   *mConnRW;

}; // class QgsSqlAnywhereProvider


#endif // QGSSQLANYWHEREPROVIDER_H
