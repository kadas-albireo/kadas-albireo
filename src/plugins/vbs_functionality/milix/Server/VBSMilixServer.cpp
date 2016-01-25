#include "VBSMilixServer.hpp"
#include "VBSMilixCommands.hpp"

#include <QApplication>
#include <QNetworkConfigurationManager>
#include <QNetworkSession>
#include <QPainter>
#include <QSettings>
#include <QSvgRenderer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>
#include <QUuid>
#include <guiddef.h>
#include <winuser.h>

#define DEBUG

#ifdef DEBUG
#define LOG(msg) gTextEdit->appendPlainText(msg);
#include <QPlainTextEdit>
static QPlainTextEdit* gTextEdit = 0;
#else
#define LOG(msg)
#endif

VBSMilixServer::VBSMilixServer( const QString& addr, int port, QWidget* parent ) : QObject( parent ), mAddr( addr ), mPort( port ), mTcpServer( 0 ), mNetworkSession( 0 )
{
  QNetworkConfigurationManager manager;
  if ( manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired )
  {
    // Get saved network configuration
    QSettings settings( QSettings::UserScope, QLatin1String( "QtProject" ) );
    settings.beginGroup( QLatin1String( "QtNetwork" ) );
    const QString id = settings.value( QLatin1String( "DefaultNetworkConfiguration" ) ).toString();
    settings.endGroup();

    // If the saved network configuration is not currently discovered use the system default
    QNetworkConfiguration config = manager.configurationFromIdentifier( id );
    if (( config.state() & QNetworkConfiguration::Discovered ) !=
        QNetworkConfiguration::Discovered )
    {
      config = manager.defaultConfiguration();
    }

    mNetworkSession = new QNetworkSession( config, this );
    connect( mNetworkSession, SIGNAL( opened() ), this, SLOT( sessionOpened() ) );

    mNetworkSession->open();
  }
  else
  {
    sessionOpened();
  }
}

VBSMilixServer::~VBSMilixServer()
{
  delete mMssService;
}

void VBSMilixServer::sessionOpened()
{
  // Save the used configuration
  if ( mNetworkSession )
  {
    QNetworkConfiguration config = mNetworkSession->configuration();
    QString id;
    if ( config.type() == QNetworkConfiguration::UserChoice )
      id = mNetworkSession->sessionProperty( QLatin1String( "UserChoiceConfiguration" ) ).toString();
    else
      id = config.identifier();

    QSettings settings( QSettings::UserScope, QLatin1String( "QtProject" ) );
    settings.beginGroup( QLatin1String( "QtNetwork" ) );
    settings.setValue( QLatin1String( "DefaultNetworkConfiguration" ), id );
    settings.endGroup();
  }

  LOG( "Session opened" );

  mTcpServer = new QTcpServer( this );
  connect( mTcpServer, SIGNAL( newConnection() ), this, SLOT( onNewConnection() ) );

  QHostAddress address;
  if(mAddr.isEmpty()) {
	  address = QHostAddress( QHostAddress::LocalHost );
	  mAddr = address.toString();
  } else {
	  address = QHostAddress( mAddr );
  }
  if ( !mTcpServer->listen( address, mPort ) )
  {
    LOG( QString( "Unable to start server on %1: %2" )
         .arg( address.toString() )
         .arg( mTcpServer->errorString() ) );
  }
  else
  {
	mPort = mTcpServer->serverPort();
    LOG( QString( "Running on %1:%2" ).arg( address.toString() ).arg( mPort ) );
    QTextStream( stdout ) << mTcpServer->serverPort() << endl;
  }
}

void VBSMilixServer::onNewConnection()
{
  QTcpSocket *tcpSocket = mTcpServer->nextPendingConnection();
  connect( tcpSocket, SIGNAL( disconnected() ), this, SLOT( onSocketDisconnected() ) );
  connect( tcpSocket, SIGNAL( readyRead() ), this, SLOT( onSocketDataReady() ) );
  LOG( QString( "New connection from %1:%2" )
       .arg( tcpSocket->peerAddress().toString() )
       .arg( tcpSocket->peerPort() ) );
}

