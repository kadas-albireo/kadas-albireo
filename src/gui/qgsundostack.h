/***************************************************************************
    qgsundostack.h
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

#ifndef QGSUNDOSTACK_H
#define QGSUNDOSTACK_H

#include <QObject>
#include <QStack>

class GUI_EXPORT QgsUndoCommand
{
  public:
    virtual ~QgsUndoCommand() {}
    virtual void undo() = 0;
    virtual void redo() = 0;
    virtual bool compress() const { return false; }
};

class GUI_EXPORT QgsUndoStack : public QObject
{
    Q_OBJECT
  public:
    QgsUndoStack( QObject* parent = 0 );
    ~QgsUndoStack();
    void clear();
    void push( QgsUndoCommand* command );
    void undo();
    void redo();
    bool canUndo() const { return !mUndoStack.isEmpty(); }
    bool canRedo() const { return !mRedoStack.isEmpty(); }

  signals:
    void canUndoChanged( bool );
    void canRedoChanged( bool );

  private:
    QStack<QgsUndoCommand*> mUndoStack;
    QStack<QgsUndoCommand*> mRedoStack;
};

#endif // QGSUNDOSTACK_H
