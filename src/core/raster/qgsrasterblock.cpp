/***************************************************************************
    qgsrasterblock.cpp - Class representing a block of raster data
     --------------------------------------
    Date                 : Oct 9, 2012
    Copyright            : (C) 2012 by Radim Blazek
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

#include <limits>

#include <QByteArray>
#include <QColor>

#include "qgslogger.h"
#include "qgsrasterblock.h"

// See #9101 before any change of NODATA_COLOR!
const QRgb QgsRasterBlock::mNoDataColor = qRgba( 0, 0, 0, 0 );

QgsRasterBlock::QgsRasterBlock()
    : mValid( true )
    , mDataType( QGis::UnknownDataType )
    , mTypeSize( 0 )
    , mWidth( 0 )
    , mHeight( 0 )
    , mHasNoDataValue( false )
    , mNoDataValue( std::numeric_limits<double>::quiet_NaN() )
    , mData( 0 )
    , mImage( 0 )
    , mNoDataBitmap( 0 )
    , mNoDataBitmapWidth( 0 )
    , mNoDataBitmapSize( 0 )
{
}

QgsRasterBlock::QgsRasterBlock( QGis::DataType theDataType, int theWidth, int theHeight )
    : mValid( true )
    , mDataType( theDataType )
    , mTypeSize( 0 )
    , mWidth( theWidth )
    , mHeight( theHeight )
    , mHasNoDataValue( false )
    , mNoDataValue( std::numeric_limits<double>::quiet_NaN() )
    , mData( 0 )
    , mImage( 0 )
    , mNoDataBitmap( 0 )
    , mNoDataBitmapWidth( 0 )
    , mNoDataBitmapSize( 0 )
{
  reset( mDataType, mWidth, mHeight );
}

QgsRasterBlock::QgsRasterBlock( QGis::DataType theDataType, int theWidth, int theHeight, double theNoDataValue )
    : mValid( true )
    , mDataType( theDataType )
    , mTypeSize( 0 )
    , mWidth( theWidth )
    , mHeight( theHeight )
    , mHasNoDataValue( true )
    , mNoDataValue( theNoDataValue )
    , mData( 0 )
    , mImage( 0 )
    , mNoDataBitmap( 0 )
    , mNoDataBitmapWidth( 0 )
    , mNoDataBitmapSize( 0 )
{
  reset( mDataType, mWidth, mHeight, mNoDataValue );
}

QgsRasterBlock::~QgsRasterBlock()
{
  QgsDebugMsg( QString( "mData = %1" ).arg(( ulong )mData ) );
  qgsFree( mData );
  delete mImage;
  qgsFree( mNoDataBitmap );
}

bool QgsRasterBlock::reset( QGis::DataType theDataType, int theWidth, int theHeight )
{
  QgsDebugMsg( QString( "theWidth= %1 theHeight = %2 theDataType = %3" ).arg( theWidth ).arg( theHeight ).arg( theDataType ) );
  if ( !reset( theDataType, theWidth, theHeight, std::numeric_limits<double>::quiet_NaN() ) )
  {
    return false;
  }
  mHasNoDataValue = false;
  // the mNoDataBitmap is created only if necessary (usually, it is not) in setIsNoData()
  return true;
}

bool QgsRasterBlock::reset( QGis::DataType theDataType, int theWidth, int theHeight, double theNoDataValue )
{
  QgsDebugMsg( QString( "theWidth= %1 theHeight = %2 theDataType = %3 theNoDataValue = %4" ).arg( theWidth ).arg( theHeight ).arg( theDataType ).arg( theNoDataValue ) );

  qgsFree( mData );
  mData = 0;
  delete mImage;
  mImage = 0;
  qgsFree( mNoDataBitmap );
  mNoDataBitmap = 0;
  mDataType = QGis::UnknownDataType;
  mTypeSize = 0;
  mWidth = 0;
  mHeight = 0;
  mHasNoDataValue = false;
  mNoDataValue = std::numeric_limits<double>::quiet_NaN();
  mValid = false;

  if ( typeIsNumeric( theDataType ) )
  {
    QgsDebugMsg( "Numeric type" );
    qgssize tSize = typeSize( theDataType );
    QgsDebugMsg( QString( "allocate %1 bytes" ).arg( tSize * theWidth * theHeight ) );
    mData = qgsMalloc( tSize * theWidth * theHeight );
    if ( mData == 0 )
    {
      QgsDebugMsg( QString( "Couldn't allocate data memory of %1 bytes" ).arg( tSize * theWidth * theHeight ) );
      return false;
    }
  }
  else if ( typeIsColor( theDataType ) )
  {
    QgsDebugMsg( "Color type" );
    QImage::Format format = imageFormat( theDataType );
    mImage = new QImage( theWidth, theHeight, format );
  }
  else
  {
    QgsDebugMsg( "Wrong data type" );
    return false;
  }

  mValid = true;
  mDataType = theDataType;
  mTypeSize = QgsRasterBlock::typeSize( mDataType );
  mWidth = theWidth;
  mHeight = theHeight;
  mHasNoDataValue = true;
  mNoDataValue = theNoDataValue;
  QgsDebugMsg( QString( "mWidth= %1 mHeight = %2 mDataType = %3 mData = %4 mImage = %5" ).arg( mWidth ).arg( mHeight ).arg( mDataType ).arg(( ulong )mData ).arg(( ulong )mImage ) );
  return true;
}

QImage::Format QgsRasterBlock::imageFormat( QGis::DataType theDataType )
{
  if ( theDataType == QGis::ARGB32 )
  {
    return QImage::Format_ARGB32;
  }
  else if ( theDataType == QGis::ARGB32_Premultiplied )
  {
    return QImage::Format_ARGB32_Premultiplied;
  }
  return QImage::Format_Invalid;
}

QGis::DataType QgsRasterBlock::dataType( QImage::Format theFormat )
{
  if ( theFormat == QImage::Format_ARGB32 )
  {
    return QGis::ARGB32;
  }
  else if ( theFormat == QImage::Format_ARGB32_Premultiplied )
  {
    return QGis::ARGB32_Premultiplied;
  }
  return QGis::UnknownDataType;
}

bool QgsRasterBlock::isEmpty() const
{
  QgsDebugMsg( QString( "mWidth= %1 mHeight = %2 mDataType = %3 mData = %4 mImage = %5" ).arg( mWidth ).arg( mHeight ).arg( mDataType ).arg(( ulong )mData ).arg(( ulong )mImage ) );
  if ( mWidth == 0 || mHeight == 0 ||
       ( typeIsNumeric( mDataType ) && mData == 0 ) ||
       ( typeIsColor( mDataType ) && mImage == 0 ) )
  {
    return true;
  }
  return false;
}

bool QgsRasterBlock::typeIsNumeric( QGis::DataType dataType )
{
  switch ( dataType )
  {
    case QGis::Byte:
    case QGis::UInt16:
    case QGis::Int16:
    case QGis::UInt32:
    case QGis::Int32:
    case QGis::Float32:
    case QGis::CInt16:
    case QGis::Float64:
    case QGis::CInt32:
    case QGis::CFloat32:
    case QGis::CFloat64:
      return true;

    case QGis::UnknownDataType:
    case QGis::ARGB32:
    case QGis::ARGB32_Premultiplied:
      return false;
  }
  return false;
}

bool QgsRasterBlock::typeIsColor( QGis::DataType dataType )
{
  switch ( dataType )
  {
    case QGis::ARGB32:
    case QGis::ARGB32_Premultiplied:
      return true;

    case QGis::UnknownDataType:
    case QGis::Byte:
    case QGis::UInt16:
    case QGis::Int16:
    case QGis::UInt32:
    case QGis::Int32:
    case QGis::Float32:
    case QGis::CInt16:
    case QGis::Float64:
    case QGis::CInt32:
    case QGis::CFloat32:
    case QGis::CFloat64:
      return false;
  }
  return false;
}

QGis::DataType QgsRasterBlock::typeWithNoDataValue( QGis::DataType dataType, double *noDataValue )
{
  QGis::DataType newDataType;

  switch ( dataType )
  {
    case QGis::Byte:
      *noDataValue = -32768.0;
      newDataType = QGis::Int16;
      break;
    case QGis::Int16:
      *noDataValue = -2147483648.0;
      newDataType = QGis::Int32;
      break;
    case QGis::UInt16:
      *noDataValue = -2147483648.0;
      newDataType = QGis::Int32;
      break;
    case QGis::UInt32:
    case QGis::Int32:
    case QGis::Float32:
    case QGis::Float64:
      *noDataValue = std::numeric_limits<double>::max() * -1.0;
      newDataType = QGis::Float64;
    default:
      QgsDebugMsg( QString( "Unknow data type %1" ).arg( dataType ) );
      return QGis::UnknownDataType;
      break;
  }
  QgsDebugMsg( QString( "newDataType = %1 noDataValue = %2" ).arg( newDataType ).arg( *noDataValue ) );
  return newDataType;
}

bool QgsRasterBlock::hasNoData() const
{
  return mHasNoDataValue || mNoDataBitmap != 0;
}

bool QgsRasterBlock::isNoDataValue( double value, double noDataValue )
{
  // TODO: optimize no data value test by memcmp()
  // More precise would be qIsNaN(value) && qIsNaN(noDataValue(bandNo)), but probably
  // not important and slower
  if ( qIsNaN( value ) ||
       qgsDoubleNear( value, noDataValue ) )
  {
    return true;
  }
  return false;
}

double QgsRasterBlock::value( int row, int column ) const
{
  return value(( qgssize )row*mWidth + column );
}

QRgb QgsRasterBlock::color( qgssize index ) const
{
  int row = floor(( double )index / mWidth );
  int column = index % mWidth;
  return color( row, column );
}

QRgb QgsRasterBlock::color( int row, int column ) const
{
  if ( !mImage ) return mNoDataColor;

  return mImage->pixel( column, row );
}

bool QgsRasterBlock::isNoData( qgssize index )
{
  if ( !mHasNoDataValue && !mNoDataBitmap ) return false;
  if ( index >= ( qgssize )mWidth*mHeight )
  {
    QgsDebugMsg( QString( "Index %1 out of range (%2 x %3)" ).arg( index ).arg( mWidth ).arg( mHeight ) );
    return true; // we consider no data if outside
  }
  if ( mHasNoDataValue )
  {
    double value = readValue( mData, mDataType, index );
    return isNoDataValue( value );
  }
  // use no data bitmap
  if ( mNoDataBitmap == 0 )
  {
    // no data are not defined
    return false;
  }
  // TODO: optimize
  int row = ( int ) index / mWidth;
  int column = index % mWidth;
  qgssize byte = ( qgssize )row * mNoDataBitmapWidth + column / 8;
  int bit = column % 8;
  int mask = 0x80 >> bit;
  //int x = mNoDataBitmap[byte] & mask;
  //QgsDebugMsg ( QString("byte = %1 bit = %2 mask = %3 nodata = %4 is nodata = %5").arg(byte).arg(bit).arg(mask, 0, 2 ).arg( x, 0, 2 ).arg( (bool)(x) ) );
  return mNoDataBitmap[byte] & mask;
}

bool QgsRasterBlock::isNoData( int row, int column )
{
  return isNoData(( qgssize )row*mWidth + column );
}

bool QgsRasterBlock::setValue( qgssize index, double value )
{
  if ( !mData )
  {
    QgsDebugMsg( "Data block not allocated" );
    return false;
  }
  if ( index >= ( qgssize )mWidth*mHeight )
  {
    QgsDebugMsg( QString( "Index %1 out of range (%2 x %3)" ).arg( index ).arg( mWidth ).arg( mHeight ) );
    return false;
  }
  writeValue( mData, mDataType, index, value );
  return true;
}

bool QgsRasterBlock::setValue( int row, int column, double value )
{
  return setValue(( qgssize )row*mWidth + column, value );
}

bool QgsRasterBlock::setColor( int row, int column, QRgb color )
{
  return setColor(( qgssize )row*mWidth + column, color );
}

bool QgsRasterBlock::setColor( qgssize index, QRgb color )
{
  if ( !mImage )
  {
    QgsDebugMsg( "Image not allocated" );
    return false;
  }

  if ( index >= ( qgssize )mImage->width()* mImage->height() )
  {
    QgsDebugMsg( QString( "index %1 out of range" ).arg( index ) );
    return false;
  }

  // setPixel() is slow, see Qt doc -> use direct access
  QRgb* bits = ( QRgb* )mImage->bits();
  bits[index] = color;
  return true;
}

bool QgsRasterBlock::setIsNoData( int row, int column )
{
  return setIsNoData(( qgssize )row*mWidth + column );
}

bool QgsRasterBlock::setIsNoData( qgssize index )
{
  if ( mHasNoDataValue )
  {
    return setValue( index, mNoDataValue );
  }
  else
  {
    if ( mNoDataBitmap == 0 )
    {
      if ( !createNoDataBitmap() )
      {
        return false;
      }
    }
    // TODO: optimize
    int row = ( int ) index / mWidth;
    int column = index % mWidth;
    qgssize byte = ( qgssize )row * mNoDataBitmapWidth + column / 8;
    int bit = column % 8;
    int nodata = 0x80 >> bit;
    //QgsDebugMsg ( QString("set byte = %1 bit = %2 no data by %3").arg(byte).arg(bit).arg(nodata, 0,2 ) );
    mNoDataBitmap[byte] = mNoDataBitmap[byte] | nodata;
    return true;
  }
}

bool QgsRasterBlock::setIsNoData()
{
  QgsDebugMsg( "Entered" );
  if ( typeIsNumeric( mDataType ) )
  {
    if ( mHasNoDataValue )
    {
      if ( !mData )
      {
        QgsDebugMsg( "Data block not allocated" );
        return false;
      }

      QgsDebugMsg( "set mData to mNoDataValue" );
      int dataTypeSize = typeSize( mDataType );
      QByteArray noDataByteArray = valueBytes( mDataType, mNoDataValue );

      char *nodata = noDataByteArray.data();
      for ( qgssize i = 0; i < ( qgssize )mWidth*mHeight; i++ )
      {
        memcpy(( char* )mData + i*dataTypeSize, nodata, dataTypeSize );
      }
    }
    else
    {
      // use bitmap
      if ( mNoDataBitmap == 0 )
      {
        if ( !createNoDataBitmap() )
        {
          return false;
        }
      }
      QgsDebugMsg( "set mNoDataBitmap to 1" );
      memset( mNoDataBitmap, 0xff, mNoDataBitmapSize );
    }
    return true;
  }
  else
  {
    // image
    if ( !mImage )
    {
      QgsDebugMsg( "Image not allocated" );
      return false;
    }
    QgsDebugMsg( "Fill image" );
    mImage->fill( mNoDataColor );
    return true;
  }
}

bool QgsRasterBlock::setIsNoDataExcept( const QRect & theExceptRect )
{
  int top = theExceptRect.top();
  int bottom = theExceptRect.bottom();
  int left = theExceptRect.left();
  int right = theExceptRect.right();
  top = qMin( qMax( top, 0 ), mHeight - 1 );
  left = qMin( qMax( left, 0 ), mWidth - 1 );
  bottom = qMax( 0, qMin( bottom, mHeight - 1 ) );
  right = qMax( 0, qMin( right, mWidth - 1 ) );

  QgsDebugMsg( "Entered" );
  if ( typeIsNumeric( mDataType ) )
  {
    if ( mHasNoDataValue )
    {
      if ( !mData )
      {
        QgsDebugMsg( "Data block not allocated" );
        return false;
      }

      QgsDebugMsg( "set mData to mNoDataValue" );
      int dataTypeSize = typeSize( mDataType );
      QByteArray noDataByteArray = valueBytes( mDataType, mNoDataValue );

      char *nodata = noDataByteArray.data();
      char *nodataRow = new char[mWidth*dataTypeSize]; // full row of no data
      for ( int c = 0; c < mWidth; c++ )
      {
        memcpy( nodataRow + c*dataTypeSize, nodata, dataTypeSize );
      }

      // top and bottom
      for ( int r = 0; r < mHeight; r++ )
      {
        if ( r >= top && r <= bottom ) continue; // middle
        qgssize i = ( qgssize )r * mWidth;
        memcpy(( char* )mData + i*dataTypeSize, nodataRow, dataTypeSize*mWidth );
      }
      // middle
      for ( int r = top; r <= bottom; r++ )
      {
        qgssize i = ( qgssize )r * mWidth;
        // middle left
        memcpy(( char* )mData + i*dataTypeSize, nodataRow, dataTypeSize*left );
        // middle right
        i += right + 1;
        int w = mWidth - right - 1;
        memcpy(( char* )mData + i*dataTypeSize, nodataRow, dataTypeSize*w );
      }
      delete [] nodataRow;
    }
    else
    {
      // use bitmap
      if ( mNoDataBitmap == 0 )
      {
        if ( !createNoDataBitmap() )
        {
          return false;
        }
      }
      QgsDebugMsg( "set mNoDataBitmap to 1" );

      char *nodataRow = new char[mNoDataBitmapWidth]; // full row of no data
      // TODO: we can simply set all bytes to 11111111 (~0) I think
      memset( nodataRow, 0, mNoDataBitmapWidth );
      for ( int c = 0; c < mWidth; c ++ )
      {
        int byte = c / 8;
        int bit = c % 8;
        char nodata = 0x80 >> bit;
        memset( nodataRow + byte, nodataRow[byte] | nodata, 1 );
      }

      // top and bottom
      for ( int r = 0; r < mHeight; r++ )
      {
        if ( r >= top && r <= bottom ) continue; // middle
        qgssize i = ( qgssize )r * mNoDataBitmapWidth;
        memcpy( mNoDataBitmap + i, nodataRow, mNoDataBitmapWidth );
      }
      // middle
      memset( nodataRow, 0, mNoDataBitmapWidth );
      for ( int c = 0; c < mWidth; c ++ )
      {
        if ( c >= left && c <= right ) continue; // middle
        int byte = c / 8;
        int bit = c % 8;
        char nodata = 0x80 >> bit;
        memset( nodataRow + byte, nodataRow[byte] | nodata, 1 );
      }
      for ( int r = top; r <= bottom; r++ )
      {
        qgssize i = ( qgssize )r * mNoDataBitmapWidth;
        memcpy( mNoDataBitmap + i, nodataRow, mNoDataBitmapWidth );
      }
      delete [] nodataRow;
    }
    return true;
  }
  else
  {
    // image
    if ( !mImage )
    {
      QgsDebugMsg( "Image not allocated" );
      return false;
    }

    if ( mImage->width() != mWidth ||  mImage->height() != mHeight )
    {
      QgsDebugMsg( "Image and block size differ" );
      return false;
    }

    QgsDebugMsg( QString( "Fill image depth = %1" ).arg( mImage->depth() ) );

    // TODO: support different depths
    if ( mImage->depth() != 32 )
    {
      QgsDebugMsg( "Unsupported image depth" );
      return false;
    }

    QRgb nodataRgba = mNoDataColor;
    QRgb *nodataRow = new QRgb[mWidth]; // full row of no data
    int rgbSize = sizeof( QRgb );
    for ( int c = 0; c < mWidth; c ++ )
    {
      nodataRow[c] = nodataRgba;
    }

    // top and bottom
    for ( int r = 0; r < mHeight; r++ )
    {
      if ( r >= top && r <= bottom ) continue; // middle
      qgssize i = ( qgssize )r * mWidth;
      memcpy(( void * )( mImage->bits() + rgbSize*i ), nodataRow, rgbSize*mWidth );
    }
    // middle
    for ( int r = top; r <= bottom; r++ )
    {
      qgssize i = ( qgssize )r * mWidth;
      // middle left
      if ( left > 0 )
      {
        memcpy(( void * )( mImage->bits() + rgbSize*i ), nodataRow, rgbSize*( left - 1 ) );
      }
      // middle right
      i += right + 1;
      int w = mWidth - right - 1;
      memcpy(( void * )( mImage->bits() + rgbSize*i ), nodataRow, rgbSize*w );
    }
    delete [] nodataRow;
    return true;
  }
}

char * QgsRasterBlock::bits( qgssize index )
{
  // Not testing type to avoid too much overhead because this method is called per pixel
  if ( index >= ( qgssize )mWidth*mHeight )
  {
    QgsDebugMsg( QString( "Index %1 out of range (%2 x %3)" ).arg( index ).arg( mWidth ).arg( mHeight ) );
    return 0;
  }
  if ( mData )
  {
    return ( char* )mData + index * mTypeSize;
  }
  if ( mImage && mImage->bits() )
  {
    return ( char* )( mImage->bits() + index * 4 );
  }

  return 0;
}

char * QgsRasterBlock::bits( int row, int column )
{
  return bits(( qgssize )row*mWidth + column );
}

char * QgsRasterBlock::bits()
{
  if ( mData )
  {
    return ( char* )mData;
  }
  if ( mImage && mImage->bits() )
  {
    return ( char* )( mImage->bits() );
  }

  return 0;
}

bool QgsRasterBlock::convert( QGis::DataType destDataType )
{
  if ( isEmpty() ) return false;
  if ( destDataType == mDataType ) return true;

  if ( typeIsNumeric( mDataType ) && typeIsNumeric( destDataType ) )
  {
    void *data = convert( mData, mDataType, destDataType, mWidth * mHeight );

    if ( data == 0 )
    {
      QgsDebugMsg( "Cannot convert raster block" );
      return false;
    }
    qgsFree( mData );
    mData = data;
    mDataType = destDataType;
    mTypeSize = typeSize( mDataType );
  }
  else if ( typeIsColor( mDataType ) && typeIsColor( destDataType ) )
  {
    QImage::Format format = imageFormat( destDataType );
    QImage image = mImage->convertToFormat( format );
    *mImage = image;
    mDataType = destDataType;
    mTypeSize = typeSize( mDataType );
  }
  else
  {
    return false;
  }

  return true;
}

void QgsRasterBlock::applyScaleOffset( double scale, double offset )
{
  if ( isEmpty() ) return;
  if ( !typeIsNumeric( mDataType ) ) return;
  if ( scale == 1.0 && offset == 0.0 ) return;

  qgssize size = ( qgssize ) mWidth * mHeight;
  for ( qgssize i = 0; i < size; ++i )
  {
    if ( !isNoData( i ) ) setValue( i, value( i ) * scale + offset );
  }
  return;
}

void QgsRasterBlock::applyNoDataValues( const QgsRasterRangeList & rangeList )
{
  if ( rangeList.isEmpty() )
  {
    return;
  }

  qgssize size = mWidth * mHeight;
  for ( qgssize i = 0; i < size; ++i )
  {
    double val = value( i );
    if ( QgsRasterRange::contains( val, rangeList ) )
    {
      //setValue( i, mNoDataValue );
      setIsNoData( i );
    }
  }
}

QImage QgsRasterBlock::image() const
{
  if ( mImage )
  {
    return QImage( *mImage );
  }
  return QImage();
}

bool QgsRasterBlock::setImage( const QImage * image )
{
  qgsFree( mData );
  mData = 0;
  delete mImage;
  mImage = 0;
  mImage = new QImage( *image );
  mWidth = mImage->width();
  mHeight = mImage->height();
  mDataType = dataType( mImage->format() );
  mTypeSize = QgsRasterBlock::typeSize( mDataType );
  mNoDataValue = std::numeric_limits<double>::quiet_NaN();
  return true;
}

QString QgsRasterBlock::printValue( double value )
{
  /*
   *  IEEE 754 double has 15-17 significant digits. It specifies:
   *
   * "If a decimal string with at most 15 significant decimal is converted to
   *  IEEE 754 double precision and then converted back to the same number of
   *  significant decimal, then the final string should match the original;
   *  and if an IEEE 754 double precision is converted to a decimal string with at
   *  least 17 significant decimal and then converted back to double, then the final
   *  number must match the original."
   *
   * If printing only 15 digits, some precision could be lost. Printing 17 digits may
   * add some confusing digits.
   *
   * Default 'g' precision on linux is 6 digits, not all significant digits like
   * some sprintf manuals say.
   *
   * We need to ensure that the number printed and used in QLineEdit or XML will
   * give the same number when parsed.
   *
   * Is there a better solution?
   */

  QString s;

  for ( int i = 15; i <= 17; i++ )
  {
    s.setNum( value, 'g', i );
    if ( s.toDouble() == value )
    {
      return s;
    }
  }
  // Should not happen
  QgsDebugMsg( "Cannot correctly parse printed value" );
  return s;
}

