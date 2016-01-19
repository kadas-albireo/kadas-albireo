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
class QgsRibbonApp;

class QgsRedlining : public QObject
{
    Q_OBJECT
  public:
    struct RedliningUi
    {
      QToolButton* buttonNewObject;
      QSpinBox* spinBoxSize;
      QgsColorButtonV2* colorButtonOutlineColor;
      QgsColorButtonV2* colorButtonFillColor;
      QComboBox* comboOutlineStyle;
      QComboBox* comboFillStyle;
    };

    QgsRedlining( QgisApp* app, const RedliningUi &ui );
    QgsRedliningLayer *getOrCreateLayer();
    QgsRedliningLayer *getLayer() const;
    void editFeature( const QgsFeature &feature );
    void editLabel( const QgsLabelPosition &labelPos );

  public slots:
    void newPoint( bool active = true );
    void newSquare( bool active = true );
    void newTriangle( bool active = true );
    void newLine( bool active = true );
    void newRectangle( bool active = true );
    void newPolygon( bool active = true );
    void newCircle( bool active = true );
    void newText( bool active = true );

  signals:
    void featureStyleChanged();

  private:
    QgsMapCanvas* mMapCanvas;
    RedliningUi mUi;
    QAction* mActionEditObject;
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

    void setTool( QgsMapTool* tool, QAction *action, bool active = true );
    static QIcon createOutlineStyleIcon( Qt::PenStyle style );
    static QIcon createFillStyleIcon( Qt::BrushStyle style );

  private slots:
    void clearLayer();
    void deactivateTool();
    void editObject();
    void saveColor();
    void saveOutlineWidth();
    void saveStyle();
    void syncStyleWidgets( const QgsFeature &feature );
    void updateFeatureStyle( const QgsFeatureId& fid );
    void readProject( const QDomDocument&doc );
    void writeProject( QDomDocument&doc );
};

#endif // QGSREDLINING_H
