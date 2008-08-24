/***************************************************************************
                    qgsproviderregistry.cpp  -  Singleton class for
                    registering data providers.
                             -------------------
    begin                : Sat Jan 10 2004
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

#include "qgsproviderregistry.h"

#include <iostream>

#include <QString>
#include <QDir>
#include <QLibrary>


#include "qgis.h"
#include "qgsdataprovider.h"
#include "qgslogger.h"
#include "qgsmessageoutput.h"
#include "qgsprovidermetadata.h"


// typedefs for provider plugin functions of interest
typedef QString providerkey_t();
typedef QString description_t();
typedef bool    isprovider_t();
typedef QString fileVectorFilters_t();

QgsProviderRegistry *QgsProviderRegistry::_instance = 0;

QgsProviderRegistry *QgsProviderRegistry::instance( QString pluginPath )
{
  if ( _instance == 0 )
  {
    _instance = new QgsProviderRegistry( pluginPath );
  }

  return _instance;

} // QgsProviderRegistry::instance



QgsProviderRegistry::QgsProviderRegistry( QString pluginPath )
{
  // At startup, examine the libs in the qgis/lib dir and store those that
  // are a provider shared lib
  // check all libs in the current plugin directory and get name and descriptions
  //TODO figure out how to register and identify data source plugin for a specific
  //TODO layer type
  /* char **argv = qApp->argv();
     QString appDir = argv[0];
     int bin = appDir.findRev("/bin", -1, false);
     QString baseDir = appDir.left(bin);
     QString mLibraryDirectory = baseDir + "/lib"; */
  mLibraryDirectory = pluginPath;

  mLibraryDirectory.setSorting( QDir::Name | QDir::IgnoreCase );
  mLibraryDirectory.setFilter( QDir::Files | QDir::NoSymLinks );

#ifdef WIN32
  mLibraryDirectory.setNameFilters( QStringList( "*.dll" ) );
#else
  mLibraryDirectory.setNameFilters( QStringList( "*.so" ) );
#endif

#ifdef QGISDEBUG
  QgsLogger::debug( "Checking " + mLibraryDirectory.path() + " for provider plugins\n", 1, __FILE__, __FUNCTION__, __LINE__ );
