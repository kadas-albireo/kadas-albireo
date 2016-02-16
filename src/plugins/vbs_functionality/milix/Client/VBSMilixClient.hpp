#ifndef VBSMILIXCLIENT_HPP
#define VBSMILIXCLIENT_HPP

#include <qglobal.h>
#include <QMap>
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
    QString militaryName;
    QImage icon;
    bool hasVariablePoints;
    int minNumPoints;
  };

  struct NPointSymbol {
    NPointSymbol(const QString& _xml, const QList<QPoint>& _points, const QList<int>& _controlPoints, bool _finalized)
      : xml(_xml), points(_points), controlPoints(_controlPoints), finalized(_finalized) {}

    QString xml;
    QList<QPoint> points;
    QList<int> controlPoints;
    bool finalized;
  };

  struct NPointSymbolGraphic {
    QImage graphic;
    QPoint offset;
    QList<QPoint> adjustedPoints;
    QList<int> controlPoints;
  };

  static const int SymbolSize;

  static bool init() { return instance()->initialize(); }
  static bool getSymbol(const QString& symbolId, SymbolDesc& result);
  static bool getSymbols(const QStringList& symbolIds, QList<SymbolDesc>& result);
  static bool appendPoint(const QRect &visibleExtent, const NPointSymbol& symbol, const QPoint& newPoint, NPointSymbolGraphic& result);
  static bool insertPoint(const QRect &visibleExtent, const NPointSymbol& symbol, const QPoint& newPoint, NPointSymbolGraphic& result);
  static bool movePoint(const QRect &visibleExtent, const NPointSymbol& symbol, int index, const QPoint& newPos, NPointSymbolGraphic& result);
  static bool canDeletePoint(const NPointSymbol& symbol, int index, bool& canDelete);
  static bool deletePoint(const QRect &visibleExtent, const NPointSymbol& symbol, int index, NPointSymbolGraphic& result);
  static bool editSymbol(const QRect &visibleExtent, const NPointSymbol& symbol, QString& newSymbolXml, QString& newSymbolMilitaryName, NPointSymbolGraphic& result);
  static bool updateSymbol(const QRect& visibleExtent, const NPointSymbol& symbol, NPointSymbolGraphic& result);
  static bool updateSymbols(const QRect& visibleExtent, const QList<NPointSymbol>& symbols, QList<NPointSymbolGraphic>& result);
  static const QString& lastError() { return instance()->mLastError; }
  static const QString& libraryVersionTag() {  return instance()->mLibraryVersionTag; }

private:
  QProcess* mProcess;
  QNetworkSession* mNetworkSession;
  QTcpSocket* mTcpSocket;
  QString mLastError;
  QString mLibraryVersionTag;

  VBSMilixClient() : mProcess( 0 ), mNetworkSession( 0 ), mTcpSocket( 0 ) {}
  static VBSMilixClient* instance(){ static VBSMilixClient i; return &i; }
  bool initialize();
  bool processRequest(const QByteArray& request, QByteArray& response, quint8 expectedReply);
  static QImage renderSvg(const QByteArray& xml);

private slots:
  void handleSocketError();
  void cleanup();
};

#endif // VBSMILIXCLIENT_HPP
