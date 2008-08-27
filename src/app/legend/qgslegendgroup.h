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
#ifndef QGSLEGENDGROUP_H
#define QGSLEGENDGROUP_H

#include <list>
#include <qgslegenditem.h>

class QgsLegendLayerFile;

/**
This is a specialised version of QLegendItem that specifies that the items below this point will be treated as a group. For example hiding this node will hide all layers below that are members of the group.

@author Tim Sutton
*/
class QgsLegendGroup : public QgsLegendItem
{
  public:
    QgsLegendGroup( QTreeWidgetItem *, QString );
    QgsLegendGroup( QTreeWidget*, QString );
    QgsLegendGroup( QString name );
    ~QgsLegendGroup();

    QgsLegendItem::DRAG_ACTION accept( LEGEND_ITEM_TYPE type );
    QgsLegendItem::DRAG_ACTION accept( const QgsLegendItem* li ) const;
    bool isLeafNode();
    bool insert( QgsLegendItem* theItem );
    /**Returns all legend layer files under this group*/
    std::list<QgsLegendLayerFile*> legendLayerFiles();
    /**Goes through all the legendlayerfiles and sets check state to checked/partially checked/unchecked*/
    void updateCheckState();
};

#endif
