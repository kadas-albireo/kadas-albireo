/***************************************************************************
                         qgssinglebandcolordatarenderer.h
                         --------------------------------
    begin                : January 2012
    copyright            : (C) 2012 by Marco Hugentobler
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

#ifndef QGSSINGLEBANDCOLORDATARENDERER_H
#define QGSSINGLEBANDCOLORDATARENDERER_H

#include "qgsrasterrenderer.h"

class QDomElement;

/** \ingroup core
  * Raster renderer pipe for single band color.
  */
class CORE_EXPORT QgsSingleBandColorDataRenderer: public QgsRasterRenderer
{
  public:
    QgsSingleBandColorDataRenderer( QgsRasterInterface* input, int band );
    ~QgsSingleBandColorDataRenderer();
    QgsRasterInterface * clone() const OVERRIDE;

    static QgsRasterRenderer* create( const QDomElement& elem, QgsRasterInterface* input );

    bool setInput( QgsRasterInterface* input ) OVERRIDE;

    QgsRasterBlock* block( int bandNo, const QgsRectangle & extent, int width, int height ) OVERRIDE;

    void writeXML( QDomDocument& doc, QDomElement& parentElem ) const OVERRIDE;

    QList<int> usesBands() const OVERRIDE;

  private:
    int mBand;
};

#endif // QGSSINGLEBANDCOLORDATARENDERER_H
