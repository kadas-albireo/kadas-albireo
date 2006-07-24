/***************************************************************************
 *   Copyright (C) 2003 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef QGSCOPYRIGHTLABELPLUGINGUI_H
#define QGSCOPYRIGHTLABELPLUGINGUI_H

#include <ui_pluginguibase.h>
#include <qfont.h>
#include <qcolor.h>

/**
@author Tim Sutton
*/
class QgsCopyrightLabelPluginGui : public QDialog, private Ui::QgsCopyrightLabelPluginGuiBase
{
Q_OBJECT;
public:
    QgsCopyrightLabelPluginGui();
    QgsCopyrightLabelPluginGui( QWidget* parent = 0, Qt::WFlags fl = 0 );
    ~QgsCopyrightLabelPluginGui();
    void setText(QString);
    void setPlacementLabels(QStringList&);
    void setPlacement(int);

public slots:
    void on_pbnOK_clicked();
    void on_pbnCancel_clicked();    
    void on_pbnHelp_clicked();
    void setEnabled(bool); 
     
private:
    static const int context_id = 32338213;
    
signals:
   //void drawRasterLayer(QString);
   //void drawVectorrLayer(QString,QString,QString);
   void changeFont(QFont);
   void changeLabel(QString);
   void changeColor(QColor);
   void changePlacement(int);
   void enableCopyrightLabel(bool);
  
};

#endif
