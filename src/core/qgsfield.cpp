/***************************************************************************
       qgsfield.cpp - Describes a field in a layer or table
        --------------------------------------
       Date                 : 01-Jan-2004
       Copyright            : (C) 2004 by Gary E.Sherman
       email                : sherman at mrcc.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsfield.h"

static const char * const ident_ =
  "$Id$";

/*
QgsField::QgsField(QString nam, QString typ, int len, int prec, bool num,
                   QString comment)
    :mName(nam), mType(typ), mLength(len), mPrecision(prec), mNumeric(num),
     mComment(comment)
{
  // This function used to lower case the field name since some stores
  // use upper case (eg. shapefiles), but that caused problems with
  // attribute actions getting confused between uppercase and
  // lowercase versions of the attribute names, so just leave the
  // names how they are now.
}*/

QgsField::QgsField( QString name, QVariant::Type type, QString typeName, int len, int prec, QString comment )
    : mName( name ), mType( type ), mTypeName( typeName ),
    mLength( len ), mPrecision( prec ), mComment( comment )
{
}


QgsField::~QgsField()
{
}

bool QgsField::operator==( const QgsField& other ) const
{
  return (( mName == other.mName ) && ( mType == other.mType ) && ( mTypeName == other.mTypeName )
          && ( mLength == other.mLength ) && ( mPrecision == other.mPrecision ) );
}

const QString & QgsField::name() const
{
  return mName;
}

QVariant::Type QgsField::type() const
{
  return mType;
}

const QString & QgsField::typeName() const
{
  return mTypeName;
}

int QgsField::length() const
{
  return mLength;
}

int QgsField::precision() const
{
  return mPrecision;
}

const QString & QgsField::comment() const
{
  return mComment;
}

void QgsField::setName( const QString & nam )
{
  mName = nam;
}

void QgsField::setType( QVariant::Type type )
{
  mType = type;
}

void QgsField::setTypeName( const QString & typeName )
{
  mTypeName = typeName;
}

void QgsField::setLength( int len )
{
  mLength = len;
}
void QgsField::setPrecision( int prec )
{
  mPrecision = prec;
}

void QgsField::setComment( const QString & comment )
{
  mComment = comment;
}
