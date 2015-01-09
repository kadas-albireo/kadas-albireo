/***************************************************************************
                              qgsexpression.cpp
                             -------------------
    begin                : August 2011
    copyright            : (C) 2011 Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsexpression.h"

#include <QtDebug>
#include <QDomDocument>
#include <QDate>
#include <QRegExp>
#include <QColor>
#include <QUuid>

#include <math.h>
#include <limits>

#include "qgsdistancearea.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsmaplayerregistry.h"
#include "qgsogcutils.h"
#include "qgsvectorlayer.h"
#include "qgssymbollayerv2utils.h"
#include "qgsvectorcolorrampv2.h"
#include "qgsstylev2.h"

// from parser
extern QgsExpression::Node* parseExpression( const QString& str, QString& parserErrorMsg );

QgsExpression::Interval::~Interval() {}

QgsExpression::Interval QgsExpression::Interval::invalidInterVal()
{
  QgsExpression::Interval inter = QgsExpression::Interval();
  inter.setValid( false );
  return inter;
}

QgsExpression::Interval QgsExpression::Interval::fromString( QString string )
{
  int seconds = 0;
  QRegExp rx( "([-+]?\\d?\\.?\\d+\\s+\\S+)", Qt::CaseInsensitive );
  QStringList list;
  int pos = 0;

  while (( pos = rx.indexIn( string, pos ) ) != -1 )
  {
    list << rx.cap( 1 );
    pos += rx.matchedLength();
  }

  QMap<int, QStringList> map;
  map.insert( 1, QStringList() << "second" << "seconds" << QObject::tr( "second|seconds", "list of words separated by | which reference years" ).split( "|" ) );
  map.insert( 0 + MINUTE, QStringList() << "minute" << "minutes" << QObject::tr( "minute|minutes", "list of words separated by | which reference minutes" ).split( "|" ) );
  map.insert( 0 + HOUR, QStringList() << "hour" << "hours" << QObject::tr( "hour|hours", "list of words separated by | which reference minutes hours" ).split( "|" ) );
  map.insert( 0 + DAY, QStringList() << "day" << "days" << QObject::tr( "day|days", "list of words separated by | which reference days" ).split( "|" ) );
  map.insert( 0 + WEEKS, QStringList() << "week" << "weeks" << QObject::tr( "week|weeks", "wordlist separated by | which reference weeks" ).split( "|" ) );
  map.insert( 0 + MONTHS, QStringList() << "month" << "months" << QObject::tr( "month|months", "list of words separated by | which reference months" ).split( "|" ) );
  map.insert( 0 + YEARS, QStringList() << "year" << "years" << QObject::tr( "year|years", "list of words separated by | which reference years" ).split( "|" ) );

  foreach ( QString match, list )
  {
    QStringList split = match.split( QRegExp( "\\s+" ) );
    bool ok;
    double value = split.at( 0 ).toDouble( &ok );
    if ( !ok )
    {
      continue;
    }

    bool matched = false;
    foreach ( int duration, map.keys() )
    {
      foreach ( QString name, map[duration] )
      {
        if ( match.contains( name, Qt::CaseInsensitive ) )
        {
          matched = true;
          break;
        }
      }

      if ( matched )
      {
        seconds += value * duration;
        break;
      }
    }
  }

  // If we can't parse the string at all then we just return invalid
  if ( seconds == 0 )
    return QgsExpression::Interval::invalidInterVal();

  return QgsExpression::Interval( seconds );
}

bool QgsExpression::Interval::operator==( const QgsExpression::Interval& other ) const
{
  return ( mSeconds == other.mSeconds );
}

///////////////////////////////////////////////
// three-value logic

enum TVL
{
  False,
  True,
  Unknown
};

static TVL AND[3][3] =
{
  // false  true    unknown
  { False, False,   False },   // false
  { False, True,    Unknown }, // true
  { False, Unknown, Unknown }  // unknown
};

static TVL OR[3][3] =
{
  { False,   True, Unknown },  // false
  { True,    True, True },     // true
  { Unknown, True, Unknown }   // unknown
};

static TVL NOT[3] = { True, False, Unknown };

static QVariant tvl2variant( TVL v )
{
  switch ( v )
  {
    case False: return 0;
    case True: return 1;
    case Unknown:
    default:
      return QVariant();
  }
}

#define TVL_True     QVariant(1)
#define TVL_False    QVariant(0)
#define TVL_Unknown  QVariant()

///////////////////////////////////////////////
// QVariant checks and conversions

inline bool isIntSafe( const QVariant& v )
{
  if ( v.type() == QVariant::Int ) return true;
  if ( v.type() == QVariant::UInt ) return true;
  if ( v.type() == QVariant::LongLong ) return true;
  if ( v.type() == QVariant::ULongLong ) return true;
  if ( v.type() == QVariant::Double ) return false;
  if ( v.type() == QVariant::String ) { bool ok; v.toString().toInt( &ok ); return ok; }
  return false;
}
inline bool isDoubleSafe( const QVariant& v )
{
  if ( v.type() == QVariant::Double ) return true;
  if ( v.type() == QVariant::Int ) return true;
  if ( v.type() == QVariant::UInt ) return true;
  if ( v.type() == QVariant::LongLong ) return true;
  if ( v.type() == QVariant::ULongLong ) return true;
  if ( v.type() == QVariant::String ) { bool ok; v.toString().toDouble( &ok ); return ok; }
  return false;
}

inline bool isDateTimeSafe( const QVariant& v )
{
  return v.type() == QVariant::DateTime || v.type() == QVariant::Date ||
         v.type() == QVariant::Time;
}

inline bool isIntervalSafe( const QVariant& v )
{
  if ( v.canConvert<QgsExpression::Interval>() )
  {
    return true;
  }

  if ( v.type() == QVariant::String )
  {
    return QgsExpression::Interval::fromString( v.toString() ).isValid();
  }
  return false;
}

inline bool isNull( const QVariant& v ) { return v.isNull(); }

///////////////////////////////////////////////
// evaluation error macros

#define ENSURE_NO_EVAL_ERROR   {  if (parent->hasEvalError()) return QVariant(); }
#define SET_EVAL_ERROR(x)   { parent->setEvalErrorString(x); return QVariant(); }

///////////////////////////////////////////////
// operators

const char* QgsExpression::BinaryOperatorText[] =
{
  // this must correspond (number and order of element) to the declaration of the enum BinaryOperator
  "OR", "AND",
  "=", "<>", "<=", ">=", "<", ">", "~", "LIKE", "NOT LIKE", "ILIKE", "NOT ILIKE", "IS", "IS NOT",
  "+", "-", "*", "/", "//", "%", "^",
  "||"
};

const char* QgsExpression::UnaryOperatorText[] =
{
  // this must correspond (number and order of element) to the declaration of the enum UnaryOperator
  "NOT", "-"
};

///////////////////////////////////////////////
// functions

// implicit conversion to string
static QString getStringValue( const QVariant& value, QgsExpression* )
{
  return value.toString();
}

static double getDoubleValue( const QVariant& value, QgsExpression* parent )
{
  bool ok;
  double x = value.toDouble( &ok );
  if ( !ok )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1' to double" ).arg( value.toString() ) );
    return 0;
  }
  return x;
}

static int getIntValue( const QVariant& value, QgsExpression* parent )
{
  bool ok;
  qint64 x = value.toLongLong( &ok );
  if ( ok && x >= std::numeric_limits<int>::min() && x <= std::numeric_limits<int>::max() )
  {
    return x;
  }
  else
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1' to int" ).arg( value.toString() ) );
    return 0;
  }
}

static QDateTime getDateTimeValue( const QVariant& value, QgsExpression* parent )
{
  QDateTime d = value.toDateTime();
  if ( d.isValid() )
  {
    return d;
  }
  else
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1' to DateTime" ).arg( value.toString() ) );
    return QDateTime();
  }
}

static QDate getDateValue( const QVariant& value, QgsExpression* parent )
{
  QDate d = value.toDate();
  if ( d.isValid() )
  {
    return d;
  }
  else
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1' to Date" ).arg( value.toString() ) );
    return QDate();
  }
}

static QTime getTimeValue( const QVariant& value, QgsExpression* parent )
{
  QTime t = value.toTime();
  if ( t.isValid() )
  {
    return t;
  }
  else
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1' to Time" ).arg( value.toString() ) );
    return QTime();
  }
}

static QgsExpression::Interval getInterval( const QVariant& value, QgsExpression* parent, bool report_error = false )
{
  if ( value.canConvert<QgsExpression::Interval>() )
    return value.value<QgsExpression::Interval>();

  QgsExpression::Interval inter = QgsExpression::Interval::fromString( value.toString() );
  if ( inter.isValid() )
  {
    return inter;
  }
  // If we get here then we can't convert so we just error and return invalid.
  if ( report_error )
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1' to Interval" ).arg( value.toString() ) );

  return QgsExpression::Interval::invalidInterVal();
}

static QgsGeometry getGeometry( const QVariant& value, QgsExpression* parent )
{
  if ( value.canConvert<QgsGeometry>() )
    return value.value<QgsGeometry>();

  parent->setEvalErrorString( "Cannot convert to QgsGeometry" );
  return QgsGeometry();
}

static QgsFeature getFeature( const QVariant& value, QgsExpression* parent )
{
  if ( value.canConvert<QgsFeature>() )
    return value.value<QgsFeature>();

  parent->setEvalErrorString( "Cannot convert to QgsFeature" );
  return 0;
}

static QgsExpression::Node* getNode( const QVariant& value, QgsExpression* parent )
{
  if ( value.canConvert<QgsExpression::Node*>() )
    return value.value<QgsExpression::Node*>();

  parent->setEvalErrorString( "Cannot convert to Node" );
  return 0;
}

// this handles also NULL values
static TVL getTVLValue( const QVariant& value, QgsExpression* parent )
{
  // we need to convert to TVL
  if ( value.isNull() )
    return Unknown;

  if ( value.type() == QVariant::Int )
    return value.toInt() != 0 ? True : False;

  bool ok;
  double x = value.toDouble( &ok );
  if ( !ok )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1' to boolean" ).arg( value.toString() ) );
    return Unknown;
  }
  return x != 0 ? True : False;
}

//////

static QVariant fcnSqrt( const QVariantList& values, const QgsFeature* /*f*/, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( sqrt( x ) );
}

static QVariant fcnAbs( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double val = getDoubleValue( values.at( 0 ), parent );
  return QVariant( fabs( val ) );
}

