/*
 *   libpal - Automated Placement of Labels Library
 *
 *   Copyright (C) 2008 Maxence Laurent, MIS-TIC, HEIG-VD
 *                      University of Applied Sciences, Western Switzerland
 *                      http://www.hes-so.ch
 *
 *   Contact:
 *      maxence.laurent <at> heig-vd <dot> ch
 *    or
 *      eric.taillard <at> heig-vd <dot> ch
 *
 * This file is part of libpal.
 *
 * libpal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libpal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libpal.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _FEATURE_H
#define _FEATURE_H

#include <iostream>
#include <fstream>
#include <cmath>

#include <geos_c.h>

#include <pal/palgeometry.h>

#include "pointset.h"
#include "util.h"


namespace pal
{
  /** optional additional info about label (for curved labels) */
  class CORE_EXPORT LabelInfo
  {
    public:
      typedef struct
      {
        unsigned short chr;
        double width;
      } CharacterInfo;

      LabelInfo( int num, double height, double maxinangle = 20.0, double maxoutangle = -20.0 )
      {
        max_char_angle_inside = maxinangle;
        // outside angle should be negative
        max_char_angle_outside = maxoutangle > 0 ? -maxoutangle : maxoutangle;
        label_height = height;
        char_num = num;
        char_info = new CharacterInfo[num];
      }
      ~LabelInfo() { delete [] char_info; }

      double max_char_angle_inside;
      double max_char_angle_outside;
      double label_height;
      int char_num;
      CharacterInfo* char_info;
  };

  class LabelPosition;
  class FeaturePart;

  class CORE_EXPORT Feature
  {
      friend class FeaturePart;

    public:
      Feature( Layer* l, const char* id, PalGeometry* userG, double lx, double ly );
      ~Feature();

      void setLabelInfo( LabelInfo* info ) { labelInfo = info; }
      void setDistLabel( double dist ) { distlabel = dist; }
      //Set label position of the feature to fixed x/y values
      void setFixedPosition( double x, double y ) { fixedPos = true; fixedPosX = x; fixedPosY = y;}
      void setQuadOffset( double x, double y ) { quadOffset = true; quadOffsetX = x; quadOffsetY = y;}
      void setPosOffset( double x, double y ) { offsetPos = true; offsetPosX = x; offsetPosY = y;}
      bool fixedPosition() const { return fixedPos; }
      //Set label rotation to fixed value
      void setFixedAngle( double a ) { fixedRotation = true; fixedAngle = a; }
      void setRepeatDistance( double dist ) { repeatDist = dist; }
      double repeatDistance() const { return repeatDist; }
      void setAlwaysShow( bool bl ) { alwaysShow = bl; }

    protected:
      Layer *layer;
      PalGeometry *userGeom;
      double label_x;
      double label_y;
      double distlabel;
      LabelInfo* labelInfo; // optional

      char *uid;

      bool fixedPos; //true in case of fixed position (only 1 candidate position with cost 0)
      double fixedPosX;
      double fixedPosY;
      bool quadOffset; // true if a quadrant offset exists
      double quadOffsetX;
      double quadOffsetY;
      bool offsetPos; //true if position is to be offset by set amount
      double offsetPosX;
      double offsetPosY;
      //Fixed (e.g. data defined) angle only makes sense together with fixed position
      bool fixedRotation;
      double fixedAngle; //fixed angle value (in rad)
      double repeatDist;

      bool alwaysShow; //true is label is to always be shown (but causes overlapping)


      // array of parts - possibly not necessary
      //int nPart;
      //FeaturePart** parts;
  };

  /**
   * \brief Main class to handle feature
   */
  class CORE_EXPORT FeaturePart : public PointSet
  {

    protected:
      Feature* f;

      int nbHoles;
      PointSet **holes;

      GEOSGeometry *the_geom;
      bool ownsGeom;

      /** \brief read coordinates from a GEOS geom */
      void extractCoords( const GEOSGeometry* geom );

      /** find duplicate (or nearly duplicate points) and remove them.
       * Probably to avoid numerical errors in geometry algorithms.
       */
      void removeDuplicatePoints();

    public:

      /**
        * \brief create a new generic feature
        *
        * \param feat a pointer for a Feat which contains the spatial entites
        * \param geom a pointer to a GEOS geometry
        */
      FeaturePart( Feature *feat, const GEOSGeometry* geom );

      /**
       * \brief Delete the feature
       */
      virtual ~FeaturePart();

      /**
       * \brief generate candidates for point feature
       * Generate candidates for point features
       * \param x x coordinates of the point
       * \param y y coordinates of the point
       * \param scale map scale is 1:scale
       * \param lPos pointer to an array of candidates, will be filled by generated candidates
       * \param delta_width delta width
       * \param angle orientation of the label
       * \return the number of generated cadidates
       */
      int setPositionForPoint( double x, double y, double scale, LabelPosition ***lPos, double delta_width, double angle );

      /**
       * generate one candidate over specified point
       */
      int setPositionOverPoint( double x, double y, double scale, LabelPosition ***lPos, double delta_width, double angle );

      /**
       * \brief generate candidates for line feature
       * Generate candidates for line features
       * \param scale map scale is 1:scale
       * \param lPos pointer to an array of candidates, will be filled by generated candidates
       * \param mapShape a pointer to the line
       * \param delta_width delta width
       * \return the number of generated cadidates
       */
      int setPositionForLine( double scale, LabelPosition ***lPos, PointSet *mapShape, double delta_width );

      LabelPosition* curvedPlacementAtOffset( PointSet* path_positions, double* path_distances,
                                              int orientation, int index, double distance );

      /**
       * Generate curved candidates for line features
       */
      int setPositionForLineCurved( LabelPosition ***lPos, PointSet* mapShape );

      /**
       * \brief generate candidates for point feature
       * Generate candidates for point features
       * \param scale map scale is 1:scale
       * \param lPos pointer to an array of candidates, will be filled by generated candidates
       * \param mapShape a pointer to the polygon
       * \param delta_width delta width
       * \return the number of generated cadidates
       */
      int setPositionForPolygon( double scale, LabelPosition ***lPos, PointSet *mapShape, double delta_width );

#if 0
      /**
       * \brief Feature against problem bbox
       * \param bbox[0] problem x min
       * \param bbox[1] problem x max
       * \param bbox[2] problem y min
       * \param bbox[3] problem y max
       * return A set of feature which are in the bbox or null if the feature is in the bbox
       */
      LinkedList<Feature*> *splitFeature( double bbox[4] );


      /**
       * \brief return the feature id
       * \return the feature id
       */
      int getId();
#endif

      /**
       * \brief return the feature
       * \return the feature
       */
      Feature* getFeature() { return f; }

      /**
       * \brief return the geometry
       * \return the geometry
       */
      const GEOSGeometry* getGeometry() const { return the_geom; }

      /**
       * \brief return the layer that feature belongs to
       * \return the layer of the feature
       */
      Layer * getLayer();

#if 0
      /**
       * \brief save the feature into file
       * Called by Pal::save()
       * \param file the file to write
       */
      void save( std::ofstream *file );
#endif

      /**
       * \brief generic method to generate candidates
       * This method will call either setPositionFromPoint(), setPositionFromLine or setPositionFromPolygon
       * \param scale the map scale is 1:scale
       * \param lPos pointer to candidates array in which candidates will be put
       * \param bbox_min min values of the map extent
       * \param bbox_max max values of the map extent
       * \param mapShape generate candidates for this spatial entites
       * \param candidates index for candidates
       * \return the number of candidates in *lPos
       */
      int setPosition( double scale, LabelPosition ***lPos, double bbox_min[2], double bbox_max[2], PointSet *mapShape, RTree<LabelPosition*, double, 2, double>*candidates
#ifdef _EXPORT_MAP_
                       , std::ofstream &svgmap
#endif
                     );

      /**
       * \brief get the unique id of the feature
       * \return the feature unique identifier
       */
      const char *getUID();


      /**
       * \brief Print feature information
       * Print feature unique id, geometry type, points, and holes on screen
       */
      void print();


      PalGeometry* getUserGeometry() { return f->userGeom; }

      void setLabelSize( double lx, double ly ) { f->label_x = lx; f->label_y = ly; }
      double getLabelWidth() const { return f->label_x; }
      double getLabelHeight() const { return f->label_y; }
      void setLabelDistance( double dist ) { f->distlabel = dist; }
      double getLabelDistance() const { return f->distlabel; }
      void setLabelInfo( LabelInfo* info ) { f->labelInfo = info; }

      bool getFixedRotation() { return f->fixedRotation; }
      double getLabelAngle() { return f->fixedAngle; }
      bool getFixedPosition() { return f->fixedPos; }
      bool getAlwaysShow() { return f->alwaysShow; }

      int getNumSelfObstacles() const { return nbHoles; }
      PointSet* getSelfObstacle( int i ) { return holes[i]; }

      /** check whether this part is connected with some other part */
      bool isConnected( FeaturePart* p2 );

      /** merge other (connected) part with this one and save the result in this part (other is unchanged).
       * Return true on success, false if the feature wasn't modified */
      bool mergeWithFeaturePart( FeaturePart* other );

      void addSizePenalty( int nbp, LabelPosition** lPos, double bbx[4], double bby[4] );

  };

} // end namespace pal

#endif
