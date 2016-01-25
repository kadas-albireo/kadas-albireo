#include "VBSMilixClient.hpp"
#include "../Server/VBSMilixCommands.hpp"
#include <QApplication>
#include <QDataStream>
#include <QEventLoop>
#include <QHostAddress>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>
#include <QPixmap>
#include <QProcess>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>
#include <QWidget>
#include <QPainter>
#include <QWebView>
#include <QDomDocument>

void VBSMilixClient::cleanup()
{
  delete mProcess;
  mProcess = 0;
  delete mTcpSocket;
  mTcpSocket = 0;
  delete mNetworkSession;
  mNetworkSession = 0;
}

bool VBSMilixClient::init()
{
  cleanup();
  mLastError = QString();

  // Start process
#ifdef Q_OS_WIN
  int port;
  QHostAddress addr(QHostAddress::LocalHost);
  mProcess = new QProcess( this );
  connect( mProcess, SIGNAL( finished( int ) ), this, SLOT( cleanup() ) );
  {
    QEventLoop evLoop;
    QTimer timer;
    timer.setSingleShot( true );
    connect( mProcess, SIGNAL( error( QProcess::ProcessError ) ), &evLoop, SLOT( quit() ) );
    connect( mProcess, SIGNAL( finished( int ) ), &evLoop, SLOT( quit() ) );
    connect( mProcess, SIGNAL( readyReadStandardOutput() ), &evLoop, SLOT( quit() ) );
    connect( &timer, SIGNAL( timeout() ), &evLoop, SLOT( quit() ) );
    mProcess->start( "milixserver" );
    timer.start( 5000 );
    evLoop.exec( QEventLoop::ExcludeUserInputEvents );
    QByteArray out = mProcess->readAllStandardOutput();
    if ( !timer.isActive() )
    {
      cleanup();
      mLastError = tr( "Timeout starting process" );
      return false;
    }
    else if ( !mProcess->isOpen() )
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
  int port = 31415;
  QString addr = "192.168.178.124";
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
    evLoop.exec( QEventLoop::ExcludeUserInputEvents );

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
  connect( mTcpSocket, SIGNAL( disconnected() ), this, SLOT( cleanup() ) );
  connect( mTcpSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), this, SLOT( handleSocketError() ) );
  {
    QEventLoop evLoop;
    connect( mTcpSocket, SIGNAL( error( QAbstractSocket::SocketError ) ), &evLoop, SLOT( quit() ) );
    connect( mTcpSocket, SIGNAL( disconnected() ), &evLoop, SLOT( quit() ) );
    connect( mTcpSocket, SIGNAL( connected() ), &evLoop, SLOT( quit() ) );
    mTcpSocket->connectToHost( addr, port );
    evLoop.exec( QEventLoop::ExcludeUserInputEvents );
  }

  if ( !mTcpSocket->isOpen() )
  {
    cleanup();
    return false;
  }

  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_INIT;
#ifdef Q_OS_WIN
  WId wid = 0;
  foreach ( QWidget* widget, QApplication::topLevelWidgets() )
  {
    if ( widget->inherits( "QMainWindow" ) )
    {
      wid = widget->effectiveWinId();
      break;
    }
  }

  istream << wid;
#else
  istream << 0;
#endif
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_INIT_OK ) )
  {
    cleanup();
    return false;
  }

  // TODO: If disconnected attempt to recover
  return true;
}

bool VBSMilixClient::processRequest( const QByteArray& request, QByteArray& response, VBSMilixServerReply expectedReply )
{
  mLastError = QString();

  if ( !mTcpSocket && !init() )
  {
    mLastError = tr( "Connection failed" );
    return false;
  }

  int requiredSize = 0;
  response.clear();

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

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  if ( replycmd == VBS_MILIX_REPLY_ERROR )
  {
    ostream >> mLastError;
    return false;
  }
  else if ( replycmd != expectedReply )
  {
    instance()->mLastError = tr( "Unexpected reply" );
    return false;
  }
  return true;
}

