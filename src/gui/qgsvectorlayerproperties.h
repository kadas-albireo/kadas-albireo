/***************************************************************************
                          qgsdlgvectorlayerproperties.h
                   Unified property dialog for vector layers
                             -------------------
    begin                : 2004-01-28
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 /* $Id$ */

#ifndef QGSVECTORLAYERPROPERTIES
#define QGSVECTORLAYERPROPERTIES

#include "ui_qgsvectorlayerpropertiesbase.h"
#include "qgisgui.h"
#include "qgsrenderer.h"

class QgsAttributeActionDialog;
class QgsLabelDialog;
class QgsVectorLayer;


class QgsVectorLayerProperties : public QDialog, private Ui::QgsVectorLayerPropertiesBase{
  Q_OBJECT
  public:
  QgsVectorLayerProperties(QgsVectorLayer *lyr = 0,QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags);
  ~QgsVectorLayerProperties();
  /**Sets the legend type to "single symbol", "graduated symbol" or "continuous color"*/
  void setLegendType(QString type);
  /**Returns the display name entered in the dialog*/
  QString displayName();
  void setRendererDirty(bool){}
  /**Sets the attribute that is used in the Identify Results dialog box*/
  void setDisplayField(QString name);

  public slots:
  void alterLayerDialog(const QString& string);
  /** Reset to original (vector layer) values */
  void reset();
  /** Get metadata about the layer in nice formatted html */
  QString getMetadata();
  /** Set transparency based on slider position */
  void sliderTransparency_valueChanged(int theValue);
  
  //
  //methods reimplemented from qt designer base class
  //


  void on_pbnCancel_clicked();
  void on_pbnOK_clicked();
  void on_pbnApply_clicked();
  void on_btnHelp_clicked();
  void on_pbnQueryBuilder_clicked();
  void on_pbnIndex_clicked();
  void on_pbnChangeSpatialRefSys_clicked();

  protected:
  QgsVectorLayer *layer;
  /**Renderer dialog which is shown*/
  QDialog* mRendererDialog;
  /**Buffer renderer, which is assigned to the vector layer when apply is pressed*/
  //QgsRenderer* bufferRenderer;
  /**Label dialog. If apply is pressed, options are applied to vector's QgsLabel */
  QgsLabelDialog* labelDialog;
  /**Actions dialog. If apply is pressed, the actions are stored for later use */
  QgsAttributeActionDialog* actionDialog;
  /**Buffer pixmap which takes the picture of renderers before they are assigned to the vector layer*/
  //QPixmap bufferPixmap;
};

inline QString QgsVectorLayerProperties::displayName()
{
    return txtDisplayName->text();
}

#endif
