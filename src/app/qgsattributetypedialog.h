/***************************************************************************
                         qgsattributetypedialog.h  -  description
                             -------------------
    begin                : June 2009
    copyright            : (C) 2009 by Richard Kostecky
    email                : csf.kostej@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSATTRIBUTETYPEDIALOG_H
#define QGSATTRIBUTETYPEDIALOG_H

#include "ui_qgsattributetypeedit.h"

#include "qgsvectorlayer.h"
#include "qgseditorconfigwidget.h"

class QDialog;
class QLayout;
class QgsField;

class APP_EXPORT QgsAttributeTypeDialog: public QDialog, private Ui::QgsAttributeTypeDialog
{
    Q_OBJECT

  public:
    QgsAttributeTypeDialog( QgsVectorLayer *vl );
    ~QgsAttributeTypeDialog();

    /**
     * Overloaded accept method which will write the feature field
     * values, then delegate to QDialog::accept()
     */
    void accept();

    /**
     * Setting index, which page should be selected
     * @param index of page to be selected
     * @param editTypeInt type of edit type which was selected before save
     */
    void setIndex( int index, QgsVectorLayer::EditType editType );

    /**
     * Setting page which is to be selected
     * @param index index of page which was selected
     */
    void setPage( int index );

    /**
     * Getter to get selected edit type
     * @return selected edit type
     */
    QgsVectorLayer::EditType editType();

    const QString editorWidgetV2Type();

    const QString editorWidgetV2Text();

    const QMap<QString, QVariant> editorWidgetV2Config();

    void setWidgetV2Config( const QMap<QString, QVariant>& config );

    /**
     * Setter to value map variable to display actual value
     * @param valueMap map which is to be dispayed in this dialog
     */
    void setValueMap( QMap<QString, QVariant> valueMap );

    /**
     * Setter to range to be displayed and edited in this dialog
     * @param rangeData range data which is to be displayed
     */
    void setRange( QgsVectorLayer::RangeData rangeData );

    /**
     * Setter to checked state to be displayed and edited in this dialog
     * @param checked string that represents the checked state
     */
    void setCheckedState( QString checked, QString unchecked );

    /**
     * Setter to value relation to be displayed and edited in this dialog
     * @param valueRelation value relation data which is to be displayed
     */
    void setValueRelation( QgsVectorLayer::ValueRelationData valueRelationData );

    /**
     * Setter to date format
     * @param dateFormat date format
     */
    void setDateFormat( QString dateFormat );

    /**
     * Setter to widget size
     * @param widgetSize size of widget
     */
    void setWidgetSize( QSize widgetSize );

    /**
     * Setter for checkbox for editable state of field
     * @param bool editable
     */
    void setFieldEditable( bool editable );

    /**
     * Sets the enabled state of the "editable" checkbox
     *
     * @param enabled sets the enabled state of the checkbox
     */
    void setFieldEditableEnabled( bool enabled );

    /**
     * Setter for checkbox to label on top
     * @param bool onTop
     */
    void setLabelOnTop( bool onTop );

    /**
     * Getter for checked state after editing
     * @return string representing the checked
     */
    QPair<QString, QString> checkedState();

    /**
     * Getter for value map after editing
     * @return map which is to be returned
     */
    QMap<QString, QVariant> &valueMap();

    /**
     * Getter for range data
     * @return range data after editing
     */
    QgsVectorLayer::RangeData rangeData();

    /**
     * Getter for value relation data
     */
    QgsVectorLayer::ValueRelationData valueRelationData();

    /**
     * Getter for date format
     */
    QString dateFormat();

    /**
     * Getter for widget size
     */
    QSize widgetSize();

    /**
     * Getter for checkbox for editable state of field
     */
    bool fieldEditable();

    /**
     * Getter for checkbox for label on top of field
     */
    bool labelOnTop();

  private slots:
    /**
     * Slot to handle change of index in combobox to select correct page
     * @param index index of value in combobox
     */
    void setStackPage( int index );

    /**
     * Slot to handle button push to delete selected rows
     */
    void removeSelectedButtonPushed( );

    /**
     * Slot to handle load from layer button pushed to display dialog to load data
     */
    void loadFromLayerButtonPushed( );

    /**
     * Slot to handle load from CSV button pushed to display dialog to load data
     */
    void loadFromCSVButtonPushed( );

    /**
     * Slot to handle change of cell to have always empty row at end
     * @param row index of row which was changed
     * @param column index of column which was changed
     */
    void vCellChanged( int row, int column );

    /**
     * update columns list
     */
    void updateLayerColumns( int idx );

    /**
     * edit the filter expression
     */
    void editValueRelationExpression();

  private:

    QString defaultWindowTitle();

    /**
     * Function to set page according to edit type
     * @param editType edit type to set page
     */
    void setPageForEditType( QgsVectorLayer::EditType editType );

    /**
     * Function to update the value map
     * @param map new map
     */
    void updateMap( const QMap<QString, QVariant> &map );

    bool mFieldEditable;
    bool mLabelOnTop;

    QMap<QString, QVariant> mValueMap;

    QgsVectorLayer *mLayer;
    int mIndex;

    QgsVectorLayer::RangeData mRangeData;
    QgsVectorLayer::ValueRelationData mValueRelationData;
    QgsVectorLayer::EditType mEditType;
    QString mDateFormat;
    QSize mWidgetSize;

    QMap<QString, QVariant> mWidgetV2Config;

    QMap< QString, QgsEditorConfigWidget* > mEditorConfigWidgets;
};

#endif
