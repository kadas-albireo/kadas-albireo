/***************************************************************************
                          qgsmaplayer.cpp  -  description
                             -------------------
    begin                : Fri Jun 28 2002
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
/* $Id$ */


#include <QDateTime>
#include <QDomNode>
#include <QFileInfo>
#include <QSettings> // TODO: get rid of it [MD]
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDomDocument>
#include <QDomElement>
#include <QDomImplementation>
#include <QTextStream>

#include <sqlite3.h>

#include "qgslogger.h"
#include "qgsrectangle.h"
#include "qgssymbol.h"
#include "qgsmaplayer.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsapplication.h"
#include "qgsproject.h"
#include "qgslogger.h"

QgsMapLayer::QgsMapLayer( QgsMapLayer::LayerType type,
                          QString lyrname,
                          QString source ) :
    mTransparencyLevel( 255 ), // 0 is completely transparent
    mValid( FALSE ), // assume the layer is invalid
    mDataSource( source ),
    mID( "" ),
    mLayerType( type )

{
  QgsDebugMsg( "lyrname is '" + lyrname + "'" );

  mCRS = new QgsCoordinateReferenceSystem();

  // Set the display name = internal name
  mLayerName = capitaliseLayerName( lyrname );
  QgsDebugMsg( "layerName is '" + mLayerName + "'" );

  // Generate the unique ID of this layer
  QDateTime dt = QDateTime::currentDateTime();
  mID = lyrname + dt.toString( "yyyyMMddhhmmsszzz" );
  // Tidy the ID up to avoid characters that may cause problems
  // elsewhere (e.g in some parts of XML). Replaces every non-word
  // character (word characters are the alphabet, numbers and
  // underscore) with an underscore.
  // Note that the first backslashe in the regular expression is
  // there for the compiler, so the pattern is actually \W
  mID.replace( QRegExp( "[\\W]" ), "_" );

  //set some generous  defaults for scale based visibility
  mMinScale = 0;
  mMaxScale = 100000000;
  mScaleBasedVisibility = false;
}



QgsMapLayer::~QgsMapLayer()
{
  delete mCRS;
}

QgsMapLayer::LayerType QgsMapLayer::type() const
{
  return mLayerType;
}

/** Get this layer's unique ID */
QString QgsMapLayer::getLayerID() const
{
  return mID;
}

/** Write property of QString layerName. */
void QgsMapLayer::setLayerName( const QString & _newVal )
{
  QgsDebugMsg( "new name is '" + _newVal + "'" );
  mLayerName = capitaliseLayerName( _newVal );
  emit layerNameChanged();
}

/** Read property of QString layerName. */
QString const & QgsMapLayer::name() const
{
  QgsDebugMsg( "returning name '" + mLayerName + "'" );
  return mLayerName;
}

QString QgsMapLayer::publicSource() const
{
  // Redo this every time we're asked for it, as we don't know if
  // dataSource has changed.
  static QRegExp regexp( " password=.* " );
  regexp.setMinimal( true );
  QString safeName( mDataSource );
  return safeName.replace( regexp, " " );
}

QString const & QgsMapLayer::source() const
{
  return mDataSource;
}

QgsRectangle QgsMapLayer::extent() const
{
  return mLayerExtent;
}

bool QgsMapLayer::draw( QgsRenderContext& rendererContext )
{
  return false;
}

void QgsMapLayer::drawLabels( QgsRenderContext& rendererContext )
{
  // QgsDebugMsg("entered.");
}

