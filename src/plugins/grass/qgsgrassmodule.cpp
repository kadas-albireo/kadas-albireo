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
#include <iostream>

#include <qapplication.h>
#include <qdir.h>
#include <qfile.h>
#include <qsettings.h>
#include <qpixmap.h>
#include <q3listbox.h>
#include <qstringlist.h>
#include <qlabel.h>
#include <QComboBox>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qinputdialog.h>
#include <qsettings.h>
#include <qpainter.h>
#include <q3painter.h>
#include <qpixmap.h>
#include <qpen.h>
#include <q3pointarray.h>
#include <qcursor.h>
#include <qnamespace.h>
#include <q3listview.h>
#include <qcolordialog.h>
#include <q3table.h>
#include <qstatusbar.h>
#include <qevent.h>
#include <qpoint.h>
#include <qsize.h>
#include <qdom.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <q3process.h>
#include <q3cstring.h>
#include <qlineedit.h>
#include <q3textbrowser.h>
#include <qdir.h>
#include <qregexp.h>
#include <q3progressbar.h>
#include <q3stylesheet.h>
#include <q3groupbox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <q3picture.h>
#include <qimage.h>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QPushButton>
#include <QGroupBox>

#include "qgis.h"
#include "qgsapplication.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsvectorlayer.h"
#include "qgsdataprovider.h"
#include "qgsfield.h"
#include "qgsfeature.h"
#include "qgsfeatureattribute.h"

extern "C" {
#include <grass/gis.h>
#include <grass/Vect.h>
}

#include "../../src/providers/grass/qgsgrass.h"
#include "../../src/providers/grass/qgsgrassprovider.h"
#include "qgsgrassattributes.h"
#include "qgsgrassmodule.h"
#include "qgsgrassmapcalc.h"
#include "qgsgrasstools.h"

bool QgsGrassModule::mExecPathInited = 0;
QStringList QgsGrassModule::mExecPath;

bool QgsGrassModule::inExecPath ( QString file )
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::inExecPath()" << std::endl;
    #endif
    // Init mExecPath 
    // Windows searches first in current directory
    if ( !mExecPathInited ) 
    {
        QString path = getenv("PATH");
        std::cerr << "path = " << path.ascii() << std::endl;

        mExecPath = path.split ( ";" );
        mExecPath.prepend ( QgsApplication::applicationDirPath() );
        mExecPathInited = true;
    }
 
    // Search for module
    for ( QStringList::iterator it = mExecPath.begin(); 
          it != mExecPath.end(); ++it ) 
    {
        if ( QFile::exists ( *it + "/" + file ) ) 
        {
            return true;
        }
    }
    return false;
}

QgsGrassModule::QgsGrassModule ( QgsGrassTools *tools, QgisApp *qgisApp, QgisIface *iface, 
	                     QString path, QWidget * parent, const char * name, Qt::WFlags f )
             //:QgsGrassModuleBase ( parent, name, f )
             //tim removed params during qt4 ui port - FIXME
             :QgsGrassModuleBase ( )
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule()" << std::endl;
    #endif

    setupUi(this);

    mPath = path;
    mTools = tools;
    mQgisApp = qgisApp;
    mIface = iface;
    mCanvas = mIface->getMapCanvas();
    mParent = parent;
    //mAppDir = QgsApplication::applicationDirPath();
    mAppDir = mTools->appDir();

    /* Read module description and create options */

    // Open QGIS module description
    QString mpath = mPath + ".qgm";
    QFile qFile ( mpath );
    if ( !qFile.exists() ) {
	QMessageBox::warning( 0, "Warning", "The module file (" + mpath + ") not found." );
	return;
    }
    if ( ! qFile.open( QIODevice::ReadOnly ) ) {
	QMessageBox::warning( 0, "Warning", "Cannot open module file (" + mpath + ")" );
	return;
    }
    QDomDocument qDoc ( "qgisgrassmodule" );
    QString err;
    int line, column;
    if ( !qDoc.setContent( &qFile,  &err, &line, &column ) ) {
	QString errmsg = "Cannot read module file (" + mpath + "):\n" + err + "\nat line "
	                 + QString::number(line) + " column " + QString::number(column);
	std::cerr << errmsg.toLocal8Bit().data() << std::endl;
	QMessageBox::warning( 0, "Warning", errmsg );
	qFile.close();
	return;
    }
    qFile.close();
    QDomElement qDocElem = qDoc.documentElement();

    // Read GRASS module description
    QString xName = qDocElem.attribute("module");

    // Binary modules on windows has .exe extension
    // but not all modules have to be binary (can be scripts)
    // => test if the module is in path and if it is not 
    // add .exe and test again
#ifdef WIN32
    if ( inExecPath ( xName ) ) 
    {
        mXName = xName;
    }
    else if ( inExecPath ( xName + ".exe" ) )
    {
        mXName = xName + ".exe";
    }
    else 
    {
        std::cerr << "Module " << xName.ascii() << " not found" << std::endl;
	QMessageBox::warning( 0, "Warning", "Module " + xName + " not found" );
        return;
    }
#else
    mXName = xName;
#endif
    
    if ( xName == "r.mapcalc" )
    {
        QGridLayout *layout = new QGridLayout ( mTabWidget->page(0), 1, 1 );

        mOptions = new QgsGrassMapcalc ( mTools, this,
               mQgisApp, mIface, mTabWidget->page(0) );

        QWidget *w = dynamic_cast<QWidget *>(mOptions);
        			
        std::cerr << "w = " << w << std::endl;

        layout->addWidget ( dynamic_cast<QWidget *>(mOptions), 0, 0 );
    }
    else
    {
        mOptions = new QgsGrassModuleStandardOptions ( mTools, this,
               mQgisApp, mIface, mXName, qDocElem, mTabWidget->page(0) );
    }
                                
    // Create manual if available
    QString gisBase = getenv("GISBASE");
    QString manPath = gisBase + "/docs/html/" + xName + ".html";
    QFile manFile ( manPath );
    if ( manFile.exists() ) {
	mManualTextBrowser->setSource ( manPath );
    }
    
    connect ( &mProcess, SIGNAL(readyReadStdout()), this, SLOT(readStdout()));
    connect ( &mProcess, SIGNAL(readyReadStderr()), this, SLOT(readStderr()));
    connect ( &mProcess, SIGNAL(launchFinished()), this, SLOT(finished()));
    connect ( &mProcess, SIGNAL(processExited()), this, SLOT(finished()));

    char *env = "GRASS_MESSAGE_FORMAT=gui";
    char *envstr = new char[strlen(env)+1];
    strcpy ( envstr, env );
    putenv( envstr );

    mOutputTextBrowser->setTextFormat(Qt::RichText);
    mOutputTextBrowser->setReadOnly(TRUE);
}

/******************* QgsGrassModuleOptions *******************/

QgsGrassModuleOptions::QgsGrassModuleOptions ( 
           QgsGrassTools *tools, QgsGrassModule *module,
           QgisApp *qgisApp, QgisIface *iface )
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleOptions()" << std::endl;
    #endif

    mTools = tools;
    mModule = module;
    mQgisApp = qgisApp;
    mIface = iface;
    mCanvas = mIface->getMapCanvas();
    //mAppDir = QgsApplication::applicationDirPath();
    mAppDir = mTools->appDir();
}

QgsGrassModuleOptions::~QgsGrassModuleOptions()
{
}

QStringList QgsGrassModuleOptions::arguments()
{
    return QStringList();
}

/*************** QgsGrassModuleStandardOptions ***********************/

