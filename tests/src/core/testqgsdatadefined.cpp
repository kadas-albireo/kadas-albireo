/***************************************************************************
     testqgsdatadefined.cpp
     ----------------------
    Date                 : November 2014
    Copyright            : (C) 2014 Nyall Dawson
    Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QtTest>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QSettings>

#include <qgsdatadefined.h>

/** \ingroup UnitTests
 * Unit tests for QgsDataDefined
 */
class TestQgsDataDefined: public QObject
{
    Q_OBJECT;

  private slots:
    void initTestCase();// will be called before the first testfunction is executed.
    void cleanupTestCase();// will be called after the last testfunction was executed.
    void init();// will be called before each testfunction is executed.
    void cleanup();// will be called after every testfunction.
    void create();//test creating a data defined container
    void gettersSetters(); //test getters and setters
    void defaultValues(); //test hasDefaultValues method

  private:
};

void TestQgsDataDefined::initTestCase()
{

}

void TestQgsDataDefined::cleanupTestCase()
{

}

void TestQgsDataDefined::init()
{

}

void TestQgsDataDefined::cleanup()
{

}

void TestQgsDataDefined::create()
{
  QgsDataDefined* dd = new QgsDataDefined( true, true, QString( "exp" ), QString( "field" ) );
  QVERIFY( dd->isActive() );
  QVERIFY( dd->useExpression() );
  QCOMPARE( dd->expressionString(), QString( "exp" ) );
  QCOMPARE( dd->field(), QString( "field" ) );
  delete dd;
}

void TestQgsDataDefined::gettersSetters()
{
  QgsDataDefined dd;
  dd.setActive( false );
  QVERIFY( !dd.isActive() );
  dd.setActive( true );
  QVERIFY( dd.isActive() );

  dd.setUseExpression( false );
  QVERIFY( !dd.useExpression() );
  dd.setUseExpression( true );
  QVERIFY( dd.useExpression() );

  dd.setExpressionString( QString( "expression" ) );
  QCOMPARE( dd.expressionString(), QString( "expression" ) );

  dd.setField( QString( "field" ) );
  QCOMPARE( dd.field(), QString( "field" ) );
}

void TestQgsDataDefined::defaultValues()
{
  QgsDataDefined* dd = new QgsDataDefined();
  QVERIFY( dd->hasDefaultValues() );
  dd->setActive( true );
  QVERIFY( !dd->hasDefaultValues() );
  delete dd;
  dd = new QgsDataDefined();
  dd->setUseExpression( true );
  QVERIFY( !dd->hasDefaultValues() );
  delete dd;
  dd = new QgsDataDefined();
  dd->setExpressionString( QString( "expression" ) );
  QVERIFY( !dd->hasDefaultValues() );
  delete dd;
  dd = new QgsDataDefined();
  dd->setField( QString( "field" ) );
  QVERIFY( !dd->hasDefaultValues() );
  delete dd;
}

QTEST_MAIN( TestQgsDataDefined )
#include "testqgsdatadefined.moc"
