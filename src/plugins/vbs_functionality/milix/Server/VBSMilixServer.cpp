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

VBSMilixServer::VBSMilixServer( const QString& addr, int port, QWidget* parent )
  : QObject( parent ), mAddr( addr ), mPort( port ), mTcpServer( 0 ), mNetworkSession( 0 ), mRequestSize( 0 ), mSymbolSize(60), mLineWidth(2), mWorkMode(MssComServer::mssWorkModeExtendedGS)
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
  if ( mAddr.isEmpty() )
  {
    address = QHostAddress( QHostAddress::LocalHost );
    mAddr = address.toString();
  }
  else
  {
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
  LOG( QString( "Received %1 bytes from %2" ).arg( request.size() ).arg( tcpSocket->peerAddress().toString() ) );
  if ( mRequestSize == 0 )
  {
    mRequestSize = *reinterpret_cast<qint32*>( request.left( sizeof( qint32 ) ).data() );
    mRequestBuffer = request.mid( sizeof( qint32 ) );
  }
  else
  {
    mRequestBuffer.append( request );
  }
  if ( mRequestBuffer.size() < mRequestSize )
  {
    return;
  }


  QByteArray reply;
  try
  {
    reply = processCommand( mRequestBuffer );
  }
  catch ( const std::exception& e )
  {
    QDataStream stream( &reply, QIODevice::WriteOnly );
    stream << VBS_MILIX_REPLY_ERROR << QString( e.what() );
    LOG( QString( "Error: %1" ).arg( e.what() ) );
  }
  catch ( _com_error e )
  {
    reply.clear();
    QDataStream stream( &reply, QIODevice::WriteOnly );
    stream << VBS_MILIX_REPLY_ERROR << QString( e.ErrorMessage() );
    LOG( QString( "Error: %1" ).arg( e.ErrorMessage() ) );
  }

  mRequestBuffer.clear();
  mRequestSize = 0;


  qint32 len = reply.size();
  LOG( QString( "Sending %1 bytes reply" ).arg( len ) );

  tcpSocket->write( reinterpret_cast<char*>( &len ), sizeof( quint32 ) );
  tcpSocket->write( reply );
}

void VBSMilixServer::onSocketDisconnected()
{
  QTcpSocket* tcpSocket = qobject_cast<QTcpSocket*>( QObject::sender() );
  LOG( QString( "Connection from %1:%2 closed" )
       .arg( tcpSocket->peerAddress().toString() )
       .arg( tcpSocket->peerPort() ) );
  tcpSocket->deleteLater();
  mRequestBuffer.clear();
  mRequestSize = 0;
  if(mAddr == QHostAddress( QHostAddress::LocalHost ).toString()) {
    QApplication::quit();
  }
}

