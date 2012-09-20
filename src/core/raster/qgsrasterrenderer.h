/***************************************************************************
                         qgsrasterrenderer.h
                         -------------------
    begin                : December 2011
    copyright            : (C) 2011 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSRASTERRENDERER_H
#define QGSRASTERRENDERER_H

#include "qgsrasterinterface.h"
#include "qgsrasterdataprovider.h"
#include <QPair>

#include "cpl_conv.h"

class QPainter;
class QgsMapToPixel;
class QgsRasterResampler;
class QgsRasterProjector;
class QgsRasterTransparency;
struct QgsRasterViewPort;

class QDomElement;

class CORE_EXPORT QgsRasterRenderer : public QgsRasterInterface
{
  public:
    QgsRasterRenderer( QgsRasterInterface* input = 0, const QString& type = "" );
    virtual ~QgsRasterRenderer();

    QgsRasterInterface * clone() const = 0;

    virtual int bandCount() const;

    virtual QgsRasterInterface::DataType dataType( int bandNo ) const;

    virtual QString type() const { return mType; }

    virtual bool setInput( QgsRasterInterface* input );

    virtual void * readBlock( int bandNo, QgsRectangle  const & extent, int width, int height )
    {
      Q_UNUSED( bandNo ); Q_UNUSED( extent ); Q_UNUSED( width ); Q_UNUSED( height );
      return 0;
    }

    bool usesTransparency() const;

    void setOpacity( double opacity ) { mOpacity = opacity; }
    double opacity() const { return mOpacity; }

    void setRasterTransparency( QgsRasterTransparency* t );
    const QgsRasterTransparency* rasterTransparency() const { return mRasterTransparency; }

    void setAlphaBand( int band ) { mAlphaBand = band; }
    int alphaBand() const { return mAlphaBand; }

    void setInvertColor( bool invert ) { mInvertColor = invert; }
    bool invertColor() const { return mInvertColor; }

    /**Get symbology items if provided by renderer*/
    virtual void legendSymbologyItems( QList< QPair< QString, QColor > >& symbolItems ) const { Q_UNUSED( symbolItems ); }

    virtual void writeXML( QDomDocument&, QDomElement& ) const {}
    /**Sets base class members from xml. Usually called from create() methods of subclasses*/
    void readXML( const QDomElement& rendererElem );

    /**Returns a list of band numbers used by the renderer*/
    virtual QList<int> usesBands() const { return QList<int>(); }

  protected:

    /**Write upper class info into rasterrenderer element (called by writeXML method of subclasses)*/
    void _writeXML( QDomDocument& doc, QDomElement& rasterRendererElem ) const;

    QString mType;

    /**Global alpha value (0-1)*/
    double mOpacity;
    /**Raster transparency per color or value. Overwrites global alpha value*/
    QgsRasterTransparency* mRasterTransparency;
    /**Read alpha value from band. Is combined with value from raster transparency / global alpha value.
        Default: -1 (not set)*/
    int mAlphaBand;

    bool mInvertColor;

    /**Maximum boundary for oversampling (to avoid too much data traffic). Default: 2.0*/
    double mMaxOversampling;
};

#endif // QGSRASTERRENDERER_H