static QVariant fcnSin( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( sin( x ) );
}
static QVariant fcnCos( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( cos( x ) );
}
static QVariant fcnTan( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( tan( x ) );
}
static QVariant fcnAsin( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( asin( x ) );
}
static QVariant fcnAcos( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( acos( x ) );
}
static QVariant fcnAtan( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( atan( x ) );
}
static QVariant fcnAtan2( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double y = getDoubleValue( values.at( 0 ), parent );
  double x = getDoubleValue( values.at( 1 ), parent );
  return QVariant( atan2( y, x ) );
}
static QVariant fcnExp( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( exp( x ) );
}
static QVariant fcnLn( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  if ( x <= 0 )
    return QVariant();
  return QVariant( log( x ) );
}
static QVariant fcnLog10( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  if ( x <= 0 )
    return QVariant();
  return QVariant( log10( x ) );
}
static QVariant fcnLog( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double b = getDoubleValue( values.at( 0 ), parent );
  double x = getDoubleValue( values.at( 1 ), parent );
  if ( x <= 0 || b <= 0 )
    return QVariant();
  return QVariant( log( x ) / log( b ) );
}
static QVariant fcnRndF( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double min = getDoubleValue( values.at( 0 ), parent );
  double max = getDoubleValue( values.at( 1 ), parent );
  if ( max < min )
    return QVariant();

  // Return a random double in the range [min, max] (inclusive)
  double f = ( double )rand() / RAND_MAX;
  return QVariant( min + f * ( max - min ) );
}
static QVariant fcnRnd( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  int min = getIntValue( values.at( 0 ), parent );
  int max = getIntValue( values.at( 1 ), parent );
  if ( max < min )
    return QVariant();

  // Return a random integer in the range [min, max] (inclusive)
  return QVariant( min + ( rand() % ( int )( max - min + 1 ) ) );
}

static QVariant fcnLinearScale( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double val = getDoubleValue( values.at( 0 ), parent );
  double domainMin = getDoubleValue( values.at( 1 ), parent );
  double domainMax = getDoubleValue( values.at( 2 ), parent );
  double rangeMin = getDoubleValue( values.at( 3 ), parent );
  double rangeMax = getDoubleValue( values.at( 4 ), parent );

  if ( domainMin >= domainMax )
  {
    parent->setEvalErrorString( QObject::tr( "Domain max must be greater than domain min" ) );
    return QVariant();
  }

  // outside of domain?
  if ( val >= domainMax )
  {
    return rangeMax;
  }
  else if ( val <= domainMin )
  {
    return rangeMin;
  }

  // calculate linear scale
  double m = ( rangeMax - rangeMin ) / ( domainMax - domainMin );
  double c = rangeMin - ( domainMin * m );

  // Return linearly scaled value
  return QVariant( m * val + c );
}

static QVariant fcnExpScale( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double val = getDoubleValue( values.at( 0 ), parent );
  double domainMin = getDoubleValue( values.at( 1 ), parent );
  double domainMax = getDoubleValue( values.at( 2 ), parent );
  double rangeMin = getDoubleValue( values.at( 3 ), parent );
  double rangeMax = getDoubleValue( values.at( 4 ), parent );
  double exponent = getDoubleValue( values.at( 5 ), parent );

  if ( domainMin >= domainMax )
  {
    parent->setEvalErrorString( QObject::tr( "Domain max must be greater than domain min" ) );
    return QVariant();
  }
  if ( exponent <= 0 )
  {
    parent->setEvalErrorString( QObject::tr( "Exponent must be greater than 0" ) );
    return QVariant();
  }

  // outside of domain?
  if ( val >= domainMax )
  {
    return rangeMax;
  }
  else if ( val <= domainMin )
  {
    return rangeMin;
  }

  // Return exponentially scaled value
  return QVariant((( rangeMax - rangeMin ) / pow( domainMax - domainMin, exponent ) ) * pow( val - domainMin, exponent ) + rangeMin );
}

static QVariant fcnMax( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  //initially set max as first value
  double maxVal = getDoubleValue( values.at( 0 ), parent );

  //check against all other values
  for ( int i = 1; i < values.length(); ++i )
  {
    double testVal = getDoubleValue( values[i], parent );
    if ( testVal > maxVal )
    {
      maxVal = testVal;
    }
  }

  return QVariant( maxVal );
}

static QVariant fcnMin( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  //initially set min as first value
  double minVal = getDoubleValue( values.at( 0 ), parent );

  //check against all other values
  for ( int i = 1; i < values.length(); ++i )
  {
    double testVal = getDoubleValue( values[i], parent );
    if ( testVal < minVal )
    {
      minVal = testVal;
    }
  }

  return QVariant( minVal );
}

static QVariant fcnClamp( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double minValue = getDoubleValue( values.at( 0 ), parent );
  double testValue = getDoubleValue( values.at( 1 ), parent );
  double maxValue = getDoubleValue( values.at( 2 ), parent );

  // force testValue to sit inside the range specified by the min and max value
  if ( testValue <= minValue )
  {
    return QVariant( minValue );
  }
  else if ( testValue >= maxValue )
  {
    return QVariant( maxValue );
  }
  else
  {
    return QVariant( testValue );
  }
}

static QVariant fcnFloor( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( floor( x ) );
}

static QVariant fcnCeil( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( ceil( x ) );
}

static QVariant fcnToInt( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  return QVariant( getIntValue( values.at( 0 ), parent ) );
}
static QVariant fcnToReal( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  return QVariant( getDoubleValue( values.at( 0 ), parent ) );
}
static QVariant fcnToString( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  return QVariant( getStringValue( values.at( 0 ), parent ) );
}

static QVariant fcnToDateTime( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  return QVariant( getDateTimeValue( values.at( 0 ), parent ) );
}

static QVariant fcnCoalesce( const QVariantList& values, const QgsFeature*, QgsExpression* )
{
  foreach ( const QVariant &value, values )
  {
    if ( value.isNull() )
      continue;
    return value;
  }
  return QVariant();
}
static QVariant fcnLower( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  return QVariant( str.toLower() );
}
static QVariant fcnUpper( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  return QVariant( str.toUpper() );
}
static QVariant fcnTitle( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  QStringList elems = str.split( " " );
  for ( int i = 0; i < elems.size(); i++ )
  {
    if ( elems[i].size() > 1 )
      elems[i] = elems[i].left( 1 ).toUpper() + elems[i].mid( 1 ).toLower();
  }
  return QVariant( elems.join( " " ) );
}

static QVariant fcnTrim( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  return QVariant( str.trimmed() );
}

static QVariant fcnWordwrap( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  if ( values.length() == 2 || values.length() == 3 )
  {
    QString str = getStringValue( values.at( 0 ), parent );
    int wrap = getIntValue( values.at( 1 ), parent );

    if ( !str.isEmpty() && wrap != 0 )
    {
      QString newstr;
      QString delimiterstr;
      if ( values.length() == 3 ) delimiterstr = getStringValue( values.at( 2 ), parent );
      if ( delimiterstr.isEmpty() ) delimiterstr = " ";
      int delimiterlength = delimiterstr.length();

      QStringList lines = str.split( "\n" );
      int strlength, strcurrent, strhit, lasthit;

      for ( int i = 0; i < lines.size(); i++ )
      {
        strlength = lines[i].length();
        strcurrent = 0;
        strhit = 0;
        lasthit = 0;

        while ( strcurrent < strlength )
        {
          // positive wrap value = desired maximum line width to wrap
          // negative wrap value = desired minimum line width before wrap
          if ( wrap > 0 )
          {
            //first try to locate delimiter backwards
            strhit = lines[i].lastIndexOf( delimiterstr, strcurrent + wrap );
            if ( strhit == lasthit || strhit == -1 )
            {
              //if no new backward delimiter found, try to locate forward
              strhit = lines[i].indexOf( delimiterstr, strcurrent + qAbs( wrap ) );
            }
            lasthit = strhit;
          }
          else
          {
            strhit = lines[i].indexOf( delimiterstr, strcurrent + qAbs( wrap ) );
          }
          if ( strhit > -1 )
          {
            newstr.append( lines[i].midRef( strcurrent, strhit - strcurrent ) );
            newstr.append( "\n" );
            strcurrent = strhit + delimiterlength;
          }
          else
          {
            newstr.append( lines[i].midRef( strcurrent ) );
            strcurrent = strlength;
          }
        }
        if ( i < lines.size() - 1 ) newstr.append( "\n" );
      }

      return QVariant( newstr );
    }
  }

  return QVariant();
}

static QVariant fcnLength( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  return QVariant( str.length() );
}
static QVariant fcnReplace( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  QString before = getStringValue( values.at( 1 ), parent );
  QString after = getStringValue( values.at( 2 ), parent );
  return QVariant( str.replace( before, after ) );
}
static QVariant fcnRegexpReplace( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  QString regexp = getStringValue( values.at( 1 ), parent );
  QString after = getStringValue( values.at( 2 ), parent );

  QRegExp re( regexp );
  if ( !re.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Invalid regular expression '%1': %2" ).arg( regexp ).arg( re.errorString() ) );
    return QVariant();
  }
  return QVariant( str.replace( re, after ) );
}

static QVariant fcnRegexpMatch( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  QString regexp = getStringValue( values.at( 1 ), parent );

  QRegExp re( regexp );
  if ( !re.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Invalid regular expression '%1': %2" ).arg( regexp ).arg( re.errorString() ) );
    return QVariant();
  }
  return QVariant( str.contains( re ) ? 1 : 0 );
}

static QVariant fcnRegexpSubstr( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  QString regexp = getStringValue( values.at( 1 ), parent );

  QRegExp re( regexp );
  if ( !re.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Invalid regular expression '%1': %2" ).arg( regexp ).arg( re.errorString() ) );
    return QVariant();
  }

  // extract substring
  re.indexIn( str );
  if ( re.captureCount() > 0 )
  {
    // return first capture
    return QVariant( re.capturedTexts()[1] );
  }
  else
  {
    return QVariant( "" );
  }
}

static QVariant fcnUuid( const QVariantList&, const QgsFeature*, QgsExpression* )
{
  return QUuid::createUuid().toString();
}

static QVariant fcnSubstr( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  int from = getIntValue( values.at( 1 ), parent );
  int len = getIntValue( values.at( 2 ), parent );
  return QVariant( str.mid( from -1, len ) );
}

static QVariant fcnRowNumber( const QVariantList&, const QgsFeature*, QgsExpression* parent )
{
  return QVariant( parent->currentRowNumber() );
}

static QVariant fcnFeatureId( const QVariantList&, const QgsFeature* f, QgsExpression* )
{
  // TODO: handling of 64-bit feature ids?
  return f ? QVariant(( int )f->id() ) : QVariant();
}

static QVariant fcnFeature( const QVariantList&, const QgsFeature* f, QgsExpression* )
{
  return f ? QVariant::fromValue( *f ) : QVariant();
}
static QVariant fcnAttribute( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsFeature feat = getFeature( values.at( 0 ), parent );
  QString attr = getStringValue( values.at( 1 ), parent );

  return feat.attribute( attr );
}
static QVariant fcnConcat( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QString concat;
  foreach ( const QVariant &value, values )
  {
    concat += getStringValue( value, parent );
  }
  return concat;
}

static QVariant fcnStrpos( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  return string.indexOf( QRegExp( getStringValue( values.at( 1 ), parent ) ) );
}

static QVariant fcnRight( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  int pos = getIntValue( values.at( 1 ), parent );
  return string.right( pos );
}

