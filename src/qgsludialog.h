/***************************************************************************
                         qgsludialog.h  -  description
                             -------------------
    begin                : September 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef QGSLUDIALOG_H
#define QGSLUDIALOG_H

#include "qgsludialogbase.uic.h"

class QgsLUDialog: public QgsLUDialogBase
{
    Q_OBJECT
 public:
    QgsLUDialog();
    ~QgsLUDialog();
    QString lowerValue() const;
    void setLowerValue(QString val);
    QString upperValue() const;
    void setUpperValue(QString val);
};

#endif
