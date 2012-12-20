/***************************************************************************
    qgsrasterdataprovider.h - DataProvider Interface for raster layers
     --------------------------------------
    Date                 : Mar 11, 2005
    Copyright            : (C) 2005 by Brendan Morley
    email                : morb at ozemail dot com dot au
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* Thank you to Marco Hugentobler for the original vector DataProvider */

#ifndef QGSRASTERDATAPROVIDER_H
#define QGSRASTERDATAPROVIDER_H

#include <QDateTime>

#include "qgslogger.h"
#include "qgsrectangle.h"
#include "qgsdataprovider.h"
#include "qgserror.h"
#include "qgsrasterinterface.h"
#include "qgscolorrampshader.h"
#include "qgsrasterpyramid.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsrasterbandstats.h"
#include "qgsrasterhistogram.h"

#include "cpl_conv.h"
#include <cmath>

class QImage;
class QgsPoint;
class QByteArray;
#include <QVariant>

#define TINY_VALUE  std::numeric_limits<double>::epsilon() * 20
#define RASTER_HISTOGRAM_BINS 256

/** \ingroup core
 * Base class for raster data providers.
 *
 *  \note  This class has been copied and pasted from
 *         QgsVectorDataProvider, and does not yet make
 *         sense for Raster layers.
 */
class CORE_EXPORT QgsRasterDataProvider : public QgsDataProvider, public QgsRasterInterface
{

    Q_OBJECT

  public:

    // This is modified copy of GDALColorInterp
    enum ColorInterpretation
    {
      UndefinedColorInterpretation = 0,
      /*! Greyscale */                                      GrayIndex = 1,
      /*! Paletted (see associated color table) */          PaletteIndex = 2, // indexed color table
      /*! Red band of RGBA image */                         RedBand = 3,
      /*! Green band of RGBA image */                       GreenBand = 4,
      /*! Blue band of RGBA image */                        BlueBand = 5,
      /*! Alpha (0=transparent, 255=opaque) */              AlphaBand = 6,
      /*! Hue band of HLS image */                          HueBand = 7,
      /*! Saturation band of HLS image */                   SaturationBand = 8,
      /*! Lightness band of HLS image */                    LightnessBand = 9,
      /*! Cyan band of CMYK image */                        CyanBand = 10,
      /*! Magenta band of CMYK image */                     MagentaBand = 11,
      /*! Yellow band of CMYK image */                      YellowBand = 12,
      /*! Black band of CMLY image */                       BlackBand = 13,
      /*! Y Luminance */                                    YCbCr_YBand = 14,
      /*! Cb Chroma */                                      YCbCr_CbBand = 15,
      /*! Cr Chroma */                                      YCbCr_CrBand = 16,
      /*! Continuous palette, QGIS addition, GRASS */       ContinuousPalette = 17,
      /*! Max current value */                              ColorInterpretationMax = 17
    };

    enum IdentifyFormat
    {
      IdentifyFormatValue = 0,
      IdentifyFormatText  = 1,
      IdentifyFormatHtml  = 1 << 1,
      // In future it should be possible to get from GetFeatureInfo (WMS) in GML
      // vector features. It is possible to use a user type with QVariant if
      // a class is declared with Q_DECLARE_METATYPE
      IdentifyFormatFeature = 1 << 2
    };

    // Progress types
    enum RasterProgressType
    {
      ProgressHistogram = 0,
      ProgressPyramids  = 1,
      ProgressStatistics = 2
    };

    enum RasterBuildPyramids
    {
      PyramidsFlagNo = 0,
      PyramidsFlagYes = 1,
      CopyExisting = 2
    };

    enum RasterPyramidsFormat
    {
      PyramidsGTiff = 0,
      PyramidsInternal = 1,
      PyramidsErdas = 2
    };

    QgsRasterDataProvider();

    QgsRasterDataProvider( const QString & uri );

    virtual ~QgsRasterDataProvider() {};

    virtual QgsRasterInterface * clone() const = 0;

    /* It makes no sense to set input on provider */
    bool setInput( QgsRasterInterface* input ) { Q_UNUSED( input ); return false; }

    // TODO: Document this better.
    /** \brief   Renders the layer as an image
     */
    virtual QImage* draw( const QgsRectangle & viewExtent, int pixelWidth, int pixelHeight ) = 0;

    // TODO: Get the supported formats by this provider

