/***************************************************************************
                          qgsattributetable.cpp  -  description
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
#include <QApplication>
#include <QMouseEvent>
#include <Q3PopupMenu>
#include <QKeyEvent>
#include <QLabel>
#include <QFont>
#include <QClipboard>
#include <Q3ValueList>

#include "qgsattributetable.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"

#include <iostream>
#include <stdlib.h>

QgsAttributeTable::QgsAttributeTable(QWidget * parent, const char *name):
        Q3Table(parent, name),
        lockKeyPressed(false),
        sort_ascending(true),
        mEditable(false),
        mEdited(false),
        mActionPopup(0)
{
  QFont f(font());
  f.setFamily("Helvetica");
  f.setPointSize(11);
  setFont(f);
  setSelectionMode(Q3Table::MultiRow);
  QObject::connect(this, SIGNAL(selectionChanged()), this, SLOT(handleChangedSelections()));
  connect(this, SIGNAL(contextMenuRequested(int, int, const QPoint&)), this, SLOT(popupMenu(int, int, const QPoint&)));
  connect(this, SIGNAL(valueChanged(int, int)), this, SLOT(storeChangedValue(int,int)));
  setReadOnly(true);
  setFocus();
}

QgsAttributeTable::~QgsAttributeTable()
{

}
void QgsAttributeTable::columnClicked(int col)
{
  QApplication::setOverrideCursor(Qt::waitCursor);

  //store the ids of the selected rows in a list
  Q3ValueList < int >idsOfSelected;
  for (int i = 0; i < numSelections(); i++)
    {
      for (int j = selection(i).topRow(); j <= selection(i).bottomRow(); j++)
        {
          idsOfSelected.append(text(j, 0).toInt());
        }
    }

  sortColumn(col, sort_ascending, true);

  //clear and rebuild rowIdMap. Overwrite sortColumn later and sort rowIdMap there
  rowIdMap.clear();
  int id;
  for (int i = 0; i < numRows(); i++)
    {
      id = text(i, 0).toInt();
      rowIdMap.insert(id, i);
    }

  QObject::disconnect(this, SIGNAL(selectionChanged()), this, SLOT(handleChangedSelections()));
  clearSelection(true);

  //select the rows again after sorting

  Q3ValueList < int >::iterator it;
  for (it = idsOfSelected.begin(); it != idsOfSelected.end(); ++it)
    {
      selectRowWithId((*it));
    }
  QObject::connect(this, SIGNAL(selectionChanged()), this, SLOT(handleChangedSelections()));

  //change the sorting order after each sort
  (sort_ascending == true) ? sort_ascending = false : sort_ascending = true;

  QApplication::restoreOverrideCursor();
}

void QgsAttributeTable::keyPressEvent(QKeyEvent * ev)
{
  if (ev->key() == Qt::Key_Control || ev->key() == Qt::Key_Shift)
    {
      lockKeyPressed = true;
    }
}

void QgsAttributeTable::keyReleaseEvent(QKeyEvent * ev)
{
  if (ev->key() == Qt::Key_Control || ev->key() == Qt::Key_Shift)
    {
      lockKeyPressed = false;
    }
}

void QgsAttributeTable::handleChangedSelections()
{
  Q3TableSelection cselection;
  if (lockKeyPressed == false)
    {
      //clear the list and evaluate the last selection
      emit selectionRemoved();
    }
  //if there is no current selection, there is nothing to do
  if (currentSelection() == -1)
    {
      emit repaintRequested();
      return;
    }

  cselection = selection(currentSelection());

  for (int index = cselection.topRow(); index <= cselection.bottomRow(); index++)
    {
      emit selected(text(index, 0).toInt());
    }

  //emit repaintRequested();

}

void QgsAttributeTable::insertFeatureId(int id, int row)
{
  rowIdMap.insert(id, row);
}

void QgsAttributeTable::selectRowWithId(int id)
{
  QMap < int, int >::iterator it = rowIdMap.find(id);
  selectRow(it.data());
}

void QgsAttributeTable::sortColumn(int col, bool ascending, bool wholeRows)
{
  //if the first entry contains a letter, sort alphanumerically, otherwise numerically
  QString firstentry = text(0, col);
  bool containsletter = false;
  for (uint i = 0; i < firstentry.length(); i++)
    {
      if (firstentry.ref(i).isLetter())
        {
          containsletter = true;
        }
    }

  if (containsletter)
    {
      qsort(0, numRows() - 1, col, ascending, true);
  } else
    {
      qsort(0, numRows() - 1, col, ascending, false);
    }

  repaintContents();
}


/**
  XXX Doesn't QString have something ilke this already?
*/
int QgsAttributeTable::compareItems(QString s1, QString s2, bool ascending, bool alphanumeric)
{
  if (alphanumeric)
    {
      if (s1 > s2)
        {
          if (ascending)
            {
              return 1;
          } else
            {
              return -1;
            }
      } else if (s1 < s2)
        {
          if (ascending)
            {
              return -1;
          } else
            {
              return 1;
            }
      } else if (s1 == s2)
        {
          return 0;
        }
  } else                        //numeric
    {
      double d1 = s1.toDouble();
      double d2 = s2.toDouble();
      if (d1 > d2)
        {
          if (ascending)
            {
              return 1;
          } else
            {
              return -1;
            }
      } else if (d1 < d2)
        {
          if (ascending)
            {
              return -1;
          } else
            {
              return 1;
            }
      } else if (d1 == d2)
        {
          return 0;
        }
    }

  return 0;                     // XXX has to return something; is this reasonable?

}

