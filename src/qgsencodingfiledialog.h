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

#include "qgsvectordataprovider.h"
#include <q3combobox.h>
#include <q3filedialog.h>

/**A file dialog which lets the user select the prefered encoding type for a data provider*/
class QgsEncodingFileDialog: public Q3FileDialog
{
    Q_OBJECT
 public:
  QgsEncodingFileDialog(const QString & dirName, const QString& filter, QWidget * parent, const QString name, const QString currentencoding=QTextCodec::codecForLocale()->name());
    ~QgsEncodingFileDialog();
    /**Returns a string describing the choosen encoding*/
    QString encoding() const;
 private:
    /**Box to choose the encoding type*/
    QComboBox* mEncodingComboBox;
};

#endif
