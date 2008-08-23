/***************************************************************************
                          qgscontinuouscolordialog.h
                        Continuous color renderer dialog
                             -------------------
    begin                : 2004-02-12
    copyright            : (C) 2004 by Gary E.Sherman
    email                : sherman at mrcc.com
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

#ifndef QGSCONTINUOUSCOLORDIALOG_H
#define QGSCONTINUOUSCOLORDIALOG_H

#include "ui_qgscontinuouscolordialogbase.h"
#include <map>

class QgsVectorLayer;


class QgsContinuousColorDialog: public QDialog, private Ui::QgsContinuousColorDialogBase
{
    Q_OBJECT

  public:
    QgsContinuousColorDialog( QgsVectorLayer* layer );
    ~QgsContinuousColorDialog();

  public slots:
    void apply();

  protected slots:
    void selectMinimumColor();
    void selectMaximumColor();
    void on_cb_polygonOutline_clicked();

  protected:
    QgsVectorLayer* mVectorLayer;
    /**Stores the relationship between provider field indices and field selection
       combobox indices. First is the combobox index, second is the provider field index */
    std::map<int, int> mFieldMap;

  private:
    /** Default constructor is private, do not use this */
    QgsContinuousColorDialog();

};

#endif
