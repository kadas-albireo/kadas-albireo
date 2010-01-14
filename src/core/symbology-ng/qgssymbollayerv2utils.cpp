
#include "qgssymbollayerv2utils.h"

#include "qgssymbollayerv2.h"
#include "qgssymbollayerv2registry.h"
#include "qgssymbolv2.h"
#include "qgsvectorcolorrampv2.h"

#include "qgslogger.h"
#include "qgsrendercontext.h"

#include <QColor>
#include <QDomNode>
#include <QDomElement>
#include <QIcon>
#include <QPainter>

QString QgsSymbolLayerV2Utils::encodeColor( QColor color )
{
  return QString( "%1,%2,%3" ).arg( color.red() ).arg( color.green() ).arg( color.blue() );
}

QColor QgsSymbolLayerV2Utils::decodeColor( QString str )
{
  QStringList lst = str.split( "," );
  if ( lst.count() != 3 )
    return QColor();
  return QColor( lst[0].toInt(), lst[1].toInt(), lst[2].toInt() );
}

QString QgsSymbolLayerV2Utils::encodePenStyle( Qt::PenStyle style )
{
  switch ( style )
  {
    case Qt::NoPen:          return "no";
    case Qt::SolidLine:      return "solid";
    case Qt::DashLine:       return "dash";
    case Qt::DotLine:        return "dot";
    case Qt::DashDotLine:    return "dash dot";
    case Qt::DashDotDotLine: return "dash dot dot";
    default: return "???";
  }
}

Qt::PenStyle QgsSymbolLayerV2Utils::decodePenStyle( QString str )
{
  if ( str == "no" ) return Qt::NoPen;
  if ( str == "solid" ) return Qt::SolidLine;
  if ( str == "dash" ) return Qt::DashLine;
  if ( str == "dot" ) return Qt::DotLine;
  if ( str == "dash dot" ) return Qt::DashDotLine;
  if ( str == "dast dot dot" ) return Qt::DashDotDotLine;
  return Qt::SolidLine;
}

QString QgsSymbolLayerV2Utils::encodePenJoinStyle( Qt::PenJoinStyle style )
{
  switch ( style )
  {
    case Qt::BevelJoin: return "bevel";
    case Qt::MiterJoin: return "miter";
    case Qt::RoundJoin: return "round";
    default: return "???";
  }
}

Qt::PenJoinStyle QgsSymbolLayerV2Utils::decodePenJoinStyle( QString str )
{
  if ( str == "bevel" ) return Qt::BevelJoin;
  if ( str == "miter" ) return Qt::MiterJoin;
  if ( str == "round" ) return Qt::RoundJoin;
  return Qt::BevelJoin;
}

QString QgsSymbolLayerV2Utils::encodePenCapStyle( Qt::PenCapStyle style )
{
  switch ( style )
  {
    case Qt::SquareCap: return "square";
    case Qt::FlatCap:   return "flat";
    case Qt::RoundCap:  return "round";
    default: return "???";
  }
}

Qt::PenCapStyle QgsSymbolLayerV2Utils::decodePenCapStyle( QString str )
{
  if ( str == "square" ) return Qt::SquareCap;
  if ( str == "flat" ) return Qt::FlatCap;
  if ( str == "round" ) return Qt::RoundCap;
  return Qt::SquareCap;
}


QString QgsSymbolLayerV2Utils::encodeBrushStyle( Qt::BrushStyle style )
{
  switch ( style )
  {
    case Qt::SolidPattern : return "solid";
    case Qt::HorPattern : return "horizontal";
    case Qt::VerPattern : return "vertical";
    case Qt::CrossPattern : return "cross";
    case Qt::BDiagPattern : return "b_diagonal";
    case Qt::FDiagPattern : return  "f_diagonal";
    case Qt::DiagCrossPattern : return "diagonal_x";
    case Qt::Dense1Pattern  : return "dense1";
    case Qt::Dense2Pattern  : return "dense2";
    case Qt::Dense3Pattern  : return "dense3";
    case Qt::Dense4Pattern  : return "dense4";
    case Qt::Dense5Pattern  : return "dense5";
    case Qt::Dense6Pattern  : return "dense6";
    case Qt::Dense7Pattern  : return "dense7";
    case Qt::NoBrush : return "no";
    default: return "???";
  }
}