    // TODO: Get the file masks supported by this provider, suitable for feeding into the file open dialog box

    /**
     * Get the extent of the data source.
     * @return QgsRectangle containing the extent of the layer
     */
    virtual QgsRectangle extent() = 0;

    /** Returns data type for the band specified by number */
    virtual QGis::DataType dataType( int bandNo ) const = 0;

    /** Returns source data type for the band specified by number,
     *  source data type may be shorter than dataType
     */
    virtual QGis::DataType srcDataType( int bandNo ) const = 0;

    /** Returns data type for the band specified by number */
    virtual int colorInterpretation( int theBandNo ) const
    {
      Q_UNUSED( theBandNo );
      return QgsRasterDataProvider::UndefinedColorInterpretation;
    }

    QString colorName( int colorInterpretation ) const
    {
      // Modified copy from GDAL
      switch ( colorInterpretation )
      {
        case UndefinedColorInterpretation:
          return "Undefined";

        case GrayIndex:
          return "Gray";

        case PaletteIndex:
          return "Palette";

        case RedBand:
          return "Red";

        case GreenBand:
          return "Green";

        case BlueBand:
          return "Blue";

        case AlphaBand:
          return "Alpha";

        case HueBand:
          return "Hue";

        case SaturationBand:
          return "Saturation";

        case LightnessBand:
          return "Lightness";

        case CyanBand:
          return "Cyan";

        case MagentaBand:
          return "Magenta";

        case YellowBand:
          return "Yellow";

        case BlackBand:
          return "Black";

        case YCbCr_YBand:
          return "YCbCr_Y";

        case YCbCr_CbBand:
          return "YCbCr_Cb";

        case YCbCr_CrBand:
          return "YCbCr_Cr";

        default:
          return "Unknown";
      }
    }
    /** Reload data (data could change) */
    virtual bool reload() { return true; }

    virtual QString colorInterpretationName( int theBandNo ) const
    {
      return colorName( colorInterpretation( theBandNo ) );
    }

    /** Get block size */
    //virtual int xBlockSize() const { return 0; }
    //virtual int yBlockSize() const { return 0; }

    /** Get raster size */
    //virtual int xSize() const { return 0; }
    //virtual int ySize() const { return 0; }

    // TODO: remove or make protected all readBlock working with void*

    /** read block of data  */
    // TODO clarify what happens on the last block (the part outside raster)
    // @note not available in python bindings
    virtual void readBlock( int bandNo, int xBlock, int yBlock, void *data )
    { Q_UNUSED( bandNo ); Q_UNUSED( xBlock ); Q_UNUSED( yBlock ); Q_UNUSED( data ); }

    /** read block of data using give extent and size */
    // @note not available in python bindings
    virtual void readBlock( int bandNo, QgsRectangle  const & viewExtent, int width, int height, void *data )
    { Q_UNUSED( bandNo ); Q_UNUSED( viewExtent ); Q_UNUSED( width ); Q_UNUSED( height ); Q_UNUSED( data ); }

    /** Read block of data using given extent and size. */
    virtual QgsRasterBlock *block( int theBandNo, const QgsRectangle &theExtent, int theWidth, int theHeight );

    /* Read a value from a data block at a given index. */
    //virtual double readValue( void *data, int type, int index );

    /* Return true if source band has no data value */
    virtual bool srcHasNoDataValue( int bandNo ) const { return mSrcHasNoDataValue.value( bandNo -1 ); }

    /** \brief Get source nodata value usage */
    virtual bool useSrcNoDataValue( int bandNo ) const { return mUseSrcNoDataValue.value( bandNo -1 ); }

    /** \brief Set source nodata value usage */
    virtual void setUseSrcNoDataValue( int bandNo, bool use );

    /** value representing null data */
    //virtual double noDataValue() const { return 0; }

    /** Value representing currentno data.
     *  WARNING: this value returned by this method is not constant. It may change
     *  for example if user disable use of source no data value. */
    virtual double noDataValue( int bandNo ) const;

    /** Value representing no data value. */
    virtual double srcNoDataValue( int bandNo ) const { return mSrcNoDataValue.value( bandNo -1 ); }

    virtual void setUserNoDataValue( int bandNo, QList<QgsRasterBlock::Range> noData );

    /** Get list of user no data value ranges */
    virtual  QList<QgsRasterBlock::Range> userNoDataValue( int bandNo ) const { return mUserNoDataValue.value( bandNo -1 ); }

