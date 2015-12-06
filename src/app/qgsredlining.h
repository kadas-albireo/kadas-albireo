/***************************************************************************
    qgsredlining.h - Redlining support
     --------------------------------------
    Date                 : Sep 2015
    Copyright            : (C) 2015 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSREDLINING_H
#define QGSREDLINING_H

#include <QObject>
#include "qgsfeature.h"
#include "qgsmaptoolmovelabel.h"

class QAction;
class QComboBox;
class QSpinBox;
class QToolButton;
class QgisApp;
class QgsColorButtonV2;
class QgsRedliningLayer;

class QgsRedlining : public QObject
{
    Q_OBJECT
  public:
    QgsRedlining( QgisApp* app );
    QgsRedliningLayer *getOrCreateLayer();
    QAction* actionNewPoint() const { return mActionNewPoint; }
    QAction* actionNewSquare() const { return mActionNewSquare; }
    QAction* actionNewTriangle() const { return mActionNewTriangle; }
    QAction* actionNewLine() const { return mActionNewLine; }
    QAction* actionNewRectangle() const { return mActionNewRectangle; }
    QAction* actionNewPolygon() const { return mActionNewPolygon; }
    QAction* actionNewCircle() const { return mActionNewCircle; }
    QAction* actionNewText() const { return mActionNewText; }

  signals:
    void featureStyleChanged();

  private:
    QgisApp* mApp;
    QToolButton* mBtnNewObject;
    QAction* mActionEditObject;
    QgsColorButtonV2* mBtnOutlineColor;
    QgsColorButtonV2* mBtnFillColor;
    QSpinBox* mSpinBorderSize;
    QComboBox* mOutlineStyleCombo;
    QComboBox* mFillStyleCombo;
    QAction* mActionNewPoint;
    QAction* mActionNewSquare;
    QAction* mActionNewTriangle;
    QAction* mActionNewLine;
    QAction* mActionNewRectangle;
    QAction* mActionNewPolygon;
    QAction* mActionNewCircle;
    QAction* mActionNewText;

    QPointer<QgsRedliningLayer> mLayer;
    QPointer<QgsMapTool> mRedliningTool;
    int mLayerRefCount;

    void activateTool( QgsMapTool* tool, QAction *action );
    static QIcon createOutlineStyleIcon( Qt::PenStyle style );
    static QIcon createFillStyleIcon( Qt::BrushStyle style );

  private slots:
    void clearLayer();
    void deactivateTool();
    void editObject();
    void newPoint();
    void newSquare();
    void newTriangle();
    void newLine();
    void newRectangle();
    void newPolygon();
    void newCircle();
    void newText();
    void saveColor();
    void saveOutlineWidth();
    void saveStyle();
    void syncStyleWidgets( const QgsFeatureId& fid );
    void updateFeatureStyle( const QgsFeatureId& fid );
    void readProject( const QDomDocument&doc );
    void writeProject( QDomDocument&doc );
};

#endif // QGSREDLINING_H