QgsGrassModuleStandardOptions::QgsGrassModuleStandardOptions ( 
           QgsGrassTools *tools, QgsGrassModule *module,
           QgisApp *qgisApp, QgisIface *iface, 
	   QString xname, QDomElement qDocElem,
           QWidget * parent, const char * name, Qt::WFlags f )
             :QgsGrassModuleOptions( tools, module, qgisApp, iface),
              QWidget ( parent, name, f )
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleStandardOptions()" << std::endl;
    std::cerr << "PATH = " <<  getenv ("PATH") << std::endl;
    #endif

    mXName = xname;
    mParent = parent;

    Q3Process *process = new Q3Process( this );
    process->addArgument( mXName );
    process->addArgument( "--interface-description" );

    // Attention: if a binary has the .exe extention it must be 
    // also in mXName but we cannot append it here because 
    // the modules can also be a script => ???
    // For now create in GISBASE/bin copy of each module without 
    // .exe extension
    if ( !process->start( ) ) {
	QMessageBox::warning( 0, "Warning", "Cannot start module " + mXName );
	return;
    }
    while ( process->isRunning () ) { // TODO: check time, if it is not running too long
    }
    QByteArray gDescArray = process->readStdout();
    QByteArray errArray = process->readStderr();
    delete process;

    QDomDocument gDoc ( "task" );
    QString err;
    int line, column;
    if ( !gDoc.setContent( (QByteArray)gDescArray, &err, &line, &column ) ) {
	QString errmsg = "Cannot read module description (" + mXName + "):\n" + err + "\nat line "
	                 + QString::number(line) + " column " + QString::number(column);
	std::cerr << errmsg.toLocal8Bit().data() << std::endl;
	std::cerr << QString(gDescArray).local8Bit().data() << std::endl;
	std::cerr << QString(errArray).local8Bit().data() << std::endl;
	QMessageBox::warning( 0, "Warning", errmsg );
	return;
    }
    QDomElement gDocElem = gDoc.documentElement();

    // Read QGIS options and create controls
    QDomNode n = qDocElem.firstChild();
    //QVBoxLayout *layout = new QVBoxLayout ( mTabWidget->page(0), 10 );
    QVBoxLayout *layout = new QVBoxLayout ( mParent, 10 );
    while( !n.isNull() ) {
	QDomElement e = n.toElement();
	if( !e.isNull() ) {
	    QString optionType = e.tagName();
	    //std::cout << "optionType = " << optionType << std::endl;

	    QString key = e.attribute("key");
	    std::cout << "key = " << key.toLocal8Bit().data() << std::endl;

	    QDomNode gnode = QgsGrassModule::nodeByKey ( gDocElem, key );
	    if ( gnode.isNull() ) {
		QMessageBox::warning( 0, "Warning", "Cannot find key " +  key );
		return;
	    }

	    if ( optionType == "option" ) {
	        bool created = false;

		// Check option type and create appropriate control
		QDomNode promptNode = gnode.namedItem ( "gisprompt" );
		if ( !promptNode.isNull() ) {
		    QDomElement promptElem = promptNode.toElement();
		    QString element = promptElem.attribute("element"); 
		    QString age = promptElem.attribute("age"); 
		    //std::cout << "element = " << element << " age = " << age << std::endl;
		    if ( age == "old" && ( element == "vector" || element == "cell") ) {
			QgsGrassModuleInput *mi = new QgsGrassModuleInput ( 
                               mModule, this, key, e, gDocElem, gnode, mParent );

			layout->addWidget ( mi );
			created = true;
			mItems.push_back(mi);
		    }
		} 

		if ( !created ) {
		    QgsGrassModuleOption *so = new QgsGrassModuleOption ( 
                              mModule, key, e, gDocElem, gnode, mParent );
		    
			layout->addWidget ( so );
			created = true;
		    mItems.push_back(so);
		}
	    } 
	    else if ( optionType == "ogr" ) 
	    {
		QgsGrassModuleGdalInput *mi = new QgsGrassModuleGdalInput ( 
		       mModule, QgsGrassModuleGdalInput::Ogr, key, e, 
		       gDocElem, gnode, mParent );
		layout->addWidget ( mi );
		mItems.push_back(mi);
	    } 
	    else if ( optionType == "gdal" ) 
	    {
		QgsGrassModuleGdalInput *mi = new QgsGrassModuleGdalInput ( 
		       mModule, QgsGrassModuleGdalInput::Gdal, key, e, 
		       gDocElem, gnode, mParent );
		layout->addWidget ( mi );
		mItems.push_back(mi);
	    } 
	    else if ( optionType == "field" ) 
	    {
		QgsGrassModuleField *mi = new QgsGrassModuleField ( 
		       mModule, this, key, e, 
		       gDocElem, gnode, mParent );
		layout->addWidget ( mi );
		mItems.push_back(mi);
	    } 
	    else if ( optionType == "selection" ) 
	    {
		QgsGrassModuleSelection *mi = new QgsGrassModuleSelection ( 
		       mModule, this, key, e, 
		       gDocElem, gnode, mParent );
		layout->addWidget ( mi );
		mItems.push_back(mi);
	    } 
	    else if ( optionType == "flag" )  
	    {
		    QgsGrassModuleFlag *flag = new QgsGrassModuleFlag ( 
                           mModule, key, e, gDocElem, gnode, mParent );
		    
		    layout->addWidget ( flag );
		    mItems.push_back(flag);
	    }
	}
	n = n.nextSibling();
    }

    layout->addStretch();
}

QStringList QgsGrassModuleStandardOptions::arguments()
{
    QStringList arg;

    for ( int i = 0; i < mItems.size(); i++ ) 
    {
	QStringList list = mItems[i]->options();
        
	for ( QStringList::Iterator it = list.begin(); 
              it != list.end(); ++it ) 
        {
	    arg.append ( *it );
        }
    } 
    return arg;
}

QgsGrassModuleItem *QgsGrassModuleStandardOptions::item ( QString id )
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleStandardOptions::item() id = " 
	      << id.toLocal8Bit().data() << std::endl;
    #endif
        
    for ( int i = 0; i < mItems.size(); i++ )
    {
	if ( mItems[i]->id() == id ) 
	{
	    return mItems[i];
	}
    }

    QMessageBox::warning( 0, "Warning", "Item with id " + id + " not found" );
    return 0;
}

QStringList QgsGrassModuleStandardOptions::checkOutput()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleStandardOptions::checkOutput()" << std::endl;
    #endif
    QStringList list;

    for ( int i = 0; i < mItems.size(); i++ ) 
    {
	if ( typeid(*(mItems[i])) != typeid (QgsGrassModuleOption) ) {
	    continue;
	}
	QgsGrassModuleOption *opt = 
	      dynamic_cast<QgsGrassModuleOption *> ( mItems[i] );
    
        std::cerr << "opt->key() = " << opt->key().ascii() << std::endl;

	if ( opt->isOutput() )
	{
            QString out = opt->outputExists();
            if ( !out.isNull() ) 
            {
	        list.append ( out );
            }
	}
    } 

    return list;
}

QStringList QgsGrassModuleStandardOptions::ready()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleStandardOptions::ready()" << std::endl;
    #endif
    QStringList list;

    for ( int i = 0; i < mItems.size(); i++ ) 
    {
	QString err = mItems[i]->ready();
	if ( !err.isNull() ) 
	{
	    list.append ( err );
	}
    } 

    return list;
}

QgsGrassModuleStandardOptions::~QgsGrassModuleStandardOptions()
{
}

