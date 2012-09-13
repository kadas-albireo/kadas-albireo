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
#include <QSettings>
#include <QDate>
#include <QRegExp>

#include <math.h>
#include <limits>

#include "qgsdistancearea.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"
#include "qgslogger.h"

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
  QRegExp rx( "(\\d?\\.?\\d+\\s+[a-z]+)", Qt::CaseInsensitive );
  QStringList list;
  int pos = 0;

  while (( pos = rx.indexIn( string, pos ) ) != -1 )
  {
    list << rx.cap( 1 );
    pos += rx.matchedLength();
  }

  foreach ( QString match, list )
  {
    QStringList split = match.split( QRegExp( "\\s+" ) );
    bool ok;
    int value = split.at( 0 ).toInt( &ok );
    if ( !ok )
    {
      continue;
    }

    if ( match.contains( "day", Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "day", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "days", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) )
      seconds += value * QgsExpression::Interval::DAY;
    if ( match.contains( "week", Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "week", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "weeks", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) )
      seconds += value * QgsExpression::Interval::WEEKS;
    if ( match.contains( "month", Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "month", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "months", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) )
      seconds += value * QgsExpression::Interval::MONTHS;
    if ( match.contains( "year", Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "year", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "years", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) )
      seconds += value * QgsExpression::Interval::YEARS;
    if ( match.contains( "second", Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "second", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "seconds", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) )
      seconds += value;
    if ( match.contains( "minute", Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "minute", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "minutes", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) )
      seconds += value * QgsExpression::Interval::MINUTE;
    if ( match.contains( "hour", Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "hour", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) ||
         match.contains( QObject::tr( "hours", "Note: Word is part matched in code" ), Qt::CaseInsensitive ) )
      seconds += value * QgsExpression::Interval::HOUR;
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
  if ( v.type() == QVariant::Double ) return false;
  if ( v.type() == QVariant::String ) { bool ok; v.toString().toInt( &ok ); return ok; }
  return false;
}
inline bool isDoubleSafe( const QVariant& v )
{
  if ( v.type() == QVariant::Double || v.type() == QVariant::Int ) return true;
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
  "OR", "AND",
  "=", "<>", "<=", ">=", "<", ">", "~", "LIKE", "NOT LIKE", "ILIKE", "NOT ILIKE", "IS", "IS NOT",
  "+", "-", "*", "/", "%", "^",
  "||"
};

const char* QgsExpression::BinaryOgcOperatorText[] =
{
  "Or", "And",
  "PropertyIsEqualTo", "PropertyIsNotEqualTo",
  "PropertyIsLessThanOrEqualTo", "PropertyIsGreaterThanOrEqualTo",
  "PropertyIsLessThan", "PropertyIsGreaterThan",
  "", "PropertyIsLike", "", "", "",
  "Add", "Sub", "Mul", "Div", "", "",
  ""
};

const char* QgsExpression::UnaryOperatorText[] =
{
  "NOT", "-"
};

