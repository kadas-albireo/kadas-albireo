/*******************************************************************
                              qgsgrassbrowser.cpp
                             -------------------
    begin                : February, 2006
    copyright            : (C) 2006 by Radim Blazek
    email                : radim.blazek@gmail.com
********************************************************************/
/********************************************************************
 This program is free software; you can redistribute it and/or modify  
 it under the terms of the GNU General Public License as published by 
 the Free Software Foundation; either version 2 of the License, or     
 (at your option) any later version.                                   
*******************************************************************/
#include <iostream>
#include <vector>

#include <QApplication>
#include <QStyle>
#include <qdir.h>
#include <qfile.h>
#include <qsettings.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qnamespace.h>
#include <qevent.h>
#include <qsize.h>
#include <qicon.h>
#include <QTreeView>
#include <QHeaderView>
#include <QMainWindow>
#include <QActionGroup>
#include <QToolBar>
#include <QAction>
#include <QTextBrowser>
#include <QSplitter>
#include <QProcess>
#include <QScrollBar>

#include "qgis.h"
#include "qgsapplication.h"
#include "qgsrasterlayer.h"

extern "C" {
#include <grass/gis.h>
#include <grass/Vect.h>
}

#include "../../src/providers/grass/qgsgrass.h"
#include "qgsgrassmodel.h"
#include "qgsgrassbrowser.h"
#include "qgsgrassselect.h"
#include "qgsgrassutils.h"

QgsGrassBrowser::QgsGrassBrowser ( QgisIface *iface, 
	 QWidget * parent, Qt::WFlags f )
             :mIface(iface), QMainWindow(parent, Qt::WType_Dialog)
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassBrowser()" << std::endl;
    #endif

    QActionGroup *ag = new QActionGroup ( this );
    QToolBar *tb = addToolBar(tr("Tools"));

    QString myIconPath = QgsApplication::themePath() + "/grass/";
    mActionAddMap = new QAction( 
	                     QIcon(myIconPath+"grass_add_map.png"), 
	                     tr("Add selected map to canvas"), this);
    mActionAddMap->setEnabled(false); 
    ag->addAction ( mActionAddMap );
    tb->addAction ( mActionAddMap );
    connect ( mActionAddMap, SIGNAL(triggered()), this, SLOT(addMap()) );

    mActionDeleteMap = new QAction( 
	                     QIcon(myIconPath+"grass_delete_map.png"), 
	                     tr("Delete selected map"), this);
    mActionDeleteMap->setEnabled(false); 
    ag->addAction ( mActionDeleteMap );
    tb->addAction ( mActionDeleteMap );
    connect ( mActionDeleteMap, SIGNAL(triggered()), this, SLOT(deleteMap()) );

    mActionSetRegion = new QAction( 
	                     QIcon(myIconPath+"grass_set_region.png"), 
	                     tr("Set current region to selected map"), this);
    mActionSetRegion->setEnabled(false); 
    ag->addAction ( mActionSetRegion );
    tb->addAction ( mActionSetRegion );
    connect ( mActionSetRegion, SIGNAL(triggered()), this, SLOT(setRegion()) );

    mActionRefresh = new QAction( 
	                     QIcon(myIconPath+"grass_refresh.png"), 
	                     tr("Refresh"), this);
    ag->addAction ( mActionRefresh );
    tb->addAction ( mActionRefresh );
    connect ( mActionRefresh, SIGNAL(triggered()), this, SLOT(refresh()) );

    // Add model
    mModel = new QgsGrassModel ( this );

    mTree = new QTreeView(0);
    mTree->header()->hide();
    mTree->setModel(mModel);

    mTextBrowser = new QTextBrowser(0);
    mTextBrowser->setTextFormat(Qt::RichText);
    mTextBrowser->setReadOnly(TRUE);

    mSplitter = new QSplitter(0);
    mSplitter->addWidget(mTree);
    mSplitter->addWidget(mTextBrowser);

    this->setCentralWidget(mSplitter);

    connect ( mTree->selectionModel(), 
	      SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
	      this, SLOT(selectionChanged(QItemSelection,QItemSelection)) );

    connect ( mTree->selectionModel(), 
	      SIGNAL(currentChanged(QModelIndex,QModelIndex)),
	      this, SLOT(currentChanged(QModelIndex,QModelIndex)) );

    connect ( mTree, SIGNAL(doubleClicked(QModelIndex)),
	      this, SLOT(doubleClicked(QModelIndex)) );
}

QgsGrassBrowser::~QgsGrassBrowser() { }

void QgsGrassBrowser::refresh()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassBrowser::refresh()" << std::endl;
    #endif

    mModel->refresh();
    mTree->update();
}

void QgsGrassBrowser::addMap()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassBrowser::addMap()" << std::endl;
    #endif
    
    QModelIndexList indexes = mTree->selectionModel()->selectedIndexes();
    bool mapSelected = false;

    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
    {
	int type = mModel->itemType(*it);
	QString uri = mModel->uri(*it);
        QString mapset = mModel->itemMapset(*it);
        QString map = mModel->itemMap(*it);
        if ( type == QgsGrassModel::Raster )
	{
            std::cerr << "add raster: " << uri.ascii() << std::endl;
            QgsRasterLayer *layer = new QgsRasterLayer( uri, map );
            mIface->addRasterLayer(layer);
	    mapSelected = true;
	}
	else if ( type == QgsGrassModel::Vector )
	{
            QgsGrassUtils::addVectorLayers ( mIface, 
                                QgsGrass::getDefaultGisdbase(),
                                QgsGrass::getDefaultLocation(),
                                mapset, map );
        } 
	else if ( type == QgsGrassModel::VectorLayer )
	{

            QStringList list = QgsGrassSelect::vectorLayers(
                                   QgsGrass::getDefaultGisdbase(),
                                   QgsGrass::getDefaultLocation(),
                          mModel->itemMapset(*it), map );

	    // TODO: common method for vector names
	    QStringList split = QStringList::split ( '/', uri );
	    QString layer = split.last();

            QString name = QgsGrassUtils::vectorLayerName ( 
                                map, layer, list.size() );
            
	    mIface->addVectorLayer( uri, name, "grass");
	    mapSelected = true;
	}
    }
}

