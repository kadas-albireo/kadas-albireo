/***************************************************************************
  qgsdelimitedtextprovider.cpp -  Data provider for delimted text
  -------------------
          begin                : 2004-02-27
          copyright            : (C) 2004 by Gary E.Sherman
          email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsdelimitedtextprovider.h"


#include <QtGlobal>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QStringList>
#include <QMessageBox>
#include <QSettings>
#include <QRegExp>
#include <QUrl>


#include "qgsapplication.h"
#include "qgsdataprovider.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgslogger.h"
#include "qgsmessageoutput.h"
#include "qgsrectangle.h"
#include "qgscoordinatereferencesystem.h"
#include "qgis.h"

static const QString TEXT_PROVIDER_KEY = "delimitedtext";
static const QString TEXT_PROVIDER_DESCRIPTION = "Delimited text data provider";



QgsDelimitedTextProvider::QgsDelimitedTextProvider( QString uri )
    : QgsVectorDataProvider( uri ),
    mXFieldIndex( -1 ), mYFieldIndex( -1 ),
    mShowInvalidLines( true )
{
  // Get the file name and mDelimiter out of the uri
  mFileName = uri.left( uri.indexOf( "?" ) );
  // split the string up on & to get the individual parameters
  QStringList parameters = uri.mid( uri.indexOf( "?" ) ).split( "&", QString::SkipEmptyParts );

  QgsDebugMsg( "Parameter count after split on &" + QString::number( parameters.size() ) );

  // get the individual parameters and assign values
  QStringList temp = parameters.filter( "delimiter=" );
  mDelimiter = temp.size() ? temp[0].mid( temp[0].indexOf( "=" ) + 1 ) : "";
  temp = parameters.filter( "delimiterType=" );
  mDelimiterType = temp.size() ? temp[0].mid( temp[0].indexOf( "=" ) + 1 ) : "";
  temp = parameters.filter( "xField=" );
  QString xField = temp.size() ? temp[0].mid( temp[0].indexOf( "=" ) + 1 ) : "";
  temp = parameters.filter( "yField=" );
  QString yField = temp.size() ? temp[0].mid( temp[0].indexOf( "=" ) + 1 ) : "";
  // Decode the parts of the uri. Good if someone entered '=' as a delimiter, for instance.
  mFileName  = QUrl::fromPercentEncoding( mFileName.toUtf8() );
  mDelimiter = QUrl::fromPercentEncoding( mDelimiter.toUtf8() );
  mDelimiterType = QUrl::fromPercentEncoding( mDelimiterType.toUtf8() );
  xField    = QUrl::fromPercentEncoding( xField.toUtf8() );
  yField    = QUrl::fromPercentEncoding( yField.toUtf8() );

  QgsDebugMsg( "Data source uri is " + uri );
  QgsDebugMsg( "Delimited text file is: " + mFileName );
  QgsDebugMsg( "Delimiter is: " + mDelimiter );
  QgsDebugMsg( "Delimiter type is: " + mDelimiterType );
  QgsDebugMsg( "xField is: " + xField );
  QgsDebugMsg( "yField is: " + yField );

  // if delimiter contains some special characters, convert them
  if ( mDelimiterType == "regexp" )
    mDelimiterRegexp = QRegExp( mDelimiter );
  else
    mDelimiter.replace( "\\t", "\t" ); // replace "\t" with a real tabulator

  // Set the selection rectangle to null
  mSelectionRectangle = QgsRectangle();
  // assume the layer is invalid until proven otherwise
  mValid = false;
  if ( mFileName.isEmpty() || mDelimiter.isEmpty() || xField.isEmpty() || yField.isEmpty() )
  {
    // uri is invalid so the layer must be too...
    QString( "Data source is invalid" );
    return;
  }

  // check to see that the file exists and perform some sanity checks
  if ( !QFile::exists( mFileName ) )
  {
    QgsDebugMsg( "Data source " + dataSourceUri() + " doesn't exist" );
    return;
  }

  // Open the file and get number of rows, etc. We assume that the
  // file has a header row and process accordingly. Caller should make
  // sure the the delimited file is properly formed.
  mFile = new QFile( mFileName );
  if ( !mFile->open( QIODevice::ReadOnly ) )
  {
    QgsDebugMsg( "Data source " + dataSourceUri() + " could not be opened" );
    delete mFile;
    return;
  }

  // now we have the file opened and ready for parsing

  // set the initial extent
  mExtent = QgsRectangle();

  QMap<int, bool> couldBeInt;
  QMap<int, bool> couldBeDouble;

  mStream = new QTextStream( mFile );
  QString line;
  mNumberFeatures = 0;
  int lineNumber = 0;
  bool firstPoint = true;
  bool hasFields = false;
  while ( !mStream->atEnd() )
  {
    lineNumber++;
    line = mStream->readLine(); // line of text excluding '\n', default local 8 bit encoding.
    if ( !hasFields )
    {
      // Get the fields from the header row and store them in the
      // fields vector
      QgsDebugMsg( "Attempting to split the input line: " + line + " using delimiter " + mDelimiter );

      QStringList fieldList;
      if ( mDelimiterType == "regexp" )
        fieldList = line.split( mDelimiterRegexp );
      else
        fieldList = line.split( mDelimiter );
      QgsDebugMsg( "Split line into " + QString::number( fieldList.size() ) + " parts" );

      // We don't know anything about a text based field other
      // than its name. All fields are assumed to be text
      int fieldPos = 0;
      for ( QStringList::Iterator it = fieldList.begin();
            it != fieldList.end(); ++it )
      {
        QString field = *it;
        if ( field.length() > 0 )
        {
          // for now, let's set field type as text
          attributeFields[fieldPos] = QgsField( *it, QVariant::String, "Text" );

          // check to see if this field matches either the x or y field
          if ( xField == *it )
          {
            QgsDebugMsg( "Found x field: " + ( *it ) );
            mXFieldIndex = fieldPos;
          }
          else if ( yField == *it )
          {
            QgsDebugMsg( "Found y field: " + ( *it ) );
            mYFieldIndex = fieldPos;
          }

          QgsDebugMsg( "Adding field: " + ( *it ) );
          // assume that the field could be integer or double
          couldBeInt.insert( fieldPos, true );
          couldBeDouble.insert( fieldPos, true );
          fieldPos++;
        }
      }
      QgsDebugMsg( "Field count for the delimited text file is " + QString::number( attributeFields.size() ) );
      hasFields = true;
    }
    else if ( mXFieldIndex != -1 && mYFieldIndex != -1 )
    {
      mNumberFeatures++;

      // split the line on the delimiter
      QStringList parts;
      if ( mDelimiterType == "regexp" )
        parts = line.split( mDelimiterRegexp );
      else
        parts = line.split( mDelimiter );

      // Skip malformed lines silently. Report line number with nextFeature()
      if ( attributeFields.size() != parts.size() )
      {
        continue;
      }

      // Get the x and y values, first checking to make sure they
      // aren't null.
      QString sX = parts[mXFieldIndex];
      QString sY = parts[mYFieldIndex];

      bool xOk = true;
      bool yOk = true;
      double x = sX.toDouble( &xOk );
      double y = sY.toDouble( &yOk );

      if ( xOk && yOk )
      {
        if ( !firstPoint )
        {
          mExtent.combineExtentWith( x, y );
        }
        else
        { // Extent for the first point is just the first point
          mExtent.set( x, y, x, y );
          firstPoint = false;
        }
      }

      int i = 0;
      for ( QStringList::iterator it = parts.begin(); it != parts.end(); ++it, ++i )
      {
        // try to convert attribute values to integer and double
        if ( couldBeInt[i] )
        {
          it->toInt( &couldBeInt[i] );
        }
        if ( couldBeDouble[i] )
        {
          it->toDouble( &couldBeDouble[i] );
        }
      }
    }
  }

  // now it's time to decide the types for the fields
  for ( QgsFieldMap::iterator it = attributeFields.begin(); it != attributeFields.end(); ++it )
  {
    if ( couldBeInt[it.key()] )
    {
      it->setType( QVariant::Int );
      it->setTypeName( "integer" );
    }
    else if ( couldBeDouble[it.key()] )
    {
      it->setType( QVariant::Double );
      it->setTypeName( "double" );
    }
  }

  if ( mXFieldIndex != -1 && mYFieldIndex != -1 )
  {
    QgsDebugMsg( "Data store is valid" );
    QgsDebugMsg( "Number of features " + QString::number( mNumberFeatures ) );
    QgsDebugMsg( "Extents " + mExtent.toString() );

    mValid = true;
  }
  else
  {
    QgsDebugMsg( "Data store is invalid. Specified x,y fields do not match those in the database" );
  }
  QgsDebugMsg( "Done checking validity" );

}

QgsDelimitedTextProvider::~QgsDelimitedTextProvider()
{
  mFile->close();
  delete mFile;
  delete mStream;
}


QString QgsDelimitedTextProvider::storageType() const
{
  return "Delimited text file";
}


bool QgsDelimitedTextProvider::nextFeature( QgsFeature& feature )
{
  // before we do anything else, assume that there's something wrong with
  // the feature
  feature.setValid( false );
  while ( ! mStream->atEnd() )
  {
    double x = 0.0;
    double y = 0.0;
    QString line = mStream->readLine(); // Default local 8 bit encoding

    // lex the tokens from the current data line
    QStringList tokens;
    if ( mDelimiterType == "regexp" )
      tokens = line.split( mDelimiterRegexp );
    else
      tokens = line.split( mDelimiter );

    bool xOk = false;
    bool yOk = false;

    // Skip indexing malformed lines.
    if ( attributeFields.size() == tokens.size() )
    {
      x = tokens[mXFieldIndex].toDouble( &xOk );
      y = tokens[mYFieldIndex].toDouble( &yOk );
    }

    if ( !( xOk && yOk ) )
    {
      // Accumulate any lines that weren't ok, to report on them
      // later, and look at the next line in the file, but only if
      // we need to.
      QgsDebugMsg( "Malformed line : " + line );
      if ( mShowInvalidLines )
        mInvalidLines << line;

      continue;
    }

    // Give every valid line in the file an id, even if it's not
    // in the current extent or bounds.
    ++mFid;             // increment to next feature ID

    // skip the feature if it's out of current bounds
    if ( ! boundsCheck( x, y ) )
      continue;

    // at this point, one way or another, the current feature values
    // are valid
    feature.setValid( true );

    feature.setFeatureId( mFid );

    QByteArray  buffer;
    QDataStream s( &buffer, static_cast<QIODevice::OpenMode>( QIODevice::WriteOnly ) ); // open on buffers's data

    switch ( QgsApplication::endian() )
    {
      case QgsApplication::NDR :
        // we're on a little-endian platform, so tell the data
        // stream to use that
        s.setByteOrder( QDataStream::LittleEndian );
        s << ( quint8 )1; // 1 is for little-endian
        break;
      case QgsApplication::XDR :
        // don't change byte order since QDataStream is big endian by default
        s << ( quint8 )0; // 0 is for big-endian
        break;
      default :
        qDebug( "%s:%d unknown endian", __FILE__, __LINE__ );
        //delete [] geometry;
        return false;
    }

    s << ( quint32 )QGis::WKBPoint;
    s << x;
    s << y;

    unsigned char* geometry = new unsigned char[buffer.size()];
    memcpy( geometry, buffer.data(), buffer.size() );

    feature.setGeometryAndOwnership( geometry, sizeof( wkbPoint ) );

    for ( QgsAttributeList::const_iterator i = mAttributesToFetch.begin();
          i != mAttributesToFetch.end();
          ++i )
    {
      QVariant val;
      switch ( attributeFields[*i].type() )
      {
        case QVariant::Int:
          val = QVariant( tokens[*i].toInt() );
          break;
        case QVariant::Double:
          val = QVariant( tokens[*i].toDouble() );
          break;
        default:
          val = QVariant( tokens[*i] );
          break;
      }
      feature.addAttribute( *i, val );
    }

    // We have a good line, so return
    return true;

  } // ! textStream EOF

  // End of the file. If there are any lines that couldn't be
  // loaded, display them now.

  if ( mShowInvalidLines && !mInvalidLines.isEmpty() )
  {
    mShowInvalidLines = false;
    QgsMessageOutput* output = QgsMessageOutput::createMessageOutput();
    output->setTitle( tr( "Error" ) );
    output->setMessage( tr( "Note: the following lines were not loaded because Qgis was "
                            "unable to determine values for the x and y coordinates:\n" ),
                        QgsMessageOutput::MessageText );

    output->appendMessage( "Start of invalid lines." );
    for ( int i = 0; i < mInvalidLines.size(); ++i )
      output->appendMessage( mInvalidLines.at( i ) );
    output->appendMessage( "End of invalid lines." );

    output->showMessage();

    // We no longer need these lines.
    mInvalidLines.empty();
  }

  return false;

} // nextFeature


void QgsDelimitedTextProvider::select( QgsAttributeList fetchAttributes,
                                       QgsRectangle rect,
                                       bool fetchGeometry,
                                       bool useIntersect )
{
  mSelectionRectangle = rect;
  mAttributesToFetch = fetchAttributes;
  mFetchGeom = fetchGeometry;
  if ( rect.isEmpty() )
  {
    mSelectionRectangle = mExtent;
  }
  else
  {
    mSelectionRectangle = rect;
  }
  rewind();
}




// Return the extent of the layer
QgsRectangle QgsDelimitedTextProvider::extent()
{
  return mExtent;
}

/**
 * Return the feature type
 */