const char* QgsExpression::UnaryOgcOperatorText[] =
{
  "Not", ""
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

static QVariant fcnSqrt( const QVariantList& values, QgsFeature* /*f*/, QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( sqrt( x ) );
}
static QVariant fcnSin( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( sin( x ) );
}
static QVariant fcnCos( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( cos( x ) );
}
static QVariant fcnTan( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( tan( x ) );
}
static QVariant fcnAsin( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( asin( x ) );
}
static QVariant fcnAcos( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( acos( x ) );
}
static QVariant fcnAtan( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( atan( x ) );
}
static QVariant fcnAtan2( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double y = getDoubleValue( values.at( 0 ), parent );
  double x = getDoubleValue( values.at( 1 ), parent );
  return QVariant( atan2( y, x ) );
}
static QVariant fcnExp( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  return QVariant( exp( x ) );
}
static QVariant fcnLn( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  if ( x <= 0 )
    return QVariant();
  return QVariant( log( x ) );
}
static QVariant fcnLog10( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double x = getDoubleValue( values.at( 0 ), parent );
  if ( x <= 0 )
    return QVariant();
  return QVariant( log10( x ) );
}
static QVariant fcnLog( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  double b = getDoubleValue( values.at( 0 ), parent );
  double x = getDoubleValue( values.at( 1 ), parent );
  if ( x <= 0 || b <= 0 )
    return QVariant();
  return QVariant( log( x ) / log( b ) );
}
static QVariant fcnToInt( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  return QVariant( getIntValue( values.at( 0 ), parent ) );
}
static QVariant fcnToReal( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  return QVariant( getDoubleValue( values.at( 0 ), parent ) );
}
static QVariant fcnToString( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  return QVariant( getStringValue( values.at( 0 ), parent ) );
}

static QVariant fcnToDateTime( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  return QVariant( getDateTimeValue( values.at( 0 ), parent ) );
}

static QVariant fcnCoalesce( const QVariantList& values, QgsFeature* , QgsExpression* )
{
  foreach ( const QVariant &value, values )
  {
    if ( value.isNull() )
      continue;
    return value;
  }
  return QVariant();
}
static QVariant fcnLower( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  return QVariant( str.toLower() );
}
static QVariant fcnUpper( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  return QVariant( str.toUpper() );
}
static QVariant fcnTitle( const QVariantList& values, QgsFeature* , QgsExpression* parent )
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
static QVariant fcnLength( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  return QVariant( str.length() );
}
static QVariant fcnReplace( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  QString before = getStringValue( values.at( 1 ), parent );
  QString after = getStringValue( values.at( 2 ), parent );
  return QVariant( str.replace( before, after ) );
}
static QVariant fcnRegexpReplace( const QVariantList& values, QgsFeature* , QgsExpression* parent )
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
static QVariant fcnSubstr( const QVariantList& values, QgsFeature* , QgsExpression* parent )
{
  QString str = getStringValue( values.at( 0 ), parent );
  int from = getIntValue( values.at( 1 ), parent );
  int len = getIntValue( values.at( 2 ), parent );
  return QVariant( str.mid( from -1, len ) );
}

static QVariant fcnRowNumber( const QVariantList& , QgsFeature* , QgsExpression* parent )
{
  return QVariant( parent->currentRowNumber() );
}

static QVariant fcnFeatureId( const QVariantList& , QgsFeature* f, QgsExpression* )
{
  // TODO: handling of 64-bit feature ids?
  return f ? QVariant(( int )f->id() ) : QVariant();
}

static QVariant fcnConcat( const QVariantList& values, QgsFeature* , QgsExpression *parent )
{
  QString concat;
  foreach ( const QVariant &value, values )
  {
    concat += getStringValue( value, parent );
  }
  return concat;
}

static QVariant fcnStrpos( const QVariantList& values, QgsFeature* , QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  return string.indexOf( QRegExp( getStringValue( values.at( 1 ), parent ) ) );
}

static QVariant fcnRight( const QVariantList& values, QgsFeature* , QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  int pos = getIntValue( values.at( 1 ), parent );
  return string.right( pos );
}

static QVariant fcnLeft( const QVariantList& values, QgsFeature* , QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  int pos = getIntValue( values.at( 1 ), parent );
  return string.left( pos );
}

static QVariant fcnRPad( const QVariantList& values, QgsFeature* , QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  int length = getIntValue( values.at( 1 ), parent );
  QString fill = getStringValue( values.at( 2 ), parent );
  return string.rightJustified( length, fill.at( 0 ), true );
}

static QVariant fcnLPad( const QVariantList& values, QgsFeature* , QgsExpression *parent )
{
  QString string = getStringValue( values.at( 0 ), parent );
  int length = getIntValue( values.at( 1 ), parent );
  QString fill = getStringValue( values.at( 2 ), parent );
  return string.leftJustified( length, fill.at( 0 ), true );
}

static QVariant fcnNow( const QVariantList&, QgsFeature* , QgsExpression * )
{
  return QVariant( QDateTime::currentDateTime() );
}

static QVariant fcnToDate( const QVariantList& values, QgsFeature* , QgsExpression * parent )
{
  return QVariant( getDateValue( values.at( 0 ), parent ) );
}

static QVariant fcnToTime( const QVariantList& values, QgsFeature* , QgsExpression * parent )
{
  return QVariant( getTimeValue( values.at( 0 ), parent ) );
}

static QVariant fcnToInterval( const QVariantList& values, QgsFeature* , QgsExpression * parent )
{
  return QVariant::fromValue( getInterval( values.at( 0 ), parent ) );
}

static QVariant fcnAge( const QVariantList& values, QgsFeature* , QgsExpression *parent )
{
  QDateTime d1 = getDateTimeValue( values.at( 0 ), parent );
  QDateTime d2 = getDateTimeValue( values.at( 1 ), parent );
  int seconds = d2.secsTo( d1 );
  return QVariant::fromValue( QgsExpression::Interval( seconds ) );
}

static QVariant fcnDay( const QVariantList& values, QgsFeature* , QgsExpression *parent )
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

static QVariant fcnYear( const QVariantList& values, QgsFeature* , QgsExpression *parent )
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

static QVariant fcnMonth( const QVariantList& values, QgsFeature* , QgsExpression *parent )
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

static QVariant fcnWeek( const QVariantList& values, QgsFeature* , QgsExpression *parent )
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

static QVariant fcnHour( const QVariantList& values, QgsFeature* , QgsExpression *parent )
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

static QVariant fcnMinute( const QVariantList& values, QgsFeature* , QgsExpression *parent )
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

static QVariant fcnSeconds( const QVariantList& values, QgsFeature* , QgsExpression *parent )
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


static QVariant fcnX( const QVariantList& , QgsFeature* f, QgsExpression* )
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
static QVariant fcnY( const QVariantList& , QgsFeature* f, QgsExpression* )
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

static QVariant pointAt( const QVariantList& values, QgsFeature* f, QgsExpression* parent ) // helper function
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

static QVariant fcnXat( const QVariantList& values, QgsFeature* f, QgsExpression* parent )
{
  QVariant v = pointAt( values, f, parent );
  if ( v.type() == QVariant::PointF )
    return QVariant( v.toPointF().x() );
  else
    return QVariant();
}
static QVariant fcnYat( const QVariantList& values, QgsFeature* f, QgsExpression* parent )
{
  QVariant v = pointAt( values, f, parent );
  if ( v.type() == QVariant::PointF )
    return QVariant( v.toPointF().y() );
  else
    return QVariant();
}

static QVariant fcnGeomArea( const QVariantList& , QgsFeature* f, QgsExpression* parent )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Polygon );
  QgsDistanceArea* calc = parent->geomCalculator();
  return QVariant( calc->measure( f->geometry() ) );
}
static QVariant fcnGeomLength( const QVariantList& , QgsFeature* f, QgsExpression* parent )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Line );
  QgsDistanceArea* calc = parent->geomCalculator();
  return QVariant( calc->measure( f->geometry() ) );
}
static QVariant fcnGeomPerimeter( const QVariantList& , QgsFeature* f, QgsExpression* parent )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Polygon );
  QgsDistanceArea* calc = parent->geomCalculator();
  return QVariant( calc->measurePerimeter( f->geometry() ) );
}

