/***************************************************************************
 *  qgsmilxlayer.cpp                                                       *
 *  ----------------                                                       *
 *  begin                : February 2016                                   *
 *  copyright            : (C) 2016 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscrscache.h"
#include "layertree/qgslayertreeview.h"
#include "qgisinterface.h"
#include "qgscrscache.h"
#include "qgsdistancearea.h"
#include "qgsmaplayerrenderer.h"
#include "qgsmilxlayer.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsbillboardregistry.h"
#include <QApplication>
#include <QVector2D>

bool QgsMilXItem::validateMssString( const QString &mssString, QString& adjustedMssString, QString &messages )
{
  bool valid = false;
  QString libVersion; MilXClient::getCurrentLibraryVersionTag( libVersion );
  return MilXClient::validateSymbolXml( mssString, libVersion, adjustedMssString, valid, messages ) && valid;
}

QgsMilXItem::~QgsMilXItem()
{
  if ( mPoints.size() == 1 )
  {
    QgsBillBoardRegistry::instance()->removeItem( this );
  }
}

void QgsMilXItem::initialize( const QString &mssString, const QString &militaryName, const QList<QgsPoint> &points, const QList<int>& controlPoints, const QList< QPair<int, double> > &attributes, const QPoint &userOffset, ControlPointState controlPointState , bool isCorridor )
{
  mMssString = mssString;
  mMilitaryName = militaryName;
  mPoints = points;
  mControlPoints = controlPoints;
  mAttributes = attributes;
  mUserOffset = userOffset;
  if ( mPoints.size() > 1 )
  {
    if ( isCorridor || controlPointState == NEED_CONTROL_POINTS_AND_INDICES )
    {
      // Do some fake geo -> screen transform, since here we have no idea about screen coordinates
      double scale = 100000.;
      QPoint origin = QPoint( mPoints[0].x() * scale, mPoints[0].y() * scale );
      QList<QPoint> screenPoints = QList<QPoint>() << QPoint( 0, 0 );
      for ( int i = 1, n = mPoints.size(); i < n; ++i )
      {
        screenPoints.append( QPoint( mPoints[i].x() * scale, mPoints[i].y() * scale ) - origin );
      }
      QList< QPair<int, double> > screenAttributes;
      if ( !mAttributes.isEmpty() )
      {
        QgsDistanceArea da;
        da.setSourceCrs( QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
        da.setEllipsoid( "WGS84" );
        da.setEllipsoidalMode( true );
        QGis::UnitType measureUnit = QGis::Degrees;
        QgsPoint otherPoint( mPoints[0].x() + 0.001, mPoints[0].y() );
        QPointF otherScreenPoint = QPointF( otherPoint.x() * scale, otherPoint.y() * scale ) - origin;
        double ellipsoidDist = da.measureLine( mPoints[0], otherPoint );
        da.convertMeasurement( ellipsoidDist, measureUnit, QGis::Meters, false );
        double screenDist = QVector2D( screenPoints[0] - otherScreenPoint ).length();
        for ( int i = 0, n = mAttributes.size(); i < n; ++i )
        {
          screenAttributes.append( qMakePair( mAttributes[i].first, mAttributes[i].second / ellipsoidDist * screenDist ) );
        }
        mAttributes.clear();
      }
      if ( MilXClient::getControlPoints( mMssString, screenPoints, screenAttributes, mControlPoints, isCorridor ) )
      {
        mPoints.clear();
        for ( int i = 0, n = screenPoints.size(); i < n; ++i )
        {
          mPoints.append( QgsPoint(( origin.x() + screenPoints[i].x() ) / scale,
                                   ( origin.y() + screenPoints[i].y() ) / scale ) );
        }
      }
    }
    else if ( controlPointState == NEED_CONTROL_POINT_INDICES )
    {
      MilXClient::getControlPointIndices( mMssString, mPoints.count(), mControlPoints );
    }
  }

  if ( militaryName.isEmpty() )
  {
    MilXClient::getMilitaryName( mMssString, mMilitaryName );
  }
}

QList<QPoint> QgsMilXItem::screenPoints( const QgsMapToPixel& mapToPixel, const QgsCoordinateTransform* crst ) const
{
  QList<QPoint> points;
  foreach ( const QgsPoint& p, mPoints )
  {
    points.append( mapToPixel.transform( crst ? crst->transform( p ) : p ).toQPointF().toPoint() );
  }
  return points;
}

QList<QPair<int, double> > QgsMilXItem::screenAttributes( const QgsMapToPixel& mapToPixel, const QgsCoordinateTransform *crst ) const
{
  if ( mAttributes.isEmpty() )
  {
    return QList<QPair<int, double> >();
  }
  QgsDistanceArea da;
  da.setSourceCrs( QgsCRSCache::instance()->crsByAuthId( "EPSG:4326" ) );
  da.setEllipsoid( "WGS84" );
  da.setEllipsoidalMode( true );
  QGis::UnitType measureUnit = QGis::Degrees;
  QgsPoint otherPoint( mPoints[0].x() + 0.001, mPoints[0].y() );
  QPointF screenPoint = mapToPixel.transform( crst ? crst->transform( mPoints[0] ) : mPoints[0] ).toQPointF();
  QPointF otherScreenPoint = mapToPixel.transform( crst ? crst->transform( otherPoint ) : otherPoint ).toQPointF();
  double ellipsoidDist = da.measureLine( mPoints[0], otherPoint );
  da.convertMeasurement( ellipsoidDist, measureUnit, QGis::Meters, false );
  double screenDist = QVector2D( screenPoint - otherScreenPoint ).length();

  QList< QPair<int, double> > screenAttribs;
  typedef QPair<int, double> AttribPt_t;
  foreach ( const AttribPt_t& attrib, mAttributes )
  {
    double value = attrib.second;
    if ( attrib.first != MilXClient::AttributeAttutide )
    {
      value = value / ellipsoidDist * screenDist;
    }
    screenAttribs.append( qMakePair( attrib.first, value ) );
  }
  return screenAttribs;
}

void QgsMilXItem::writeMilx( QDomDocument& doc, QDomElement& graphicListEl ) const
{
  QDomElement graphicEl = doc.createElement( "MilXGraphic" );
  graphicListEl.appendChild( graphicEl );

  QDomElement stringXmlEl = doc.createElement( "MssStringXML" );
  stringXmlEl.appendChild( doc.createTextNode( mMssString ) );
  graphicEl.appendChild( stringXmlEl );

  QDomElement nameEl = doc.createElement( "Name" );
  nameEl.appendChild( doc.createTextNode( mMilitaryName ) );
  graphicEl.appendChild( nameEl );

  QDomElement pointListEl = doc.createElement( "PointList" );
  graphicEl.appendChild( pointListEl );

  foreach ( const QgsPoint& p, mPoints )
  {
    QDomElement pEl = doc.createElement( "Point" );
    pointListEl.appendChild( pEl );

    QDomElement pXEl = doc.createElement( "X" );
    pXEl.appendChild( doc.createTextNode( QString::number( p.x(), 'f', 6 ) ) );
    pEl.appendChild( pXEl );
    QDomElement pYEl = doc.createElement( "Y" );
    pYEl.appendChild( doc.createTextNode( QString::number( p.y(), 'f', 6 ) ) );
    pEl.appendChild( pYEl );
  }
  if ( !mAttributes.isEmpty() )
  {
    QDomElement attribListEl = doc.createElement( "LocationAttributeList" );
    graphicEl.appendChild( attribListEl );
    for ( int i = 0, n = mAttributes.size(); i < n; ++i )
    {
      QDomElement attrTypeEl = doc.createElement( "AttrType" );
      attrTypeEl.appendChild( doc.createTextNode( MilXClient::attributeName( mAttributes[i].first ) ) );
      QDomElement attrValueEl = doc.createElement( "Value" );
      attrValueEl.appendChild( doc.createTextNode( QString::number( mAttributes[i].second ) ) );
      QDomElement attribEl = doc.createElement( "LocationAttribute" );
      attribEl.appendChild( attrTypeEl );
      attribEl.appendChild( attrValueEl );
      attribListEl.appendChild( attribEl );
    }
  }

  QDomElement offsetEl = doc.createElement( "Offset" );
  graphicEl.appendChild( offsetEl );

  QDomElement factorXEl = doc.createElement( "FactorX" );
  factorXEl.appendChild( doc.createTextNode( QString::number( double( mUserOffset.x() ) / MilXClient::getSymbolSize() ) ) );
  offsetEl.appendChild( factorXEl );

  QDomElement factorYEl = doc.createElement( "FactorY" );
  factorYEl.appendChild( doc.createTextNode( QString::number( -double( mUserOffset.y() ) / MilXClient::getSymbolSize() ) ) );
  offsetEl.appendChild( factorYEl );
}

void QgsMilXItem::readMilx( const QDomElement& graphicEl, const QgsCoordinateTransform* crst, int symbolSize )
{
  QString militaryName = graphicEl.firstChildElement( "Name" ).text();
  bool isCorridor = graphicEl.firstChildElement( "IsMIPCorridorPointList" ).text().toInt();
  QString symbolXml = graphicEl.firstChildElement( "MssStringXML" ).text();
  QList<QgsPoint> points;

  QDomNodeList pointEls = graphicEl.firstChildElement( "PointList" ).elementsByTagName( "Point" );
  for ( int iPoint = 0, nPoints = pointEls.count(); iPoint < nPoints; ++iPoint )
  {
    QDomElement pointEl = pointEls.at( iPoint ).toElement();
    double x = pointEl.firstChildElement( "X" ).text().toDouble();
    double y = pointEl.firstChildElement( "Y" ).text().toDouble();
    if ( crst )
    {
      points.append( crst->transform( QgsPoint( x, y ) ) );
    }
    else
    {
      points.append( QgsPoint( x, y ) );
    }
  }
  QList< QPair<int, double> > attributes;
  QDomNodeList attribEls = graphicEl.firstChildElement( "LocationAttributeList" ).elementsByTagName( "LocationAttribute" );
  for ( int iAttr = 0, nAttrs = attribEls.count(); iAttr < nAttrs; ++iAttr )
  {
    QDomElement attribEl = attribEls.at( iAttr ).toElement();
    attributes.append( qMakePair( MilXClient::attributeIdx( attribEl.firstChildElement( "AttrType" ).text() ), attribEl.firstChildElement( "Value" ).text().toDouble() ) );
  }
  double offsetX = graphicEl.firstChildElement( "Offset" ).firstChildElement( "FactorX" ).text().toDouble() * symbolSize;
  double offsetY = -1. * ( graphicEl.firstChildElement( "Offset" ).firstChildElement( "FactorY" ).text().toDouble() * symbolSize );
  initialize( symbolXml, militaryName, points, QList<int>(), attributes, QPoint( offsetX, offsetY ), QgsMilXItem::NEED_CONTROL_POINT_INDICES, isCorridor );
}

///////////////////////////////////////////////////////////////////////////////

class QgsMilXLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( QgsMilXLayer* layer, QgsRenderContext& rendererContext )
        : QgsMapLayerRenderer( layer->id() )
        , mLayer( layer )
        , mRendererContext( rendererContext )
    {}

    bool render() override
    {
      const QList<QgsMilXItem*>& items = mLayer->mItems;
      QList<QPoint> itemOrigins;
      QList<MilXClient::NPointSymbol> symbols;
      QStringList flags = mRendererContext.customRenderFlags().split( ";" );
      bool omitSinglePoint = flags.contains( "globe" ) || flags.contains( "kml" );
      for ( int i = 0, n = items.size(); i < n; ++i )
      {
        if ( omitSinglePoint && !items[i]->isMultiPoint() )
        {
          // Don't render single-point symbols
          continue;
        }
        QList<QPoint> points = items[i]->screenPoints( mRendererContext.mapToPixel(), mRendererContext.coordinateTransform() );
        if ( points.isEmpty() )
          continue;
        QList< QPair<int, double> > screenAttribs = items[i]->screenAttributes( mRendererContext.mapToPixel(), mRendererContext.coordinateTransform() );
        itemOrigins.append( points.front() );
        symbols.append( MilXClient::NPointSymbol( items[i]->mssString(), points, items[i]->controlPoints(), screenAttribs, true, !mLayer->mIsApproved ) );
      }
      if ( symbols.isEmpty() )
      {
        return true;
      }
      QList<MilXClient::NPointSymbolGraphic> result;
      if ( !MilXClient::updateSymbols( screenExtent(), symbols, result ) )
      {
        return false;
      }
      mLayer->mMargin = 0;
      for ( int i = 0, n = result.size(); i < n; ++i )
      {
        QPoint renderPos = itemOrigins[i] + result[i].offset + items[i]->userOffset();
        if ( !items[i]->isMultiPoint() )
        {
          // Draw line from visual reference point to actual refrence point
          mRendererContext.painter()->drawLine( itemOrigins[i], itemOrigins[i] + items[i]->userOffset() );
        }
        mRendererContext.painter()->drawImage( renderPos, result[i].graphic );
        mLayer->mMargin = qMax( mLayer->mMargin, qMax( result[i].graphic.width() + items[i]->userOffset().x(), result[i].graphic.height() + items[i]->userOffset().y() ) );
      }
      return true;
    }

  private:
    QgsMilXLayer* mLayer;
    QgsRenderContext& mRendererContext;

    QRect screenExtent() const
    {
      QgsRectangle mapRect = mRendererContext.coordinateTransform() ? mRendererContext.coordinateTransform()->transform( mRendererContext.extent() ) : mRendererContext.extent();
      QPoint topLeft = mRendererContext.mapToPixel().transform( mapRect.xMinimum(), mapRect.yMinimum() ).toQPointF().toPoint();
      QPoint topRight = mRendererContext.mapToPixel().transform( mapRect.xMaximum(), mapRect.yMinimum() ).toQPointF().toPoint();
      QPoint bottomLeft = mRendererContext.mapToPixel().transform( mapRect.xMinimum(), mapRect.yMaximum() ).toQPointF().toPoint();
      QPoint bottomRight = mRendererContext.mapToPixel().transform( mapRect.xMaximum(), mapRect.yMaximum() ).toQPointF().toPoint();
      int xMin = qMin( qMin( topLeft.x(), topRight.x() ), qMin( bottomLeft.x(), bottomRight.x() ) );
      int xMax = qMax( qMax( topLeft.x(), topRight.x() ), qMax( bottomLeft.x(), bottomRight.x() ) );
      int yMin = qMin( qMin( topLeft.y(), topRight.y() ), qMin( bottomLeft.y(), bottomRight.y() ) );
      int yMax = qMax( qMax( topLeft.y(), topRight.y() ), qMax( bottomLeft.y(), bottomRight.y() ) );
      return QRect( xMin, yMin, xMax - xMin, yMax - yMin ).normalized();
    }
};

///////////////////////////////////////////////////////////////////////////////

QgsMilXLayer::QgsMilXLayer( QgsLayerTreeViewMenuProvider* menuProvider , const QString &name )
    : QgsPluginLayer( layerTypeKey(), name ), mMenuProvider( menuProvider ), mMargin( 0 ), mIsApproved( false )
{
  mValid = true;
  setCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), false );

  if ( mMenuProvider )
    mMenuProvider->addLegendLayerActionForLayer( "milx_approved_layer", this );
}

QgsMilXLayer::QgsMilXLayer( QgisInterface* iface, const QString &name )
    : QgsPluginLayer( layerTypeKey(), name ), mMenuProvider( iface->layerTreeView()->menuProvider() ), mMargin( 0 ), mIsApproved( false )
{
  mValid = true;
  setCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), false );

  if ( mMenuProvider )
    mMenuProvider->addLegendLayerActionForLayer( "milx_approved_layer", this );
}

QgsMilXLayer::~QgsMilXLayer()
{
  if ( mMenuProvider )
    mMenuProvider->removeLegendLayerActionsForLayer( this );
  qDeleteAll( mItems );
}

void QgsMilXLayer::addItem( QgsMilXItem *item )
{
  mItems.append( item );
  if ( !item->isMultiPoint() && !item->points().isEmpty() )
  {
    int symbolSize = MilXClient::getSymbolSize();
    MilXClient::NPointSymbol symbol( item->mssString(), QList<QPoint>() << QPoint( 0, 0 ), QList<int>(), QList< QPair<int, double> >(), true, true );
    MilXClient::NPointSymbolGraphic graphic;
    if ( MilXClient::updateSymbol( QRect( -symbolSize, -symbolSize, 2 * symbolSize, 2 * symbolSize ), symbol, graphic, false ) )
    {
      QgsBillBoardRegistry::instance()->addItem( item, item->militaryName(), graphic.graphic, item->points().front(), graphic.offset.x() + graphic.graphic.width() / 2, id() );
    }
  }
}

bool QgsMilXLayer::testPick( const QgsPoint& mapPos, const QgsMapSettings& mapSettings, QVariant& pickResult )
{
  QPoint screenPos = mapSettings.mapToPixel().transform( mapPos ).toQPointF().toPoint();
  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", mapSettings.destinationCrs().authid() );
  QList<MilXClient::NPointSymbol> symbols;
  for ( int i = 0, n = mItems.size(); i < n; ++i )
  {
    QList<QPoint> points = mItems[i]->screenPoints( mapSettings.mapToPixel(), crst );
    for ( int j = 0, m = points.size(); j < m; ++j )
    {
      points[j] += mItems[i]->userOffset();
    }
    QList< QPair<int, double> > screenAttribs = mItems[i]->screenAttributes( mapSettings.mapToPixel(), crst );
    symbols.append( MilXClient::NPointSymbol( mItems[i]->mssString(), points, mItems[i]->controlPoints(), screenAttribs, true, true ) );
  }
  int selectedSymbol = -1;
  if ( MilXClient::pickSymbol( symbols, screenPos, selectedSymbol ) && selectedSymbol >= 0 )
  {
    pickResult = QVariant::fromValue( selectedSymbol );
    return true;
  }
  return false;
}

void QgsMilXLayer::handlePick( const QVariant& pick )
{
  emit symbolPicked( pick.toInt() );
}

QVariantList QgsMilXLayer::getItems( const QgsRectangle& extent ) const
{
  QList<QVariant> items;
  for ( int i = 0, n = mItems.size(); i < n; ++i )
  {
    bool include = true;
    foreach ( const QgsPoint& point, mItems[i]->points() )
    {
      if ( !extent.contains( point ) )
      {
        include = false;
        break;
      }
    }
    if ( include )
    {
      items.append( i );
    }
  }
  return items;
}

void QgsMilXLayer::deleteItems( const QVariantList &items )
{
  QVector<int> indices;
  foreach ( const QVariant& item, items )
  {
    indices.append( item.toInt() );
  }
  // Sort in descending order to avoid invalidating indices
  qSort( indices.begin(), indices.end(), qGreater<int>() );
  foreach ( int idx, indices )
  {
    delete mItems.takeAt( idx );
  }
}

void QgsMilXLayer::invalidateBillboards()
{
  foreach ( QgsMilXItem* item, mItems )
  {
    if ( !item->isMultiPoint() && !item->points().isEmpty() )
    {
      int symbolSize = MilXClient::getSymbolSize();
      MilXClient::NPointSymbol symbol( item->mssString(), QList<QPoint>() << QPoint( 0, 0 ), QList<int>(), QList< QPair<int, double> >(), true, true );
      MilXClient::NPointSymbolGraphic graphic;
      if ( MilXClient::updateSymbol( QRect( -symbolSize, -symbolSize, 2 * symbolSize, 2 * symbolSize ), symbol, graphic, false ) )
      {
        QgsBillBoardRegistry::instance()->addItem( item, item->militaryName(), graphic.graphic, item->points().front(), graphic.offset.x() + graphic.graphic.width() / 2, id() );
      }
    }
  }
}

void QgsMilXLayer::exportToMilxly( QDomElement& milxDocumentEl, int dpi )
{
  QDomDocument doc = milxDocumentEl.ownerDocument();

  QDomElement milxLayerEl = doc.createElement( "MilXLayer" );
  milxDocumentEl.appendChild( milxLayerEl );

  QDomElement milxLayerNameEl = doc.createElement( "Name" );
  milxLayerNameEl.appendChild( doc.createTextNode( name() ) );
  milxLayerEl.appendChild( milxLayerNameEl );

  QDomElement milxLayerTypeEl = doc.createElement( "LayerType" );
  milxLayerTypeEl.appendChild( doc.createTextNode( "Normal" ) );
  milxLayerEl.appendChild( milxLayerTypeEl );

  QDomElement graphicListEl = doc.createElement( "GraphicList" );
  milxLayerEl.appendChild( graphicListEl );

  foreach ( const QgsMilXItem* item, mItems )
  {
    item->writeMilx( doc, graphicListEl );
  }

  QDomElement crsEl = doc.createElement( "CoordSystemType" );
  crsEl.appendChild( doc.createTextNode( "WGS84" ) );
  milxLayerEl.appendChild( crsEl );

  QDomElement symbolSizeEl = doc.createElement( "SymbolSize" );
  symbolSizeEl.appendChild( doc.createTextNode( QString::number(( MilXClient::getSymbolSize() * 25.4 ) / dpi ) ) );
  milxLayerEl.appendChild( symbolSizeEl );

  QDomElement bwEl = doc.createElement( "DisplayBW" );
  bwEl.appendChild( doc.createTextNode( mIsApproved ? "1" : "0" ) );
  milxLayerEl.appendChild( bwEl );
}

bool QgsMilXLayer::importMilxly( QDomElement& milxLayerEl, int dpi, QString& errorMsg )
{
  setLayerName( milxLayerEl.firstChildElement( "Name" ).text() );
  //    QString layerType = milxLayerEl.firstChildElement( "LayerType" ).text(); // TODO
  float symbolSize = milxLayerEl.firstChildElement( "SymbolSize" ).text().toFloat(); // This is in mm
  symbolSize = ( symbolSize * dpi ) / 25.4; // mm to px
  QString crs = milxLayerEl.firstChildElement( "CoordSystemType" ).text();
  if ( crs.isEmpty() )
  {
    errorMsg = tr( "The file is corrupt" );
    return false;
  }
  QString utmZone = milxLayerEl.firstChildElement( "CoordSystemUtmZone" ).text();
  const QgsCoordinateTransform* crst = 0;
  if ( crs == "SwissLv03" )
  {
    crst = QgsCoordinateTransformCache::instance()->transform( "EPSG:21781", "EPSG:4326" );
  }
  else if ( crs == "WGS84" )
  {
    crst = 0;
  }
  else if ( crs == "UTM" )
  {
    QgsCoordinateReferenceSystem utmCrs;
    QString zoneLetter = utmZone.right( 1 ).toUpper();
    QString zoneNumber = utmZone.left( utmZone.length() - 1 );
    QString projZone = zoneNumber + ( zoneLetter == "S" ? " +south" : "" );
    utmCrs.createFromProj4( QString( "+proj=utm +zone=%1 +datum=WGS84 +units=m +no_defs" ).arg( projZone ) );
    crst = QgsCoordinateTransformCache::instance()->transform( utmCrs.authid(), "EPSG:4326" );
  }

  mIsApproved = milxLayerEl.firstChildElement( "DisplayBW" ).text().toInt();

  QDomNodeList graphicEls = milxLayerEl.firstChildElement( "GraphicList" ).elementsByTagName( "MilXGraphic" );
  for ( int iGraphic = 0, nGraphics = graphicEls.count(); iGraphic < nGraphics; ++iGraphic )
  {
    QDomElement graphicEl = graphicEls.at( iGraphic ).toElement();
    QgsMilXItem* item = new QgsMilXItem();
    item->readMilx( graphicEl, crst, symbolSize );
    addItem( item );
  }
  return true;
}

bool QgsMilXLayer::readXml( const QDomNode& layer_node )
{
  QDomElement milxLayerEl = layer_node.firstChildElement( "MilXLayer" );
  if ( !milxLayerEl.isNull() )
  {
    QString errorMsg;
    return importMilxly( milxLayerEl, 96, errorMsg );
  }
  return true;
}

bool QgsMilXLayer::writeXml( QDomNode & layer_node, QDomDocument & /*document*/ )
{
  QDomElement layerElement = layer_node.toElement();
  layerElement.setAttribute( "type", "plugin" );
  layerElement.setAttribute( "name", layerTypeKey() );
  exportToMilxly( layerElement, 96 );
  return true;
}

QgsLegendSymbologyList QgsMilXLayer::legendSymbologyItems( const QSize& /*iconSize*/ )
{
  return QgsLegendSymbologyList();
}

QgsMapLayerRenderer* QgsMilXLayer::createMapRenderer( QgsRenderContext& rendererContext )
{
  return new Renderer( this, rendererContext );
}

QgsRectangle QgsMilXLayer::extent()
{
  QgsRectangle r;
  foreach ( QgsMilXItem* item, mItems )
  {
    foreach ( const QgsPoint& point, item->points() )
    {
      if ( r.isNull() )
      {
        r.set( point.x(), point.y(), point.x(), point.y() );
      }
      else
      {
        r.include( point );
      }
    }
  }
  return r;
}

int QgsMilXLayer::margin() const
{
  return mMargin;
}
