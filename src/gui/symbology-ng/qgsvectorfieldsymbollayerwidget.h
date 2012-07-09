/***************************************************************************
    qgsvectorfieldsymbollayerwidget.h
    ---------------------
    begin                : October 2011
    copyright            : (C) 2011 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSVECTORFIELDSYMBOLLAYERWIDGET_H
#define QGSVECTORFIELDSYMBOLLAYERWIDGET_H

#include "qgssymbollayerv2widget.h"
#include "ui_widget_vectorfield.h"

class QgsVectorFieldSymbolLayer;

class GUI_EXPORT QgsVectorFieldSymbolLayerWidget: public QgsSymbolLayerV2Widget, private Ui::WidgetVectorFieldBase
{
    Q_OBJECT
  public:
    QgsVectorFieldSymbolLayerWidget( const QgsVectorLayer* vl, QWidget* parent = 0 );
    ~QgsVectorFieldSymbolLayerWidget();

    static QgsSymbolLayerV2Widget* create( const QgsVectorLayer* vl ) { return new QgsVectorFieldSymbolLayerWidget( vl ); }

    // from base class
    virtual void setSymbolLayer( QgsSymbolLayerV2* layer );
    virtual QgsSymbolLayerV2* symbolLayer();

  protected:
    QgsVectorFieldSymbolLayer* mLayer;

  private slots:
    void on_mScaleSpinBox_valueChanged( double d );
    void on_mXAttributeComboBox_currentIndexChanged( int index );
    void on_mYAttributeComboBox_currentIndexChanged( int index );
    void on_mCartesianRadioButton_toggled( bool checked );
    void on_mPolarRadioButton_toggled( bool checked );
    void on_mHeightRadioButton_toggled( bool checked );
    void on_mDegreesRadioButton_toggled( bool checked );
    void on_mRadiansRadioButton_toggled( bool checked );
    void on_mClockwiseFromNorthRadioButton_toggled( bool checked );
    void on_mCounterclockwiseFromEastRadioButton_toggled( bool checked );
};

#endif // QGSVECTORFIELDSYMBOLLAYERWIDGET_H
