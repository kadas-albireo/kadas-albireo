/***************************************************************************
 *  MilXClient.hpp                                                         *
 *  --------------                                                         *
 *  begin                : Oct 01, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MILXCLIENT_HPP
#define MILXCLIENT_HPP

#include <qglobal.h>
#include <QMap>
#include <QObject>
#include <QPoint>
#include <QPixmap>
#include <QThread>

class QNetworkSession;
class QRect;
class QProcess;
class QStringList;
class QTcpSocket;


class MilXClientWorker : public QObject {
  Q_OBJECT
public:
  MilXClientWorker(QObject* parent = 0);

public slots:
  bool initialize();
  bool getCurrentLibraryVersionTag(QString& versionTag);
  bool processRequest(const QByteArray& request, QByteArray& response, quint8 expectedReply);

private:
  QProcess* mProcess;
  QNetworkSession* mNetworkSession;
  QTcpSocket* mTcpSocket;
  QString mLastError;
  QString mLibraryVersionTag;

private slots:
  void handleSocketError();
  void cleanup();
};


class MilXClient : public QThread
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

  static void setSymbolSize(int value) { instance()->mSymbolSize = value; instance()->setSymbolOptions(instance()->mSymbolSize, instance()->mLineWidth, instance()->mWorkMode); }
  static void setLineWidth(int value) { instance()->mLineWidth = value; instance()->setSymbolOptions(instance()->mSymbolSize, instance()->mLineWidth, instance()->mWorkMode); }
  static int getSymbolSize() { return instance()->mSymbolSize; }
  static int getLineWidth() { return instance()->mLineWidth; }
  static void setWorkMode(int workMode) { instance()->mWorkMode = workMode; instance()->setSymbolOptions(instance()->mSymbolSize, instance()->mLineWidth, instance()->mWorkMode); }

  static bool init();
  static bool getSymbol(const QString& symbolId, SymbolDesc& result);
  static bool getSymbols(const QStringList& symbolIds, QList<SymbolDesc>& result);
  static bool appendPoint(const QRect &visibleExtent, const NPointSymbol& symbol, const QPoint& newPoint, NPointSymbolGraphic& result);
  static bool insertPoint(const QRect &visibleExtent, const NPointSymbol& symbol, const QPoint& newPoint, NPointSymbolGraphic& result);
  static bool movePoint(const QRect &visibleExtent, const NPointSymbol& symbol, int index, const QPoint& newPos, NPointSymbolGraphic& result);
  static bool canDeletePoint(const NPointSymbol& symbol, int index, bool& canDelete);
  static bool deletePoint(const QRect &visibleExtent, const NPointSymbol& symbol, int index, NPointSymbolGraphic& result);
  static bool editSymbol(const QRect &visibleExtent, const NPointSymbol& symbol, QString& newSymbolXml, QString& newSymbolMilitaryName, NPointSymbolGraphic& result);
  static bool updateSymbol(const QRect& visibleExtent, const NPointSymbol& symbol, NPointSymbolGraphic& result, bool returnPoints);
  static bool updateSymbols(const QRect& visibleExtent, const QList<NPointSymbol>& symbols, QList<NPointSymbolGraphic>& result);
  static bool validateSymbolXml(const QString& symbolXml, const QString &mssVersion, QString &adjustedSymbolXml, bool& valid, QString& messages);
  static bool downgradeSymbolXml(const QString& symbolXml, const QString &mssVersion, QString &adjustedSymbolXml, bool& valid, QString& messages);
  static bool hitTest(const NPointSymbol& symbol, const QPoint& clickPos, bool& hitTestResult);
  static bool pickSymbol(const QList<NPointSymbol>& symbols, const QPoint& clickPos, int& selectedSymbol);
  static bool getSupportedLibraryVersionTags(QStringList& versionTags, QStringList& versionNames);
  static bool getCurrentLibraryVersionTag(QString& versionTag);
  static bool getControlPoints(const QString& symbolXml, int nPoints, QList<int>& controlPoints);
  static bool getMilitaryName(const QString& symbolXml, QString& militaryName);

private:
  MilXClientWorker mWorker;
  int mSymbolSize;
  int mLineWidth;
  int mWorkMode;

  MilXClient();
  ~MilXClient();
  static MilXClient* instance(){ static MilXClient i; return &i; }
  static QImage renderSvg(const QByteArray& xml);

  bool processRequest( const QByteArray& request, QByteArray& response, quint8 expectedReply );
  bool setSymbolOptions(int symbolSize, int lineWidth, int workMode );
};

#endif // MILXCLIENT_HPP
