/* **************************************************************************
                qgspseudocolorshader.h -  description
                       -------------------
begin                : Fri Dec 28 2007
copyright            : (C) 2007 by Peter J. Ersts
email                : ersts@amnh.org

This class contains code that was originally part of the larger QgsRasterLayer
class originally created circa 2004 by T.Sutton, Gary E.Sherman, Steve Halasz
****************************************************************************/

/* **************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSPSEUDOCOLORSHADER_H
#define QGSPSEUDOCOLORSHADER_H

#include "qgsrastershaderfunction.h"

/** \ingroup core
 * A raster color shader that highlighs low values in blue and high values in red.
 */
class CORE_EXPORT QgsPseudoColorShader : public QgsRasterShaderFunction
{

  public:
    QgsPseudoColorShader( double theMinimumValue = 0.0, double theMaximumValue = 255.0 );

    /** \brief generates and new RGB value based on one input value */
    bool shade( double, int*, int*, int* );

    /** \brief generates and new RGB value based on original RGB value */
    bool shade( double, double, double, int*, int*, int* );

    /** \brief Set the maximum value */
    void setMaximumValue( double );

    /** \brief Return the minimum value */
    void setMinimumValue( double );

  private:
    void setClassBreaks();

    double mBreakSize;
    double mClassBreakMin1;
    double mClassBreakMax1;
    double mClassBreakMin2;
    double mClassBreakMax2;
    double mClassBreakMin3;
};
#endif