static QVariant fcnRound( const QVariantList& values , QgsFeature *f, QgsExpression* parent )
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

static QVariant fcnScale( const QVariantList&, QgsFeature*, QgsExpression* parent )
{
  return QVariant( parent->scale() );
}

static QVariant fcnFormatNumber( const QVariantList& values, QgsFeature*, QgsExpression* parent )
{
  double value = getDoubleValue( values.at( 0 ), parent );
  int places = getIntValue( values.at( 1 ), parent );
  return QString( "%L1" ).arg( value, 0, 'f', places );
}

QList<QgsExpression::FunctionDef> QgsExpression::gmBuiltinFunctions;

const QList<QgsExpression::FunctionDef> &QgsExpression::BuiltinFunctions()
{
  if ( gmBuiltinFunctions.isEmpty() )
  {
    // math
    gmBuiltinFunctions
    << FunctionDef( "sqrt", 1, fcnSqrt, QObject::tr( "Math" ) )
    << FunctionDef( "sin", 1, fcnSin, QObject::tr( "Math" ) )
    << FunctionDef( "cos", 1, fcnCos, QObject::tr( "Math" ) )
    << FunctionDef( "tan", 1, fcnTan, QObject::tr( "Math" ) )
    << FunctionDef( "asin", 1, fcnAsin, QObject::tr( "Math" ) )
    << FunctionDef( "acos", 1, fcnAcos, QObject::tr( "Math" ) )
    << FunctionDef( "atan", 1, fcnAtan, QObject::tr( "Math" ) )
    << FunctionDef( "atan2", 2, fcnAtan2, QObject::tr( "Math" ) )
    << FunctionDef( "exp", 1, fcnExp, QObject::tr( "Math" ) )
    << FunctionDef( "ln", 1, fcnLn, QObject::tr( "Math" ) )
    << FunctionDef( "log10", 1, fcnLog10, QObject::tr( "Math" ) )
    << FunctionDef( "log", 2, fcnLog, QObject::tr( "Math" ) )
    << FunctionDef( "round", -1, fcnRound, QObject::tr( "Math" ) )
    // casts
    << FunctionDef( "toint", 1, fcnToInt, QObject::tr( "Conversions" ) )
    << FunctionDef( "toreal", 1, fcnToReal, QObject::tr( "Conversions" ) )
    << FunctionDef( "tostring", 1, fcnToString, QObject::tr( "Conversions" ) )
    << FunctionDef( "todatetime", 1, fcnToDateTime, QObject::tr( "Conversions" ) )
    << FunctionDef( "todate", 1, fcnToDate, QObject::tr( "Conversions" ) )
    << FunctionDef( "totime", 1, fcnToTime, QObject::tr( "Conversions" ) )
    << FunctionDef( "tointerval", 1, fcnToInterval, QObject::tr( "Conversions" ) )
    // conditionals
    << FunctionDef( "coalesce", -1, fcnCoalesce, QObject::tr( "Conditionals" ) )
    // Date and Time
    << FunctionDef( "$now", 0, fcnNow, QObject::tr( "Date and Time" ) )
    << FunctionDef( "age", 2, fcnAge, QObject::tr( "Date and Time" ) )
    << FunctionDef( "year", 1, fcnYear, QObject::tr( "Date and Time" ) )
    << FunctionDef( "month", 1, fcnMonth, QObject::tr( "Date and Time" ) )
    << FunctionDef( "week", 1, fcnWeek, QObject::tr( "Date and Time" ) )
    << FunctionDef( "day", 1, fcnDay, QObject::tr( "Date and Time" ) )
    << FunctionDef( "hour", 1, fcnHour, QObject::tr( "Date and Time" ) )
    << FunctionDef( "minute", 1, fcnMinute, QObject::tr( "Date and Time" ) )
    << FunctionDef( "second", 1, fcnSeconds, QObject::tr( "Date and Time" ) )
    // string manipulation
    << FunctionDef( "lower", 1, fcnLower, QObject::tr( "String" ) )
    << FunctionDef( "upper", 1, fcnUpper, QObject::tr( "String" ) )
    << FunctionDef( "title", 1, fcnTitle, QObject::tr( "String" ) )
    << FunctionDef( "length", 1, fcnLength, QObject::tr( "String" ) )
    << FunctionDef( "replace", 3, fcnReplace, QObject::tr( "String" ) )
    << FunctionDef( "regexp_replace", 3, fcnRegexpReplace, QObject::tr( "String" ) )
    << FunctionDef( "substr", 3, fcnSubstr, QObject::tr( "String" ) )
    << FunctionDef( "concat", -1, fcnConcat, QObject::tr( "String" ) )
    << FunctionDef( "strpos", 2, fcnStrpos, QObject::tr( "String" ) )
    << FunctionDef( "left", 2, fcnLeft, QObject::tr( "String" ) )
    << FunctionDef( "right", 2, fcnRight, QObject::tr( "String" ) )
    << FunctionDef( "rpad", 3, fcnRPad, QObject::tr( "String" ) )
    << FunctionDef( "lpad", 3, fcnLPad, QObject::tr( "String" ) )
    << FunctionDef( "format_number", 2, fcnFormatNumber, QObject::tr( "String" ) )

    // geometry accessors
    << FunctionDef( "xat", 1, fcnXat, QObject::tr( "Geometry" ), "", true )
    << FunctionDef( "yat", 1, fcnYat, QObject::tr( "Geometry" ), "", true )
    << FunctionDef( "$area", 0, fcnGeomArea, QObject::tr( "Geometry" ), "", true )
    << FunctionDef( "$length", 0, fcnGeomLength, QObject::tr( "Geometry" ), "", true )
    << FunctionDef( "$perimeter", 0, fcnGeomPerimeter, QObject::tr( "Geometry" ), "", true )
    << FunctionDef( "$x", 0, fcnX, QObject::tr( "Geometry" ), "", true )
    << FunctionDef( "$y", 0, fcnY, QObject::tr( "Geometry" ), "" , true )
    // special columns
    << FunctionDef( "$rownum", 0, fcnRowNumber, QObject::tr( "Record" ) )
    << FunctionDef( "$id", 0, fcnFeatureId, QObject::tr( "Record" ) )
    << FunctionDef( "$scale", 0, fcnScale, QObject::tr( "Record" ) )
    ;
  }

  return gmBuiltinFunctions;
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
    if ( QString::compare( name, BuiltinFunctions()[i].mName, Qt::CaseInsensitive ) == 0 )
      return i;
  }
  return -1;
}

