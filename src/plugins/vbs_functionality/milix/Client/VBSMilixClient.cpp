#include "VBSMilixClient.hpp"
#include "../Server/VBSMilixCommands.hpp"
#include <QApplication>
#include <QDataStream>
#include <QEventLoop>
#include <QHostAddress>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>
#include <QImage>
#include <QProcess>
#include <QSettings>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <rsvgrenderer.h>

VBSMilixClientWorker::VBSMilixClientWorker( QObject* parent )
    : QObject( parent ), mProcess( 0 ), mNetworkSession( 0 ), mTcpSocket( 0 )
{
}

void VBSMilixClientWorker::cleanup()
{
  if ( mProcess )
    mProcess->deleteLater();
  mProcess = 0;
  if ( mTcpSocket )
    mTcpSocket->deleteLater();
  mTcpSocket = 0;
  delete mNetworkSession;
  mNetworkSession = 0;
}

bool VBSMilixClientWorker::initialize()
{
  if ( mTcpSocket )
  {
    return true;
  }

  cleanup();
  mLastError = QString();

  // Start process
#ifdef Q_OS_WIN
  int port;
  QHostAddress addr( QHostAddress::LocalHost );
  mProcess = new QProcess( this );
  connect( mProcess, SIGNAL( finished( int ) ), this, SLOT( cleanup() ) );
  {
    mProcess->start( "milixserver" );
    mProcess->waitForReadyRead( 5000 );
    QByteArray out = mProcess->readAllStandardOutput();
    if ( !mProcess->isOpen() )
    {
      cleanup();
      mLastError = tr( "Process failed to start: %1" ).arg( mProcess->errorString() );
      return false;
    }
    else if ( out.isEmpty() )
    {
      cleanup();
      mLastError = tr( "Could not determine process port" );
      return false;
    }
    port = QString( out ).toInt();
  }
#else
  int port = atoi( qgetenv( "MILIX_SERVER_PORT" ) );
  QHostAddress addr = QHostAddress( QString( qgetenv( "MILIX_SERVER_ADDR" ) ) );
#endif

  // Initialize network
  QNetworkConfigurationManager manager;
  if ( manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired )
  {
    // Get saved network configuration
    QSettings settings( QSettings::UserScope, QLatin1String( "QtProject" ) );
    settings.beginGroup( QLatin1String( "QtNetwork" ) );
    QString id = settings.value( QLatin1String( "DefaultNetworkConfiguration" ) ).toString();
    settings.endGroup();

    // If the saved network configuration is not currently discovered use the system default
    QNetworkConfiguration config = manager.configurationFromIdentifier( id );
    if (( config.state() & QNetworkConfiguration::Discovered ) !=
        QNetworkConfiguration::Discovered )
    {
      config = manager.defaultConfiguration();
    }

    mNetworkSession = new QNetworkSession( config, this );
    QEventLoop evLoop;
    connect( mNetworkSession, SIGNAL( opened() ), &evLoop, SLOT( quit() ) );
    mNetworkSession->open();
    evLoop.exec();

    // Save the used configuration
    config = mNetworkSession->configuration();
    if ( config.type() == QNetworkConfiguration::UserChoice )
      id = mNetworkSession->sessionProperty( QLatin1String( "UserChoiceConfiguration" ) ).toString();
    else
      id = config.identifier();

    settings.beginGroup( QLatin1String( "QtNetwork" ) );
    settings.setValue( QLatin1String( "DefaultNetworkConfiguration" ), id );
    settings.endGroup();
  }

  // Connect to server
  mTcpSocket = new QTcpSocket( this );
  QTimer timeoutTimer;
  timeoutTimer.setSingleShot( true );
  connect( mTcpSocket, SIGNAL( disconnected() ), this, SLOT( cleanup() ) );
  connect( mTcpSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( handleSocketError() ) );
  {
    QEventLoop evLoop;
    connect( mTcpSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), &evLoop, SLOT( quit() ) );
    connect( mTcpSocket, SIGNAL( disconnected() ), &evLoop, SLOT( quit() ) );
    connect( mTcpSocket, SIGNAL( connected() ), &evLoop, SLOT( quit() ) );
    connect( &timeoutTimer, SIGNAL( timeout() ), &evLoop, SLOT( quit() ) );
    mTcpSocket->connectToHost( addr, port );
    timeoutTimer.start( 1000 );
    evLoop.exec();
  }
  QTextStream( stdout ) << mTcpSocket->state() << " : " << mLastError << endl;
  if ( mTcpSocket->state() != QTcpSocket::ConnectedState )
  {
    mTcpSocket->abort();
    cleanup();
    return false;
  }

  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  QString lang = QSettings().value( "/locale/currentLang", "en" ).toString().left( 2 ).toUpper();
  istream << VBS_MILIX_REQUEST_INIT;
  istream << lang;
  QByteArray response;
  if ( !processRequest( request, response, VBS_MILIX_REPLY_INIT_OK ) )
  {
    cleanup();
    return false;
  }
  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> mLibraryVersionTag;

  return true;
}

