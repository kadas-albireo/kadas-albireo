/***************************************************************************
 *  qgsvbscrashhandler.h                                                   *
 *  -------------------                                                    *
 *  begin                : Sep 15, 2015                                    *
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

#ifndef QGSVBSCRASHHANDLER_H
#define QGSVBSCRASHHANDLER_H

class QgsVBSCrashHandler {
public:
	QgsVBSCrashHandler();
	~QgsVBSCrashHandler();
private:
	bool mHandlerInstalled;
};

#endif // QGSVBSCRASHHANDLER_H
