/***************************************************************************
                         qgsgraduatedsymboldialog.h  -  description
                             -------------------
    begin                : Oct 2003
    copyright            : (C) 2003 by Marco Hugentobler
    email                : mhugent@geo.unizh.ch
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

#ifndef QGSGRADUATEDSYMBOLDIALOG_H
#define QGSGRADUATEDSYMBOLDIALOG_H

#include "ui_qgsgraduatedsymboldialogbase.h"
#include "qgssinglesymboldialog.h"
#include <map>

class QgsVectorLayer;


class QgsGraduatedSymbolDialog: public QDialog, private Ui::QgsGraduatedSymbolDialogBase
{
    Q_OBJECT
 public:
    /**Enumeration describing the automatic settings of values*/
    enum mode{EMPTY, EQUAL_INTERVAL, QUANTILES};
    QgsGraduatedSymbolDialog(QgsVectorLayer* layer);
    ~QgsGraduatedSymbolDialog();
 public slots:
     void apply();
 protected slots:
     /**Changes only the number of classes*/
     void adjustNumberOfClasses();
     /**Sets a new classification field and a new classification mode*/
     void adjustClassification();
     /**Changes the display of the single symbol dialog*/
     void changeCurrentValue();
     /**Writes changes in the single symbol dialog to the corresponding QgsRangeRenderItem*/
     void applySymbologyChanges();
     /**Shows a dialog to modify lower and upper values*/
     void modifyClass(QListWidgetItem* item);
 protected:
     /**Pointer to the associated vector layer*/
     QgsVectorLayer* mVectorLayer;
     /**Stores the names and numbers of the fields with numeric values*/
     std::map<QString,int> mFieldMap;
     /**Stores the classes*/
     std::map<QString,QgsSymbol*> mEntries;
     /**Dialog which shows the settings of the activated class*/
     QgsSingleSymbolDialog sydialog;
     int mClassificationField;

 protected slots:
     /**Removes a class from the classification*/
     void deleteCurrentClass();

 private:
     /**Default constructor is privat to not use is*/
     QgsGraduatedSymbolDialog();
};

#endif