int QgsExpression::functionCount()
{
  return BuiltinFunctions().size();
}


QgsExpression::QgsExpression( const QString& expr )
    : mExpression( expr )
    , mRowNumber( 0 )
    , mScale( 0 )
    , mCalc( NULL )
{
  mRootNode = ::parseExpression( mExpression, mParserErrorString );

  if ( mParserErrorString.isNull() )
  {
    Q_ASSERT( mRootNode != NULL );
  }
}

QgsExpression::~QgsExpression()
{
  delete mRootNode;
  delete mCalc;
}

QStringList QgsExpression::referencedColumns()
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

bool QgsExpression::needsGeometry()
{
  if ( !mRootNode )
    return false;
  return mRootNode->needsGeometry();
}

void QgsExpression::initGeomCalculator()
{
  mCalc = new QgsDistanceArea;
  QSettings settings;
  QString ellipsoid = settings.value( "/qgis/measure/ellipsoid", GEO_NONE ).toString();
  mCalc->setEllipsoid( ellipsoid );
  mCalc->setEllipsoidalMode( false );
}

bool QgsExpression::prepare( const QgsFieldMap& fields )
{
  mEvalErrorString = QString();
  if ( !mRootNode )
  {
    mEvalErrorString = QObject::tr( "No root node! Parsing failed?" );
    return false;
  }

  return mRootNode->prepare( this, fields );
}

QVariant QgsExpression::evaluate( QgsFeature* f )
{
  mEvalErrorString = QString();
  if ( !mRootNode )
  {
    mEvalErrorString = QObject::tr( "No root node! Parsing failed?" );
    return QVariant();
  }

  return mRootNode->eval( this, f );
}

QVariant QgsExpression::evaluate( QgsFeature* f, const QgsFieldMap& fields )
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


void QgsExpression::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  if ( !mRootNode )
    return;

  mRootNode->toOgcFilter( doc, element );
}

QgsExpression* QgsExpression::createFromOgcFilter( QDomElement &element )
{
  if ( element.isNull() || !element.hasChildNodes() )
    return NULL;

  QgsExpression *expr = new QgsExpression();

  QDomElement childElem = element.firstChildElement();
  while ( !childElem.isNull() )
  {
    QString errorMsg;
    QgsExpression::Node *node = QgsExpression::Node::createFromOgcFilter( childElem, errorMsg );
    if ( !node )
    {
      // invalid expression, parser error
      expr->mParserErrorString = errorMsg;
      return expr;
    }

    // use the concat binary operator to append to the root node
    if ( !expr->mRootNode )
    {
      expr->mRootNode = node;
    }
    else
    {
      expr->mRootNode = new QgsExpression::NodeBinaryOperator( boConcat, expr->mRootNode, node );
    }

    childElem = childElem.nextSiblingElement();
  }

  return expr;
}

void QgsExpression::acceptVisitor( QgsExpression::Visitor& v )
{
  if ( mRootNode )
    mRootNode->accept( v );
}

QString QgsExpression::replaceExpressionText( QString action, QgsFeature &feat,
    QgsVectorLayer* layer,
    const QMap<QString, QVariant> *substitutionMap )
{
  QString expr_action;

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

    if ( substitutionMap && substitutionMap->contains( to_replace ) )
    {
      expr_action += action.mid( start, pos - start ) + substitutionMap->value( to_replace ).toString();
      continue;
    }

    QgsExpression exp( to_replace );
    if ( exp.hasParserError() )
    {
      QgsDebugMsg( "Expression parser error: " + exp.parserErrorString() );
      expr_action += action.mid( start, index - start );
      continue;
    }

    QVariant result = exp.evaluate( &feat, layer->pendingFields() );
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
  return expr_action;
}


