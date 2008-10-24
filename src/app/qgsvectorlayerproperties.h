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
#include "qgsaddattrdialog.h"
#include "qgsdelattrdialog.h"
#include "qgsfield.h"

class QgsMapLayer;

class QgsAttributeActionDialog;
class QgsLabelDialog;
class QgsVectorLayer;

class QgsVectorLayerProperties : public QDialog, private Ui::QgsVectorLayerPropertiesBase
{
    Q_OBJECT
  public:
    QgsVectorLayerProperties( QgsVectorLayer *lyr = 0, QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags );
    ~QgsVectorLayerProperties();
    /**Sets the legend type to "single symbol", "graduated symbol" or "continuous color"*/
    void setLegendType( QString type );
    /**Returns the display name entered in the dialog*/
    QString displayName();
    void setRendererDirty( bool ) {}
    /**Sets the attribute that is used in the Identify Results dialog box*/
    void setDisplayField( QString name );

    /**Adds an attribute to the table (but does not commit it yet)
    @param name attribute name
    @param type attribute type
    @return false in case of a name conflict, true in case of success*/
    bool addAttribute( QString name, QString type );

    /**Deletes an attribute (but does not commit it)
      @param name attribute name
      @return false in case of a non-existing attribute.*/
    bool deleteAttribute( int attr );

  public slots:

    void alterLayerDialog( const QString& string );

    /** Reset to original (vector layer) values */
    void reset();

    /** Get metadata about the layer in nice formatted html */
    QString metadata();

    /** Set transparency based on slider position */
    void sliderTransparency_valueChanged( int theValue );

    /** Toggles on the label check box */
    void setLabelCheckBox();

    /** Called when apply button is pressed or dialog is accepted */
    void apply();

    /** toggle editing of layer */
    void toggleEditing();

    /** editing of layer was toggled */
    void editingToggled();

    //
    //methods reimplemented from qt designer base class
    //

    void on_buttonBox_helpRequested();
    void on_pbnQueryBuilder_clicked();
    void on_pbnIndex_clicked();
    void on_pbnChangeSpatialRefSys_clicked();
    void on_pbnLoadDefaultStyle_clicked();
    void on_pbnSaveDefaultStyle_clicked();
    void on_pbnLoadStyle_clicked();
    void on_pbnSaveStyleAs_clicked();

    void addAttribute();
    void deleteAttribute();

    void attributeAdded( int idx );
    void attributeDeleted( int idx );

  signals:

    /** emitted when changes to layer were saved to update legend */
    void refreshLegend( QString layerID, bool expandItem );

    void toggleEditing( QgsMapLayer * );

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

    void updateButtons();
    void loadRows();
    void setRow( int row, int idx, const QgsField &field );

    /**Buffer pixmap which takes the picture of renderers before they are assigned to the vector layer*/
    //QPixmap bufferPixmap;
    static const int context_id = 94000531;

};

inline QString QgsVectorLayerProperties::displayName()
{
  return txtDisplayName->text();
}
#endif
