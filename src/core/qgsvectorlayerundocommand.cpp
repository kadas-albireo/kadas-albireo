/***************************************************************************
    qgsvectorlayerundocommand.cpp
    ---------------------
    begin                : June 2009
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

#include "qgsvectorlayerundocommand.h"

#include "qgsgeometry.h"
#include "qgsvectorlayer.h"

#include "qgslogger.h"

QgsUndoCommand::GeometryChangeEntry::GeometryChangeEntry()
    : original( NULL ), target( NULL )
{
}

void QgsUndoCommand::GeometryChangeEntry::setOriginalGeometry( QgsGeometry& orig )
{
  if ( orig.type() != QGis::UnknownGeometry )
  {
    original = new QgsGeometry( orig );
  }
  else
  {
    original = NULL;
  }
}

void QgsUndoCommand::GeometryChangeEntry::setTargetGeometry( QgsGeometry& dest )
{
  if ( dest.type() != QGis::UnknownGeometry )
  {
    target = new QgsGeometry( dest );
  }
  else
  {
    target = NULL;
  }
}


QgsUndoCommand::GeometryChangeEntry::~GeometryChangeEntry()
{
  delete original;
  delete target;
}


QgsUndoCommand::QgsUndoCommand( QgsVectorLayer* vlayer, QString text )
    : QUndoCommand()
{
  setText( text );
  mLayer = vlayer;
  mFirstRun = true;
}

void QgsUndoCommand::redo()
{
  // when the command is added to the undo stack, the redo() function is called
  // but we have already done the changes in the layer, so we ignore the first redo() call
  if ( mFirstRun )
  {
    mFirstRun = false;
    return;
  }

  mLayer->redoEditCommand( this );
}

void QgsUndoCommand::undo()
{
  mLayer->undoEditCommand( this );
}


void QgsUndoCommand::storeGeometryChange( QgsFeatureId featureId, QgsGeometry& original, QgsGeometry& target )
{
  if ( mGeometryChange.contains( featureId ) )
  {
    // geometry has been modified already: just alter the resulting geometry
    mGeometryChange[featureId].setTargetGeometry( target );
  }
  else
  {
    // create new entry about changed geometry
    mGeometryChange.insert( featureId, GeometryChangeEntry() );
    mGeometryChange[featureId].setOriginalGeometry( original );
    mGeometryChange[featureId].setTargetGeometry( target );
  }
}

void QgsUndoCommand::storeAttributeChange( QgsFeatureId featureId, int field, QVariant original, QVariant target, bool isFirstChange )
{
  AttributeChangeEntry entry;
  entry.isFirstChange = isFirstChange;
  entry.original = original;
  entry.target = target;
  mAttributeChange[featureId].insert( field, entry );
}

void QgsUndoCommand::storeAttributeAdd( int index, const QgsField & value )
{
  mAddedAttributes.insert( index, value );
}

void QgsUndoCommand::storeAttributeDelete( int index, const QgsField & orig )
{
  mDeletedAttributes.insert( index, orig );
}

void QgsUndoCommand::storeFeatureDelete( QgsFeatureId featureId )
{
  mDeletedFeatureIdChange.insert( featureId );
}

void QgsUndoCommand::storeFeatureAdd( QgsFeature& feature )
{
  mAddedFeatures.append( feature );
}