QGis::WkbType QgsDelimitedTextProvider::geometryType() const
{
  return QGis::WKBPoint;
}

/**
 * Return the feature type
 */
long QgsDelimitedTextProvider::featureCount() const
{
  return mNumberFeatures;
}

/**
 * Return the number of fields
 */
uint QgsDelimitedTextProvider::fieldCount() const
{
  return attributeFields.size();
}


const QgsFieldMap & QgsDelimitedTextProvider::fields() const
{
  return attributeFields;
}

void QgsDelimitedTextProvider::rewind()
{
  // Reset feature id to 0
  mFid = 0;
  // Skip ahead one line since first record is always assumed to be
  // the header record
  mStream->seek( 0 );
  mStream->readLine();
}

bool QgsDelimitedTextProvider::isValid()
{
  return mValid;
}

/**
 * Check to see if the point is within the selection rectangle
 */
bool QgsDelimitedTextProvider::boundsCheck( double x, double y )
{
  // no selection rectangle => always in the bounds
  if ( mSelectionRectangle.isEmpty() )
    return true;

  return ( x <= mSelectionRectangle.xMaximum() ) && ( x >= mSelectionRectangle.xMinimum() ) &&
         ( y <= mSelectionRectangle.yMaximum() ) && ( y >= mSelectionRectangle.yMinimum() );
}

int QgsDelimitedTextProvider::capabilities() const
{
  return 0;
}


QgsCoordinateReferenceSystem QgsDelimitedTextProvider::crs()
{
  // TODO: make provider projection-aware
  return QgsCoordinateReferenceSystem(); // return default CRS
}




QString  QgsDelimitedTextProvider::name() const
{
  return TEXT_PROVIDER_KEY;
} // ::name()



QString  QgsDelimitedTextProvider::description() const
{
  return TEXT_PROVIDER_DESCRIPTION;
} //  QgsDelimitedTextProvider::name()


/**
 * Class factory to return a pointer to a newly created
 * QgsDelimitedTextProvider object
 */
QGISEXTERN QgsDelimitedTextProvider *classFactory( const QString *uri )
{
  return new QgsDelimitedTextProvider( *uri );
}

/** Required key function (used to map the plugin to a data store type)
*/
QGISEXTERN QString providerKey()
{
  return TEXT_PROVIDER_KEY;
}

/**
 * Required description function
 */
QGISEXTERN QString description()
{
  return TEXT_PROVIDER_DESCRIPTION;
}

/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */
QGISEXTERN bool isProvider()
{
  return true;
}
