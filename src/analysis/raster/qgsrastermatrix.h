/***************************************************************************
                          qgsrastermatrix.h
                          -----------------
    begin                : 2010-10-23
    copyright            : (C) 20010 by Marco Hugentobler
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

#ifndef QGSRASTERMATRIX_H
#define QGSRASTERMATRIX_H

class ANALYSIS_EXPORT QgsRasterMatrix
{
  public:

    enum TwoArgOperator
    {
      opPLUS,
      opMINUS,
      opMUL,
      opDIV,
      opPOW,
    };

    enum OneArgOperator
    {
      opSQRT,
      opSIN,
      opCOS,
      opTAN,
      opASIN,
      opACOS,
      opATAN
    };

    /**Takes ownership of data array*/
    QgsRasterMatrix();
    QgsRasterMatrix( int nCols, int nRows, float* data );
    QgsRasterMatrix( const QgsRasterMatrix& m );
    ~QgsRasterMatrix();

    /**Returns true if matrix is 1x1 (=scalar number)*/
    bool isNumber() const { return ( mColumns == 1 && mRows == 1 ); }
    double number() const { return mData[0]; }

    /**Returns data array (but not ownership)*/
    float* data() { return mData; }
    /**Returns data and ownership. Sets data and nrows, ncols of this matrix to 0*/
    float* takeData();

    void setData( int cols, int rows, float* data );

    int nColumns() const { return mColumns; }
    int nRows() const { return mRows; }

    QgsRasterMatrix& operator=( const QgsRasterMatrix& m );
    /**Adds another matrix to this one*/
    bool add( const QgsRasterMatrix& other );
    /**Subtracts another matrix from this one*/
    bool subtract( const QgsRasterMatrix& other );
    bool multiply( const QgsRasterMatrix& other );
    bool divide( const QgsRasterMatrix& other );
    bool power( const QgsRasterMatrix& other );

    bool squareRoot();
    bool sinus();
    bool asinus();
    bool cosinus();
    bool acosinus();
    bool tangens();
    bool atangens();

  private:
    int mColumns;
    int mRows;
    float* mData;

    /**+,-,*,/,^*/
    bool twoArgumentOperation( TwoArgOperator op, const QgsRasterMatrix& other );
    bool testPowerValidity( double base, double power );
};

#endif // QGSRASTERMATRIX_H