bool QgsMapLayer::readXML( QDomNode & layer_node )
{
  QgsCoordinateReferenceSystem savedCRS;
  CUSTOM_CRS_VALIDATION savedValidation;
  bool layerError;

  QDomElement element = layer_node.toElement();

  // XXX not needed? QString type = element.attribute("type");

  // set data source
  QDomNode mnl = layer_node.namedItem( "datasource" );
  QDomElement mne = mnl.toElement();
  mDataSource = mne.text();

  // Set the CRS from project file, asking the user if necessary.
  // Make it the saved CRS to have WMS layer projected correctly.
  // We will still overwrite whatever GDAL etc picks up anyway
  // further down this function.
  QDomNode srsNode = layer_node.namedItem( "srs" );
  mCRS->readXML( srsNode );
  mCRS->validate();
  savedCRS = *mCRS;

  // Do not validate any projections in children, they will be overwritten anyway.
  // No need to ask the user for a projections when it is overwritten, is there?
  savedValidation = QgsCoordinateReferenceSystem::customSrsValidation();
  QgsCoordinateReferenceSystem::setCustomSrsValidation( NULL );

  // now let the children grab what they need from the Dom node.
  layerError = !readXml( layer_node );

  // overwrite CRS with what we read from project file before the raster/vector
  // file readnig functions changed it. They will if projections is specfied in the file.
  // FIXME: is this necessary?
  QgsCoordinateReferenceSystem::setCustomSrsValidation( savedValidation );
  *mCRS = savedCRS;

  // Abort if any error in layer, such as not found.
  if ( layerError )
  {
    return false;
  }

  // the internal name is just the data source basename
  QFileInfo dataSourceFileInfo( mDataSource );
  //internalName = dataSourceFileInfo.baseName();

  // set ID
  mnl = layer_node.namedItem( "id" );
  if ( ! mnl.isNull() )
  {
    mne = mnl.toElement();
    if ( ! mne.isNull() && mne.text().length() > 10 ) // should be at least 17 (yyyyMMddhhmmsszzz)
    {
      mID = mne.text();
    }
  }

  // use scale dependent visibility flag
  QString hasScaleBasedVisibility = element.attribute( "hasScaleBasedVisibilityFlag" );
  if ( "1" == hasScaleBasedVisibility )
  {
    toggleScaleBasedVisibility( true );
  }
  else
  {
    toggleScaleBasedVisibility( false );
  }
  setMinimumScale( element.attribute( "minimumScale" ).toFloat() );
  setMaximumScale( element.attribute( "maximumScale" ).toFloat() );

  // set name
  mnl = layer_node.namedItem( "layername" );
  mne = mnl.toElement();
  setLayerName( mne.text() );

  //read transparency level
  QDomNode transparencyNode = layer_node.namedItem( "transparencyLevelInt" );
  if ( ! transparencyNode.isNull() )
  {
    // set transparency level only if it's in project
    // (otherwise it sets the layer transparent)
    QDomElement myElement = transparencyNode.toElement();
    setTransparency( myElement.text().toInt() );
  }

  return true;
} // void QgsMapLayer::readXML


bool QgsMapLayer::readXml( QDomNode & layer_node )
{
  // NOP by default; children will over-ride with behavior specific to them

  return true;
} // void QgsMapLayer::readXml



bool QgsMapLayer::writeXML( QDomNode & layer_node, QDomDocument & document )
{
  // general layer metadata
  QDomElement maplayer = document.createElement( "maplayer" );

  // use scale dependent visibility flag
  if ( hasScaleBasedVisibility() )
  {
    maplayer.setAttribute( "hasScaleBasedVisibilityFlag", 1 );
  }
  else
  {
    maplayer.setAttribute( "hasScaleBasedVisibilityFlag", 0 );
  }
  maplayer.setAttribute( "minimumScale", minimumScale() );
  maplayer.setAttribute( "maximumScale", maximumScale() );

  // ID
  QDomElement id = document.createElement( "id" );
  QDomText idText = document.createTextNode( getLayerID() );
  id.appendChild( idText );

  maplayer.appendChild( id );

  // data source
  QDomElement dataSource = document.createElement( "datasource" );
  QDomText dataSourceText = document.createTextNode( source() );
  dataSource.appendChild( dataSourceText );

  maplayer.appendChild( dataSource );


  // layer name
  QDomElement layerName = document.createElement( "layername" );
  QDomText layerNameText = document.createTextNode( name() );
  layerName.appendChild( layerNameText );

  maplayer.appendChild( layerName );

  // zorder
  // This is no longer stored in the project file. It is superflous since the layers
  // are written and read in the proper order.

  // spatial reference system id
  QDomElement mySrsElement = document.createElement( "srs" );
  mCRS->writeXML( mySrsElement, document );
  maplayer.appendChild( mySrsElement );

  // <transparencyLevelInt>
  QDomElement transparencyLevelIntElement = document.createElement( "transparencyLevelInt" );
  QDomText    transparencyLevelIntText    = document.createTextNode( QString::number( getTransparency() ) );
  transparencyLevelIntElement.appendChild( transparencyLevelIntText );
  maplayer.appendChild( transparencyLevelIntElement );
  // now append layer node to map layer node

  layer_node.appendChild( maplayer );

  return writeXml( maplayer, document );

} // bool QgsMapLayer::writeXML