QgsExpression::Node* QgsExpression::Node::createFromOgcFilter( QDomElement &element, QString &errorMessage )
{
  if ( element.isNull() )
    return NULL;

  // check for unary operators
  int unaryOpCount = sizeof( UnaryOgcOperatorText ) / sizeof( UnaryOgcOperatorText[0] );
  for ( int i = 0; i < unaryOpCount; i++ )
  {
    QString ogcOperatorName = UnaryOgcOperatorText[ i ];
    if ( ogcOperatorName.isEmpty() )
      continue;

    if ( element.localName() == ogcOperatorName )
    {
      QgsExpression::Node *node = QgsExpression::NodeUnaryOperator::createFromOgcFilter( element, errorMessage );
      if ( node )
        return node;

      return NULL;
    }
  }

  // check for binary operators
  int binaryOpCount = sizeof( BinaryOgcOperatorText ) / sizeof( BinaryOgcOperatorText[0] );
  for ( int i = 0; i < binaryOpCount; i++ )
  {
    QString ogcOperatorName = BinaryOgcOperatorText[ i ];
    if ( ogcOperatorName.isEmpty() )
      continue;

    if ( element.localName() == ogcOperatorName )
    {
      QgsExpression::Node *node = QgsExpression::NodeBinaryOperator::createFromOgcFilter( element, errorMessage );
      if ( node )
        return node;

      return NULL;
    }
  }

  // check for other OGC operators, convert them to expressions

  if ( element.localName() == "PropertyIsNull" )
  {
    return QgsExpression::NodeBinaryOperator::createFromOgcFilter( element, errorMessage );
  }
  else if ( element.localName() == "Literal" )
  {
    return QgsExpression::NodeLiteral::createFromOgcFilter( element, errorMessage );
  }
  else if ( element.localName() == "Function" )
  {
    return QgsExpression::NodeFunction::createFromOgcFilter( element, errorMessage );
  }
  else if ( element.localName() == "PropertyName" )
  {
    return QgsExpression::NodeColumnRef::createFromOgcFilter( element, errorMessage );
  }
  else if ( element.localName() == "PropertyIsBetween" )
  {
    // <ogc:PropertyIsBetween> encode a Range check
    QgsExpression::Node *operand = 0, *lowerBound = 0;
    QgsExpression::Node *operand2 = 0, *upperBound = 0;

    QDomElement operandElem = element.firstChildElement();
    while ( !operandElem.isNull() )
    {
      if ( operandElem.localName() == "LowerBoundary" )
      {
        QDomElement lowerBoundElem = operandElem.firstChildElement();
        lowerBound = createFromOgcFilter( lowerBoundElem, errorMessage );
      }
      else if ( operandElem.localName() ==  "UpperBoundary" )
      {
        QDomElement upperBoundElem = operandElem.firstChildElement();
        upperBound = createFromOgcFilter( upperBoundElem, errorMessage );
      }
      else
      {
        // <ogc:expression>
        // both operand and operand2 contain the same expression,
        // they are respectively compared to lower bound and upper bound
        operand = createFromOgcFilter( operandElem, errorMessage );
        operand2 = createFromOgcFilter( operandElem, errorMessage );
      }

      if ( operand && lowerBound && operand2 && upperBound )
        break;

      operandElem = operandElem.nextSiblingElement();
    }

    if ( !operand || !lowerBound || !operand2 || !upperBound )
    {
      if ( operand )
        delete operand;

      if ( lowerBound )
        delete lowerBound;

      if ( upperBound )
        delete upperBound;

      errorMessage = "missing some required sub-elements in ogc:PropertyIsBetween";
      return NULL;
    }

    QgsExpression::Node *geOperator = new QgsExpression::NodeBinaryOperator( boGE, operand, lowerBound );
    QgsExpression::Node *leOperator = new QgsExpression::NodeBinaryOperator( boLE, operand2, upperBound );
    return new QgsExpression::NodeBinaryOperator( boAnd, geOperator, leOperator );
  }

  errorMessage += QString( "unable to convert '%1' element to a valid expression: it is not supported yet or it has invalid arguments" ).arg( element.tagName() );
  return NULL;
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

void QgsExpression::NodeList::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  foreach ( Node* n, mList )
  {
    n->toOgcFilter( doc, element );
  }
}

//

QVariant QgsExpression::NodeUnaryOperator::eval( QgsExpression* parent, QgsFeature* f )
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

bool QgsExpression::NodeUnaryOperator::prepare( QgsExpression* parent, const QgsFieldMap& fields )
{
  return mOperand->prepare( parent, fields );
}

QString QgsExpression::NodeUnaryOperator::dump() const
{
  return QString( "%1 %2" ).arg( UnaryOperatorText[mOp] ).arg( mOperand->dump() );
}

void QgsExpression::NodeUnaryOperator::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  QDomElement uoElem;
  switch ( mOp )
  {
    case uoMinus:
      uoElem = doc.createElement( "ogc:Literal" );
      uoElem.appendChild( doc.createTextNode( "-" ) );
      break;
    case uoNot:
      uoElem = doc.createElement( "ogc:Not" );
      break;

    default:
      element.appendChild( doc.createComment( QString( "Unary operator %1 not implemented yet" ).arg( UnaryOperatorText[mOp] ) ) );
      return;
  }
  mOperand->toOgcFilter( doc, uoElem );
  element.appendChild( uoElem );
}

QgsExpression::Node* QgsExpression::NodeUnaryOperator::createFromOgcFilter( QDomElement &element, QString &errorMessage )
{
  if ( element.isNull() )
    return NULL;

  int unaryOpCount = sizeof( UnaryOgcOperatorText ) / sizeof( UnaryOgcOperatorText[0] );
  for ( int i = 0; i < unaryOpCount; i++ )
  {
    QString ogcOperatorName = UnaryOgcOperatorText[ i ];
    if ( ogcOperatorName.isEmpty() )
      continue;

    if ( element.localName() != ogcOperatorName )
      continue;

    QDomElement operandElem = element.firstChildElement();
    QgsExpression::Node* operand = QgsExpression::Node::createFromOgcFilter( operandElem, errorMessage );
    if ( !operand )
    {
      if ( errorMessage.isEmpty() )
        errorMessage = QString( "invalid operand for '%1' unary operator" ).arg( ogcOperatorName );
      return NULL;
    }

    return new QgsExpression::NodeUnaryOperator(( UnaryOperator ) i, operand );
  }

  errorMessage = QString( "%1 unary operator not supported." ).arg( element.tagName() );
  return NULL;
}