static QVariant fcnLeft( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  int pos = getIntValue( values.at( 1 ), parent );
  return string.left( pos );
}

static QVariant fcnRPad( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  int length = getIntValue( values.at( 1 ), parent );
  QString fill = getStringValue( values.at( 2 ), parent );
  return string.leftJustified( length, fill.at( 0 ), true );
}

static QVariant fcnLPad( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  int length = getIntValue( values.at( 1 ), parent );
  QString fill = getStringValue( values.at( 2 ), parent );
  return string.rightJustified( length, fill.at( 0 ), true );
}

static QVariant fcnFormatString( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  for ( int n = 1; n < values.length(); n++ )
  {
    string = string.arg( getStringValue( values.at( n ), parent ) );
  }
  return string;
}


static QVariant fcnNow( const QVariantList&, const QgsFeature*, QgsExpression * )
{
  return QVariant( QDateTime::currentDateTime() );
}

static QVariant fcnToDate( const QVariantList& values, const QgsFeature*, QgsExpression * parent )
{
  return QVariant( getDateValue( values.at( 0 ), parent ) );
}

static QVariant fcnToTime( const QVariantList& values, const QgsFeature*, QgsExpression * parent )
{
  return QVariant( getTimeValue( values.at( 0 ), parent ) );
}

static QVariant fcnToInterval( const QVariantList& values, const QgsFeature*, QgsExpression * parent )
{
  return QVariant::fromValue( getInterval( values.at( 0 ), parent ) );
}

static QVariant fcnAge( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QDateTime d1 = getDateTimeValue( values.at( 0 ), parent );
  QDateTime d2 = getDateTimeValue( values.at( 1 ), parent );
  int seconds = d2.secsTo( d1 );
  return QVariant::fromValue( QgsExpression::Interval( seconds ) );
}

static QVariant fcnDay( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QVariant value = values.at( 0 );
  QgsExpression::Interval inter = getInterval( value, parent, false );
  if ( inter.isValid() )
  {
    return QVariant( inter.days() );
  }
  else
  {
    QDateTime d1 =  getDateTimeValue( value, parent );
    return QVariant( d1.date().day() );
  }
}

static QVariant fcnYear( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QVariant value = values.at( 0 );
  QgsExpression::Interval inter = getInterval( value, parent, false );
  if ( inter.isValid() )
  {
    return QVariant( inter.years() );
  }
  else
  {
    QDateTime d1 =  getDateTimeValue( value, parent );
    return QVariant( d1.date().year() );
  }
}

static QVariant fcnMonth( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QVariant value = values.at( 0 );
  QgsExpression::Interval inter = getInterval( value, parent, false );
  if ( inter.isValid() )
  {
    return QVariant( inter.months() );
  }
  else
  {
    QDateTime d1 =  getDateTimeValue( value, parent );
    return QVariant( d1.date().month() );
  }
}

static QVariant fcnWeek( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QVariant value = values.at( 0 );
  QgsExpression::Interval inter = getInterval( value, parent, false );
  if ( inter.isValid() )
  {
    return QVariant( inter.weeks() );
  }
  else
  {
    QDateTime d1 =  getDateTimeValue( value, parent );
    return QVariant( d1.date().weekNumber() );
  }
}

static QVariant fcnHour( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QVariant value = values.at( 0 );
  QgsExpression::Interval inter = getInterval( value, parent, false );
  if ( inter.isValid() )
  {
    return QVariant( inter.hours() );
  }
  else
  {
    QDateTime d1 =  getDateTimeValue( value, parent );
    return QVariant( d1.time().hour() );
  }
}

static QVariant fcnMinute( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QVariant value = values.at( 0 );
  QgsExpression::Interval inter = getInterval( value, parent, false );
  if ( inter.isValid() )
  {
    return QVariant( inter.minutes() );
  }
  else
  {
    QDateTime d1 =  getDateTimeValue( value, parent );
    return QVariant( d1.time().minute() );
  }
}

static QVariant fcnSeconds( const QVariantList& values, const QgsFeature*, QgsExpression *parent )
{
  QVariant value = values.at( 0 );
  QgsExpression::Interval inter = getInterval( value, parent, false );
  if ( inter.isValid() )
  {
    return QVariant( inter.seconds() );
  }
  else
  {
    QDateTime d1 =  getDateTimeValue( value, parent );
    return QVariant( d1.time().second() );
  }
}


#define ENSURE_GEOM_TYPE(f, g, geomtype)   if (!f) return QVariant(); \
  QgsGeometry* g = f->geometry(); \
  if (!g || g->type() != geomtype) return QVariant();


static QVariant fcnX( const QVariantList&, const QgsFeature* f, QgsExpression* )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Point );
  if ( g->isMultipart() )
  {
    return g->asMultiPoint()[ 0 ].x();
  }
  else
  {
    return g->asPoint().x();
  }
}
static QVariant fcnY( const QVariantList&, const QgsFeature* f, QgsExpression* )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Point );
  if ( g->isMultipart() )
  {
    return g->asMultiPoint()[ 0 ].y();
  }
  else
  {
    return g->asPoint().y();
  }
}

static QVariant pointAt( const QVariantList& values, const QgsFeature* f, QgsExpression* parent ) // helper function
{
  int idx = getIntValue( values.at( 0 ), parent );
  ENSURE_GEOM_TYPE( f, g, QGis::Line );
  QgsPolyline polyline = g->asPolyline();
  if ( idx < 0 )
    idx += polyline.count();

  if ( idx < 0 || idx >= polyline.count() )
  {
    parent->setEvalErrorString( QObject::tr( "Index is out of range" ) );
    return QVariant();
  }
  return QVariant( QPointF( polyline[idx].x(), polyline[idx].y() ) );
}

static QVariant fcnXat( const QVariantList& values, const QgsFeature* f, QgsExpression* parent )
{
  QVariant v = pointAt( values, f, parent );
  if ( v.type() == QVariant::PointF )
    return QVariant( v.toPointF().x() );
  else
    return QVariant();
}
static QVariant fcnYat( const QVariantList& values, const QgsFeature* f, QgsExpression* parent )
{
  QVariant v = pointAt( values, f, parent );
  if ( v.type() == QVariant::PointF )
    return QVariant( v.toPointF().y() );
  else
    return QVariant();
}
static QVariant fcnGeometry( const QVariantList&, const QgsFeature* f, QgsExpression* )
{
  QgsGeometry* geom = f ? f->geometry() : 0;
  if ( geom )
    return  QVariant::fromValue( *geom );
  else
    return QVariant();
}
static QVariant fcnGeomFromWKT( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString wkt = getStringValue( values.at( 0 ), parent );
  QgsGeometry* geom = QgsGeometry::fromWkt( wkt );
  if ( geom )
    return QVariant::fromValue( *geom );
  else
    return QVariant();
}
static QVariant fcnGeomFromGML( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QString gml = getStringValue( values.at( 0 ), parent );
  QgsGeometry* geom = QgsOgcUtils::geometryFromGML( gml );

  if ( geom )
    return QVariant::fromValue( *geom );
  else
    return QVariant();
}

static QVariant fcnGeomArea( const QVariantList&, const QgsFeature* f, QgsExpression* parent )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Polygon );
  QgsDistanceArea* calc = parent->geomCalculator();
  return QVariant( calc->measure( f->geometry() ) );
}
static QVariant fcnGeomLength( const QVariantList&, const QgsFeature* f, QgsExpression* parent )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Line );
  QgsDistanceArea* calc = parent->geomCalculator();
  return QVariant( calc->measure( f->geometry() ) );
}
static QVariant fcnGeomPerimeter( const QVariantList&, const QgsFeature* f, QgsExpression* parent )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Polygon );
  QgsDistanceArea* calc = parent->geomCalculator();
  return QVariant( calc->measurePerimeter( f->geometry() ) );
}

static QVariant fcnBounds( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry geom = getGeometry( values.at( 0 ), parent );
  QgsGeometry* geomBounds = QgsGeometry::fromRect( geom.boundingBox() );
  if ( geomBounds )
  {
    return QVariant::fromValue( *geomBounds );
  }
  else
  {
    return QVariant();
  }
}

static QVariant fcnBoundsWidth( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry geom = getGeometry( values.at( 0 ), parent );
  return QVariant::fromValue( geom.boundingBox().width() );
}

static QVariant fcnBoundsHeight( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry geom = getGeometry( values.at( 0 ), parent );
  return QVariant::fromValue( geom.boundingBox().height() );
}

static QVariant fcnXMin( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry geom = getGeometry( values.at( 0 ), parent );
  return QVariant::fromValue( geom.boundingBox().xMinimum() );
}

static QVariant fcnXMax( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry geom = getGeometry( values.at( 0 ), parent );
  return QVariant::fromValue( geom.boundingBox().xMaximum() );
}

static QVariant fcnYMin( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry geom = getGeometry( values.at( 0 ), parent );
  return QVariant::fromValue( geom.boundingBox().yMinimum() );
}

static QVariant fcnYMax( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry geom = getGeometry( values.at( 0 ), parent );
  return QVariant::fromValue( geom.boundingBox().yMaximum() );
}

static QVariant fcnBbox( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return fGeom.intersects( sGeom.boundingBox() ) ? TVL_True : TVL_False;
}
static QVariant fcnDisjoint( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return fGeom.disjoint( &sGeom ) ? TVL_True : TVL_False;
}
static QVariant fcnIntersects( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return fGeom.intersects( &sGeom ) ? TVL_True : TVL_False;
}
static QVariant fcnTouches( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return fGeom.touches( &sGeom ) ? TVL_True : TVL_False;
}
static QVariant fcnCrosses( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return fGeom.crosses( &sGeom ) ? TVL_True : TVL_False;
}
static QVariant fcnContains( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return fGeom.contains( &sGeom ) ? TVL_True : TVL_False;
}
static QVariant fcnOverlaps( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return fGeom.overlaps( &sGeom ) ? TVL_True : TVL_False;
}
static QVariant fcnWithin( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return fGeom.within( &sGeom ) ? TVL_True : TVL_False;
}
static QVariant fcnBuffer( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  if ( values.length() < 2 || values.length() > 3 )
    return QVariant();

  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  double dist = getDoubleValue( values.at( 1 ), parent );
  int seg = 8;
  if ( values.length() == 3 )
    seg = getIntValue( values.at( 2 ), parent );

  QgsGeometry* geom = fGeom.buffer( dist, seg );
  if ( geom )
    return QVariant::fromValue( *geom );
  return QVariant();
}
static QVariant fcnCentroid( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry* geom = fGeom.centroid();
  if ( geom )
    return QVariant::fromValue( *geom );
  return QVariant();
}
static QVariant fcnConvexHull( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry* geom = fGeom.convexHull();
  if ( geom )
    return QVariant::fromValue( *geom );
  return QVariant();
}
static QVariant fcnDifference( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  QgsGeometry* geom = fGeom.difference( &sGeom );
  if ( geom )
    return QVariant::fromValue( *geom );
  return QVariant();
}
static QVariant fcnDistance( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  return QVariant( fGeom.distance( sGeom ) );
}
static QVariant fcnIntersection( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  QgsGeometry* geom = fGeom.intersection( &sGeom );
  if ( geom )
    return QVariant::fromValue( *geom );
  return QVariant();
}
static QVariant fcnSymDifference( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  QgsGeometry* geom = fGeom.symDifference( &sGeom );
  if ( geom )
    return QVariant::fromValue( *geom );
  return QVariant();
}
static QVariant fcnCombine( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QgsGeometry sGeom = getGeometry( values.at( 1 ), parent );
  QgsGeometry* geom = fGeom.combine( &sGeom );
  if ( geom )
    return QVariant::fromValue( *geom );
  return QVariant();
}
static QVariant fcnGeomToWKT( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  if ( values.length() < 1 || values.length() > 2 )
    return QVariant();

  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  int prec = 8;
  if ( values.length() == 2 )
    prec = getIntValue( values.at( 1 ), parent );
  QString wkt = fGeom.exportToWkt( prec );
  return QVariant( wkt );
}