bool QgsMapLayer::writeXml( QDomNode & layer_node, QDomDocument & document )
{
  // NOP by default; children will over-ride with behavior specific to them

  return true;
} // void QgsMapLayer::writeXml




bool QgsMapLayer::isValid()
{
  return mValid;
}


void QgsMapLayer::invalidTransformInput()
{
  QgsLogger::warning( "QgsMapLayer::invalidTransformInput() called" );
  // TODO: emit a signal - it will be used to update legend
}


QString QgsMapLayer::lastErrorTitle()
{
  return QString();
}

QString QgsMapLayer::lastError()
{
  return QString();
}

void QgsMapLayer::connectNotify( const char * signal )
{
  QgsDebugMsg( "QgsMapLayer connected to " + QString( signal ) );
} //  QgsMapLayer::connectNotify



void QgsMapLayer::toggleScaleBasedVisibility( bool theVisibilityFlag )
{
  mScaleBasedVisibility = theVisibilityFlag;
}

bool QgsMapLayer::hasScaleBasedVisibility()
{
  return mScaleBasedVisibility;
}

void QgsMapLayer::setMinimumScale( float theMinScale )
{
  mMinScale = theMinScale;
}

float QgsMapLayer::minimumScale()
{
  return mMinScale;
}


void QgsMapLayer::setMaximumScale( float theMaxScale )
{
  mMaxScale = theMaxScale;
}

float QgsMapLayer::maximumScale()
{
  return mMaxScale;
}


QStringList QgsMapLayer::subLayers()
{
  return QStringList();  // Empty
}

void QgsMapLayer::setLayerOrder( QStringList layers )
{
  // NOOP
}

void QgsMapLayer::setSubLayerVisibility( QString name, bool vis )
{
  // NOOP
}

const QgsCoordinateReferenceSystem& QgsMapLayer::srs()
{
  return *mCRS;
}

void QgsMapLayer::setCrs( const QgsCoordinateReferenceSystem& srs )
{
  *mCRS = srs;
}

unsigned int QgsMapLayer::getTransparency()
{
  return mTransparencyLevel;
}

void QgsMapLayer::setTransparency( unsigned int theInt )
{
  mTransparencyLevel = theInt;
}

QString QgsMapLayer::capitaliseLayerName( const QString name )
{
  // Capitalise the first letter of the layer name if requested
  QSettings settings;
  bool capitaliseLayerName =
    settings.value( "qgis/capitaliseLayerName", QVariant( false ) ).toBool();

  QString layerName( name );

  if ( capitaliseLayerName )
    layerName = layerName.left( 1 ).toUpper() + layerName.mid( 1 );

  return layerName;
}

QString QgsMapLayer::loadDefaultStyle( bool & theResultFlag )
{
  QString myURI = publicSource();
  QFileInfo myFileInfo( myURI );
  QString key;
  if ( myFileInfo.exists() )
  {
    // get the file name for our .qml style file
    key = myFileInfo.path() + QDir::separator() + myFileInfo.completeBaseName() + ".qml";
  }
  else
  {
    key = myURI;
  }
  return loadNamedStyle( key, theResultFlag );
}

bool QgsMapLayer::loadNamedStyleFromDb( const QString db, const QString theURI, QString &qml )
{
  bool theResultFlag = false;

  // read from database
  sqlite3 *myDatabase;
  sqlite3_stmt *myPreparedStatement;
  const char *myTail;
  int myResult;

  QgsDebugMsg( QString( "Trying to load style for \"%1\" from \"%2\"" ).arg( theURI ).arg( db ) );

  if ( !QFile( db ).exists() )
    return false;

  myResult = sqlite3_open( db.toUtf8().data(), &myDatabase );
  if ( myResult != SQLITE_OK )
  {
    return false;
  }

  QString mySql = "select qml from tbl_styles where style=?";
  myResult = sqlite3_prepare( myDatabase, mySql.toUtf8().data(), mySql.length(), &myPreparedStatement, &myTail );
  if ( myResult == SQLITE_OK )
  {
    QByteArray param = theURI.toUtf8();

    if ( sqlite3_bind_text( myPreparedStatement, 1, param.data(), param.length(), SQLITE_STATIC ) == SQLITE_OK &&
         sqlite3_step( myPreparedStatement ) == SQLITE_ROW )
    {
      qml = QString::fromUtf8(( char * )sqlite3_column_text( myPreparedStatement, 0 ) );
      theResultFlag = true;
    }

    sqlite3_finalize( myPreparedStatement );
  }

  sqlite3_close( myDatabase );

  return theResultFlag;
}