//

QVariant QgsExpression::NodeBinaryOperator::eval( QgsExpression* parent, QgsFeature* f )
{
  QVariant vL = mOpLeft->eval( parent, f );
  ENSURE_NO_EVAL_ERROR;
  QVariant vR = mOpRight->eval( parent, f );
  ENSURE_NO_EVAL_ERROR;

  switch ( mOp )
  {
    case boPlus:
    case boMinus:
    case boMul:
    case boDiv:
    case boMod:
      if ( isNull( vL ) || isNull( vR ) )
        return QVariant();
      else if ( isIntSafe( vL ) && isIntSafe( vR ) )
      {
        // both are integers - let's use integer arithmetics
        int iL = getIntValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        int iR = getIntValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        if ( mOp == boDiv && iR == 0 ) return QVariant(); // silently handle division by zero and return NULL
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
          // XXX escape % and _  ???
          regexp.replace( "%", ".*" );
          regexp.replace( "_", "." );
          matches = QRegExp( regexp, mOp == boLike || mOp == boNotLike ? Qt::CaseSensitive : Qt::CaseInsensitive ).exactMatch( str );
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


bool QgsExpression::NodeBinaryOperator::prepare( QgsExpression* parent, const QgsFieldMap& fields )
{
  bool resL = mOpLeft->prepare( parent, fields );
  bool resR = mOpRight->prepare( parent, fields );
  return resL && resR;
}

QString QgsExpression::NodeBinaryOperator::dump() const
{
  return QString( "%1 %2 %3" ).arg( mOpLeft->dump() ).arg( BinaryOperatorText[mOp] ).arg( mOpRight->dump() );
}

void QgsExpression::NodeBinaryOperator::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  if ( mOp == boConcat )
  {
    // the concat binary operator must only convert its operands
    mOpLeft->toOgcFilter( doc, element );
    mOpRight->toOgcFilter( doc, element );
    return;
  }

  if ( mOp == boIs || mOp == boIsNot )
  {
    // check if one of the operands is NULL
    QgsExpression::NodeLiteral *opLeftLiteral = dynamic_cast<QgsExpression::NodeLiteral *>( mOpLeft );
    QgsExpression::NodeLiteral *opRightLiteral = dynamic_cast<QgsExpression::NodeLiteral *>( mOpRight );

    if ( opLeftLiteral && opLeftLiteral->value().isNull() &&
         opRightLiteral && opRightLiteral->value().isNull() )
    {
      // why could anybody find useful to use NULL IS NULL???
      // BTW avoid issues by converting it to 1 = 1
      QDomElement eqElem = doc.createElement( "ogc:PropertyIsEqual" );

      QDomElement literalElem = doc.createElement( "ogc:Literal" );
      literalElem.appendChild( doc.createTextNode( "1" ) );
      eqElem.appendChild( literalElem );

      literalElem = doc.createElement( "ogc:Literal" );
      literalElem.appendChild( doc.createTextNode( "1" ) );
      eqElem.appendChild( literalElem );

      element.appendChild( eqElem );
    }
    else if (( opLeftLiteral && opLeftLiteral->value().isNull() ) ||
             ( opRightLiteral && opRightLiteral->value().isNull() ) )
    {
      // at least one operand is NULL, use <ogc:PropertyIsNull> element
      QDomElement isNullElem = doc.createElement( "ogc:PropertyIsNull" );
      QgsExpression::Node *operand = opLeftLiteral->value().isNull() ? mOpRight : mOpLeft;
      operand->toOgcFilter( doc, isNullElem );

      if ( mOp == boIsNot )
      {
        // append to <ogc:Not> element if IS NOT operator was required
        QDomElement notOpElem = doc.createElement( "ogc:Not" );
        notOpElem.appendChild( isNullElem );
        element.appendChild( notOpElem );
      }
      else
      {
        element.appendChild( isNullElem );
      }
    }
    else
    {
      // both operands are not null, use <ogc:PropertyIsEqual> element
      QDomElement eqElem = doc.createElement( "ogc:PropertyIsEqual" );
      mOpLeft->toOgcFilter( doc, eqElem );
      mOpRight->toOgcFilter( doc, eqElem );
      element.appendChild( eqElem );
    }
    return;
  }

  if ( mOp == boILike )
  {
    // XXX why ogc:PropertyIsLikeType extends ogc:ComparisonOpsType
    // which has no matchCase attribute? Shouldn't it be better if
    // would extend BinaryComparisonOpType which has that attribute
    // and doesn't require to have a ogc:PropertyName as first parameter?
    QgsExpression ilikeExpr( QString( "upper( %1 ) LIKE upper( %2 )" ).arg( mOpLeft->dump() ).arg( mOpRight->dump() ) );
    ilikeExpr.toOgcFilter( doc, element );
    return;
  }

  QString opText = BinaryOgcOperatorText[mOp];
  if ( opText.isEmpty() )
  {
    // not implemented binary operators
    // TODO: regex, % (mod), ^ (pow) are not supported yet
    element.appendChild( doc.createComment( QString( "Binary operator %1 not implemented yet" ).arg( BinaryOperatorText[mOp] ) ) );
    return;
  }

  QDomElement boElem = doc.createElement( "ogc:" + opText );
  if ( mOp == boLike )
  {
    // setup wildcards to <ogc:PropertyIsLike>
    boElem.setAttribute( "wildCard", "%" );
    boElem.setAttribute( "singleChar", "?" );
    boElem.setAttribute( "escapeChar", "!" );
  }

  mOpLeft->toOgcFilter( doc, boElem );
  mOpRight->toOgcFilter( doc, boElem );
  element.appendChild( boElem );
}

QgsExpression::Node* QgsExpression::NodeBinaryOperator::createFromOgcFilter( QDomElement &element, QString &errorMessage )
{
  if ( element.isNull() )
    return NULL;

  QgsExpression::Node* opLeft = 0;
  QgsExpression::Node* opRight = 0;

  // convert ogc:PropertyIsNull to IS operator with NULL right operand
  if ( element.localName() == "PropertyIsNull" )
  {
    QDomElement operandElem = element.firstChildElement();
    opLeft = QgsExpression::Node::createFromOgcFilter( operandElem, errorMessage );
    if ( !opLeft )
      return NULL;

    opRight = new QgsExpression::NodeLiteral( QVariant() );
    return new QgsExpression::NodeBinaryOperator( boIs, opLeft, opRight );
  }

  // the other binary operators
  int binaryOpCount = sizeof( BinaryOgcOperatorText ) / sizeof( BinaryOgcOperatorText[0] );
  for ( int i = 0; i < binaryOpCount; i++ )
  {
    QString ogcOperatorName = BinaryOgcOperatorText[ i ];
    if ( ogcOperatorName.isEmpty() )
      continue;

    if ( element.localName() != ogcOperatorName )
      continue;

    QDomElement operandElem = element.firstChildElement();
    opLeft = QgsExpression::Node::createFromOgcFilter( operandElem, errorMessage );
    if ( !opLeft )
    {
      if ( errorMessage.isEmpty() )
        errorMessage = QString( "invalid left operand for '%1' binary operator" ).arg( ogcOperatorName );
      break;
    }

    operandElem = operandElem.nextSiblingElement();
    opRight = QgsExpression::Node::createFromOgcFilter( operandElem, errorMessage );
    if ( !opRight )
    {
      if ( errorMessage.isEmpty() )
        errorMessage = QString( "invalid right operand for '%1' binary operator" ).arg( ogcOperatorName );
      break;
    }

    return new QgsExpression::NodeBinaryOperator(( BinaryOperator ) i, opLeft, opRight );
  }

  if ( !opLeft && !opRight )
  {
    errorMessage = QString( "'%1' binary operator not supported." ).arg( element.tagName() );
    return NULL;
  }

  if ( opLeft )
    delete opLeft;

  if ( opRight )
    delete opRight;

  return NULL;
}

//

QVariant QgsExpression::NodeInOperator::eval( QgsExpression* parent, QgsFeature* f )
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

bool QgsExpression::NodeInOperator::prepare( QgsExpression* parent, const QgsFieldMap& fields )
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

void QgsExpression::NodeInOperator::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  // XXX use a function instead of multiple comparations?

  QDomElement *parent = &element;

  QDomElement orElem;
  if ( mList->list().size() > 1 )
  {
    orElem = doc.createElement( "ogc:Or" );
    element.appendChild( orElem );

    parent = &orElem;
  }

  foreach ( Node* n, mList->list() )
  {
    QDomElement eqElem = doc.createElement( "ogc:PropertyIsEqualTo" );
    mNode->toOgcFilter( doc, eqElem );
    n->toOgcFilter( doc, eqElem );

    parent->appendChild( eqElem );
  }
}

//

QVariant QgsExpression::NodeFunction::eval( QgsExpression* parent, QgsFeature* f )
{
  const FunctionDef& fd = BuiltinFunctions()[mFnIndex];

  // evaluate arguments
  QVariantList argValues;
  if ( mArgs )
  {
    foreach ( Node* n, mArgs->list() )
    {
      QVariant v = n->eval( parent, f );
      ENSURE_NO_EVAL_ERROR;
      if ( isNull( v ) && fd.mFcn != fcnCoalesce )
        return QVariant(); // all "normal" functions return NULL, when any parameter is NULL (so coalesce is abnormal)
      argValues.append( v );
    }
  }

  // run the function
  QVariant res = fd.mFcn( argValues, f, parent );
  ENSURE_NO_EVAL_ERROR;

  // everything went fine
  return res;
}

bool QgsExpression::NodeFunction::prepare( QgsExpression* parent, const QgsFieldMap& fields )
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
  const FunctionDef& fd = BuiltinFunctions()[mFnIndex];
  if ( fd.mParams == 0 )
    return fd.mName; // special column
  else
    return QString( "%1(%2)" ).arg( fd.mName ).arg( mArgs ? mArgs->dump() : QString() ); // function
}

