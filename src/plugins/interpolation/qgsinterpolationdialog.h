/***************************************************************************
                              qgsinterpolationdialog.h
                              ------------------------
  begin                : March 10, 2008
  copyright            : (C) 2008 by Marco Hugentobler
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSINTERPOLATIONDIALOG_H
#define QGSINTERPOLATIONDIALOG_H

#include "ui_qgsinterpolationdialogbase.h"
#include "qgisinterface.h"
#include <QFileInfo>

class QgsInterpolatorDialog;
class QgsVectorLayer;

class QgsInterpolationDialog: public QDialog, private Ui::QgsInterpolationDialogBase
{
    Q_OBJECT
  public:
    QgsInterpolationDialog( QWidget* parent, QgisInterface* iface );
    ~QgsInterpolationDialog();

  private slots:

    void on_buttonBox_accepted();
    void on_mInputLayerComboBox_currentIndexChanged( const QString& text );
    void on_mOutputFileButton_clicked();
    void on_mConfigureInterpolationButton_clicked();
    void on_mInterpolationMethodComboBox_currentIndexChanged( const QString &text );

  private:
    QgisInterface* mIface;
    /**Dialog to get input for the current interpolation method*/
    QgsInterpolatorDialog* mInterpolatorDialog;

    /**Returns the vector layer that is selected in the layer combo box.
     Returns 0 in case of error.*/
    QgsVectorLayer* getCurrentVectorLayer();
};

#endif
