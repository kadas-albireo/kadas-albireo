/***************************************************************************
     qgscompositionchecker.h - check rendering of Qgscomposition against an expected image
                     --------------------------------------
               Date                 : 5 Juli 2012
               Copyright            : (C) 2012 by Marco Hugentobler
               email                : marco@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSCOMPOSITIONCHECKER_H
#define QGSCOMPOSITIONCHECKER_H

#include "qgsmultirenderchecker.h"
#include <QString>

class QgsComposition;
class QImage;

/**Renders a composition to an image and compares with an expected output*/
class QgsCompositionChecker : public QgsMultiRenderChecker
{
  public:
    QgsCompositionChecker( const QString& testName, QgsComposition* composition );
    ~QgsCompositionChecker();

    bool testComposition( QString &theReport, int page = 0, int pixelDiff = 0 );

  private:
    QgsCompositionChecker(); //forbidden

    QString mTestName;
    QgsComposition* mComposition;
    QSize mSize;
    int mDotsPerMeter;
};

#endif // QGSCOMPOSITIONCHECKER_H
