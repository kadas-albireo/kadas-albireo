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
#include "qgsmaplayerrenderer.h"
#include "qgsmilxlayer.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsbillboardregistry.h"

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

void QgsMilXItem::initialize( const QString &mssString, const QString &militaryName, const QList<QgsPoint> &points, const QList<int>& controlPoints, const QPoint &userOffset, ControlPointState controlPointState )
{
  mMssString = mssString;
  mMilitaryName = militaryName;
  mPoints = points;
  mControlPoints = controlPoints;
  mUserOffset = userOffset;
  if ( mPoints.size() > 1 )
  {
    if ( controlPointState == NEED_CONTROL_POINTS_AND_INDICES )
    {
      // Do some fake geo -> screen transform, since here we have no idea about screen coordinates
      double scale = 100000.;
      QPoint origin = QPoint( mPoints[0].x() * scale, mPoints[0].y() * scale );
      QList<QPoint> screenPoints = QList<QPoint>() << QPoint( 0, 0 );
      for ( int i = 1, n = mPoints.size(); i < n; ++i )
      {
        screenPoints.append( QPoint( mPoints[i].x() * scale, mPoints[i].y() * scale ) - origin );
      }
      if ( MilXClient::getControlPoints( mMssString, screenPoints, mControlPoints ) )
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

void QgsMilXItem::writeMilx( QDomDocument& doc, QDomElement& graphicListEl, const QString& versionTag, QString& messages ) const
{
  bool valid = false;
  QString symbolXml;
  MilXClient::downgradeSymbolXml( mMssString, versionTag, symbolXml, valid, messages );
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
  factorXEl.appendChild( doc.createTextNode( QString::number( mUserOffset.x() / MilXClient::getSymbolSize() ) ) );
  offsetEl.appendChild( factorXEl );

  QDomElement factorYEl = doc.createElement( "FactorY" );
  factorYEl.appendChild( doc.createTextNode( QString::number( mUserOffset.y() / MilXClient::getSymbolSize() ) ) );
  offsetEl.appendChild( factorYEl );
}

void QgsMilXItem::readMilx( const QDomElement& graphicEl, const QString& symbolXml, const QgsCoordinateTransform* crst, int symbolSize )
{
  QString militaryName = graphicEl.firstChildElement( "Name" ).text();
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

  double offsetX = graphicEl.firstChildElement( "Offset" ).firstChildElement( "FactorX" ).text().toDouble() * symbolSize;
  double offsetY = graphicEl.firstChildElement( "Offset" ).firstChildElement( "FactorY" ).text().toDouble() * symbolSize;
  initialize( symbolXml, militaryName, points, QList<int>(), QPoint( offsetX, offsetY ), QgsMilXItem::NEED_CONTROL_POINT_INDICES );
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
      bool isGlobe = mRendererContext.customRenderFlags().split( ";" ).contains( "globe" );
      for ( int i = 0, n = items.size(); i < n; ++i )
      {
        if ( isGlobe && !items[i]->isMultiPoint() )
        {
          // Only render multipoint symbols in globe
          continue;
        }
        QList<QPoint> points = items[i]->screenPoints( mRendererContext.mapToPixel(), mRendererContext.coordinateTransform() );
        itemOrigins.append( points.front() );
        symbols.append( MilXClient::NPointSymbol( items[i]->mssString(), points, items[i]->controlPoints(), true, !mLayer->mIsApproved ) );
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
  if ( !item->isMultiPoint() )
  {
    int symbolSize = MilXClient::getSymbolSize();
    MilXClient::NPointSymbol symbol( item->mssString(), QList<QPoint>() << QPoint( 0, 0 ), QList<int>(), true, true );
    MilXClient::NPointSymbolGraphic graphic;
    if ( MilXClient::updateSymbol( QRect( -symbolSize, -symbolSize, 2 * symbolSize, 2 * symbolSize ), symbol, graphic, false ) )
    {
      QgsBillBoardRegistry::instance()->addItem( this, graphic.graphic, item->points().front(), id() );
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
    symbols.append( MilXClient::NPointSymbol( mItems[i]->mssString(), points, mItems[i]->controlPoints(), true, true ) );
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

void QgsMilXLayer::exportToMilxly( QDomElement& milxDocumentEl, const QString& versionTag, QStringList& exportMessages )
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
  symbolSizeEl.appendChild( doc.createTextNode( QString::number( MilXClient::getSymbolSize() ) ) );
  milxLayerEl.appendChild( symbolSizeEl );

  QDomElement bwEl = doc.createElement( "DisplayBW" );
  bwEl.appendChild( doc.createTextNode( mIsApproved ? "1" : "0" ) );
  milxLayerEl.appendChild( bwEl );
}

bool QgsMilXLayer::importMilxly( QDomElement& milxLayerEl, const QString& fileMssVer, QString& errorMsg, QStringList& importMessages )
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

  mIsApproved = milxLayerEl.firstChildElement( "DisplayBW" ).text().toInt();

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
    MilXClient::validateSymbolXml( mssStringXml, fileMssVer, adjustedSymbolXml, valid, messages );
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

    QgsMilXItem* item = new QgsMilXItem();
    item->readMilx( graphicEl, adjustedSymbolXmls[iGraphic], crst, symbolSize );
    addItem( item );
  }
  return true;
}

bool QgsMilXLayer::readXml( const QDomNode& layer_node )
{
  QString verTag; MilXClient::getCurrentLibraryVersionTag( verTag );
  QDomElement milxLayerEl = layer_node.firstChildElement( "MilXLayer" );
  if ( !milxLayerEl.isNull() )
  {
    QString errMsg;
    QStringList messages;
    return importMilxly( milxLayerEl, verTag, errMsg, messages );
  }
  return true;
}

bool QgsMilXLayer::writeXml( QDomNode & layer_node, QDomDocument & /*document*/ )
{
  QDomElement layerElement = layer_node.toElement();
  layerElement.setAttribute( "type", "plugin" );
  layerElement.setAttribute( "name", layerTypeKey() );

  QString verTag; MilXClient::getCurrentLibraryVersionTag( verTag );
  QStringList messages;
  exportToMilxly( layerElement, verTag, messages );
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
