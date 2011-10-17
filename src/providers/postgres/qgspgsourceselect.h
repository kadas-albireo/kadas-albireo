/***************************************************************************
                          qgpgsourceselect.h  -  description
                             -------------------
    begin                : Sat Jun 22 2002
    copyright            : (C) 2002 by Gary E.Sherman
    email                : sherman at mrcc.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSPGSOURCESELECT_H
#define QGSPGSOURCESELECT_H
#include "ui_qgsdbsourceselectbase.h"
#include "qgisgui.h"
#include "qgsdbfilterproxymodel.h"
#include "qgsdbtablemodel.h"
#include "qgscontexthelp.h"

extern "C"
{
#include <libpq-fe.h>
}

#include <QThread>
#include <QMap>
#include <QPair>
#include <QIcon>
#include <QItemDelegate>

class QPushButton;
class QStringList;
class QgsGeomColumnTypeThread;
class QgisApp;

class QgsPgSourceSelectDelegate : public QItemDelegate
{
    Q_OBJECT;

  public:
    QgsPgSourceSelectDelegate( QObject *parent = NULL ) : QItemDelegate( parent )
    {
    }

    /** Used to create an editor for when the user tries to
     * change the contents of a cell */
    QWidget *createEditor(
      QWidget *parent,
      const QStyleOptionViewItem &option,
      const QModelIndex &index ) const
    {
      Q_UNUSED( option );
      if ( index.column() == QgsDbTableModel::dbtmSql )
      {
        QLineEdit *le = new QLineEdit( parent );
        le->setText( index.data( Qt::DisplayRole ).toString() );
        return le;
      }


      if ( index.column() == QgsDbTableModel::dbtmPkCol )
      {
        QStringList values = index.data( Qt::UserRole + 1 ).toStringList();

        if ( values.size() > 0 )
        {
          QComboBox *cb = new QComboBox( parent );
          cb->addItems( values );
          cb->setCurrentIndex( cb->findText( index.data( Qt::DisplayRole ).toString() ) );
          return cb;
        }
      }

      return NULL;
    }

    void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const
    {
      QComboBox *cb = qobject_cast<QComboBox *>( editor );
      if ( cb )
        model->setData( index, cb->currentText() );

      QLineEdit *le = qobject_cast<QLineEdit *>( editor );
      if ( le )
        model->setData( index, le->text() );
    }
};


/*! \class QgsPgSourceSelect
 * \brief Dialog to create connections and add tables from PostgresQL.
 *
 * This dialog allows the user to define and save connection information
 * for PostGIS enabled PostgreSQL databases. The user can then connect and add
 * tables from the database to the map canvas.
 */
class QgsPgSourceSelect : public QDialog, private Ui::QgsDbSourceSelectBase
{
    Q_OBJECT

  public:

    //! static function to delete a connection
    static void deleteConnection( QString key );


    //! Constructor
    QgsPgSourceSelect( QWidget *parent = 0, Qt::WFlags fl = QgisGui::ModalDialogFlags, bool managerMode = false, bool embeddedMode = false );
    //! Destructor
    ~QgsPgSourceSelect();
    //! Populate the connection list combo box
    void populateConnectionList();
    //! String list containing the selected tables
    QStringList selectedTables();
    //! Connection info (database, host, user, password)
    QString connectionInfo();

  signals:
    void addDatabaseLayers( QStringList const & layerPathList,
                            QString const & providerKey );
    void connectionsChanged();

  public slots:
    //! Determines the tables the user selected and closes the dialog
    void addTables();
    void buildQuery();

    /*! Connects to the database using the stored connection parameters.
    * Once connected, available layers are displayed.
    */
    void on_btnConnect_clicked();
    void on_cbxAllowGeometrylessTables_stateChanged( int );
    //! Opens the create connection dialog to build a new connection
    void on_btnNew_clicked();
    //! Opens a dialog to edit an existing connection
    void on_btnEdit_clicked();
    //! Deletes the selected connection
    void on_btnDelete_clicked();
    //! Saves the selected connections to file
    void on_btnSave_clicked();
    //! Loads the selected connections from file
    void on_btnLoad_clicked();
    void on_mSearchTableEdit_textChanged( const QString & text );
    void on_mSearchColumnComboBox_currentIndexChanged( const QString & text );
    void on_mSearchModeComboBox_currentIndexChanged( const QString & text );
    void setSql( const QModelIndex& index );
    //! Store the selected database
    void on_cmbConnections_activated( int );
    void setLayerType( QString schema, QString table, QString column,
                       QString type );
    void on_mTablesTreeView_clicked( const QModelIndex &index );
    void on_mTablesTreeView_doubleClicked( const QModelIndex &index );
    //!Sets a new regular expression to the model
    void setSearchExpression( const QString& regexp );

    void on_buttonBox_helpRequested() { QgsContextHelp::run( metaObject()->className() ); }

  private:
    typedef QPair<QString, QString> geomPair;
    typedef QList<geomPair> geomCol;

    //! Connections manager mode
    bool mManagerMode;

    //! Embedded mode, without 'Close'
    bool mEmbeddedMode;

    // queue another query for the thread
    void addSearchGeometryColumn( const QString &schema, const QString &table, const QString &column );

    // Set the position of the database connection list to the last
    // used one.
    void setConnectionListPosition();
    // Combine the schema, table and column data into a single string
    // useful for display to the user
    QString fullDescription( QString schema, QString table, QString column, QString type );
    // The column labels
    QStringList mColumnLabels;
    // Our thread for doing long running queries
    QgsGeomColumnTypeThread* mColumnTypeThread;
    QString m_connInfo;
    QStringList m_selectedTables;
    bool mUseEstimatedMetadata;
    // Storage for the range of layer type icons
    QMap<QString, QPair<QString, QIcon> > mLayerIcons;

    //! Model that acts as datasource for mTableTreeWidget
    QgsDbTableModel mTableModel;
    QgsDbFilterProxyModel mProxyModel;

    QString layerURI( const QModelIndex &index );
    QPushButton *mBuildQueryButton;
    QPushButton *mAddButton;
};


// Perhaps this class should be in its own file??
//
// A class that determines the geometry type of a given database
// schema.table.column, with the option of doing so in a separate
// thread.

class QgsGeomColumnTypeThread : public QThread
{
    Q_OBJECT
  public:

    void setConnInfo( QString s, bool useEstimatedMetadata );
    void addGeometryColumn( QString schema, QString table, QString column );

    // These functions get the layer types and pass that information out
    // by emitting the setLayerType() signal. The getLayerTypes()
    // function does the actual work, but use the run() function if you
    // want the work to be done as a separate thread from the calling
    // process.
    virtual void run() { getLayerTypes(); }
    void getLayerTypes();

  signals:
    void setLayerType( QString schema, QString table, QString column,
                       QString type );

  public slots:
    void stop();


  private:
    QString mConnInfo;
    bool mUseEstimatedMetadata;
    bool mStopped;
    std::vector<QString> schemas, tables, columns;
};

#endif // QGSPGSOURCESELECT_H