#endif

  if ( mLibraryDirectory.count() == 0 )
  {
    QString msg = QObject::tr( "No Data Provider Plugins",
                               "No QGIS data provider plugins found in:" );
    msg += "\n" + mLibraryDirectory.path() + "\n\n";
    msg += QObject::tr( "No vector layers can be loaded. Check your QGIS installation" );

    QgsMessageOutput* output = QgsMessageOutput::createMessageOutput();
    output->setTitle( QObject::tr( "No Data Providers" ) );
    output->setMessage( msg, QgsMessageOutput::MessageText );
    output->showMessage();
  }
  else
  {
    const QFileInfoList list = mLibraryDirectory.entryInfoList();
    QListIterator<QFileInfo> it( list );
    QFileInfo fi;

    while ( it.hasNext() )
    {
      fi = it.next();

      QLibrary *myLib = new QLibrary( fi.filePath() );

      bool loaded = myLib->load();
      //we will build up a debug message and print on one line to avoid terminal spam
      QString myMessage =  "Checking  " + myLib->fileName() + " : " ;

      if ( loaded )
      {
        // get the description and the key for the provider plugin
        isprovider_t *isProvider = ( isprovider_t * ) cast_to_fptr( myLib->resolve( "isProvider" ) );

        //MH: Added a further test to detect non-provider plugins linked to provider plugins.
        //Only pure provider plugins have 'type' not defined
        isprovider_t *hasType = ( isprovider_t * ) cast_to_fptr( myLib->resolve( "type" ) );

        if ( !hasType && isProvider )
        {
          // check to see if this is a provider plugin
          if ( isProvider() )
          {
            // looks like a provider. get the key and description
            description_t *pDesc = ( description_t * ) cast_to_fptr( myLib->resolve( "description" ) );
            providerkey_t *pKey = ( providerkey_t * ) cast_to_fptr( myLib->resolve( "providerKey" ) );
            if ( pDesc && pKey )
            {
              // add this provider to the provider map
              mProviders[pKey()] =
                new QgsProviderMetadata( pKey(), pDesc(), myLib->fileName() );
              //myMessage += "Loaded " + QString(pDesc()) + " ok";

              // now get vector file filters, if any
              fileVectorFilters_t *pFileVectorFilters =
                ( fileVectorFilters_t * ) cast_to_fptr( myLib->resolve( "fileVectorFilters" ) );

              if ( pFileVectorFilters )
              {
                QString vectorFileFilters = pFileVectorFilters();

                // now get vector file filters, if any
                fileVectorFilters_t *pVectorFileFilters =
                  ( fileVectorFilters_t * ) cast_to_fptr( myLib->resolve( "fileVectorFilters" ) );

                if ( pVectorFileFilters )
                {
                  QString fileVectorFilters = pVectorFileFilters();

                  if ( ! fileVectorFilters.isEmpty() )
                  {
                    mVectorFileFilters += fileVectorFilters;
                    myMessage += QString( "... loaded ok (and with %1 file filters)" ).
                                 arg( fileVectorFilters.split( ";;" ).count() );
                  }
                  else
                  {
                    //myMessage += ", but it has no vector file filters for " + QString(pKey());
                    myMessage += "... loaded ok (0 file filters)";
                  }
                }
              }
              else
              {
                //myMessage += ", but unable to invoke fileVectorFilters()";
                myMessage += "... loaded ok (null file filters)";
              }
            }
            else
            {
              //myMessage += ", but unable to find one of the required provider functions (providerKey() or description()) in ";
              myMessage += "...not usable";

            }
          }
          else
          {
            //myMessage += ", but this is not a valid provider, skipping.";
            myMessage += "..invalid";
          }
        }
        else
        {
          //myMessage += ", but this is not a valid provider or has no type, skipping.";
          myMessage += "..invalid (no type)";
        }
      }
      else
      {
        myMessage += "...invalid (lib not loadable)";
      }

      QgsDebugMsg( myMessage );

      delete myLib;
    }
  }

} // QgsProviderRegistry ctor


QgsProviderRegistry::~QgsProviderRegistry()
{
}


/** convenience function for finding any existing data providers that match "providerKey"

  Necessary because [] map operator will create a QgsProviderMetadata
  instance.  Also you cannot use the map [] operator in const members for that
  very reason.  So there needs to be a convenient way to find a data provider
  without accidentally adding a null meta data item to the metadata map.
*/
static
QgsProviderMetadata * findMetadata_( QgsProviderRegistry::Providers const & metaData,
                                     QString const & providerKey )
{
  QgsProviderRegistry::Providers::const_iterator i =
    metaData.find( providerKey );

  if ( i != metaData.end() )
  {
    return i->second;
  }

  return 0x0;
} // findMetadata_



QString QgsProviderRegistry::library( QString const & providerKey ) const
{
  QgsProviderMetadata * md = findMetadata_( mProviders, providerKey );

  if ( md )
  {
    return md->library();
  }

  return QString();
}


QString QgsProviderRegistry::pluginList( bool asHTML ) const
{
  Providers::const_iterator it = mProviders.begin();
  QString list;

  if ( mProviders.empty() )
  {
    list = QObject::tr( "No data provider plugins are available. No vector layers can be loaded" );
  }
  else
  {
    if ( asHTML )
    {
      list += "<ol>";
    }
    while ( it != mProviders.end() )
    {
      QgsProviderMetadata *mp = ( *it ).second;

      if ( asHTML )
      {
        list += "<li>" + mp->description() + "<br>";
      }
      else
      {
        list += mp->description() + "\n";
      }

      it++;
    }
    if ( asHTML )
    {
      list += "</ol>";
    }
  }

  return list;
}


void QgsProviderRegistry::setLibraryDirectory( QDir const & path )
{
  mLibraryDirectory = path;
}