static QVariant fcnRound( const QVariantList& values, const QgsFeature *f, QgsExpression* parent )
{
  Q_UNUSED( f );
  if ( values.length() == 2 )
  {
    double number = getDoubleValue( values.at( 0 ), parent );
    double scaler = pow( 10.0, getIntValue( values.at( 1 ), parent ) );
    return QVariant( qRound( number * scaler ) / scaler );
  }

  if ( values.length() == 1 )
  {
    double number = getIntValue( values.at( 0 ), parent );
    return QVariant( qRound( number ) ).toInt();
  }

  return QVariant();
}

static QVariant fcnPi( const QVariantList& values, const QgsFeature *f, QgsExpression* parent )
{
  Q_UNUSED( values );
  Q_UNUSED( f );
  Q_UNUSED( parent );
  return M_PI;
}

static QVariant fcnScale( const QVariantList&, const QgsFeature*, QgsExpression* parent )
{
  return QVariant( parent->scale() );
}

static QVariant fcnFormatNumber( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  double value = getDoubleValue( values.at( 0 ), parent );
  int places = getIntValue( values.at( 1 ), parent );
  return QString( "%L1" ).arg( value, 0, 'f', places );
}

static QVariant fcnFormatDate( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QDateTime dt = getDateTimeValue( values.at( 0 ), parent );
  QString format = getStringValue( values.at( 1 ), parent );
  return dt.toString( format );
}

static QVariant fcnColorRgb( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  int red = getIntValue( values.at( 0 ), parent );
  int green = getIntValue( values.at( 1 ), parent );
  int blue = getIntValue( values.at( 2 ), parent );
  QColor color = QColor( red, green, blue );
  if ( ! color.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1:%2:%3' to color" ).arg( red ).arg( green ).arg( blue ) );
    color = QColor( 0, 0, 0 );
  }

  return QString( "%1,%2,%3" ).arg( color.red() ).arg( color.green() ).arg( color.blue() );
}

static QVariant fcnIf( const QVariantList &values, const QgsFeature *f, QgsExpression *parent )
{
  QgsExpression::Node* node = getNode( values.at( 0 ), parent );
  ENSURE_NO_EVAL_ERROR;
  QVariant value = node->eval( parent, f );
  ENSURE_NO_EVAL_ERROR;
  if ( value.toBool() )
  {
    node = getNode( values.at( 1 ), parent );
    ENSURE_NO_EVAL_ERROR;
    value = node->eval( parent, f );
    ENSURE_NO_EVAL_ERROR;
  }
  else
  {
    node = getNode( values.at( 2 ), parent );
    ENSURE_NO_EVAL_ERROR;
    value = node->eval( parent, f );
    ENSURE_NO_EVAL_ERROR;
  }
  return value;
}

static QVariant fncColorRgba( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  int red = getIntValue( values.at( 0 ), parent );
  int green = getIntValue( values.at( 1 ), parent );
  int blue = getIntValue( values.at( 2 ), parent );
  int alpha = getIntValue( values.at( 3 ), parent );
  QColor color = QColor( red, green, blue, alpha );
  if ( ! color.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1:%2:%3:%4' to color" ).arg( red ).arg( green ).arg( blue ).arg( alpha ) );
    color = QColor( 0, 0, 0 );
  }
  return QgsSymbolLayerV2Utils::encodeColor( color );
}

QVariant fcnRampColor( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  QString rampName = getStringValue( values.at( 0 ), parent );
  const QgsVectorColorRampV2 *mRamp = QgsStyleV2::defaultStyle()->colorRampRef( rampName );
  if ( ! mRamp )
  {
    parent->setEvalErrorString( QObject::tr( "\"%1\" is not a valid color ramp" ).arg( rampName ) );
    return QColor( 0, 0, 0 ).name();
  }
  double value = getDoubleValue( values.at( 1 ), parent );
  QColor color = mRamp->color( value );
  return QgsSymbolLayerV2Utils::encodeColor( color );
}

static QVariant fcnColorHsl( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  // Hue ranges from 0 - 360
  double hue = getIntValue( values.at( 0 ), parent ) / 360.0;
  // Saturation ranges from 0 - 100
  double saturation = getIntValue( values.at( 1 ), parent ) / 100.0;
  // Lightness ranges from 0 - 100
  double lightness = getIntValue( values.at( 2 ), parent ) / 100.0;

  QColor color = QColor::fromHslF( hue, saturation, lightness );

  if ( ! color.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1:%2:%3' to color" ).arg( hue ).arg( saturation ).arg( lightness ) );
    color = QColor( 0, 0, 0 );
  }

  return QString( "%1,%2,%3" ).arg( color.red() ).arg( color.green() ).arg( color.blue() );
}

static QVariant fncColorHsla( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  // Hue ranges from 0 - 360
  double hue = getIntValue( values.at( 0 ), parent ) / 360.0;
  // Saturation ranges from 0 - 100
  double saturation = getIntValue( values.at( 1 ), parent ) / 100.0;
  // Lightness ranges from 0 - 100
  double lightness = getIntValue( values.at( 2 ), parent ) / 100.0;
  // Alpha ranges from 0 - 255
  double alpha = getIntValue( values.at( 3 ), parent ) / 255.0;

  QColor color = QColor::fromHslF( hue, saturation, lightness, alpha );
  if ( ! color.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1:%2:%3:%4' to color" ).arg( hue ).arg( saturation ).arg( lightness ).arg( alpha ) );
    color = QColor( 0, 0, 0 );
  }
  return QgsSymbolLayerV2Utils::encodeColor( color );
}

static QVariant fcnColorHsv( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  // Hue ranges from 0 - 360
  double hue = getIntValue( values.at( 0 ), parent ) / 360.0;
  // Saturation ranges from 0 - 100
  double saturation = getIntValue( values.at( 1 ), parent ) / 100.0;
  // Value ranges from 0 - 100
  double value = getIntValue( values.at( 2 ), parent ) / 100.0;

  QColor color = QColor::fromHsvF( hue, saturation, value );

  if ( ! color.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1:%2:%3' to color" ).arg( hue ).arg( saturation ).arg( value ) );
    color = QColor( 0, 0, 0 );
  }

  return QString( "%1,%2,%3" ).arg( color.red() ).arg( color.green() ).arg( color.blue() );
}

static QVariant fncColorHsva( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  // Hue ranges from 0 - 360
  double hue = getIntValue( values.at( 0 ), parent ) / 360.0;
  // Saturation ranges from 0 - 100
  double saturation = getIntValue( values.at( 1 ), parent ) / 100.0;
  // Value ranges from 0 - 100
  double value = getIntValue( values.at( 2 ), parent ) / 100.0;
  // Alpha ranges from 0 - 255
  double alpha = getIntValue( values.at( 3 ), parent ) / 255.0;

  QColor color = QColor::fromHsvF( hue, saturation, value, alpha );
  if ( ! color.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1:%2:%3:%4' to color" ).arg( hue ).arg( saturation ).arg( value ).arg( alpha ) );
    color = QColor( 0, 0, 0 );
  }
  return QgsSymbolLayerV2Utils::encodeColor( color );
}

static QVariant fcnColorCmyk( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  // Cyan ranges from 0 - 100
  double cyan = getIntValue( values.at( 0 ), parent ) / 100.0;
  // Magenta ranges from 0 - 100
  double magenta = getIntValue( values.at( 1 ), parent ) / 100.0;
  // Yellow ranges from 0 - 100
  double yellow = getIntValue( values.at( 2 ), parent ) / 100.0;
  // Black ranges from 0 - 100
  double black = getIntValue( values.at( 3 ), parent ) / 100.0;

  QColor color = QColor::fromCmykF( cyan, magenta, yellow, black );

  if ( ! color.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1:%2:%3:%4' to color" ).arg( cyan ).arg( magenta ).arg( yellow ).arg( black ) );
    color = QColor( 0, 0, 0 );
  }

  return QString( "%1,%2,%3" ).arg( color.red() ).arg( color.green() ).arg( color.blue() );
}

static QVariant fncColorCmyka( const QVariantList &values, const QgsFeature *, QgsExpression *parent )
{
  // Cyan ranges from 0 - 100
  double cyan = getIntValue( values.at( 0 ), parent ) / 100.0;
  // Magenta ranges from 0 - 100
  double magenta = getIntValue( values.at( 1 ), parent ) / 100.0;
  // Yellow ranges from 0 - 100
  double yellow = getIntValue( values.at( 2 ), parent ) / 100.0;
  // Black ranges from 0 - 100
  double black = getIntValue( values.at( 3 ), parent ) / 100.0;
  // Alpha ranges from 0 - 255
  double alpha = getIntValue( values.at( 4 ), parent ) / 255.0;

  QColor color = QColor::fromCmykF( cyan, magenta, yellow, black, alpha );
  if ( ! color.isValid() )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1:%2:%3:%4:%5' to color" ).arg( cyan ).arg( magenta ).arg( yellow ).arg( black ).arg( alpha ) );
    color = QColor( 0, 0, 0 );
  }
  return QgsSymbolLayerV2Utils::encodeColor( color );
}

static QVariant fcnSpecialColumn( const QVariantList& values, const QgsFeature* /*f*/, QgsExpression* parent )
{
  QString varName = getStringValue( values.at( 0 ), parent );
  return QgsExpression::specialColumn( varName );
}

static QVariant fcnGetGeometry( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsFeature feat = getFeature( values.at( 0 ), parent );
  QgsGeometry* geom = feat.geometry();
  if ( geom )
    return QVariant::fromValue( *geom );
  return QVariant();
}

static QVariant fcnTransformGeometry( const QVariantList& values, const QgsFeature*, QgsExpression* parent )
{
  QgsGeometry fGeom = getGeometry( values.at( 0 ), parent );
  QString sAuthId = getStringValue( values.at( 1 ), parent );
  QString dAuthId = getStringValue( values.at( 2 ), parent );

  QgsCoordinateReferenceSystem s;
  if ( ! s.createFromOgcWmsCrs( sAuthId ) )
    return QVariant::fromValue( fGeom );
  QgsCoordinateReferenceSystem d;
  if ( ! d.createFromOgcWmsCrs( dAuthId ) )
    return QVariant::fromValue( fGeom );

  QgsCoordinateTransform t( s, d );
  if ( fGeom.transform( t ) == 0 )
    return QVariant::fromValue( fGeom );
  return QVariant();
}

