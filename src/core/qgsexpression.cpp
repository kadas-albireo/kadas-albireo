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
#include <QSettings>
#include <math.h>

#include "qgsdistancearea.h"
#include "qgsfeature.h"
#include "qgsgeometry.h"

// from parser
extern QgsExpression::Node* parseExpression( const QString& str, QString& parserErrorMsg );


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
  "=", "<>", "<=", ">=", "<", ">", "~", "LIKE", "ILIKE", "IS", "IS NOT",
  "+", "-", "*", "/", "%", "^",
  "||"
};

const char* QgsExpression::UnaryOperatorText[] =
{
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
  int x = value.toInt( &ok );
  if ( !ok )
  {
    parent->setEvalErrorString( QObject::tr( "Cannot convert '%1' to int" ).arg( value.toString() ) );
    return 0;
  }
  return x;
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

#define ENSURE_GEOM_TYPE(f, g, geomtype)   if (!f) return QVariant(); \
  QgsGeometry* g = f->geometry(); \
  if (!g || g->type() != geomtype) return QVariant();


static QVariant fcnX( const QVariantList& , QgsFeature* f, QgsExpression* )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Point );
  return g->asPoint().x();
}
static QVariant fcnY( const QVariantList& , QgsFeature* f, QgsExpression* )
{
  ENSURE_GEOM_TYPE( f, g, QGis::Point );
  return g->asPoint().y();
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

typedef QgsExpression::FunctionDef FnDef;

FnDef QgsExpression::BuiltinFunctions[] =
{
  // math
  FnDef( "sqrt", 1, fcnSqrt, "Math" ),
  FnDef( "sin", 1, fcnSin, "Math" ),
  FnDef( "cos", 1, fcnCos, "Math" ),
  FnDef( "tan", 1, fcnTan, "Math" ),
  FnDef( "asin", 1, fcnAsin, "Math" ),
  FnDef( "acos", 1, fcnAcos, "Math" ),
  FnDef( "atan", 1, fcnAtan, "Math" ),
  FnDef( "atan2", 2, fcnAtan2, "Math" ),
  FnDef( "exp", 1, fcnExp, "Math" ),
  FnDef( "ln", 1, fcnLn, "Math" ),
  FnDef( "log10", 1, fcnLog10, "Math" ),
  FnDef( "log", 2, fcnLog, "Math" ),
  // casts
  FnDef( "toint", 1, fcnToInt, "Conversions" ),
  FnDef( "toreal", 1, fcnToReal, "Conversions" ),
  FnDef( "tostring", 1, fcnToString, "Conversions" ),
  // string manipulation
  FnDef( "lower", 1, fcnLower, "String", "<b>Convert to lower case</b> "
  "<br> Converts a string to lower case letters. "
  "<br> <i>Usage:</i><br>lower('HELLO WORLD') will return 'hello world'" ),
  FnDef( "upper", 1, fcnUpper, "String" , "<b>Convert to upper case</b> "
  "<br> Converts a string to upper case letters. "
  "<br> <i>Usage:</i><br>upper('hello world') will return 'HELLO WORLD'" ),
  FnDef( "length", 1, fcnLength, "String", "<b>Length of string</b> "
  "<br> Returns the legnth of a string. "
  "<br> <i>Usage:</i><br>length('hello') will return 5" ),
  FnDef( "replace", 3, fcnReplace, "String", "<b>Replace a section of a string.</b> " ),
  FnDef( "regexp_replace", 3, fcnRegexpReplace, "String" ),
  FnDef( "substr", 3, fcnSubstr, "String" ),
  // geometry accessors
  FnDef( "xat", 1, fcnXat, "Geometry", "", true ),
  FnDef( "yat", 1, fcnYat, "Geometry", "", true ),
  FnDef( "$area", 0, fcnGeomArea, "Geometry", "", true ),
  FnDef( "$length", 0, fcnGeomLength, "Geometry", "", true ),
  FnDef( "$perimeter", 0, fcnGeomPerimeter, "Geometry", "", true ),
  FnDef( "$x", 0, fcnX, "Geometry", "", true ),
  FnDef( "$y", 0, fcnY, "Geometry", "" , true ),
  // special columns
  FnDef( "$rownum", 0, fcnRowNumber, "Record" ),
  FnDef( "$id", 0, fcnFeatureId, "Record" )
};


bool QgsExpression::isFunctionName( QString name )
{
  return functionIndex( name ) != -1;
}

int QgsExpression::functionIndex( QString name )
{
  int count = functionCount();
  for ( int i = 0; i < count; i++ )
  {
    if ( QString::compare( name, BuiltinFunctions[i].mName, Qt::CaseInsensitive ) == 0 )
      return i;
  }
  return -1;
}

int QgsExpression::functionCount()
{
  return ( sizeof( BuiltinFunctions ) / sizeof( FunctionDef ) );
}


QgsExpression::QgsExpression( const QString& expr )
    : mExpression( expr ), mRowNumber( 0 ), mCalc( NULL )
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
  mCalc->setProjectionsEnabled( false );
  QSettings settings;
  QString ellipsoid = settings.value( "/qgis/measure/ellipsoid", "WGS84" ).toString();
  mCalc->setEllipsoid( ellipsoid );
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
  if ( !mRootNode ) return "(no root)";

  return mRootNode->dump();
}