void VBSMilixServer::onSocketDataReady()
{
  QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>( QObject::sender() );
  QByteArray request = tcpSocket->readAll();

  LOG( QString( "Processing connection from %1" ).arg( tcpSocket->peerAddress().toString() ) );


  QByteArray reply;
  try
  {
    reply = processCommand( request );
  }
  catch ( const std::exception& e )
  {
    QDataStream stream( &reply, QIODevice::WriteOnly );
    stream << VBS_MILIX_REPLY_ERROR << QString( e.what() );
    LOG( QString( "Error: %1" ).arg( e.what() ) );
  }
  catch ( _com_error e )
  {
    QDataStream stream( &reply, QIODevice::WriteOnly );
    stream << VBS_MILIX_REPLY_ERROR << QString( e.ErrorMessage() );
    LOG( QString( "Error: %1" ).arg( e.ErrorMessage() ) );
  }


  qint32 len = reply.size();
  LOG( QString( "Sending %1 bytes reply" ).arg( len ) );

  tcpSocket->write( reinterpret_cast<char*>( &len ), sizeof( quint32 ) );
  tcpSocket->write( reply );
}

void VBSMilixServer::onSocketDisconnected()
{
  QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>( QObject::sender() );
  LOG( QString( "Connection from %1:%2 closed, exiting" )
       .arg( tcpSocket->peerAddress().toString() )
       .arg( tcpSocket->peerPort() ) );
  tcpSocket->deleteLater();
  QApplication::quit();
}

