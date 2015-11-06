/***************************************************************************
    qgsselectgrouplayerdialog.cpp
    -----------------------------
    begin                : January 2015
    copyright            : (C) 2015 by Marco Hugentobler
    email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsselectgrouplayerdialog.h"
#include "qgslayertree.h"
#include "qgslayertreemodel.h"

QgsSelectGroupLayerDialog::QgsSelectGroupLayerDialog( QgsLayerTreeModel* model, QWidget* parent, Qt::WindowFlags f ): QDialog( parent, f )
{
  setupUi( this );
  mTreeView->setModel( model );
}

QgsSelectGroupLayerDialog::~QgsSelectGroupLayerDialog()
{
}

QStringList QgsSelectGroupLayerDialog::selectedGroups() const
{
  QStringList groups;
  QgsLayerTreeModel* model = mTreeView->layerTreeModel();
  foreach ( QModelIndex index, mTreeView->selectionModel()->selectedIndexes() )
  {
    QgsLayerTreeNode* node = model->index2node( index );
    if ( QgsLayerTree::isGroup( node ) )
      groups << QgsLayerTree::toGroup( node )->name();
  }
  return groups;
}

QStringList QgsSelectGroupLayerDialog::selectedLayerIds() const
{
  QStringList layerIds;
  QgsLayerTreeModel* model = mTreeView->layerTreeModel();
  foreach ( QModelIndex index, mTreeView->selectionModel()->selectedIndexes() )
  {
    QgsLayerTreeNode* node = model->index2node( index );
    if ( QgsLayerTree::isLayer( node ) )
      layerIds << QgsLayerTree::toLayer( node )->layerId();
  }
  return layerIds;
}

QStringList QgsSelectGroupLayerDialog::selectedLayerNames() const
{
  QStringList layerNames;
  QgsLayerTreeModel* model = mTreeView->layerTreeModel();
  foreach ( QModelIndex index, mTreeView->selectionModel()->selectedIndexes() )
  {
    QgsLayerTreeNode* node = model->index2node( index );
    if ( QgsLayerTree::isLayer( node ) )
      layerNames << QgsLayerTree::toLayer( node )->layerName();
  }
  return layerNames;
}
