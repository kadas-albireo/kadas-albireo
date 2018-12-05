/***************************************************************************
 *  qgsmaptoolbullseye.h                                                   *
 *  --------------                                                         *
 *  begin                : November 2018                                   *
 *  copyright            : (C) 2018 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSMAPTOOLBULLSEYE_H
#define QGSMAPTOOLBULLSEYE_H

#include "qgsbottombar.h"
#include "qgsmaptool.h"
#include "ui_qgsbullseyewidgetbase.h"

class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QgsBullsEyeLayer;
class QgsBullsEyeWidget;
class QgsLayerTreeView;

class APP_EXPORT QgsBullsEyeTool : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsBullsEyeTool( QgsMapCanvas* canvas , QgsLayerTreeView *layerTreeView );
    ~QgsBullsEyeTool();

    void activate() override;
    void deactivate() override;
    void canvasReleaseEvent( QMouseEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

  private:
    QgsBullsEyeWidget* mWidget;
    bool mPicking = false;
    QAction* mActionEditLayer = nullptr;
    QgsLayerTreeView* mLayerTreeView;

  private slots:
    void setPicking( bool picking = true );
    void close();
    void addLayerTreeMenuAction( QgsMapLayer* mapLayer );
    void removeLayerTreeMenuAction( const QString& mapLayerId );
    void editCurrentLayer();
};


class APP_EXPORT QgsBullsEyeWidget : public QgsBottomBar
{
    Q_OBJECT

  public:
    QgsBullsEyeWidget( QgsMapCanvas* canvas , QgsLayerTreeView *layerTreeView );
    void centerPicked( const QgsPoint& pos );

  public slots:
    void createLayer( QString layerName = QString() );
    void setLayer( QgsMapLayer* layer );

  private:
    QgsLayerTreeView* mLayerTreeView;
    Ui::QgsBullsEyeWidgetBase ui;
    QgsBullsEyeLayer* mCurrentLayer = nullptr;

    void updateGrid();

  signals:
    void requestPickCenter();
    void close();

  private slots:
    void updateLayer();
    void updateColor( const QColor& color );
    void updateFontSize( int fontSize );
    void updateLabeling( int index );
    void updateLineWidth( int width );
    void repopulateLayers();
    void currentLayerChanged( int cur );
    void updateSelectedLayer( QgsMapLayer* layer );
};

#endif // QGSMAPTOOLBULLSEYE_H
