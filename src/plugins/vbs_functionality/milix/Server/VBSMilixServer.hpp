#ifndef VBSMILIXSERVER_HPP
#define VBSMILIXSERVER_HPP

#define _AFXDLL

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
    ~VBSMilixServer();

private slots:
    void sessionOpened();
    void onNewConnection();
    void onSocketDataReady();
    void onSocketDisconnected();

private:
	QString mAddr;
	int mPort;
    QTcpServer* mTcpServer;
    QNetworkSession* mNetworkSession;
    MssComServer::IMssSymbolProviderServiceGSPtr mMssService;
    MssComServer::IMssSymbolProviderGSPtr mMssSymbolProvider;
    MssComServer::IMssSymbolFormatGSPtr mMssSymbolFormat;
    MssComServer::IMssSymbolEditorGSPtr mMssSymbolEditor;
	MssComServer::IMssNPointDrawingTargetGSPtr mMssNPointDrawingTarget;

    QByteArray processCommand(QByteArray& request);
    bool getSymbolInfo(const QString& symbolXml, QString& name, QString& svgXml, bool& hasVariablePoints, int& minPointCount, QString& errorMsg);
    bool renderSymbol(const QRect& visibleExtent, const QString& symbolXml, const QList<QPoint>& points, QByteArray& svgXml, QPoint& offset, QString& errorMsg);
};

#endif // VBSMILIXSERVER_HPP
