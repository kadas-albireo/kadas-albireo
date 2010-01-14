/***************************************************************************
                          qgsgpsdetector.cpp  -  description
                          --------------------
    begin                : January 13th, 2009
    copyright            : (C) 2009 by Juergen E. Fischer
    email                : jef at norbit dot de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgpsdetector.h"
#include "qextserialenumerator.h"
#include "qgslogger.h"
#include "qgsgpsconnection.h"
#include "qgsnmeaconnection.h"

#include <QStringList>
#include <QFileInfo>
#include <QTimer>

QList< QPair<QString, QString> > QgsGPSDetector::availablePorts()
{
  QList< QPair<QString, QString> > devs;

#ifdef linux
  // look for linux serial devices
  foreach( QString linuxDev, QStringList() << "/dev/ttyS%1" << "/dev/ttyUSB%1" << "/dev/rfcomm%1" )
  {
    for ( int i = 0; i < 10; ++i )
    {
      if ( QFileInfo( linuxDev.arg( i ) ).exists() )
      {
        devs << QPair<QString, QString>( linuxDev.arg( i ), linuxDev.arg( i ) );
      }
    }
  }
#endif

#ifdef __FreeBSD__ // freebsd
  // and freebsd devices (untested)
  foreach( QString freebsdDev, QStringList() << "/dev/cuaa%1" << "/dev/ucom%1" )
  {
    for ( int i = 0; i < 10; ++i )
    {
      if ( QFileInfo( freebsdDev.arg( i ) ).exists() )
      {
        devs << QPair<QString, QString>( freebsdDev.arg( i ), freebsdDev.arg( i ) );
      }
    }
  }
#endif

#ifdef sparc
  // and solaris devices (also untested)
  QString solarisDev( "/dev/cua/%1" );
  for ( char i = 'a'; i < 'k'; ++i )
  {
    if ( QFileInfo( solarisDev.arg( i ) ).exists() )
    {
      devs << QPair<QString, QString>( solarisDev.arg( i ), solarisDev.arg( i ) );
    }
  }
#endif

#if defined(Q_WS_WIN) || defined(Q_WS_MAC)
  QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
  foreach( QextPortInfo port, ports )
  {
    devs << QPair<QString, QString>( port.portName, port.friendName );
  }
#endif

  // OpenBSD, NetBSD etc? Anyone?

  return devs;
}

QgsGPSDetector::QgsGPSDetector( QString portName )
{
  mConn = 0;
  mBaudList << BAUD4800 << BAUD9600 << BAUD38400;

  if ( portName.isEmpty() )
  {
    mPortList = availablePorts();
  }
  else
  {
    mPortList << QPair<QString, QString>( portName, portName );
  }

  mPortIndex = 0;
  mBaudIndex = -1;

  advance();
}

QgsGPSDetector::~QgsGPSDetector()
{
  if ( mConn )
    delete mConn;
}

void QgsGPSDetector::advance()
{
  if ( mConn )
  {
    delete mConn;
  }

  QextSerialPort *port = 0;

  do
  {
    mBaudIndex++;
    if ( mBaudIndex == mBaudList.size() )
    {
      mBaudIndex = 0;
      mPortIndex++;
    }

    if ( mPortIndex == mPortList.size() )
    {
      emit detectionFailed();
      deleteLater();
      return;
    }

    if ( port )
      delete port;

    port = new QextSerialPort( mPortList[ mPortIndex ].first, QextSerialPort::EventDriven );
    port->setBaudRate( mBaudList[ mBaudIndex ] );
    port->setFlowControl( FLOW_OFF );
    port->setParity( PAR_NONE );
    port->setDataBits( DATA_8 );
    port->setStopBits( STOP_1 );
  }
  while ( !port->open( QIODevice::ReadOnly | QIODevice::Unbuffered ) );

  mConn = new QgsNMEAConnection( port );
  connect( mConn, SIGNAL( stateChanged( const QgsGPSInformation & ) ), this, SLOT( detected( const QgsGPSInformation & ) ) );
  connect( mConn, SIGNAL( destroyed( QObject * ) ), this, SLOT( connDestroyed( QObject * ) ) );

  // leave 2s to pickup a valid string
  QTimer::singleShot( 2000, this, SLOT( advance() ) );
}

void QgsGPSDetector::detected( const QgsGPSInformation& info )
{
  if ( !mConn )
  {
    // advance if connection was destroyed
    advance();
  }
  else if ( mConn->status() == QgsGPSConnection::GPSDataReceived )
  {
    // signal detection
    QgsGPSConnection *conn = mConn;
    mConn = 0;
    emit detected( conn );
    deleteLater();
  }
}

void QgsGPSDetector::connDestroyed( QObject *obj )
{
  if ( obj == mConn )
  {
    mConn = 0;
  }
}
