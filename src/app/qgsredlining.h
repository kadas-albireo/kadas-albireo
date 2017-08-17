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
#include "qgsredliningmanager.h"
#include "qgsfeature.h"
#include "qgsbottombar.h"
#include "qgsmaptoolmovelabel.h"
#include "qgsmaptoolsredlining.h"
#include "ui_qgsredliningtexteditor.h"

class QAction;
class QComboBox;
class QSpinBox;
class QToolButton;
class QgisApp;
class QgsColorButtonV2;
class QgsMapToolDrawShape;
class QgsRedliningLayer;
class QgsRedliningBottomBar;
class QgsRibbonApp;

class QgsRedliningLabelEditor : public QgsRedliningAttribEditor
{
    Q_OBJECT
  public:
    QgsRedliningLabelEditor();
    void set( const QgsAttributes &attribs, const QgsFields &fields ) override;
    void get( QgsAttributes &attribs, const QgsFields &fields ) const override;
    bool isValid() const override { return !ui.lineEditText->text().isEmpty(); }
    void setFocus() override { ui.lineEditText->setFocus(); }

  private:
    Ui::QgsRedliningTextEditor ui;

  private slots:
    void saveFont();
};

class QgsRedlining : public QgsRedliningManager
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
    void editFeature( const QgsFeature &feature ) override;
    void editLabel( const QgsLabelPosition &labelPos ) override;
    void editFeatures( const QList<QgsFeature>& features ) override;

  public slots:
    void setMarkerTool( const QString& shape, bool active, const QgsFeature *editFeature, QAction *action );
    void setPointTool( bool active = true, const QgsFeature* editFeature = 0 ) { setMarkerTool( "circle", active, editFeature, mActionNewPoint ); }
    void setSquareTool( bool active = true, const QgsFeature* editFeature = 0 ) { setMarkerTool( "rectangle", active, editFeature, mActionNewSquare ); }
    void setTriangleTool( bool active = true, const QgsFeature* editFeature = 0 ) { setMarkerTool( "triangle", active, editFeature, mActionNewTriangle ); }
    void setLineTool( bool active = true, const QgsFeature* editFeature = 0 );
    void setRectangleTool( bool active = true, const QgsFeature* editFeature = 0 );
    void setPolygonTool( bool active = true, const QgsFeature* editFeature = 0 );
    void setCircleTool( bool active = true, const QgsFeature* editFeature = 0 );
    void setTextTool( bool active = true );

  signals:
    void featureStyleChanged();

  private:
    QgisApp* mApp;
    RedliningUi mUi;
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

    void setLayer( QgsRedliningLayer* layer );
    void setTool( QgsMapTool *tool, QAction *action, bool active = true );
    static QIcon createOutlineStyleIcon( Qt::PenStyle style );
    static QIcon createFillStyleIcon( Qt::BrushStyle style );

  private slots:
    void checkLayerRemoved( const QString& layerId );
    void deactivateTool();
    void saveColor();
    void saveOutlineWidth();
    void saveStyle();
    void syncStyleWidgets( const QgsFeature &feature );
    void updateToolRubberbandStyle();
    void readProject( const QDomDocument&doc );
    void writeProject( QDomDocument&doc );
};

#endif // QGSREDLINING_H
