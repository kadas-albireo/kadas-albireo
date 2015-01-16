/***************************************************************************
                          NormVecDecorator.h  -  description
                             -------------------
    copyright            : (C) 2004 by Marco Hugentobler
    email                : mhugent@geo.unizh.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef NORMVECDECORATOR_H
#define NORMVECDECORATOR_H

#include "TriDecorator.h"
#include <TriangleInterpolator.h>
#include <MathUtils.h>
#include "qgslogger.h"
class QProgressDialog;

/**Decorator class which adds the functionality of estimating normals at the data points*/
class ANALYSIS_EXPORT NormVecDecorator: public TriDecorator
{
  public:
    /**Enumeration for the state of a point. NORMAL means, that the point is not on a breakline, BREAKLINE means that the point is on a breakline (but not an endpoint of it) and ENDPOINT means, that it is an endpoint of a breakline*/
    enum pointState {NORMAL, BREAKLINE, ENDPOINT};
    NormVecDecorator();
    NormVecDecorator( Triangulation* tin );
    virtual ~NormVecDecorator();
    /**Adds a point to the triangulation*/
    int addPoint( Point3D* p ) override;
    /**Calculates the normal at a point on the surface and assigns it to 'result'. Returns true in case of success and false in case of failure*/
    bool calcNormal( double x, double y, Vector3D* result ) override;
    /**Calculates the normal of a triangle-point for the point with coordinates x and y. This is needed, if a point is on a break line and there is no unique normal stored in 'mNormVec'. Returns false, it something went wrong and true otherwise*/
    bool calcNormalForPoint( double x, double y, int point, Vector3D* result );
    /**Calculates x-, y and z-value of the point on the surface and assigns it to 'result'. Returns true in case of success and flase in case of failure*/
    bool calcPoint( double x, double y, Point3D* result ) override;
    /**Eliminates the horizontal triangles by swapping or by insertion of new points. If alreadyestimated is true, a re-estimation of the normals will be done*/
    virtual void eliminateHorizontalTriangles() override;
    /**Estimates the first derivative a point. Return true in case of succes and false otherwise*/
    bool estimateFirstDerivative( int pointno );
    /**This method adds the functionality of estimating normals at the data points. Return true in the case of success and false otherwise*/
    bool estimateFirstDerivatives( QProgressDialog* d = 0 );
    /**Returns a pointer to the normal vector for the point with the number n*/
    Vector3D* getNormal( int n ) const;
    /**Finds out, in which triangle a point with coordinates x and y is and assigns the triangle points to p1, p2, p3 and the estimated normals to v1, v2, v3. The vectors are normaly taken from 'mNormVec', exept if p1, p2 or p3 is a point on a breakline. In this case, the normal is calculated on-the-fly. Returns false, if something went wrong and true otherwise*/
    bool getTriangle( double x, double y, Point3D* p1, Vector3D* v1, Point3D* p2, Vector3D* v2, Point3D* p3, Vector3D* v3 );
    /**This function behaves similar to the one above. Additionally, the numbers of the points are returned (ptn1, ptn2, ptn3) as well as the pointStates of the triangle points (state1, state2, state3)*/
    //! @note not available in python bindings
    bool getTriangle( double x, double y, Point3D* p1, int* ptn1, Vector3D* v1, pointState* state1, Point3D* p2, int* ptn2, Vector3D* v2, pointState* state2, Point3D* p3, int* ptn3, Vector3D* v3, pointState* state3 );
    /**Returns the state of the point with the number 'pointno'*/
    pointState getState( int pointno ) const;
    /**Sets an interpolator*/
    void setTriangleInterpolator( TriangleInterpolator* inter ) override;
    /**Swaps the edge which is closest to the point with x and y coordinates (if this is possible) and forces recalculation of the concerned normals (if alreadyestimated is true)*/
    virtual bool swapEdge( double x, double y ) override;
    /**Saves the triangulation as a (line) shapefile
      @return true in case of success*/
    virtual bool saveAsShapefile( const QString& fileName ) const override;

  protected:
    /**Is true, if the normals already have been estimated*/
    bool alreadyestimated;
    const static unsigned int mDefaultStorageForNormals = 100000;
    /**Association with an interpolator object*/
    TriangleInterpolator* mInterpolator;
    /**Vector that stores the normals for the points. If 'estimateFirstDerivatives()' was called and there is a null pointer, this means, that the triangle point is on a breakline*/
    QVector<Vector3D*>* mNormVec;
    /**Vector who stores, it a point is not on a breakline, if it is a normal point of the breakline or if it is an endpoint of a breakline*/
    QVector<pointState>* mPointState;
    /**Sets the state (BREAKLINE, NORMAL, ENDPOINT) of a point*/
    void setState( int pointno, pointState s );
};

inline NormVecDecorator::NormVecDecorator(): TriDecorator(), mInterpolator( 0 ), mNormVec( new QVector<Vector3D*>( mDefaultStorageForNormals ) ), mPointState( new QVector<pointState>( mDefaultStorageForNormals ) )
{
  alreadyestimated = false;
}

inline NormVecDecorator::NormVecDecorator( Triangulation* tin ): TriDecorator( tin ), mInterpolator( 0 ), mNormVec( new QVector<Vector3D*>( mDefaultStorageForNormals ) ), mPointState( new QVector<pointState>( mDefaultStorageForNormals ) )
{
  alreadyestimated = false;
}

inline void NormVecDecorator::setTriangleInterpolator( TriangleInterpolator* inter )
{
  mInterpolator = inter;
}

inline Vector3D* NormVecDecorator::getNormal( int n ) const
{
  if ( mNormVec )
  {
    return mNormVec->at( n );
  }
  else
  {
    QgsDebugMsg( "warning, null pointer" );
    return 0;
  }
}

#endif