QString QgsGrassModule::label ( QString path ) 
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::label()" << std::endl;
    #endif

    // Open QGIS module description
    path.append ( ".qgm" );
    QFile qFile ( path );
    if ( !qFile.exists() ) {
	return QString ( "Not available, decription not found (" + path + ")" );
    }
    if ( ! qFile.open( QIODevice::ReadOnly ) ) {
	return QString ( "Not available, cannot open description (" + path + ")" ) ;
    }
    QDomDocument qDoc ( "qgisgrassmodule" );
    QString err;
    int line, column;
    if ( !qDoc.setContent( &qFile,  &err, &line, &column ) ) {
	QString errmsg = "Cannot read module file (" + path + "):\n" + err + "\nat line "
	                 + QString::number(line) + " column " + QString::number(column);
	std::cerr << errmsg.toLocal8Bit().data() << std::endl;
	QMessageBox::warning( 0, "Warning", errmsg );
	qFile.close();
	return QString ( "Not available, incorrect description (" + path + ")" );
    }
    qFile.close();
    QDomElement qDocElem = qDoc.documentElement();

    return ( qDocElem.attribute("label") );
}

QPixmap QgsGrassModule::pixmap ( QString path, int height ) 
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::pixmap()" << std::endl;
    #endif

    std::vector<QPixmap> pixmaps;

    // Create vector of available pictures
    int cnt = 1;
    while ( 1 ) 
    {
	// SVG
	QString fpath = path + "." + QString::number(cnt) + ".svg";
        QFileInfo fi ( fpath );
        if ( fi.exists() ) 
	{
	    Q3Picture pic;
	    if ( ! pic.load ( fpath, "svg" ) ) break;

	    QRect br = pic.boundingRect();

	    double scale = 1. * height / br.height();

	    int width = (int) ( scale * br.width() );
            if ( width <= 0 ) width = height; //should not happen
	    QPixmap pixmap ( width, height );
	    pixmap.fill ( QColor(255,255,255) );
	    QPainter painter ( &pixmap );
            painter.setRenderHint(QPainter::Antialiasing);

            // I am not sure if this factor is OK
            scale *= 72.0 / pixmap.logicalDpiX() ;
	    painter.scale ( scale, scale );
            
	    painter.drawPicture ( -br.x(), -br.y(), pic );
	    painter.end();
	    
	    pixmaps.push_back ( pixmap );
	} 
	else // PNG 
	{
	    fpath = path + "." + QString::number(cnt) + ".png";
	    fi.setFile ( fpath );

	    if ( !fi.exists() ) break;

	    QPixmap pixmap;
	   
	    if ( ! pixmap.load(fpath, "PNG" ) ) break;
	    
	    double scale = 1. * height / pixmap.height();
	    int width = (int) ( scale * pixmap.width() );

	    QImage img = pixmap.convertToImage();
	    img = img.smoothScale ( width, height ); 
	    pixmap.convertFromImage ( img );

	    pixmaps.push_back ( pixmap );
	}
        cnt++;
    }	

    // Get total width
    int width = 0;
    for ( int i = 0; i < pixmaps.size(); i++ ) {
	width += pixmaps[i].width();
    }

    if ( width <= 0 ) width = height; //should not happen
    
    int plusWidth = 8; // +
    int arrowWidth = 9; // ->
    int buffer = 10; // buffer around a sign
    if ( pixmaps.size() > 1 ) width += arrowWidth + 2 * buffer; // ->
    if ( pixmaps.size() > 2 ) width += plusWidth + 2 * buffer; // +

    QPixmap pixmap ( width, height );
    pixmap.fill(QColor(255,255,255));
    QPainter painter ( &pixmap );

    QColor color(200,200,200);
    painter.setBrush(QBrush(color));

    painter.setRenderHint(QPainter::Antialiasing);
    
    int pos = 0;
    for ( int i = 0; i < pixmaps.size(); i++ ) {
	if ( i == 1 && pixmaps.size() == 3 ) { // +
	    pos += buffer;
    
            painter.setPen( QPen(color,3) );
	    painter.drawLine ( pos, height/2, pos+plusWidth, height/2 );
	    painter.drawLine ( pos+plusWidth/2, height/2-plusWidth/2, pos+plusWidth/2, height/2+plusWidth/2 );
	    pos += buffer + plusWidth;
	}
	if ( (i == 1 && pixmaps.size() == 2) || (i == 2 && pixmaps.size() == 3)  ) { // ->
	    pos += buffer;
            painter.setPen( QPen(color,3) );
	    painter.drawLine ( pos, height/2, pos+arrowWidth-arrowWidth/2, height/2 );

	    Q3PointArray pa(3);
	    pa.setPoint(0, pos+arrowWidth/2+1, height/2-arrowWidth/2);
	    pa.setPoint(1, pos+arrowWidth, height/2 );
	    pa.setPoint(2, pos+arrowWidth/2+1, height/2+arrowWidth/2);
            painter.setPen( QPen(color,1) );
	    painter.drawPolygon ( pa );
	    
	    pos += buffer + arrowWidth;
	}
	painter.drawPixmap ( pos,0, pixmaps[i] );
	pos += pixmaps[i].width();
    }
    painter.end();

    return pixmap;
}

void QgsGrassModule::run()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::run()" << std::endl;
    #endif

    if ( mProcess.isRunning() ) {
	mProcess.kill();
	mRunButton->setText( tr("Run") );
    } else { 
	QString command;
	
	if ( mProcess.isRunning() ) {
	}

	mProcess.clearArguments();
	mProcess.addArgument( mXName );
	command = mXName;

        // Check if options are ready
        QStringList readyErrors = mOptions->ready();
        if ( readyErrors.size() > 0 )
        {
            QString err;
            for ( int i = 0; i < readyErrors.size(); i++ ) 
            {
                err.append ( readyErrors.at(i) + "<br>" );
            }
            QMessageBox::warning ( 0, "Warning", err );
            return;
        }

        // Check if output exists
        QStringList outputExists = mOptions->checkOutput();
        if ( outputExists.size() > 0 )
        {
            int ret = QMessageBox::question ( 0, "Warning", 
                          "Output " + outputExists.join(",")
                          + " exists! Overwrite?",  
                          QMessageBox::Yes,  QMessageBox::No );

            if ( ret == QMessageBox::No ) return;

            // r.mapcalc does not use standard parser
            if ( typeid(*mOptions) != typeid(QgsGrassMapcalc) )
            {
	        mProcess.addArgument( "--o" );
	        command.append ( " --o" );
            }
        }
  
        QStringList list = mOptions->arguments();

	for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
	    std::cerr << "option: " << (*it).toLocal8Bit().data() << std::endl;
	    command.append ( " " + *it );
	    mProcess.addArgument( *it );
	}

	/* WARNING - TODO: there was a bug in GRASS 6.0.0 / 6.1.CVS (< 2005-04-29):
	 * db_start_driver set GISRC_MODE_MEMORY eviroment variable to 1 if 
	 * G_get_gisrc_mode() == G_GISRC_MODE_MEMORY but the variable wasn't unset 
	 * if  G_get_gisrc_mode() == G_GISRC_MODE_FILE. Because QGIS GRASS provider starts drivers in 
	 * G_GISRC_MODE_MEMORY mode, the variable remains set in variable when a module is run
	 * -> unset GISRC_MODE_MEMORY. Remove later once 6.1.x / 6.0.1 is widespread.
	 */
	putenv ( "GISRC_MODE_MEMORY" );  // unset
	
	mProcess.start( );
	
	std::cerr << "command" << command.toLocal8Bit().data() << std::endl;
	mOutputTextBrowser->clear();

	command.replace ( "&", "&amp;" );
	command.replace ( "<", "&lt;" );
	command.replace ( ">", "&gt;" );
	mOutputTextBrowser->append( "<B>" +  command + "</B>" );
	mTabWidget->setCurrentPage(1);
	mRunButton->setText( tr("Stop") );
    }
}

