/***************************************************************************
    qgsrasterprojector.h - Raster projector
     --------------------------------------
    Date                 : Jan 16, 2011
    Copyright            : (C) 2005 by Radim Blazek
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

/* This code takes ideas from WarpBuilder in Geotools.
 * Thank you to Ing. Andrea Aime, Ing. Simone Giannecchini and GeoSolutions S.A.S.
 * See : http://geo-solutions.blogspot.com/2011/01/developers-corner-improving.html
 */

#ifndef QGSRASTERPROJECTOR_H
#define QGSRASTERPROJECTOR_H

#include <QVector>
#include <QList>

#include "qgsrectangle.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"
#include "qgsrasterinterface.h"

#include <cmath>

class QgsPoint;

class CORE_EXPORT QgsRasterProjector : public QgsRasterInterface
{
  public:
    /** \brief QgsRasterProjector implements approximate projection support for
     * it calculates grid of points in source CRS for target CRS + extent
     * which are used to calculate affine transformation matrices.
     * */
    QgsRasterProjector(
      QgsCoordinateReferenceSystem theSrcCRS,
      QgsCoordinateReferenceSystem theDestCRS,
      QgsRectangle theDestExtent,
      int theDestRows, int theDestCols,
      double theMaxSrcXRes, double theMaxSrcYRes,
      QgsRectangle theExtent
    );
    QgsRasterProjector(
      QgsCoordinateReferenceSystem theSrcCRS,
      QgsCoordinateReferenceSystem theDestCRS,
      double theMaxSrcXRes, double theMaxSrcYRes,
      QgsRectangle theExtent
    );
    QgsRasterProjector();

    /** \brief The destructor */
    ~QgsRasterProjector();

    QgsRasterInterface * clone() const;

    int bandCount() const;

    QgsRasterInterface::DataType dataType( int bandNo ) const;

    /** \brief set source and destination CRS */
    void setCRS( QgsCoordinateReferenceSystem theSrcCRS, QgsCoordinateReferenceSystem theDestCRS );

    /** \brief Get source CRS */
    QgsCoordinateReferenceSystem srcCrs() const  { return mSrcCRS; }

    /** \brief Get destination CRS */
    QgsCoordinateReferenceSystem destCrs() const  { return mDestCRS; }

    /** \brief set maximum source resolution */
    void setMaxSrcRes( double theMaxSrcXRes, double theMaxSrcYRes )
    {
      mMaxSrcXRes = theMaxSrcXRes; mMaxSrcYRes = theMaxSrcYRes;
    }

    /** \brief get destination point for _current_ destination position */
    void destPointOnCPMatrix( int theRow, int theCol, double *theX, double *theY );

    /** \brief Get matrix upper left row/col indexes for destination row/col */
    int matrixRow( int theDestRow );
    int matrixCol( int theDestCol );

    /** \brief get destination point for _current_ matrix position */
    QgsPoint srcPoint( int theRow, int theCol );

    /** \brief Get precise source row and column indexes for current source extent and resolution */
    inline void preciseSrcRowCol( int theDestRow, int theDestCol, int *theSrcRow, int *theSrcCol );

    /** \brief Get approximate source row and column indexes for current source extent and resolution */
    inline void approximateSrcRowCol( int theDestRow, int theDestCol, int *theSrcRow, int *theSrcCol );

    /** \brief Get source row and column indexes for current source extent and resolution */
    void srcRowCol( int theDestRow, int theDestCol, int *theSrcRow, int *theSrcCol );

    /** \brief Calculate matrix */
    void calc();

    /** \brief insert rows to matrix */
    void insertRows();

    /** \brief insert columns to matrix */
    void insertCols();

    /* calculate single control point in current matrix */
    void calcCP( int theRow, int theCol );

    /** \brief calculate matrix row */
    bool calcRow( int theRow );

    /** \brief calculate matrix column */
    bool calcCol( int theCol );

    /** \brief calculate source extent */
    void calcSrcExtent();

    /** \brief calculate minimum source width and height */
    void calcSrcRowsCols();

    /** \brief check error along columns
      * returns true if within threshold */
    bool checkCols();

    /** \brief check error along rows
      * returns true if within threshold */
    bool checkRows();

    /** Calculate array of src helper points */
    void calcHelper( int theMatrixRow, QgsPoint *thePoints );

    /** Calc / switch helper */
    void nextHelper();

    /** get source extent */
    QgsRectangle srcExtent() { return mSrcExtent; }

    /** get/set source width/height */
    int srcRows() { return mSrcRows; }
    int srcCols() { return mSrcCols; }
    void setSrcRows( int theRows ) { mSrcRows = theRows; mSrcXRes = mSrcExtent.height() / mSrcRows; }
    void setSrcCols( int theCols ) { mSrcCols = theCols; mSrcYRes = mSrcExtent.width() / mSrcCols; }

    /** get mCPMatrix as string */
    QString cpToString();

    int dstRows() const { return mDestRows; }
    int dstCols() const { return mDestCols; }

    void * readBlock( int bandNo, QgsRectangle  const & extent, int width, int height );

  private:
    /** Source CRS */
    QgsCoordinateReferenceSystem mSrcCRS;

    /** Destination CRS */
    QgsCoordinateReferenceSystem mDestCRS;

    /** Reverse coordinate transform (from destination to source) */
    QgsCoordinateTransform mCoordinateTransform;

    /** Destination extent */
    QgsRectangle mDestExtent;

    /** Source extent */
    QgsRectangle mSrcExtent;

    /** Source raster extent */
    QgsRectangle mExtent;

    /** Number of destination rows */
    int mDestRows;

    /** Number of destination columns */
    int mDestCols;

    /** Destination x resolution */
    double mDestXRes;

    /** Destination y resolution */
    double mDestYRes;

    /** Number of source rows */
    int mSrcRows;

    /** Number of source columns */
    int mSrcCols;

    /** Source x resolution */
    double mSrcXRes;

    /** Source y resolution */
    double mSrcYRes;

    /** number of destination rows per matrix row */
    double mDestRowsPerMatrixRow;

    /** number of destination cols per matrix col */
    double mDestColsPerMatrixCol;

    /** Grid of source control points */
    QList< QList<QgsPoint> > mCPMatrix;

    /** Array of source points for each destination column on top of current CPMatrix grid row */
    /* Warning: using QList is slow on access */
    QgsPoint *pHelperTop;

    /** Array of source points for each destination column on bottom of current CPMatrix grid row */
    /* Warning: using QList is slow on access */
    QgsPoint *pHelperBottom;

    /** Current mHelperTop matrix row */
    int mHelperTopRow;

    /** Number of mCPMatrix columns */
    int mCPCols;
    /** Number of mCPMatrix rows */
    int mCPRows;

    /** Maximum tolerance in destination units */
    double mSqrTolerance;

    /** Maximum source resolution */
    double mMaxSrcXRes;
    double mMaxSrcYRes;

    /** Use approximation */
    bool mApproximate;
};

#endif