static QVariant fcnGetFeature( const QVariantList& values, const QgsFeature *, QgsExpression* parent )
{
  //arguments: 1. layer id / name, 2. key attribute, 3. eq value
  QString layerString = getStringValue( values.at( 0 ), parent );
  QgsVectorLayer* vl = qobject_cast<QgsVectorLayer*>( QgsMapLayerRegistry::instance()->mapLayer( layerString ) ); //search by id first
  if ( !vl )
  {
    QList<QgsMapLayer *> layersByName = QgsMapLayerRegistry::instance()->mapLayersByName( layerString );
    if ( layersByName.size() > 0 )
    {
      vl = qobject_cast<QgsVectorLayer*>( layersByName.at( 0 ) );
    }
  }

  //no layer found
  if ( !vl )
  {
    return QVariant();
  }

  QString attribute = getStringValue( values.at( 1 ), parent );
  int attributeId = vl->fieldNameIndex( attribute );
  if ( attributeId == -1 )
  {
    return QVariant();
  }

  const QVariant& attVal = values.at( 2 );
  QgsFeatureRequest req;
  if ( !parent->needsGeometry() )
  {
    req.setFlags( QgsFeatureRequest::NoGeometry );
  }
  QgsFeatureIterator fIt = vl->getFeatures( req );

  QgsFeature fet;
  while ( fIt.nextFeature( fet ) )
  {
    if ( fet.attribute( attributeId ) == attVal )
    {
      return QVariant::fromValue( fet );
    }
  }
  return QVariant();
}

bool QgsExpression::registerFunction( QgsExpression::Function* function )
{
  int fnIdx = functionIndex( function->name() );
  if ( fnIdx != -1 )
  {
    return false;
  }
  QgsExpression::gmFunctions.append( function );
  return true;
}

bool QgsExpression::unregisterFunction( QString name )
{
  // You can never override the built in functions.
  if ( QgsExpression::BuiltinFunctions().contains( name ) )
  {
    return false;
  }
  int fnIdx = functionIndex( name );
  if ( fnIdx != -1 )
  {
    QgsExpression::gmFunctions.removeAt( fnIdx );
    return true;
  }
  return false;
}



QStringList QgsExpression::gmBuiltinFunctions;

const QStringList &QgsExpression::BuiltinFunctions()
{
  if ( gmBuiltinFunctions.isEmpty() )
  {
    gmBuiltinFunctions
    << "abs" << "sqrt" << "cos" << "sin" << "tan"
    << "asin" << "acos" << "atan" << "atan2"
    << "exp" << "ln" << "log10" << "log"
    << "round" << "rand" << "randf" << "max" << "min" << "clamp"
    << "scale_linear" << "scale_exp" << "floor" << "ceil"
    << "toint" << "toreal" << "tostring"
    << "todatetime" << "todate" << "totime" << "tointerval"
    << "coalesce" << "regexp_match" << "$now" << "age" << "year"
    << "month" << "week" << "day" << "hour"
    << "minute" << "second" << "lower" << "upper"
    << "title" << "length" << "replace" << "trim" << "wordwrap"
    << "regexp_replace" << "regexp_substr"
    << "substr" << "concat" << "strpos" << "left"
    << "right" << "rpad" << "lpad"
    << "format_number" << "format_date"
    << "color_rgb" << "color_rgba" << "ramp_color"
    << "color_hsl" << "color_hsla" << "color_hsv" << "color_hsva"
    << "color_cymk" << "color_cymka"
    << "xat" << "yat" << "$area"
    << "$length" << "$perimeter" << "$x" << "$y"
    << "$rownum" << "$id" << "$scale" << "_specialcol_";
  }
  return gmBuiltinFunctions;
}

QList<QgsExpression::Function*> QgsExpression::gmFunctions;

const QList<QgsExpression::Function*> &QgsExpression::Functions()
{
  if ( gmFunctions.isEmpty() )
  {
    gmFunctions
    << new StaticFunction( "sqrt", 1, fcnSqrt, "Math" )
    << new StaticFunction( "abs", 1, fcnAbs, "Math" )
    << new StaticFunction( "cos", 1, fcnCos, "Math" )
    << new StaticFunction( "sin", 1, fcnSin, "Math" )
    << new StaticFunction( "tan", 1, fcnTan, "Math" )
    << new StaticFunction( "asin", 1, fcnAsin, "Math" )
    << new StaticFunction( "acos", 1, fcnAcos, "Math" )
    << new StaticFunction( "atan", 1, fcnAtan, "Math" )
    << new StaticFunction( "atan2", 2, fcnAtan2, "Math" )
    << new StaticFunction( "exp", 1, fcnExp, "Math" )
    << new StaticFunction( "ln", 1, fcnLn, "Math" )
    << new StaticFunction( "log10", 1, fcnLog10, "Math" )
    << new StaticFunction( "log", 2, fcnLog, "Math" )
    << new StaticFunction( "round", -1, fcnRound, "Math" )
    << new StaticFunction( "rand", 2, fcnRnd, "Math" )
    << new StaticFunction( "randf", 2, fcnRndF, "Math" )
    << new StaticFunction( "max", -1, fcnMax, "Math" )
    << new StaticFunction( "min", -1, fcnMin, "Math" )
    << new StaticFunction( "clamp", 3, fcnClamp, "Math" )
    << new StaticFunction( "scale_linear", 5, fcnLinearScale, "Math" )
    << new StaticFunction( "scale_exp", 6, fcnExpScale, "Math" )
    << new StaticFunction( "floor", 1, fcnFloor, "Math" )
    << new StaticFunction( "ceil", 1, fcnCeil, "Math" )
    << new StaticFunction( "$pi", 0, fcnPi, "Math" )
    << new StaticFunction( "toint", 1, fcnToInt, "Conversions" )
    << new StaticFunction( "toreal", 1, fcnToReal, "Conversions" )
    << new StaticFunction( "tostring", 1, fcnToString, "Conversions" )
    << new StaticFunction( "todatetime", 1, fcnToDateTime, "Conversions" )
    << new StaticFunction( "todate", 1, fcnToDate, "Conversions" )
    << new StaticFunction( "totime", 1, fcnToTime, "Conversions" )
    << new StaticFunction( "tointerval", 1, fcnToInterval, "Conversions" )
    << new StaticFunction( "coalesce", -1, fcnCoalesce, "Conditionals" )
    << new StaticFunction( "if", 3, fcnIf, "Conditionals", "", False, QStringList(), true )
    << new StaticFunction( "regexp_match", 2, fcnRegexpMatch, "Conditionals" )
    << new StaticFunction( "$now", 0, fcnNow, "Date and Time" )
    << new StaticFunction( "age", 2, fcnAge, "Date and Time" )
    << new StaticFunction( "year", 1, fcnYear, "Date and Time" )
    << new StaticFunction( "month", 1, fcnMonth, "Date and Time" )
    << new StaticFunction( "week", 1, fcnWeek, "Date and Time" )
    << new StaticFunction( "day", 1, fcnDay, "Date and Time" )
    << new StaticFunction( "hour", 1, fcnHour, "Date and Time" )
    << new StaticFunction( "minute", 1, fcnMinute, "Date and Time" )
    << new StaticFunction( "second", 1, fcnSeconds, "Date and Time" )
    << new StaticFunction( "lower", 1, fcnLower, "String" )
    << new StaticFunction( "upper", 1, fcnUpper, "String" )
    << new StaticFunction( "title", 1, fcnTitle, "String" )
    << new StaticFunction( "trim", 1, fcnTrim, "String" )
    << new StaticFunction( "wordwrap", -1, fcnWordwrap, "String" )
    << new StaticFunction( "length", 1, fcnLength, "String" )
    << new StaticFunction( "replace", 3, fcnReplace, "String" )
    << new StaticFunction( "regexp_replace", 3, fcnRegexpReplace, "String" )
    << new StaticFunction( "regexp_substr", 2, fcnRegexpSubstr, "String" )
    << new StaticFunction( "substr", 3, fcnSubstr, "String" )
    << new StaticFunction( "concat", -1, fcnConcat, "String" )
    << new StaticFunction( "strpos", 2, fcnStrpos, "String" )
    << new StaticFunction( "left", 2, fcnLeft, "String" )
    << new StaticFunction( "right", 2, fcnRight, "String" )
    << new StaticFunction( "rpad", 3, fcnRPad, "String" )
    << new StaticFunction( "lpad", 3, fcnLPad, "String" )
    << new StaticFunction( "format", -1, fcnFormatString, "String" )
    << new StaticFunction( "format_number", 2, fcnFormatNumber, "String" )
    << new StaticFunction( "format_date", 2, fcnFormatDate, "String" )
    << new StaticFunction( "color_rgb", 3, fcnColorRgb, "Color" )
    << new StaticFunction( "color_rgba", 4, fncColorRgba, "Color" )
    << new StaticFunction( "ramp_color", 2, fcnRampColor, "Color" )
    << new StaticFunction( "color_hsl", 3, fcnColorHsl, "Color" )
    << new StaticFunction( "color_hsla", 4, fncColorHsla, "Color" )
    << new StaticFunction( "color_hsv", 3, fcnColorHsv, "Color" )
    << new StaticFunction( "color_hsva", 4, fncColorHsva, "Color" )
    << new StaticFunction( "color_cmyk", 4, fcnColorCmyk, "Color" )
    << new StaticFunction( "color_cmyka", 5, fncColorCmyka, "Color" )
    << new StaticFunction( "$geometry", 0, fcnGeometry, "GeometryGroup", "", true )
    << new StaticFunction( "$area", 0, fcnGeomArea, "GeometryGroup", "", true )
    << new StaticFunction( "$length", 0, fcnGeomLength, "GeometryGroup", "", true )
    << new StaticFunction( "$perimeter", 0, fcnGeomPerimeter, "GeometryGroup", "", true )
    << new StaticFunction( "$x", 0, fcnX, "GeometryGroup", "", true )
    << new StaticFunction( "$y", 0, fcnY, "GeometryGroup", "", true )
    << new StaticFunction( "xat", 1, fcnXat, "GeometryGroup", "", true )
    << new StaticFunction( "yat", 1, fcnYat, "GeometryGroup", "", true )
    << new StaticFunction( "xmin", 1, fcnXMin, "GeometryGroup", "", true )
    << new StaticFunction( "xmax", 1, fcnXMax, "GeometryGroup", "", true )
    << new StaticFunction( "ymin", 1, fcnYMin, "GeometryGroup", "", true )
    << new StaticFunction( "ymax", 1, fcnYMax, "GeometryGroup", "", true )
    << new StaticFunction( "geomFromWKT", 1, fcnGeomFromWKT, "GeometryGroup" )
    << new StaticFunction( "geomFromGML", 1, fcnGeomFromGML, "GeometryGroup" )
    << new StaticFunction( "bbox", 2, fcnBbox, "GeometryGroup" )
    << new StaticFunction( "disjoint", 2, fcnDisjoint, "GeometryGroup" )
    << new StaticFunction( "intersects", 2, fcnIntersects, "GeometryGroup" )
    << new StaticFunction( "touches", 2, fcnTouches, "GeometryGroup" )
    << new StaticFunction( "crosses", 2, fcnCrosses, "GeometryGroup" )
    << new StaticFunction( "contains", 2, fcnContains, "GeometryGroup" )
    << new StaticFunction( "overlaps", 2, fcnOverlaps, "GeometryGroup" )
    << new StaticFunction( "within", 2, fcnWithin, "GeometryGroup" )
    << new StaticFunction( "buffer", -1, fcnBuffer, "GeometryGroup" )
    << new StaticFunction( "centroid", 1, fcnCentroid, "GeometryGroup" )
    << new StaticFunction( "bounds", 1, fcnBounds, "GeometryGroup", "", true )
    << new StaticFunction( "bounds_width", 1, fcnBoundsWidth, "GeometryGroup", "", true )
    << new StaticFunction( "bounds_height", 1, fcnBoundsHeight, "GeometryGroup", "", true )
    << new StaticFunction( "convexHull", 1, fcnConvexHull, "GeometryGroup" )
    << new StaticFunction( "difference", 2, fcnDifference, "GeometryGroup" )
    << new StaticFunction( "distance", 2, fcnDistance, "GeometryGroup" )
    << new StaticFunction( "intersection", 2, fcnIntersection, "GeometryGroup" )
    << new StaticFunction( "symDifference", 2, fcnSymDifference, "GeometryGroup" )
    << new StaticFunction( "combine", 2, fcnCombine, "GeometryGroup" )
    << new StaticFunction( "union", 2, fcnCombine, "GeometryGroup" )
    << new StaticFunction( "geomToWKT", -1, fcnGeomToWKT, "GeometryGroup" )
    << new StaticFunction( "geometry", 1, fcnGetGeometry, "GeometryGroup" )
    << new StaticFunction( "transform", 3, fcnTransformGeometry, "GeometryGroup" )
    << new StaticFunction( "$rownum", 0, fcnRowNumber, "Record" )
    << new StaticFunction( "$id", 0, fcnFeatureId, "Record" )
    << new StaticFunction( "$currentfeature", 0, fcnFeature, "Record" )
    << new StaticFunction( "$scale", 0, fcnScale, "Record" )
    << new StaticFunction( "$uuid", 0, fcnUuid, "Record" )
    << new StaticFunction( "getFeature", 3, fcnGetFeature, "Record" )

    //return all attributes string for referencedColumns - this is caught by
    // QgsFeatureRequest::setSubsetOfAttributes and causes all attributes to be fetched by the
    // feature request
    << new StaticFunction( "attribute", 2, fcnAttribute, "Record", QString(), false, QStringList( QgsFeatureRequest::AllAttributes ) )

    << new StaticFunction( "_specialcol_", 1, fcnSpecialColumn, "Special" )
    ;
  }
  return gmFunctions;
}

