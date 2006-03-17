/***************************************************************************
    qgsgrassplugin.h  -  GRASS menu
                             -------------------
    begin                : March, 2004
    copyright            : (C) 2004 by Radim Blazek
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
 /*  $Id$ */
#ifndef QGSGRASSPLUGIN_H
#define QGSGRASSPLUGIN_H
#include "../qgisplugin.h"
#include <qwidget.h>
#include <qpen.h>

#include "qgisapp.h"

#include <vector>

class QgsRubberBand;

class QgsGrassTools;
class QgsGrassNewMapset;
class QToolBar;
/**
* \class QgsGrassPlugin
* \brief OpenModeller plugin for QGIS
*
*/
class QgsGrassPlugin:public QObject, public QgisPlugin
{
Q_OBJECT public:
      /** 
       * Constructor for a plugin. The QgisApp and QgisIface pointers are passed by 
       * QGIS when it attempts to instantiate the plugin.
       * @param qgis Pointer to the QgisApp object
       * @param qI Pointer to the QgisIface object. 
       */
      QgsGrassPlugin(QgisApp * , QgisIface * );
  /**
   * Virtual function to return the name of the plugin. The name will be used when presenting a list 
   * of installable plugins to the user
   */
  virtual QString name();
  /**
   * Virtual function to return the version of the plugin. 
   */
  virtual QString version();
  /**
   * Virtual function to return a description of the plugins functions 
   */
  virtual QString description();
  /**
   * Return the plugin type
   */
  virtual int type();
  //! Destructor
  virtual ~ QgsGrassPlugin();

  //! Get Region Pen
  QPen & regionPen(void);
  //! Set Region Pen
  void setRegionPen(QPen &);

public slots:
  //! init the gui
  virtual void initGui();
  //! Show the dialog box for new vector
  void addVector();
  //! Show the dialog box for new raster
  void addRaster();
  //! Start vector editing
  void edit();
  //! unload the plugin
  void unload();
  //! show the help document
  void help();
  //! Display current region
  void displayRegion();
  //! Switch region on/off
  void switchRegion(bool on);
  //! Change region
  void changeRegion(void);
  //! Redraw region
  void redrawRegion(void);
  //! Post render
  void postRender(QPainter *);
  //! Open tools 
  void openTools(void);
  //! Create new mapset
  void newMapset();
  //! Open existing mapset
  void openMapset();
  //! Close mapset
  void closeMapset();
  //! Current mapset changed (opened/closed)
  void mapsetChanged(); 
  //! Create new vector
  void newVector();
  //! Read project
  void projectRead();
  //! New project
  void newProject();


private:
  //! Name of the plugin
  QString pluginNameQString;
  //! Version
  QString pluginVersionQString;
  //! Descrption of the plugin
  QString pluginDescriptionQString;
  //! Plugin type as defined in QgisPlugin::PLUGINTYPE
  int pluginType;
  //! Id of the plugin's menu. Used for unloading
  std::vector<int> menuId;
  //! Pointer to our toolbar
  QToolBar *toolBarPointer;
  //! Pionter to QGIS main application object
  QgisApp *mQgis;
  //! Pointer to the QGIS interface object
  QgisIface *qGisInterface;
  //! Pointer to canvas
  QgsMapCanvas *mCanvas;

  //! Pointer to Display region acction
  QAction *mRegionAction;
  //! Region width
  QPen mRegionPen;
  // Region rubber band
  QgsRubberBand *mRegion;
  //! GRASS tools
  QgsGrassTools *mTools;
  //! Pointer to QgsGrassNewMapset
  QgsGrassNewMapset *mNewMapset;

  // Actions
  QAction *mOpenMapsetAction;
  QAction *mNewMapsetAction;
  QAction *mCloseMapsetAction;
  QAction *mAddVectorAction;
  QAction *mAddRasterAction;
  QAction *mOpenToolsAction;
  QAction *mEditRegionAction; 
  QAction *mEditAction;
  QAction *mNewVectorAction;
};

#endif // QGSGRASSPLUGIN_H
