/***************************************************************************
                             qgsmarkercatalogue.h
                             -------------------
    begin                : March 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef QGSMARKERCATALOGUE_H
#define QGSMARKERCATALOGUE_H

#include <QStringList>

class QImage;
class QString;
class QPicture;
class QPen;
class QBrush;
class QPainter;


/** Catalogue of point symbols */
class CORE_EXPORT QgsMarkerCatalogue
{

  public:
    //! Destructor
    ~QgsMarkerCatalogue();

    //! Access to canonical QgsMarkerCatalogue instance
    static QgsMarkerCatalogue *instance();

    /**List of available markers*/
    QStringList list();

    /** Returns pixmap of the marker
     * \param fullName full name, e.g. hard:circle, svg:/home/usr1/marker1.svg
     */
    QImage imageMarker( QString fullName, double size, QPen pen, QBrush brush, bool qtBug = true );

    /** Returns qpicture of the marker
     * \param fullName full name, e.g. hard:circle, svg:/home/usr1/marker1.svg
     */
    QPicture pictureMarker( QString fullName, double size, QPen pen, QBrush brush, bool qtBug = true );

    /** Returns a pixmap given a file name of a svg marker
     *  NOTE: this method needs to be public static for QgsMarkerDialog::visualizeMarkers */
    static void svgMarker( QPainter * thepPainter, QString name, double size );
  private:

    /**Constructor*/
    QgsMarkerCatalogue();

    static QgsMarkerCatalogue *mMarkerCatalogue;

    /** List of availabel markers*/
    QStringList mList;

    /** Hard coded */
    void hardMarker( QPainter * thepPainter, int imageSize, QString name, double size, QPen pen, QBrush brush, bool qtBug = true );

};

#endif // QGSMARKERCATALOGUE_H


