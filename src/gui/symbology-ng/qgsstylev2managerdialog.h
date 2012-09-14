/***************************************************************************
    qgsstylev2managerdialog.h
    ---------------------
    begin                : November 2009
    copyright            : (C) 2009 by Martin Dobias
    email                : wonder.sk at gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSSTYLEV2MANAGERDIALOG_H
#define QGSSTYLEV2MANAGERDIALOG_H

#include <QDialog>
#include <QStandardItem>
#include <QAction>
#include <QMenu>

#include "ui_qgsstylev2managerdialogbase.h"
#include "qgscontexthelp.h"

class QgsStyleV2;

class GUI_EXPORT QgsStyleV2ManagerDialog : public QDialog, private Ui::QgsStyleV2ManagerDialogBase
{
    Q_OBJECT

  public:
    QgsStyleV2ManagerDialog( QgsStyleV2* style, QWidget* parent = NULL );

    //! open add color ramp dialog, return color ramp's name if the ramp has been added
    static QString addColorRampStatic( QWidget* parent, QgsStyleV2* style );

  public slots:
    void addItem();
    void editItem();
    void removeItem();
    void exportItems();
    void importItems();

    //! adds symbols of some type to list
    void populateList();

    //! called when the dialog is going to be closed
    void onFinished();

    void on_buttonBox_helpRequested() { QgsContextHelp::run( metaObject()->className() ); }

    void itemChanged( QStandardItem* item );

    void groupChanged( const QModelIndex& );
    void groupRenamed( QStandardItem * );
    void addGroup();
    void removeGroup();

    //! carryout symbol grouping using check boxes
    void groupSymbolsAction();

    //! edit the selected smart group
    void editSmartgroupAction();

    //! symbol changed from one group
    void regrouped( QStandardItem* );

    //! filter the symbols based on input search term
    void filterSymbols( QString );

    //! Listen to tag changes
    void tagsChanged();

    //! Perform symbol specific tasks when selected
    void symbolSelected( const QModelIndex& );

    //! Context menu for the groupTree
    void grouptreeContextMenu( const QPoint& );

    //! Context menu for the listItems ( symbols list )
    void listitemsContextMenu( const QPoint& );


  protected:

    //! populate combo box with known style items (symbols, color ramps)
    void populateTypes();

    //! populate the groups
    void populateGroups();
    //! build the groups tree
    void buildGroupTree( QStandardItem* &parent );
    //! to set symbols checked when in editing mode
    void setSymbolsChecked( QStringList );

    //! populate list view with symbols of the current type with the given names
    void populateSymbols( QStringList symbolNames, bool checkable = false );

    //! populate list view with color ramps
    void populateColorRamps( QStringList colorRamps, bool check );

    int currentItemType();
    QString currentItemName();

    //! add a new symbol to style
    bool addSymbol();
    //! add a new color ramp to style
    bool addColorRamp();

    bool editSymbol();
    bool editColorRamp();

    bool removeSymbol();
    bool removeColorRamp();

    //! Enables or disbables the symbol specific inputs
    void enableSymbolInputs( bool );
    //! Enables or disables the groupTree specific inputs
    void enableGroupInputs( bool );
    //! Enables or diables the groupTree items for grouping mode
    void enableItemsForGroupingMode( bool );

    //! Event filter to capture tagsLineEdit out of focus
    bool eventFilter( QObject* , QEvent* );

    //! sets the text of the item with bold font
    void setBold( QStandardItem* );

    QgsStyleV2* mStyle;

    QString mStyleFilename;

    bool mModified;

    //! Mode to display the symbol list
    bool mGrouppingMode;

    //! space to store symbol tags
    QStringList mTagList;
};

#endif