QByteArray VBSMilixServer::processCommand( QByteArray &request )
{
  QByteArray reply;
  QDataStream ostream( &reply, QIODevice::WriteOnly );

  QDataStream istream( &request, QIODevice::ReadOnly );
  VBSMilixServerRequest req;
  istream >> req;

  LOG( QString( "Processing request %1" ).arg( req ) );

  if( req != VBS_MILIX_REQUEST_INIT && mMssService == nullptr )
  {
    LOG( "Error: MSS not initialized" );
    ostream << VBS_MILIX_REPLY_ERROR << QString( "MSS not initialized" );
    return reply;
  }

  if ( req == VBS_MILIX_REQUEST_INIT )
  {
    QString language;
    istream >> language;

    if(language.startsWith("de", Qt::CaseInsensitive)) {
      mLanguage = MssComServer::mssLanguageGermanGS;
    } else if(language.startsWith("fr", Qt::CaseInsensitive)) {
      mLanguage = MssComServer::mssLanguageFrenchGS;
    } else if(language.startsWith("it", Qt::CaseInsensitive)) {
      mLanguage = MssComServer::mssLanguageItalianGS;
    } else {
      mLanguage = MssComServer::mssLanguageEnglishGS;
    }

    if ( mMssService != nullptr )
    {
      LOG( "Already initialized" );
      ostream << VBS_MILIX_REPLY_INIT_OK << bstr2qstring(mMssSymbolProvider->LibraryVersionTag);
      return reply;
    }
    mMssService = MssComServer::IMssSymbolProviderServiceGSPtr( __uuidof( MssComServer::MssSymbolProviderServiceGS ) );

    // _bstr_t license = mMssService->GetDefaultLicense( "" );
    // LOG( QString( "Initializing MSS license %1" ).arg(( char* )license ) );
    LOG( "Initializing MSS license" );
    mMssService->InitLicense( "CH_GRUNDRE_GEOINFO_VBS_3F0A23AB7263B54E5A280E0AC53B94BD402F401F" );

    mMssService->OwnerHandle = 0;

    mMssService->GuiLanguage = mLanguage;

    _bstr_t library = mMssService->GetDefaultLibrary( "" );
    LOG( QString( "Loading MSS library %1" ).arg(( char* )library ) );
    mMssSymbolProvider = mMssService->MssLoadLibrary( library, 0 );
    mMssSymbolProvider->Language = mLanguage;

    mMssSymbolProvider->DefaultRenderingOptions->RenderingCanvasType = MssComServer::mssRenderingCanvasTypeGdiPlusGS;
    mMssSymbolProvider->DefaultRenderingOptions->AntiAliasingStyle = MssComServer::mssAntiAliasingStyleAllGS;

    mMssNPointDrawingTarget = MssComServer::IMssNPointDrawingTargetGSPtr( __uuidof( MssComServer::MssNPointDrawingTargetGS ) );

    LOG( QString( "Initialized MSS version %1" ).arg( bstr2qstring(mMssSymbolProvider->LibraryVersionTag) ) );
    ostream << VBS_MILIX_REPLY_INIT_OK << bstr2qstring(mMssSymbolProvider->LibraryVersionTag);
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_GET_SYMBOL )
  {
    QString symbolXml;
    istream >> symbolXml;

    QString name;
    QString militaryName;
    QByteArray svgXml;
    bool hasVariablePoints;
    int minPointCount;
    QString errorMsg;

    if ( !getSymbolInfo( symbolXml, name, militaryName, svgXml, hasVariablePoints, minPointCount, errorMsg ) )
    {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_GET_SYMBOL << name << militaryName << svgXml << hasVariablePoints << minPointCount;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_GET_SYMBOLS )
  {
    QStringList symbolXmls;
    istream >> symbolXmls;

    ostream << VBS_MILIX_REPLY_GET_SYMBOLS << symbolXmls.size();

    foreach ( const QString& symbolXml, symbolXmls )
    {
      QString name;
      QString militaryName;
      QByteArray svgXml;
      bool hasVariablePoints;
      int minPointCount;
      QString errorMsg;

      if ( !getSymbolInfo( symbolXml, name, militaryName, svgXml, hasVariablePoints, minPointCount, errorMsg ) )
      {
        LOG( QString( "Error: %1" ).arg( errorMsg ) );
        QByteArray errreply;
        QDataStream errostream( &errreply, QIODevice::WriteOnly );
        errostream << VBS_MILIX_REPLY_ERROR << errorMsg;
        return errreply;
      }
      ostream << name << militaryName << svgXml << hasVariablePoints << minPointCount;
    }
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_APPEND_POINT )
  {
    QRect visibleExtent;
    SymbolInput input;
    QPoint newPoint;
    istream >> visibleExtent >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized >> newPoint;
    input.points.append(newPoint);
    SymbolOutput output;
    QString errorMsg;
    if( !renderSymbol( visibleExtent, input, output, errorMsg ) ) {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_APPEND_POINT << output.svgXml << output.offset << output.adjustedPoints << output.controlPoints;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_INSERT_POINT )
  {
    QRect visibleExtent;
    SymbolInput input;
    QPoint newPoint;
    istream >> visibleExtent >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized >> newPoint;
    SymbolOutput output;
    QString errorMsg;
    if(!insertPoint(visibleExtent, input, newPoint, output, errorMsg)) {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_INSERT_POINT << output.svgXml << output.offset << output.adjustedPoints << output.controlPoints;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_MOVE_POINT )
  {
    QRect visibleExtent;
    SymbolInput input;
    int moveIndex;
    QPoint newPos;
    istream >> visibleExtent >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized >> moveIndex >> newPos;
    SymbolOutput output;
    QString errorMsg;
    if(!movePoint(visibleExtent, input, moveIndex, newPos, output, errorMsg)) {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_MOVE_POINT << output.svgXml << output.offset << output.adjustedPoints << output.controlPoints;
    return reply;
  }
  else if( req == VBS_MILIX_REQUEST_CAN_DELETE_POINT )
  {
    SymbolInput input;
    int index;
    istream >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized >> index;

    bool canDelete;
    QString errorMsg;
    if ( !canDeletePoint( input, index, canDelete, errorMsg ) )
    {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_CAN_DELETE_POINT << canDelete;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_DELETE_POINT )
  {
    QRect visibleExtent;
    SymbolInput input;
    int deleteIndex;
    istream >> visibleExtent >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized >> deleteIndex;
    SymbolOutput output;
    QString errorMsg;
    if(!deletePoint(visibleExtent, input, deleteIndex, output, errorMsg)) {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_DELETE_POINT << output.svgXml << output.offset << output.adjustedPoints << output.controlPoints;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_EDIT_SYMBOL )
  {
    QRect visibleExtent;
    SymbolInput input;
    istream >> visibleExtent >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized;
    QString outputSymbolXml;
    QString outputMilitaryName;
    SymbolOutput output;
    QString errorMsg;
    if(!editSymbol(visibleExtent, input, outputSymbolXml, outputMilitaryName, output, errorMsg)) {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }
    ostream << VBS_MILIX_REPLY_EDIT_SYMBOL << outputSymbolXml << outputMilitaryName << output.svgXml << output.offset << output.adjustedPoints << output.controlPoints;
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_UPDATE_SYMBOL )
  {
    QRect visibleExtent;
    SymbolInput input;
    bool returnPoints;
    istream >> visibleExtent >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized >> returnPoints;
    SymbolOutput output;
    QString errorMsg;
    if ( !renderSymbol( visibleExtent, input, output, errorMsg ) )
    {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }

    ostream << VBS_MILIX_REPLY_UPDATE_SYMBOL << output.svgXml << output.offset;
    if(returnPoints) {
      ostream << output.adjustedPoints << output.controlPoints;
    }
    return reply;
  }
  else if ( req == VBS_MILIX_REQUEST_UPDATE_SYMBOLS )
  {
    QRect visibleExtent;
    int nSymbols;
    istream >> visibleExtent >> nSymbols;

    ostream << VBS_MILIX_REPLY_UPDATE_SYMBOLS << nSymbols;

    for ( int i = 0; i < nSymbols; ++i )
    {
      SymbolInput input;
      istream >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized;
      SymbolOutput output;
      QString errorMsg;
      if ( !renderSymbol( visibleExtent, input, output, errorMsg ) )
      {
        LOG( QString( "Error: %1" ).arg( errorMsg ) );
        QByteArray errreply;
        QDataStream errostream( &errreply, QIODevice::WriteOnly );
        errostream << VBS_MILIX_REPLY_ERROR << errorMsg;
        return errreply;
      }
      ostream << output.svgXml << output.offset;
    }
    return reply;
  }
  else if( req == VBS_MILIX_REQUEST_VALIDATE_SYMBOLXML)
  {
    QString symbolXml;
    QString mssVersion;
    istream >> symbolXml >> mssVersion;

    QString curVer = bstr2qstring(mMssSymbolProvider->LibraryVersionTag);

    MssComServer::IMssStringObjGSPtr mssString;
    BSTR messages = L"";
    MssComServer::TMssVerificationResultGS result;

    // Upgrade if older, validate if equal
    if(mssVersion < curVer) {
      MssComServer::IMssSymbolConverterGSPtr converter = mMssSymbolProvider->CreateConverter(mssVersion.toLocal8Bit().data());
      result = converter->UpgradeMssString(symbolXml.toLocal8Bit().data(), &mssString, &messages);
    } else if(mssVersion == curVer) {
      result = mMssSymbolProvider->VerifyMssString(symbolXml.toLocal8Bit().data(), &mssString, &messages);
    } else {
      // Newer versions not supported
      result = MssComServer::mssVrErrorGS;
      messages = L"Version too new";
    }
    QString outSymbolXml = mssString != 0 ? bstr2qstring(mssString->XmlString) : symbolXml;
    bool valid = result != MssComServer::mssVrErrorGS;
    QString outMessages = bstr2qstring(messages);
    LOG(QString("Validate %1 - valid: %2, messages: %3, adjusted: %4").arg(symbolXml).arg(valid).arg(outMessages).arg(outSymbolXml));
    ostream << VBS_MILIX_REPLY_VALIDATE_SYMBOLXML << outSymbolXml << valid << outMessages;
    return reply;
  }
  else if( req == VBS_MILIX_REQUEST_DOWNGRADE_SYMBOLXML)
  {
    QString symbolXml;
    QString mssVersion;
    istream >> symbolXml >> mssVersion;

    QString curVer = bstr2qstring(mMssSymbolProvider->LibraryVersionTag);

    QString outSymbolXml = symbolXml;
    QString outMessages;
    MssComServer::TMssVerificationResultGS result;

    // Upgrade if older, validate if equal
    if(mssVersion < curVer) {
      MssComServer::IMssStringObjGSPtr mssString;
      BSTR downgradedSymbolXml;
      BSTR messages;
      mssString = mMssService->CreateMssStringObjStr( symbolXml.toLocal8Bit().data() );
      MssComServer::IMssSymbolConverterGSPtr converter = mMssSymbolProvider->CreateConverter(mssVersion.toLocal8Bit().data());
      result = converter->DowngradeMssString(mssString, &downgradedSymbolXml, &messages);
      outSymbolXml = result != MssComServer::mssVrErrorGS != 0 ? bstr2qstring(downgradedSymbolXml) : symbolXml;
      outMessages = bstr2qstring(messages);
    } else if(mssVersion == curVer) {
      result = MssComServer::mssVrOkGS;
    } else {
      // Newer versions not supported
      result = MssComServer::mssVrErrorGS;
      outMessages = "Version too new";
    }
    bool valid = result != MssComServer::mssVrErrorGS;
    LOG(QString("Downgrade %1 - valid: %2, messages: %3, adjusted: %4").arg(symbolXml).arg(valid).arg(outMessages).arg(outSymbolXml));
    ostream << VBS_MILIX_REPLY_DOWNGRADE_SYMBOLXML << outSymbolXml << valid << outMessages;
    return reply;
  }
  else if(req == VBS_MILIX_REQUEST_HIT_TEST)
  {
    SymbolInput input;
    QPoint clickPos;
    istream >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized >> clickPos;
    bool hitTestResult = false;
    QString errorMsg;
    if ( !hitTest( input, clickPos, hitTestResult, errorMsg ) )
    {
      LOG( QString( "Error: %1" ).arg( errorMsg ) );
      ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
      return reply;
    }

    ostream << VBS_MILIX_REPLY_HIT_TEST << hitTestResult;
    return reply;
  }
  else if(req == VBS_MILIX_REQUEST_GET_LIBRARY_VERSION_TAGS)
  {
    QStringList libraryVersionTags;
    QStringList libraryVersionNames;
    getLibraryVersionTags(libraryVersionTags, libraryVersionNames);
    ostream << VBS_MILIX_REPLY_GET_LIBRARY_VERSION_TAGS << libraryVersionTags << libraryVersionNames;
	return reply;
  }
  else if(req == VBS_MILIX_REQUEST_PICK_SYMBOL)
  {
    QPoint clickPos;
    int nSymbols;
    istream >> clickPos >> nSymbols;
    for ( int i = 0; i < nSymbols; ++i )
    {
      SymbolInput input;
      istream >> input.symbolXml >> input.points >> input.controlPoints >> input.finalized;
      bool hitTestResult = false;
      QString errorMsg;
      if ( !hitTest( input, clickPos, hitTestResult, errorMsg ) )
      {
        LOG( QString( "Error: %1" ).arg( errorMsg ) );
        ostream << VBS_MILIX_REPLY_ERROR << errorMsg;
        return reply;
      }
      else if(hitTestResult == true)
      {
        ostream << VBS_MILIX_REPLY_PICK_SYMBOL << i;
        return reply;
      }
    }
    ostream << VBS_MILIX_REPLY_PICK_SYMBOL << -1;
    return reply;
  }
  else if(req == VBS_MILIX_REQUEST_SET_SYMBOL_OPTIONS)
  {
    int workMode;
    istream >> mSymbolSize >> mLineWidth >> workMode;
    mWorkMode = static_cast<MssComServer::TMssWorkModeGS>(workMode);
    ostream << VBS_MILIX_REPLY_SET_SYMBOL_OPTIONS;
    return reply;

  }
  else if(req == VBS_MILIX_REQUEST_GET_CONTROL_POINTS)
  {
    QString symbolXml;
    QList<QPoint> points;
    istream >> symbolXml >> points;

    MssComServer::IMssSymbolFormatGSPtr symbolFormat = mMssService->CreateFormatObj();
    symbolFormat->SymbolSize = 60;
    symbolFormat->RelLineWidth = mLineWidth / 60.;
    symbolFormat->WorkMode = mWorkMode;

    MssComServer::IMssStringObjGSPtr mssStringObj = mMssService->CreateMssStringObjStr( symbolXml.toLocal8Bit().data() );
    MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = mMssSymbolProvider->CreateNPointGraphic( mssStringObj, symbolFormat );
    if ( !mssNPointGraphic )
    {
      ostream << VBS_MILIX_REPLY_ERROR << QString( "CreateNPointGraphic failed for %1" ).arg( symbolXml );
      return reply;
    }

    MssComServer::IMssNPointDrawingCreationItemGSPtr mssCreationItem = mMssNPointDrawingTarget->CreateNewNPointGraphic( mssNPointGraphic, false );
    MssComServer::IMssNPointDrawingItemGSPtr mssDrawingItem = mssCreationItem->DrawingItem;
    QVector<MssComServer::TMssPointGS> mssPoints;
    for(int i = 0, n = points.size(); i < n; ++i)
    {
      MssComServer::TMssPointGS mssPoint;
      mssPoint.X = points[i].x();
      mssPoint.Y = points[i].y();
      mssPoints.append( mssPoint );
    }
    int nPoints = mssPoints.size();
    mssDrawingItem->AddPoints(&mssPoints[0], nPoints);
    QList<int> controlPoints;
    for(int i = 0; i < nPoints; ++i) {
      if(mssDrawingItem->IsCtrlPoint(i) == VARIANT_TRUE) {
        controlPoints.append(i);
      }
    }
    ostream << VBS_MILIX_REPLY_GET_CONTROL_POINTS << controlPoints;
    return reply;
  }
  else
  {
    LOG( "Error: Unrecognized command" );
    ostream << VBS_MILIX_REPLY_ERROR << QString( "Unrecognized command" );
  }
  return reply;
}

bool VBSMilixServer::getSymbolInfo(const QString& symbolXml, QString& name, QString& militaryName, QByteArray& svgXml, bool& hasVariablePoints, int& minPointCount, QString& errorMsg )
{
  MssComServer::IMssSymbolFormatGSPtr mssSymbolFormat = mMssService->CreateFormatObj();
  mssSymbolFormat->SymbolSize = 30; // Render previews smaller
  mssSymbolFormat->WorkMode = MssComServer::mssWorkModeExtendedGS;

  MssComServer::IMssStringObjGSPtr mssStringObj = mMssService->CreateMssStringObjStr( symbolXml.toLocal8Bit().data() );
  MssComServer::IMssSymbolGraphicGSPtr mssSymbolGraphic = mMssSymbolProvider->CreateSymbolGraphic( mssStringObj, mssSymbolFormat );
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = mMssSymbolProvider->CreateNPointGraphic( mssStringObj, mssSymbolFormat );
  if ( !mssSymbolGraphic->IsValid || !mssNPointGraphic->IsValid )
  {
    errorMsg = QString( "CreateSymbolGraphic or CreateNPointGraphic failed for %1" ).arg( symbolXml );
    return false;
  }

  svgXml = bstr2qstring(mssSymbolGraphic->CreateSvg() ).toUtf8();
  name = mMssSymbolProvider->LookupSymbolName( mssStringObj, MssComServer::mssWorkModeExtendedGS, mLanguage, MssComServer::mssNameFormatNodeNameGS );
  militaryName = mMssSymbolProvider->LookupSymbolName( mssStringObj, MssComServer::mssWorkModeExtendedGS, mLanguage, MssComServer::mssNameFormatFormattedModifiersGS );
  MssComServer::IMssNPointDrawingCreationItemGSPtr mssCreationItem = mMssNPointDrawingTarget->CreateNewNPointGraphic( mssNPointGraphic, false );
  hasVariablePoints = mssCreationItem->HasVariablePoints;
  minPointCount = mssCreationItem->EnoughPointCount;
  return true;
}

bool VBSMilixServer::renderSymbol(const QRect& visibleExtent, const SymbolInput &input, SymbolOutput &output, QString& errorMsg )
{
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = 0;
  MssComServer::IMssNPointDrawingCreationItemGSPtr mssCreationItem = 0;
  MssComServer::IMssNPointDrawingItemGSPtr mssDrawingItem = 0;

  return createDrawingItem(input, errorMsg, mssNPointGraphic, mssCreationItem, mssDrawingItem)
      && renderItem(mssNPointGraphic, mssCreationItem, mssDrawingItem, visibleExtent, output, errorMsg);
}

bool VBSMilixServer::insertPoint(const QRect& visibleExtent, const SymbolInput& input, const QPoint& newPoint, SymbolOutput& output, QString& errorMsg)
{
  LOG("Insert point");
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = 0;
  MssComServer::IMssNPointDrawingCreationItemGSPtr mssCreationItem = 0;
  MssComServer::IMssNPointDrawingItemGSPtr mssDrawingItem = 0;
  if(!createDrawingItem(input, errorMsg, mssNPointGraphic, mssCreationItem, mssDrawingItem)) {
    return false;
  }

  MssComServer::TMssPointGS mssNewPoint;
  mssNewPoint.X = newPoint.x();
  mssNewPoint.Y = newPoint.y();
  long insertIndex = mssDrawingItem->FindInsertPointIndex(mssNewPoint, 5);
  LOG(QString("Insert index: %1").arg(insertIndex));
  if(insertIndex >= 0) {
    mssDrawingItem->PrepareAndInsertPoint(&insertIndex, &mssNewPoint);
  }

  return renderItem(mssNPointGraphic, mssCreationItem, mssDrawingItem, visibleExtent, output, errorMsg);
}

bool VBSMilixServer::movePoint(const QRect& visibleExtent, const SymbolInput& input, int moveIndex, const QPoint& newPos, SymbolOutput& output, QString& errorMsg)
{
  LOG(QString("Move point %1").arg(moveIndex));
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = 0;
  MssComServer::IMssNPointDrawingCreationItemGSPtr mssCreationItem = 0;
  MssComServer::IMssNPointDrawingItemGSPtr mssDrawingItem = 0;
  if(!createDrawingItem(input, errorMsg, mssNPointGraphic, mssCreationItem, mssDrawingItem)) {
    return false;
  }
  LOG(QString("Moved point is control point: %1").arg(mssDrawingItem->IsCtrlPoint(moveIndex) == VARIANT_TRUE));

  MssComServer::TMssPointGS mssNewPos;
  mssNewPos.X = newPos.x();
  mssNewPos.Y = newPos.y();
  mssDrawingItem->EditPoint(moveIndex, &mssNewPos);

  return renderItem(mssNPointGraphic, mssCreationItem, mssDrawingItem, visibleExtent, output, errorMsg);
}

bool VBSMilixServer::canDeletePoint(const SymbolInput& input, int index, bool& canDelete, QString& errorMsg)
{
  LOG(QString("Can delete point %1").arg(index));
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = 0;
  MssComServer::IMssNPointDrawingCreationItemGSPtr mssCreationItem = 0;
  MssComServer::IMssNPointDrawingItemGSPtr mssDrawingItem = 0;
  if(!createDrawingItem(input, errorMsg, mssNPointGraphic, mssCreationItem, mssDrawingItem)) {
    return false;
  }
  canDelete = mssDrawingItem->CanDeletePoint(index);
  return true;
}

bool VBSMilixServer::deletePoint(const QRect &visibleExtent, const SymbolInput& input, int deleteIndex, SymbolOutput &output, QString& errorMsg)
{
  LOG(QString("Delete point %1").arg(deleteIndex));
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = 0;
  MssComServer::IMssNPointDrawingCreationItemGSPtr mssCreationItem = 0;
  MssComServer::IMssNPointDrawingItemGSPtr mssDrawingItem = 0;
  if(!createDrawingItem(input, errorMsg, mssNPointGraphic, mssCreationItem, mssDrawingItem)) {
    return false;
  }

  mssDrawingItem->PrepareAndDeletePoint(deleteIndex);

  return renderItem(mssNPointGraphic, mssCreationItem, mssDrawingItem, visibleExtent, output, errorMsg);
}

bool VBSMilixServer::editSymbol(const QRect& visibleExtent, const SymbolInput& input, QString& outputSymbolXml, QString& outputMilitaryName, SymbolOutput& output, QString& errorMsg)
{
  MssComServer::IMssStringObjGSPtr mssStringObj = mMssService->CreateMssStringObjStr( input.symbolXml.toLocal8Bit().data() );

  MssComServer::IMssSymbolFormatGSPtr mssSymbolFormat = mMssService->CreateFormatObj();
  if(input.points.size() == 1) {
    mssSymbolFormat->SymbolSize = mSymbolSize;
  } else {
    mssSymbolFormat->SymbolSize = 60;
    mssSymbolFormat->RelLineWidth = mLineWidth / 60.;
  }
  mssSymbolFormat->WorkMode = mWorkMode;

  MssComServer::IMssNPointGraphicTemplateGSPtr mssGraphicTemplate = mMssSymbolProvider->CreateNPointGraphic( mssStringObj, mssSymbolFormat );

  MssComServer::IMssSymbolHierarchyGSPtr hierarchy = mMssSymbolProvider->CreateHierarchy(false);
  hierarchy->AddSchema( mssGraphicTemplate->Schema->SchemaType );

  MssComServer::IMssSymbolEditorGSPtr mssSymbolEditor = mMssSymbolProvider->CreateEditor();
  mssSymbolEditor->SymbolFormat->WorkMode = MssComServer::mssWorkModeExtendedGS;
  mssSymbolEditor->CustomHierarchy = hierarchy;
  if ( !mssSymbolEditor->EditSymbol( mssStringObj ) )
  {
    LOG( "Error: Editing failed" );
    errorMsg = "Editing failed or canceled";
    return false;
  }
  SymbolInput newInput = input;
  newInput.symbolXml = bstr2qstring(mssStringObj->XmlString);
  outputSymbolXml = newInput.symbolXml;
  outputMilitaryName = mMssSymbolProvider->LookupSymbolName( mssStringObj, MssComServer::mssWorkModeExtendedGS, mLanguage, MssComServer::mssNameFormatFormattedModifiersGS );
  LOG(QString("New symbol XML: %1").arg(outputSymbolXml));
  return renderSymbol(visibleExtent, newInput, output, errorMsg);
}

bool VBSMilixServer::hitTest(const SymbolInput& input, const QPoint& clickPos, bool& hitTestResult, QString& errorMsg)
{
  MssComServer::IMssNPointGraphicTemplateGSPtr mssNPointGraphic = 0;
  MssComServer::IMssNPointDrawingCreationItemGSPtr mssCreationItem = 0;
  MssComServer::IMssNPointDrawingItemGSPtr mssDrawingItem = 0;

  if(!createDrawingItem(input, errorMsg, mssNPointGraphic, mssCreationItem, mssDrawingItem)) {
    return false;
  }
  MssComServer::TMssPointGS p;
  p.X = clickPos.x();
  p.Y = clickPos.y();
  hitTestResult = mssDrawingItem->HitTest(p, 15);
  return true;
}

void VBSMilixServer::getLibraryVersionTags(QStringList &versionTags, QStringList& versionNames)
{
  for(int i = 0; i < mMssSymbolProvider->VersionLabelCount; ++i) {
    MssComServer::IMssVersionLabelGSPtr label = mMssSymbolProvider->VersionLabel[i];
    versionTags.append(bstr2qstring(label->LibVersionTag));
    versionNames.append(bstr2qstring(label->LabelName));
  }
}

bool VBSMilixServer::createDrawingItem(const SymbolInput& input, QString& errorMsg, MssComServer::IMssNPointGraphicTemplateGSPtr& mssNPointGraphic, MssComServer::IMssNPointDrawingCreationItemGSPtr &mssCreationItem, MssComServer::IMssNPointDrawingItemGSPtr& mssDrawingItem)
{
  MssComServer::IMssSymbolFormatGSPtr symbolFormat = mMssService->CreateFormatObj();
  if(input.points.size() == 1) {
    symbolFormat->SymbolSize = mSymbolSize;
  } else {
    symbolFormat->SymbolSize = 60;
    symbolFormat->RelLineWidth = mLineWidth / 60.;
  }
  symbolFormat->WorkMode = mWorkMode;

  MssComServer::IMssStringObjGSPtr mssStringObj = mMssService->CreateMssStringObjStr( input.symbolXml.toLocal8Bit().data() );
  mssNPointGraphic = mMssSymbolProvider->CreateNPointGraphic( mssStringObj, symbolFormat );
  if ( !mssNPointGraphic )
  {
    errorMsg = QString( "CreateNPointGraphic failed for %1" ).arg( input.symbolXml );
    return false;
  }

  // Create creation item, add regular points
  mssCreationItem = mMssNPointDrawingTarget->CreateNewNPointGraphic( mssNPointGraphic, false );
  QVector<MssComServer::TMssPointGS> mssPoints;
  for(int i = 0, n = input.points.size(); i < n; ++i)
  {
    if(!input.controlPoints.contains(i))
    {
      MssComServer::TMssPointGS mssPoint;
      mssPoint.X = input.points[i].x();
      mssPoint.Y = input.points[i].y();
      mssPoints.append( mssPoint );
    }
  }
  int nPoints = mssPoints.size();
  LOG(QString("%1 points were specified").arg(nPoints));
  mssCreationItem->AddPoints(&mssPoints[0], nPoints);

  // Create drawing item, set control points
  mssDrawingItem = mssCreationItem->DrawingItem;
  if(input.finalized) {
    for(int i = 0, n = input.points.size(); i < n; ++i)
    {
      if(input.controlPoints.contains(i))
      {
        if(mssDrawingItem->IsCtrlPoint(i) == VARIANT_FALSE) {
          LOG("ERROR");
        }
        MssComServer::TMssPointGS mssPoint;
        mssPoint.X = input.points[i].x();
        mssPoint.Y = input.points[i].y();
        mssDrawingItem->EditPoint(i, &mssPoint);
      }
    }
  }
  return true;
}

bool VBSMilixServer::renderItem(MssComServer::IMssNPointGraphicTemplateGSPtr& mssNPointGraphic, MssComServer::IMssNPointDrawingCreationItemGSPtr &mssCreationItem, MssComServer::IMssNPointDrawingItemGSPtr& mssDrawingItem, const QRect& visibleExtent, SymbolOutput& output, QString& errorMsg)
{
  // Convert visible rect to TMssRectGS format
  MssComServer::TMssRectGS mssVisibleRect;
  mssVisibleRect.Left = visibleExtent.left();
  mssVisibleRect.Top = visibleExtent.top();
  mssVisibleRect.Right = visibleExtent.right();
  mssVisibleRect.Bottom = visibleExtent.bottom();

  int nPoints = mssDrawingItem->PointCount;
  QVector<MssComServer::TMssPointGS> mssPoints = QVector<MssComServer::TMssPointGS>( nPoints );
  mssDrawingItem->GetPoints( 0, &mssPoints[0], nPoints );
  LOG( QString( "Rendering N point symbol with %1 points" ).arg( mssPoints.size() ) );

  MssComServer::IMssSymbolGraphicGSPtr mssSymbolGraphic = mssNPointGraphic->RenderImageEx( &mssPoints[0], nPoints, mssVisibleRect, 72 );
  // The symbol graphic is invalid if it is completely outside the view extent
  if ( mssSymbolGraphic->IsValid )
  {
    output.svgXml = bstr2qstring(mssSymbolGraphic->CreateSvg()).toUtf8();
    output.offset.rx() = -mssSymbolGraphic->GetInsertPointOffsetX();
    output.offset.ry() = -mssSymbolGraphic->GetInsertPointOffsetY();
  } else {
	  LOG("Symbol outside viewing extent");
  }

  LOG(QString("Creation item has %1 points (min: %2)").arg(mssCreationItem->PointCount).arg(mssCreationItem->EnoughPointCount));
  for(int i = 0; i < nPoints; ++i) {
    output.adjustedPoints.append(QPoint(mssPoints[i].X, mssPoints[i].Y));
    if(mssDrawingItem->IsCtrlPoint(i) == VARIANT_TRUE) {
      output.controlPoints.append(i);
    }
  }
  LOG(QString("Returning %1 points (of which %2 control points)").arg(output.adjustedPoints.size()).arg(output.controlPoints.size()));
  return true;
}

int main( int argc, char* argv[] )
{
  QApplication app( argc, argv );
#ifdef DEBUG
  gTextEdit = new QPlainTextEdit();
  gTextEdit->show();
#endif
  int port;
  QString addr;
  if ( argc > 2 )
  {
    addr = argv[1];
    port = atoi( argv[2] );
  }
  VBSMilixServer s( addr, port );
  return app.exec();
}