bool VBSMilixClientWorker::getCurrentLibraryVersionTag( QString& versionTag )
{
  if ( initialize() )
  {
    versionTag = mLibraryVersionTag;
    return true;
  }
  return false;
}

bool VBSMilixClientWorker::processRequest( const QByteArray& request, QByteArray& response, quint8 expectedReply )
{
  mLastError = QString();

  if ( !mTcpSocket && !initialize() )
  {
    mLastError = tr( "Connection failed" );
    return false;
  }

  int requiredSize = 0;
  response.clear();

  qint32 len = request.size();
  mTcpSocket->write( reinterpret_cast<char*>( &len ), sizeof( quint32 ) );
  mTcpSocket->write( request );

  do
  {
    mTcpSocket->waitForReadyRead();

    if ( !mLastError.isEmpty() || !mTcpSocket->isValid() )
    {
      return false;
    }
    response += mTcpSocket->readAll();
    if ( requiredSize == 0 )
    {
      requiredSize = *reinterpret_cast<qint32*>( response.left( sizeof( qint32 ) ).data() );
      response = response.mid( sizeof( qint32 ) );
    }
  }
  while ( response.size() < requiredSize );
  Q_ASSERT( mTcpSocket->bytesAvailable() == 0 );

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  if ( replycmd == VBS_MILIX_REPLY_ERROR )
  {
    ostream >> mLastError;
    return false;
  }
  else if ( replycmd != expectedReply )
  {
    mLastError = tr( "Unexpected reply" );
    return false;
  }
  return true;
}

void VBSMilixClientWorker::handleSocketError()
{
  if ( !mTcpSocket )
  {
    return;
  }
  QTcpSocket::SocketError socketError = mTcpSocket->error();

  switch ( socketError )
  {
    case QAbstractSocket::RemoteHostClosedError:
      mLastError = tr( "Connection closed" );
      break;
    case QAbstractSocket::HostNotFoundError:
      mLastError = tr( "Could not find specified host" );
      break;
    case QAbstractSocket::ConnectionRefusedError:
      mLastError = tr( "Connection refused" );
      break;
    default:
      mLastError = tr( "An error occured: %1" ).arg( mTcpSocket->errorString() );
  }
}

///////////////////////////////////////////////////////////////////////////////

VBSMilixClient::VBSMilixClient()
{
  start();
  QEventLoop loop;
  connect( this, SIGNAL( started() ), &loop, SLOT( quit() ) );
  loop.exec( QEventLoop::ExcludeUserInputEvents );
  mWorker.moveToThread( this );
}

VBSMilixClient::~VBSMilixClient()
{
  quit();
  wait();
}

bool VBSMilixClient::processRequest( const QByteArray& request, QByteArray& response, VBSMilixServerReply expectedReply )
{
  bool result;
  QMetaObject::invokeMethod( &mWorker, "processRequest", Qt::BlockingQueuedConnection, Q_RETURN_ARG( bool, result ), Q_ARG( const QByteArray&, request ), Q_ARG( QByteArray&, response ), Q_ARG( quint8, expectedReply ) );
  return result;
}

bool VBSMilixClient::init()
{
  bool result;
  QMetaObject::invokeMethod( &instance()->mWorker, "initialize", Qt::BlockingQueuedConnection, Q_RETURN_ARG( bool, result ) );
  return result;
}

bool VBSMilixClient::getSymbol( const QString& symbolId, SymbolDesc &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_GET_SYMBOL;
  istream << symbolId;
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_GET_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  QByteArray svgxml;
  ostream >> result.name >> result.militaryName >> svgxml >> result.hasVariablePoints >> result.minNumPoints;
  result.icon = renderSvg( svgxml );
  return true;
}

