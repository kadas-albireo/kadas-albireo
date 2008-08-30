/***************************************************************************
    qgsencodingfiledialog.h - File dialog which queries the encoding type
     --------------------------------------
    Date                 : 16-Feb-2005
    Copyright            : (C) 2005 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSENCODINGFILEDIALOG_H
#define QGSENCODINGFILEDIALOG_H

#include <QFileDialog>
class QComboBox;

/** \ingroup gui
 * A file dialog which lets the user select the prefered encoding type for a data provider.
 **/
class GUI_EXPORT QgsEncodingFileDialog: public QFileDialog
{
    Q_OBJECT
  public:
    QgsEncodingFileDialog( QWidget * parent = 0,
                           const QString & caption = QString(), const QString & directory = QString(),
                           const QString & filter = QString(), const QString & encoding = QString() );
    ~QgsEncodingFileDialog();
    /**Returns a string describing the choosen encoding*/
    QString encoding() const;
  private:
    /**Box to choose the encoding type*/
    QComboBox* mEncodingComboBox;
};

#endif
