/***************************************************************************
    qgsfeatureselectiondlg.h
     --------------------------------------
    Date                 : 11.6.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSFEATURESELECTIONDLG_H
#define QGSFEATURESELECTIONDLG_H

class QgsGenericFeatureSelectionManager;

#include "ui_qgsfeatureselectiondlg.h"

class GUI_EXPORT QgsFeatureSelectionDlg : public QDialog, private Ui::QgsFeatureSelectionDlg
{
    Q_OBJECT

  public:
    explicit QgsFeatureSelectionDlg( QgsVectorLayer* vl, QWidget *parent = 0 );

    const QgsFeatureIds& selectedFeatures();

  private:
    QgsGenericFeatureSelectionManager* mFeatureSelection;
    QgsVectorLayer* mVectorLayer;
};

#endif // QGSFEATURESELECTIONDLG_H