void QgsGrassModule::finished()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::finished()" << std::endl;
    #endif

    if ( mProcess.normalExit() ) {
	if ( mProcess.exitStatus () == 0 ) {
	    mOutputTextBrowser->append( "<B>Successfully finished</B>" );
	    mProgressBar->setProgress ( 100, 100 ); 
	} else {
	    mOutputTextBrowser->append( "<B>Finished with error</B>" );
	}
    } else {
	mOutputTextBrowser->append( "<B>Module crashed or killed</B>" );
    }
    mRunButton->setText( tr("Run") );
}

void QgsGrassModule::readStdout()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::readStdout()" << std::endl;
    #endif

    QString line;
    while ( mProcess.canReadLineStdout() ) {
       	line = QString::fromLocal8Bit( mProcess.readLineStdout().ascii() );
	mOutputTextBrowser->append ( line );
    }
}

void QgsGrassModule::readStderr()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::readStderr()" << std::endl;
    #endif

    QString line;
    QRegExp rxpercent ( "GRASS_INFO_PERCENT: (\\d+)" );
    QRegExp rxmessage ( "GRASS_INFO_MESSAGE\\(\\d+,\\d+\\): (.*)" );
    QRegExp rxwarning ( "GRASS_INFO_WARNING\\(\\d+,\\d+\\): (.*)" );
    QRegExp rxerror ( "GRASS_INFO_ERROR\\(\\d+,\\d+\\): (.*)" );
    QRegExp rxend ( "GRASS_INFO_END\\(\\d+,\\d+\\)" );
        

    while ( mProcess.canReadLineStderr() ) {
       	line = QString::fromLocal8Bit( mProcess.readLineStderr().ascii() );
        //std::cerr << "stderr: " << line << std::endl;

	if ( rxpercent.search ( line ) != -1 ) {
	    int progress = rxpercent.cap(1).toInt();
	    mProgressBar->setProgress ( progress, 100 );
	} else if ( rxmessage.search ( line ) != -1 ) {
	    mOutputTextBrowser->append ( rxmessage.cap(1) );
	} else if ( rxwarning.search ( line ) != -1 ) {
	    QString warn = rxwarning.cap(1);
	    QString img = mAppDir + "/share/qgis/themes/default/grass/grass_module_warning.png";
	    mOutputTextBrowser->append ( "<img src=\"" + img + "\">" + warn );
	} else if ( rxerror.search ( line ) != -1 ) {
	    QString error = rxerror.cap(1);
	    QString img = mAppDir + "/share/qgis/themes/default/grass/grass_module_error.png";
	    mOutputTextBrowser->append ( "<img src=\"" + img + "\">" + error );
	} else if ( rxend.search ( line ) != -1 ) {
	    // Do nothing
	} else {
	    mOutputTextBrowser->append ( line + "\n" );
	}
    }
}

void QgsGrassModule::close()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::close()" << std::endl;
    #endif

    QTabWidget *tw = dynamic_cast<QTabWidget *>(mParent);
    tw->removePage (this );
    delete this;
}

QgisIface *QgsGrassModule::qgisIface() { return mIface; }
QgisApp *QgsGrassModule::qgisApp() { return mQgisApp; }

QgsGrassModule::~QgsGrassModule()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::~QgsGrassModule()" << std::endl;
    #endif
}

QDomNode QgsGrassModule::nodeByKey ( QDomElement elem, QString key )
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModule::nodeByKey() key = " << key.toLocal8Bit().data() << std::endl;
    #endif
    QDomNode n = elem.firstChild();

    while( !n.isNull() ) {
	QDomElement e = n.toElement();

	if( !e.isNull() ) {
	    if ( e.tagName() == "parameter" || e.tagName() == "flag" ) {
		if ( e.attribute("name") == key ) {
		    return n;
		}
	    }
	}
	n = n.nextSibling();
     }

     return QDomNode();
}

/******************* QgsGrassModuleOption ****************************/

QgsGrassModuleOption::QgsGrassModuleOption ( QgsGrassModule *module, QString key, 
	                                   QDomElement &qdesc, QDomElement &gdesc, QDomNode &gnode,
	                                   QWidget * parent)
                    : QGroupBox ( parent ),
                      QgsGrassModuleItem ( module, key, qdesc, gdesc, gnode ),
	mIsOutput(false), mValueType(String), mHaveLimits(false)
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleOption::QgsGrassModuleOption" << std::endl;
    #endif
    setSizePolicy ( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum );
    
    if ( mHidden ) hide();
    
    mLayout = new QVBoxLayout ();

    QString tit;
    if ( mDescription.length() > 40 ) {
	tit = mDescription.left(40) + " ...";
    } else {
	tit = mDescription;
    }
	    
    setTitle ( " " + tit + " " );

    // Is it output?
    QDomNode promptNode = gnode.namedItem ( "gisprompt" );
    if ( !promptNode.isNull() ) {
	QDomElement promptElem = promptNode.toElement();
	QString element = promptElem.attribute("element"); 
	QString age = promptElem.attribute("age"); 
	if ( age == "new" )
        {
            mOutputElement = element;
            mIsOutput = true;
	}
    } 
	
    // String without options 
    if ( !mHidden ) 
    {
        QDomElement gelem = gnode.toElement();

	// Predefined values ?
	QDomNode valuesNode = gnode.namedItem ( "values" );
	QDomElement valuesElem = valuesNode.toElement(); // null if valuesNode is null
	
	if ( !valuesNode.isNull() && valuesNode.childNodes().count() > 1 ) 
	{
            setLayout(mLayout);
            // predefined values -> ComboBox or CheckBox

	    // one or many?
   	    if ( gelem.attribute("multiple") == "yes" ) {
		mControlType = CheckBoxes;
	    } else {
	        mControlType = ComboBox;
		mComboBox = new QComboBox ( this );
                mLayout->addWidget ( mComboBox );
	    }

	    // List of values to be excluded 
	    QStringList exclude = QStringList::split ( ',', qdesc.attribute("exclude") );

	    QDomNode valueNode = valuesElem.firstChild();

	    while( !valueNode.isNull() ) {
		QDomElement valueElem = valueNode.toElement();

		if( !valueElem.isNull() && valueElem.tagName() == "value" ) 
		{

		    QDomNode n = valueNode.namedItem ( "name" );
		    if ( !n.isNull() ) {
			QDomElement e = n.toElement();
			QString val = e.text().stripWhiteSpace();
			
			if ( exclude.contains(val) == 0 ) { 
			    n = valueNode.namedItem ( "description" );
			    QString desc;
			    if ( !n.isNull() ) {
				e = n.toElement();
				desc = e.text().stripWhiteSpace();
			    } else {
				desc = val;
			    }
			    desc.replace( 0, 1, desc.left(1).upper() );

			    if ( mControlType == ComboBox ) {
				mComboBox->insertItem ( desc );
			    } else {
				QCheckBox *cb = new QCheckBox ( desc, this );
				mCheckBoxes.push_back ( cb );
                                mLayout->addWidget ( cb );
			    }
			    
			    mValues.push_back ( val );
			}
		    }
		}
		
		valueNode = valueNode.nextSibling();
	     }
	} 
	else // No values
	{
	    // Line edit
	    mControlType = LineEdit;

	    QDomNode n = gnode.namedItem ( "default" );
	    if ( !n.isNull() ) {
		QDomElement e = n.toElement();
		mDefault = e.text().stripWhiteSpace();
	    }

   	    if ( gelem.attribute("type") == "integer" ) 
            {
		mValueType = Integer;
	    } else if ( gelem.attribute("type") == "float" ) {
		mValueType = Double;
	    }

            QStringList minMax;
	    if ( valuesNode.childNodes().count() == 1 )
            { 
	        QDomNode valueNode = valuesElem.firstChild();

		QDomNode n = valueNode.namedItem ( "name" );
		if ( !n.isNull() ) {
		    QDomElement e = n.toElement();
		    QString val = e.text().stripWhiteSpace();
                    minMax = val.split("-");
                    if ( minMax.size() == 2 ) 
                    {
                        mHaveLimits = true;
                        mMin = minMax.at(0).toDouble();
                        mMax = minMax.at(1).toDouble();
                    }
                }
            }

            addLineEdit();

            // add/delete buttons for multiple options
   	    if ( gelem.attribute("multiple") == "yes" ) 
            {
                QHBoxLayout *l = new QHBoxLayout (this);
                QVBoxLayout *vl = new QVBoxLayout ();
                l->insertLayout( -1, mLayout );
                l->insertLayout( -1, vl );

                // TODO: how to keep both buttons on the top?
                QPushButton *b = new QPushButton ( "+", this);
                connect( b, SIGNAL(clicked()), this, SLOT(addLineEdit()) );
                vl->addWidget ( b, 0, Qt::AlignTop );

                b = new QPushButton ( "-", this);
                connect( b, SIGNAL(clicked()), this, SLOT(removeLineEdit()) );
                vl->addWidget ( b, 0, Qt::AlignTop );
        
                // Dont enable this, it makes the group box expanding
                // vl->addStretch();
            }
            else 
            {
                setLayout ( mLayout );
            }  
	}
    }
}