bool VBSMilixClient::getSymbols( const QStringList& symbolIds, QList<SymbolDesc> &result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_GET_SYMBOLS;
  istream << symbolIds;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_GET_SYMBOLS ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  int nResults;
  ostream >> nResults;
  if ( nResults != symbolIds.size() )
  {
    return false;
  }
  for ( int i = 0; i < nResults; ++i )
  {
    QByteArray svgxml;
    SymbolDesc desc;
    desc.symbolId = symbolIds[i];
    ostream >> desc.name >> desc.militaryName >> svgxml >> desc.hasVariablePoints >> desc.minNumPoints;
    desc.icon = renderSvg( svgxml );
    result.append( desc );
  }
  return true;
}

bool VBSMilixClient::appendPoint( const QRect &visibleExtent, const NPointSymbol& symbol, const QPoint& newPoint, NPointSymbolGraphic& result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_APPEND_POINT << visibleExtent << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized << newPoint;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_APPEND_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  QByteArray svgxml; ostream >> svgxml;
  result.graphic = renderSvg( svgxml );
  ostream >> result.offset;
  ostream >> result.adjustedPoints;
  ostream >> result.controlPoints;
  return true;
}

bool VBSMilixClient::insertPoint( const QRect &visibleExtent, const NPointSymbol& symbol, const QPoint& newPoint, NPointSymbolGraphic& result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_INSERT_POINT << visibleExtent << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized << newPoint;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_INSERT_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  QByteArray svgxml; ostream >> svgxml;
  result.graphic = renderSvg( svgxml );
  ostream >> result.offset;
  ostream >> result.adjustedPoints;
  ostream >> result.controlPoints;
  return true;
}

bool VBSMilixClient::movePoint( const QRect &visibleExtent, const NPointSymbol& symbol, int index, const QPoint& newPos, NPointSymbolGraphic& result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_MOVE_POINT << visibleExtent << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized << index << newPos;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_MOVE_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  QByteArray svgxml; ostream >> svgxml;
  result.graphic = renderSvg( svgxml );
  ostream >> result.offset;
  ostream >> result.adjustedPoints;
  ostream >> result.controlPoints;
  return true;
}

bool VBSMilixClient::canDeletePoint( const NPointSymbol& symbol, int index, bool& canDelete )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_CAN_DELETE_POINT;
  istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized << index;
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_CAN_DELETE_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> canDelete;
  return true;
}

bool VBSMilixClient::deletePoint( const QRect &visibleExtent, const NPointSymbol& symbol, int index, NPointSymbolGraphic& result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_DELETE_POINT << visibleExtent << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized << index;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_DELETE_POINT ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  QByteArray svgxml; ostream >> svgxml;
  result.graphic = renderSvg( svgxml );
  ostream >> result.offset;
  ostream >> result.adjustedPoints;
  ostream >> result.controlPoints;
  return true;
}

bool VBSMilixClient::editSymbol( const QRect &visibleExtent, const NPointSymbol& symbol, QString& newSymbolXml, QString &newSymbolMilitaryName, NPointSymbolGraphic& result )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_EDIT_SYMBOL << visibleExtent << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_EDIT_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> newSymbolXml;
  ostream >> newSymbolMilitaryName;
  QByteArray svgxml; ostream >> svgxml;
  result.graphic = renderSvg( svgxml );
  ostream >> result.offset;
  ostream >> result.adjustedPoints;
  ostream >> result.controlPoints;
  return true;
}

bool VBSMilixClient::updateSymbol( const QRect& visibleExtent, const NPointSymbol& symbol, NPointSymbolGraphic& result, bool returnPoints )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_UPDATE_SYMBOL;
  istream << visibleExtent;
  istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized << returnPoints;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_UPDATE_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  QByteArray svgxml; ostream >> svgxml;
  result.graphic = renderSvg( svgxml );
  ostream >> result.offset;
  if ( returnPoints )
  {
    ostream >> result.adjustedPoints;
    ostream >> result.controlPoints;
  }
  return true;
}

