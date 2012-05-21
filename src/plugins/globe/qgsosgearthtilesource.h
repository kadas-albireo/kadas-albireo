/***************************************************************************
    qgsosgearthtilesource.h
    ---------------------
    begin                : August 2010
    copyright            : (C) 2010 by Pirmin Kalberer
    email                : pka at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef OSGEARTH_DRIVER_QGIS_DRIVEROPTIONS
#define OSGEARTH_DRIVER_QGIS_DRIVEROPTIONS 1

#include "qgsmaprenderer.h"

#include <QImage>
class QgisInterface;

#include <osgEarth/Common>
#include <osgEarth/TileSource>

using namespace osgEarth;

namespace osgEarth
{
  namespace Drivers
  {
    class QgsOsgEarthTileSource : public TileSource
    {
      public:
        QgsOsgEarthTileSource( QgisInterface* theQgisInterface, const TileSourceOptions& options = TileSourceOptions() );

        void initialize( const std::string& referenceURI, const Profile* overrideProfile = NULL );

        osg::Image* createImage( const TileKey& key, ProgressCallback* progress );

        virtual osg::HeightField* createHeightField( const TileKey &key, ProgressCallback* progress )
        {
          Q_UNUSED( key );
          Q_UNUSED( progress );
          //NI
          OE_WARN << "[QGIS] Driver does not support heightfields" << std::endl;
          return NULL;
        }

        virtual std::string getExtension()  const
        {
          //All QGIS tiles are in JPEG format
          return "jpg";
        }

        virtual bool supportsPersistentCaching() const
        {
          return false;
        }

      private:

        QImage* createQImage( int width, int height ) const;
        bool intersects( const TileKey* key );

        //! Pointer to the QGIS interface object
        QgisInterface *mQGisIface;
        QgsCoordinateTransform *mCoordTranform;
        QgsMapRenderer* mMapRenderer;

    };
  }
} // namespace osgEarth::Drivers

#endif // OSGEARTH_DRIVER_QGIS_DRIVEROPTIONS

