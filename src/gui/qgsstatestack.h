/***************************************************************************
    QgsStateStack.h
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

#ifndef QGSSTATESTACK_H
#define QGSSTATESTACK_H

#include <QObject>
#include <QSharedPointer>
#include <QStack>

class GUI_EXPORT QgsStateStack : public QObject
{
    Q_OBJECT
  public:
    struct State
    {
      virtual ~State() {}
    };

    class GUI_EXPORT StateChangeCommand
    {
      public:
        StateChangeCommand( QgsStateStack* stateStack, State* newState, bool compress );
        virtual void undo();
        virtual void redo();
        virtual bool compress() const { return mCompress; }
      private:
        QgsStateStack* mStateStack;
        QSharedPointer<State> mPrevState, mNextState;
        bool mCompress;
    };

    QgsStateStack( State* initialState, QObject* parent = 0 );
    ~QgsStateStack();
    void clear( State* cleanState );
    void updateState( State* newState, bool mergeable = false );
    void push( StateChangeCommand* command );
    void undo();
    void redo();
    bool canUndo() const { return !mUndoStack.isEmpty(); }
    bool canRedo() const { return !mRedoStack.isEmpty(); }
    const State* state() const { return mState.data(); }
    State* mutableState() { return mState.data(); }

  signals:
    void canUndoChanged( bool );
    void canRedoChanged( bool );
    void stateChanged();

  protected:
    QSharedPointer<State> mState;

  private:
    QStack<StateChangeCommand*> mUndoStack;
    QStack<StateChangeCommand*> mRedoStack;
};

#endif // QGSSTATESTACK_H
