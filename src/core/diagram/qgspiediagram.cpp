/***************************************************************************
    qgspiediagram.cpp
    ---------------------
    begin                : March 2011
    copyright            : (C) 2011 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgspiediagram.h"
#include "qgsdiagramrendererv2.h"
#include "qgsrendercontext.h"
#include "qgsexpression.h"

#include <QPainter>


QgsPieDiagram::QgsPieDiagram()
{
  mCategoryBrush.setStyle( Qt::SolidPattern );
  mPen.setStyle( Qt::SolidLine );
}

QgsPieDiagram::~QgsPieDiagram()
{
}

QgsDiagram* QgsPieDiagram::clone() const
{
  return new QgsPieDiagram( *this );
}

QSizeF QgsPieDiagram::diagramSize( const QgsFeature& feature, const QgsRenderContext& c, const QgsDiagramSettings& s, const QgsDiagramInterpolationSettings& is )
{
  Q_UNUSED( c );

  QVariant attrVal;
  if ( is.classificationAttributeIsExpression )
  {
    QgsExpression* expression = getExpression( is.classificationAttributeExpression, feature.fields() );
    attrVal = expression->evaluate( feature );
  }
  else
  {
    attrVal = feature.attributes()[is.classificationAttribute];
  }

  if ( !attrVal.isValid() )
  {
    return QSizeF(); //zero size if attribute is missing
  }

  double scaledValue = attrVal.toDouble();
  double scaledLowerValue = is.lowerValue;
  double scaledUpperValue = is.upperValue;
  double scaledLowerSizeWidth = is.lowerSize.width();
  double scaledLowerSizeHeight = is.lowerSize.height();
  double scaledUpperSizeWidth = is.upperSize.width();
  double scaledUpperSizeHeight = is.upperSize.height();

  // interpolate the squared value if scale by area
  if ( s.scaleByArea )
  {
    scaledValue = sqrt( scaledValue );
    scaledLowerValue = sqrt( scaledLowerValue );
    scaledUpperValue = sqrt( scaledUpperValue );
    scaledLowerSizeWidth = sqrt( scaledLowerSizeWidth );
    scaledLowerSizeHeight = sqrt( scaledLowerSizeHeight );
    scaledUpperSizeWidth = sqrt( scaledUpperSizeWidth );
    scaledUpperSizeHeight = sqrt( scaledUpperSizeHeight );
  }

  //interpolate size
  double scaledRatio = ( scaledValue - scaledLowerValue ) / ( scaledUpperValue - scaledLowerValue );

  QSizeF size = QSizeF( is.upperSize.width() * scaledRatio + is.lowerSize.width() * ( 1 - scaledRatio ),
                        is.upperSize.height() * scaledRatio + is.lowerSize.height() * ( 1 - scaledRatio ) );

  // Scale, if extension is smaller than the specified minimum
  if ( size.width() <= s.minimumSize && size.height() <= s.minimumSize )
  {
    bool p = false; // preserve height == width
    if ( size.width() == size.height() )
      p = true;

    size.scale( s.minimumSize, s.minimumSize, Qt::KeepAspectRatio );

    // If height == width, recover here (overwrite floating point errors)
    if ( p )
      size.setWidth( size.height() );
  }

  return size;
}

QSizeF QgsPieDiagram::diagramSize( const QgsAttributes& attributes, const QgsRenderContext& c, const QgsDiagramSettings& s )
{
  Q_UNUSED( c );
  Q_UNUSED( attributes );
  return s.size;
}

int  QgsPieDiagram::sCount = 0;

void QgsPieDiagram::renderDiagram( const QgsFeature& feature, QgsRenderContext& c, const QgsDiagramSettings& s, const QPointF& position )
{
  QPainter* p = c.painter();
  if ( !p )
  {
    return;
  }

  //get sum of values
  QList<double> values;
  double currentVal = 0;
  double valSum = 0;
  int valCount = 0;

  QList<QString>::const_iterator catIt = s.categoryAttributes.constBegin();
  for ( ; catIt != s.categoryAttributes.constEnd(); ++catIt )
  {
    QgsExpression* expression = getExpression( *catIt, feature.fields() );
    currentVal = expression->evaluate( feature ).toDouble();
    values.push_back( currentVal );
    valSum += currentVal;
    if ( currentVal ) valCount++;
  }

  //draw the slices
  double totalAngle = 0;
  double currentAngle;

  //convert from mm / map units to painter units
  QSizeF spu = sizePainterUnits( s.size, s, c );
  double w = spu.width();
  double h = spu.height();

  double baseX = position.x();
  double baseY = position.y() - h;

  mPen.setColor( s.penColor );
  setPenWidth( mPen, s, c );
  p->setPen( mPen );

  // there are some values > 0 available
  if ( valSum > 0 )
  {
    QList<double>::const_iterator valIt = values.constBegin();
    QList< QColor >::const_iterator colIt = s.categoryColors.constBegin();
    for ( ; valIt != values.constEnd(); ++valIt, ++colIt )
    {
      if ( *valIt )
      {
        currentAngle = *valIt / valSum * 360 * 16;
        mCategoryBrush.setColor( *colIt );
        p->setBrush( mCategoryBrush );
        // if only 1 value is > 0, draw a circle
        if ( valCount == 1 )
        {
          p->drawEllipse( baseX, baseY, w, h );
        }
        else
        {
          p->drawPie( baseX, baseY, w, h, totalAngle + s.angleOffset, currentAngle );
        }
        totalAngle += currentAngle;
      }
    }
  }
  else // valSum > 0
  {
    // draw empty circle if no values are defined at all
    mCategoryBrush.setColor( Qt::transparent );
    p->setBrush( mCategoryBrush );
    p->drawEllipse( baseX, baseY, w, h );
  }
}