void QgsAttributeTable::qsort(int lower, int upper, int col, bool ascending, bool alphanumeric)
{
  int i, j;
  QString v;
  if (upper > lower)
    {
      //chose a random element (this avoids n^2 worst case)
      int element = int ( (double)rand() / (double)RAND_MAX * (upper - lower) + lower);
      swapRows(element, upper);
      v = text(upper, col);
      i = lower - 1;
      j = upper;
      for (;;)
      {
	  while (compareItems(text(++i, col), v, ascending, alphanumeric) == -1);
	  while (compareItems(text(--j, col), v, ascending, alphanumeric) == 1 && j > 0); //make sure that j does not get negative
          if (i >= j)
            {
              break;
            }
          swapRows(i, j);
        }
      swapRows(i, upper);
      qsort(lower, i - 1, col, ascending, alphanumeric);
      qsort(i + 1, upper, col, ascending, alphanumeric);
    }
}

void QgsAttributeTable::contentsMouseReleaseEvent(QMouseEvent * e)
{
  contentsMouseMoveEvent(e);    //send out a move event to keep the selections updated 
  emit repaintRequested();
}

void QgsAttributeTable::popupMenu(int row, int col, const QPoint& pos)
{
  std::cerr << "context menu requested" << std::endl;

  // Duplication of code in qgsidentufyresults.cpp. Consider placing
  // in a seperate class
  if (mActionPopup == 0)
  {
    mActionPopup = new Q3PopupMenu();

    QLabel* popupLabel = new QLabel( mActionPopup );
    popupLabel->setText( tr("<center>Run action</center>") );
// TODO: Qt4 uses "QAction"s - need to refactor.
#if QT_VERSION < 0x040000
    mActionPopup->insertItem(popupLabel);
#endif
    mActionPopup->insertSeparator();

    QgsAttributeAction::aIter	iter = mActions.begin();
    for (int j = 0; iter != mActions.end(); ++iter, ++j)
    {
      int id = mActionPopup->insertItem(iter->name(), this, 
					SLOT(popupItemSelected(int)));
      mActionPopup->setItemParameter(id, j);
    }
  }

  // Get and store the attribute values and their column names are
  // these are needed for substituting into the actions if the user
  // chooses one.
  mActionValues.clear();
  Q3Header* header = horizontalHeader();

#ifdef QGISDEBUG
  if (header->count() != numCols())
    std::cerr << "Something wrong with the table (file " 
	      << __FILE__<< ", line " << __LINE__ 
	      << ")." << std::endl;
#endif

  for (int i = 0; i < numCols(); ++i)
    mActionValues.push_back(std::make_pair(header->label(i), text(row, i)));

  // The item that was clicked on, stored as an index into the
  // mActionValues vector.
  mClickedOnValue = col;

  mActionPopup->popup(pos);  
}

void QgsAttributeTable::popupItemSelected(int id)
{
  mActions.doAction(id, mActionValues, mClickedOnValue);
}

bool QgsAttributeTable::addAttribute(const QString& name, const QString& type)
{
    //first test if an attribute with the same name is already in the table
    for(int i=0;i<horizontalHeader()->count();++i)
    {
	if(horizontalHeader()->label(i)==name)
	{
	    //name conflict
	    return false;
	}
    }
    mAddedAttributes.insert(std::make_pair(name,type));
#ifdef QGISDEBUG
    qWarning(("inserting attribute "+name+" of type "+type).toLocal8Bit().data());
    //add a new column at the end of the table
    qWarning(("numCols: "+QString::number(numCols())).toLocal8Bit().data());
#endif
    insertColumns(numCols());
    horizontalHeader()->setLabel(numCols()-1,name);
    mEdited=true;
    return true;
}

