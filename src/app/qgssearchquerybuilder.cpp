/***************************************************************************
    qgssearchquerybuilder.cpp  - Query builder for search strings
    ----------------------
    begin                : March 2006
    copyright            : (C) 2006 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include <iostream>
#include <q3listbox.h>
#include <QMessageBox>
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgssearchquerybuilder.h"
#include "qgssearchstring.h"
#include "qgssearchtreenode.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"


QgsSearchQueryBuilder::QgsSearchQueryBuilder(QgsVectorLayer* layer,
                                             QWidget *parent, Qt::WFlags fl)
  : QDialog(parent, fl), mLayer(layer)
{
  setupUi(this);
  
  setWindowTitle("Search query builder");
  
  // disable unsupported operators
  btnIn->setEnabled(false);
  btnNotIn->setEnabled(false);
  btnPct->setEnabled(false);
  
  // change to ~
  btnILike->setText("~");
  
  lblDataUri->setText(layer->name());
  populateFields();
}

QgsSearchQueryBuilder::~QgsSearchQueryBuilder()
{
}


void QgsSearchQueryBuilder::populateFields()
{
  const QgsFieldMap& fields = mLayer->getDataProvider()->fields();
  for (QgsFieldMap::const_iterator it = fields.begin(); it != fields.end(); ++it)
  {
    QString fieldName = it->name();
    
    mFieldMap[fieldName] = it.key();
    lstFields->insertItem(fieldName);
  }
}

void QgsSearchQueryBuilder::getFieldValues(uint limit)
{
  // clear the values list 
  lstValues->clear();
  
  QgsVectorDataProvider* provider = mLayer->getDataProvider();
  
  // determine the field type
  QString fieldName = lstFields->currentText();
  int fieldIndex = mFieldMap[fieldName];
  QgsField field = provider->fields()[fieldIndex];
  bool numeric = (field.type() == QVariant::Int || field.type() == QVariant::Double);
  
  QgsFeature feat;
  QString value;

  QgsAttributeList attrs;
  attrs.append(fieldIndex);
  
  provider->select(attrs, QgsRect(), false);
  
  while (provider->getNextFeature(feat) &&
         (limit == 0 || lstValues->count() != limit))
  {
    const QgsAttributeMap& attributes = feat.attributeMap();
    value = attributes[fieldIndex].toString();
     
    if (!numeric)
    {
      // put string in single quotes
      value = "'" + value + "'";
    }
    
    // add item only if it's not there already
    if (lstValues->findItem(value) == 0)
      lstValues->insertItem(value);
    
  }
}

void QgsSearchQueryBuilder::on_btnSampleValues_clicked()
{
  getFieldValues(25);
}

void QgsSearchQueryBuilder::on_btnGetAllValues_clicked()
{
  getFieldValues(0);
}

void QgsSearchQueryBuilder::on_btnTest_clicked()
{
  long count = countRecords(txtSQL->text());
  
  // error?
  if (count == -1)
    return;

  QString str;
  if (count)
    str.sprintf(tr("Found %d matching features.","",count), count);
  else
    str = tr("No matching features found.");
  QMessageBox::information(this, tr("Search results"), str);
}

// This method tests the number of records that would be returned
long QgsSearchQueryBuilder::countRecords(QString searchString) 
{
  QgsSearchString search;
  if (!search.setString(searchString))
  {
    QMessageBox::critical(this, tr("Search string parsing error"), search.parserErrorMsg());
    return -1;
  }
  
  QgsSearchTreeNode* searchTree = search.tree();
  if (searchTree == NULL)
  {
    // entered empty search string
    return mLayer->featureCount();
  }
  
  QApplication::setOverrideCursor(Qt::waitCursor);
  
  int count = 0;
  QgsFeature feat;
  QgsVectorDataProvider* provider = mLayer->getDataProvider();
  const QgsFieldMap& fields = provider->fields();
  QgsAttributeList allAttributes = provider->allAttributesList();

  provider->select(allAttributes, QgsRect(), false);

  while (provider->getNextFeature(feat))
  {
    if (searchTree->checkAgainst(fields, feat.attributeMap()))
    {
      count++;
    }
    
    // check if there were errors during evaulating
    if (searchTree->hasError())
      break;
  }

  QApplication::restoreOverrideCursor();
  
  return count;
}


void QgsSearchQueryBuilder::on_btnOk_clicked()
{
  // if user hits Ok and there is no query, skip the validation
  if(txtSQL->text().stripWhiteSpace().length() > 0)
  {
    accept();
    return;
  }

  // test the query to see if it will result in a valid layer
  long numRecs = countRecords(txtSQL->text());
  if (numRecs == -1)
  {
    // error shown in countRecords
  }
  else if (numRecs == 0)
  {
    QMessageBox::warning(this, tr("No Records"), tr("The query you specified results in zero records being returned."));
  }
  else
  {
    accept();
  }

}

void QgsSearchQueryBuilder::on_btnEqual_clicked()
{
  txtSQL->insert(" = ");
}

void QgsSearchQueryBuilder::on_btnLessThan_clicked()
{
  txtSQL->insert(" < ");
}

void QgsSearchQueryBuilder::on_btnGreaterThan_clicked()
{
  txtSQL->insert(" > ");
}

void QgsSearchQueryBuilder::on_btnPct_clicked()
{
  txtSQL->insert(" % ");
}

void QgsSearchQueryBuilder::on_btnIn_clicked()
{
  txtSQL->insert(" IN ");
}

void QgsSearchQueryBuilder::on_btnNotIn_clicked()
{
  txtSQL->insert(" NOT IN ");
}

void QgsSearchQueryBuilder::on_btnLike_clicked()
{
  txtSQL->insert(" LIKE ");
}

QString QgsSearchQueryBuilder::searchString()
{
  return txtSQL->text();
}

void QgsSearchQueryBuilder::setSearchString(QString searchString)
{
  txtSQL->setText(searchString);
}

void QgsSearchQueryBuilder::on_lstFields_doubleClicked( Q3ListBoxItem *item )
{
  txtSQL->insert(item->text());
}

void QgsSearchQueryBuilder::on_lstValues_doubleClicked( Q3ListBoxItem *item )
{
  txtSQL->insert(item->text());
}

void QgsSearchQueryBuilder::on_btnLessEqual_clicked()
{
  txtSQL->insert(" <= ");
}

void QgsSearchQueryBuilder::on_btnGreaterEqual_clicked()
{
  txtSQL->insert(" >= ");
}

void QgsSearchQueryBuilder::on_btnNotEqual_clicked()
{
  txtSQL->insert(" != ");
}

void QgsSearchQueryBuilder::on_btnAnd_clicked()
{
  txtSQL->insert(" AND ");
}

void QgsSearchQueryBuilder::on_btnNot_clicked()
{
  txtSQL->insert(" NOT ");
}

void QgsSearchQueryBuilder::on_btnOr_clicked()
{
  txtSQL->insert(" OR ");
}

void QgsSearchQueryBuilder::on_btnClear_clicked()
{
  txtSQL->clear();
}

void QgsSearchQueryBuilder::on_btnILike_clicked()
{
  //txtSQL->insert(" ILIKE ");
  txtSQL->insert(" ~ ");
}