QDir const & QgsProviderRegistry::libraryDirectory() const
{
  return mLibraryDirectory;
}



// typedef for the QgsDataProvider class factory
typedef QgsDataProvider * classFactoryFunction_t( const QString * );



/** Copied from QgsVectorLayer::setDataProvider
 *  TODO: Make it work in the generic environment
 *
 *  TODO: Is this class really the best place to put a data provider loader?
 *        It seems more sensible to provide the code in one place rather than
 *        in qgsrasterlayer, qgsvectorlayer, serversourceselect, etc.
 */
QgsDataProvider* QgsProviderRegistry::getProvider( QString const & providerKey,
    QString const & dataSource )
{
  // XXX should I check for and possibly delete any pre-existing providers?
  // XXX How often will that scenario occur?

  // load the plugin
  QString lib = library( providerKey );

#ifdef TESTPROVIDERLIB
  const char *cLib = lib.toUtf8();

  // test code to help debug provider loading problems
  //  void *handle = dlopen(cLib, RTLD_LAZY);
  void *handle = dlopen( cOgrLib, RTLD_LAZY | RTLD_GLOBAL );
  if ( !handle )
  {
    QgsLogger::warning( "Error in dlopen" );
  }
  else
  {
    QgsDebugMsg( "dlopen suceeded" );
    dlclose( handle );
  }

#endif

  // load the data provider
  QLibrary* myLib = new QLibrary( lib );

  QgsDebugMsg( "Library name is " + myLib->fileName() );

  bool loaded = myLib->load();

  if ( loaded )
  {
    QgsDebugMsg( "Loaded data provider library" );
    QgsDebugMsg( "Attempting to resolve the classFactory function" );

    classFactoryFunction_t * classFactory =
      ( classFactoryFunction_t * ) cast_to_fptr( myLib->resolve( "classFactory" ) );

    if ( classFactory )
    {
      QgsDebugMsg( "Getting pointer to a dataProvider object from the library" );

      //XXX - This was a dynamic cast but that kills the Windows
      //      version big-time with an abnormal termination error
      //      QgsDataProvider* dataProvider = (QgsDataProvider*)
      //      (classFactory((const char*)(dataSource.utf8())));

      QgsDataProvider * dataProvider = ( *classFactory )( &dataSource );

      if ( dataProvider )
      {
        QgsDebugMsg( "Instantiated the data provider plugin" );
        QgsDebugMsg( "provider name: " + dataProvider->name() );

        if ( dataProvider->isValid() )
        {
          delete myLib;
          return dataProvider;
        }
        else
        {   // this is likely because the dataSource is invalid, and isn't
          // necessarily a reflection on the data provider itself
          QgsDebugMsg( "Invalid data provider" );

          myLib->unload();
          delete myLib;
          return 0;
        }
      }
      else
      {
        QgsLogger::warning( "Unable to instantiate the data provider plugin" );

        myLib->unload();
        delete myLib;
        return 0;
      }
    }
  }
  else
  {
    QgsLogger::warning( "Failed to load " + lib );
    delete myLib;
    return 0;
  }

  QgsDebugMsg( "exiting" );

  return 0;  // factory didn't exist

} // QgsProviderRegistry::setDataProvider





QString QgsProviderRegistry::fileVectorFilters() const
{
  return mVectorFileFilters;
} //  QgsProviderRegistry::fileVectorFilters


QStringList QgsProviderRegistry::providerList() const
{
  QStringList lst;
  for ( Providers::const_iterator it = mProviders.begin(); it != mProviders.end(); it++ )
  {
    lst.append( it->first );
  }
  return lst;
}


const QgsProviderMetadata* QgsProviderRegistry::providerMetadata( const QString& providerKey ) const
{
  return findMetadata_( mProviders, providerKey );
}


/*
QgsDataProvider *
QgsProviderRegistry::openVector( QString const & dataSource, QString const & providerKey )
{
    return getProvider( providerKey, dataSource );
} // QgsProviderRegistry::openVector
*/
