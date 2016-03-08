/***************************************************************************
 *  qgsvbsmilixlayer.cpp                                                   *
 *  -------------------                                                    *
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
#include "qgsmaplayerrenderer.h"
#include "qgsvbsmilixlayer.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"

void QgsVBSMilixItem::initialize( const QString &mssString, const QString &militaryName, const QList<QgsPoint> &points, const QList<int>& controlPoints, const QPoint &userOffset, bool haveEnoughPoints )
{
  mMssString = mssString;
  mMilitaryName = militaryName;
  mPoints = points;
  mControlPoints = controlPoints;
  mUserOffset = userOffset;
  mHaveEnoughPoints = haveEnoughPoints;
}

QList<QPoint> QgsVBSMilixItem::screenPoints( const QgsMapToPixel& mapToPixel, const QgsCoordinateTransform* crst ) const
{
  QList<QPoint> points;
  foreach ( const QgsPoint& p, mPoints )
  {
    points.append( mapToPixel.transform( crst ? crst->transform( p ) : p ).toQPointF().toPoint() );
  }
  return points;
}

void QgsVBSMilixItem::writeMilx( QDomDocument& doc, QDomElement& graphicListEl, const QString& versionTag, QString& messages ) const
{
  bool valid = false;
  QString symbolXml;
  VBSMilixClient::downgradeSymbolXml( mMssString, versionTag, symbolXml, valid, messages );
  if ( !valid )
  {
    return;
  }

  QDomElement graphicEl = doc.createElement( "MilXGraphic" );
  graphicListEl.appendChild( graphicEl );

  QDomElement stringXmlEl = doc.createElement( "MssStringXML" );
  stringXmlEl.appendChild( doc.createTextNode( symbolXml ) );
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

  QDomElement offsetEl = doc.createElement( "Offset" );
  graphicEl.appendChild( offsetEl );

  QDomElement factorXEl = doc.createElement( "FactorX" );
  factorXEl.appendChild( doc.createTextNode( QString::number( mUserOffset.x() / VBSMilixClient::getSymbolSize() ) ) );
  offsetEl.appendChild( factorXEl );

  QDomElement factorYEl = doc.createElement( "FactorY" );
  factorYEl.appendChild( doc.createTextNode( QString::number( mUserOffset.y() / VBSMilixClient::getSymbolSize() ) ) );
  offsetEl.appendChild( factorYEl );
}

void QgsVBSMilixItem::readMilx( const QDomElement& graphicEl, const QString& symbolXml, const QgsCoordinateTransform* crst, int symbolSize )
{
  QString militaryName = graphicEl.firstChildElement( "Name" ).text();

  mPoints.clear();
  QDomNodeList pointEls = graphicEl.firstChildElement( "PointList" ).elementsByTagName( "Point" );
  for ( int iPoint = 0, nPoints = pointEls.count(); iPoint < nPoints; ++iPoint )
  {
    QDomElement pointEl = pointEls.at( iPoint ).toElement();
    double x = pointEl.firstChildElement( "X" ).text().toDouble();
    double y = pointEl.firstChildElement( "Y" ).text().toDouble();
    if ( crst )
    {
      mPoints.append( crst->transform( QgsPoint( x, y ) ) );
    }
    else
    {
      mPoints.append( QgsPoint( x, y ) );
    }
  }

  double offsetX = graphicEl.firstChildElement( "Offset" ).firstChildElement( "FactorX" ).text().toDouble() * symbolSize;
  double offsetY = graphicEl.firstChildElement( "Offset" ).firstChildElement( "FactorY" ).text().toDouble() * symbolSize;
  mUserOffset = QPoint( offsetX, offsetY );
  mMssString = symbolXml;
  mMilitaryName = militaryName;
}

///////////////////////////////////////////////////////////////////////////////

class QgsVBSMilixLayer::Renderer : public QgsMapLayerRenderer
{
  public:
    Renderer( QgsVBSMilixLayer* layer, QgsRenderContext& rendererContext )
        : QgsMapLayerRenderer( layer->id() )
        , mLayer( layer )
        , mRendererContext( rendererContext )
    {}

    bool render() override
    {
      const QList<QgsVBSMilixItem*>& items = mLayer->mItems;
      QList<QPoint> itemOrigins;
      QList<VBSMilixClient::NPointSymbol> symbols;
      for ( int i = 0, n = items.size(); i < n; ++i )
      {
        if ( !items[i]->isMultiPoint() && mRendererContext.customRenderFlags().split( ";" ).contains( "globe" ) )
        {
          // Only render multipoint symbols in globe
          continue;
        }
        QList<QPoint> points = items[i]->screenPoints( mRendererContext.mapToPixel(), mRendererContext.coordinateTransform() );
        itemOrigins.append( points.front() );

        symbols.append( VBSMilixClient::NPointSymbol( items[i]->mssString(), points, items[i]->controlPoints(), items[i]->hasEnoughPoints() ) );
      }
      if ( symbols.isEmpty() )
      {
        return true;
      }
      QList<VBSMilixClient::NPointSymbolGraphic> result;
      if ( !VBSMilixClient::updateSymbols( screenExtent(), symbols, result ) )
      {
        return false;
      }
      for ( int i = 0, n = result.size(); i < n; ++i )
      {
        QPoint renderPos = itemOrigins[i] + result[i].offset + items[i]->userOffset();
        if ( !items[i]->isMultiPoint() )
        {
          // Draw line from visual reference point to actual refrence point
          mRendererContext.painter()->drawLine( itemOrigins[i], itemOrigins[i] + items[i]->userOffset() );
        }
        mRendererContext.painter()->drawImage( renderPos, result[i].graphic );
      }
      return true;
    }

  private:
    QgsVBSMilixLayer* mLayer;
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

QgsVBSMilixLayer::QgsVBSMilixLayer( const QString &name )
    : QgsPluginLayer( "MilX_Layer", name )
{
  mValid = true;
  setCrs( QgsCoordinateReferenceSystem( "EPSG:4326" ), false );
}

QgsVBSMilixLayer::~QgsVBSMilixLayer()
{
  qDeleteAll( mItems );
}

bool QgsVBSMilixLayer::testPick( const QgsPoint& mapPos, const QgsMapSettings& mapSettings, QVariant& pickResult )
{
  QPoint screenPos = mapSettings.mapToPixel().transform( mapPos ).toQPointF().toPoint();
  const QgsCoordinateTransform* crst = QgsCoordinateTransformCache::instance()->transform( "EPSG:4326", mapSettings.destinationCrs().authid() );
  QList<VBSMilixClient::NPointSymbol> symbols;
  for ( int i = 0, n = mItems.size(); i < n; ++i )
  {
    QList<QPoint> points = mItems[i]->screenPoints( mapSettings.mapToPixel(), crst );
    for ( int j = 0, m = points.size(); j < m; ++j )
    {
      points[j] += mItems[i]->userOffset();
    }
    symbols.append( VBSMilixClient::NPointSymbol( mItems[i]->mssString(), points, mItems[i]->controlPoints(), mItems[i]->hasEnoughPoints() ) );
  }
  int selectedSymbol = -1;
  if ( VBSMilixClient::pickSymbol( symbols, screenPos, selectedSymbol ) && selectedSymbol >= 0 )
  {
    pickResult = QVariant::fromValue( selectedSymbol );
    return true;
  }
  return false;
}

void QgsVBSMilixLayer::handlePick( const QVariant& pick )
{
  emit symbolPicked( pick.toInt() );
}

void QgsVBSMilixLayer::exportToMilxly( QDomElement& milxDocumentEl, const QString& versionTag, QStringList& exportMessages )
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

  foreach ( const QgsVBSMilixItem* item, mItems )
  {
    QString messages;
    item->writeMilx( doc, graphicListEl, versionTag, messages );
    if ( !messages.isEmpty() )
    {
      exportMessages.append( QString( "%1:\n%2\n" ).arg( item->mssString() ).arg( messages ) );
    }
  }

  QDomElement crsEl = doc.createElement( "CoordSystemType" );
  crsEl.appendChild( doc.createTextNode( "WGS84" ) );
  milxLayerEl.appendChild( crsEl );

  QDomElement symbolSizeEl = doc.createElement( "SymbolSize" );
  symbolSizeEl.appendChild( doc.createTextNode( QString::number( VBSMilixClient::getSymbolSize() ) ) );
  milxLayerEl.appendChild( symbolSizeEl );

  QDomElement bwEl = doc.createElement( "DisplayBW" );
  bwEl.appendChild( doc.createTextNode( "0" ) ); // TODO
  milxLayerEl.appendChild( bwEl );
}

bool QgsVBSMilixLayer::importMilxly( QDomElement& milxLayerEl, const QString& fileMssVer, QString& errorMsg, QStringList& importMessages )
{
  setLayerName( milxLayerEl.firstChildElement( "Name" ).text() );
  //    QString layerType = milxLayerEl.firstChildElement( "LayerType" ).text(); // TODO
  int symbolSize = milxLayerEl.firstChildElement( "SymbolSize" ).text().toInt();
  QString crs = milxLayerEl.firstChildElement( "CoordSystemType" ).text();
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

  QDomNodeList graphicEls = milxLayerEl.firstChildElement( "GraphicList" ).elementsByTagName( "MilXGraphic" );

  // Dry run to validate
  QStringList validationErrors;
  QStringList adjustedSymbolXmls;
  for ( int iGraphic = 0, nGraphics = graphicEls.count(); iGraphic < nGraphics; ++iGraphic )
  {
    QDomElement graphicEl = graphicEls.at( iGraphic ).toElement();
    QString mssStringXml = graphicEl.firstChildElement( "MssStringXML" ).text();
    QString adjustedSymbolXml;
    bool valid = false;
    QString messages;
    VBSMilixClient::validateSymbolXml( mssStringXml, fileMssVer, adjustedSymbolXml, valid, messages );
    adjustedSymbolXmls.append( adjustedSymbolXml );
    if ( !valid )
    {
      validationErrors.append( QString( "%1:\n%2\n" ).arg( mssStringXml ).arg( messages ) );
    }
    else if ( !messages.isEmpty() )
    {
      importMessages.append( QString( "%1:\n%2\n" ).arg( mssStringXml ).arg( messages ) );
    }
  }
  if ( !validationErrors.isEmpty() )
  {
    errorMsg = tr( "The following validation errors occured:\n%1" ).arg( validationErrors.join( "\n" ) );
    return false;
  }
  for ( int iGraphic = 0, nGraphics = graphicEls.count(); iGraphic < nGraphics; ++iGraphic )
  {
    QDomElement graphicEl = graphicEls.at( iGraphic ).toElement();

    QgsVBSMilixItem* item = new QgsVBSMilixItem();
    item->readMilx( graphicEl, adjustedSymbolXmls[iGraphic], crst, symbolSize );
    addItem( item );
  }
  return true;
}

QgsLegendSymbologyList QgsVBSMilixLayer::legendSymbologyItems( const QSize& /*iconSize*/ )
{
  return QgsLegendSymbologyList();
}

QgsMapLayerRenderer* QgsVBSMilixLayer::createMapRenderer( QgsRenderContext& rendererContext )
{
  return new Renderer( this, rendererContext );
}

QgsRectangle QgsVBSMilixLayer::extent()
{
  QgsRectangle r;
  foreach ( QgsVBSMilixItem* item, mItems )
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
