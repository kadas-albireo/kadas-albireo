/***************************************************************************
                              qgs_map_serv.cpp
 A server application supporting WMS/SLD syntax for HTTP GET and Orchestra
map service syntax for SOAP/HTTP POST
                              -------------------
  begin                : July 04, 2006
  copyright            : (C) 2006 by Marco Hugentobler & Ionut Iosifescu Enescu
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsapplication.h"
#include "qgscapabilitiescache.h"
#include "qgsconfigcache.h"
#include "qgsgetrequesthandler.h"
#include "qgssoaprequesthandler.h"
#include "qgsproviderregistry.h"
#include "qgsmapserverlogger.h"
#include "qgswmsserver.h"
#include "qgsmaprenderer.h"
#include "qgsmapserviceexception.h"
#include "qgsprojectparser.h"
#include "qgssldparser.h"
#include <QDomDocument>
#include <QImage>
#include <QSettings>
#include <QDateTime>

//for CMAKE_INSTALL_PREFIX
#include "qgsconfig.h"

#include <fcgi_stdio.h>


void dummyMessageHandler( QtMsgType type, const char *msg )
{
#ifdef QGSMSDEBUG
  QString output;

  switch ( type )
  {
    case QtDebugMsg:
      output += "Debug: ";
      break;
    case QtCriticalMsg:
      output += "Critical: ";
      break;
    case QtWarningMsg:
      output += "Warning: ";
      break;
    case QtFatalMsg:
      output += "Fatal: ";
  }

  output += msg;

  QgsMapServerLogger::instance()->printMessage( output );

  if ( type == QtFatalMsg )
    abort();
#endif
}

void printRequestInfos()
{
#ifdef QGSMSDEBUG
  //print out some infos about the request
  QgsMSDebugMsg( "************************new request**********************" );
  QgsMSDebugMsg( QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss" ) );

  if ( getenv( "REMOTE_ADDR" ) != NULL )
  {
    QgsMSDebugMsg( "remote ip: " + QString( getenv( "REMOTE_ADDR" ) ) );
  }
  if ( getenv( "REMOTE_HOST" ) != NULL )
  {
    QgsMSDebugMsg( "remote host: " + QString( getenv( "REMOTE_HOST" ) ) );
  }
  if ( getenv( "REMOTE_USER" ) != NULL )
  {
    QgsMSDebugMsg( "remote user: " + QString( getenv( "REMOTE_USER" ) ) );
  }
  if ( getenv( "REMOTE_IDENT" ) != NULL )
  {
    QgsMSDebugMsg( "REMOTE_IDENT: " + QString( getenv( "REMOTE_IDENT" ) ) );
  }
  if ( getenv( "CONTENT_TYPE" ) != NULL )
  {
    QgsMSDebugMsg( "CONTENT_TYPE: " + QString( getenv( "CONTENT_TYPE" ) ) );
  }
  if ( getenv( "AUTH_TYPE" ) != NULL )
  {
    QgsMSDebugMsg( "AUTH_TYPE: " + QString( getenv( "AUTH_TYPE" ) ) );
  }
  if ( getenv( "HTTP_USER_AGENT" ) != NULL )
  {
    QgsMSDebugMsg( "HTTP_USER_AGENT: " + QString( getenv( "HTTP_USER_AGENT" ) ) );
  }
#endif //QGSMSDEBUG
}

QFileInfo defaultProjectFile()
{
  QDir currentDir;
  QgsMSDebugMsg( "current directory: " + currentDir.absolutePath() );
  fprintf( FCGI_stderr, "current directory: %s\n", currentDir.absolutePath().toUtf8().constData() );
  QStringList nameFilterList;
  nameFilterList << "*.qgs";
  QFileInfoList projectFiles = currentDir.entryInfoList( nameFilterList, QDir::Files, QDir::Name );
  if ( projectFiles.size() < 1 )
  {
    return QFileInfo();
  }
  return projectFiles.at( 0 );
}

QFileInfo defaultAdminSLD()
{
  return QFileInfo( "admin.sld" );
}

int fcgi_accept()
{
#ifdef Q_OS_WIN
  if ( FCGX_IsCGI() )
    return FCGI_Accept();
  else
    return FCGX_Accept( &FCGI_stdin->fcgx_stream, &FCGI_stdout->fcgx_stream, &FCGI_stderr->fcgx_stream, &environ );
#else
  return FCGI_Accept();
#endif
}

int main( int argc, char * argv[] )
{
#ifndef _MSC_VER
  qInstallMsgHandler( dummyMessageHandler );
#endif

  QgsApplication qgsapp( argc, argv, false );

  //Default prefix path may be altered by environment variable
  char* prefixPath = getenv( "QGIS_PREFIX_PATH" );
  if ( prefixPath )
  {
    QgsApplication::setPrefixPath( prefixPath, TRUE );
  }
#if !defined(Q_OS_WIN)
  else
  {
    // init QGIS's paths - true means that all path will be inited from prefix
    QgsApplication::setPrefixPath( CMAKE_INSTALL_PREFIX, TRUE );
  }
#endif

  // Instantiate the plugin directory so that providers are loaded
  QgsProviderRegistry::instance( QgsApplication::pluginPath() );
#if defined(QGSMSDEBUG) && !defined(_MSC_VER)
  //write to qgis_wms_server.log in application directory
  QgsMapServerLogger::instance()->setLogFilePath( qgsapp.applicationDirPath() + "/qgis_wms_server.log" );
#endif
  QgsMSDebugMsg( "Prefix  PATH: " + QgsApplication::prefixPath() );
  QgsMSDebugMsg( "Plugin  PATH: " + QgsApplication::pluginPath() );
  QgsMSDebugMsg( "PkgData PATH: " + QgsApplication::pkgDataPath() );
  QgsMSDebugMsg( "User DB PATH: " + QgsApplication::qgisUserDbFilePath() );

  QgsMSDebugMsg( qgsapp.applicationDirPath() + "/qgis_wms_server.log" );

  //create config cache and search for config files in the current directory.
  //These configurations are used if no mapfile parameter is present in the request
  QgsConfigCache configCache;
  QString defaultConfigFilePath;
  QFileInfo projectFileInfo = defaultProjectFile(); //try to find a .qgs file in the server directory
  if ( projectFileInfo.exists() )
  {
    defaultConfigFilePath = projectFileInfo.absoluteFilePath();
  }
  else
  {
    QFileInfo adminSLDFileInfo = defaultAdminSLD();
    if ( adminSLDFileInfo.exists() )
    {
      defaultConfigFilePath = adminSLDFileInfo.absoluteFilePath();
    }
  }

  //create cache for capabilities XML
  QgsCapabilitiesCache capabilitiesCache;

  //creating QgsMapRenderer is expensive (access to srs.db), so we do it here before the fcgi loop
  QgsMapRenderer* theMapRenderer = new QgsMapRenderer();

  while ( fcgi_accept() >= 0 )
  {
    printRequestInfos(); //print request infos if in debug mode

    //use QgsGetRequestHandler in case of HTTP GET and QgsSOAPRequestHandler in case of HTTP POST
    QgsRequestHandler* theRequestHandler = 0;
    char* requestMethod = getenv( "REQUEST_METHOD" );
    if ( requestMethod != NULL )
    {
      if ( strcmp( requestMethod, "POST" ) == 0 )
      {
        QgsMSDebugMsg( "Creating QgsSOAPRequestHandler" );
        theRequestHandler = new QgsSOAPRequestHandler();
      }
      else
      {
        QgsMSDebugMsg( "Creating QgsGetRequestHandler" );
        theRequestHandler = new QgsGetRequestHandler();
      }
    }
    else
    {
      QgsMSDebugMsg( "Creating QgsGetRequestHandler" );
      theRequestHandler = new QgsGetRequestHandler();
    }

    std::map<QString, QString> parameterMap;

    try
    {
      parameterMap = theRequestHandler->parseInput();
    }
    catch ( QgsMapServiceException& e )
    {
      QgsMSDebugMsg( "An exception was thrown during input parsing" );
      theRequestHandler->sendServiceException( e );
      continue;
    }

    //set admin config file to wms server object
    QString configFilePath = defaultConfigFilePath;
    std::map<QString, QString>::const_iterator mapFileIt = parameterMap.find( "MAP" );
    if ( mapFileIt != parameterMap.end() )
    {
      configFilePath = mapFileIt->second;
    }

    QgsConfigParser* adminConfigParser = configCache.searchConfiguration( configFilePath );
    if ( !adminConfigParser )
    {
      QgsMSDebugMsg( "parse error on config file " + configFilePath );
      theRequestHandler->sendServiceException( QgsMapServiceException( "", "Configuration file problem" ) );
      continue;
    }

    //sld parser might need information about request parameters
    adminConfigParser->setParameterMap( parameterMap );

    //request to WMS?
    std::map<QString, QString>::const_iterator serviceIt = parameterMap.find( "SERVICE" );
    if ( serviceIt == parameterMap.end() )
    {
      //tell the user that service parameter is mandatory
      QgsMSDebugMsg( "unable to find 'SERVICE' parameter, exiting..." );
      theRequestHandler->sendServiceException( QgsMapServiceException( "ServiceNotSpecified", "Service not specified. The SERVICE parameter is mandatory" ) );
      delete theRequestHandler;
      continue;
    }

    QgsWMSServer* theServer = 0;
    try
    {
      theServer = new QgsWMSServer( parameterMap, theMapRenderer );
    }
    catch ( QgsMapServiceException e ) //admin.sld may be invalid
    {
      theRequestHandler->sendServiceException( e );
      continue;
    }

    theServer->setAdminConfigParser( adminConfigParser );


    //request type
    std::map<QString, QString>::const_iterator requestIt = parameterMap.find( "REQUEST" );
    if ( requestIt == parameterMap.end() )
    {
      //do some error handling
      QgsMSDebugMsg( "unable to find 'REQUEST' parameter, exiting..." );
      theRequestHandler->sendServiceException( QgsMapServiceException( "OperationNotSupported", "Please check the value of the REQUEST parameter" ) );
      delete theRequestHandler;
      delete theServer;
      continue;
    }

    if ( requestIt->second == "GetCapabilities" )
    {
      const QDomDocument* capabilitiesDocument = capabilitiesCache.searchCapabilitiesDocument( configFilePath );
      if( !capabilitiesDocument ) //capabilities xml not in cache. Create a new one
      {
        QgsMSDebugMsg( "Capabilities document not found in cache" );
        QDomDocument doc;
        try
        {
          doc = theServer->getCapabilities();
        }
        catch ( QgsMapServiceException& ex )
        {
          theRequestHandler->sendServiceException( ex );
          delete theRequestHandler;
          delete theServer;
          continue;
        }
        capabilitiesCache.insertCapabilitiesDocument( configFilePath, &doc );
        capabilitiesDocument = capabilitiesCache.searchCapabilitiesDocument( configFilePath );
      }
      else
      {
        QgsMSDebugMsg( "Found capabilities document in cache" );
      }

      if( capabilitiesDocument )
      {
        theRequestHandler->sendGetCapabilitiesResponse( *capabilitiesDocument );
      }
      delete theRequestHandler;
      delete theServer;
      continue;
    }
    else if ( requestIt->second == "GetMap" )
    {
      QImage* result = 0;
      try
      {
        result = theServer->getMap();
      }
      catch ( QgsMapServiceException& ex )
      {
        QgsMSDebugMsg( "Catched exception during GetMap request" );
        theRequestHandler->sendServiceException( ex );
        delete theRequestHandler;
        delete theServer;
        continue;
      }

      if ( result )
      {
        QgsMSDebugMsg( "Sending GetMap response" );
        theRequestHandler->sendGetMapResponse( serviceIt->second, result );
      }
      else
      {
        //do some error handling
        QgsMSDebugMsg( "result image is 0" );
      }
      delete result;
      delete theRequestHandler;
      delete theServer;
      continue;
    }
    else if ( requestIt->second == "GetFeatureInfo" )
    {
      QDomDocument featureInfoDoc;
      try
      {
        if ( theServer->getFeatureInfo( featureInfoDoc ) != 0 )
        {
          delete theRequestHandler;
          delete theServer;
          continue;
        }
      }
      catch ( QgsMapServiceException& ex )
      {
        theRequestHandler->sendServiceException( ex );
        delete theRequestHandler;
        delete theServer;
        continue;
      }

      //info format for GetFeatureInfo
      QString infoFormat;
      std::map<QString, QString>::const_iterator p_it = parameterMap.find( "INFO_FORMAT" );
      if ( p_it != parameterMap.end() )
      {
        infoFormat = p_it->second;
      }

      theRequestHandler->sendGetFeatureInfoResponse( featureInfoDoc, infoFormat );
      delete theRequestHandler;
      delete theServer;
      continue;
    }
    else if ( requestIt->second == "GetStyle" )
    {
      try
      {
        QDomDocument doc = theServer->getStyle();
        theRequestHandler->sendGetStyleResponse( doc );
      }
      catch ( QgsMapServiceException& ex )
      {
        theRequestHandler->sendServiceException( ex );
      }

      delete theRequestHandler;
      delete theServer;
      continue;
    }
    else if ( requestIt->second == "GetLegendGraphics" )
    {
      QImage* result = 0;
      try
      {
        result = theServer->getLegendGraphics();
      }
      catch ( QgsMapServiceException& ex )
      {
        theRequestHandler->sendServiceException( ex );
      }

      if ( result )
      {
        QgsMSDebugMsg( "Sending GetLegendGraphics response" );
        //sending is the same for GetMap and GetLegendGraphics
        theRequestHandler->sendGetMapResponse( serviceIt->second, result );
      }
      else
      {
        //do some error handling
        QgsMSDebugMsg( "result image is 0" );
      }
      delete result;
      delete theRequestHandler;
      delete theServer;
      continue;

    }
    else if ( requestIt->second == "GetPrint" )
    {
      QByteArray* printOutput = 0;
      try
      {
        printOutput = theServer->getPrint( theRequestHandler->format() );
      }
      catch ( QgsMapServiceException& ex )
      {
        theRequestHandler->sendServiceException( ex );
      }

      if ( printOutput )
      {
        theRequestHandler->sendGetPrintResponse( printOutput );
      }
      delete printOutput;
      delete theRequestHandler;
      delete theServer;
      continue;
    }
    else//unknown request
    {
      QgsMapServiceException e( "OperationNotSupported", "Operation " + requestIt->second + " not supported" );
      theRequestHandler->sendServiceException( e );
      delete theRequestHandler;
      delete theServer;
    }
  }

  delete theMapRenderer;
  return 0;
}


