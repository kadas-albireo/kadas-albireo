/***************************************************************************
    qgslegendpropertygroup.cpp
    ---------------------
    begin                : January 2007
    copyright            : (C) 2007 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsapplication.h"
#include "qgisapp.h"
#include "qgslegendpropertygroup.h"
#include <QCoreApplication>
#include <QIcon>

QgsLegendPropertyGroup::QgsLegendPropertyGroup( QTreeWidgetItem* theLegendItem, QString theString )
    : QgsLegendItem( theLegendItem, theString )
{
  mType = LEGEND_PROPERTY_GROUP;
  QIcon myIcon = QgsApplication::getThemeIcon( "/mIconProperties.svg" );
  setText( 0, theString );
  setIcon( 0, myIcon );
}


QgsLegendPropertyGroup::~QgsLegendPropertyGroup()
{

}