void QgsAttributeTable::deleteAttribute(const QString& name)
{
    //check, if there is already an attribute with this name in mAddedAttributes
    std::map<QString,QString>::iterator iter=mAddedAttributes.find(name);
    if(iter!=mAddedAttributes.end())
    {
	mAddedAttributes.erase(iter);
	removeAttrColumn(name);
    }
    else
    {
#ifdef QGISDEBUG
	qWarning(("QgsAttributeTable: deleteAttribute "+name).toLocal8Bit().data());
#endif
	mDeletedAttributes.insert(name);
	removeAttrColumn(name);
    }
    mEdited=true;
}

void QgsAttributeTable::copySelectedRows()
{
  // Copy selected rows to the clipboard

  QString toClipboard;
  const char fieldSep = '\t';

  // Pick up the headers first
  Q3Header* header = horizontalHeader();
  for (int i = 0; i < header->count(); ++i)
    toClipboard += header->label(i) + fieldSep;
  toClipboard += '\n';

  // Then populate with the cell contents
  for (int i = 0; i < numSelections(); ++i)
  {
    Q3TableSelection sel = selection(i);
    for (int row = sel.topRow(); row < sel.topRow()+sel.numRows(); ++row)
    {
      for (int column = 0; column < numCols(); ++column)
	toClipboard += text(row, column) + fieldSep;
      toClipboard += '\n';
    }
  }
#ifdef QGISDEBUG
  std::cerr << "Selected data in table is:\n" << toClipboard.data();
#endif
  // And then copy to the clipboard
  QClipboard* clipboard = QApplication::clipboard();

  // With qgis running under Linux, but with a Windows based X
  // server (Xwin32), ::Selection was necessary to get the data into
  // the Windows clipboard (which seems contrary to the Qt
  // docs). With a Linux X server, ::Clipboard was required.
  // The simple solution was to put the text into both clipboards.

  // The ::Selection setText() below one may need placing inside so
  // #ifdef so that it doesn't get compiled under Windows.
  clipboard->setText(toClipboard, QClipboard::Selection);
  clipboard->setText(toClipboard, QClipboard::Clipboard);
}

bool QgsAttributeTable::commitChanges(QgsVectorLayer* layer)
{
    bool returnvalue=true;
    if(layer)
    {
	if(!layer->commitAttributeChanges(mDeletedAttributes, mAddedAttributes, mChangedValues))
	{
	    returnvalue=false;
	}
    }
    else
    {
	returnvalue=false;
    }
    mEdited=false;
    clearEditingStructures();
    return returnvalue;
}

bool QgsAttributeTable::rollBack(QgsVectorLayer* layer)
{
  if(layer)
  {
    fillTable(layer);
  }
  mEdited=false;
  clearEditingStructures();
  return true;
}


void QgsAttributeTable::fillTable(QgsVectorLayer* layer)
{
  QgsVectorDataProvider* provider=layer->getDataProvider();
  if(provider)
  {
    QgsFeature *fet;
    int row = 0;
  
    std::vector<QgsFeature*>& addedFeatures = layer->addedFeatures();
    std::set<int>& deletedFeatures = layer->deletedFeatureIds();

    // set up the column headers
    Q3Header *colHeader = horizontalHeader();
    std::vector < QgsField > fields = provider->fields();
    int fieldcount=provider->fieldCount();
#ifdef QGISDEBUG
    for (int l = 0; l < fields.size(); l++)
      std::cout << "field: " << fields[l].name().toLocal8Bit().data() << " | " << fields[l].isNumeric() << " | " << fields[l].type().toLocal8Bit().data() << std::endl;
#endif
  
    setNumRows(provider->featureCount() + addedFeatures.size() - deletedFeatures.size());
    setNumCols(fieldcount+1);
    colHeader->setLabel(0, "id"); //label for the id-column

    for (int h = 1; h <= fieldcount; h++)
    {
        colHeader->setLabel(h, fields[h - 1].name());
#ifdef QGISDEBUG
        qWarning("Setting column label "+fields[h - 1].name());
#endif
    }

    //go through the features and fill the values into the table
    provider->reset();
    while ((fet = provider->getNextFeature(true)))
    {
      if(deletedFeatures.find(fet->featureId()) == deletedFeatures.end())
      {
        putFeatureInTable(row, fet);
        row++;
      }
      delete fet;
    }

    //also consider the not commited features
    for(std::vector<QgsFeature*>::iterator it = addedFeatures.begin(); it != addedFeatures.end(); it++)
    {
      putFeatureInTable(row, *it);
      row++;
    }

    provider->reset();
  }
}

