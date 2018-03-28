/***************************************************************************
 *  qgsmaptoolguidegrid.h                                                  *
 *  --------------                                                         *
 *  begin                : March 2018                                      *
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

#ifndef QGSMAPTOOLGUIDEGRID_H
#define QGSMAPTOOLGUIDEGRID_H

#include "qgsbottombar.h"
#include "qgsmaptool.h"
#include "ui_qgsguidegridwidgetbase.h"

class QLineEdit;
class QDoubleSpinBox;
class QSpinBox;
class QgsGuideGridLayer;
class QgsGuideGridWidget;

class APP_EXPORT QgsGuideGridTool : public QgsMapTool
{
    Q_OBJECT
  public:
    enum PickMode {PICK_NONE, PICK_TOP_LEFT, PICK_BOTTOM_RIGHT};
    QgsGuideGridTool( QgsMapCanvas* canvas );

    void activate() override;
    void deactivate() override;
    void canvasReleaseEvent( QMouseEvent *e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

  private:
    QgsGuideGridWidget* mWidget;
    PickMode mPickMode = PICK_NONE;

  private slots:
    void setPickMode( QgsGuideGridTool::PickMode pickMode );
    void close();

  signals:
    void pickResult( PickMode pickMode, QgsPoint pos );
};


class APP_EXPORT QgsGuideGridWidget : public QgsBottomBar
{
    Q_OBJECT

  public:
    QgsGuideGridWidget( QgsMapCanvas* canvas );
    void init();
    void pointPicked( QgsGuideGridTool::PickMode pickMode, const QgsPoint& pos );

  private:
    QgsCoordinateReferenceSystem mCrs;
    QgsRectangle mCurRect;
    Ui::QgsGuideGridWidgetBase ui;

    QgsGuideGridLayer* getGuideGridLayer() const;
    void updateGrid();
    void showEvent( QShowEvent */*event*/ ) override { setFixedSize( size() ); updatePosition(); }

  signals:
    void requestPick( QgsGuideGridTool::PickMode pickMode );
    void close();

  private slots:
    void topLeftEdited();
    void bottomRightEdited();
    void updateIntervals();
    void updateBottomRight();
    void updateColor( const QColor& color );
    void updateFontSize( int fontSize );
    void updateLabeling( int labelingMode );
    void pickTopLeftPos() { emit requestPick( QgsGuideGridTool::PICK_TOP_LEFT ); }
    void pickBottomRight() { emit requestPick( QgsGuideGridTool::PICK_BOTTOM_RIGHT ); }
};

#endif // QGSMAPTOOLGUIDEGRID_H
