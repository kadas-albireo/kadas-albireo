/***************************************************************************
                                qgsmeasure.h
                               ------------------
        begin                : March 2005
        copyright            : (C) 2005 by Radim Blazek
        email                : blazek@itc.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMEASUREDIALOG_H
#define QGSMEASUREDIALOG_H

#include "ui_qgsmeasurebase.h"

#include "qgspoint.h"
#include "qgsdistancearea.h"
#include "qgscontexthelp.h"

class QgsMeasureTool;
class QCloseEvent;

class APP_EXPORT QgsMeasureDialog : public QDialog, private Ui::QgsMeasureBase
{
    Q_OBJECT

  public:

    //! Constructor
    QgsMeasureDialog( QgsMeasureTool* tool, Qt::WindowFlags f = 0 );

    //! Update table for new part
    void addPart();

    //! Update table for removed point
    void removePoint();

    //! Updates the measurements in the UI
    void updateMeasurements();

    //! Return the measurement for the specified part
    QString getPartMeasurement( int partIdx ) const;

    //! measures the specified geometry
    double measureGeometry( const QList<QgsPoint>& points, bool measureArea ) const;

    //! formats measured value to most appropriate units
    QString formatValue( double value , bool measureArea );

  public slots:
    //! Reset and start new
    void restart();

    //! Show the help for the dialog
    void on_buttonBox_helpRequested() { QgsContextHelp::run( metaObject()->className() ); }

    //! When any external settings change
    void updateSettings();

  private slots:
    void unitsChanged( const QString &units );
    void finish();

  private:
    //! shows/hides table, shows correct units
    void updateUi();

    //! Converts the measurement, depending on settings in options and current transformation
    void convertMeasurement( double &measure, QGis::UnitType &u, bool isArea );

    //! indicates whether we're measuring distances or areas
    bool mMeasureArea;

    //! Number of decimal places we want.
    int mDecimalPlaces;

    //! Current unit for input values
    QGis::UnitType mCanvasUnits;

    //! Current unit for output values
    QGis::UnitType mDisplayUnits;

    //! Our measurement object
    QgsDistanceArea mDa;

    //! pointer to measure tool which owns this dialog
    QgsMeasureTool* mTool;
};

#endif
