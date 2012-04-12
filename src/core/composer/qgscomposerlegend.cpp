/***************************************************************************
                         qgscomposerlegend.cpp  -  description
                         ---------------------
    begin                : June 2008
    copyright            : (C) 2008 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposerlegend.h"
#include "qgscomposerlegenditem.h"
#include "qgscomposermap.h"
#include "qgsmaplayer.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaprenderer.h"
#include "qgsrenderer.h" //for brush scaling
#include "qgssymbol.h"
#include "qgssymbolv2.h"
#include <QDomDocument>
#include <QDomElement>
#include <QPainter>

QgsComposerLegend::QgsComposerLegend( QgsComposition* composition )
    : QgsComposerItem( composition )
    , mTitle( tr( "Legend" ) )
    , mBoxSpace( 2 )
    , mGroupSpace( 2 )
    , mLayerSpace( 2 )
    , mSymbolSpace( 2 )
    , mIconLabelSpace( 2 )
    , mComposerMap( 0 )

{
  //QStringList idList = layerIdList();
  //mLegendModel.setLayerSet( idList );

  mTitleFont.setPointSizeF( 16.0 );
  mGroupFont.setPointSizeF( 14.0 );
  mLayerFont.setPointSizeF( 12.0 );
  mItemFont.setPointSizeF( 12.0 );

  mSymbolWidth = 7;
  mSymbolHeight = 4;
  mWrapChar = "";
  mlineSpacing = 1.5;
  adjustBoxSize();

  connect( &mLegendModel, SIGNAL( layersChanged() ), this, SLOT( synchronizeWithModel() ) );
}

QgsComposerLegend::QgsComposerLegend(): QgsComposerItem( 0 ), mComposerMap( 0 )
{

}

QgsComposerLegend::~QgsComposerLegend()
{

}

void QgsComposerLegend::paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget )
{
  Q_UNUSED( itemStyle );
  Q_UNUSED( pWidget );
  paintAndDetermineSize( painter );
}

QSizeF QgsComposerLegend::paintAndDetermineSize( QPainter* painter )
{
  QSizeF size;
  double maxXCoord = 0;



  //go through model...
  QStandardItem* rootItem = mLegendModel.invisibleRootItem();
  if ( !rootItem )
  {
    return size;
  }


  if ( painter )
  {
    painter->save();
    drawBackground( painter );
    painter->setPen( QPen( QColor( 0, 0, 0 ) ) );
  }

  int numLayerItems = rootItem->rowCount();
  QStandardItem* currentLayerItem = 0;

  //draw title
  double currentItemMaxX = 0; //maximum x-coordinate for current item
  double currentYCoordinate = 0;
  if ( painter )
  {
    if ( !mTitle.isEmpty() )
    {
      currentYCoordinate = mBoxSpace;

      painter->setPen( QColor( 0, 0, 0 ) );
      QStringList lines = splitStringForWrapping( mTitle );
      for ( QStringList::Iterator titlePart = lines.begin(); titlePart != lines.end(); ++titlePart )
      {
        currentYCoordinate += fontAscentMillimeters( mTitleFont );
        drawText( painter, mBoxSpace, currentYCoordinate, *titlePart, mTitleFont );
        currentItemMaxX = 2 * mBoxSpace + textWidthMillimeters( mTitleFont, *titlePart );
        maxXCoord = qMax( maxXCoord, currentItemMaxX );
        if ( titlePart != lines.end() )
          currentYCoordinate += mlineSpacing;
      }
    }
  }

  for ( int i = 0; i < numLayerItems; ++i )
  {
    currentLayerItem = rootItem->child( i );
    QgsComposerLegendItem* currentLegendItem = dynamic_cast<QgsComposerLegendItem*>( currentLayerItem );
    if ( currentLegendItem )
    {
      QgsComposerLegendItem::ItemType type = currentLegendItem->itemType();
      if ( type == QgsComposerLegendItem::GroupItem )
      {
        drawGroupItem( painter, dynamic_cast<QgsComposerGroupItem*>( currentLegendItem ), currentYCoordinate, currentItemMaxX );
        maxXCoord = qMax( maxXCoord, currentItemMaxX );
      }
      else if ( type == QgsComposerLegendItem::LayerItem )
      {
        drawLayerItem( painter, dynamic_cast<QgsComposerLayerItem*>( currentLegendItem ), currentYCoordinate, currentItemMaxX );
        maxXCoord = qMax( maxXCoord, currentItemMaxX );
      }
    }
  }

  currentYCoordinate += mBoxSpace;

  size.setHeight( currentYCoordinate );
  size.setWidth( maxXCoord );

  //adjust box if width or height is to small
  if ( painter && currentYCoordinate > rect().height() )
  {
    setSceneRect( QRectF( transform().dx(), transform().dy(), rect().width(), currentYCoordinate ) );
  }
  if ( painter && maxXCoord > rect().width() )
  {
    setSceneRect( QRectF( transform().dx(), transform().dy(), maxXCoord, rect().height() ) );
  }

  if ( painter )
  {
    painter->restore();

    //draw frame and selection boxes if necessary
    drawFrame( painter );
    if ( isSelected() )
    {
      drawSelectionBoxes( painter );
    }
  }

  return size;
}

void QgsComposerLegend::drawGroupItem( QPainter* p, QgsComposerGroupItem* groupItem, double& currentYCoord, double& maxXCoord )
{
  if ( !p || !groupItem )
  {
    return;
  }

  currentYCoord += mGroupSpace;
  double currentMaxXCoord = 0;

  p->setPen( QColor( 0, 0, 0 ) );

  QStringList lines = splitStringForWrapping( groupItem->text() );
  for ( QStringList::Iterator groupPart = lines.begin(); groupPart != lines.end(); ++groupPart )
  {
    currentYCoord += fontAscentMillimeters( mGroupFont );
    drawText( p, mBoxSpace, currentYCoord, *groupPart, mGroupFont );
    currentMaxXCoord = 2 * mBoxSpace + textWidthMillimeters( mGroupFont, *groupPart );
    maxXCoord = qMax( currentMaxXCoord, maxXCoord );
    if ( groupPart != lines.end() )
      currentYCoord += mlineSpacing;
  }

  //children can be other group items or layer items
  int numChildItems = groupItem->rowCount();
  QStandardItem* currentChildItem = 0;

  for ( int i = 0; i < numChildItems; ++i )
  {
    currentChildItem = groupItem->child( i );
    QgsComposerLegendItem* currentLegendItem = dynamic_cast<QgsComposerLegendItem*>( currentChildItem );
    QgsComposerLegendItem::ItemType type = currentLegendItem->itemType();
    if ( type == QgsComposerLegendItem::GroupItem )
    {
      drawGroupItem( p, dynamic_cast<QgsComposerGroupItem*>( currentLegendItem ), currentYCoord, currentMaxXCoord );
      maxXCoord = qMax( currentMaxXCoord, maxXCoord );
    }
    else if ( type == QgsComposerLegendItem::LayerItem )
    {
      drawLayerItem( p, dynamic_cast<QgsComposerLayerItem*>( currentLegendItem ), currentYCoord, currentMaxXCoord );
      maxXCoord = qMax( currentMaxXCoord, maxXCoord );
    }
  }
}

void QgsComposerLegend::drawLayerItem( QPainter* p, QgsComposerLayerItem* layerItem, double& currentYCoord, double& maxXCoord )
{
  if ( !layerItem )
  {
    return;
  }

  int opacity = 255;
  QgsMapLayer* currentLayer = QgsMapLayerRegistry::instance()->mapLayer( layerItem->layerID() );
  if ( currentLayer )
  {
    opacity = currentLayer->getTransparency();
  }

  //Let the user omit the layer title item by having an empty layer title string
  if ( !layerItem->text().isEmpty() )
  {
    currentYCoord += mLayerSpace;

    //draw layer Item
    if ( p )
    {
      p->setPen( QColor( 0, 0, 0 ) );

      QStringList lines = splitStringForWrapping( layerItem->text() );
      for ( QStringList::Iterator layerItemPart = lines.begin(); layerItemPart != lines.end(); ++layerItemPart )
      {
        currentYCoord += fontAscentMillimeters( mLayerFont );
        drawText( p, mBoxSpace, currentYCoord, *layerItemPart , mLayerFont );
        maxXCoord = qMax( maxXCoord, 2 * mBoxSpace + textWidthMillimeters( mLayerFont, *layerItemPart ) );
        if ( layerItemPart != lines.end() )
          currentYCoord += mlineSpacing ;
      }
    }
  }
  else //layer title omited
  {
    //symbol space will be added before the item later
    currentYCoord += ( mLayerSpace - mSymbolSpace );
  }

  //and child items
  drawLayerChildItems( p, layerItem, currentYCoord, maxXCoord, opacity );
}

void QgsComposerLegend::adjustBoxSize()
{
  QSizeF size = paintAndDetermineSize( 0 );
  if ( size.isValid() )
  {
    setSceneRect( QRectF( transform().dx(), transform().dy(), size.width(), size.height() ) );
  }
}

void QgsComposerLegend::drawLayerChildItems( QPainter* p, QStandardItem* layerItem, double& currentYCoord, double& maxXCoord, int layerOpacity )
{
  if ( !layerItem )
  {
    return;
  }

  //Draw all symbols first and the texts after (to find out the x coordinate to have the text aligned)
  QList<double> childYCoords;
  QList<double> realItemHeights;

  double textHeight = fontHeightCharacterMM( mItemFont, QChar( '0' ) );
  double itemHeight = qMax( mSymbolHeight, textHeight );

  double textAlignCoord = 0; //alignment for legend text

  QStandardItem* currentItem;

  int numChildren = layerItem->rowCount();

  for ( int i = 0; i < numChildren; ++i )
  {
    //real symbol height. Can be different from standard height in case of point symbols
    double realSymbolHeight;
    double realItemHeight = itemHeight; //will be adjusted if realSymbolHeight turns out to be larger

    currentYCoord += mSymbolSpace;
    double currentXCoord = mBoxSpace;

    currentItem = layerItem->child( i, 0 );

    if ( !currentItem )
    {
      continue;
    }

    int lineCount = splitStringForWrapping( currentItem->text() ).count();

    QgsSymbol* symbol = 0;
    QgsComposerSymbolItem* symbolItem = dynamic_cast<QgsComposerSymbolItem*>( currentItem );
    if ( symbolItem )
    {
      symbol = symbolItem->symbol();
    }

    QgsSymbolV2* symbolNg = 0;
    QgsComposerSymbolV2Item* symbolV2Item = dynamic_cast<QgsComposerSymbolV2Item*>( currentItem );
    if ( symbolV2Item )
    {
      symbolNg = symbolV2Item->symbolV2();
    }
    QgsComposerRasterSymbolItem* rasterItem = dynamic_cast<QgsComposerRasterSymbolItem*>( currentItem );

    if ( symbol )  //item with symbol?
    {
      //draw symbol
      drawSymbol( p, symbol, currentYCoord + ( itemHeight - mSymbolHeight ) / 2, currentXCoord, realSymbolHeight, layerOpacity );
      realItemHeight = qMax( realSymbolHeight, itemHeight );
      currentXCoord += mIconLabelSpace;
    }
    else if ( symbolNg ) //item with symbol NG?
    {
      drawSymbolV2( p, symbolNg, currentYCoord + ( itemHeight - mSymbolHeight ) / 2, currentXCoord, realSymbolHeight, layerOpacity );
      realItemHeight = qMax( realSymbolHeight, itemHeight );
      currentXCoord += mIconLabelSpace;
    }
    else if ( rasterItem )
    {
      if ( p )
      {
        p->setBrush( rasterItem->color() );
        p->drawRect( QRectF( currentXCoord, currentYCoord + ( itemHeight - mSymbolHeight ) / 2, mSymbolWidth, mSymbolHeight ) );
      }
      currentXCoord += mSymbolWidth;
      currentXCoord += mIconLabelSpace;
    }
    else //item with icon?
    {
      QIcon symbolIcon = currentItem->icon();
      if ( !symbolIcon.isNull() && p )
      {
        symbolIcon.paint( p, currentXCoord, currentYCoord + ( itemHeight - mSymbolHeight ) / 2, mSymbolWidth, mSymbolHeight );
        currentXCoord += mSymbolWidth;
        currentXCoord += mIconLabelSpace;
      }
    }

    childYCoords.push_back( currentYCoord );
    realItemHeights.push_back( realItemHeight );
    currentYCoord += lineCount > 0 ? ( realItemHeight + mlineSpacing ) * lineCount : realItemHeight;
    textAlignCoord = qMax( currentXCoord, textAlignCoord );
  }

  maxXCoord = qMax( maxXCoord, textAlignCoord );
  for ( int i = 0; i < numChildren; ++i )
  {
    if ( p )
    {
      p->setPen( QColor( 0, 0, 0 ) );

      QStringList lines = splitStringForWrapping( layerItem->child( i, 0 )->text() );
      double textY = childYCoords.at( i ) + textHeight + ( realItemHeights.at( i ) - textHeight ) / 2;
      for ( QStringList::Iterator itemPart = lines.begin(); itemPart != lines.end(); ++itemPart )
      {
        drawText( p, textAlignCoord, textY , *itemPart , mItemFont );
        maxXCoord = qMax( maxXCoord, textAlignCoord + mBoxSpace + textWidthMillimeters( mItemFont,  *itemPart ) );
        if ( itemPart != lines.end() )
          textY += mlineSpacing + textHeight + ( realItemHeights.at( i ) - textHeight ) / 2;
      }
    }
  }
}

void QgsComposerLegend::drawSymbol( QPainter* p, QgsSymbol* s, double currentYCoord, double& currentXPosition, double& symbolHeight, int layerOpacity ) const
{
  if ( !s )
  {
    return;
  }

  QGis::GeometryType symbolType = s->type();
  switch ( symbolType )
  {
    case QGis::Point:
      drawPointSymbol( p, s, currentYCoord, currentXPosition, symbolHeight, layerOpacity );
      break;
    case QGis::Line:
      drawLineSymbol( p, s, currentYCoord, currentXPosition, layerOpacity );
      symbolHeight = mSymbolHeight;
      break;
    case QGis::Polygon:
      drawPolygonSymbol( p, s, currentYCoord, currentXPosition, layerOpacity );
      symbolHeight = mSymbolHeight;
      break;
    case QGis::UnknownGeometry:
    case QGis::NoGeometry:
      // shouldn't occur
      break;
  }
}

void QgsComposerLegend::drawSymbolV2( QPainter* p, QgsSymbolV2* s, double currentYCoord, double& currentXPosition, double& symbolHeight, int layerOpacity ) const
{
  Q_UNUSED( layerOpacity );
  if ( !p || !s )
  {
    return;
  }

  double rasterScaleFactor = 1.0;
  if ( p )
  {
    QPaintDevice* paintDevice = p->device();
    if ( !paintDevice )
    {
      return;
    }
    rasterScaleFactor = ( paintDevice->logicalDpiX() + paintDevice->logicalDpiY() ) / 2.0 / 25.4;
  }

  //consider relation to composer map for symbol sizes in mm
  bool sizeInMapUnits = s->outputUnit() == QgsSymbolV2::MapUnit;
  double mmPerMapUnit = 1;
  if ( mComposerMap )
  {
    mmPerMapUnit = mComposerMap->mapUnitsToMM();
  }
  QgsMarkerSymbolV2* markerSymbol = dynamic_cast<QgsMarkerSymbolV2*>( s );

  //Consider symbol size for point markers
  double height = mSymbolHeight;
  double width = mSymbolWidth;
  double size = 0;
  //Center small marker symbols
  double widthOffset = 0;
  double heightOffset = 0;

  if ( markerSymbol )
  {
    size = markerSymbol->size();
    height = size;
    width = size;
    if ( mComposerMap && sizeInMapUnits )
    {
      height *= mmPerMapUnit;
      width *= mmPerMapUnit;
      markerSymbol->setSize( width );
    }
    if ( width < mSymbolWidth )
    {
      widthOffset = ( mSymbolWidth - width ) / 2.0;
    }
    if ( height < mSymbolHeight )
    {
      heightOffset = ( mSymbolHeight - height ) / 2.0;
    }
  }

  p->save();
  p->translate( currentXPosition + widthOffset, currentYCoord + heightOffset );
  p->scale( 1.0 / rasterScaleFactor, 1.0 / rasterScaleFactor );

  if ( markerSymbol && sizeInMapUnits )
  {
    s->setOutputUnit( QgsSymbolV2::MM );
  }
  s->drawPreviewIcon( p, QSize( width * rasterScaleFactor, height * rasterScaleFactor ) );

  if ( markerSymbol && sizeInMapUnits )
  {
    s->setOutputUnit( QgsSymbolV2::MapUnit );
    markerSymbol->setSize( size );
  }

  p->restore();
  currentXPosition += width;
  currentXPosition += 2 * widthOffset;
  symbolHeight = height + 2 * heightOffset;
}

void QgsComposerLegend::drawPointSymbol( QPainter* p, QgsSymbol* s, double currentYCoord, double& currentXPosition, double& symbolHeight, int opacity ) const
{
  if ( !s )
  {
    return;
  }

  QImage pointImage;
  double rasterScaleFactor = 1.0;
  if ( p )
  {
    QPaintDevice* paintDevice = p->device();
    if ( !paintDevice )
    {
      return;
    }

    rasterScaleFactor = ( paintDevice->logicalDpiX() + paintDevice->logicalDpiY() ) / 2.0 / 25.4;
  }

  //width scale is 1.0
  pointImage = s->getPointSymbolAsImage( 1.0, false, Qt::yellow, 1.0, 0.0, rasterScaleFactor, opacity / 255.0 );

  if ( p )
  {
    p->save();
    p->scale( 1.0 / rasterScaleFactor, 1.0 / rasterScaleFactor );

    QPointF imageTopLeft( currentXPosition * rasterScaleFactor, currentYCoord * rasterScaleFactor );
    p->drawImage( imageTopLeft, pointImage );
    p->restore();
  }

  currentXPosition += s->pointSize(); //pointImage.width() / rasterScaleFactor;
  symbolHeight = s->pointSize(); //pointImage.height() / rasterScaleFactor;
}

void QgsComposerLegend::drawLineSymbol( QPainter* p, QgsSymbol* s, double currentYCoord, double& currentXPosition, int opacity ) const
{
  if ( !s )
  {
    return;
  }

  double yCoord = currentYCoord + mSymbolHeight / 2;

  if ( p )
  {
    p->save();
    QPen symbolPen = s->pen();
    QColor penColor = symbolPen.color();
    penColor.setAlpha( opacity );
    symbolPen.setColor( penColor );
    symbolPen.setCapStyle( Qt::FlatCap );
    p->setPen( symbolPen );
    p->drawLine( QPointF( currentXPosition, yCoord ), QPointF( currentXPosition + mSymbolWidth, yCoord ) );
    p->restore();
  }

  currentXPosition += mSymbolWidth;
}

void QgsComposerLegend::drawPolygonSymbol( QPainter* p, QgsSymbol* s, double currentYCoord, double& currentXPosition, int opacity ) const
{
  if ( !s )
  {
    return;
  }

  if ( p )
  {
    //scale brush and set transparencies
    QBrush symbolBrush = s->brush();
    QColor brushColor = symbolBrush.color();
    brushColor.setAlpha( opacity );
    symbolBrush.setColor( brushColor );
    QPaintDevice* paintDevice = p->device();
    if ( paintDevice )
    {
      double rasterScaleFactor = ( paintDevice->logicalDpiX() + paintDevice->logicalDpiY() ) / 2.0 / 25.4;
      QgsRenderer::scaleBrush( symbolBrush, rasterScaleFactor );
    }
    p->setBrush( symbolBrush );

    QPen symbolPen = s->pen();
    QColor penColor = symbolPen.color();
    penColor.setAlpha( opacity );
    symbolPen.setColor( penColor );
    p->setPen( symbolPen );

    p->drawRect( QRectF( currentXPosition, currentYCoord, mSymbolWidth, mSymbolHeight ) );
  }

  currentXPosition += mSymbolWidth;
}

QStringList QgsComposerLegend::layerIdList() const
{
  //take layer list from map renderer (to have legend order)
  if ( mComposition )
  {
    QgsMapRenderer* r = mComposition->mapRenderer();
    if ( r )
    {
      return r->layerSet();
    }
  }
  return QStringList();
}

void QgsComposerLegend::synchronizeWithModel()
{
  adjustBoxSize();
  update();
}

void QgsComposerLegend::setTitleFont( const QFont& f )
{
  mTitleFont = f;
  adjustBoxSize();
  update();
}

void QgsComposerLegend::setGroupFont( const QFont& f )
{
  mGroupFont = f;
  adjustBoxSize();
  update();
}

void QgsComposerLegend::setLayerFont( const QFont& f )
{
  mLayerFont = f;
  adjustBoxSize();
  update();
}

void QgsComposerLegend::setItemFont( const QFont& f )
{
  mItemFont = f;
  adjustBoxSize();
  update();
}

QFont QgsComposerLegend::titleFont() const
{
  return mTitleFont;
}

QFont QgsComposerLegend::groupFont() const
{
  return mGroupFont;
}

QFont QgsComposerLegend::layerFont() const
{
  return mLayerFont;
}

QFont QgsComposerLegend::itemFont() const
{
  return mItemFont;
}

void QgsComposerLegend::updateLegend()
{
  mLegendModel.setLayerSet( layerIdList() );
  adjustBoxSize();
  update();
}

bool QgsComposerLegend::writeXML( QDomElement& elem, QDomDocument & doc ) const
{
  if ( elem.isNull() )
  {
    return false;
  }

  QDomElement composerLegendElem = doc.createElement( "ComposerLegend" );

  //write general properties
  composerLegendElem.setAttribute( "title", mTitle );
  composerLegendElem.setAttribute( "titleFont", mTitleFont.toString() );
  composerLegendElem.setAttribute( "groupFont", mGroupFont.toString() );
  composerLegendElem.setAttribute( "layerFont", mLayerFont.toString() );
  composerLegendElem.setAttribute( "itemFont", mItemFont.toString() );
  composerLegendElem.setAttribute( "boxSpace", QString::number( mBoxSpace ) );
  composerLegendElem.setAttribute( "groupSpace", QString::number( mGroupSpace ) );
  composerLegendElem.setAttribute( "layerSpace", QString::number( mLayerSpace ) );
  composerLegendElem.setAttribute( "symbolSpace", QString::number( mSymbolSpace ) );
  composerLegendElem.setAttribute( "iconLabelSpace", QString::number( mIconLabelSpace ) );
  composerLegendElem.setAttribute( "symbolWidth", mSymbolWidth );
  composerLegendElem.setAttribute( "symbolHeight", mSymbolHeight );
  composerLegendElem.setAttribute( "wrapChar", mWrapChar );

  if ( mComposerMap )
  {
    composerLegendElem.setAttribute( "map", mComposerMap->id() );
  }

  //write model properties
  mLegendModel.writeXML( composerLegendElem, doc );

  elem.appendChild( composerLegendElem );
  return _writeXML( composerLegendElem, doc );
}

bool QgsComposerLegend::readXML( const QDomElement& itemElem, const QDomDocument& doc )
{
  if ( itemElem.isNull() )
  {
    return false;
  }

  //read general properties
  mTitle = itemElem.attribute( "title" );
  //title font
  QString titleFontString = itemElem.attribute( "titleFont" );
  if ( !titleFontString.isEmpty() )
  {
    mTitleFont.fromString( titleFontString );
  }
  //group font
  QString groupFontString = itemElem.attribute( "groupFont" );
  if ( !groupFontString.isEmpty() )
  {
    mGroupFont.fromString( groupFontString );
  }

  //layer font
  QString layerFontString = itemElem.attribute( "layerFont" );
  if ( !layerFontString.isEmpty() )
  {
    mLayerFont.fromString( layerFontString );
  }
  //item font
  QString itemFontString = itemElem.attribute( "itemFont" );
  if ( !itemFontString.isEmpty() )
  {
    mItemFont.fromString( itemFontString );
  }

  //spaces
  mBoxSpace = itemElem.attribute( "boxSpace", "2.0" ).toDouble();
  mGroupSpace = itemElem.attribute( "groupSpace", "3.0" ).toDouble();
  mLayerSpace = itemElem.attribute( "layerSpace", "3.0" ).toDouble();
  mSymbolSpace = itemElem.attribute( "symbolSpace", "2.0" ).toDouble();
  mIconLabelSpace = itemElem.attribute( "iconLabelSpace", "2.0" ).toDouble();
  mSymbolWidth = itemElem.attribute( "symbolWidth", "7.0" ).toDouble();
  mSymbolHeight = itemElem.attribute( "symbolHeight", "14.0" ).toDouble();

  mWrapChar = itemElem.attribute( "wrapChar" );

  //composer map
  if ( !itemElem.attribute( "map" ).isEmpty() )
  {
    mComposerMap = mComposition->getComposerMapById( itemElem.attribute( "map" ).toInt() );
  }

  //read model properties
  QDomNodeList modelNodeList = itemElem.elementsByTagName( "Model" );
  if ( modelNodeList.size() > 0 )
  {
    QDomElement modelElem = modelNodeList.at( 0 ).toElement();
    mLegendModel.readXML( modelElem, doc );
  }

  //restore general composer item properties
  QDomNodeList composerItemList = itemElem.elementsByTagName( "ComposerItem" );
  if ( composerItemList.size() > 0 )
  {
    QDomElement composerItemElem = composerItemList.at( 0 ).toElement();
    _readXML( composerItemElem, doc );
  }

  emit itemChanged();
  return true;
}

void QgsComposerLegend::setComposerMap( const QgsComposerMap* map )
{
  mComposerMap = map;
  QObject::connect( map, SIGNAL( destroyed( QObject* ) ), this, SLOT( invalidateCurrentMap() ) );
}

void QgsComposerLegend::invalidateCurrentMap()
{
  disconnect( mComposerMap, SIGNAL( destroyed( QObject* ) ), this, SLOT( invalidateCurrentMap() ) );
  mComposerMap = 0;
}

QStringList QgsComposerLegend::splitStringForWrapping( QString stringToSplt )
{
  QStringList list;
  // If the string contains nothing then just return the string without spliting.
  if ( mWrapChar.count() == 0 )
    list << stringToSplt;
  else
    list = stringToSplt.split( mWrapChar );
  return list;
}
