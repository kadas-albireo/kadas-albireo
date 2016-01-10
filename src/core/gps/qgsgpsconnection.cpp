/***************************************************************************
                          qgsgpsconnection.cpp  -  description
                          --------------------
    begin                : November 30th, 2009
    copyright            : (C) 2009 by Marco Hugentobler
    email                : marco at hugis dot net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgpsconnection.h"

#include <QCoreApplication>
#include <QTime>
#include <QIODevice>
#include <QStringList>
#include <QFileInfo>

#include "qextserialport.h"
#include "qextserialenumerator.h"

#include "qgsnmeaconnection.h"
#include "qgslogger.h"

#include "info.h"

QgsGPSConnection::QgsGPSConnection( QIODevice* dev ): QObject( 0 ), mSource( dev ), mStatus( NotConnected )
{
  clearLastGPSInformation();
  QObject::connect( dev, SIGNAL( readyRead() ), this, SLOT( parseData() ) );
}

QgsGPSConnection::~QgsGPSConnection()
{
  cleanupSource();
}

bool QgsGPSConnection::connect()
{
  if ( !mSource )
  {
    return false;
  }

  bool connected = mSource->open( QIODevice::ReadWrite | QIODevice::Unbuffered );
  if ( connected )
  {
    mStatus = Connected;
  }
  return connected;
}

bool QgsGPSConnection::close()
{
  if ( !mSource )
  {
    return false;
  }

  mSource->close();
  return true;
}

void QgsGPSConnection::cleanupSource()
{
  if ( mSource )
  {
    mSource->close();
  }
  delete mSource;
  mSource = 0;
}

void QgsGPSConnection::setSource( QIODevice* source )
{
  cleanupSource();
  mSource = source;
  clearLastGPSInformation();
}

void QgsGPSConnection::clearLastGPSInformation()
{
  mLastGPSInformation = emptyGPSInformation();
}

bool QgsGPSConnection::gpsInfoValid( const QgsGPSInformation& info )
{
  bool valid = false;
  if ( info.status == 'V' || info.fixType == NMEA_FIX_BAD || info.quality == 0 ) // some sources say that 'V' indicates position fix, but is below acceptable quality
  {
    return false;
  }
  else if ( info.fixType == NMEA_FIX_2D )
  {
    valid = true;
  }
  else if ( info.status == 'A' || info.fixType == NMEA_FIX_3D || info.quality > 0 ) // good
  {
    valid = true;
  }

  return valid;
}

QgsGPSInformation QgsGPSConnection::emptyGPSInformation()
{
  QgsGPSInformation gpsInfo;
  gpsInfo.direction = 0;
  gpsInfo.elevation = 0;
  gpsInfo.hdop = 0;
  gpsInfo.latitude = 0;
  gpsInfo.longitude = 0;
  gpsInfo.pdop = 0;
  gpsInfo.satellitesInView.clear();
  gpsInfo.speed = 0;
  gpsInfo.vdop = 0;
  gpsInfo.hacc = -1;
  gpsInfo.vacc = -1;
  gpsInfo.quality = -1;  // valid values: 0,1,2, maybe others
  gpsInfo.satellitesUsed = 0;
  gpsInfo.fixMode = ' ';
  gpsInfo.fixType = 0; // valid values: 1,2,3
  gpsInfo.status = ' '; // valid values: A,V
  gpsInfo.utcDateTime.setDate( QDate() );
  gpsInfo.satPrn.clear();
  gpsInfo.utcDateTime.setTime( QTime() );
  gpsInfo.satInfoComplete = false;
  return gpsInfo;
}