void * QgsRasterBlock::convert( void *srcData, QGis::DataType srcDataType, QGis::DataType destDataType, qgssize size )
{
  int destDataTypeSize = typeSize( destDataType );
  void *destData = qgsMalloc( destDataTypeSize * size );
  for ( qgssize i = 0; i < size; i++ )
  {
    double value = readValue( srcData, srcDataType, i );
    writeValue( destData, destDataType, i, value );
    //double newValue = readValue( destData, destDataType, i );
    //QgsDebugMsg( QString("convert %1 type %2 to %3: %4 -> %5").arg(i).arg(srcDataType).arg(destDataType).arg( value ).arg( newValue ) );
  }
  return destData;
}

QByteArray QgsRasterBlock::valueBytes( QGis::DataType theDataType, double theValue )
{
  qgssize size = QgsRasterBlock::typeSize( theDataType );
  QByteArray ba;
  ba.resize(( int )size );
  char * data = ba.data();
  quint8 uc;
  quint16 us;
  qint16 s;
  quint32 ui;
  qint32 i;
  float f;
  double d;
  switch ( theDataType )
  {
    case QGis::Byte:
      uc = ( quint8 )theValue;
      memcpy( data, &uc, size );
      break;
    case QGis::UInt16:
      us = ( quint16 )theValue;
      memcpy( data, &us, size );
      break;
    case QGis::Int16:
      s = ( qint16 )theValue;
      memcpy( data, &s, size );
      break;
    case QGis::UInt32:
      ui = ( quint32 )theValue;
      memcpy( data, &ui, size );
      break;
    case QGis::Int32:
      i = ( qint32 )theValue;
      memcpy( data, &i, size );
      break;
    case QGis::Float32:
      f = ( float )theValue;
      memcpy( data, &f, size );
      break;
    case QGis::Float64:
      d = ( double )theValue;
      memcpy( data, &d, size );
      break;
    default:
      QgsDebugMsg( "Data type is not supported" );
  }
  return ba;
}