bool VBSMilixClient::updateSymbols( const QRect& visibleExtent, const QList<NPointSymbol>& symbols, QList<NPointSymbolGraphic>& result )
{
  int nSymbols = symbols.length();
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_UPDATE_SYMBOLS;
  istream << visibleExtent;
  istream << nSymbols;
  foreach ( const NPointSymbol& symbol, symbols )
  {
    istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized;
  }
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_UPDATE_SYMBOLS ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  int nOutSymbols;
  ostream >> nOutSymbols;
  if ( nOutSymbols != nSymbols )
  {
    return false;
  }
  for ( int i = 0; i < nOutSymbols; ++i )
  {
    NPointSymbolGraphic symbolGraphic;
    QByteArray svgxml; ostream >> svgxml;
    symbolGraphic.graphic = renderSvg( svgxml );
    ostream >> symbolGraphic.offset;
    result.append( symbolGraphic );
  }
  return true;
}

bool VBSMilixClient::validateSymbolXml( const QString& symbolXml, const QString& mssVersion, QString& adjustedSymbolXml, bool& valid, QString& messages )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_VALIDATE_SYMBOLXML;
  istream << symbolXml << mssVersion;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_VALIDATE_SYMBOLXML ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> adjustedSymbolXml >> valid >> messages;
  return true;
}

bool VBSMilixClient::downgradeSymbolXml( const QString& symbolXml, const QString& mssVersion, QString& adjustedSymbolXml, bool& valid, QString& messages )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_DOWNGRADE_SYMBOLXML;
  istream << symbolXml << mssVersion;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_DOWNGRADE_SYMBOLXML ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> adjustedSymbolXml >> valid >> messages;
  return true;
}

bool VBSMilixClient::hitTest( const NPointSymbol& symbol, const QPoint& clickPos, bool& hitTestResult )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_HIT_TEST;
  istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized << clickPos;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_HIT_TEST ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> hitTestResult;
  return true;
}

bool VBSMilixClient::pickSymbol( const QList<NPointSymbol>& symbols, const QPoint& clickPos, int& selectedSymbol )
{
  int nSymbols = symbols.length();
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_PICK_SYMBOL << clickPos;
  istream << nSymbols;
  foreach ( const NPointSymbol& symbol, symbols )
  {
    istream << symbol.xml << symbol.points << symbol.controlPoints << symbol.finalized;
  }
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_PICK_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> selectedSymbol;
  return true;
}

bool VBSMilixClient::getSupportedLibraryVersionTags( QStringList& versionTags, QStringList& versionNames )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_GET_LIBRARY_VERSION_TAGS;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_GET_LIBRARY_VERSION_TAGS ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> versionTags >> versionNames;
  return true;
}

bool VBSMilixClient::getCurrentLibraryVersionTag( QString& versionTag )
{
  bool result;
  QMetaObject::invokeMethod( &instance()->mWorker, "getCurrentLibraryVersionTag", Qt::BlockingQueuedConnection, Q_RETURN_ARG( bool, result ), Q_ARG( QString&, versionTag ) );
  return result;
}

bool VBSMilixClient::setSymbolOptions( int symbolSize, int lineWidth , int workMode )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_SET_SYMBOL_OPTIONS << symbolSize << lineWidth << workMode;

  QByteArray response;
  if ( !processRequest( request, response, VBS_MILIX_REPLY_SET_SYMBOL_OPTIONS ) )
  {
    return false;
  }
  return true;
}

bool VBSMilixClient::getControlPoints( const QString& symbolXml, int nPoints, QList<int>& controlPoints )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_GET_CONTROL_POINTS << symbolXml << nPoints;

  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_GET_CONTROL_POINTS ) )
  {
    return false;
  }
  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  ostream >> controlPoints;
  return true;
}

QImage VBSMilixClient::renderSvg( const QByteArray& xml )
{
  if ( xml.isEmpty() )
  {
    return QImage();
  }
  RSvgRendererHandle* handle = rsvgrenderer_read_data( reinterpret_cast<const unsigned char*>( xml.constData() ), xml.length() );
  if ( !handle )
  {
    return QImage();
  }
  QImage image( handle->width, handle->height, QImage::Format_ARGB32 );
  image.fill( Qt::transparent );
  rsvgrenderer_write_pixmap( handle, image.bits(), image.width(), image.height(), image.bytesPerLine() );
  rsvgrenderer_destroy( handle );
  return image;
}
