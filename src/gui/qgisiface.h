/***************************************************************************
                          qgisiface.h 
 Interface class for exposing functions in QgisApp for use by plugins
                             -------------------
  begin                : 2004-02-11
  copyright            : (C) 2004 by Gary E.Sherman
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
/* $Id$ */
#ifndef QGISIFACE_H
#define QGISIFACE_H
#include "qgisinterface.h"

class QMenu;
class QgsMapCanvas;
class QgsMapLayer;
/** \class QgisIface
 * \brief Interface class to provide access to private methods in QgisApp
 * for use by plugins.
 * 
 * Only those functions "exposed" by QgisIface can be called from within a
 * plugin.
 */
class QgisIface : public QgisInterface
{
    Q_OBJECT;

    public:
        /**
         * Constructor.
         * @param qgis Pointer to the QgisApp object
         */
        QgisIface(QgisApp *qgis=0, const char *name=0);
        ~QgisIface();
        /* Exposed functions */
        //! Zoom map to full extent
        void zoomFull();
        //! Zoom map to previous extent
        void zoomPrevious();
        //! Zoom to active layer
        void zoomActiveLayer();
        //! Add a vector layer
        bool addVectorLayer(QString vectorLayerPath, QString baseName, QString providerKey);
        //! Add a raster layer given its file name
        bool addRasterLayer(QString rasterLayerPath);
        //! Add a raster layer given a raster layer obj
        bool addRasterLayer(QgsRasterLayer * theRasterLayer,bool theForceRenderFlag=false);
        //! Add a project
        bool addProject(QString theProjectName);
        //! Start a new blank project
        void newProject(bool thePromptToSaveFlag=false);
        //! Get pointer to the active layer (layer selected in the legend)
        QgsMapLayer *activeLayer();
        //! Get source of the active layer
        QString activeLayerSource();

	void addPluginMenu(QString name, QAction* action);
	void removePluginMenu(QString name, QAction* action);

	void removePluginMenuItem(QString name, int menuId);

        //! Add an icon to the plugins toolbar
        int addToolBarIcon(QAction *qAction);
        //! Remove an icon (action) from the plugin toolbar
        void removeToolBarIcon(QAction *qAction);
        /** Open a url in the users browser. By default the QGIS doc directory is used
         * as the base for the URL. To open a URL that is not relative to the installed
         * QGIS documentation, set useQgisDocDirectory to false.
         * @param url URL to open
         * @param useQgisDocDirectory If true, the URL will be formed by concatenating 
         * url to the QGIS documentation directory path (<prefix>/share/doc)
         */
        void openURL(QString url, bool useQgisDocDirectory=true);

        /** 
         * Get the menu info mapped by menu name (key is name, value is menu id)
         */
        std::map<QString,int> menuMapByName();
        /** 
         * Get the menu info mapped by menu id (key is menu id, value is name)
         */
        std::map<int,QString> menuMapById();
        /** Return a pointer to the map canvas used by qgisapp */
        QgsMapCanvas * getMapCanvas();	
        /** Return a pointer to the map layer registry */
        QgsMapLayerRegistry * getLayerRegistry();	


        /** Gives access to main QgisApp object

          Even though this class is supposed to act as a Facade for the
          QgisApp, the plug-ins need direct access to the application object
          for their connect() calls.

          @todo XXX this may call into question the current need for this
          interface?  Maybe these connect() calls can be done in some other
          less intrusive way?

        */
        QgisApp * app();

    public slots:
         void emitCurrentLayerChanged ( QgsMapLayer * layer );
         

    signals:
         //! Emited whenever current (selected) layer changes
         //  the pointer to layer can be null if no layer is selected
         void currentLayerChanged ( QgsMapLayer * layer );

    private:

        /// QgisIface aren't copied
        QgisIface( QgisIface const & );
        
        /// QgisIface aren't copied
        QgisIface & operator=( QgisIface const & );

        //! Pointer to the QgisApp object
        QgisApp *qgis;
};


#endif //#define QGISIFACE_H
