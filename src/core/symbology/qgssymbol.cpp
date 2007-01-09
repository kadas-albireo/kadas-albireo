/***************************************************************************
                          QgsSymbol.cpp  -  description
                             -------------------
    begin                : Sun Aug 11 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc dot com
       Romans 3:23=>Romans 6:23=>Romans 5:8=>Romans 10:9,10=>Romans 12
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */
#include <cmath>
#include <iostream>

#include "qgssymbol.h"
#include "qgssymbologyutils.h"
#include "qgsmarkercatalogue.h"

#include <QPainter>
#include <QDomNode>
#include <QDomDocument>

QgsSymbol::QgsSymbol(QGis::VectorType t, QString lvalue, QString uvalue, QString label) : 
      mLowerValue(lvalue),
      mUpperValue(uvalue),
      mLabel(label),
      mType(t),
      mPointSymbolName( "hard:circle" ),
      mPointSize( 6 ),
      mPointSymbolImage(1,1, QImage::Format_ARGB32_Premultiplied),
      mWidthScale(1.0),
      mCacheUpToDate( false ),
      mCacheUpToDate2( false )
{}


QgsSymbol::QgsSymbol(QGis::VectorType t, QString lvalue, QString uvalue, QString label, QColor c) : 
      mLowerValue(lvalue),
      mUpperValue(uvalue),
      mLabel(label),
      mType(t),
      mPen( c ),
      mBrush( c ),
      mPointSymbolName( "hard:circle" ),
      mPointSize( 6 ),
      mPointSymbolImage(1,1, QImage::Format_ARGB32_Premultiplied),
      mWidthScale(1.0),
      mCacheUpToDate( false ),
      mCacheUpToDate2( false )
{}

QgsSymbol::QgsSymbol()
    : mPointSymbolName( "hard:circle" ),
      mPointSize( 6 ),
      mPointSymbolImage(1,1, QImage::Format_ARGB32_Premultiplied),
      mWidthScale(1.0),
      mCacheUpToDate( false ),
      mCacheUpToDate2( false )
{}


QgsSymbol::QgsSymbol(QColor c)
    : mPen( c ),
      mBrush( c ),
      mPointSymbolName( "hard:circle" ),
      mPointSize( 6 ),
      mPointSymbolImage(1,1, QImage::Format_ARGB32_Premultiplied),
      mWidthScale(1.0),
      mCacheUpToDate( false ),
      mCacheUpToDate2( false )
{}

QgsSymbol::QgsSymbol(const QgsSymbol& s)
{
  if (this != &s)
  {
    mLowerValue = s.mLowerValue;
    mUpperValue = s.mUpperValue;
    mLabel = s.mLabel;
    mType = s.mType;
    mPen = s.mPen;
    mBrush = s.mBrush;
    mPointSymbolName = s.mPointSymbolName;
    mPointSize = s.mPointSize;
    mPointSymbolImage = s.mPointSymbolImage;
    mPointSymbolImageSelected = s.mPointSymbolImageSelected;
    mWidthScale = s.mWidthScale;
    mPointSymbolImage2 = s.mPointSymbolImage2;
    mPointSymbolImageSelected2 = s.mPointSymbolImageSelected2;
    mCacheUpToDate = s.mCacheUpToDate;
    mCacheUpToDate2 = s.mCacheUpToDate2;
    mSelectionColor = s.mSelectionColor;
    mSelectionColor2 = s.mSelectionColor2;
  }
}

QgsSymbol::~QgsSymbol()
{

}


QColor QgsSymbol::color() const
{
  return mPen.color();
}

void QgsSymbol::setColor(QColor c)
{
  mPen.setColor(c);
  mCacheUpToDate = mCacheUpToDate2 = false;
}

QColor QgsSymbol::fillColor() const
{
  return mBrush.color();
}

void QgsSymbol::setFillColor(QColor c)
{
  mBrush.setColor(c);
  mCacheUpToDate = mCacheUpToDate2 = false;
}

int QgsSymbol::lineWidth() const
{
  return mPen.width();
}

void QgsSymbol::setLineWidth(int w)
{
  mPen.setWidth(w);
  mCacheUpToDate = mCacheUpToDate2 = false;
}

void QgsSymbol::setLineStyle( Qt::PenStyle s )
{
  mPen.setStyle(s);
  mCacheUpToDate = mCacheUpToDate2 = false;
}

void QgsSymbol::setFillStyle( Qt::BrushStyle s )
{
  mBrush.setStyle(s);
  mCacheUpToDate = mCacheUpToDate2 = false;
}

