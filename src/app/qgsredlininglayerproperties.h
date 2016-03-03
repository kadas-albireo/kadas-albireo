/***************************************************************************
                          qgsredlininglayerproperties.h
                             -------------------
    begin                : March 2016
    copyright            : (C) 2016 by Sandro Mani
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

#ifndef QGSREDLININGLAYERPROPERTIES_H
#define QGSREDLININGLAYERPROPERTIES_H

#include "qgsoptionsdialogbase.h"
#include "ui_qgsredlininglayerpropertiesbase.h"

class QgsMapLayerPropertiesFactory;
class QgsRedliningLayer;
class QgsVectorLayerPropertiesPage;

class APP_EXPORT QgsRedliningLayerProperties : public QgsOptionsDialogBase, private Ui::QgsRedliningLayerPropertiesBase
{
    Q_OBJECT

  public:
    QgsRedliningLayerProperties( QgsRedliningLayer *lyr = 0, QWidget *parent = 0, Qt::WindowFlags fl = QgisGui::ModalDialogFlags );
    ~QgsRedliningLayerProperties();

    void addPropertiesPageFactory( QgsMapLayerPropertiesFactory *factory );

  public slots:
    /** Called when apply button is pressed or dialog is accepted */
    void apply();

    /** Called when cancel button is pressed */
    void onCancel();

  protected:

    QgsRedliningLayer *layer;

    QList<QgsVectorLayerPropertiesPage*> mLayerPropertiesPages;
};

#endif // QGSREDLININGLAYERPROPERTIES_H