Qt::BrushStyle QgsSymbolLayerV2Utils::decodeBrushStyle( QString str )
{
  if ( str == "solid" ) return Qt::SolidPattern;
  if ( str == "horizontal" ) return Qt::HorPattern;
  if ( str == "vertical" ) return Qt::VerPattern;
  if ( str == "cross" ) return Qt::CrossPattern;
  if ( str == "b_diagonal" ) return Qt::BDiagPattern;
  if ( str == "f_diagonal" ) return Qt::FDiagPattern;
  if ( str == "diagonal_x" ) return Qt::DiagCrossPattern;
  if ( str == "dense1" ) return Qt::Dense1Pattern;
  if ( str == "dense2" ) return Qt::Dense2Pattern;
  if ( str == "dense3" ) return Qt::Dense3Pattern;
  if ( str == "dense4" ) return Qt::Dense4Pattern;
  if ( str == "dense5" ) return Qt::Dense5Pattern;
  if ( str == "dense6" ) return Qt::Dense6Pattern;
  if ( str == "dense7" ) return Qt::Dense7Pattern;
  if ( str == "no" ) return Qt::NoBrush;
  return Qt::SolidPattern;
}

QString QgsSymbolLayerV2Utils::encodePoint( QPointF point )
{
  return QString( "%1,%2" ).arg( point.x() ).arg( point.y() );
}

QPointF QgsSymbolLayerV2Utils::decodePoint( QString str )
{
  QStringList lst = str.split( ',' );
  if ( lst.count() != 2 )
    return QPointF( 0, 0 );
  return QPointF( lst[0].toDouble(), lst[1].toDouble() );
}

QIcon QgsSymbolLayerV2Utils::symbolPreviewIcon( QgsSymbolV2* symbol, QSize size )
{
  return QIcon( symbolPreviewPixmap( symbol, size ) );
}