void QgsSymbol::setNamedPointSymbol(QString name)
{
    mPointSymbolName = name;
    mCacheUpToDate = mCacheUpToDate2 = false;
}

QString QgsSymbol::pointSymbolName() const
{
    return mPointSymbolName;
}

void QgsSymbol::setPointSize(int s)
{
    if ( s < 3 )  
	mPointSize = 3;
    else 
    	mPointSize = s;

    mCacheUpToDate = mCacheUpToDate2 = false;
}

int QgsSymbol::pointSize() const
{
    return mPointSize;
}


QImage QgsSymbol::getLineSymbolAsImage()
{
    QImage img(15, 15, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(255,255,255,0).rgba());
    QPainter p(&img);
    p.setPen(mPen);
    p.drawLine(0, 0, 15, 15);
    return img; //this is ok because of qts sharing mechanism
}

QImage QgsSymbol::getPolygonSymbolAsImage()
{
   QImage img(15, 15, QImage::Format_ARGB32_Premultiplied);
   img.fill(QColor(255,255,255,0).rgba());
   QPainter p(&img);
   p.setPen(mPen);
   p.setBrush(mBrush);
   p.drawRect(0, 0, 15, 15);
   return img; //this is ok because of qts sharing mechanism 
}

QImage QgsSymbol::getPointSymbolAsImage(  double widthScale,
               bool selected, QColor selectionColor )
{

	if ( !mCacheUpToDate 
	     || ( selected && mSelectionColor != selectionColor ) )
	{
	    if ( selected ) {
	        cache(  selectionColor );
	    } else {
	        cache(  mSelectionColor );
	    }
	}

	if ( selected )
  {
	    return mPointSymbolImageSelected;
	}
        
 return mPointSymbolImage;
}

void QgsSymbol::cache(  QColor selectionColor )
{
    QPen pen = mPen;
    pen.setColor ( selectionColor ); 
    QBrush brush = mBrush;
    // For symbols that have a different coloured border, the line
    // below causes the fill colour to be wrong for the print
    // composer. Not sure why... 
    // brush.setColor ( selectionColor ); 

    mPointSymbolImage = QgsMarkerCatalogue::instance()->pixmapMarker ( mPointSymbolName, mPointSize,
	                        mPen, mBrush );
    
    mPointSymbolImageSelected = QgsMarkerCatalogue::instance()->pixmapMarker ( 
	     mPointSymbolName, mPointSize, pen, brush );

    mSelectionColor = selectionColor;
    mCacheUpToDate = true;
}

void QgsSymbol::cache2( double widthScale, QColor selectionColor )
{
    //std::cerr << "QgsSymbol::cache2 widthScale = " << widthScale << std::endl;

    QPen pen = mPen;
    pen.setWidth ( (int) ( widthScale * pen.width() ) );

    
    mPointSymbolImage2 = QgsMarkerCatalogue::instance()->pixmapMarker ( mPointSymbolName, mPointSize,
	                        pen, mBrush, false );

    QBrush brush = mBrush;
    brush.setColor ( selectionColor ); 
    pen.setColor ( selectionColor ); 

    mPointSymbolImageSelected2 = QgsMarkerCatalogue::instance()->pixmapMarker ( 
	               mPointSymbolName, mPointSize, pen, brush,  false );

    mSelectionColor2 = selectionColor;
    
    mWidthScale = widthScale;
    mCacheUpToDate2 = true;
}

