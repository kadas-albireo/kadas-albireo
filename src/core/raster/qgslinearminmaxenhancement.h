/* **************************************************************************
              qgslinearminmaxenhancement.h -  description
                       -------------------
begin                : Fri Nov 16 2007
copyright            : (C) 2007 by Peter J. Ersts
email                : ersts@amnh.org

****************************************************************************/

/* **************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSLINEARMINMAXENHANCEMENT_H
#define QGSLINEARMINMAXENHANCEMENT_H

#include "qgscontrastenhancementfunction.h"

/** \ingroup core
  * A colour enhancement function that performs a linear enhanceContrast between min and max.
  */
class CORE_EXPORT QgsLinearMinMaxEnhancement : public QgsContrastEnhancementFunction
{

  public:
    QgsLinearMinMaxEnhancement( QgsContrastEnhancement::QgsRasterDataType, double, double );

    int enhance( double );

};

#endif