void QgsExpression::NodeFunction::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  const FunctionDef& fd = BuiltinFunctions()[mFnIndex];
  if ( fd.mParams == 0 )
    return; // TODO: special column

  QDomElement funcElem = doc.createElement( "ogc:Function" );
  funcElem.setAttribute( "name", fd.mName );
  mArgs->toOgcFilter( doc, funcElem );
  element.appendChild( funcElem );
}

QgsExpression::Node* QgsExpression::NodeFunction::createFromOgcFilter( QDomElement &element, QString &errorMessage )
{
  if ( element.isNull() )
    return NULL;

  if ( element.localName() != "Function" )
  {
    errorMessage = QString( "ogc:Function expected, got %1" ).arg( element.tagName() );
    return NULL;
  }

  for ( int i = 0; i < BuiltinFunctions().size(); i++ )
  {
    QgsExpression::FunctionDef funcDef = BuiltinFunctions()[i];

    if ( element.attribute( "name" ) != funcDef.mName )
      continue;

    QgsExpression::NodeList *args = new QgsExpression::NodeList();

    QDomElement operandElem = element.firstChildElement();
    while ( !operandElem.isNull() )
    {
      QgsExpression::Node* op = QgsExpression::Node::createFromOgcFilter( operandElem, errorMessage );
      if ( !op )
      {
        delete args;
        return NULL;
      }
      args->append( op );

      operandElem = operandElem.nextSiblingElement();
    }

    return new QgsExpression::NodeFunction( i, args );
  }

  return NULL;
}

