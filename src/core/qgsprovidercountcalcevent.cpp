/***************************************************************************
      qgsprovidercountcalcevent.cpp  -  Notification that the exact count
                                        of a layer has been calculated.
                             -------------------
    begin                : Feb 1, 2005
    copyright            : (C) 2005 by Brendan Morley
    email                : morb at ozemail dot com dot au
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include "qgsprovidercountcalcevent.h"
//Added by qt3to4:
#include <QCustomEvent>

QgsProviderCountCalcEvent::QgsProviderCountCalcEvent( long numberFeatures )
    : QCustomEvent( QGis::ProviderCountCalcEvent ),
      n( numberFeatures )
{
  // NO-OP
}


long QgsProviderCountCalcEvent::numberFeatures() const
{
  return n;
}

