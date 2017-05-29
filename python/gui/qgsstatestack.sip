class QgsStateStack : public QObject {
%TypeHeaderCode
#include <qgsstatestack.h>
%End
public:
  class State {
  public:
    virtual ~State();
  };

  class StateChangeCommand
  {
    public:
      StateChangeCommand( QgsStateStack* stateStack, QgsStateStack::State* newState, bool compress );
      virtual void undo();
      virtual void redo();
      virtual bool compress() const;
  };

  QgsStateStack( QgsStateStack::State* initialState, QObject* parent = 0 );
  ~QgsStateStack();
  void clear(QgsStateStack::State* cleanState);
  void updateState( QgsStateStack::State* newState, bool mergeable = false );
  void push( StateChangeCommand* command );
  void undo();
  void redo();
  bool canUndo() const;
  bool canRedo() const;
  const QgsStateStack::State* state() const;
  QgsStateStack::State* mutableState();

signals:
  void canUndoChanged( bool );
  void canRedoChanged( bool );
  void stateChanged();
};
