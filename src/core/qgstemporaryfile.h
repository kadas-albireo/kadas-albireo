/***************************************************************************
 *  qgstemporaryfile.h - Class for managing temporary files                *
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

#ifndef QGSTEMPORARYFILE_H
#define QGSTEMPORARYFILE_H

#include <QObject>
#include <QList>

class QTemporaryFile;

class QgsTemporaryFile : public QObject
{
  public:
    static QString createNewFile( const QString &templateName );

  private:
    QList<QTemporaryFile*> mFiles;
    QgsTemporaryFile() {}
    ~QgsTemporaryFile();
    static QgsTemporaryFile* instance();
    QString _createNewFile( const QString &templateName );
};

#endif // QGSTEMPORARYFILE_H
