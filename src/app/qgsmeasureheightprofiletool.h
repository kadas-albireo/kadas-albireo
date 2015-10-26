/***************************************************************************
    qgsmeasureheightprofiletool.h  -  map tool for measuring height profiles
    ---------------------
    begin                : October 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSMEASUREHEIGHTPROFILETOOL_H
#define QGSMEASUREHEIGHTPROFILETOOL_H

#include "qgsmaptool.h"

class QgsMeasureHeightProfileDialog;
class QgsRubberBand;

class APP_EXPORT QgsMeasureHeightProfileTool : public QgsMapTool
{
    Q_OBJECT
  public:

    QgsMeasureHeightProfileTool( QgsMapCanvas* canvas );
    ~QgsMeasureHeightProfileTool();
    void restart();

    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * e ) override;
    void activate() override;
    void deactivate() override;

  private:
    QgsMeasureHeightProfileDialog* mDialog;
    QgsRubberBand *mRubberBand;
    QgsRubberBand *mRubberBandPoints;
    bool mMoving;
};

#endif // QGSMEASUREHEIGHTPROFILETOOL_H
