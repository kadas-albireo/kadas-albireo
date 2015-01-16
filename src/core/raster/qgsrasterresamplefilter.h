/***************************************************************************
                         qgsrasterresamplefilter.h
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

#ifndef QGSRASTERRESAMPLEFILTER_H
#define QGSRASTERRESAMPLEFILTER_H

#include "qgsrasterdataprovider.h"
#include "qgsrasterinterface.h"

class QgsRasterResampler;

class QDomElement;

/** \ingroup core
  * Resample filter pipe for rasters.
  */
class CORE_EXPORT QgsRasterResampleFilter : public QgsRasterInterface
{
  public:
    QgsRasterResampleFilter( QgsRasterInterface *input = 0 );
    ~QgsRasterResampleFilter();

    QgsRasterInterface * clone() const override;

    int bandCount() const override;

    QGis::DataType dataType( int bandNo ) const override;

    bool setInput( QgsRasterInterface* input ) override;

    QgsRasterBlock *block( int bandNo, const QgsRectangle &extent, int width, int height ) override;

    /**Set resampler for zoomed in scales. Takes ownership of the object*/
    void setZoomedInResampler( QgsRasterResampler* r );
    const QgsRasterResampler* zoomedInResampler() const { return mZoomedInResampler; }

    /**Set resampler for zoomed out scales. Takes ownership of the object*/
    void setZoomedOutResampler( QgsRasterResampler* r );
    const QgsRasterResampler* zoomedOutResampler() const { return mZoomedOutResampler; }

    void setMaxOversampling( double os ) { mMaxOversampling = os; }
    double maxOversampling() const { return mMaxOversampling; }

    void writeXML( QDomDocument& doc, QDomElement& parentElem ) const override;

    /**Sets base class members from xml. Usually called from create() methods of subclasses*/
    void readXML( const QDomElement& filterElem ) override;

  protected:
    /**Resampler used if screen resolution is higher than raster resolution (zoomed in). 0 means no resampling (nearest neighbour)*/
    QgsRasterResampler* mZoomedInResampler;
    /**Resampler used if raster resolution is higher than raster resolution (zoomed out). 0 mean no resampling (nearest neighbour)*/
    QgsRasterResampler* mZoomedOutResampler;

    /**Maximum boundary for oversampling (to avoid too much data traffic). Default: 2.0*/
    double mMaxOversampling;

  private:
};

#endif // QGSRASTERRESAMPLEFILTER_H