QMap<QString, QVariant> QgsExpression::gmSpecialColumns;
QMap<QString, QString> QgsExpression::gmSpecialColumnGroups;

void QgsExpression::setSpecialColumn( const QString& name, QVariant variant )
{
  int fnIdx = functionIndex( name );
  if ( fnIdx != -1 )
  {
    // function of the same name already exists
    return;
  }
  gmSpecialColumns[ name ] = variant;
}

void QgsExpression::unsetSpecialColumn( const QString& name )
{
  QMap<QString, QVariant>::iterator fit = gmSpecialColumns.find( name );
  if ( fit != gmSpecialColumns.end() )
  {
    gmSpecialColumns.erase( fit );
  }
}

QVariant QgsExpression::specialColumn( const QString& name )
{
  int fnIdx = functionIndex( name );
  if ( fnIdx != -1 )
  {
    // function of the same name already exists
    return QVariant();
  }
  QMap<QString, QVariant>::iterator it = gmSpecialColumns.find( name );
  if ( it == gmSpecialColumns.end() )
  {
    return QVariant();
  }
  return it.value();
}

bool QgsExpression::hasSpecialColumn( const QString& name )
{
  static bool initialized = false;
  if ( !initialized )
  {
    // Pre-register special columns that will exist within QGIS so that expressions that may use them are parsed correctly.
    // This is really sub-optimal, we should get rid of the special columns and instead have contexts in which some values
    // are defined and some are not ($rownum makes sense only in field calculator, $scale only when rendering, $page only for composer etc.)

    //pairs of column name to group name
    QList< QPair<QString, QString> > lst;
    lst << qMakePair( QString( "$page" ), QString( "Composer" ) );
    lst << qMakePair( QString( "$feature" ), QString( "Atlas" ) );
    lst << qMakePair( QString( "$numpages" ), QString( "Composer" ) );
    lst << qMakePair( QString( "$numfeatures" ), QString( "Atlas" ) );
    lst << qMakePair( QString( "$atlasfeatureid" ), QString( "Atlas" ) );
    lst << qMakePair( QString( "$atlasgeometry" ), QString( "Atlas" ) );
    lst << qMakePair( QString( "$atlasfeature" ), QString( "Atlas" ) );
    lst << qMakePair( QString( "$map" ), QString( "Composer" ) );

    QList< QPair<QString, QString> >::const_iterator it = lst.constBegin();
    for ( ; it != lst.constEnd(); ++it )
    {
      setSpecialColumn(( *it ).first, QVariant() );
      gmSpecialColumnGroups[( *it ).first ] = ( *it ).second;
    }

    initialized = true;
  }

  if ( functionIndex( name ) != -1 )
    return false;
  return gmSpecialColumns.contains( name );
}

bool QgsExpression::isValid( const QString &text, const QgsFields &fields, QString &errorMessage )
{
  QgsExpression exp( text );
  exp.prepare( fields );
  errorMessage = exp.parserErrorString();
  return !exp.hasParserError();
}

QList<QgsExpression::Function*> QgsExpression::specialColumns()
{
  QList<Function*> defs;
  for ( QMap<QString, QVariant>::const_iterator it = gmSpecialColumns.begin(); it != gmSpecialColumns.end(); ++it )
  {
    //check for special column group name
    QString group = gmSpecialColumnGroups.value( it.key(), "Record" );
    defs << new StaticFunction( it.key(), 0, 0, group );
  }
  return defs;
}

QString QgsExpression::quotedColumnRef( QString name )
{
  return QString( "\"%1\"" ).arg( name.replace( "\"", "\"\"" ) );
}

QString QgsExpression::quotedString( QString text )
{
  text.replace( "'", "''" );
  text.replace( '\\', "\\\\" );
  text.replace( '\n', "\\n" );
  text.replace( '\t', "\\t" );
  return QString( "'%1'" ).arg( text );
}

bool QgsExpression::isFunctionName( QString name )
{
  return functionIndex( name ) != -1;
}

int QgsExpression::functionIndex( QString name )
{
  int count = functionCount();
  for ( int i = 0; i < count; i++ )
  {
    if ( QString::compare( name, Functions()[i]->name(), Qt::CaseInsensitive ) == 0 )
      return i;
  }
  return -1;
}

int QgsExpression::functionCount()
{
  return Functions().size();
}


QgsExpression::QgsExpression( const QString& expr )
    : mRowNumber( 0 )
    , mScale( 0 )
    , mExp( expr )
    , mCalc( 0 )
{
  mRootNode = ::parseExpression( expr, mParserErrorString );

  if ( mParserErrorString.isNull() )
    Q_ASSERT( mRootNode );
}

QgsExpression::~QgsExpression()
{
  delete mCalc;
  delete mRootNode;
}

QStringList QgsExpression::referencedColumns() const
{
  if ( !mRootNode )
    return QStringList();

  QStringList columns = mRootNode->referencedColumns();

  // filter out duplicates
  for ( int i = 0; i < columns.count(); i++ )
  {
    QString col = columns.at( i );
    for ( int j = i + 1; j < columns.count(); j++ )
    {
      if ( QString::compare( col, columns[j], Qt::CaseInsensitive ) == 0 )
      {
        // this column is repeated: remove it!
        columns.removeAt( j-- );
      }
    }
  }

  return columns;
}

bool QgsExpression::needsGeometry() const
{
  if ( !mRootNode )
    return false;
  return mRootNode->needsGeometry();
}

void QgsExpression::initGeomCalculator()
{
  if ( mCalc )
    return;

  // Use planimetric as default
  mCalc = new QgsDistanceArea();
  mCalc->setEllipsoidalMode( false );
}

void QgsExpression::setGeomCalculator( const QgsDistanceArea &calc )
{
  delete mCalc;
  mCalc = new QgsDistanceArea( calc );
}

bool QgsExpression::prepare( const QgsFields& fields )
{
  mEvalErrorString = QString();
  if ( !mRootNode )
  {
    mEvalErrorString = QObject::tr( "No root node! Parsing failed?" );
    return false;
  }

  return mRootNode->prepare( this, fields );
}

QVariant QgsExpression::evaluate( const QgsFeature* f )
{
  mEvalErrorString = QString();
  if ( !mRootNode )
  {
    mEvalErrorString = QObject::tr( "No root node! Parsing failed?" );
    return QVariant();
  }

  return mRootNode->eval( this, f );
}

QVariant QgsExpression::evaluate( const QgsFeature* f, const QgsFields& fields )
{
  // first prepare
  bool res = prepare( fields );
  if ( !res )
    return QVariant();

  // then evaluate
  return evaluate( f );
}

QString QgsExpression::dump() const
{
  if ( !mRootNode )
    return QObject::tr( "(no root)" );

  return mRootNode->dump();
}

void QgsExpression::acceptVisitor( QgsExpression::Visitor& v ) const
{
  if ( mRootNode )
    mRootNode->accept( v );
}

