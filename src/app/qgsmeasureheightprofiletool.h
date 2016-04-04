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

class QgsGeometry;
class QgsMeasureHeightProfileDialog;
class QgsMapToolDrawPolyLine;
class QgsRubberBand;
class QgsVectorLayer;

class APP_EXPORT QgsMeasureHeightProfileTool : public QgsMapTool
{
    Q_OBJECT
  public:

    QgsMeasureHeightProfileTool( QgsMapCanvas* canvas );
    ~QgsMeasureHeightProfileTool();

    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * e ) override;
    void activate() override;
    void deactivate() override;

    void setGeometry( QgsGeometry* geometry, QgsVectorLayer *layer );

  public slots:
    void pickLine();

  private:
    QgsMapToolDrawPolyLine* mDrawTool;
    QgsRubberBand *mPosMarker;
    QgsMeasureHeightProfileDialog* mDialog;
    bool mPicking;

  private slots:
    void drawCleared();
    void drawFinished();
};

#endif // QGSMEASUREHEIGHTPROFILETOOL_H
