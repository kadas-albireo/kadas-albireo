/***************************************************************************
 *  qgsvbsmapwidget.h                                                      *
 *  -------------------                                                    *
 *  begin                : Sep 16, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
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

#ifndef QGSVBSMAPWIDGET_H
#define QGSVBSMAPWIDGET_H

#include <QDockWidget>

class QgisInterface;
class QgsMapCanvas;
class QgsRectangle;
class QToolButton;
class QMenu;
class QLineEdit;


class QgsVBSMapWidget : public QDockWidget
{
    Q_OBJECT
  public:
    QgsVBSMapWidget( int number, const QString& title, QgisInterface* iface, QWidget* parent = 0 );
    void setInitialLayers( const QStringList& initialLayers ) { mInitialLayers = initialLayers; }
    int getNumber() const { return mNumber; }
    QStringList getLayers() const;

    QgsRectangle getMapExtent() const;
    void setMapExtent( const QgsRectangle& extent );

    bool getLocked() const;
    void setLocked( bool locked );

  private:
    QgisInterface* mIface;
    int mNumber;
    QToolButton* mLayerSelectionButton;
    QMenu* mLayerSelectionMenu;
    QToolButton* mLockViewButton;
    QToolButton* mCloseButton;
    QLineEdit* mTitleLineEdit;
    QgsMapCanvas* mMapCanvas;
    QStringList mInitialLayers;

    void showEvent( QShowEvent * ) override;

  private slots:
    void setCanvasLocked( bool locked );
    void syncCanvasExtents();
    void updateLayerSelectionMenu();
    void updateLayerSet();
    void updateMapProjection();

};

#endif // QGSVBSMAPWIDGET_H
