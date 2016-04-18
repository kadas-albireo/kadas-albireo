/***************************************************************************
    qgsddalconnpool.cpp
    ---------------------
    begin                : March 2016
    copyright            : (C) 2016 by Sandro Mani
    email                : smani at sourcepole dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsgdalconnpool.h"


QgsGdalConnPool* QgsGdalConnPool::instance()
{
  static QgsGdalConnPool sInstance;
  return &sInstance;
}

QgsGdalConnPool::QgsGdalConnPool() : QgsConnectionPool<QgsGdalConn*, QgsGdalConnPoolGroup>()
{
  QgsDebugCall;
}

QgsGdalConnPool::~QgsGdalConnPool()
{
  QgsDebugCall;
}