QString QgsMapLayer::loadNamedStyle( const QString theURI, bool &theResultFlag )
{
  theResultFlag = false;

  QDomDocument myDocument( "qgis" );

  // location of problem associated with errorMsg
  int line, column;
  QString myErrorMessage;

  QFile myFile( theURI );
  if ( myFile.open( QFile::ReadOnly ) )
  {
    // read file
    theResultFlag = myDocument.setContent( &myFile, &myErrorMessage, &line, &column );
    if ( !theResultFlag )
      myErrorMessage = tr( "%1 at line %2 column %3" ).arg( myErrorMessage ).arg( line ).arg( column );
    myFile.close();
  }
  else
  {
    QFileInfo project( QgsProject::instance()->fileName() );
    QgsDebugMsg( QString( "project fileName: %1" ).arg( project.absoluteFilePath() ) );

    QString qml;
    if ( loadNamedStyleFromDb( QDir( QgsApplication::qgisSettingsDirPath() ).absoluteFilePath( "qgis.qmldb" ), theURI, qml ) ||
         ( project.exists() && loadNamedStyleFromDb( project.absoluteDir().absoluteFilePath( project.baseName() + ".qmldb" ), theURI, qml ) ) ||
         loadNamedStyleFromDb( QDir( QgsApplication::pkgDataPath() ).absoluteFilePath( "resources/qgis.qmldb" ), theURI, qml ) )
    {
      theResultFlag = myDocument.setContent( qml, &myErrorMessage, &line, &column );
      if ( !theResultFlag )
      {
        myErrorMessage = tr( "%1 at line %2 column %3" ).arg( myErrorMessage ).arg( line ).arg( column );
      }
    }
    else
    {
      myErrorMessage = tr( "style not found in database" );
    }
  }

  if ( !theResultFlag )
  {
    return myErrorMessage;
  }

  // now get the layer node out and pass it over to the layer
  // to deserialise...
  QDomElement myRoot = myDocument.firstChildElement( "qgis" );
  if ( myRoot.isNull() )
  {
    myErrorMessage = "Error: qgis element could not be found in " + theURI;
    theResultFlag = false;
    return myErrorMessage;
  }

  QString errorMsg;
  theResultFlag = readSymbology( myRoot, errorMsg );
  if ( !theResultFlag )
  {
    myErrorMessage = QObject::tr( "Loading style file " ) + theURI + QObject::tr( " failed because:" ) + "\n" + errorMsg;
    return myErrorMessage;
  }

  return "";
}

QString QgsMapLayer::saveDefaultStyle( bool & theResultFlag )
{
  return saveNamedStyle( publicSource(), theResultFlag );
}

