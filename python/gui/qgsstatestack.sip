class QgsStateStack : public QObject {
%TypeHeaderCode
#include <qgsstatestack.h>
%End
public:
  struct State {
    virtual ~State();
  };

  class StateChangeCommand
  {
    public:
      StateChangeCommand( QgsStateStack* stateStack, State* newState, bool compress );
      virtual void undo();
      virtual void redo();
      virtual bool compress() const;
  };

  QgsStateStack( State* initialState, QObject* parent = 0 );
  ~QgsStateStack();
  void clear(State* cleanState);
  void updateState( State* newState, bool mergeable = false );
  void push( StateChangeCommand* command );
  void undo();
  void redo();
  bool canUndo() const;
  bool canRedo() const;
  const State* state() const;
  State* mutableState();

signals:
  void canUndoChanged( bool );
  void canRedoChanged( bool );
  void stateChanged();
};
