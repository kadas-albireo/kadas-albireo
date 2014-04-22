/***************************************************************************
                         qgscomposertable.cpp
                         --------------------
    begin                : January 2010
    copyright            : (C) 2010 by Marco Hugentobler
    email                : marco at hugis dot net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposertable.h"
#include "qgslogger.h"
#include <QPainter>
#include <QSettings>

QgsComposerTable::QgsComposerTable( QgsComposition* composition )
    : QgsComposerItem( composition )
    , mLineTextDistance( 1.0 )
    , mShowGrid( true )
    , mGridStrokeWidth( 0.5 )
    , mGridColor( QColor( 0, 0, 0 ) )
{
  //get default composer font from settings
  QSettings settings;
  QString defaultFontString = settings.value( "/Composer/defaultFont" ).toString();
  if ( !defaultFontString.isEmpty() )
  {
    mHeaderFont.setFamily( defaultFontString );
    mContentFont.setFamily( defaultFontString );
  }
}

QgsComposerTable::~QgsComposerTable()
{

}

void QgsComposerTable::refreshAttributes()
{

  mMaxColumnWidthMap.clear();
  mAttributeMaps.clear();

  //getFeatureAttributes
  if ( !getFeatureAttributes( mAttributeMaps ) )
  {
    return;
  }

  //since attributes have changed, we also need to recalculate the column widths
  //and size of table
  adjustFrameToSize();
}

void QgsComposerTable::paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget )
{
  Q_UNUSED( itemStyle );
  Q_UNUSED( pWidget );
  if ( !painter )
  {
    return;
  }

  if ( mComposition->plotStyle() == QgsComposition::Print ||
       mComposition->plotStyle() == QgsComposition::Postscript )
  {
    //exporting composition, so force an attribute refresh
    //we do this in case vector layer has changed via an external source (eg, another database user)
    refreshAttributes();
  }

  drawBackground( painter );
  painter->setPen( Qt::SolidLine );

  //now draw the text
  double currentX = mGridStrokeWidth;
  double currentY;

  QMap<int, QString> headerMap = getHeaderLabels();
  QMap<int, QString>::const_iterator columnIt = headerMap.constBegin();

  for ( ; columnIt != headerMap.constEnd(); ++columnIt )
  {
    currentY = mGridStrokeWidth;
    currentY += mLineTextDistance;
    currentY += fontAscentMillimeters( mHeaderFont );
    currentX += mLineTextDistance;
    drawText( painter, currentX, currentY, columnIt.value(), mHeaderFont );

    currentY += mLineTextDistance;
    currentY += mGridStrokeWidth;

    //draw the attribute values
    QList<QgsAttributeMap>::const_iterator attIt = mAttributeMaps.begin();
    for ( ; attIt != mAttributeMaps.end(); ++attIt )
    {
      currentY += fontAscentMillimeters( mContentFont );
      currentY += mLineTextDistance;

      const QgsAttributeMap &currentAttributeMap = *attIt;
      QString str = currentAttributeMap[ columnIt.key()].toString();
      drawText( painter, currentX, currentY, str, mContentFont );
      currentY += mLineTextDistance;
      currentY += mGridStrokeWidth;
    }

    currentX += mMaxColumnWidthMap[columnIt.key()];
    currentX += mLineTextDistance;
    currentX += mGridStrokeWidth;
  }

  //and the borders
  if ( mShowGrid )
  {
    QPen gridPen;
    gridPen.setWidthF( mGridStrokeWidth );
    gridPen.setColor( mGridColor );
    gridPen.setJoinStyle( Qt::MiterJoin );
    painter->setPen( gridPen );
    drawHorizontalGridLines( painter, mAttributeMaps.size() );
    drawVerticalGridLines( painter, mMaxColumnWidthMap );
  }

  //draw frame and selection boxes if necessary
  drawFrame( painter );
  if ( isSelected() )
  {
    drawSelectionBoxes( painter );
  }
}

void QgsComposerTable::setHeaderFont( const QFont& f )
{
  mHeaderFont = f;
  //since font attributes have changed, we need to recalculate the table size
  adjustFrameToSize();
}

void QgsComposerTable::setContentFont( const QFont& f )
{
  mContentFont = f;
  //since font attributes have changed, we need to recalculate the table size
  adjustFrameToSize();
}

void QgsComposerTable::adjustFrameToSize()
{
  //check how much space each column needs
  if ( !calculateMaxColumnWidths( mMaxColumnWidthMap, mAttributeMaps ) )
  {
    return;
  }
  //adapt item frame to max width / height
  adaptItemFrame( mMaxColumnWidthMap, mAttributeMaps );

  repaint();
}

bool QgsComposerTable::tableWriteXML( QDomElement& elem, QDomDocument & doc ) const
{
  elem.setAttribute( "lineTextDist", QString::number( mLineTextDistance ) );
  elem.setAttribute( "headerFont", mHeaderFont.toString() );
  elem.setAttribute( "contentFont", mContentFont.toString() );
  elem.setAttribute( "gridStrokeWidth", QString::number( mGridStrokeWidth ) );
  elem.setAttribute( "gridColorRed", mGridColor.red() );
  elem.setAttribute( "gridColorGreen", mGridColor.green() );
  elem.setAttribute( "gridColorBlue", mGridColor.blue() );
  elem.setAttribute( "showGrid", mShowGrid );
  return _writeXML( elem, doc );
}

bool QgsComposerTable::tableReadXML( const QDomElement& itemElem, const QDomDocument& doc )
{
  if ( itemElem.isNull() )
  {
    return false;
  }

  mHeaderFont.fromString( itemElem.attribute( "headerFont", "" ) );
  mContentFont.fromString( itemElem.attribute( "contentFont", "" ) );
  mLineTextDistance = itemElem.attribute( "lineTextDist", "1.0" ).toDouble();
  mGridStrokeWidth = itemElem.attribute( "gridStrokeWidth", "0.5" ).toDouble();
  mShowGrid = itemElem.attribute( "showGrid", "1" ).toInt();

  //grid color
  int gridRed = itemElem.attribute( "gridColorRed", "0" ).toInt();
  int gridGreen = itemElem.attribute( "gridColorGreen", "0" ).toInt();
  int gridBlue = itemElem.attribute( "gridColorBlue", "0" ).toInt();
  mGridColor = QColor( gridRed, gridGreen, gridBlue );

  //restore general composer item properties
  QDomNodeList composerItemList = itemElem.elementsByTagName( "ComposerItem" );
  if ( composerItemList.size() > 0 )
  {
    QDomElement composerItemElem = composerItemList.at( 0 ).toElement();
    _readXML( composerItemElem, doc );
  }
  return true;
}

bool QgsComposerTable::calculateMaxColumnWidths( QMap<int, double>& maxWidthMap, const QList<QgsAttributeMap>& attributeMaps ) const
{
  maxWidthMap.clear();
  QMap<int, QString> headerMap = getHeaderLabels();
  QMap<int, QString>::const_iterator headerIt = headerMap.constBegin();
  for ( ; headerIt != headerMap.constEnd(); ++headerIt )
  {
    maxWidthMap.insert( headerIt.key(), textWidthMillimeters( mHeaderFont, headerIt.value() ) );
  }

  //go through all the attributes and adapt the max width values
  QList<QgsAttributeMap>::const_iterator attIt = attributeMaps.constBegin();

  double currentAttributeTextWidth;

  for ( ; attIt != attributeMaps.constEnd(); ++attIt )
  {
    QgsAttributeMap::const_iterator attIt2 = attIt->constBegin();
    for ( ; attIt2 != attIt->constEnd(); ++attIt2 )
    {
      currentAttributeTextWidth = textWidthMillimeters( mContentFont, attIt2.value().toString() );
      if ( currentAttributeTextWidth > maxWidthMap[ attIt2.key()] )
      {
        maxWidthMap[ attIt2.key()] = currentAttributeTextWidth;
      }
    }
  }
  return true;
}

void QgsComposerTable::adaptItemFrame( const QMap<int, double>& maxWidthMap, const QList<QgsAttributeMap>& attributeMaps )
{
  //calculate height
  int n = attributeMaps.size();
  double totalHeight = fontAscentMillimeters( mHeaderFont )
                       + n * fontAscentMillimeters( mContentFont )
                       + ( n + 1 ) * mLineTextDistance * 2
                       + ( n + 2 ) * mGridStrokeWidth;

  //adapt frame to total width
  double totalWidth = 0;
  QMap<int, double>::const_iterator maxColWidthIt = maxWidthMap.constBegin();
  for ( ; maxColWidthIt != maxWidthMap.constEnd(); ++maxColWidthIt )
  {
    totalWidth += maxColWidthIt.value();
  }
  totalWidth += ( 2 * maxWidthMap.size() * mLineTextDistance );
  totalWidth += ( maxWidthMap.size() + 1 ) * mGridStrokeWidth;

  QgsComposerItem::setSceneRect( QRectF( pos().x(), pos().y(), totalWidth, totalHeight ) );
}

void QgsComposerTable::drawHorizontalGridLines( QPainter* p, int nAttributes )
{
  //horizontal lines
  double halfGridStrokeWidth = mGridStrokeWidth / 2.0;
  double currentY = halfGridStrokeWidth;
  p->drawLine( QPointF( halfGridStrokeWidth, currentY ), QPointF( rect().width() - halfGridStrokeWidth, currentY ) );
  currentY += mGridStrokeWidth;
  currentY += ( fontAscentMillimeters( mHeaderFont ) + 2 * mLineTextDistance );
  for ( int i = 0; i < nAttributes; ++i )
  {
    p->drawLine( QPointF( halfGridStrokeWidth, currentY ), QPointF( rect().width() - halfGridStrokeWidth, currentY ) );
    currentY += mGridStrokeWidth;
    currentY += ( fontAscentMillimeters( mContentFont ) + 2 * mLineTextDistance );
  }
  p->drawLine( QPointF( halfGridStrokeWidth, currentY ), QPointF( rect().width() - halfGridStrokeWidth, currentY ) );
}

void QgsComposerTable::drawVerticalGridLines( QPainter* p, const QMap<int, double>& maxWidthMap )
{
  //vertical lines
  double halfGridStrokeWidth = mGridStrokeWidth / 2.0;
  double currentX = halfGridStrokeWidth;
  p->drawLine( QPointF( currentX, halfGridStrokeWidth ), QPointF( currentX, rect().height() - halfGridStrokeWidth ) );
  currentX += mGridStrokeWidth;
  QMap<int, double>::const_iterator maxColWidthIt = maxWidthMap.constBegin();
  for ( ; maxColWidthIt != maxWidthMap.constEnd(); ++maxColWidthIt )
  {
    currentX += ( maxColWidthIt.value() + 2 * mLineTextDistance );
    p->drawLine( QPointF( currentX, halfGridStrokeWidth ), QPointF( currentX, rect().height() - halfGridStrokeWidth ) );
    currentX += mGridStrokeWidth;
  }
}



