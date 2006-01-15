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
#include "plugingui.h"
#include "qgsapplication.h"

#include <QPainter>
#include <cmath>

#include <iostream>

QgsNorthArrowPluginGui::QgsNorthArrowPluginGui() : QDialog()
{
  setupUi(this);
  //temporary hack until this is implemented
  tabNorthArrowOptions->removePage( tabIcon );
  rotatePixmap(0);
}

QgsNorthArrowPluginGui::QgsNorthArrowPluginGui(QWidget* parent, Qt::WFlags fl)
: QDialog(parent, fl)
{
  setupUi(this);
  //temporary hack until this is implemented
  tabNorthArrowOptions->removePage( tabIcon );
}  

QgsNorthArrowPluginGui::~QgsNorthArrowPluginGui()
{
}

void QgsNorthArrowPluginGui::on_pbnOK_clicked()
{
  // Hide the dialog
  hide();
  //close the dialog
  emit rotationChanged(sliderRotation->value());
  emit enableAutomatic(cboxAutomatic->isChecked());
  emit changePlacement(cboPlacement->currentText());
  emit enableNorthArrow(cboxShow->isChecked());
  emit needToRefresh();

  done(1);
}
void QgsNorthArrowPluginGui::on_pbnCancel_clicked()
{
  close(1);
}

void QgsNorthArrowPluginGui::setRotation(int theInt)
{
  rotatePixmap(theInt);
  sliderRotation->setValue(theInt);
}

void QgsNorthArrowPluginGui::setPlacement(QString thePlacementQString)
{
  cboPlacement->setCurrentText(tr(thePlacementQString));
}

void QgsNorthArrowPluginGui::setEnabled(bool theBool)
{
  cboxShow->setChecked(theBool);
}

void QgsNorthArrowPluginGui::setAutomatic(bool theBool)
{
  cboxAutomatic->setChecked(theBool);
}

void QgsNorthArrowPluginGui::setAutomaticDisabled()
{
  cboxAutomatic->setEnabled(false);
}


//overides function by the same name created in .ui
void QgsNorthArrowPluginGui::on_spinAngle_valueChanged( int theInt)
{

}

void QgsNorthArrowPluginGui::on_sliderRotation_valueChanged( int theInt)
{
  rotatePixmap(theInt);
}

void QgsNorthArrowPluginGui::rotatePixmap(int theRotationInt)
{
  QPixmap myQPixmap;
  QString myFileNameQString = QgsApplication::pkgDataPath() + "/images/north_arrows/default.png";
  //std::cout << "Trying to load " << myFileNameQString << std::cout;
  if (myQPixmap.load(myFileNameQString))
  {
    QPixmap  myPainterPixmap(myQPixmap.height(),myQPixmap.width());
    myPainterPixmap.fill();
    QPainter myQPainter;
    myQPainter.begin(&myPainterPixmap);	

    myQPainter.setRenderHint(QPainter::SmoothPixmapTransform);

    double centerXDouble = myQPixmap.width()/2;
    double centerYDouble = myQPixmap.height()/2;
    //save the current canvas rotation
    myQPainter.save();
    //myQPainter.translate( (int)centerXDouble, (int)centerYDouble );

    //rotate the canvas
    myQPainter.rotate( theRotationInt );
    //work out how to shift the image so that it appears in the center of the canvas
    //(x cos a + y sin a - x, -x sin a + y cos a - y)
    const double PI = 3.14159265358979323846;
    double myRadiansDouble = (PI/180) * theRotationInt;
    int xShift = static_cast<int>((
                (centerXDouble * cos(myRadiansDouble)) + 
                (centerYDouble * sin(myRadiansDouble))
                ) - centerXDouble);
    int yShift = static_cast<int>((
                (-centerXDouble * sin(myRadiansDouble)) + 
                (centerYDouble * cos(myRadiansDouble))
                ) - centerYDouble);	

    //draw the pixmap in the proper position
    myQPainter.drawPixmap(xShift,yShift,myQPixmap);

    //unrotate the canvas again
    myQPainter.restore();
    myQPainter.end();

    pixmapLabel->setPixmap(myPainterPixmap);
  }
  else
  {
    QPixmap  myPainterPixmap(200,200);
    myPainterPixmap.fill();
    QPainter myQPainter;
    myQPainter.begin(&myPainterPixmap);	
    QFont myQFont("time", 18, QFont::Bold);
    myQPainter.setFont(myQFont);
    myQPainter.setPen(Qt::red);
    myQPainter.drawText(10, 20, QString("Pixmap Not Found"));
    myQPainter.end();
    pixmapLabel->setPixmap(myPainterPixmap);    
  }

}

// Called when the widget needs to be updated.
//

void QgsNorthArrowPluginGui::paintEvent( QPaintEvent * thePaintEvent)
{
  rotatePixmap(sliderRotation->value());
}

//
// Called when the widget has been resized.
// 

void QgsNorthArrowPluginGui::resizeEvent( QResizeEvent * theResizeEvent)
{
  rotatePixmap(sliderRotation->value());  
}