void QgsGrassModuleOption::addLineEdit()
{
    std::cerr << "QgsGrassModuleOption::addLineEdit()" << std::endl;

    // TODO make the widget growing with new lines. HOW???!!!
    QLineEdit *lineEdit = new QLineEdit ( this );
    mLineEdits.push_back (lineEdit );
    lineEdit->setText ( mDefault );

    if ( mValueType == Integer ) 
    {
	if ( mHaveLimits ) {
	    mValidator = new QIntValidator( (int)mMin, (int)mMax, this );
	} else {
	    mValidator = new QIntValidator( this );
	}
	lineEdit->setValidator ( mValidator );
    } else if ( mValueType == Double ) {
	if ( mHaveLimits ) {
	    mValidator = new QDoubleValidator( mMin, mMax, 10, this );
	} else {
	    mValidator = new QDoubleValidator( this );
	}
	lineEdit->setValidator ( mValidator );
    }

    mLayout->addWidget ( lineEdit );
}

void QgsGrassModuleOption::removeLineEdit()
{
    std::cerr << "QgsGrassModuleOption::removeLineEdit()" << std::endl;
    if ( mLineEdits.size() < 2 ) return;
    delete mLineEdits.at(mLineEdits.size()-1);
    mLineEdits.pop_back(); 
}

QString QgsGrassModuleOption::outputExists()
{
    std::cerr << "QgsGrassModuleOption::outputExists()" << std::endl;

    if ( !mIsOutput ) return QString();

    QLineEdit *lineEdit = mLineEdits.at(0);
    QString value = lineEdit->text().trimmed();
    std::cerr << "mKey = " << mKey.ascii() << std::endl;
    std::cerr << "value = " << value.ascii() << std::endl;
    std::cerr << "mOutputElement = " << mOutputElement.ascii() << std::endl;

    if ( value.length() == 0 ) return QString();

    QString path = QgsGrass::getDefaultGisdbase() + "/"
                 + QgsGrass::getDefaultLocation() + "/"
                 + QgsGrass::getDefaultMapset() + "/" 
                 + mOutputElement + "/" + value;

    QFileInfo fi(path);

    if ( fi.exists() )
    {
        return (lineEdit->text());
    }

    return QString();
}

QStringList QgsGrassModuleOption::options()
{
    QStringList list;
    
    if ( mHidden ) {
        list.push_back( mKey + "=" + mAnswer );
    } else {
	if ( mControlType == LineEdit ) {
            QString vals; 
            for ( int i = 0; i < mLineEdits.size(); i++ ) 
            {
                QLineEdit *lineEdit = mLineEdits.at(i);
                if( lineEdit->text().trimmed().length() > 0 )
                {
                    if ( vals.length() > 0 ) vals.append (",");
                    vals.append ( lineEdit->text().trimmed() );
                }
            }
            list.push_back( mKey + "=" + vals );
	} else if ( mControlType == ComboBox ) {
            list.push_back( mKey + "=" + mValues[mComboBox->currentItem()] );
	} else if ( mControlType == CheckBoxes ) {
	    QString opt = mKey + "=";
	    int cnt = 0;
	    for ( int i = 0; i < mCheckBoxes.size(); i++ ) {
		if ( mCheckBoxes[i]->isChecked() ) {
		    if ( cnt > 0 ) opt.append ( "," );
		    opt.append ( mValues[i] );
		}
	    }
	    list.push_back( opt );
	}
    }
    return list;
}

QString QgsGrassModuleOption::ready()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleOption::ready()" << std::endl;
    #endif

    QString error;

    if ( mControlType == LineEdit )
    { 
       if ( mLineEdits.at(0)->text().trimmed().length() == 0 )
       {
           error.append ( title() + ":&nbsp;missing value" );
       }
    }
    return error;
}

QgsGrassModuleOption::~QgsGrassModuleOption()
{
}

QgsGrassModuleFlag::QgsGrassModuleFlag ( QgsGrassModule *module, QString key,
	                                   QDomElement &qdesc, QDomElement &gdesc, QDomNode &gnode,
	                                   QWidget * parent)
                    :QCheckBox ( parent ), QgsGrassModuleItem ( module, key, qdesc, gdesc, gnode )
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleFlag::QgsGrassModuleFlag" << std::endl;
    #endif

    if ( mHidden ) hide();
    
    if ( mAnswer == "on" )
        setChecked ( true );
    else 
        setChecked ( false );
    
    setText ( mDescription );
}

QStringList QgsGrassModuleFlag::options()
{
    QStringList list;
    if (  isChecked() ) {
        list.push_back( "-" + mKey );
    }
    return list;
}

QgsGrassModuleFlag::~QgsGrassModuleFlag()
{
}

/************************** QgsGrassModuleInput ***************************/

