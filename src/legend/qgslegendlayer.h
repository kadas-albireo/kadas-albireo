/***************************************************************************
 *   Copyright (C) 2005 by Tim Sutton   *
 *   aps02ts@macbuntu   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef QGSLEGENDLAYER_H
#define QGSLEGENDLAYER_H

//#include <qobject.h>
#include <qgslegenditem.h>
#include <QFileInfo>

class QgsLegendLayer;
class QgsLegendLayerFile;
class QgsLegendPropertyGroup;
class QgsMapLayer;
class QTreeWidget;

/**
Container for layer, including layer file(s), symbology class breaks and properties

@author Tim Sutton
*/
class QgsLegendLayer : public QgsLegendItem
{
public:
    QgsLegendLayer(QTreeWidgetItem * ,QString);
    QgsLegendLayer(QTreeWidget* ,QString);
    QgsLegendLayer(QString name);
    ~QgsLegendLayer();
    /**Sets an icon characterising the type of layer(s) it contains.
     Note: cannot be in the constructor because layers are added after creation*/
    void setLayerTypeIcon();
    bool isLeafNode();
    QgsLegendItem::DRAG_ACTION accept(LEGEND_ITEM_TYPE type);
    QgsLegendItem::DRAG_ACTION accept(const QgsLegendItem* li) const;
    /**Returns the map layer associated with the first QgsLegendLayerFile or 0 if
     there is no QgsLegendLayerFile*/
    QgsMapLayer* firstMapLayer();
    /**Returns the map layers associated with the QgsLegendLayerFiles*/
    std::list<QgsMapLayer*> mapLayers();
    /**Returns the legend layer file items associated with this legend layer*/
    std::list<QgsLegendLayerFile*> legendLayerFiles();
    /**Copies the symbology settings of the layer to all maplayers in the QgsLegendLayerFileGroup.
   This method should be called whenever a layer in this group changes it symbology settings
  (normally from QgsMapLayer::refreshLegend)*/
  void updateLayerSymbologySettings(const QgsMapLayer* mapLayer);
};

#endif
