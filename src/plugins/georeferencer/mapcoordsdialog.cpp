/***************************************************************************
     mapcoordsdialog.cpp
     --------------------------------------
    Date                 : 2005
    Copyright            : (C) 2005 by Lars Luthman
    Email                : larsl at users dot sourceforge dot net
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QValidator>
#include <mapcoordsdialog.h>

#include <qgsmapcanvas.h>
#include <qgsmaptoolemitpoint.h>

MapCoordsDialog::MapCoordsDialog()
{

}


MapCoordsDialog::MapCoordsDialog( const QgsPoint& pixelCoords, QgsMapCanvas* qgisCanvas,
                                  QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl )
{
  setupUi( this );
  mPixelCoords = pixelCoords;
  mQgisCanvas = qgisCanvas;
  leXCoord->setValidator( new QDoubleValidator( this ) );
  leYCoord->setValidator( new QDoubleValidator( this ) );

  mToolEmitPoint = new QgsMapToolEmitPoint( qgisCanvas );
  mToolEmitPoint->setButton( btnPointFromCanvas );
  connect(( QgsMapToolEmitPoint* )mToolEmitPoint, SIGNAL( canvasClicked( QgsPoint&, Qt::MouseButton ) ),
          this, SLOT( maybeSetXY( QgsPoint&, Qt::MouseButton ) ) );

  connect( leXCoord, SIGNAL( textChanged( const QString& ) ), this, SLOT( updateOK() ) );
  connect( leYCoord, SIGNAL( textChanged( const QString& ) ), this, SLOT( updateOK() ) );
  updateOK();
}


MapCoordsDialog::~MapCoordsDialog()
{

  delete mToolEmitPoint;
}

void MapCoordsDialog::updateOK()
{
  bool enable = ( leXCoord->text().size() != 0 && leYCoord->text().size() != 0 );
  buttonOk->setEnabled( enable );
}

void MapCoordsDialog::on_buttonOk_clicked()
{
  QgsPoint mapCoords( leXCoord->text().toDouble(), leYCoord->text().toDouble() );
  emit pointAdded( mPixelCoords, mapCoords );
  accept();
}

void MapCoordsDialog::on_buttonCancel_clicked()
{
  reject();
}

void MapCoordsDialog::maybeSetXY( QgsPoint & xy, Qt::MouseButton button )
{
  // Only LeftButton should set point
  if ( Qt::LeftButton == button )
  {
    leXCoord->clear();
    leYCoord->clear();
    leXCoord->insert( QString::number( xy.x(), 'f', 7 ) );
    leYCoord->insert( QString::number( xy.y(), 'f', 7 ) );
  }

  mQgisCanvas->setMapTool( mPrevMapTool );
}

void MapCoordsDialog::on_btnPointFromCanvas_clicked()
{
  mPrevMapTool = mQgisCanvas->mapTool();
  mQgisCanvas->setMapTool( mToolEmitPoint );
}

