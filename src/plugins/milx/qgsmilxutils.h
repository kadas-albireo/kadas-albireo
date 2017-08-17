/***************************************************************************
 *  qgsmilxutils.h                                                         *
 *  -----------------                                                      *
 *  begin                : August, 2017                                    *
 *  copyright            : (C) 2017 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMILXUTILS_H
#define QGSMILXUTILS_H

#include <QWidget>

class QComboBox;
class QgisInterface;
class QgsMapLayer;
class QgsMilXLayer;

class QgsMilXLayerSelectionWidget : public QWidget
{
    Q_OBJECT
  public:
    QgsMilXLayerSelectionWidget( QgisInterface *iface, QWidget* parent = 0 );
    QgsMilXLayer* getTargetLayer() const;
  signals:
    void targetLayerChanged( QgsMilXLayer* layer );
  private:
    QComboBox* mLayersCombo;
    QgisInterface* mIface;

  private slots:
    void createLayer();
    void setCurrentLayer( int idx );
    void setCurrentLayer( QgsMapLayer* layer );
    void repopulateLayers();
};

#endif // QGSMILXUTILS_H