QString QgsExpression::replaceExpressionText( const QString &action, const QgsFeature *feat,
    QgsVectorLayer *layer,
    const QMap<QString, QVariant> *substitutionMap, const QgsDistanceArea *distanceArea )
{
  QString expr_action;

  QMap<QString, QVariant> savedValues;
  if ( substitutionMap )
  {
    // variables with a local scope (must be restored after evaluation)
    for ( QMap<QString, QVariant>::const_iterator sit = substitutionMap->begin(); sit != substitutionMap->end(); ++sit )
    {
      QVariant oldValue = QgsExpression::specialColumn( sit.key() );
      if ( !oldValue.isNull() )
        savedValues.insert( sit.key(), oldValue );

      // set the new value
      QgsExpression::setSpecialColumn( sit.key(), sit.value() );
    }
  }

  int index = 0;
  while ( index < action.size() )
  {
    QRegExp rx = QRegExp( "\\[%([^\\]]+)%\\]" );

    int pos = rx.indexIn( action, index );
    if ( pos < 0 )
      break;

    int start = index;
    index = pos + rx.matchedLength();
    QString to_replace = rx.cap( 1 ).trimmed();
    QgsDebugMsg( "Found expression: " + to_replace );

    QgsExpression exp( to_replace );
    if ( exp.hasParserError() )
    {
      QgsDebugMsg( "Expression parser error: " + exp.parserErrorString() );
      expr_action += action.mid( start, index - start );
      continue;
    }

    if ( distanceArea )
    {
      //if QgsDistanceArea specified for area/distance conversion, use it
      exp.setGeomCalculator( *distanceArea );
    }

    QVariant result;
    if ( layer )
    {
      result = exp.evaluate( feat, layer->pendingFields() );
    }
    else
    {
      result = exp.evaluate( feat );
    }
    if ( exp.hasEvalError() )
    {
      QgsDebugMsg( "Expression parser eval error: " + exp.evalErrorString() );
      expr_action += action.mid( start, index - start );
      continue;
    }

    QgsDebugMsg( "Expression result is: " + result.toString() );
    expr_action += action.mid( start, pos - start ) + result.toString();
  }

  expr_action += action.mid( index );

  // restore overwritten local values
  for ( QMap<QString, QVariant>::const_iterator sit = savedValues.begin(); sit != savedValues.end(); ++sit )
  {
    QgsExpression::setSpecialColumn( sit.key(), sit.value() );
  }

  return expr_action;
}

double QgsExpression::evaluateToDouble( const QString &text, const double fallbackValue )
{
  bool ok;
  //first test if text is directly convertible to double
  double convertedValue = text.toDouble( &ok );
  if ( ok )
  {
    return convertedValue;
  }

  //otherwise try to evalute as expression
  QgsExpression expr( text );
  QVariant result = expr.evaluate();
  convertedValue = result.toDouble( &ok );
  if ( expr.hasEvalError() || !ok )
  {
    return fallbackValue;
  }
  return convertedValue;
}


///////////////////////////////////////////////
// nodes

QString QgsExpression::NodeList::dump() const
{
  QString msg; bool first = true;
  foreach ( Node* n, mList )
  {
    if ( !first ) msg += ", "; else first = false;
    msg += n->dump();
  }
  return msg;
}


//

QVariant QgsExpression::NodeUnaryOperator::eval( QgsExpression* parent, const QgsFeature* f )
{
  QVariant val = mOperand->eval( parent, f );
  ENSURE_NO_EVAL_ERROR;

  switch ( mOp )
  {
    case uoNot:
    {
      TVL tvl = getTVLValue( val, parent );
      ENSURE_NO_EVAL_ERROR;
      return tvl2variant( NOT[tvl] );
    }

    case uoMinus:
      if ( isIntSafe( val ) )
        return QVariant( - getIntValue( val, parent ) );
      else if ( isDoubleSafe( val ) )
        return QVariant( - getDoubleValue( val, parent ) );
      else
        SET_EVAL_ERROR( QObject::tr( "Unary minus only for numeric values." ) );
      break;
    default:
      Q_ASSERT( 0 && "unknown unary operation" );
  }
  return QVariant();
}

bool QgsExpression::NodeUnaryOperator::prepare( QgsExpression* parent, const QgsFields& fields )
{
  return mOperand->prepare( parent, fields );
}

QString QgsExpression::NodeUnaryOperator::dump() const
{
  return QString( "%1 %2" ).arg( UnaryOperatorText[mOp] ).arg( mOperand->dump() );
}

//

QVariant QgsExpression::NodeBinaryOperator::eval( QgsExpression* parent, const QgsFeature* f )
{
  QVariant vL = mOpLeft->eval( parent, f );
  ENSURE_NO_EVAL_ERROR;
  QVariant vR = mOpRight->eval( parent, f );
  ENSURE_NO_EVAL_ERROR;

  switch ( mOp )
  {
    case boPlus:
      if ( vL.type() == QVariant::String && vR.type() == QVariant::String )
      {
        QString sL = getStringValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        QString sR = getStringValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        return QVariant( sL + sR );
      }
    case boMinus:
    case boMul:
    case boDiv:
    case boMod:
    {
      if ( isNull( vL ) || isNull( vR ) )
        return QVariant();
      else if ( mOp != boDiv && isIntSafe( vL ) && isIntSafe( vR ) )
      {
        // both are integers - let's use integer arithmetics
        int iL = getIntValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        int iR = getIntValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        return QVariant( computeInt( iL, iR ) );
      }
      else if ( isDateTimeSafe( vL ) && isIntervalSafe( vR ) )
      {
        QDateTime dL = getDateTimeValue( vL, parent );  ENSURE_NO_EVAL_ERROR;
        QgsExpression::Interval iL = getInterval( vR, parent ); ENSURE_NO_EVAL_ERROR;
        if ( mOp == boDiv || mOp == boMul || mOp == boMod )
        {
          parent->setEvalErrorString( QObject::tr( "Can't preform /, *, or % on DateTime and Interval" ) );
          return QVariant();
        }
        return QVariant( computeDateTimeFromInterval( dL, &iL ) );
      }
      else
      {
        // general floating point arithmetic
        double fL = getDoubleValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        double fR = getDoubleValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        if ( mOp == boDiv && fR == 0 )
          return QVariant(); // silently handle division by zero and return NULL
        return QVariant( computeDouble( fL, fR ) );
      }
    }
    case boIntDiv:
    {
      //integer division
      double fL = getDoubleValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
      double fR = getDoubleValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
      if ( fR == 0 )
        return QVariant(); // silently handle division by zero and return NULL
      return QVariant( qFloor( fL / fR ) );
    }
    case boPow:
      if ( isNull( vL ) || isNull( vR ) )
        return QVariant();
      else
      {
        double fL = getDoubleValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        double fR = getDoubleValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        return QVariant( pow( fL, fR ) );
      }

    case boAnd:
    {
      TVL tvlL = getTVLValue( vL, parent ), tvlR = getTVLValue( vR, parent );
      ENSURE_NO_EVAL_ERROR;
      return tvl2variant( AND[tvlL][tvlR] );
    }

    case boOr:
    {
      TVL tvlL = getTVLValue( vL, parent ), tvlR = getTVLValue( vR, parent );
      ENSURE_NO_EVAL_ERROR;
      return tvl2variant( OR[tvlL][tvlR] );
    }

    case boEQ:
    case boNE:
    case boLT:
    case boGT:
    case boLE:
    case boGE:
      if ( isNull( vL ) || isNull( vR ) )
      {
        return TVL_Unknown;
      }
      else if ( isDoubleSafe( vL ) && isDoubleSafe( vR ) )
      {
        // do numeric comparison if both operators can be converted to numbers
        double fL = getDoubleValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        double fR = getDoubleValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        return compare( fL - fR ) ? TVL_True : TVL_False;
      }
      else
      {
        // do string comparison otherwise
        QString sL = getStringValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        QString sR = getStringValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        int diff = QString::compare( sL, sR );
        return compare( diff ) ? TVL_True : TVL_False;
      }

    case boIs:
    case boIsNot:
      if ( isNull( vL ) && isNull( vR ) ) // both operators null
        return ( mOp == boIs ? TVL_True : TVL_False );
      else if ( isNull( vL ) || isNull( vR ) ) // one operator null
        return ( mOp == boIs ? TVL_False : TVL_True );
      else // both operators non-null
      {
        bool equal = false;
        if ( isDoubleSafe( vL ) && isDoubleSafe( vR ) )
        {
          double fL = getDoubleValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
          double fR = getDoubleValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
          equal = fL == fR;
        }
        else
        {
          QString sL = getStringValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
          QString sR = getStringValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
          equal = QString::compare( sL, sR ) == 0;
        }
        if ( equal )
          return mOp == boIs ? TVL_True : TVL_False;
        else
          return mOp == boIs ? TVL_False : TVL_True;
      }

    case boRegexp:
    case boLike:
    case boNotLike:
    case boILike:
    case boNotILike:
      if ( isNull( vL ) || isNull( vR ) )
        return TVL_Unknown;
      else
      {
        QString str    = getStringValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        QString regexp = getStringValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        // TODO: cache QRegExp in case that regexp is a literal string (i.e. it will stay constant)
        bool matches;
        if ( mOp == boLike || mOp == boILike || mOp == boNotLike || mOp == boNotILike ) // change from LIKE syntax to regexp
        {
          QString esc_regexp = QRegExp::escape( regexp );
          // XXX escape % and _  ???
          esc_regexp.replace( "%", ".*" );
          esc_regexp.replace( "_", "." );
          matches = QRegExp( esc_regexp, mOp == boLike || mOp == boNotLike ? Qt::CaseSensitive : Qt::CaseInsensitive ).exactMatch( str );
        }
        else
        {
          matches = QRegExp( regexp ).indexIn( str ) != -1;
        }

        if ( mOp == boNotLike || mOp == boNotILike )
        {
          matches = !matches;
        }

        return matches ? TVL_True : TVL_False;
      }

    case boConcat:
      if ( isNull( vL ) || isNull( vR ) )
        return QVariant();
      else
      {
        QString sL = getStringValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        QString sR = getStringValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        return QVariant( sL + sR );
      }

    default: break;
  }
  Q_ASSERT( false );
  return QVariant();
}

bool QgsExpression::NodeBinaryOperator::compare( double diff )
{
  switch ( mOp )
  {
    case boEQ: return diff == 0;
    case boNE: return diff != 0;
    case boLT: return diff < 0;
    case boGT: return diff > 0;
    case boLE: return diff <= 0;
    case boGE: return diff >= 0;
    default: Q_ASSERT( false ); return false;
  }
}

int QgsExpression::NodeBinaryOperator::computeInt( int x, int y )
{
  switch ( mOp )
  {
    case boPlus: return x+y;
    case boMinus: return x-y;
    case boMul: return x*y;
    case boDiv: return x/y;
    case boMod: return x%y;
    default: Q_ASSERT( false ); return 0;
  }
}

QDateTime QgsExpression::NodeBinaryOperator::computeDateTimeFromInterval( QDateTime d, QgsExpression::Interval *i )
{
  switch ( mOp )
  {
    case boPlus: return d.addSecs( i->seconds() );
    case boMinus: return d.addSecs( -i->seconds() );
    default: Q_ASSERT( false ); return QDateTime();
  }
}

double QgsExpression::NodeBinaryOperator::computeDouble( double x, double y )
{
  switch ( mOp )
  {
    case boPlus: return x+y;
    case boMinus: return x-y;
    case boMul: return x*y;
    case boDiv: return x/y;
    case boMod: return fmod( x,y );
    default: Q_ASSERT( false ); return 0;
  }
}


bool QgsExpression::NodeBinaryOperator::prepare( QgsExpression* parent, const QgsFields& fields )
{
  bool resL = mOpLeft->prepare( parent, fields );
  bool resR = mOpRight->prepare( parent, fields );
  return resL && resR;
}

