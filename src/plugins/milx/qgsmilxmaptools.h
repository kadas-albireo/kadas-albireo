/***************************************************************************
 *  qgsmilxmaptoolS.h                                                      *
 *  -----------------                                                      *
 *  begin                : Oct 01, 2015                                    *
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

#ifndef QGSMILXMAPTOOLS_H
#define QGSMILXMAPTOOLS_H

#include "qgsbottombar.h"
#include "qgsmaptool.h"
#include <QAction>
#include <QPointer>

class QgsBottomBar;
class QGraphicsRectItem;
class QPushButton;
class QgisInterface;
class QgsMilXAnnotationItem;
class QgsMilXEditTool;
class QgsMilXItem;
class QgsMilXLayer;

class QgsMilXCreateTool : public QgsMapTool
{
  public:
    QgsMilXCreateTool( QgsMapCanvas* canvas, QgsMilXLayer *layer, const QString& symbolXml, const QString& symbolMilitaryName, int nMinPoints, bool hasVariablePoints, const QPixmap& preview );
    ~QgsMilXCreateTool();
    void canvasPressEvent( QMouseEvent * e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

  private:
    QString mSymbolXml;
    QString mSymbolMilitaryName;
    int mMinNPoints;
    int mNPressedPoints;
    bool mHasVariablePoints;
    QPointer<QgsMilXAnnotationItem> mItem;
    QgsMilXLayer* mLayer;
};

class QgsMilxEditBottomBar : public QgsBottomBar
{
    Q_OBJECT
  public:
    QgsMilxEditBottomBar( QgsMilXEditTool* tool );

  private:
    QgsMilXEditTool* mTool;
    QLabel* mStatusLabel;
    QPushButton* mCopyButton;
    QPushButton* mMoveButton;
    QMenu* mCopyMenu;
    QMenu* mMoveMenu;

  private:
    QString createLayer();
    void copyMoveSymbols( const QString& targetLayerId, bool move );

  private slots:
    void onClose();
    void repopulateLayers();
    void updateStatus();

    void copyToLayer() { copyMoveSymbols( qobject_cast<QAction*>( sender() )->data().toString(), false ); }
    void copyToNewLayer() { copyMoveSymbols( createLayer(), false ); }
    void moveToLayer() { copyMoveSymbols( qobject_cast<QAction*>( sender() )->data().toString(), true ); }
    void moveToNewLayer() { copyMoveSymbols( createLayer(), true ); }
};

class QgsMilXEditTool : public QgsMapTool
{
    Q_OBJECT
  public:
    QgsMilXEditTool( QgisInterface* iface, QgsMilXLayer* layer, QgsMilXItem* layerItem );
    ~QgsMilXEditTool();
    void activate() override;
    void canvasPressEvent( QMouseEvent* e ) override;
    void canvasMoveEvent( QMouseEvent * e ) override;
    void canvasReleaseEvent( QMouseEvent * e ) override;
    void keyReleaseEvent( QKeyEvent *e ) override;

  signals:
    void itemsChanged();

  private:
    friend class QgsMilxEditBottomBar;
    QgisInterface* mIface;
    QgsMilxEditBottomBar* mBottomBar;
    QList<QgsMilXAnnotationItem*> mItems;
    QPointer<QgsMilXLayer> mLayer;
    QGraphicsRectItem* mRectItem;
    QPoint mMouseMoveLastXY;
    bool mPanning;
    QgsMilXAnnotationItem* mActiveAnnotation;
    int mAnnotationMoveAction;

  private slots:
    void removeItemFromList();
    void updateRect();
    void checkLayerHidden();
};

#endif // QGSMILXMAPTOOLS_H