QPixmap QgsSymbolLayerV2Utils::symbolPreviewPixmap( QgsSymbolV2* symbol, QSize size )
{
  QPixmap pixmap( size );
  QPainter painter;
  painter.begin( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  painter.eraseRect( QRect( QPoint( 0, 0 ), size ) );
  symbol->drawPreviewIcon( &painter, size );
  painter.end();
  return pixmap;
}


QIcon QgsSymbolLayerV2Utils::symbolLayerPreviewIcon( QgsSymbolLayerV2* layer, QgsSymbolV2::OutputUnit u, QSize size )
{
  QPixmap pixmap( size );
  QPainter painter;
  painter.begin( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  painter.eraseRect( QRect( QPoint( 0, 0 ), size ) );
  QgsRenderContext renderContext;
  renderContext.setPainter( &painter );
  QgsSymbolV2RenderContext symbolContext( &renderContext, u );
  layer->drawPreviewIcon( symbolContext, size );
  painter.end();
  return QIcon( pixmap );
}

QIcon QgsSymbolLayerV2Utils::colorRampPreviewIcon( QgsVectorColorRampV2* ramp, QSize size )
{
  return QIcon( colorRampPreviewPixmap( ramp, size ) );
}

QPixmap QgsSymbolLayerV2Utils::colorRampPreviewPixmap( QgsVectorColorRampV2* ramp, QSize size )
{
  QPixmap pixmap( size );
  QPainter painter;
  painter.begin( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  painter.eraseRect( QRect( QPoint( 0, 0 ), size ) );
  for ( int i = 0; i < size.width(); i++ )
  {
    QPen pen( ramp->color(( double ) i / size.width() ) );
    painter.setPen( pen );
    painter.drawLine( i, 0, i, size.height() - 1 );
  }
  painter.end();
  return pixmap;
}


#include <QPolygonF>

#include <cmath>
#include <cfloat>


// calculate line's angle and tangent
static bool lineInfo( QPointF p1, QPointF p2, double& angle, double& t )
{
  double x1 = p1.x(), y1 = p1.y(), x2 = p2.x(), y2 = p2.y();

  if ( x1 == x2 && y1 == y2 )
    return false;

  // tangent
  t = ( x1 == x2 ? t = DBL_MAX : ( y2 - y1 ) / ( x2 - x1 ) );

  // angle
  if ( t == DBL_MAX )
    angle = ( y2 >= y1 ? M_PI / 2 : M_PI * 3 / 2 );  // angle is 90 or 270
  else if ( t >= 0 )
    angle = ( y2 >= y1 ? atan( t ) : M_PI + atan( t ) );
  else // t < 0
    angle = ( y2 >= y1 ? M_PI + atan( t ) : M_PI * 2 + atan( t ) );

  return true;
}

// offset a point with an angle and distance
static QPointF offsetPoint( QPointF pt, double angle, double dist )
{
  return QPointF( pt.x() + dist * cos( angle ), pt.y() + dist * sin( angle ) );
}

// calc intersection of two (infinite) lines defined by one point and tangent
static QPointF linesIntersection( QPointF p1, double t1, QPointF p2, double t2 )
{
  // parallel lines?
  if (( t1 == DBL_MAX && t2 == DBL_MAX ) || t1 == t2 )
    return QPointF();

  double x, y;
  if ( t1 == DBL_MAX || t2 == DBL_MAX )
  {
    // in case one line is with angle 90 resp. 270 degrees (tangent undefined)
    // swap them so that line 2 is with undefined tangent
    if ( t1 == DBL_MAX )
    {
      QPointF pSwp = p1; p1 = p2; p2 = pSwp;
      double  tSwp = t1; t1 = t2; t2 = tSwp;
    }

    x = p2.x();
  }
  else
  {
    // usual case
    x = (( p1.y() - p2.y() ) + t2 * p2.x() - t1 * p1.x() ) / ( t2 - t1 );
  }

  y = p1.y() + t1 * ( x - p1.x() );
  return QPointF( x, y );
}


QPolygonF offsetLine( QPolygonF polyline, double dist )
{
  QPolygonF newLine;

  if ( polyline.count() < 2 )
    return newLine;

  double angle = 0.0, t_new, t_old = 0;
  QPointF pt_old, pt_new;
  QPointF p1 = polyline[0], p2;

  for ( int i = 1; i < polyline.count(); i++ )
  {
    p2 = polyline[i];

    if ( !lineInfo( p1, p2, angle, t_new ) )
      continue; // not a line...

    pt_new = offsetPoint( p1, angle + M_PI / 2, dist );

    if ( i != 1 )
    {
      // if it's not the first line segment
      // calc intersection with last line (with offset)
      pt_new = linesIntersection( pt_old, t_old, pt_new, t_new );
    }

    newLine.append( pt_new );

    pt_old = pt_new;
    t_old = t_new;
    p1 = p2;
  }

  // last line segment:
  pt_new = offsetPoint( p2, angle + M_PI / 2, dist );
  newLine.append( pt_new );
  return newLine;
}

/////


QgsSymbolV2* QgsSymbolLayerV2Utils::loadSymbol( QDomElement& element )
{
  QgsSymbolLayerV2List layers;
  QDomNode layerNode = element.firstChild();

  while ( !layerNode.isNull() )
  {
    QDomElement e = layerNode.toElement();
    if ( !e.isNull() )
    {
      if ( e.tagName() != "layer" )
      {
        QgsDebugMsg( "unknown tag " + e.tagName() );
      }
      else
      {
        QgsSymbolLayerV2* layer = loadSymbolLayer( e );
        if ( layer != NULL )
          layers.append( layer );
      }
    }
    layerNode = layerNode.nextSibling();
  }

  if ( layers.count() == 0 )
  {
    QgsDebugMsg( "no layers for symbol" );
    return NULL;
  }

  QString symbolType = element.attribute( "type" );
  QString unitString = element.attribute( "outputUnit", "MM" );

  QgsSymbolV2* symbol = 0;
  if ( symbolType == "line" )
    symbol = new QgsLineSymbolV2( layers );
  else if ( symbolType == "fill" )
    symbol = new QgsFillSymbolV2( layers );
  else if ( symbolType == "marker" )
    symbol = new QgsMarkerSymbolV2( layers );
  else
  {
    QgsDebugMsg( "unknown symbol type " + symbolType );
    return NULL;
  }

  if ( unitString == "MM" )
  {
    symbol->setOutputUnit( QgsSymbolV2::MM );
  }
  else
  {
    symbol->setOutputUnit( QgsSymbolV2::MapUnit );
  }
  return symbol;
}

QgsSymbolLayerV2* QgsSymbolLayerV2Utils::loadSymbolLayer( QDomElement& element )
{
  QString layerClass = element.attribute( "class" );
  bool locked = element.attribute( "locked" ).toInt();
  int pass = element.attribute( "pass" ).toInt();

  // parse properties
  QgsStringMap props = parseProperties( element );

  QgsSymbolLayerV2* layer;
  layer = QgsSymbolLayerV2Registry::instance()->createSymbolLayer( layerClass, props );
  if ( layer )
  {
    layer->setLocked( locked );
    layer->setRenderingPass( pass );
    return layer;
  }
  else
  {
    QgsDebugMsg( "unknown class " + layerClass );
    return NULL;
  }
}

static QString _nameForSymbolType( QgsSymbolV2::SymbolType type )
{
  switch ( type )
  {
    case QgsSymbolV2::Line: return "line";
    case QgsSymbolV2::Marker: return "marker";
    case QgsSymbolV2::Fill: return "fill";
    default: return "";
  }
}

QDomElement QgsSymbolLayerV2Utils::saveSymbol( QString name, QgsSymbolV2* symbol, QDomDocument& doc, QgsSymbolV2Map* subSymbols )
{
  QDomElement symEl = doc.createElement( "symbol" );
  symEl.setAttribute( "type", _nameForSymbolType( symbol->type() ) );
  symEl.setAttribute( "name", name );
  QString unitString = "MM";
  if ( symbol->outputUnit() == QgsSymbolV2::MapUnit )
  {
    unitString = "MapUnit";
  }
  symEl.setAttribute( "outputUnit", unitString );
  QgsDebugMsg( "num layers " + QString::number( symbol->symbolLayerCount() ) );
  for ( int i = 0; i < symbol->symbolLayerCount(); i++ )
  {
    QgsSymbolLayerV2* layer = symbol->symbolLayer( i );

    QDomElement layerEl = doc.createElement( "layer" );
    layerEl.setAttribute( "class", layer->layerType() );
    layerEl.setAttribute( "locked", layer->isLocked() );
    layerEl.setAttribute( "pass", layer->renderingPass() );

    if ( subSymbols != NULL && layer->subSymbol() != NULL )
    {
      QString subname = QString( "@%1@%2" ).arg( name ).arg( i );
      subSymbols->insert( subname, layer->subSymbol() );
    }

    saveProperties( layer->properties(), doc, layerEl );
    symEl.appendChild( layerEl );
  }

  return symEl;
}


QgsStringMap QgsSymbolLayerV2Utils::parseProperties( QDomElement& element )
{
  QgsStringMap props;
  QDomElement e = element.firstChildElement();
  while ( !e.isNull() )
  {
    if ( e.tagName() != "prop" )
    {
      QgsDebugMsg( "unknown tag " + e.tagName() );
    }
    else
    {
      QString propKey = e.attribute( "k" );
      QString propValue = e.attribute( "v" );
      props[propKey] = propValue;
    }
    e = e.nextSiblingElement();
  }
  return props;
}


void QgsSymbolLayerV2Utils::saveProperties( QgsStringMap props, QDomDocument& doc, QDomElement& element )
{
  for ( QgsStringMap::iterator it = props.begin(); it != props.end(); ++it )
  {
    QDomElement propEl = doc.createElement( "prop" );
    propEl.setAttribute( "k", it.key() );
    propEl.setAttribute( "v", it.value() );
    element.appendChild( propEl );
  }
}

QgsSymbolV2Map QgsSymbolLayerV2Utils::loadSymbols( QDomElement& element )
{
  // go through symbols one-by-one and load them

  QgsSymbolV2Map symbols;
  QDomElement e = element.firstChildElement();

  while ( !e.isNull() )
  {
    if ( e.tagName() == "symbol" )
    {
      QgsSymbolV2* symbol = QgsSymbolLayerV2Utils::loadSymbol( e );
      if ( symbol != NULL )
        symbols.insert( e.attribute( "name" ), symbol );
    }
    else
    {
      QgsDebugMsg( "unknown tag: " + e.tagName() );
    }
    e = e.nextSiblingElement();
  }


  // now walk through the list of symbols and find those prefixed with @
  // these symbols are sub-symbols of some other symbol layers
  // e.g. symbol named "@foo@1" is sub-symbol of layer 1 in symbol "foo"
  QStringList subsymbols;

  for ( QMap<QString, QgsSymbolV2*>::iterator it = symbols.begin(); it != symbols.end(); ++it )
  {
    if ( it.key()[0] != '@' )
      continue;

    // add to array (for deletion)
    subsymbols.append( it.key() );

    QStringList parts = it.key().split( "@" );
    if ( parts.count() < 3 )
    {
      QgsDebugMsg( "found subsymbol with invalid name: " + it.key() );
      delete it.value(); // we must delete it
      continue; // some invalid syntax
    }
    QString symname = parts[1];
    int symlayer = parts[2].toInt();

    if ( !symbols.contains( symname ) )
    {
      QgsDebugMsg( "subsymbol references invalid symbol: " + symname );
      delete it.value(); // we must delete it
      continue;
    }

    QgsSymbolV2* sym = symbols[symname];
    if ( symlayer < 0 || symlayer >= sym->symbolLayerCount() )
    {
      QgsDebugMsg( "subsymbol references invalid symbol layer: " + QString::number( symlayer ) );
      delete it.value(); // we must delete it
      continue;
    }

    // set subsymbol takes ownership
    bool res = sym->symbolLayer( symlayer )->setSubSymbol( it.value() );
    if ( !res )
    {
      QgsDebugMsg( "symbol layer refused subsymbol: " + it.key() );
    }


  }

  // now safely remove sub-symbol entries (they have been already deleted or the ownership was taken away)
  for ( int i = 0; i < subsymbols.count(); i++ )
    symbols.take( subsymbols[i] );

  return symbols;
}

QDomElement QgsSymbolLayerV2Utils::saveSymbols( QgsSymbolV2Map& symbols, QString tagName, QDomDocument& doc )
{
  QDomElement symbolsElem = doc.createElement( tagName );

  QMap<QString, QgsSymbolV2*> subSymbols;

  // save symbols
  for ( QMap<QString, QgsSymbolV2*>::iterator its = symbols.begin(); its != symbols.end(); ++its )
  {
    QDomElement symEl = saveSymbol( its.key(), its.value(), doc, &subSymbols );
    symbolsElem.appendChild( symEl );
  }

  // add subsymbols, don't allow subsymbols for them (to keep things simple)
  for ( QMap<QString, QgsSymbolV2*>::iterator itsub = subSymbols.begin(); itsub != subSymbols.end(); ++itsub )
  {
    QDomElement subsymEl = saveSymbol( itsub.key(), itsub.value(), doc );
    symbolsElem.appendChild( subsymEl );
  }

  return symbolsElem;
}

void QgsSymbolLayerV2Utils::clearSymbolMap( QgsSymbolV2Map& symbols )
{
  foreach( QString name, symbols.keys() )
  delete symbols.value( name );
  symbols.clear();
}


QgsVectorColorRampV2* QgsSymbolLayerV2Utils::loadColorRamp( QDomElement& element )
{
  QString rampType = element.attribute( "type" );

  // parse properties
  QgsStringMap props = QgsSymbolLayerV2Utils::parseProperties( element );

  if ( rampType == "gradient" )
    return QgsVectorGradientColorRampV2::create( props );
  else if ( rampType == "random" )
    return QgsVectorRandomColorRampV2::create( props );
  else if ( rampType == "colorbrewer" )
    return QgsVectorColorBrewerColorRampV2::create( props );
  else
  {
    QgsDebugMsg( "unknown colorramp type " + rampType );
    return NULL;
  }
}


QDomElement QgsSymbolLayerV2Utils::saveColorRamp( QString name, QgsVectorColorRampV2* ramp, QDomDocument& doc )
{
  QDomElement rampEl = doc.createElement( "colorramp" );
  rampEl.setAttribute( "type", ramp->type() );
  rampEl.setAttribute( "name", name );

  QgsSymbolLayerV2Utils::saveProperties( ramp->properties(), doc, rampEl );
  return rampEl;
}

double QgsSymbolLayerV2Utils::lineWidthScaleFactor( QgsRenderContext* c, QgsSymbolV2::OutputUnit u )
{
  if ( !c )
  {
    return 1.0;
  }

  if ( u == QgsSymbolV2::MM )
  {
    return c->scaleFactor();
  }
  else //QgsSymbol::MapUnit
  {
    double mup = c->mapToPixel().mapUnitsPerPixel();
    if ( mup > 0 )
    {
      return 1.0 / mup;
    }
    else
    {
      return 1.0;
    }
  }
}

double QgsSymbolLayerV2Utils::pixelSizeScaleFactor( QgsRenderContext* c, QgsSymbolV2::OutputUnit u )
{
  if ( !c )
  {
    return 1.0;
  }

  if ( u == QgsSymbolV2::MM )
  {
    return ( c->scaleFactor() * c->rasterScaleFactor() );
  }
  else //QgsSymbol::MapUnit
  {
    return c->rasterScaleFactor() / c->mapToPixel().mapUnitsPerPixel();
  }
}