int QgsExpression::NodeBinaryOperator::precedence() const
{
  // see left/right in qgsexpressionparser.yy
  switch ( mOp )
  {
    case boOr:
      return 1;

    case boAnd:
      return 2;

    case boEQ:
    case boNE:
    case boLE:
    case boGE:
    case boLT:
    case boGT:
    case boRegexp:
    case boLike:
    case boILike:
    case boNotLike:
    case boNotILike:
    case boIs:
    case boIsNot:
      return 3;

    case boPlus:
    case boMinus:
      return 4;

    case boMul:
    case boDiv:
    case boIntDiv:
    case boMod:
      return 5;

    case boPow:
      return 6;

    case boConcat:
      return 7;
  }
  Q_ASSERT( 0 && "unexpected binary operator" );
  return -1;
}

QString QgsExpression::NodeBinaryOperator::dump() const
{
  QgsExpression::NodeBinaryOperator *lOp = dynamic_cast<QgsExpression::NodeBinaryOperator *>( mOpLeft );
  QgsExpression::NodeBinaryOperator *rOp = dynamic_cast<QgsExpression::NodeBinaryOperator *>( mOpRight );

  QString fmt;
  fmt += lOp && lOp->precedence() < precedence() ? "(%1)" : "%1";
  fmt += " %2 ";
  fmt += rOp && rOp->precedence() <= precedence() ? "(%3)" : "%3";

  return fmt.arg( mOpLeft->dump() ).arg( BinaryOperatorText[mOp] ).arg( mOpRight->dump() );
}

//

QVariant QgsExpression::NodeInOperator::eval( QgsExpression* parent, const QgsFeature* f )
{
  if ( mList->count() == 0 )
    return mNotIn ? TVL_True : TVL_False;
  QVariant v1 = mNode->eval( parent, f );
  ENSURE_NO_EVAL_ERROR;
  if ( isNull( v1 ) )
    return TVL_Unknown;

  bool listHasNull = false;

  foreach ( Node* n, mList->list() )
  {
    QVariant v2 = n->eval( parent, f );
    ENSURE_NO_EVAL_ERROR;
    if ( isNull( v2 ) )
      listHasNull = true;
    else
    {
      bool equal = false;
      // check whether they are equal
      if ( isDoubleSafe( v1 ) && isDoubleSafe( v2 ) )
      {
        double f1 = getDoubleValue( v1, parent ); ENSURE_NO_EVAL_ERROR;
        double f2 = getDoubleValue( v2, parent ); ENSURE_NO_EVAL_ERROR;
        equal = f1 == f2;
      }
      else
      {
        QString s1 = getStringValue( v1, parent ); ENSURE_NO_EVAL_ERROR;
        QString s2 = getStringValue( v2, parent ); ENSURE_NO_EVAL_ERROR;
        equal = QString::compare( s1, s2 ) == 0;
      }

      if ( equal ) // we know the result
        return mNotIn ? TVL_False : TVL_True;
    }
  }

  // item not found
  if ( listHasNull )
    return TVL_Unknown;
  else
    return mNotIn ? TVL_True : TVL_False;
}

bool QgsExpression::NodeInOperator::prepare( QgsExpression* parent, const QgsFields& fields )
{
  bool res = mNode->prepare( parent, fields );
  foreach ( Node* n, mList->list() )
  {
    res = res && n->prepare( parent, fields );
  }
  return res;
}

QString QgsExpression::NodeInOperator::dump() const
{
  return QString( "%1 IN (%2)" ).arg( mNode->dump() ).arg( mList->dump() );
}

//

QVariant QgsExpression::NodeFunction::eval( QgsExpression* parent, const QgsFeature* f )
{
  Function* fd = Functions()[mFnIndex];

  // evaluate arguments
  QVariantList argValues;
  if ( mArgs )
  {
    foreach ( Node* n, mArgs->list() )
    {
      QVariant v;
      if ( fd->lazyEval() )
      {
        // Pass in the node for the function to eval as it needs.
        v = QVariant::fromValue( n );
      }
      else
      {
        v = n->eval( parent, f );
        ENSURE_NO_EVAL_ERROR;
        if ( isNull( v ) && fd->name() != "coalesce" )
          return QVariant(); // all "normal" functions return NULL, when any parameter is NULL (so coalesce is abnormal)
      }
      argValues.append( v );
    }
  }

  // run the function
  QVariant res = fd->func( argValues, f, parent );
  ENSURE_NO_EVAL_ERROR;

  // everything went fine
  return res;
}

bool QgsExpression::NodeFunction::prepare( QgsExpression* parent, const QgsFields& fields )
{
  bool res = true;
  if ( mArgs )
  {
    foreach ( Node* n, mArgs->list() )
    {
      res = res && n->prepare( parent, fields );
    }
  }
  return res;
}

QString QgsExpression::NodeFunction::dump() const
{
  Function* fd = Functions()[mFnIndex];
  if ( fd->params() == 0 )
    return fd->name(); // special column
  else
    return QString( "%1(%2)" ).arg( fd->name() ).arg( mArgs ? mArgs->dump() : QString() ); // function
}

QStringList QgsExpression::NodeFunction::referencedColumns() const
{
  Function* fd = Functions()[mFnIndex];
  QStringList functionColumns = fd->referencedColumns();

  if ( !mArgs )
  {
    //no referenced columns in arguments, just return function's referenced columns
    return functionColumns;
  }

  foreach ( Node* n, mArgs->list() )
  {
    functionColumns.append( n->referencedColumns() );
  }

  //remove duplicates and return
  return functionColumns.toSet().toList();
}

//

QVariant QgsExpression::NodeLiteral::eval( QgsExpression*, const QgsFeature* )
{
  return mValue;
}

bool QgsExpression::NodeLiteral::prepare( QgsExpression* /*parent*/, const QgsFields& /*fields*/ )
{
  return true;
}


QString QgsExpression::NodeLiteral::dump() const
{
  if ( mValue.isNull() )
    return "NULL";

  switch ( mValue.type() )
  {
    case QVariant::Int: return QString::number( mValue.toInt() );
    case QVariant::Double: return QString::number( mValue.toDouble() );
    case QVariant::String: return quotedString( mValue.toString() );
    default: return QObject::tr( "[unsupported type;%1; value:%2]" ).arg( mValue.typeName() ).arg( mValue.toString() );
  }
}

//

QVariant QgsExpression::NodeColumnRef::eval( QgsExpression* /*parent*/, const QgsFeature* f )
{
  if ( f )
  {
    if ( mIndex >= 0 )
      return f->attribute( mIndex );
    else
      return f->attribute( mName );
  }
  return QVariant( "[" + mName + "]" );
}

bool QgsExpression::NodeColumnRef::prepare( QgsExpression* parent, const QgsFields& fields )
{
  for ( int i = 0; i < fields.count(); ++i )
  {
    if ( QString::compare( fields[i].name(), mName, Qt::CaseInsensitive ) == 0 )
    {
      mIndex = i;
      return true;
    }
  }
  parent->mEvalErrorString = QObject::tr( "Column '%1' not found" ).arg( mName );
  mIndex = -1;
  return false;
}

QString QgsExpression::NodeColumnRef::dump() const
{
  return QRegExp( "^[A-Za-z_\x80-\xff][A-Za-z0-9_\x80-\xff]*$" ).exactMatch( mName ) ? mName : quotedColumnRef( mName );
}

//

QVariant QgsExpression::NodeCondition::eval( QgsExpression* parent, const QgsFeature* f )
{
  foreach ( WhenThen* cond, mConditions )
  {
    QVariant vWhen = cond->mWhenExp->eval( parent, f );
    TVL tvl = getTVLValue( vWhen, parent );
    ENSURE_NO_EVAL_ERROR;
    if ( tvl == True )
    {
      QVariant vRes = cond->mThenExp->eval( parent, f );
      ENSURE_NO_EVAL_ERROR;
      return vRes;
    }
  }

  if ( mElseExp )
  {
    QVariant vElse = mElseExp->eval( parent, f );
    ENSURE_NO_EVAL_ERROR;
    return vElse;
  }

  // return NULL if no condition is matching
  return QVariant();
}

bool QgsExpression::NodeCondition::prepare( QgsExpression* parent, const QgsFields& fields )
{
  bool res;
  foreach ( WhenThen* cond, mConditions )
  {
    res = cond->mWhenExp->prepare( parent, fields )
          & cond->mThenExp->prepare( parent, fields );
    if ( !res ) return false;
  }

  if ( mElseExp )
    return mElseExp->prepare( parent, fields );

  return true;
}

QString QgsExpression::NodeCondition::dump() const
{
  QString msg = QString( "CASE" );
  foreach ( WhenThen* cond, mConditions )
  {
    msg += QString( " WHEN %1 THEN %2" ).arg( cond->mWhenExp->dump() ).arg( cond->mThenExp->dump() );
  }
  if ( mElseExp )
    msg += QString( " ELSE %1" ).arg( mElseExp->dump() );
  msg += QString( " END" );
  return msg;
}

QStringList QgsExpression::NodeCondition::referencedColumns() const
{
  QStringList lst;
  foreach ( WhenThen* cond, mConditions )
  {
    lst += cond->mWhenExp->referencedColumns() + cond->mThenExp->referencedColumns();
  }

  if ( mElseExp )
    lst += mElseExp->referencedColumns();

  return lst;
}

bool QgsExpression::NodeCondition::needsGeometry() const
{
  foreach ( WhenThen* cond, mConditions )
  {
    if ( cond->mWhenExp->needsGeometry() ||
         cond->mThenExp->needsGeometry() )
      return true;
  }

  if ( mElseExp && mElseExp->needsGeometry() )
    return true;

  return false;
}

QString QgsExpression::helptext( QString name )
{
  QgsExpression::initFunctionHelp();
  return gFunctionHelpTexts.value( name, QObject::tr( "function help for %1 missing" ).arg( name ) );
}

QHash<QString, QString> QgsExpression::gGroups;

QString QgsExpression::group( QString name )
{
  if ( gGroups.isEmpty() )
  {
    gGroups.insert( "Operators", QObject::tr( "Operators" ) );
    gGroups.insert( "Conditionals", QObject::tr( "Conditionals" ) );
    gGroups.insert( "Fields and Values", QObject::tr( "Fields and Values" ) );
    gGroups.insert( "Math", QObject::tr( "Math" ) );
    gGroups.insert( "Conversions", QObject::tr( "Conversions" ) );
    gGroups.insert( "Date and Time", QObject::tr( "Date and Time" ) );
    gGroups.insert( "String", QObject::tr( "String" ) );
    gGroups.insert( "Color", QObject::tr( "Color" ) );
    gGroups.insert( "GeometryGroup", QObject::tr( "Geometry" ) );
    gGroups.insert( "Record", QObject::tr( "Record" ) );
  }

  //return the translated name for this group. If group does not
  //have a translated name in the gGroups hash, return the name
  //unchanged
  return gGroups.value( name, name );
}
