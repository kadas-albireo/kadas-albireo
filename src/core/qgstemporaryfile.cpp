/***************************************************************************
 *  qgstemporaryfile.cpp - Class for managing temporary files              *
 *  -------------------                                                    *
 *  begin                : Nov 11, 2015                                    *
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

#include "qgstemporaryfile.h"
#include <QTemporaryFile>
#include <QDir>

QgsTemporaryFile::~QgsTemporaryFile()
{
  foreach ( QTemporaryFile* file, mFiles )
  {
    QFile( file->fileName() + ".aux.xml" ).remove();
  }
}

QString QgsTemporaryFile::createNewFile( const QString& templateName )
{
  return instance()->_createNewFile( templateName );
}

QgsTemporaryFile* QgsTemporaryFile::instance()
{
  static QgsTemporaryFile i;
  return &i;
}

QString QgsTemporaryFile::_createNewFile( const QString& templateName )
{
  QTemporaryFile* tmpFile = new QTemporaryFile( QDir::temp().absoluteFilePath( "XXXXXX_" + templateName ), this );
  mFiles.append( tmpFile );
  return tmpFile->open() ? tmpFile->fileName() : QString();
}