QgsGrassModuleInput::QgsGrassModuleInput ( QgsGrassModule *module,
       					   QgsGrassModuleStandardOptions *options, QString key,
	                                   QDomElement &qdesc, QDomElement &gdesc, QDomNode &gnode,
	                                   QWidget * parent)
                    : Q3GroupBox ( 1, Qt::Vertical, parent ),
                      QgsGrassModuleItem ( module, key, qdesc, gdesc, gnode ),
		      mModuleStandardOptions(options),
		      mUpdate(false), mVectorTypeOption(0), mVectorLayerOption(0)
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleInput::QgsGrassModuleInput" << std::endl;
    #endif
    mVectorTypeMask = GV_POINT | GV_LINE | GV_AREA;

    QString tit;
    if ( mDescription.isEmpty() ) {
	tit = "Input";
    } else {
	if ( mDescription.length() > 40 ) {
	    tit = mDescription.left(40) + " ...";
	} else {
	    tit = mDescription;
	}
    }
	    
    setTitle ( " " + tit + " " );

    QDomNode promptNode = gnode.namedItem ( "gisprompt" );
    QDomElement promptElem = promptNode.toElement();
    QString element = promptElem.attribute("element"); 

    if ( element == "vector" ) 
    {
        mType = Vector;	

	// Read type mask if "typeoption" is defined
	QString opt = qdesc.attribute("typeoption");
	if ( ! opt.isNull() ) {
	
	    QDomNode optNode = QgsGrassModule::nodeByKey ( gdesc, opt );

	    if ( optNode.isNull() ) 
	    {
		QMessageBox::warning( 0, "Warning", "Cannot find typeoption " +  opt );
	    } 
	    else 
	    {
		mVectorTypeOption = opt;

		QDomNode valuesNode = optNode.namedItem ( "values" );
		if ( valuesNode.isNull() ) 
		{
		    QMessageBox::warning( 0, "Warning", "Cannot find values for typeoption " +  opt );
		}
		else
		{
		    mVectorTypeMask = 0; GV_POINT | GV_LINE | GV_AREA;
		    
		    QDomElement valuesElem = valuesNode.toElement();
		    QDomNode valueNode = valuesElem.firstChild();
	
		    while( !valueNode.isNull() ) {
			QDomElement valueElem = valueNode.toElement();

			if( !valueElem.isNull() && valueElem.tagName() == "value" ) 
			{
			    QDomNode n = valueNode.namedItem ( "name" );
			    if ( !n.isNull() ) {
				QDomElement e = n.toElement();
				QString val = e.text().stripWhiteSpace();
		
				if ( val == "point" ) {
				    mVectorTypeMask |= GV_POINT;
				} else if ( val == "line" ) {
				    mVectorTypeMask |= GV_LINE;
				} else if ( val == "area" ) {
				    mVectorTypeMask |= GV_AREA;
				}
			    }
			}
			
			valueNode = valueNode.nextSibling();
		     }
		}
	    }
	}

	// Read type mask defined in configuration
	opt = qdesc.attribute("typemask");
	if ( ! opt.isNull() ) {
	    int mask = 0;
	    
	    if ( opt.find("point") >= 0 ) {
		mask |= GV_POINT;
	    }
	    if ( opt.find("line") >= 0 ) {
		mask |= GV_LINE;
	    }
	    if ( opt.find("area") >= 0 ) {
		mask |= GV_AREA;
	    }
	
            mVectorTypeMask &= mask;
	}

	// Read "layeroption" if defined
	opt = qdesc.attribute("layeroption");
	if ( ! opt.isNull() ) {
	
	    QDomNode optNode = QgsGrassModule::nodeByKey ( gdesc, opt );

	    if ( optNode.isNull() ) 
	    {
		QMessageBox::warning( 0, "Warning", "Cannot find layeroption " +  opt );
	    } 
	    else 
	    {
		mVectorLayerOption = opt;
	    }
	}

	// Read "mapid"
	mMapId = qdesc.attribute("mapid");
    } 
    else if ( element == "cell" ) 
    {
       mType = Raster;
    } 
    else 
    {
	QMessageBox::warning( 0, "Warning", "GRASS element " + element + " not supported" );
    }

    if ( qdesc.attribute("update") == "yes" ) {
	mUpdate = true;
    }

    mLayerComboBox = new QComboBox ( this );

    // Of course, activated(int) is not enough, but there is no signal BEFORE the cobo is opened
    //connect ( mLayerComboBox, SIGNAL( activated(int) ), this, SLOT(updateQgisLayers()) );
    
    // Connect to canvas 
    QgsMapCanvas *canvas = mModule->qgisIface()->getMapCanvas();
    connect ( canvas, SIGNAL(layersChanged()), this, SLOT(updateQgisLayers()) );

    connect ( mLayerComboBox, SIGNAL(activated(int)), this, SLOT(changed(int)) );

    if ( !mMapId.isEmpty() )
    {
	QgsGrassModuleItem *item = mModuleStandardOptions->item(mMapId);
	if ( item ) 
	{
	    QgsGrassModuleInput *mapInput = 
			   dynamic_cast<QgsGrassModuleInput *>(item);

	    connect ( mapInput, SIGNAL(valueChanged()), this, SLOT(updateQgisLayers()) );
	}
    }
    
    // Fill in QGIS layers 
    updateQgisLayers();
}

void QgsGrassModuleInput::updateQgisLayers()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleInput::updateQgisLayers" << std::endl;
    #endif
    QString current = mLayerComboBox->currentText ();
    mLayerComboBox->clear();
    mMaps.resize(0);
    mVectorTypes.resize(0);
    mVectorLayerNames.resize(0);
    mMapLayers.resize(0);
    mVectorFields.resize(0);

    QgsMapCanvas *canvas = mModule->qgisIface()->getMapCanvas();

    // Find map option
    QString sourceMap;
    if ( !mMapId.isEmpty() )
    {
        QgsGrassModuleItem *item = mModuleStandardOptions->item(mMapId);
	if ( item ) 
	{
            QgsGrassModuleInput *mapInput = 
		           dynamic_cast<QgsGrassModuleInput *>(item);
	    sourceMap = mapInput->currentMap();
	}
    }

    // Note: QDir::cleanPath is using '/' also on Windows
    //QChar sep = QDir::separator();
    QChar sep = '/';

    int nlayers = canvas->layerCount();
    for ( int i = 0; i < nlayers; i++ ) {
	QgsMapLayer *layer = canvas->getZpos(i);

        #ifdef QGISDEBUG
        std::cerr << "layer->type() = " << layer->type() << std::endl;
        #endif
	if (  mType == Vector && layer->type() == QgsMapLayer::VECTOR ) {
	    QgsVectorLayer *vector = (QgsVectorLayer*)layer;
            std::cerr << "vector->providerType() = " << vector->providerType().ascii() << std::endl;
	    if ( vector->providerType() != "grass" ) continue;

	    //TODO dynamic_cast ?
	    QgsGrassProvider *provider = (QgsGrassProvider *) vector->getDataProvider();

	    // Check type mask
	    int geomType = provider->geometryType();

	    if ( (geomType == QGis::WKBPoint && !(mVectorTypeMask & GV_POINT) ) ||
	         (geomType == QGis::WKBLineString && !(mVectorTypeMask & GV_LINE) ) ||
	         (geomType == QGis::WKBPolygon && !(mVectorTypeMask & GV_AREA) )
	       )
	    {
		continue;
	    }

	    // TODO add map() mapset() location() gisbase() to grass provider
	    QString source = QDir::cleanPath ( provider->getDataSourceUri() );
            #ifdef QGISDEBUG
            std::cerr << "source = " << source.ascii() << std::endl;
            #endif
	    
	    // Check GISBASE and LOCATION
	    QStringList split = QStringList::split ( sep, source );
            
	    if ( split.size() < 4 ) continue;
	    split.pop_back(); // layer

	    QString map = split.last();
	    split.pop_back(); // map

	    QString mapset = split.last();
	    split.pop_back(); // mapset
	    
	    //QDir locDir ( sep + split.join ( QString(sep) ) ) ;
	    //QString loc = locDir.canonicalPath();
            QString loc =  source.remove ( QRegExp("/[^/]+/[^/]+/[^/]+$") ); 
            loc = QDir(loc).canonicalPath();

	    QDir curlocDir ( QgsGrass::getDefaultGisdbase() + sep + QgsGrass::getDefaultLocation() );
	    QString curloc = curlocDir.canonicalPath();
            
            #ifdef QGISDEBUG
            std::cerr << "loc = " << loc.ascii() << std::endl;
            std::cerr << "curloc = " << curloc.ascii() << std::endl;
            std::cerr << "mapset = " << mapset.ascii() << std::endl;
            std::cerr << "QgsGrass::getDefaultMapset() = " << QgsGrass::getDefaultMapset().ascii() << std::endl;
            #endif
            
	    if ( loc != curloc ) continue;
             
	    if ( mUpdate && mapset != QgsGrass::getDefaultMapset() ) continue;

	    // Check if it comes from source map if necessary
            if ( !mMapId.isEmpty() )
	    {
		 QString cm = map + "@" + mapset;
	         if ( sourceMap != cm ) continue;
	    }

	    mLayerComboBox->insertItem( layer->name() );
	    if ( layer->name() == current ) mLayerComboBox->setCurrentText ( current );

	    mMaps.push_back ( map + "@" + mapset );

	    if ( geomType == QGis::WKBPoint ) {
	        mVectorTypes.push_back ( "point" );
	    } else if ( geomType == QGis::WKBLineString ) {
	        mVectorTypes.push_back ( "line" );
	    } else if ( geomType == QGis::WKBPolygon ) {
	        mVectorTypes.push_back ( "area" );
	    } else {
	        mVectorTypes.push_back ( "unknown" );
	    }

	    mMapLayers.push_back ( vector );
	    mVectorLayerNames.push_back ( QString::number(provider->grassLayer()) );
            std::vector<QgsField> fields = vector->fields();
	    mVectorFields.push_back ( fields );
	} 
	else if ( mType == Raster && layer->type() == QgsMapLayer::RASTER ) 
	{
	    // Check if it is GRASS raster
	    QString source = QDir::cleanDirPath ( layer->source() ); 

	    if ( source.contains( "cellhd" ) == 0 ) continue;
	    
	    // Most probably GRASS layer, check GISBASE and LOCATION
	    QStringList split = QStringList::split ( sep, source );
            
	    if ( split.size() < 4 ) continue;

	    QString map = split.last();
	    split.pop_back(); // map
	    if ( split.last() != "cellhd" ) continue;
	    split.pop_back(); // cellhd

	    QString mapset = split.last();
	    split.pop_back(); // mapset
	    
	    //QDir locDir ( sep + split.join ( QString(sep) ) ) ;
	    //QString loc = locDir.canonicalPath();
            QString loc =  source.remove ( QRegExp("/[^/]+/[^/]+/[^/]+$") ); 
            loc = QDir(loc).canonicalPath();

	    QDir curlocDir ( QgsGrass::getDefaultGisdbase() + sep + QgsGrass::getDefaultLocation() );
	    QString curloc = curlocDir.canonicalPath();
            
	    if ( loc != curloc ) continue;

	    if ( mUpdate && mapset != QgsGrass::getDefaultMapset() ) continue;

	    mLayerComboBox->insertItem( layer->name() );
	    if ( layer->name() == current ) mLayerComboBox->setCurrentText ( current );
	    mMaps.push_back ( map + "@" + mapset );
	}
    }
}