void QgsAttributeTable::putFeatureInTable(int row, QgsFeature* fet)
{
  //id-field
  int id = fet->featureId();
  setText(row, 0, QString::number(id));
  insertFeatureId(id, row);  //insert the id into the search tree of qgsattributetable
  const std::vector < QgsFeatureAttribute >& attr = fet->attributeMap();
  for (int i = 0; i < attr.size(); i++)
  {
    // get the field values
    setText(row, i + 1, attr[i].fieldValue());
  }
}

void QgsAttributeTable::storeChangedValue(int row, int column)
{
    //id column is not editable
    if(column>0)
    {
	//find feature id
	int id=text(row,0).toInt();
	QString attribute=horizontalHeader()->label(column);
#ifdef QGISDEBUG
	qWarning(("feature id: "+QString::number(id)).toLocal8Bit().data());
	qWarning(("attribute: "+attribute).toLocal8Bit().data());
#endif
	std::map<int,std::map<QString,QString> >::iterator iter=mChangedValues.find(id);
	if(iter==mChangedValues.end())
	{
	    std::map<QString,QString> themap;
	    mChangedValues.insert(std::make_pair(id,themap));
	    iter=mChangedValues.find(id);
	}
	iter->second.erase(attribute);
	iter->second.insert(std::make_pair(attribute,text(row,column)));
#ifdef QGISDEBUG
	qWarning(("value: "+text(row,column)).toLocal8Bit().data());
#endif	
	mEdited=true;
    }
}

void QgsAttributeTable::clearEditingStructures()
{
    mDeletedAttributes.clear();
    mAddedAttributes.clear();
    for(std::map<int,std::map<QString,QString> >::iterator iter=mChangedValues.begin();iter!=mChangedValues.end();++iter)
    {
	iter->second.clear();
    }
    mChangedValues.clear();
}

void QgsAttributeTable::removeAttrColumn(const QString& name)
{
    Q3Header* header=horizontalHeader();
    for(int i=0;i<header->count();++i)
    {
	if(header->label(i)==name)
	{
	    removeColumn(i);
	    break;
	}
    }
}

void QgsAttributeTable::bringSelectedToTop()
{
    blockSignals(true);
    int swaptorow=0;
    std::list<Q3TableSelection> selections;
    bool removeselection;

    for(int i=0;i<numSelections();++i)
    {
	selections.push_back(selection(i));
    }

    Q3TableSelection sel;  

    for(std::list<Q3TableSelection>::iterator iter=selections.begin();iter!=selections.end();++iter)
    {
	removeselection=true;
	while(isRowSelected(swaptorow, true))//selections are not necessary stored in ascending order
	    {
		++swaptorow;
	    }
	
	for(int j=iter->topRow();j<=iter->bottomRow();++j)
	{   
	    if(j>swaptorow)//selections are not necessary stored in ascending order
	    {
		swapRows(j,swaptorow);
		selectRow(swaptorow);
		++swaptorow;	
		
	    }
	    else
	    {
		removeselection=false;//keep selection
	    }
	}
	if(removeselection)
	{	    
	    removeSelection(*iter);
	}
    }

    //clear and rebuild rowIdMap.
    rowIdMap.clear();
    int id;
    for (int i = 0; i < numRows(); i++)
    {
      id = text(i, 0).toInt();
      rowIdMap.insert(id, i);
    }

    blockSignals(false);

}

void QgsAttributeTable::selectRowsWithId(const std::vector<int>& ids)
{
  // to select more rows at once effectively, we stop sending signals to handleChangedSelections()
  // otherwise it will repaint map everytime row is selected
  
  QObject::disconnect(this, SIGNAL(selectionChanged()), this, SLOT(handleChangedSelections()));
    
  clearSelection(false);
  for (int i = 0; i < ids.size(); i++)
  {
    selectRowWithId(ids[i]);
    emit selected(ids[i]);
  }
  
  QObject::connect(this, SIGNAL(selectionChanged()), this, SLOT(handleChangedSelections()));

  emit repaintRequested();
}

void QgsAttributeTable::showRowsWithId(const std::vector<int>& ids)
{
  setUpdatesEnabled(false);
  
  // hide all rows first
  for (int i = 0; i < numRows(); i++)
    hideRow(i);
  
  // show only matching rows
  for (int i = 0; i < ids.size(); i++)
    showRow(rowIdMap[ids[i]]);
  
  clearSelection(); // deselect all
  setUpdatesEnabled(true);
  repaintContents();
}

void QgsAttributeTable::showAllRows()
{
  for (int i = 0; i < numRows(); i++)
    showRow(i);
}
