/***************************************************************************
                          qgssymbol.h  -  description
                             -------------------
    begin                : Sat Jun 22 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc.com
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

#ifndef QGSSYMBOL_H
#define QGSSYMBOL_H

#include <iostream>

#include "qgis.h"
#include <qbrush.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qdom.h>

class QString;

/**Encapsulates settings for drawing (QPen, QBrush, Point symbol) and classification
 (lower value, upper value)*/
class QgsSymbol{

 public:
    /**Constructor*/
    QgsSymbol(QGis::VectorType t, QString lvalue="", QString uvalue="", QString label="");
    /**Constructor*/
    QgsSymbol(QGis::VectorType t, QString lvalue, QString uvalue, QString label, QColor c);
    /**old constructors*/
    QgsSymbol();
    QgsSymbol(QColor c);
    /**Sets the brush*/
    virtual void setBrush(QBrush b);
    /**Gets a reference to m_brush, Don't use the brush to change color/style */
    virtual const QBrush& brush() const;
    /**Set the color*/
    virtual void setColor(QColor c);
    /**Get the current color*/
    virtual QColor color() const;
    /**Get the fill color*/
    virtual QColor fillColor() const;
    /**Sets the fill color*/
    virtual void setFillColor(QColor c);
    /**Get the line width*/
    virtual int lineWidth() const;
    /**Sets the line width*/
    virtual void setLineWidth(int w);
    /**Sets the pen*/
    virtual void setPen(QPen p);
    /**Gets a reference to m_pen. Don't use the pen to change color/style  */
    virtual const QPen& pen() const;

    /**Set the line (pen) style*/
    virtual void setLineStyle(Qt::PenStyle s);
    /**Set the fill (brush) style*/
    virtual void setFillStyle(Qt::BrushStyle s);
    virtual void setLowerValue(QString value);
    virtual QString lowerValue() const;
    virtual void setUpperValue(QString value);
    virtual QString upperValue() const;
    virtual void setLabel(QString label);
    virtual QString label() const;

    /**Set point symbol from name*/
    virtual void setNamedPointSymbol(QString name);
    /**Get point symbol*/
    virtual QString pointSymbolName() const;
    /**Set size*/
    virtual void setPointSize(int s);
    /**Get size*/
    virtual int pointSize() const;
    //! Destructor
    virtual ~QgsSymbol();

    //! Get a little icon for the legend
    virtual QPixmap getLineSymbolAsPixmap();

    //! Get a little icon for the legend
    virtual QPixmap getPolygonSymbolAsPixmap();
    
    /** Get QPixmap representation of point symbol with current settings
      */
    virtual QPixmap getPointSymbolAsPixmap( double widthScale = 1., 
	               bool selected = false, QColor selectionColor = Qt::yellow );

    /**Writes the contents of the symbol to a configuration file
     @ return true in case of success*/
    virtual bool writeXML( QDomNode & item, QDomDocument & document ) const;
    /**Reads the contents of the symbol from a configuration file
     @ return true in case of success*/
    virtual bool readXML( QDomNode & symbol );
    /**Returns if this symbol is point/ line or polygon*/
    QGis::VectorType type() const {return mType;}

 protected:
    /**Lower value for classification*/
    QString mLowerValue;
    /**Upper value for classification*/
    QString mUpperValue;
    QString mLabel;
    /**Vector type (point, line, polygon)*/
    QGis::VectorType mType;

    QPen mPen;
    QBrush mBrush;
    /* Point symbol name */
    QString mPointSymbolName;
    /* Point size */
    int mPointSize; 

    /* TODO Because for printing we always need a symbol without oversampling but with line width scale, 
     *      we keep also separate picture with line width scale */

    //
    //
    // NOTE THE LOGIC OF THESE MEMBER VARS NEED TO BE REVISITED NOW THAT
    // I HAVE REMOVED SVG OVERSAMPLING (NEEDED IN QT3 WITH POOR SVG SUPPORT)
    // Tim Sutton 2006 XXX FIXME
    //
    //

    
    /* Point symbol cache  */
    QPixmap mPointSymbolPixmap;

    /* Point symbol cache  */
    QPixmap mPointSymbolPixmapSelected;

    /* Current line width scale used by mPointSymbolVectorPixmap */
    double mWidthScale;
    
    /* Point symbol cache but with line width scale mWidthScale */
    QPixmap mPointSymbolPixmap2;
    QPixmap mPointSymbolPixmapSelected2;
    
    /* Create point symbol mPointSymbolPixmap/mPointSymbolPixmap cache */
    void cache(  QColor selectionColor );

    /* Create point symbol mPointSymbolPixmap2 cache */
    void cache2( double widthScale, QColor selectionColor );

    /* mPointSymbolPixmap/mPointSymbolPixmap cache updated */
    bool mCacheUpToDate;

    /* mPointSymbolPixmap2 cache updated */
    bool mCacheUpToDate2;

    /* Selection color used in cache */
    QColor mSelectionColor;
    QColor mSelectionColor2;
};

inline void QgsSymbol::setBrush(QBrush b)
{
    mBrush=b;
}

inline const QBrush& QgsSymbol::brush() const
{
    return mBrush;
}

inline void QgsSymbol::setPen(QPen p)
{
    mPen=p;
}

inline const QPen& QgsSymbol::pen() const
{
    return mPen;
}

inline void QgsSymbol::setLowerValue(QString value)
{
    mLowerValue=value;
}

inline QString QgsSymbol::lowerValue() const
{
    return mLowerValue;
}

inline void QgsSymbol::setUpperValue(QString value)
{
    mUpperValue=value;
}

inline QString QgsSymbol::upperValue() const
{
    return mUpperValue;
}

inline void QgsSymbol::setLabel(QString label)
{
    mLabel=label;
}

inline QString QgsSymbol::label() const
{
    return mLabel;
}

#endif // QGSSYMBOL_H


