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

#include <iostream>
#include "qgsfield.h"

#include <qstring.h>
#include <iostream>

static const char * const ident_ = 
   "$Id$";


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
}

QgsField::~QgsField()
{
}

bool QgsField::operator==(const QgsField other) const
{
    return ((mName == other.mName) && (mType == other.mType)
	    && (mLength == other.mLength) && (mPrecision == other.mPrecision));
}

bool QgsField::operator!=(const QgsField other) const
{
    return !(*this == other);
}

QString const & QgsField::name() const
{
  return mName;
}

QString const & QgsField::type() const
{
  return mType;
}

int QgsField::length() const
{
  return mLength;
}

int QgsField::precision() const
{
  return mPrecision;
}

bool QgsField::isNumeric() const
{
  return mNumeric;
}

QString const & QgsField::comment() const
{
  return mComment;
}

void QgsField::setName(QString const & nam)
{
  mName = nam;
}

void QgsField::setType(QString const & typ)
{
  mType = typ;
}
void QgsField::setLength(int len)
{
  mLength = len;
}
void QgsField::setPrecision(int prec)
{
  mPrecision = prec;
}
void QgsField::setNumeric(bool num)
{
  mNumeric = num;
}
void QgsField::setComment(QString comment)
{
  mComment = comment;
}