void QgsGrassBrowser::doubleClicked(const QModelIndex & index)
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassBrowser::doubleClicked()" << std::endl;
    #endif

    addMap();
}

void QgsGrassBrowser::deleteMap()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassBrowser::deleteMap()" << std::endl;
    #endif
    
    QModelIndexList indexes = mTree->selectionModel()->selectedIndexes();
    bool mapSelected = false;

    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
    {
	int type = mModel->itemType(*it);
	QString mapset = mModel->itemMapset(*it);
	QString map = mModel->itemMap(*it);

        QString typeName;
        if ( type == QgsGrassModel::Raster ) typeName = "rast";
        else if ( type == QgsGrassModel::Vector ) typeName = "vect";

	if ( mapset != QgsGrass::getDefaultMapset() ) 
	{
            continue; // should not happen
        }
         
        int ret = QMessageBox::question ( 0, "Warning",
              "Delete map <b>" + map + "</b>",
              QMessageBox::Yes,  QMessageBox::No );

        if ( ret == QMessageBox::No ) continue;

        QString module = "g.remove";
#ifdef WIN32
	module.append(".exe");
#endif
        QProcess process(this);
        process.start(module, QStringList( typeName + "=" + map ) );
        if ( !process.waitForFinished() )
        {
            QMessageBox::warning( 0, "Warning", "Cannot delete map "
                                  + map ); 
        }
        else
        {
            refresh();
        }
    }
}

void QgsGrassBrowser::setRegion()
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassBrowser::setRegion()" << std::endl;
    #endif

    struct Cell_head window;
    
    QModelIndexList indexes = mTree->selectionModel()->selectedIndexes();

    QgsGrass::setLocation( QgsGrass::getDefaultGisdbase(),
                         QgsGrass::getDefaultLocation() );

    // TODO multiple selection - extent region to all maps
    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
    {
	int type = mModel->itemType(*it);
	QString uri = mModel->uri(*it);
        QString mapset = mModel->itemMapset(*it);
        QString map = mModel->itemMap(*it);

        if ( type == QgsGrassModel::Raster )
	{

            if ( G_get_cellhd ( map.toLocal8Bit().data(), 
                          mapset.toLocal8Bit().data(), &window) < 0 )
            {
                QMessageBox::warning( 0, "Warning", 
                         "Cannot read raster map region" ); 
                return;
            }
	}
	else if ( type == QgsGrassModel::Vector )
	{
            G_get_window ( &window ); // get current resolution

            struct Map_info Map;

            int level = Vect_open_old_head ( &Map, 
                  map.toLocal8Bit().data(), mapset.toLocal8Bit().data());

            if ( level < 2 ) 
            { 
                QMessageBox::warning( 0, "Warning", 
                         "Cannot read vector map region" ); 
                return;
            }

            BOUND_BOX box;
            Vect_get_map_box (&Map, &box );
	    window.north = box.N;
	    window.south = box.S;
	    window.west  = box.W;
	    window.east  = box.E;
            
            Vect_close (&Map);
        } 
    }

    // Reset mapset (selected maps could be in a different one)
    QgsGrass::setMapset( QgsGrass::getDefaultGisdbase(),
                         QgsGrass::getDefaultLocation(),
                         QgsGrass::getDefaultMapset() );

    if ( G_put_window ( &window ) == -1 )
    { 
	QMessageBox::warning( 0, "Warning", 
		 "Cannot write new region" ); 
        return;
    }
    emit regionChanged();
}

void QgsGrassBrowser::selectionChanged(const QItemSelection & selected, const QItemSelection & deselected)
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassBrowser::selectionChanged()" << std::endl;
    #endif

    mActionAddMap->setEnabled(false);
    mActionDeleteMap->setEnabled(false);
    mActionSetRegion->setEnabled(false);
    
    QModelIndexList indexes = mTree->selectionModel()->selectedIndexes();

    mTextBrowser->clear();

    QList<QModelIndex>::const_iterator it = indexes.begin();
    for (; it != indexes.end(); ++it)
    {
        mTextBrowser->append ( mModel->itemInfo(*it) );
        mTextBrowser->verticalScrollBar()->setValue(0);
	
	int type = mModel->itemType(*it);
        if ( type == QgsGrassModel::Raster || 
             type == QgsGrassModel::Vector || 
             type == QgsGrassModel::VectorLayer )
	{
	    mActionAddMap->setEnabled(true);
	}
        if ( type == QgsGrassModel::Raster || type == QgsGrassModel::Vector )
	{
            mActionSetRegion->setEnabled(true);

	    QString mapset = mModel->itemMapset(*it);
            if ( mapset == QgsGrass::getDefaultMapset() ) 
            {
	        mActionDeleteMap->setEnabled(true);
            }
	}
    }
}

void QgsGrassBrowser::currentChanged(const QModelIndex & current, const QModelIndex & previous)
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassBrowser::currentChanged()" << std::endl;
    #endif
}

