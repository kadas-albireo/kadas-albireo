#ifndef VBSMILIXCLIENT_HPP
#define VBSMILIXCLIENT_HPP

#include <qglobal.h>
#include <QObject>
#include <QPoint>
#include <QPixmap>

class QNetworkSession;
class QRect;
class QProcess;
class QStringList;
class QTcpSocket;

class VBSMilixClient : public QObject
{
  Q_OBJECT
public:
  struct SymbolDesc {
    QString symbolId;
    QString name;
    QPixmap icon;
    bool hasVariablePoints;
    bool minNumPoints;
  };

  struct NPointSymbol {
    QString xml;
    QList<QPoint> points;
  };
  struct NPointSymbolPixmap {
    QPixmap pixmap;
    QPoint offset;
  };

  static bool getSymbols(const QStringList& symbolIds, QList<SymbolDesc>& result);
  static bool getSymbol(const QString& xml, QPixmap &pixmap, QString &name, bool &hasVariablePoints, int &minNumPoints);
  static bool getNPointSymbol(const QString& xml, const QList<QPoint>& points, const QRect &visibleExtent, QPixmap& pixmap, QPoint& offset);
  static bool getNPointSymbols(const QList<NPointSymbol>& symbols, const QRect& visibleExtent, QList< NPointSymbolPixmap >& result );
  static bool editSymbol(const QString& xml, QString& outXml);
  static const QString& lastError() { return instance()->mLastError; }

private:
  QProcess* mProcess;
  QNetworkSession* mNetworkSession;
  QTcpSocket* mTcpSocket;
  QString mLastError;

  VBSMilixClient() : mProcess( 0 ), mNetworkSession( 0 ), mTcpSocket( 0 ) {}
  static VBSMilixClient* instance(){ static VBSMilixClient i; return &i; }
  bool init();
  bool processRequest(const QByteArray& request, QByteArray& response, quint8 expectedReply);
  static QPixmap renderSvg(const QByteArray& xml);

private slots:
  void handleSocketError();
  void cleanup();
};

#endif // VBSMILIXCLIENT_HPP
