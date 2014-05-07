/***************************************************************************
    qgsunitselectionwidget.h
    -------------------
    begin                : Mar 24, 2014
    copyright            : (C) 2014 Sandro Mani
    email                : smani@sourcepole.ch

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSUNITSELECTIONWIDGET_H
#define QGSUNITSELECTIONWIDGET_H

#include <QWidget>
#include "qgssymbolv2.h"
#include "ui_qgsunitselectionwidget.h"
#include "ui_qgsmapunitscaledialog.h"


/** Dialog allowing the user to choose the minimum and maximum scale of an object in map units */
class GUI_EXPORT QgsMapUnitScaleDialog : public QDialog, private Ui::QgsMapUnitScaleDialog
{
    Q_OBJECT

  public:
    QgsMapUnitScaleDialog( QWidget* parent );

    /** Returns the map unit scale */
    QgsMapUnitScale getMapUnitScale() const;
    /** Sets the map unit scale */
    void setMapUnitScale( const QgsMapUnitScale& scale );

  private slots:
    void configureMinComboBox();
    void configureMaxComboBox();
};

/** Widget displaying a combobox allowing the user to choose between millimeter and map units
 *  If the user chooses map units,  a button appears allowing the specification of minimum and maximum scale */
class GUI_EXPORT QgsUnitSelectionWidget : public QWidget, private Ui::QgsUnitSelectionWidget
{
    Q_OBJECT

  private:
    QgsMapUnitScaleDialog* mUnitScaleDialog;
    int mMapUnitIdx;

  public:
    QgsUnitSelectionWidget( QWidget* parent = 0 );

    /** Sets the units which the user can choose from in the combobox. mapUnitIdx specifies which entry corresponds to the map units, or -1 if none */
    void setUnits( const QStringList& units, int mapUnitIdx );

    /** Get the selected unit index */
    int getUnit() const { return mUnitCombo->currentIndex(); }
    /** Sets the selected unit index */
    void setUnit( int unitIndex );
    /** Returns the map unit scale */
    QgsMapUnitScale getMapUnitScale() const { return mUnitScaleDialog->getMapUnitScale(); }
    /** Sets the map unit scale */
    void setMapUnitScale( const QgsMapUnitScale& scale ) { mUnitScaleDialog->setMapUnitScale( scale ); }

  signals:
    void changed();

  private slots:
    void showDialog();
    void toggleUnitRangeButton();
};

#endif // QGSUNITSELECTIONWIDGET_H
