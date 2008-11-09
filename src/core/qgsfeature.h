/***************************************************************************
                      qgsfeature.h - Spatial Feature Class
                     --------------------------------------
Date                 : 09-Sep-2003
Copyright            : (C) 2003 by Gary E.Sherman
email                : sherman at mrcc.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#ifndef QGSFEATURE_H
#define QGSFEATURE_H

#include <QMap>
#include <QString>
#include <QVariant>
#include <QList>

class QgsGeometry;
class QgsRectangle;
class QgsFeature;

// key = field index, value = field value
typedef QMap<int, QVariant> QgsAttributeMap;

// key = feature id, value = changed attributes
typedef QMap<int, QgsAttributeMap> QgsChangedAttributesMap;

// key = feature id, value = changed geometry
typedef QMap<int, QgsGeometry> QgsGeometryMap;

// key = field index, value = field name
typedef QMap<int, QString> QgsFieldNameMap;

typedef QList<QgsFeature> QgsFeatureList;

/** \ingroup core
 * The feature class encapsulates a single feature including its id,
 * geometry and a list of field/values attributes.
 *
 * @author Gary E.Sherman
 */
class CORE_EXPORT QgsFeature
{
  public:
    //! Constructor
    QgsFeature( int id = 0, QString typeName = "" );

    /** copy ctor needed due to internal pointer */
    QgsFeature( QgsFeature const & rhs );

    /** assignment operator needed due to internal pointer */
    QgsFeature & operator=( QgsFeature const & rhs );

    //! Destructor
    ~QgsFeature();


    /**
     * Get the feature id for this feature
     * @return Feature id
     */
    int id() const;

    /**
     * Set the feature id for this feature
     * @param id Feature id
     */
    void setFeatureId( int id );


    /** returns the feature's type name
     */
    QString typeName() const;


    /** sets the feature's type name
     */
    void setTypeName( QString typeName );

    /**
     * Get the attributes for this feature.
     * @return A std::map containing the field name/value mapping
     */
    const QgsAttributeMap& attributeMap() const;

    /**Sets all the attributes in one go*/
    void setAttributeMap( const QgsAttributeMap& attributeMap );

    /**
     * Add an attribute to the map
     */
    void addAttribute( int field, QVariant attr );

    /**Deletes an attribute and its value*/
    void deleteAttribute( int field );

    /**Changes an existing attribute value
       @param field index of the field
       @param attr attribute name and value to be set */
    void changeAttribute( int field, QVariant attr );

    /**
     * Return the validity of this feature. This is normally set by
     * the provider to indicate some problem that makes the feature
     * invalid or to indicate a null feature.
     */
    bool isValid() const;

    /**
     * Set the validity of the feature.
     */
    void setValid( bool validity );

    /**
     * Return the dirty state of this feature.
     * Dirty is set if (e.g.) the feature's geometry has been modified in-memory.
     */
    bool isDirty() const;

    /**
     * Reset the dirtiness of the feature.  (i.e. make clean)
     * You would normally do this after it's saved to permanent storage (e.g. disk, an ACID-compliant database)
     */
    void clean();

    /**
     * Get the geometry object associated with this feature
     */
    QgsGeometry *geometry();

    /**
     * Get the geometry object associated with this feature
     * The caller assumes responsibility for the QgsGeometry*'s destruction.
     */
    QgsGeometry *geometryAndOwnership();

    /** Set this feature's geometry from another QgsGeometry object (deep copy)
     */
    void setGeometry( const QgsGeometry& geom );

    /** Set this feature's geometry (takes geometry ownership)
     */
    void setGeometry( QgsGeometry* geom );

    /**
     * Set this feature's geometry from WKB
     *
     * This feature assumes responsibility for destroying geom.
     */
    void setGeometryAndOwnership( unsigned char * geom, size_t length );

  private:

    //! feature id
    int mFid;

    /** map of attributes accessed by field index */
    QgsAttributeMap mAttributes;

    /** pointer to geometry in binary WKB format

       This is usually set by a call to OGRGeometry::exportToWkb()
     */
    QgsGeometry *mGeometry;

    /** Indicator if the mGeometry is owned by this QgsFeature.
        If so, this QgsFeature takes responsibility for the mGeometry's destruction.
     */
    bool mOwnsGeometry;

    //! Flag to indicate if this feature is valid
    // TODO: still applies? [MD]
    bool mValid;

    //! Flag to indicate if this feature is dirty (e.g. geometry has been modified in-memory)
    // TODO: still applies? [MD]
    bool mDirty;

    /// feature type name
    QString mTypeName;


}; // class QgsFeature


#endif