///////////////////////////////////////////////
// nodes

QString QgsExpression::NodeList::dump() const
{
  QString msg; bool first = true;
  foreach( Node* n, mList )
  {
    if ( !first ) msg += ", "; else first = false;
    msg += n->dump();
  }
  return msg;
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
    case boILike:
      if ( isNull( vL ) || isNull( vR ) )
        return TVL_Unknown;
      else
      {
        QString str    = getStringValue( vL, parent ); ENSURE_NO_EVAL_ERROR;
        QString regexp = getStringValue( vR, parent ); ENSURE_NO_EVAL_ERROR;
        // TODO: cache QRegExp in case that regexp is a literal string (i.e. it will stay constant)
        bool matches;
        if ( mOp == boLike || mOp == boILike ) // change from LIKE syntax to regexp
        {
          // XXX escape % and _  ???
          regexp.replace( "%", ".*" );
          regexp.replace( "_", "." );
          matches = QRegExp( regexp, mOp == boLike ? Qt::CaseSensitive : Qt::CaseInsensitive ).exactMatch( str );
        }
        else
        {
          matches = QRegExp( regexp ).indexIn( str ) != -1;
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

  foreach( Node* n, mList->list() )
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
  foreach( Node* n, mList->list() )
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

QVariant QgsExpression::NodeFunction::eval( QgsExpression* parent, QgsFeature* f )
{
  const FunctionDef& fd = BuiltinFunctions[mFnIndex];

  // evaluate arguments
  QVariantList argValues;
  if ( mArgs )
  {
    foreach( Node* n, mArgs->list() )
    {
      QVariant v = n->eval( parent, f );
      ENSURE_NO_EVAL_ERROR;
      if ( isNull( v ) )
        return QVariant(); // all "normal" functions return NULL when any parameter is NULL
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
    foreach( Node* n, mArgs->list() )
    {
      res = res && n->prepare( parent, fields );
    }
  }
  return res;
}

QString QgsExpression::NodeFunction::dump() const
{
  const FnDef& fd = BuiltinFunctions[mFnIndex];
  if ( fd.mParams == 0 )
    return fd.mName; // special column
  else
    return QString( "%1(%2)" ).arg( fd.mName ).arg( mArgs ? mArgs->dump() : QString() ); // function
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
    default: return QString( "[unsupported type;%1; value:%2]" ).arg( mValue.typeName() ).arg( mValue.toString() );
  }
}

//

QVariant QgsExpression::NodeColumnRef::eval( QgsExpression* /*parent*/, QgsFeature* f )
{
  return f->attributeMap()[mIndex];
}

bool QgsExpression::NodeColumnRef::prepare( QgsExpression* parent, const QgsFieldMap& fields )
{
  foreach( int i, fields.keys() )
  {
    if ( QString::compare( fields[i].name(), mName, Qt::CaseInsensitive ) == 0 )
    {
      mIndex = i;
      return true;
    }
  }
  parent->mEvalErrorString = QObject::tr( "Column '%1'' not found" ).arg( mName );
  mIndex = -1;
  return false;
}

QString QgsExpression::NodeColumnRef::dump() const
{
  return mName;
}
