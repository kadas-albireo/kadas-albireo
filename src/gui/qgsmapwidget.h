/***************************************************************************
 *  qgsmapwidget.h                                                      *
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

#ifndef QGSMAPWIDGET_H
#define QGSMAPWIDGET_H

#include <QDockWidget>

class QgsMapCanvas;
class QgsRectangle;
class QToolButton;
class QMenu;
class QLineEdit;
class QLabel;
class QStackedWidget;


class GUI_EXPORT QgsMapWidget : public QDockWidget
{
    Q_OBJECT
  public:
    QgsMapWidget( int number, const QString& title, QgsMapCanvas* masterCanvas, QWidget* parent = 0 );
    void setInitialLayers( const QStringList& initialLayers, bool updateMenu = false );
    int getNumber() const { return mNumber; }
    QStringList getLayers() const;

    QgsRectangle getMapExtent() const;
    void setMapExtent( const QgsRectangle& extent );

    bool getLocked() const;
    void setLocked( bool locked );


  private:
    int mNumber;
    QgsMapCanvas* mMasterCanvas;
    QToolButton* mLayerSelectionButton;
    QMenu* mLayerSelectionMenu;
    QToolButton* mLockViewButton;
    QToolButton* mCloseButton;
    QStackedWidget* mTitleStackedWidget;
    QLabel* mTitleLabel;
    QLineEdit* mTitleLineEdit;
    QgsMapCanvas* mMapCanvas;
    QStringList mInitialLayers;
    bool mUnsetFixedSize;

    void showEvent( QShowEvent * ) override;
    bool eventFilter( QObject *obj, QEvent *ev ) override;

  private slots:
    void setCanvasLocked( bool locked );
    void syncCanvasExtents();
    void updateLayerSelectionMenu();
    void updateLayerSet();
    void updateMapProjection();

};

#endif // QGSMAPWIDGET_H
