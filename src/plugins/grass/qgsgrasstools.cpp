/***************************************************************************
                              qgsgrasstools.cpp
                             -------------------
    begin                : March, 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgrasstools.h"
#include "qgsgrassbrowser.h"
#include "qgsgrassmodule.h"
#include "qgsgrassshell.h"
#include "qgsgrass.h"

#include "qgisinterface.h"
#include "qgsapplication.h"
#include "qgslogger.h"

#include <QCloseEvent>
#include <QDomDocument>
#include <QFile>
#include <QHeaderView>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>


//
// For experimental model view alternative ui by Tim
//
//
#include "qgsdetaileditemdata.h"
#include "qgsdetaileditemdelegate.h"
#include <QSortFilterProxyModel>
#include <QStandardItem>


#if defined(WIN32)
#include <windows.h>
static QString getShortPath( const QString &path )
{
  TCHAR buf[MAX_PATH];
  GetShortPathName( path.toAscii(), buf, MAX_PATH );
  return buf;
}
#endif


QgsGrassTools::QgsGrassTools( QgisInterface *iface,
                              QWidget * parent, const char * name, Qt::WFlags f )
    : QDialog( parent, f ), QgsGrassToolsBase()
{

  setupUi( this );
  QgsDebugMsg( "QgsGrassTools()" );
  qRegisterMetaType<QgsDetailedItemData>();

  setWindowTitle( tr( "GRASS Tools" ) );
  //    setupUi(this);

  mIface = iface;
  mCanvas = mIface->mapCanvas();

  connect( qApp, SIGNAL( aboutToQuit() ),
           this, SLOT( closeTools() ) );

  //
  // Radims original tree view code.
  //
  mModulesTree->header()->hide();
  connect( mModulesTree, SIGNAL( itemClicked( QTreeWidgetItem *, int ) ),
           this, SLOT( moduleClicked( QTreeWidgetItem *, int ) ) );

  //
  // Tims experimental list view with filter
  //
  mModelTools = new QStandardItemModel( 0, 1 );
  mModelProxy = new QSortFilterProxyModel( this );
  mModelProxy->setSourceModel( mModelTools );
  mModelProxy->setFilterRole( Qt::UserRole + 2 );

  mListView->setModel( mModelProxy );
  mListView->setItemDelegateForColumn( 0, new QgsDetailedItemDelegate() );
  mListView->setUniformItemSizes( false );
  //mListView2 = new QListView(this);
  //mDockWidget = new QDockWidget(tr("Grass Tools"), 0);
  //mDockWidget->setWidget(mListView2);
  //mDockWidget->setObjectName("GrassTools");
  //mDockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  //mIface->addDockWidget(Qt::LeftDockWidgetArea, mDockWidget);
  connect( mListView, SIGNAL( clicked( const QModelIndex ) ),
           this, SLOT( listItemClicked( const QModelIndex ) ) );
  //
  // End of Tims experimental bit
  //

  //
  // Load the modules lists
  //
  // Show before loadConfig() so that user can see loading
  QString conf = QgsApplication::pkgDataPath() + "/grass/config/default.qgc";
  restorePosition();

  QApplication::setOverrideCursor( Qt::WaitCursor );
  loadConfig( conf );
  QApplication::restoreOverrideCursor();
  //statusBar()->hide();

  // set the dialog title
  QString title = tr( "GRASS Tools: " ) + QgsGrass::getDefaultLocation()
                  + "/" + QgsGrass::getDefaultMapset();
  setWindowTitle( title );


  // Add map browser
  mBrowser = new QgsGrassBrowser( mIface, this );
  mTabWidget->addTab( mBrowser, tr( "Browser" ) );

  connect( mBrowser, SIGNAL( regionChanged() ),
           this, SLOT( emitRegionChanged() ) );
}

void QgsGrassTools::moduleClicked( QTreeWidgetItem * item, int column )
{
  QgsDebugMsg( "entered." );
  if ( !item ) return;

  QString name = item->text( 1 );
  QgsDebugMsg( QString( "name = %1" ).arg( name ) );
  runModule( name );
}

void QgsGrassTools::runModule( QString name )
{
  if ( name.length() == 0 ) return;  // Section

#ifndef WIN32
  QgsGrassShell* sh = 0;
#endif

  QString path = QgsApplication::pkgDataPath() + "/grass/modules/" + name;
  QgsDebugMsg( QString( "path = %1" ).arg( path ) );
  QWidget *m;
  if ( name == "shell" )
  {
    // Set history file
    QString mapsetPath = QgsGrass::getDefaultGisdbase() + "/"
                         + QgsGrass::getDefaultLocation() + "/"
                         + QgsGrass::getDefaultMapset();

    // bash
    QString hist = "HISTFILE=" + mapsetPath + "/.bash_history";
    char *histChar = new char[hist.length()+1];
    strcpy( histChar, hist.toAscii().constData() );
    putenv( histChar );

    // csh/tcsh
#ifndef WIN32
    hist = "histfile=" + mapsetPath + "/.history";
    histChar = new char[hist.length()+1];
    strcpy( histChar, hist.toAscii().constData() );
    putenv( histChar );
#endif

#ifdef WIN32
    // Run MSYS if available
    // Note: I was not able to run cmd.exe and command.com
    //       with QProcess

    QString msysPath = appDir() + "/msys/bin/rxvt.exe";
    QString myArguments = "-backspacekey ^H -sl 2500 -fg white -bg black -sr -fn Courier-16 -tn msys -geometry 80x25 -e    /bin/sh --login -i";
    QFile file( msysPath );

    if ( !file.exists() )
    {
      QMessageBox::warning( 0, tr( "Warning" ),
                            tr( "Cannot find MSYS (" ) + msysPath + ")" );
    }
    else
    {
      QProcess *proc = new QProcess( this );
      //allow msys to exist in a path with spaces
      msysPath =  "\"" + msysPath + "\""  ;
      proc->start( msysPath + " " +  myArguments );
      proc->waitForStarted();
      if ( proc->state() != QProcess::Running )
      {
        QMessageBox::warning( 0, "Warning",
                              "Cannot start MSYS (" + msysPath + ")" );
      }
    }
    return;
#else

#ifdef HAVE_OPENPTY
    sh = new QgsGrassShell( this, mTabWidget );
    m = dynamic_cast<QWidget *>( sh );
#else
    QMessageBox::warning( 0, tr( "Warning" ), tr( "GRASS Shell is not compiled." ) );
#endif // HAVE_OPENPTY

#endif // ! WIN32
  }
  else
  {
    m = dynamic_cast<QWidget *>( new QgsGrassModule( this, name,
                                 mIface, path, mTabWidget ) );
  }

  int height = mTabWidget->iconSize().height();
  QPixmap pixmap = QgsGrassModule::pixmap( path, height );

  // Icon size in QT4 does not seem to be variable
  // -> put smaller icons in the middle
  QPixmap pixmap2( mTabWidget->iconSize() );
  QPalette pal;
  pixmap2.fill( pal.color( QPalette::Window ) );
  QPainter painter( &pixmap2 );
  int x = ( int )(( mTabWidget->iconSize().width() - pixmap.width() ) / 2 );
  painter.drawPixmap( x, 0, pixmap );
  painter.end();

  QIcon is;
  is.addPixmap( pixmap2 );
  mTabWidget->addTab( m, is, "" );


  mTabWidget->setCurrentIndex( mTabWidget->count() - 1 );

  // We must call resize to reset COLUMNS enviroment variable
  // used by bash !!!
#ifndef WIN32
  if ( sh ) sh->resizeTerminal();
#endif
}

bool QgsGrassTools::loadConfig( QString filePath )
{
  QgsDebugMsg( filePath );
  mModulesTree->clear();
  mModulesTree->setIconSize( QSize( 80, 22 ) );

  QFile file( filePath );

  if ( !file.exists() )
  {
    QMessageBox::warning( 0, tr( "Warning" ), tr( "The config file (" ) + filePath + tr( ") not found." ) );
    return false;
  }
  if ( ! file.open( QIODevice::ReadOnly ) )
  {
    QMessageBox::warning( 0, tr( "Warning" ), tr( "Cannot open config file (" ) + filePath + tr( ")" ) );
    return false;
  }

  QDomDocument doc( "qgisgrass" );
  QString err;
  int line, column;
  if ( !doc.setContent( &file,  &err, &line, &column ) )
  {
    QString errmsg = tr( "Cannot read config file (" ) + filePath + "):\n" + err + tr( "\nat line " )
                     + QString::number( line ) + tr( " column " ) + QString::number( column );
    QgsDebugMsg( errmsg );
    QMessageBox::warning( 0, tr( "Warning" ), errmsg );
    file.close();
    return false;
  }

  QDomElement docElem = doc.documentElement();
  QDomNodeList modulesNodes = docElem.elementsByTagName( "modules" );

  if ( modulesNodes.count() == 0 )
  {
    file.close();
    return false;
  }

  QDomNode modulesNode = modulesNodes.item( 0 );
  QDomElement modulesElem = modulesNode.toElement();

  // Go through the sections and modules and add them to the list view
  addModules( 0, modulesElem );

  file.close();
  return true;
}

void QgsGrassTools::addModules( QTreeWidgetItem *parent, QDomElement &element )
{
  QDomNode n = element.firstChild();

  QTreeWidgetItem *item;
  QTreeWidgetItem *lastItem = 0;
  while ( !n.isNull() )
  {
    QDomElement e = n.toElement();
    if ( !e.isNull() )
    {
// QgsDebugMsg(QString("tag = %1").arg(e.tagName()));

      if ( e.tagName() == "section" && e.tagName() == "grass" )
      {
        QgsDebugMsg( QString( "Unknown tag: %1" ).arg( e.tagName() ) );
        continue;
      }

      if ( parent )
      {
        item = new QTreeWidgetItem( parent, lastItem );
      }
      else
      {
        item = new QTreeWidgetItem( mModulesTree, lastItem );
      }

      if ( e.tagName() == "section" )
      {
        QString label = e.attribute( "label" );
        QgsDebugMsg( QString( "label = %1" ).arg( label ) );
        item->setText( 0, label );
        item->setExpanded( true );

        addModules( item, e );

        lastItem = item;
      }
      else if ( e.tagName() == "grass" )
      { // GRASS module
        QString name = e.attribute( "name" );
        QgsDebugMsg( QString( "name = %1" ).arg( name ) );

        QString path = QgsApplication::pkgDataPath() + "/grass/modules/" + name;
        QString label = QgsGrassModule::label( path );
        QPixmap pixmap = QgsGrassModule::pixmap( path, 25 );

        item->setText( 0, name + " - " + label );
        item->setIcon( 0, QIcon( pixmap ) );
        item->setText( 1, name );
        lastItem = item;


        //
        // Experimental work by Tim - add this item to our list model
        //
        QStandardItem * mypDetailItem = new QStandardItem( name );
        mypDetailItem->setData( name, Qt::UserRole + 1 ); //for calling runModule later
        QString mySearchText = name + " - " + label;
        mypDetailItem->setData( mySearchText, Qt::UserRole + 2 ); //for filtering later
        mypDetailItem->setData( pixmap, Qt::DecorationRole );
        mypDetailItem->setCheckable( false );
        mypDetailItem->setEditable( false );
        // setData in the delegate with a variantised QgsDetailedItemData
        QgsDetailedItemData myData;
        myData.setTitle( name );
        myData.setDetail( label );
        myData.setIcon( pixmap );
        myData.setCheckable( false );
        myData.setRenderAsWidget( true );
        QVariant myVariant = qVariantFromValue( myData );
        mypDetailItem->setData( myVariant, Qt::UserRole );
        mModelTools->appendRow( mypDetailItem );
        //
        // End of experimental work by Tim
        //
      }
    }
    n = n.nextSibling();
  }
}

void QgsGrassTools::mapsetChanged()
{
  QgsDebugMsg( "entered." );

  QString title = tr( "GRASS Tools: " ) + QgsGrass::getDefaultLocation()
                  + "/" + QgsGrass::getDefaultMapset();
  setWindowTitle( title );

  closeTools();
  mBrowser->setLocation( QgsGrass::getDefaultGisdbase(), QgsGrass::getDefaultLocation() );
}

QgsGrassTools::~QgsGrassTools()
{
  QgsDebugMsg( "entered." );
  saveWindowLocation();
}

QString QgsGrassTools::appDir( void )
{
#if defined(WIN32)
  return getShortPath( QgsApplication::applicationDirPath() );
#else
  return QgsApplication::applicationDirPath();
#endif
}

void QgsGrassTools::close( void )
{
  saveWindowLocation();
  hide();
}

void QgsGrassTools::closeEvent( QCloseEvent *e )
{
  saveWindowLocation();
  e->accept();
}

void QgsGrassTools::restorePosition()
{
  QSettings settings;
  restoreGeometry( settings.value( "/GRASS/windows/tools/geometry" ).toByteArray() );
  show();
}

void QgsGrassTools::saveWindowLocation()
{
  QSettings settings;
  settings.setValue( "/GRASS/windows/tools/geometry", saveGeometry() );
}

void QgsGrassTools::emitRegionChanged()
{
  QgsDebugMsg( "entered." );
  emit regionChanged();
}

void QgsGrassTools::closeTools()
{
  QgsDebugMsg( "entered." );

  for ( int i = mTabWidget->count() - 1; i > 1; i-- )
  {
    delete mTabWidget->widget( i );
    mTabWidget->removeTab( i );
  }
}



//
// Helper function for Tim's experimental model list
//

void QgsGrassTools::on_mFilterInput_textChanged( QString theText )
{
  QgsDebugMsg( "PluginManager filter changed to :" + theText );
  QRegExp::PatternSyntax mySyntax = QRegExp::PatternSyntax( QRegExp::RegExp );
  Qt::CaseSensitivity myCaseSensitivity = Qt::CaseInsensitive;
  QRegExp myRegExp( theText, myCaseSensitivity, mySyntax );
  mModelProxy->setFilterRegExp( myRegExp );
}

void QgsGrassTools::listItemClicked( const QModelIndex &theIndex )
{
  if ( theIndex.column() == 0 )
  {
    //
    // If the model has been filtered, the index row in the proxy wont match
    // the index row in the underlying model so we need to jump through this
    // little hoop to get the correct item
    //
    QStandardItem * mypItem =
      mModelTools->findItems( theIndex.data( Qt::DisplayRole ).toString() ).first();
    QString myModuleName = mypItem->data( Qt::UserRole + 1 ).toString();
    runModule( myModuleName );
  }
}

