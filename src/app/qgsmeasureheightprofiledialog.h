/***************************************************************************
                                qgsmeasureheightprofiledialog.h
                               ------------------
        begin                : October 2015
        copyright            : (C) 2015 Sandro Mani
        email                : manisandro@gmail.com
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMEASUREHEIGHTPROFILEDIALOG_H
#define QGSMEASUREHEIGHTPROFILEDIALOG_H

#include <QDialog>
#include "qgscoordinatereferencesystem.h"
#include "qgspoint.h"

class QgsPoint;
class QgsMeasureHeightProfileTool;
class QwtPlot;
class QwtPlotCurve;
class QwtPlotMarker;


class APP_EXPORT QgsMeasureHeightProfileDialog : public QDialog
{
    Q_OBJECT
  public:
    QgsMeasureHeightProfileDialog( QgsMeasureHeightProfileTool* tool, QWidget* parent = 0, Qt::WindowFlags f = 0 );
    void setPoints( const QList<QgsPoint> &points, const QgsCoordinateReferenceSystem& crs );
    void setMarkerPos( int segment, const QgsPoint& p );

  private slots:
    void finish();
    void replot();

  private:
    QgsMeasureHeightProfileTool* mTool;
    QwtPlot* mPlot;
    QwtPlotCurve* mPlotCurve;
    QwtPlotMarker* mPlotMarker;
    QList<QgsPoint> mPoints;
    QVector<double> mSegmentLengths;
    double mTotLength;
    QgsCoordinateReferenceSystem mPointsCrs;
    int mNSamples;
};

#endif // QGSMEASUREHEIGHTPROFILEDIALOG_H