QStringList QgsGrassModuleInput::options()
{
    QStringList list;
    QString opt;

    int current = mLayerComboBox->currentItem();

    // TODO: this is hack for network nodes, do it somehow better
    if ( mMapId.isEmpty() )
    {
        opt = mKey + "=";

	if ( current <  mMaps.size() ) {
	    opt.append ( mMaps[current] );
	}
	list.push_back( opt );
    }

    if ( !mVectorTypeOption.isNull() && current < mVectorTypes.size() ) 
    {
	opt = mVectorTypeOption + "=" + mVectorTypes[current] ;
	list.push_back( opt );
    }
	
    if ( !mVectorLayerOption.isNull() && current < mVectorLayerNames.size() ) 
    {
	opt = mVectorLayerOption + "=" + mVectorLayerNames[current] ;
	list.push_back( opt );
    }
    
    return list;
}

std::vector<QgsField> QgsGrassModuleInput::currentFields()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleInput::currentFields" << std::endl;
    #endif

    std::vector<QgsField> fields;
    
    int current = mLayerComboBox->currentItem();

    if ( current >= 0 && current <  mVectorFields.size() ) 
    {
	fields = mVectorFields[current];
    }

    return fields;
}

QgsMapLayer * QgsGrassModuleInput::currentLayer()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleInput::currentLayer" << std::endl;
    #endif

    int current = mLayerComboBox->currentItem();

    if ( current >= 0 && current <  mMapLayers.size() ) 
    {
	return mMapLayers[current];
    }

    return 0;
}

QString QgsGrassModuleInput::currentMap()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleInput::currentMap" << std::endl;
    #endif

    int current = mLayerComboBox->currentItem();

    if ( current >= 0 && current <  mMaps.size() ) 
    {
	return mMaps[current];
    }

    return QString();
}

void QgsGrassModuleInput::changed(int i)
{
    emit valueChanged();
}

QString QgsGrassModuleInput::ready()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleInput::ready()" << std::endl;
    #endif

    QString error;

    std::cerr << "count = " << mLayerComboBox->count() << std::endl;
    if ( mLayerComboBox->count() == 0 )
    {
       error.append ( title() + ":&nbsp;no input" );
    }
    return error;
}

QgsGrassModuleInput::~QgsGrassModuleInput()
{
}

/********************** QgsGrassModuleItem *************************/

QgsGrassModuleItem::QgsGrassModuleItem( QgsGrassModule *module, QString key,
	 				QDomElement &qdesc, QDomElement &gdesc, QDomNode &gnode )
	:mKey(key),
	mHidden(false), 
	mModule(module)
{ 
    mAnswer = qdesc.attribute("answer", "");
    
    if ( qdesc.attribute("hidden") == "yes" ) {
	mHidden = true;
    }

    if ( !qdesc.attribute("label").isEmpty() )
    {
	mDescription = qdesc.attribute("label");
    }
    else
    {
	QDomNode n = gnode.namedItem ( "description" );
	if ( !n.isNull() ) {
	    QDomElement e = n.toElement();
	    mDescription = e.text().stripWhiteSpace();
	    mDescription.replace( 0, 1, mDescription.left(1).upper() );
	}
    }

    mId = qdesc.attribute("id");
}

bool QgsGrassModuleItem::hidden() { return mHidden; }

QStringList QgsGrassModuleItem::options() { return QStringList(); }

QgsGrassModuleItem::~QgsGrassModuleItem() {}

/***************** QgsGrassModuleGdalInput *********************/

QgsGrassModuleGdalInput::QgsGrassModuleGdalInput ( 
	QgsGrassModule *module, int type, QString key, QDomElement &qdesc, 
	QDomElement &gdesc, QDomNode &gnode, QWidget * parent)
      : Q3GroupBox ( 1, Qt::Vertical, parent ),
        QgsGrassModuleItem ( module, key, qdesc, gdesc, gnode ),
        mType(type), mOgrLayerOption(0)
{
    QString tit;
    if ( mDescription.isEmpty() ) 
    {
	tit = "OGR/GDAL Input";
    } 
    else 
    {
	if ( mDescription.length() > 40 ) {
	    tit = mDescription.left(40) + " ...";
	} else {
	    tit = mDescription;
	}
    }
	    
    setTitle ( " " + tit + " " );

    QDomNode promptNode = gnode.namedItem ( "gisprompt" );
    QDomElement promptElem = promptNode.toElement();
    QString element = promptElem.attribute("element"); 

    // Read "layeroption" is defined
    QString opt = qdesc.attribute("layeroption");
    if ( ! opt.isNull() ) {
    
	QDomNode optNode = QgsGrassModule::nodeByKey ( gdesc, opt );

	if ( optNode.isNull() ) 
	{
	    QMessageBox::warning( 0, "Warning", "Cannot find layeroption " +  opt );
	} 
	else 
	{
	    mOgrLayerOption = opt;
	}
    }

    mLayerComboBox = new QComboBox ( this );

    // Of course, activated(int) is not enough, but there is no signal 
    // BEFORE the cobo is opened 
    // connect ( mLayerComboBox, SIGNAL( activated(int) ), this, SLOT(updateQgisLayers()) );
    
    // Connect to canvas 
    QgsMapCanvas *canvas = mModule->qgisIface()->getMapCanvas();
    connect ( canvas, SIGNAL(addedLayer(QgsMapLayer *)), this, SLOT(updateQgisLayers()) );
    connect ( canvas, SIGNAL(removedLayer(QString)), this, SLOT(updateQgisLayers()) );
    
    // Fill in QGIS layers 
    updateQgisLayers();
}