bool QgsSymbol::writeXML( QDomNode & item, QDomDocument & document ) const
{
    bool returnval = false;
    returnval = true; // no error checking yet
    QDomElement symbol=document.createElement("symbol");
    item.appendChild(symbol);

    QDomElement lowervalue=document.createElement("lowervalue");
    QDomText lowervaluetxt=document.createTextNode(mLowerValue);
    symbol.appendChild(lowervalue);
    lowervalue.appendChild(lowervaluetxt);

    QDomElement uppervalue=document.createElement("uppervalue");
    QDomText uppervaluetxt=document.createTextNode(mUpperValue);
    symbol.appendChild(uppervalue);
    uppervalue.appendChild(uppervaluetxt);

    QDomElement label=document.createElement("label");
    QDomText labeltxt=document.createTextNode(mLabel);
    symbol.appendChild(label);
    label.appendChild(labeltxt);

    QDomElement pointsymbol=document.createElement("pointsymbol");
    QDomText pointsymboltxt=document.createTextNode(pointSymbolName());
    symbol.appendChild(pointsymbol);
    pointsymbol.appendChild(pointsymboltxt);

    QDomElement pointsize=document.createElement("pointsize");
    QDomText pointsizetxt=document.createTextNode( QString::number(pointSize()) );
    symbol.appendChild(pointsize);
    pointsize.appendChild(pointsizetxt);

    QDomElement outlinecolor=document.createElement("outlinecolor");
    outlinecolor.setAttribute("red",QString::number(mPen.color().red()));
    outlinecolor.setAttribute("green",QString::number(mPen.color().green()));
    outlinecolor.setAttribute("blue",QString::number(mPen.color().blue()));
    symbol.appendChild(outlinecolor);
    QDomElement outlinestyle=document.createElement("outlinestyle");
    QDomText outlinestyletxt=document.createTextNode(QgsSymbologyUtils::penStyle2QString(mPen.style()));
    outlinestyle.appendChild(outlinestyletxt);
    symbol.appendChild(outlinestyle);
    QDomElement outlinewidth=document.createElement("outlinewidth");
    QDomText outlinewidthtxt=document.createTextNode(QString::number(mPen.width()));
    outlinewidth.appendChild(outlinewidthtxt);
    symbol.appendChild(outlinewidth);
    QDomElement fillcolor=document.createElement("fillcolor");
    fillcolor.setAttribute("red",QString::number(mBrush.color().red()));
    fillcolor.setAttribute("green",QString::number(mBrush.color().green()));
    fillcolor.setAttribute("blue",QString::number(mBrush.color().blue()));
    symbol.appendChild(fillcolor);
    QDomElement fillpattern=document.createElement("fillpattern");
    QDomText fillpatterntxt=document.createTextNode(QgsSymbologyUtils::brushStyle2QString(mBrush.style()));
    fillpattern.appendChild(fillpatterntxt);
    symbol.appendChild(fillpattern);
    fillpattern.appendChild(fillpatterntxt);
    
    return returnval;
}

bool QgsSymbol::readXML( QDomNode & synode )
{
    // Legacy project file formats didn't have support for pointsymbol nor
    // pointsize DOM elements.  Therefore we should check whether these
    // actually exist.

    QDomNode lvalnode = synode.namedItem("lowervalue");
    if( ! lvalnode.isNull() )
    {
	QDomElement lvalelement = lvalnode.toElement();
	mLowerValue=lvalelement.text();
    }

    QDomNode uvalnode = synode.namedItem("uppervalue");
    if( ! uvalnode.isNull() )
    {
	QDomElement uvalelement = uvalnode.toElement();
	mUpperValue=uvalelement.text();
    }

    QDomNode labelnode = synode.namedItem("label");
    if( ! labelnode.isNull() )
    {
	QDomElement labelelement = labelnode.toElement();
	mLabel=labelelement.text();
    }

    QDomNode psymbnode = synode.namedItem("pointsymbol");

    if ( ! psymbnode.isNull() )
    {
        QDomElement psymbelement = psymbnode.toElement();
        setNamedPointSymbol( psymbelement.text() );
    }
    
    QDomNode psizenode = synode.namedItem("pointsize");
    
    if ( !  psizenode.isNull() )
    {
        QDomElement psizeelement = psizenode.toElement();
        setPointSize( psizeelement.text().toInt() );
    }

    QDomNode outlcnode = synode.namedItem("outlinecolor");
    QDomElement oulcelement = outlcnode.toElement();
    int red = oulcelement.attribute("red").toInt();
    int green = oulcelement.attribute("green").toInt();
    int blue = oulcelement.attribute("blue").toInt();
    setColor(QColor(red, green, blue));

    QDomNode outlstnode = synode.namedItem("outlinestyle");
    QDomElement outlstelement = outlstnode.toElement();
    setLineStyle(QgsSymbologyUtils::qString2PenStyle(outlstelement.text()));

    QDomNode outlwnode = synode.namedItem("outlinewidth");
    QDomElement outlwelement = outlwnode.toElement();
    setLineWidth(outlwelement.text().toInt());

    QDomNode fillcnode = synode.namedItem("fillcolor");
    QDomElement fillcelement = fillcnode.toElement();
    red = fillcelement.attribute("red").toInt();
    green = fillcelement.attribute("green").toInt();
    blue = fillcelement.attribute("blue").toInt();
    setFillColor(QColor(red, green, blue));

    QDomNode fillpnode = synode.namedItem("fillpattern");
    QDomElement fillpelement = fillpnode.toElement();
    setFillStyle(QgsSymbologyUtils::qString2BrushStyle(fillpelement.text()));

    return true;
}