    virtual double minimumValue( int bandNo ) const { Q_UNUSED( bandNo ); return 0; }
    virtual double maximumValue( int bandNo ) const { Q_UNUSED( bandNo ); return 0; }

    virtual QList<QgsColorRampShader::ColorRampItem> colorTable( int bandNo ) const
    { Q_UNUSED( bandNo ); return QList<QgsColorRampShader::ColorRampItem>(); }

    // Defined in parent
    /** \brief Returns the sublayers of this layer - Useful for providers that manage their own layers, such as WMS */
    virtual QStringList subLayers() const
    {
      return QStringList();
    }

    /** \brief Create pyramid overviews */
    virtual QString buildPyramids( const QList<QgsRasterPyramid>  & thePyramidList,
                                   const QString &  theResamplingMethod = "NEAREST",
                                   RasterPyramidsFormat theFormat = PyramidsGTiff )
    {
      Q_UNUSED( thePyramidList ); Q_UNUSED( theResamplingMethod ); Q_UNUSED( theFormat );
      return "FAILED_NOT_SUPPORTED";
    };

    /** \brief Accessor for ths raster layers pyramid list.
     * @param overviewList used to construct the pyramid list (optional), when empty the list is defined by the provider.
     * A pyramid list defines the
     * POTENTIAL pyramids that can be in a raster. To know which of the pyramid layers
     * ACTUALLY exists you need to look at the existsFlag member in each struct stored in the
     * list.
     */
    virtual QList<QgsRasterPyramid> buildPyramidList( QList<int> overviewList = QList<int>() )
    { Q_UNUSED( overviewList ); return QList<QgsRasterPyramid>(); };

    /** \brief Returns true if raster has at least one populated histogram. */
    bool hasPyramids();

    /**
     * Get metadata in a format suitable for feeding directly
     * into a subset of the GUI raster properties "Metadata" tab.
     */
    virtual QString metadata() = 0;

    /** \brief Identify raster value(s) found on the point position. The context
     *         parameters theExtent, theWidth and theHeigh are important to identify
     *         on the same zoom level as a displayed map and to do effective
     *         caching (WCS). If context params are not specified the highest
     *         resolution is used. capabilities() may be used to test if format
     *         is supported by provider. Values are set to 'no data' or empty string
     *         if point is outside data source extent.
     *
     * \note  The arbitraryness of the returned document is enforced by WMS standards
     *        up to at least v1.3.0
     * @param thePoint coordinates in data source CRS
     * @param theFormat result format
     * @param theExtent context extent
     * @param theWidth context width
     * @param theHeight context height
     * @return map of values for all bands, keys are band numbers (from 1), empty
     *         if failed
     */
    virtual QMap<int, QVariant> identify( const QgsPoint & thePoint, IdentifyFormat theFormat, const QgsRectangle &theExtent = QgsRectangle(), int theWidth = 0, int theHeight = 0 );


    QMap<QString, QString> identify( const QgsPoint & thePoint, const QgsRectangle &theExtent = QgsRectangle(), int theWidth = 0, int theHeight = 0 );

    /**
     * \brief   Returns the caption error text for the last error in this provider
     *
     * If an operation returns 0 (e.g. draw()), this function
     * returns the text of the error associated with the failure.
     * Interactive users of this provider can then, for example,
     * call a QMessageBox to display the contents.
     *
     */
    virtual QString lastErrorTitle() = 0;

    /**
     * \brief   Returns the verbose error text for the last error in this provider
     *
     * If an operation returns 0 (e.g. draw()), this function
     * returns the text of the error associated with the failure.
     * Interactive users of this provider can then, for example,
     * call a QMessageBox to display the contents.
     *
     */
    virtual QString lastError() = 0;

    /**
     * \brief   Returns the format of the error text for the last error in this provider
     *
     * \note added in 1.6
     */
    virtual QString lastErrorFormat();

    /**Returns the dpi of the output device.
      @note: this method was added in version 1.2*/
    int dpi() const {return mDpi;}

    /**Sets the output device resolution.
      @note: this method was added in version 1.2*/
    void setDpi( int dpi ) {mDpi = dpi;}

    static QStringList cStringList2Q_( char ** stringList );

    static QString makeTableCell( const QString & value );
    static QString makeTableCells( const QStringList & values );