bool QgsRasterBlock::createNoDataBitmap()
{
  mNoDataBitmapWidth = mWidth / 8 + 1;
  mNoDataBitmapSize = ( qgssize )mNoDataBitmapWidth * mHeight;
  QgsDebugMsg( QString( "allocate %1 bytes" ).arg( mNoDataBitmapSize ) );
  mNoDataBitmap = ( char* )qgsMalloc( mNoDataBitmapSize );
  if ( mNoDataBitmap == 0 )
  {
    QgsDebugMsg( QString( "Couldn't allocate no data memory of %1 bytes" ).arg( mNoDataBitmapSize ) );
    return false;
  }
  memset( mNoDataBitmap, 0, mNoDataBitmapSize );
  return true;
}

QRect QgsRasterBlock::subRect( const QgsRectangle & theExtent, int theWidth, int theHeight, const QgsRectangle &  theSubExtent )
{
  QgsDebugMsg( "theExtent = " + theExtent.toString() );
  QgsDebugMsg( "theSubExtent = " + theSubExtent.toString() );
  double xRes = theExtent.width() / theWidth;
  double yRes = theExtent.height() / theHeight;

  QgsDebugMsg( QString( "theWidth = %1 theHeight = %2 xRes = %3 yRes = %4" ).arg( theWidth ).arg( theHeight ).arg( xRes ).arg( yRes ) );

  int top = 0;
  int bottom = theHeight - 1;
  int left = 0;
  int right = theWidth - 1;

  if ( theSubExtent.yMaximum() < theExtent.yMaximum() )
  {
    top = qRound(( theExtent.yMaximum() - theSubExtent.yMaximum() ) / yRes );
  }
  if ( theSubExtent.yMinimum() > theExtent.yMinimum() )
  {
    bottom = qRound(( theExtent.yMaximum() - theSubExtent.yMinimum() ) / yRes ) - 1;
  }

  if ( theSubExtent.xMinimum() > theExtent.xMinimum() )
  {
    left = qRound(( theSubExtent.xMinimum() - theExtent.xMinimum() ) / xRes );
  }
  if ( theSubExtent.xMaximum() < theExtent.xMaximum() )
  {
    right = qRound(( theSubExtent.xMaximum() - theExtent.xMinimum() ) / xRes ) - 1;
  }
  QRect subRect = QRect( left, top, right - left + 1, bottom - top + 1 );
  QgsDebugMsg( QString( "subRect: %1 %2 %3 %4" ).arg( subRect.x() ).arg( subRect.y() ).arg( subRect.width() ).arg( subRect.height() ) );
  return subRect;
}