//

QVariant QgsExpression::NodeLiteral::eval( QgsExpression* , QgsFeature* )
{
  return mValue;
}

bool QgsExpression::NodeLiteral::prepare( QgsExpression* /*parent*/, const QgsFieldMap& /*fields*/ )
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
    case QVariant::String: return QString( "'%1'" ).arg( mValue.toString() );
    default: return QObject::tr( "[unsupported type;%1; value:%2]" ).arg( mValue.typeName() ).arg( mValue.toString() );
  }
}

void QgsExpression::NodeLiteral::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  QString value;
  if ( !mValue.isNull() )
  {
    switch ( mValue.type() )
    {
      case QVariant::Int:
        value = QString::number( mValue.toInt() );
        break;
      case QVariant::Double:
        value = QString::number( mValue.toDouble() );
        break;
      case QVariant::String:
        value = mValue.toString();
        break;
      default:
        break;
    }
  }
  QDomElement litElem = doc.createElement( "ogc:Literal" );
  litElem.appendChild( doc.createTextNode( value ) );
  element.appendChild( litElem );
}

QgsExpression::Node* QgsExpression::NodeLiteral::createFromOgcFilter( QDomElement &element, QString &errorMessage )
{
  if ( element.isNull() )
    return NULL;

  if ( element.localName() != "Literal" )
  {
    errorMessage = QString( "ogc:Literal expected, got %1" ).arg( element.tagName() );
    return NULL;
  }

  QgsExpression::Node *root = 0;

  // the literal content can have more children (e.g. CDATA section, text, ...)
  QDomNode childNode = element.firstChild();
  while ( !childNode.isNull() )
  {
    QgsExpression::Node* operand = 0;

    if ( childNode.nodeType() == QDomNode::ElementNode )
    {
      // found a element node (e.g. PropertyName), convert it
      QDomElement operandElem = childNode.toElement();
      operand = QgsExpression::Node::createFromOgcFilter( operandElem, errorMessage );
      if ( !operand )
      {
        if ( root )
          delete root;

        errorMessage = QString( "'%1' is an invalid or not supported content for ogc:Literal" ).arg( operandElem.tagName() );
        return NULL;
      }
    }
    else
    {
      // probably a text/CDATA node
      QVariant value = childNode.nodeValue();

      // try to convert the node content to number if possible,
      // otherwise let's use it as string
      bool ok;
      double d = value.toDouble( &ok );
      if ( ok )
        value = d;

      operand = new QgsExpression::NodeLiteral( value );
      if ( !operand )
        continue;
    }

    // use the concat operator to merge the ogc:Literal children
    if ( !root )
    {
      root = operand;
    }
    else
    {
      root = new QgsExpression::NodeBinaryOperator( boConcat, root, operand );
    }

    childNode = childNode.nextSibling();
  }

  if ( root )
    return root;

  return NULL;
}

//

QVariant QgsExpression::NodeColumnRef::eval( QgsExpression* /*parent*/, QgsFeature* f )
{
  return f->attributeMap()[mIndex];
}

bool QgsExpression::NodeColumnRef::prepare( QgsExpression* parent, const QgsFieldMap& fields )
{
  foreach ( int i, fields.keys() )
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
  return mName;
}

void QgsExpression::NodeColumnRef::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  QDomElement propElem = doc.createElement( "ogc:PropertyName" );
  propElem.appendChild( doc.createTextNode( mName ) );
  element.appendChild( propElem );
}

QgsExpression::Node* QgsExpression::NodeColumnRef::createFromOgcFilter( QDomElement &element, QString &errorMessage )
{
  if ( element.isNull() )
    return NULL;

  if ( element.localName() != "PropertyName" )
  {
    errorMessage = QString( "ogc:PropertyName expected, got %1" ).arg( element.tagName() );
    return NULL;
  }

  return new QgsExpression::NodeColumnRef( element.firstChild().nodeValue() );
}

//

QVariant QgsExpression::NodeCondition::eval( QgsExpression* parent, QgsFeature* f )
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

bool QgsExpression::NodeCondition::prepare( QgsExpression* parent, const QgsFieldMap& fields )
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
  QString msg = "CONDITION:\n";
  foreach ( WhenThen* cond, mConditions )
  {
    msg += QString( "- WHEN %1 THEN %2\n" ).arg( cond->mWhenExp->dump() ).arg( cond->mThenExp->dump() );
  }
  if ( mElseExp )
    msg += QString( "- ELSE %1" ).arg( mElseExp->dump() );
  return msg;
}

void QgsExpression::NodeCondition::toOgcFilter( QDomDocument &doc, QDomElement &element ) const
{
  // TODO: if(cond) ... [else if (cond2) ...]* [else ...]
  element.appendChild( doc.createComment( "CASE operator not implemented yet" ) );
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