    /** Time stamp of data source in the moment when data/metadata were loaded by provider */
    virtual QDateTime timestamp() const { return mTimestamp; }

    /** Current time stamp of data source */
    virtual QDateTime dataTimestamp() const { return QDateTime(); }

    /**Writes into the provider datasource*/
    // TODO: add data type (may be defferent from band type)
    virtual bool write( void* data, int band, int width, int height, int xOffset, int yOffset )
    {
      Q_UNUSED( data );
      Q_UNUSED( band );
      Q_UNUSED( width );
      Q_UNUSED( height );
      Q_UNUSED( xOffset );
      Q_UNUSED( yOffset );
      return false;
    }

    /** Creates a new dataset with mDataSourceURI
        @return true in case of success*/
#if 0
    virtual bool create( const QString& format, int nBands,
                         QGis::DataType type,
                         int width, int height, double* geoTransform,
                         const QgsCoordinateReferenceSystem& crs,
                         QStringList createOptions = QStringList() /*e.v. color table*/ )
    {
      Q_UNUSED( format );
      Q_UNUSED( nBands );
      Q_UNUSED( type );
      Q_UNUSED( width );
      Q_UNUSED( height );
      Q_UNUSED( geoTransform );
      Q_UNUSED( crs );
      Q_UNUSED( createOptions );
      return false;
    }
#endif

    static QgsRasterDataProvider* create( const QString &providerKey,
                                          const QString &uri,
                                          const QString& format, int nBands,
                                          QGis::DataType type,
                                          int width, int height, double* geoTransform,
                                          const QgsCoordinateReferenceSystem& crs,
                                          QStringList createOptions = QStringList() );


    /** Set no data value on created dataset
     *  @param bandNo band number
     *  @param noDataValue no data value
     */
    virtual bool setNoDataValue( int bandNo, double noDataValue ) { Q_UNUSED( bandNo ); Q_UNUSED( noDataValue ); return false; }

    /**Returns the formats supported by create()*/
    virtual QStringList createFormats() const { return QStringList(); }

    /** Remove dataset*/
    virtual bool remove() { return false; }

    static QStringList pyramidResamplingMethods( QString providerKey )
    {
      return providerKey == "gdal" ?
             QStringList() << tr( "Average" ) << tr( "Nearest Neighbour" ) << tr( "Gauss" ) <<
             tr( "Cubic" ) << tr( "Mode" ) << tr( "None" ) : QStringList();
    }

    /** Validates creation options for a specific dataset and destination format - used by GDAL provider only.
     * See also validateCreationOptionsFormat() in gdal provider for validating options based on format only. */
    virtual QString validateCreationOptions( const QStringList& createOptions, QString format )
    { Q_UNUSED( createOptions ); Q_UNUSED( format ); return QString(); }

  signals:
    /** Emit a signal to notify of the progress event.
      * Emited theProgress is in percents (0.0-100.0) */
    void progress( int theType, double theProgress, QString theMessage );
    void progressUpdate( int theProgress );

  protected:
    /**Dots per inch. Extended WMS (e.g. QGIS mapserver) support DPI dependent output and therefore
    are suited for printing. A value of -1 means it has not been set
    @note: this member has been added in version 1.2*/
    int mDpi;

    /** \brief Cell value representing original source no data. e.g. -9999, indexed from 0  */
    QList<double> mSrcNoDataValue;

    /** \brief Source nodata value exist */
    QList<bool> mSrcHasNoDataValue;

    /** \brief Use source nodata value. User can disable usage of source nodata
     *  value as nodata. It may happen that a value is wrongly given by GDAL
     *  as nodata (e.g. 0) and it has to be treated as regular value. */
    QList<bool> mUseSrcNoDataValue;

    /** \brief Internal value representing nodata. Indexed from 0.
     *  This values is used to represent nodata if no source nodata is available
     *  or if the source nodata use was disabled.
     *  It would be also possible to use wider type only if nodata is really necessary
     *  in following interfaces, but that could make difficult to subclass
     *  QgsRasterInterface.
     */
    QList<double> mInternalNoDataValue;

    /** \brief Flag indicating if the nodatavalue is valid*/
    //bool mValidNoDataValue;

    /** \brief List of lists of user defined additional no data values
     *  for each band, indexed from 0 */
    QList< QList<QgsRasterBlock::Range> > mUserNoDataValue;

    QgsRectangle mExtent;

};
#endif