void QgsGrassModuleGdalInput::updateQgisLayers()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleGdalInput::updateQgisLayers" << std::endl;
    #endif
    QString current = mLayerComboBox->currentText ();
    mLayerComboBox->clear();
    mUri.resize(0);
    mOgrLayers.resize(0);

    QgsMapCanvas *canvas = mModule->qgisIface()->getMapCanvas();

    int nlayers = canvas->layerCount();
    for ( int i = 0; i < nlayers; i++ ) {
	QgsMapLayer *layer = canvas->getZpos(i);

	if (  mType == Ogr && layer->type() == QgsMapLayer::VECTOR ) 
	{
	    QgsVectorLayer *vector = (QgsVectorLayer*)layer;
	    if ( vector->providerType() != "ogr" ) continue;

	    QgsDataProvider *provider = vector->getDataProvider();
	    QString uri = provider->getDataSourceUri();

	    mLayerComboBox->insertItem( layer->name() );
	    if ( layer->name() == current ) mLayerComboBox->setCurrentText ( current );
	    mUri.push_back ( uri );
	} 
	else if ( mType == Gdal && layer->type() == QgsMapLayer::RASTER ) 
	{
            QString uri = layer->source();
	    mLayerComboBox->insertItem( layer->name() );
	    if ( layer->name() == current ) mLayerComboBox->setCurrentText ( current );
	    mUri.push_back ( uri );
	}
    }
}

QStringList QgsGrassModuleGdalInput::options()
{
    QStringList list;

    int current = mLayerComboBox->currentItem();

    QString opt(mKey + "=");

    if ( current <  mUri.size() ) {
	opt.append ( mUri[current] );
    }
    list.push_back( opt );

    // TODO
    /*
    if ( !mOgrLayerOption.isNull() ) 
    {
	opt = mOgrLayerOption + "=" ;
	list.push_back( opt );
    }
    */
    return list;
}

QgsGrassModuleGdalInput::~QgsGrassModuleGdalInput()
{
}

/***************** QgsGrassModuleField *********************/

QgsGrassModuleField::QgsGrassModuleField ( 
	QgsGrassModule *module, QgsGrassModuleStandardOptions *options,
       	QString key, QDomElement &qdesc, 
	QDomElement &gdesc, QDomNode &gnode, QWidget * parent)
      : Q3GroupBox ( 1, Qt::Vertical, parent ),
        QgsGrassModuleItem ( module, key, qdesc, gdesc, gnode ),
	mModuleStandardOptions(options), mLayerInput(0)
{
    QString tit;
    if ( mDescription.isEmpty() ) 
    {
	tit = "Attribute field";
    } 
    else 
    {
	if ( mDescription.length() > 40 ) {
	    tit = mDescription.left(40) + " ...";
	} else {
	    tit = mDescription;
	}
    }
	    
    setTitle ( " " + tit + " " );

    QDomNode promptNode = gnode.namedItem ( "gisprompt" );
    QDomElement promptElem = promptNode.toElement();
    QString element = promptElem.attribute("element"); 

    mLayerId = qdesc.attribute("layerid");

    mType = qdesc.attribute("type");

    QgsGrassModuleItem *item = mModuleStandardOptions->item(mLayerId);
    // TODO check type
    if ( item ) 
    {
        mLayerInput = dynamic_cast<QgsGrassModuleInput *>(item);
        connect ( mLayerInput, SIGNAL(valueChanged()), this, SLOT(updateFields()) );
    }

    mFieldComboBox = new QComboBox ( this );

    // Fill in layer current fields
    updateFields();
}

void QgsGrassModuleField::updateFields()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleField::updateFields" << std::endl;
    #endif
    QString current = mFieldComboBox->currentText ();
    mFieldComboBox->clear();

    QgsMapCanvas *canvas = mModule->qgisIface()->getMapCanvas();

    if ( mLayerInput == 0 ) return;

    std::vector<QgsField> fields = mLayerInput->currentFields();

    for ( int i = 0; i < fields.size(); i++ )
    {
	if ( mType.contains ( fields[i].type() ) )
	{
            mFieldComboBox->insertItem( fields[i].name() );
	    if ( fields[i].name() == current )
	    {
		mFieldComboBox->setCurrentText ( current );
	    }
	}
    }
}

QStringList QgsGrassModuleField::options()
{
    QStringList list;

    QString opt(mKey + "=" + mFieldComboBox->currentText() );
    list.push_back( opt );

    return list;
}

QgsGrassModuleField::~QgsGrassModuleField()
{
}

/***************** QgsGrassModuleSelection *********************/

QgsGrassModuleSelection::QgsGrassModuleSelection ( 
	QgsGrassModule *module, QgsGrassModuleStandardOptions *options,
       	QString key, QDomElement &qdesc, 
	QDomElement &gdesc, QDomNode &gnode, QWidget * parent)
      : Q3GroupBox ( 1, Qt::Vertical, parent ),
        QgsGrassModuleItem ( module, key, qdesc, gdesc, gnode ),
	mModuleStandardOptions(options), mLayerInput(0),
	mVectorLayer(0)
{
    QString tit;
    if ( mDescription.isEmpty() ) 
    {
	tit = "Attribute field";
    } 
    else 
    {
	if ( mDescription.length() > 40 ) {
	    tit = mDescription.left(40) + " ...";
	} else {
	    tit = mDescription;
	}
    }
	    
    setTitle ( " " + tit + " " );

    QDomNode promptNode = gnode.namedItem ( "gisprompt" );
    QDomElement promptElem = promptNode.toElement();
    QString element = promptElem.attribute("element"); 

    mLayerId = qdesc.attribute("layerid");

    mType = qdesc.attribute("type");

    QgsGrassModuleItem *item = mModuleStandardOptions->item(mLayerId);
    // TODO check type
    if ( item ) 
    {
        mLayerInput = dynamic_cast<QgsGrassModuleInput *>(item);
        connect ( mLayerInput, SIGNAL(valueChanged()), this, SLOT(updateSelection()) );
    }

    mLineEdit = new QLineEdit ( this );

    // Fill in layer current fields
    updateSelection();
}

void QgsGrassModuleSelection::updateSelection()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModuleSelection::updateSelection" << std::endl;
    #endif

    mLineEdit->setText("");
    QgsMapCanvas *canvas = mModule->qgisIface()->getMapCanvas();
    if ( mLayerInput == 0 ) return;

    QgsMapLayer *layer = mLayerInput->currentLayer();
    if ( !layer ) return;
    QgsVectorLayer *vector = dynamic_cast<QgsVectorLayer*>(layer);
	    
    QgsGrassProvider *provider = (QgsGrassProvider *) vector->getDataProvider();
    int keyField = provider->keyField();

    if ( keyField < 0 ) return;
    
    QString cats;
    QgsFeature *feature = vector->getFirstFeature(true, true);

    int i = 0;
    while ( feature )
    {
	std::vector<QgsFeatureAttribute> attr = feature->attributeMap();
	if ( attr.size() > keyField )
	{
	    if ( i > 0 ) cats.append( "," );
	    cats.append( attr[keyField].fieldValue() );
	    i++;
	}
        feature = vector->getNextFeature(true, true);
    }
    if ( mVectorLayer != vector ) 
    {
	if ( mVectorLayer ) 
	{
            disconnect ( mVectorLayer, SIGNAL(selectionChanged()), this, SLOT(updateSelection()) );
	}

        connect ( vector, SIGNAL(selectionChanged()), this, SLOT(updateSelection()) );
	mVectorLayer = vector;
    }
    
    mLineEdit->setText(cats);
}

QStringList QgsGrassModuleSelection::options()
{
    QStringList list;

    QString opt(mKey + "=" + mLineEdit->text() );
    list.push_back( opt );

    return list;
}

QgsGrassModuleSelection::~QgsGrassModuleSelection()
{
}
