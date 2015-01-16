/***************************************************************************
      qgsgrassrasterprovider.h  -  QGIS Data provider for
                           GRASS rasters
                             -------------------
    begin                : 16 Jan, 2010
    copyright            : (C) 2010 by Radim Blazek
    email                : radim dot blazek at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSGRASSRASTERPROVIDER_H
#define QGSGRASSRASTERPROVIDER_H

extern "C"
{
#include <grass/version.h>
#include <grass/gis.h>
#if GRASS_VERSION_MAJOR > 6
#include <grass/raster.h>
#endif
}

#include "qgscoordinatereferencesystem.h"
#include "qgsrasterdataprovider.h"
#include "qgsrectangle.h"
#include "qgscolorrampshader.h"

#include <QString>
#include <QStringList>
#include <QDomElement>
#include <QMap>
#include <QVector>
#include <QTemporaryFile>
#include <QProcess>
#include <QHash>

class QgsCoordinateTransform;

/**
  \brief Read raster value for given coordinates

  Executes qgis.g.info and keeps it open comunicating through pipe. Restarts the command if raster was updated.
*/

class QgsGrassRasterValue
{
  public:
    QgsGrassRasterValue();
    ~QgsGrassRasterValue();
    void start( QString gisdbase, QString location, QString mapset, QString map );
    // returns raster value, NaN for no data
    // ok is set to true if ok or false on error
    double value( double x, double y, bool *ok );
  private:
    QString mGisdbase;      // map gisdabase
    QString mLocation;      // map location name (not path!)
    QString mMapset;        // map mapset
    QString mMapName;       // map name
    QTemporaryFile mGisrcFile;
    QProcess *mProcess;
};
/**

  \brief Data provider for GRASS raster layers.

  This provider implements the
  interface defined in the QgsDataProvider class to provide access to spatial
  data residing in a OGC Web Map Service.

*/
class QgsGrassRasterProvider : public QgsRasterDataProvider
{
    Q_OBJECT

  public:
    /**
    * Constructor for the provider.
    *
    * \param   uri   HTTP URL of the Web Server.  If needed a proxy will be used
    *                otherwise we contact the host directly.
    *
    */
    QgsGrassRasterProvider( QString const & uri = 0 );

    //! Destructor
    ~QgsGrassRasterProvider();

    QgsRasterInterface * clone() const override;

    /** \brief   Renders the layer as an image
     */
    QImage* draw( QgsRectangle  const & viewExtent, int pixelWidth, int pixelHeight ) override;

    /** return a provider name

    Essentially just returns the provider key.  Should be used to build file
    dialogs so that providers can be shown with their supported types. Thus
    if more than one provider supports a given format, the user is able to
    select a specific provider to open that file.

    @note

    Instead of being pure virtual, might be better to generalize this
    behavior and presume that none of the sub-classes are going to do
    anything strange with regards to their name or description?

    */
    QString name() const override;


    /** return description

    Return a terse string describing what the provider is.

    @note

    Instead of being pure virtual, might be better to generalize this
    behavior and presume that none of the sub-classes are going to do
    anything strange with regards to their name or description?

    */
    QString description() const override;

    /*! Get the QgsCoordinateReferenceSystem for this layer
     * @note Must be reimplemented by each provider.
     * If the provider isn't capable of returning
     * its projection an empty srs will be return, ti will return 0
     */
    virtual QgsCoordinateReferenceSystem crs() override;

    /** Return the extent for this data layer
    */
    virtual QgsRectangle extent() override;

    /**Returns true if layer is valid
    */
    bool isValid() override;

    QgsRasterIdentifyResult identify( const QgsPoint & thePoint, QgsRaster::IdentifyFormat theFormat, const QgsRectangle &theExtent = QgsRectangle(), int theWidth = 0, int theHeight = 0 ) override;

    /**
     * \brief   Returns the caption error text for the last error in this provider
     *
     * If an operation returns 0 (e.g. draw()), this function
     * returns the text of the error associated with the failure.
     * Interactive users of this provider can then, for example,
     * call a QMessageBox to display the contents.
     */
    QString lastErrorTitle() override;

    /**
     * \brief   Returns the verbose error text for the last error in this provider
     *
     * If an operation returns 0 (e.g. draw()), this function
     * returns the text of the error associated with the failure.
     * Interactive users of this provider can then, for example,
     * call a QMessageBox to display the contents.
     */

    QString lastError() override;

    /** Returns a bitmask containing the supported capabilities
        Note, some capabilities may change depending on which
        sublayers are visible on this provider, so it may
        be prudent to check this value per intended operation.
      */
    int capabilities() const override;

    QGis::DataType dataType( int bandNo ) const override;
    QGis::DataType srcDataType( int bandNo ) const override;

    int bandCount() const override;

    int colorInterpretation( int bandNo ) const override;

    int xBlockSize() const override;
    int yBlockSize() const override;

    int xSize() const override;
    int ySize() const override;

    void readBlock( int bandNo, int xBlock, int yBlock, void *data ) override;
    void readBlock( int bandNo, QgsRectangle  const & viewExtent, int width, int height, void *data ) override;

    QgsRasterBandStats bandStatistics( int theBandNo,
                                       int theStats = QgsRasterBandStats::All,
                                       const QgsRectangle & theExtent = QgsRectangle(),
                                       int theSampleSize = 0 ) override;

    QList<QgsColorRampShader::ColorRampItem> colorTable( int bandNo )const override;

    // void buildSupportedRasterFileFilter( QString & theFileFiltersString );

    /**
     * Get metadata in a format suitable for feeding directly
     * into a subset of the GUI raster properties "Metadata" tab.
     */
    QString metadata() override;

    virtual QDateTime dataTimestamp() const override;
  private:

    /**
    * Flag indicating if the layer data source is a valid layer
    */
    bool mValid;

    QString mGisdbase;      // map gisdabase
    QString mLocation;      // map location name (not path!)
    QString mMapset;        // map mapset
    QString mMapName;       // map name

    RASTER_MAP_TYPE mGrassDataType; // CELL_TYPE, DCELL_TYPE, FCELL_TYPE

    int mCols;
    int mRows;
    int mYBlockSize;

    QHash<QString, QString> mInfo;

    QgsCoordinateReferenceSystem mCrs;

    QgsGrassRasterValue mRasterValue;

    double mNoDataValue;
};

#endif
