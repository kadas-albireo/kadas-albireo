/***************************************************************************
    QgsStateStack.cpp
    -------------------------------------------------------------
  begin                : May 16, 2017
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

#include "qgsstatestack.h"

QgsStateStack::QgsStateStack( State *initialState, QObject* parent )
    : QObject( parent ), mState( initialState )
{}

QgsStateStack::~QgsStateStack()
{
  qDeleteAll( mUndoStack );
  qDeleteAll( mRedoStack );
}

void QgsStateStack::clear( State* cleanState )
{
  qDeleteAll( mUndoStack );
  mUndoStack.clear();
  qDeleteAll( mRedoStack );
  mRedoStack.clear();
  emit canUndoChanged( false );
  emit canRedoChanged( false );
  mState = QSharedPointer<State>( cleanState );
  emit stateChanged();
}

void QgsStateStack::undo()
{
  while ( !mUndoStack.isEmpty() )
  {
    StateChangeCommand* command = mUndoStack.pop();
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

void QgsStateStack::redo()
{
  while ( !mRedoStack.isEmpty() )
  {
    StateChangeCommand* command = mRedoStack.pop();
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

void QgsStateStack::updateState( State *newState, bool mergeable )
{
  push( new StateChangeCommand( this, newState, mergeable ) );
}

void QgsStateStack::push( StateChangeCommand* command )
{
  qDeleteAll( mRedoStack );
  mRedoStack.clear();
  mUndoStack.push( command );
  command->redo();
  emit canUndoChanged( true );
}
///////////////////////////////////////////////////////////////////////////////

QgsStateStack::StateChangeCommand::StateChangeCommand( QgsStateStack* stateStack, State* nextState, bool compress )
    : mStateStack( stateStack ), mPrevState( stateStack->mState ), mNextState( nextState ), mCompress( compress )
{
}

void QgsStateStack::StateChangeCommand::undo()
{
  mStateStack->mState = mPrevState;
  emit mStateStack->stateChanged();
}

void QgsStateStack::StateChangeCommand::redo()
{
  mStateStack->mState = mNextState;
  emit mStateStack->stateChanged();
}
