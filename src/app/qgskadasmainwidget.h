#ifndef QGSKADASMAINWIDGET_H
#define QGSKADASMAINWIDGET_H

#include "ui_qgskadasmainwidgetbase.h"

class QgsLayerTreeView;
class QgsLayerTreeMapCanvasBridge;

class QgsKadasMainWidget: public QWidget, private Ui::QgsKadasMainWidgetBase
{
    Q_OBJECT
  public:
    QgsKadasMainWidget( QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsKadasMainWidget();

  private slots:
    void open();
    //! project was read
    void readProject( const QDomDocument & );

  private:
    void setActionToButton( QAction* action, QPushButton* button );

    bool addProject( const QString& projectFile );
    //! check to see if file is dirty and if so, prompt the user th save it
    bool saveDirty();
    //! mark project dirty
    void markDirty();
    //! Save project. Returns true if the user selected a file to save to, false if not.
    bool fileSave();
    /** add this file to the recently opened/saved projects list
     *  pass settings by reference since creating more than one
     * instance simultaneously results in data loss.
     */
    void saveRecentProjectPath( QString projectPath, QSettings & settings );
    //! Update project menu with the current list of recently accessed projects
    void updateRecentProjectPaths();
    //! clear out any stuff from project
    void closeProject();
    void removeAllLayers();

    //! list of recently opened/saved project files
    QStringList mRecentProjectPaths;
    //! Table of contents (legend) for the map
    QgsLayerTreeView* mLayerTreeView;
    //! Helper class that connects layer tree with map canvas
    QgsLayerTreeMapCanvasBridge* mLayerTreeCanvasBridge;
};

#endif // QGSKADASMAINWIDGET_H
