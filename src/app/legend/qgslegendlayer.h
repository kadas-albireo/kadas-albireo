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
class QgsRasterLayer;
class QgsVectorLayer;

class QTreeWidget;

typedef std::list< std::pair<QString, QPixmap> > SymbologyList;

/**
Container for layer, including layer file(s), symbology class breaks and properties

@author Tim Sutton
*/
class QgsLegendLayer : public QgsLegendItem
{
    Q_OBJECT

  public:
    QgsLegendLayer( QTreeWidgetItem *, QString );
    QgsLegendLayer( QTreeWidget*, QString );
    QgsLegendLayer( QString name );
    ~QgsLegendLayer();
    /**Sets an icon characterising the type of layer(s) it contains.
     Note: cannot be in the constructor because layers are added after creation*/
    void setLayerTypeIcon();
    bool isLeafNode();
    QgsLegendItem::DRAG_ACTION accept( LEGEND_ITEM_TYPE type );
    QgsLegendItem::DRAG_ACTION accept( const QgsLegendItem* li ) const;
    /**Returns the map layer associated with the first QgsLegendLayerFile or 0 if
     there is no QgsLegendLayerFile*/
    QgsMapLayer* firstMapLayer() const;
    /**Returns first map layer's file or 0 if there's no QgsLegendLayerFile */
    QgsLegendLayerFile* firstLayerFile() const;
    /**Returns the map layers associated with the QgsLegendLayerFiles*/
    std::list<QgsMapLayer*> mapLayers();
    /**Returns the legend layer file items associated with this legend layer*/
    std::list<QgsLegendLayerFile*> legendLayerFiles();
    /**Goes through all the legendlayerfiles and sets check state to checked/partially checked/unchecked*/
    void updateCheckState();

    /**Updates symbology of the layer and copies symbology to other layer files in the group*/
    void refreshSymbology( const QString& key, double widthScale = 1.0 );

    /**Goes through all the legendlayerfiles and adds editing/overview pixmaps to the icon. If not all layer files
    have the same editing/overview state, a tristate is applied*/
    void updateIcon();

    /** called to add appropriate menu items to legend's popup menu */
    void addToPopupMenu( QMenu& theMenu, QAction* toggleEditingAction );

    /**Determines whether there are layers in overview*/
    bool isInOverview();

  public slots:

    /**Toggle show in overview*/
    void showInOverview();

    /**Show layer attribute table*/
    void table();

    void saveAsShapefile();
    void saveSelectionAsShapefile();

  protected:

    /** Prepare and change symbology for vector layer */
    void vectorLayerSymbology( const QgsVectorLayer* mapLayer, double widthScale = 1.0 );

    /** Prepare and change symbology for raster layer */
    void rasterLayerSymbology( QgsRasterLayer* mapLayer );

    /** Removes the symbology items of a layer and adds new ones.
     * If other files are in the same legend layer, the new symbology settings are copied.
     * Note: the QIcon* are deleted and therefore need to be allocated by calling
     * functions using operator new
     */
    void changeSymbologySettings( const QgsMapLayer* mapLayer, const SymbologyList& newSymbologyItems );

    /** Copies the symbology settings of the layer to all maplayers in the QgsLegendLayerFileGroup.
     * This method should be called whenever a layer in this group changes it symbology settings
     */
    void updateLayerSymbologySettings( const QgsMapLayer* mapLayer );

    QPixmap getOriginalPixmap() const;

  private:
    /** Helper method to make the font bold from all ctors.
     *  Not to be confused with setFont() which is inherited
     *  from the QTreeWidgetItem base class.
     */
    void setupFont();
};

#endif
