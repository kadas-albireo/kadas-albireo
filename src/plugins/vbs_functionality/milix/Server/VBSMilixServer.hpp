#ifndef VBSMILIXSERVER_HPP
#define VBSMILIXSERVER_HPP

#define _AFXDLL

#include <QMap>
#include <QObject>
#include <QPoint>
#include <SDKDDKVer.h>
#include <afxwin.h>
#include <iostream>

#import "C:\Program Files\gs-soft\MSS\MssComServerV5\TLB\Win64\MssComServerV5.tlb" rename("LoadLibrary", "MssLoadLibrary")

class QNetworkSession;
class QRect;
class QTcpServer;

class VBSMilixServer : public QObject
{
    Q_OBJECT

public:
    VBSMilixServer(const QString& addr, int port, QWidget* parent = 0);

private slots:
    void sessionOpened();
    void onNewConnection();
    void onSocketDataReady();
    void onSocketDisconnected();

private:
    struct SymbolInput {
      QString symbolXml;
      QList<QPoint> points;
      QList<int> controlPoints;
      bool finalized;
    };

    struct SymbolOutput {
      QByteArray svgXml;
      QPoint offset;
      QList<QPoint> adjustedPoints;
      QList<int> controlPoints;
    };

    QString mAddr;
    int mPort;
    QTcpServer* mTcpServer;
    QNetworkSession* mNetworkSession;
    QByteArray mRequestBuffer;
    int mRequestSize;
    int mSymbolSize;
    MssComServer::TMssLanguageTypeGS mLanguage;

    MssComServer::IMssSymbolProviderServiceGSPtr mMssService;
    MssComServer::IMssSymbolProviderGSPtr mMssSymbolProvider;
    MssComServer::IMssNPointDrawingTargetGSPtr mMssNPointDrawingTarget;

    QByteArray processCommand(QByteArray& request);
    bool getSymbolInfo(const QString& symbolXml, QString& name, QString &militaryName, QByteArray& svgXml, bool& hasVariablePoints, int& minPointCount, QString& errorMsg);
    bool renderSymbol(const QRect& visibleExtent, const SymbolInput& input, SymbolOutput& output, QString& errorMsg);
    bool insertPoint(const QRect& visibleExtent, const SymbolInput& input, const QPoint& newPoint, SymbolOutput& output, QString& errorMsg);
    bool movePoint(const QRect& visibleExtent, const SymbolInput& input, int moveIndex, const QPoint& newPos, SymbolOutput& output, QString& errorMsg);
    bool canDeletePoint(const SymbolInput& input, int index, bool& canDelete, QString& errorMsg);
    bool deletePoint(const QRect& visibleExtent, const SymbolInput& input, int deleteIndex, SymbolOutput& output, QString& errorMsg);
    bool editSymbol(const QRect& visibleExtent, const SymbolInput& input, QString& outputSymbolXml, QString &outputMilitaryName, SymbolOutput& output, QString& errorMsg);
    bool hitTest(const SymbolInput& input, const QPoint &clickPos, bool& hitTestResult, QString& errorMsg);
    void getLibraryVersionTags(QStringList& versionTags, QStringList &versionNames);

    bool createDrawingItem(const SymbolInput& input, QString& errorMsg, MssComServer::IMssNPointGraphicTemplateGSPtr& mssNPointGraphic, MssComServer::IMssNPointDrawingCreationItemGSPtr& mssCreationItem, MssComServer::IMssNPointDrawingItemGSPtr& mssDrawingItem);
    bool renderItem(MssComServer::IMssNPointGraphicTemplateGSPtr& mssNPointGraphic, MssComServer::IMssNPointDrawingCreationItemGSPtr& mssCreationItem, MssComServer::IMssNPointDrawingItemGSPtr& mssDrawingItem, const QRect& visibleExtent, SymbolOutput& output, QString& errorMsg);

    QString bstr2qstring(_bstr_t bstr){ return QString::fromWCharArray(( wchar_t* )bstr ); }
};

#endif // VBSMILIXSERVER_HPP