QString QgsMapLayer::saveNamedStyle( const QString theURI, bool & theResultFlag )
{
  QString myErrorMessage;

  QDomImplementation DomImplementation;
  QDomDocumentType documentType =
    DomImplementation.createDocumentType(
      "qgis", "http://mrcc.com/qgis.dtd", "SYSTEM" );
  QDomDocument myDocument( documentType );
  QDomElement myRootNode = myDocument.createElement( "qgis" );
  myRootNode.setAttribute( "version", QString( "%1" ).arg( QGis::QGIS_VERSION ) );
  myDocument.appendChild( myRootNode );

  QString errorMsg;
  if ( !writeSymbology( myRootNode, myDocument, errorMsg ) )
  {
    return QObject::tr( "Could not save symbology because:" ) + "\n" + errorMsg;
  }

  // check if the uri is a file or ends with .qml,
  // which indicates that it should become one
  // everything else goes to the database.
  QFileInfo myFileInfo( theURI );
  if ( myFileInfo.exists() || theURI.endsWith( ".qml", Qt::CaseInsensitive ) )
  {
    QFileInfo myDirInfo( myFileInfo.path() );  //excludes file name
    if ( !myDirInfo.isWritable() )
    {
      return QObject::tr( "The directory containing your dataset needs to be writeable!" );
    }

    // now construct the file name for our .qml style file
    QString myFileName = myFileInfo.path() + QDir::separator() + myFileInfo.completeBaseName() + ".qml";

    QFile myFile( myFileName );
    if ( myFile.open( QFile::WriteOnly | QFile::Truncate ) )
    {
      QTextStream myFileStream( &myFile );
      // save as utf-8 with 2 spaces for indents
      myDocument.save( myFileStream, 2 );
      myFile.close();
      theResultFlag = true;
      return QObject::tr( "Created default style file as " ) + myFileName;
    }
    else
    {
      theResultFlag = false;
      return QObject::tr( "ERROR: Failed to created default style file as %1 Check file permissions and retry." ).arg( myFileName );
    }
  }
  else
  {
    QString qml = myDocument.toString();

    // read from database
    sqlite3 *myDatabase;
    sqlite3_stmt *myPreparedStatement;
    const char *myTail;
    int myResult;

    myResult = sqlite3_open( QDir( QgsApplication::qgisSettingsDirPath() ).absoluteFilePath( "qgis.qmldb" ).toUtf8().data(), &myDatabase );
    if ( myResult != SQLITE_OK )
    {
      return tr( "User database could not be opened." );
    }

    QByteArray param0 = theURI.toUtf8();
    QByteArray param1 = qml.toUtf8();

    QString mySql = "create table if not exists tbl_styles(style varchar primary key,qml varchar)";
    myResult = sqlite3_prepare( myDatabase, mySql.toUtf8().data(), mySql.length(), &myPreparedStatement, &myTail );
    if ( myResult == SQLITE_OK )
    {
      if ( sqlite3_step( myPreparedStatement ) != SQLITE_DONE )
      {
        sqlite3_finalize( myPreparedStatement );
        sqlite3_close( myDatabase );
        theResultFlag = false;
        return tr( "The style table could not be created." );
      }
    }

    sqlite3_finalize( myPreparedStatement );

    mySql = "insert into tbl_styles(style,qml) values (?,?)";
    myResult = sqlite3_prepare( myDatabase, mySql.toUtf8().data(), mySql.length(), &myPreparedStatement, &myTail );
    if ( myResult == SQLITE_OK )
    {
      if ( sqlite3_bind_text( myPreparedStatement, 1, param0.data(), param0.length(), SQLITE_STATIC ) == SQLITE_OK &&
           sqlite3_bind_text( myPreparedStatement, 2, param1.data(), param1.length(), SQLITE_STATIC ) == SQLITE_OK &&
           sqlite3_step( myPreparedStatement ) == SQLITE_DONE )
      {
        theResultFlag = true;
        myErrorMessage = tr( "The style %1 was saved to database" ).arg( theURI );
      }
    }

    sqlite3_finalize( myPreparedStatement );

    if ( !theResultFlag )
    {
      QString mySql = "update tbl_styles set qml=? where style=?";
      myResult = sqlite3_prepare( myDatabase, mySql.toUtf8().data(), mySql.length(), &myPreparedStatement, &myTail );
      if ( myResult == SQLITE_OK )
      {
        if ( sqlite3_bind_text( myPreparedStatement, 2, param0.data(), param0.length(), SQLITE_STATIC ) == SQLITE_OK &&
             sqlite3_bind_text( myPreparedStatement, 1, param1.data(), param1.length(), SQLITE_STATIC ) == SQLITE_OK &&
             sqlite3_step( myPreparedStatement ) == SQLITE_DONE )
        {
          theResultFlag = true;
          myErrorMessage = tr( "The style %1 was updated in the database." ).arg( theURI );
        }
        else
        {
          theResultFlag = false;
          myErrorMessage = tr( "The style %1 could not be updated in the database." ).arg( theURI );
        }
      }
      else
      {
        // try an update
        theResultFlag = false;
        myErrorMessage = tr( "The style %1 could not be inserted into database." ).arg( theURI );
      }

      sqlite3_finalize( myPreparedStatement );
    }

    sqlite3_close( myDatabase );
  }

  return myErrorMessage;
}
