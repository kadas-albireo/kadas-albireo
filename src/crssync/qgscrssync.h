/***************************************************************************
                              qgscrssync.h
                              --------------
  begin                : September 20th, 2017
  copyright            : (C) 2017 by Sandro Mani
  email                : manisandro at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSCRSSYNCDB_H
#define QGSCRSSYNCDB_H

namespace QgsCrsSync
{

  /*! Update proj.4 parameters in our database from proj.4
   * @returns number of updated CRS on success and
   *   negative number of failed updates in case of errors.
   */
  int syncDb();

} // QgsCrsSync

#endif // QGSCRSSYNCDB_H