QByteArray VBSMilixServer::processCommand( QByteArray &request )
{
  QByteArray reply;
  QDataStream ostream( &reply, QIODevice::WriteOnly );

  QDataStream istream( &request, QIODevice::ReadOnly );
  VBSMilixServerRequest req;
  istream >> req;

  LOG( QString( "Processing request %1" ).arg( req ) );

  if ( req == VBS_MILIX_REQUEST_INIT )
  {
    OLE_HANDLE wid;
    istream >> wid;

    if ( mMssService != nullptr )
    {
      LOG( "Error: Already initialized" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "Already initialized" );
      return reply;
    }
    mMssService = MssComServer::IMssSymbolProviderServiceGSPtr( __uuidof( MssComServer::MssSymbolProviderServiceGS ) );

    _bstr_t license = mMssService->GetDefaultLicense( "" );
    LOG( QString( "Initializing MSS license %1" ).arg(( char* )license ) );
    mMssService->InitLicense( license );

    mMssService->OwnerHandle = wid;

    _bstr_t library = mMssService->GetDefaultLibrary( "" );
    LOG( QString( "Loading MSS library %1" ).arg(( char* )library ) );
    mMssSymbolProvider = mMssService->MssLoadLibrary( library, 0 );

    mMssSymbolProvider->DefaultRenderingOptions->RenderingCanvasType = MssComServer::mssRenderingCanvasTypeGdiPlusGS;
    mMssSymbolProvider->DefaultRenderingOptions->AntiAliasingStyle = MssComServer::mssAntiAliasingStyleAllGS;

    mMssSymbolFormat = mMssService->CreateFormatObj();
    mMssSymbolFormat->SymbolSize = 60;
    mMssSymbolFormat->WorkMode = MssComServer::mssWorkModeExtendedGS;

    mMssSymbolEditor = mMssSymbolProvider->CreateEditor();
    mMssSymbolEditor->SymbolFormat->WorkMode = MssComServer::mssWorkModeExtendedGS;

    mMssNPointDrawingTarget = MssComServer::IMssNPointDrawingTargetGSPtr( __uuidof( MssComServer::MssNPointDrawingTargetGS ) );

    LOG( QString( "Initialized MSS version %1" ).arg(( char* )mMssService->MssVersion ) );
    ostream << VBS_MILIX_REPLY_INIT_OK;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_GET_SYMBOL )
  {
    if ( mMssService == nullptr )
    {
      LOG( "Error: MSS not initialized" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "MSS not initialized" );
      return reply;
    }

    QString symbolXml;
    istream >> symbolXml;

    QString name;
    QString svgXml;
    bool hasVariablePoints;
    int minPointCount;
    QString errorMsg;

    if ( !getSymbolInfo( symbolXml, name, svgXml, hasVariablePoints, minPointCount, errorMsg ) )
    {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_GET_SYMBOL << name << svgXml << hasVariablePoints << minPointCount;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_GET_SYMBOLS )
  {
    if ( mMssService == nullptr )
    {
      LOG( "Error: MSS not initialized" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "MSS not initialized" );
      return reply;
    }

    QStringList symbolXmls;
    istream >> symbolXmls;

    ostream << VBS_MILIX_REPLY_GET_SYMBOL << symbolXmls.size();

    foreach ( const QString& symbolXml, symbolXmls )
    {
      QString name;
      QString svgXml;
      bool hasVariablePoints;
      int minPointCount;
      QString errorMsg;

      if ( !getSymbolInfo( symbolXml, name, svgXml, hasVariablePoints, minPointCount, errorMsg ) )
      {
        LOG( QString( "Error: %1" ).arg( errorMsg ) );
        QByteArray errreply;
        QDataStream errostream( &errreply, QIODevice::WriteOnly );
        errostream << VBS_MILIX_REPLY_ERROR << errorMsg;
        return errreply;
      }
      ostream << name << svgXml << hasVariablePoints << minPointCount;
    }
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_GET_NPOINT_SYMBOL )
  {
    if ( mMssService == nullptr )
    {
      LOG( "Error: MSS not initialized" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "MSS not initialized" );
      return reply;
    }

    QRect extent;
    QString symbolXml;
    QList<QPoint> points;
    istream >> extent >> symbolXml >> points;

    QByteArray svgXml;
    QPoint offset;
    QString errorMsg;
    if ( !renderSymbol( extent, symbolXml, points, svgXml, offset, errorMsg ) )
    {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_GET_NPOINT_SYMBOL << svgXml << offset;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_GET_NPOINT_SYMBOLS )
  {
    if ( mMssService == nullptr )
    {
      LOG( "Error: MSS not initialized" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "MSS not initialized" );
      return reply;
    }

    QRect extent;
    int nSymbols;
    istream >> extent >> nSymbols;

    ostream << VBS_MILIX_REPLY_GET_NPOINT_SYMBOLS << nSymbols;

    for ( int i = 0; i < nSymbols; ++i )
    {
      QString symbolXml;
      QList<QPoint> points;
      istream >> symbolXml >> points;

      QByteArray svgXml;
      QPoint offset;
      QString errorMsg;
      if ( !renderSymbol( extent, symbolXml, points, svgXml, offset, errorMsg ) )
      {
        LOG( QString( "Error: %1" ).arg( errorMsg ) );
        QByteArray errreply;
        QDataStream errostream( &errreply, QIODevice::WriteOnly );
        errostream << VBS_MILIX_REPLY_ERROR << errorMsg;
        return errreply;
      }
      ostream << svgXml << offset;
    }
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_EDIT_SYMBOL )
  {
    QString symbolXml;
    istream >> symbolXml;

    if ( mMssService == nullptr )
    {
      LOG( "Error: MSS not initialized" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "MSS not initialized" );
      return reply;
    }
    MssComServer::IMssStringObjGSPtr mssStringObj = mMssService->CreateMssStringObjStr( symbolXml.toLocal8Bit().data() );
    if ( !mMssSymbolEditor->EditSymbol( mssStringObj ) )
    {
      LOG( "Error: Editing failed" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "Editing failed" );
      return reply;
    }
    ostream << VBS_MILIX_REPLY_EDIT_SYMBOL << QString( mssStringObj->XmlString );
  }
  else
  {
    LOG( "Error: Unrecognized command" );
    ostream << VBS_MILIX_REPLY_ERROR << QString( "Unrecognized command" );
  }
  return reply;
}

bool VBSMilixServer::getSymbolInfo( const QString& symbolXml, QString& name, QString& svgXml, bool& hasVariablePoints, int& minPointCount, QString& errorMsg )
{
  MssComServer::IMssStringObjGSPtr mssStringObj = mMssService->CreateMssStringObjStr( symbolXml.toLocal8Bit().data() );
  MssComServer::IMssSymbolGraphicGSPtr mssSymbolGraphic = mMssSymbolProvider->CreateSymbolGraphic( mssStringObj, mMssSymbolFormat );
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = mMssSymbolProvider->CreateNPointGraphic( mssStringObj, mMssSymbolFormat );
  if ( !mssSymbolGraphic->IsValid || !mssNPointGraphic->IsValid )
  {
    errorMsg = "CreateSymbolGraphic or CreateNPointGraphic failed";
    return false;
  }

  QPixmap symbol;

  QByteArray xml = mssSymbolGraphic->CreateXaml();
  /*QSvgRenderer sr( xml );
  if ( sr.isValid() )
  {
    symbol = QPixmap( sr.defaultSize() );
    symbol.fill( Qt::transparent );
    QPainter p( &symbol );
    sr.render( &p );
  }
  else
  {
    HBITMAP bitmap = reinterpret_cast<HBITMAP>( mssSymbolGraphic->CreateBitmap( 0xFFFFFF ) );
    if ( !bitmap )
    {
      LOG( "Error: CreateBitmap failed" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "CreateBitmap failed" );
      return reply;
    }
    symbol = QPixmap::fromWinHBITMAP( bitmap );
    if ( symbol.isNull() )
    {
      LOG( "Error: Pixmap is null" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "Pixmap is null" );
      return reply;
    }
  }*/

  name = mMssSymbolProvider->LookupSymbolName( mssStringObj, MssComServer::mssWorkModeInternationalGS, MssComServer::mssLanguageEnglishGS, MssComServer::mssNameFormatNodeNameGS );
  hasVariablePoints = mssNPointGraphic->Schema->HasVariablePoints;
  minPointCount = mssNPointGraphic->Schema->MinPointCount;
  return true;
}

bool VBSMilixServer::renderSymbol( const QRect& visibleExtent, const QString& symbolXml, const QList<QPoint>& points, QByteArray& svgXml, QPoint& offset, QString& errorMsg )
{
  MssComServer::IMssStringObjGSPtr mssStringObj = mMssService->CreateMssStringObjStr( symbolXml.toLocal8Bit().data() );
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = mMssSymbolProvider->CreateNPointGraphic( mssStringObj, mMssSymbolFormat );
  if ( !mssNPointGraphic )
  {
    errorMsg = "CreateNPointGraphic failed";
    return false;
  }

  // Convert point list to TMssPointGS format
  QVector<MssComServer::TMssPointGS> mssPoints;
  foreach ( const QPoint& point, points )
  {
    MssComServer::TMssPointGS mssPoint;
    mssPoint.X = point.x();
    mssPoint.Y = point.y();
    mssPoints.append( mssPoint );
  }

  int nPoints = mssPoints.size();

  // Adjusts points so that they form a valid items
  MssComServer::IMssNPointDrawingCreationItemGSPtr creationItem = mMssNPointDrawingTarget->CreateNewNPointGraphic( mssNPointGraphic, false );
  creationItem->AddPoints( &mssPoints[0], nPoints );
  MssComServer::IMssNPointDrawingItemGSPtr drawingItem = creationItem->DrawingItem;
  drawingItem->CompletePoints();
  nPoints = drawingItem->PointCount;
  mssPoints.resize( nPoints );
  drawingItem->GetPoints( 0, &mssPoints[0], nPoints );
  LOG( QString( "Rendering N point symbol with %1 (%2 were specified) points" ).arg( mssPoints.size() ).arg( points.size() ) );

  // Offset from reference point (so that first point stays at the same spot)
  offset = QPoint( 0, 0 );
  for ( int i = 1; i < nPoints; ++i )
  {
    offset.rx() = qMin( offset.x(), int( mssPoints[i].X - mssPoints[0].X ) );
    offset.ry() = qMin( offset.y(), int( mssPoints[i].Y - mssPoints[0].Y ) );
  }
  MssComServer::IMssSymbolGraphicGSPtr mssSymbolGraphic = mssNPointGraphic->RenderImage( &mssPoints[0], nPoints, 72 );
  if ( !mssSymbolGraphic->IsValid )
  {
    errorMsg = "RenderImage failed";
    return false;
  }

  QPixmap symbol;

  QByteArray symbolXml = mssSymbolGraphic->CreateXaml();
  /*QSvgRenderer sr( xml );
  if ( sr.isValid() )
  {
    symbol = QPixmap( sr.defaultSize() );
    symbol.fill( Qt::transparent );
    QPainter p( &symbol );
    sr.render( &p );
  }
  else
  {
    HBITMAP bitmap = reinterpret_cast<HBITMAP>( mssSymbolGraphic->CreateBitmap( 0xFFFFFF ) );
    if ( !bitmap )
    {
      LOG( "Error: CreateBitmap failed" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "CreateBitmap failed" );
      return reply;
    }
    symbol = QPixmap::fromWinHBITMAP( bitmap );
    if ( symbol.isNull() )
    {
      LOG( "Error: Pixmap is null" );
      ostream << VBS_MILIX_REPLY_ERROR << QString( "Pixmap is null" );
      return reply;
    }
  }
  */
  return true;
}

int main( int argc, char* argv[] )
{
  QApplication app( argc, argv );
#ifdef DEBUG
  gTextEdit = new QPlainTextEdit();
  gTextEdit->show();
#endif
  int port = 0;
  QString addr;
  if(argc > 2) {
	  addr = argv[1];
	  port = atoi(argv[2]);
  }
  VBSMilixServer s(addr, port);
  return app.exec();
}