void VBSMilixClient::handleSocketError()
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
  Q_ASSERT( replycmd == VBS_MILIX_REPLY_GET_SYMBOLS );
  int nResults;
  ostream >> nResults;
  if ( nResults != symbolIds.size() )
  {
    return false;
  }
  for ( int i = 0; i < nResults; ++i )
  {
    QByteArray symbolXml;
    SymbolDesc desc;
    desc.symbolId = symbolIds[i];
    ostream >> desc.name >> symbolXml >> desc.hasVariablePoints >> desc.minNumPoints;
    desc.icon = renderSvg( symbolXml );
    result.append( desc );
  }
  return true;
}

bool VBSMilixClient::getSymbol( const QString& xml, QPixmap& pixmap, QString& name, bool& hasVariablePoints, int& minNumPoints )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_GET_SYMBOL;
  istream << xml;
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_GET_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  Q_ASSERT( replycmd == VBS_MILIX_REPLY_GET_SYMBOL );
  QByteArray svgxml;
  ostream >> name >> svgxml >> hasVariablePoints >> minNumPoints;
  pixmap = renderSvg( svgxml );
  return true;
}

bool VBSMilixClient::getNPointSymbol( const QString& xml, const QList<QPoint>& points, const QRect& visibleExtent, QPixmap& pixmap, QPoint& offset )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_GET_NPOINT_SYMBOL;
  istream << visibleExtent;
  istream << xml;
  istream << points;
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_GET_NPOINT_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  Q_ASSERT( replycmd == VBS_MILIX_REPLY_GET_NPOINT_SYMBOL );
  //ostream >> pixmap >> offset;
  QByteArray svgxml;
  ostream >> svgxml >> offset;
  pixmap = renderSvg( svgxml );
  return true;
}

bool VBSMilixClient::getNPointSymbols( const QList<NPointSymbol> &symbols, const QRect &visibleExtent, QList<NPointSymbolPixmap> &result )
{
  int nSymbols = symbols.length();
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_GET_NPOINT_SYMBOLS;
  istream << visibleExtent;
  istream << nSymbols;
  foreach ( const NPointSymbol& symbol, symbols )
  {
    istream << symbol.xml << symbol.points;
  }
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_GET_NPOINT_SYMBOLS ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  Q_ASSERT( replycmd == VBS_MILIX_REPLY_GET_NPOINT_SYMBOLS );
  int nOutSymbols;
  ostream >> nOutSymbols;
  if ( nOutSymbols != nSymbols )
  {
    return false;
  }
  for ( int i = 0; i < nOutSymbols; ++i )
  {
    QByteArray svgxml;
    NPointSymbolPixmap symbolPixmap;
    ostream >> svgxml >> symbolPixmap.offset;
    symbolPixmap.pixmap = renderSvg( svgxml );
    result.append( symbolPixmap );
  }
  return true;
}

bool VBSMilixClient::editSymbol( const QString& xml, QString& outXml )
{
  QByteArray request;
  QDataStream istream( &request, QIODevice::WriteOnly );
  istream << VBS_MILIX_REQUEST_EDIT_SYMBOL;
  istream << xml;
  QByteArray response;
  if ( !instance()->processRequest( request, response, VBS_MILIX_REPLY_EDIT_SYMBOL ) )
  {
    return false;
  }

  QDataStream ostream( &response, QIODevice::ReadOnly );
  VBSMilixServerReply replycmd = 0; ostream >> replycmd;
  Q_ASSERT( replycmd == VBS_MILIX_REPLY_EDIT_SYMBOL );
  ostream >> outXml;
  return true;
}

QPixmap VBSMilixClient::renderSvg( const QByteArray& xml )
{
  // TODO: Can't find a way to get the svg size from the webview
  QDomDocument doc;
  doc.setContent( xml );
  QDomElement svgEl = doc.firstChildElement( "svg" );
  int width = svgEl.attribute( "width" ).toDouble();
  int height = svgEl.attribute( "height" ).toDouble();

  QWebView web;
  web.setRenderHint( QPainter::SmoothPixmapTransform );
  web.setRenderHint( QPainter::Antialiasing );
  web.setRenderHint( QPainter::TextAntialiasing );
  web.setContent( xml, "image/svg+xml" );
  web.setStyleSheet( "background:transparent;" );
  web.setAttribute( Qt::WA_TranslucentBackground );
  web.resize( width, height );
  QPixmap pixmap( QSize( width, height ) );
  pixmap.fill( Qt::transparent );
  QPainter p( &pixmap );
  web.render( &p );
  return pixmap;
}
