/***************************************************************************
                          QgsAttributeTableDisplay.cpp  -  description
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

#include "qgsattributetabledisplay.h"

#include "qgisapp.h"
#include "qgsapplication.h"
#include "qgsaddattrdialog.h"
#include "qgsdelattrdialog.h"
#include "qgsfeature.h"
#include "qgssearchquerybuilder.h"
#include "qgssearchtreenode.h"
#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"
#include "qgscontexthelp.h"

#include <QMessageBox>
#include <QIcon>
#include <QPixmap>
#include <QToolButton>

QgsAttributeTableDisplay::QgsAttributeTableDisplay(QgsVectorLayer* layer, QgisApp * qgisApp)
: QDialog(),
  mLayer(layer),
  mQgisApp(qgisApp)
{
  setupUi(this);
  setTheme();
  connect(mRemoveSelectionButton, SIGNAL(clicked()), this, SLOT(removeSelection()));
  connect(mSelectedToTopButton, SIGNAL(clicked()), this, SLOT(selectedToTop()));
  connect(mInvertSelectionButton, SIGNAL(clicked()), this, SLOT(invertSelection()));
  connect(mCopySelectedRowsButton, SIGNAL(clicked()), this, SLOT(copySelectedRowsToClipboard()));
  connect(mAddAttributeButton, SIGNAL(clicked()), this, SLOT(addAttribute()));
  connect(mDeleteAttributeButton, SIGNAL(clicked()), this, SLOT(deleteAttributes()));
  connect(btnStartEditing, SIGNAL(clicked()), this, SLOT(startEditing()));
  connect(btnStopEditing, SIGNAL(clicked()), this, SLOT(stopEditing()));
  connect(mSearchButton, SIGNAL(clicked()), this, SLOT(search()));
  connect(mSearchShowResults, SIGNAL(activated(int)), this, SLOT(searchShowResultsChanged(int)));
  connect(btnAdvancedSearch, SIGNAL(clicked()), this, SLOT(advancedSearch()));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(close()));

  mAddAttributeButton->setEnabled(false);
  mDeleteAttributeButton->setEnabled(false);

  btnStopEditing->setEnabled(false);
  int cap=layer->getDataProvider()->capabilities();
  if((cap&QgsVectorDataProvider::ChangeAttributeValues)
      ||(cap&QgsVectorDataProvider::AddAttributes)
      ||(cap&QgsVectorDataProvider::DeleteAttributes))
  {
    btnStartEditing->setEnabled(true);
  }
  else
  {
    btnStartEditing->setEnabled(false);
  }

  // fill in mSearchColumns with available columns
  QgsVectorDataProvider* provider = mLayer->getDataProvider();
  if (provider)
  {
    std::vector < QgsField > fields = provider->fields();
    int fieldcount = provider->fieldCount();
    for (int h = 1; h <= fieldcount; h++)
    {
      mSearchColumns->insertItem(fields[h - 1].name());
    }
  }
  
  // TODO: create better labels
  mSearchShowResults->insertItem(tr("select"));
  mSearchShowResults->insertItem(tr("select and bring to top"));
  mSearchShowResults->insertItem(tr("show only matching"));
}

QgsAttributeTableDisplay::~QgsAttributeTableDisplay()
{
}
QgsAttributeTable *QgsAttributeTableDisplay::table()
{
  return tblAttributes;
}
void QgsAttributeTableDisplay::setTheme()
{
  QString myIconPath = QgsApplication::themePath();
  mAddAttributeButton->setPixmap(QPixmap(myIconPath+"/mActionNewAttribute.png"));
  mRemoveSelectionButton->setPixmap(QPixmap(myIconPath+"/mActionUnselectAttributes.png"));
  mSelectedToTopButton->setPixmap(QPixmap(myIconPath+"/mActionSelectedToTop.png"));
  mInvertSelectionButton->setPixmap(QPixmap(myIconPath+"/mActionInvertSelection.png"));
  mCopySelectedRowsButton->setPixmap(QPixmap(myIconPath+"/mActionCopySelected.png"));
  mAddAttributeButton->setPixmap(QPixmap(myIconPath+"/mActionNewAttribute.png"));
  mDeleteAttributeButton->setPixmap(QPixmap(myIconPath+"/mActionDeleteAttribute.png"));
}

void QgsAttributeTableDisplay::setTitle(QString title)
{
  setCaption(title);
}

void QgsAttributeTableDisplay::deleteAttributes()
{
  QgsDelAttrDialog dialog(table()->horizontalHeader());
  if(dialog.exec()==QDialog::Accepted)
  {
    const std::list<QString>* attlist=dialog.selectedAttributes();
    for(std::list<QString>::const_iterator iter=attlist->begin();iter!=attlist->end();++iter)
    {
      table()->deleteAttribute(*iter);
    }
  }
}

void QgsAttributeTableDisplay::addAttribute()
{
  QgsAddAttrDialog dialog(mLayer->getDataProvider());
  if(dialog.exec()==QDialog::Accepted)
  {
    if(!table()->addAttribute(dialog.name(),dialog.type()))
    {
      QMessageBox::information(0,"Name conflict","The attribute could not be inserted. The name already exists in the table",QMessageBox::Ok);
    }
  }
}

void QgsAttributeTableDisplay::startEditing()
{
  QgsVectorDataProvider* provider=mLayer->getDataProvider();
  bool editing=false; 

  if(provider)
  {
    if(provider->capabilities()&QgsVectorDataProvider::AddAttributes)
    {
      mAddAttributeButton->setEnabled(true);
      editing=true;
    }
    if(provider->capabilities()&QgsVectorDataProvider::DeleteAttributes)
    {
      
      mDeleteAttributeButton->setEnabled(true);
      editing=true;
    }
    if(provider->capabilities()&QgsVectorDataProvider::ChangeAttributeValues)
    {
      table()->setReadOnly(false);
      table()->setColumnReadOnly(0,true);//id column is not editable
      editing=true;
    }
    if(editing)
    {
      btnStartEditing->setEnabled(false);
      btnStopEditing->setEnabled(true);
      btnClose->setEnabled(false);
      //make the dialog modal when in editable
      //otherwise map editing and table editing
      //may disturb each other
      hide();
      setModal(true);
      show();
    }
  }
}

void QgsAttributeTableDisplay::stopEditing()
{
  if(table()->edited())
  {
    //commit or roll back?
    int commit=QMessageBox::information(0,"Stop editing","Do you want to save the changes?",QMessageBox::Yes,QMessageBox::No);
    if(commit==QMessageBox::Yes)
    {
      if(!table()->commitChanges(mLayer))
      {
        QMessageBox::information(0,"Error","Could not commit changes",QMessageBox::Ok);
      }
    }
    else
    {
      table()->rollBack(mLayer);
    }
  }
  btnStartEditing->setEnabled(true);
  btnStopEditing->setEnabled(false);
  btnClose->setEnabled(true);
  mAddAttributeButton->setEnabled(false);
  mDeleteAttributeButton->setEnabled(false);
  table()->setReadOnly(true);
  //make this dialog modeless again
  hide();
  setModal(false);
  show();
}

void QgsAttributeTableDisplay::selectedToTop()
{
  table()->bringSelectedToTop();
}

void QgsAttributeTableDisplay::invertSelection()
{
  if(mLayer)
  {
    mLayer->invertSelection();
  }
}

void QgsAttributeTableDisplay::removeSelection()
{
    mLayer->removeSelection();
    table()->clearSelection();
    mLayer->triggerRepaint();
}

void QgsAttributeTableDisplay::copySelectedRowsToClipboard()
{
  // Deprecated
  // table()->copySelectedRows();

  // Use the Application's copy method instead
  mQgisApp->editCopy(mLayer);
}

void QgsAttributeTableDisplay::search()
{
  // if selected field is numeric, numeric comparison will be used
  // else attributes containing entered text will be matched

  QgsVectorDataProvider* provider = mLayer->getDataProvider();
  int item = mSearchColumns->currentItem();
  bool numeric = provider->fields()[item].isNumeric();
  
  QString str;
  str = mSearchColumns->currentText();
  if (numeric)
    str += " = '";
  else
    str += " ~ '";
  str += mSearchText->text();
  str += "'";

  doSearch(str);
}


void QgsAttributeTableDisplay::advancedSearch()
{
  QgsSearchQueryBuilder dlg(mLayer, this);
  dlg.setSearchString(mSearchString);
  if (dlg.exec())
  {
    doSearch(dlg.searchString());
  }
}


void QgsAttributeTableDisplay::searchShowResultsChanged(int item)
{
  QApplication::setOverrideCursor(Qt::waitCursor);

  if (item == 2) // show only matching
  {
    table()->showRowsWithId(mSearchIds);
  }
  else
  {    
    // make sure that all rows are shown
    table()->showAllRows();
    
    // select matching
    table()->selectRowsWithId(mSearchIds);
  
    if (item == 1) // select matching and bring to top
      table()->bringSelectedToTop();
  }

  QApplication::restoreOverrideCursor();
}


void QgsAttributeTableDisplay::doSearch(const QString& searchString)
{
  mSearchString = searchString;

  // parse search string (and build parsed tree)
  QgsSearchString search;
  if (!search.setString(searchString))
  {
    QMessageBox::critical(this, tr("Search string parsing error"), search.parserErrorMsg());
    return;
  }
  QgsSearchTreeNode* searchTree = search.tree();
  if (searchTree == NULL)
  {
    QMessageBox::information(this, tr("Search results"), tr("You've supplied an empty search string."));
    return;
  }

#ifdef QGISDEBUG
  std::cout << "Search by attribute: " << searchString.toLocal8Bit().data() << std::endl
            << " parsed as: " << search.tree()->makeSearchString().toLocal8Bit().data() << std::endl;
#endif

  QApplication::setOverrideCursor(Qt::waitCursor);

  // TODO: need optimized getNextFeature which won't extract geometry
  // or search by traversing table ... which one is quicker?
  QgsFeature* fet;
  QgsVectorDataProvider* provider = mLayer->getDataProvider();
  provider->reset();
  mSearchIds.clear();
  while ((fet = provider->getNextFeature(true)))
  {
    if (searchTree->checkAgainst(fet->attributeMap()))
    {
      mSearchIds.push_back(fet->featureId());
    }
    delete fet;
    
    // check if there were errors during evaulating
    if (searchTree->hasError())
      break;
  }
  provider->reset();

  QApplication::restoreOverrideCursor();

  if (searchTree->hasError())
  {
    QMessageBox::critical(this, tr("Error during search"), searchTree->errorMsg());
    return;
  }

  // update table
  searchShowResultsChanged(mSearchShowResults->currentItem());
   
  QString str;
  if (mSearchIds.size())
    str.sprintf(tr("Found %d matching features."), mSearchIds.size());
  else
    str = tr("No matching features found.");
  QMessageBox::information(this, tr("Search results"), str);

}

void QgsAttributeTableDisplay::closeEvent(QCloseEvent* ev)
{
  ev->ignore();
  emit deleted();
  delete this;
}

void QgsAttributeTableDisplay::on_btnHelp_clicked()
{
  showHelp();
}
void QgsAttributeTableDisplay::showHelp()
{
  QgsContextHelp::run(context_id);
}
