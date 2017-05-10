/***************************************************************************
    qgsundostack.cpp
    -------------------------------------------------------------
  begin                : May 10, 2017
  copyright            : (C) 2017 by Sandro Mani
  email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsundostack.h"

QgsUndoStack::QgsUndoStack( QObject* parent )
    : QObject( parent )
{}

QgsUndoStack::~QgsUndoStack()
{
  qDeleteAll( mUndoStack );
  qDeleteAll( mRedoStack );
}

void QgsUndoStack::clear()
{
  qDeleteAll( mUndoStack );
  mUndoStack.clear();
  qDeleteAll( mRedoStack );
  mRedoStack.clear();
  emit canUndoChanged( false );
  emit canRedoChanged( false );
}

void QgsUndoStack::push( QgsUndoCommand* command )
{
  qDeleteAll( mRedoStack );
  mRedoStack.clear();
  mUndoStack.push( command );
  command->redo();
  emit canUndoChanged( true );
}

void QgsUndoStack::undo()
{
  while ( !mUndoStack.isEmpty() )
  {
    QgsUndoCommand* command = mUndoStack.pop();
    command->undo();
    mRedoStack.push( command );
    if ( !command->compress() )
    {
      break;
    }
  }
  emit canUndoChanged( !mUndoStack.isEmpty() );
  emit canRedoChanged( !mRedoStack.isEmpty() );
}

void QgsUndoStack::redo()
{
  while ( !mRedoStack.isEmpty() )
  {
    QgsUndoCommand* command = mRedoStack.pop();
    command->redo();
    mUndoStack.push( command );
    if ( mRedoStack.isEmpty() || !mRedoStack.top()->compress() )
    {
      break;
    }
  }
  emit canUndoChanged( !mUndoStack.isEmpty() );
  emit canRedoChanged( !mRedoStack.isEmpty() );
}
