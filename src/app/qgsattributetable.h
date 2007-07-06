/***************************************************************************
                          qgsattributetable.h  -  description
                             -------------------
    begin                : Sat Nov 23 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc dot com
        Romans 3:23=>Romans 6:23=>Romans 5:8=>Romans 10:9,10=>Romans 12
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
#ifndef QGSATTRIBUTETABLE_H
#define QGSATTRIBUTETABLE_H

#include <Q3Table>
#include <QMap>

#include <map>
#include <set>

class QgsVectorLayer;
class QgsFeature;
class QMouseEvent;
class QKeyEvent;
class QAction;
class QMenu;

#include "qgsattributeaction.h"
#include "qgsvectorlayer.h"

#include <vector>
#include <utility>
/**
 *@author Gary E.Sherman
 */

class QgsAttributeTable:public Q3Table
{
  Q_OBJECT 

  public:
    QgsAttributeTable(QWidget * parent = 0, const char *name = 0);
    ~QgsAttributeTable();

    /**Inserts the feature with the specified id into rowIdMap. This function has to be called (e.g. from QgsShapeFileLayer) when a row is inserted into the table*/
    void insertFeatureId(int id, int row);
    /**Selects the row which belongs to the feature with the specified id*/
    void selectRowWithId(int id);
    /**Sorts a column. This method replaces the one from QTable to allow numeric sorting*/
    virtual void sortColumn(int col, bool ascending=true, bool wholeRows=false);
    /* Use this to give this class the current attribute actions,
       which are used when the user requests a popup menu */
    void setAttributeActions(const QgsAttributeAction& actions)
    { mActions = actions; }
    /**Returns if the table contains uncommited changes*/
    bool edited() const {return mEdited;}
    /**Switches editing mode on and off*/
    void setEditable(bool enabled){mEditable=enabled;}
    /**Adds an attribute to the table (but does not commit it yet)
      @param name attribute name
      @param type attribute type
      @return false in case of a name conflict, true in case of success*/
    bool addAttribute(const QString& name, const QString& type);
    /**Deletes an attribute (but does not commit it)
      @param name attribute name*/
    void deleteAttribute(const QString& name);

    /** Copies the selected rows to the clipboard 
        Deprecated: See QgisApp::editCopy() instead */
    void copySelectedRows();

    /**
      Attempts to commit any changes to disk.  Returns the result of the attempt.
      If a commit fails, the in-memory changes are left alone.

      This allows editing to continue if the commit failed on e.g. a
      disallowed value in a Postgres database - the user can re-edit and try
      again.

      Delegates to QgsVectorLayer to decide, which changes
      belong to not commited features or to commited ones.

     */
    bool commitChanges(QgsVectorLayer* layer);

    /**Discard all changes and restore
      the state before editing was started*/
    bool rollBack(QgsVectorLayer* layer);
    /**Fills the contents of a provider into this table*/
    void fillTable(QgsVectorLayer* layer);
    /**Swaps the selected rows such that the selected ones are on the top of the table*/
    void bringSelectedToTop();
    /** Selects rows with chosen feature IDs */
    void selectRowsWithId(const QgsFeatureIds& ids);
    /** Shows only rows with chosen feature IDs, others get hidden */
    void showRowsWithId(const QgsFeatureIds& ids);
    /** Shows all rows */
    void showAllRows();

    public slots:
      void columnClicked(int col);
      void rowClicked(int row);
    // Called when the user requests a popup menu
    void popupMenu(int row, int col, const QPoint& pos);
    // Called when the user chooses an item on the popup menu
    void popupItemSelected(QAction * menuAction);
    protected slots:
      void handleChangedSelections();
    /**Writes changed values to 'mChangedValues'*/
    void storeChangedValue(int row, int column);

  protected:
    /**Flag telling if the ctrl-button or the shift-button is pressed*/
    bool lockKeyPressed;
    /**Search tree to find a row corresponding to a feature id*/
    QMap<int,int> rowIdMap;
    /**Flag indicating, which sorting order should be used*/
    bool sort_ascending;
    bool mEditable;
    /**True if table has been edited and contains uncommited changes*/
    bool mEdited;
    /**Map containing the added attributes. The key is the attribute name
      and the value the attribute type*/
    QgsNewAttributesMap mAddedAttributes;
    /**Set containing the attribute names of deleted attributes*/
    QgsDeletedAttributesSet mDeletedAttributes;
    /**Nested map containing the changed attribute values. The int is the feature id, 
      the first QString the attribute name and the second QString the new value*/
    QgsChangedAttributesMap mChangedValues;
    /**Stors the numbers of the last selected rows. This is used to check for selection changes before emit repaintRequested()*/
    std::set<int> mLastSelectedRows;

    /**Compares the content of two cells either alphanumeric or numeric. If 'ascending' is true, -1 means s1 is less, 0 equal, 1 greater. If 'ascending' is false, -1 means s1 is more, 0 equal, 1 greater. This method is used mainly to sort a column*/
    int compareItems(QString s1, QString s2, bool ascending, bool alphanumeric);
    void keyPressEvent(QKeyEvent* ev);
    void keyReleaseEvent(QKeyEvent* ev);
    /**Method used by sortColumn (implementation of a quicksort)*/
    void qsort(int lower, int upper, int col, bool ascending, bool alphanumeric);
    /**Clears mAddedAttributes, mDeletedAttributes and mChangedValues*/
    void clearEditingStructures();
    /**Removes the column belonging to an attribute from the table
      @name attribut name*/
    void removeAttrColumn(const QString& name);
    /** puts attributes of feature to the chosen table row */
    void putFeatureInTable(int row, QgsFeature& fet);
    void contentsMouseReleaseEvent(QMouseEvent* e);
    /**This function compares the current selection and the selection of the last repaint. Returns true if there are differences in the selection.
     Also, mLastSelectedRows is updated*/
    bool checkSelectionChanges();
    
    /** returns column index for field index or -1 on invalid field index */
    int colIndexFromFieldIndex(int fieldId);
    /** returns field index for a column or -1 when on invalid column */
    int fieldIndexFromColIndex(int colIndex);

signals:

    /**Is emitted when a row was selected*/
    void selected(int, bool);
    /**Is emitted when all rows have been deselected*/
    void selectionRemoved(bool);
    /**Is emmited when a set of related selection and deselection signals have been emitted*/
    void repaintRequested();

  private:
    // Data to do with providing a popup menu of actions that
    std::vector<std::pair<QString, QString> > mActionValues;
    int mClickedOnValue;
    QMenu* mActionPopup;
    QgsAttributeAction mActions;
    
    QgsFieldMap mFields;
};

#endif
