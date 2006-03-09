/*******************************************************************
                              qgsgrasstree.cpp
                             -------------------
    begin                : February, 2006
    copyright            : (C) 2006 by Radim Blazek
    email                : radim.blazek@gmail.com
********************************************************************/
/********************************************************************
 This program is free software; you can redistribute it and/or modify  
 it under the terms of the GNU General Public License as published by 
 the Free Software Foundation; either version 2 of the License, or     
 (at your option) any later version.                                   
*******************************************************************/
#include <iostream>
#include <vector>
#include <map>

#include <QApplication>
#include <QStyle>
#include <qdir.h>
#include <qfile.h>
#include <qsettings.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qnamespace.h>
#include <qevent.h>
#include <qsize.h>
#include <qicon.h>
#include <QTreeWidgetItem>
#include <QModelIndex>
#include <QVariant>
#include <QRegExp>

#include "qgis.h"
#include "qgsapplication.h"
#include "qgsfeatureattribute.h" // TODO: remove - this is only for qgsgrassprovider.h

extern "C" {
#include <grass/gis.h>
#include <grass/Vect.h>
}

#include "../../src/providers/grass/qgsgrass.h"
#include "../../src/providers/grass/qgsgrassprovider.h"
#include "qgsgrassmodel.h"
#include "qgsgrassselect.h"

/* 
 * Internal data structure starts (at present) with LOCATION
 * as top level element, that is root but it does not appear 
 * in the tree view. First elements shown in the view are mapsets
 */

class QgsGrassModelItem
{
public:
    QgsGrassModelItem(QgsGrassModelItem *parent, int row, QString name,
	            QString path, int type);
    QgsGrassModelItem();
    ~QgsGrassModelItem();

    // Copy mGisbase, mLocation, mMapset, mMap, mLayer
    void copyNames ( QgsGrassModelItem *item);

    void populate();
    bool populated() { return mPopulated; }

    int type() { return mType; }

    // Map URI
    QString uri();

    QgsGrassModelItem *child ( int i );
    QgsGrassModelItem *mParent;
    QVariant data (int role = Qt::DisplayRole);
    QString name();
    QString info();
    QString htmlTableRow ( QString s1, QString s2 );
    QString htmlTableRow ( QStringList  list );

    int mType;

    QString mGisbase;
    QString mLocation;
    QString mMapset;
    QString mMap;
    QString mLayer;

    QVector<QgsGrassModelItem*> mChildren;
    bool mPopulated;

    QgsGrassModel *mModel;
};

QgsGrassModelItem::QgsGrassModelItem()
     :mParent(0),mPopulated(false),mType(QgsGrassModel::None)
{
}

QgsGrassModelItem::~QgsGrassModelItem() 
{ 
    for (int i = 0; i < mChildren.size();i++) 
    {
	delete mChildren[i];
    }
    mChildren.clear(); 
}

void QgsGrassModelItem::copyNames ( QgsGrassModelItem *item  ) 
{ 
    mModel = item->mModel;
    mGisbase = item->mGisbase;
    mLocation = item->mLocation;
    mMapset = item->mMapset;
    mMap = item->mMap;
    mLayer = item->mLayer;
}

QVariant QgsGrassModelItem::data (int role) 
{ 
    if (role != Qt::DisplayRole) return QVariant();

    return name();
}

QString QgsGrassModelItem::info() 
{ 
    QString tblStart = "<table border=1 cellspacing=1 cellpadding=1>";
    switch ( mType ) 
    {
	case QgsGrassModel::Location:
	    return "Location: " + mLocation; 
	    break;
	case QgsGrassModel::Mapset:
	    return "Location: " + mLocation + "<br>Mapset: " + mMapset; 
	    break;
	case QgsGrassModel::Vectors:
	case QgsGrassModel::Rasters:
	    return "Location: " + mLocation + "<br>Mapset: " + mMapset; 
	    break;
	case QgsGrassModel::Raster:
	  {
	    QString str = tblStart;
	    str += htmlTableRow("<b>Raster</b>", "<b>" + mMap + "</b>" );
	    
	    struct Cell_head head;
	    QgsGrass::setLocation( mGisbase, mLocation );
	    
	    if( G_get_cellhd( mMap.toLocal8Bit().data(), 
			      mMapset.toLocal8Bit().data(), &head) != 0 ) 
	    {
		str += "<tr><td colspan=2>Cannot open raster header</td></tr>";
	    }
	    else
	    {
		str += htmlTableRow ( "Rows", QString::number(head.rows));
		str += htmlTableRow ( "Columns", QString::number(head.cols) );
		str += htmlTableRow ( "N-S resolution", QString::number(head.ns_res) );
		str += htmlTableRow ( "E-W resolution", QString::number(head.ew_res) );
		str += htmlTableRow ( "North", QString::number(head.north) );
		str += htmlTableRow ( "South", QString::number(head.south) );
		str += htmlTableRow ( "East", QString::number(head.east) );
		str += htmlTableRow ( "West", QString::number(head.west) );
		
	        int rasterType = G_raster_map_type( mMap.toLocal8Bit().data(),
		                                mMapset.toLocal8Bit().data() );

                QString format;
	        if( rasterType == CELL_TYPE ) 
		{
		    format = "integer (" + QString::number(head.format);
		    format += head.format==0 ? " byte)" : "bytes)";
		}
 	        else if( rasterType == FCELL_TYPE ) 
		{
		    format += "floating point (4 bytes)";
		}
		else if( rasterType == DCELL_TYPE )
		{
		    format += "floating point (8 bytes)";
		}
		else
		{
		    format += "unknown";
		}
                str += htmlTableRow ( "Format", format );
	    }

	    struct FPRange range;
	    if ( G_read_fp_range( mMap.toLocal8Bit().data(),
                           mMapset.toLocal8Bit().data(), &range ) != -1 )
	    {
		double min, max;
		G_get_fp_range_min_max( &range, &min, &max );

		str += htmlTableRow ( "Minimum value", QString::number(min));
		str += htmlTableRow ( "Maximum value", QString::number(max));
	    }

	    
	    struct History hist;
	    if ( G_read_history( mMap.toLocal8Bit().data(),
  			         mMapset.toLocal8Bit().data(), &hist) >= 0 )
	    {
                if ( QString(hist.datsrc_1).length() > 0 
                     || QString(hist.datsrc_2).length() > 0 )
                {
                    str += htmlTableRow ( "Data source", QString(hist.datsrc_1) + " " 
		                       + QString(hist.datsrc_2) );
                }
                if ( QString(hist.keywrd).length() > 0 ) 
                { 
		    str += htmlTableRow ( "Data description", QString(hist.keywrd) );
                }
                if ( hist.edlinecnt > 0 ) 
                {
                    QString h;
		    for (int i = 0; i < hist.edlinecnt; i++)
		    {
			h += QString(hist.edhist[i]) + "<br>";
		    }
		    str += htmlTableRow ( "Comments", h);
                }
	    }
	    str += "</table>";
						
	    return str; 
	  }
	  break;

	case QgsGrassModel::Vector:
          {
	    QString str = tblStart;
	    str += htmlTableRow("<b>Vector</b>", "<b>" + mMap + "</b>" );

	    QgsGrass::setLocation( mGisbase, mLocation );
	    
            struct Map_info Map;
            int level = Vect_open_old_head ( &Map, mMap.toLocal8Bit().data(), 
                                             mMapset.toLocal8Bit().data());
            
            if ( level >= 2 ) 
            {
                int is3d = Vect_is_3d (&Map);

                // Number of elements
                str += htmlTableRow ( "Points", QString::number(Vect_get_num_primitives(&Map, GV_POINT)) );
                str += htmlTableRow ( "Lines", QString::number(Vect_get_num_primitives(&Map, GV_LINE)) );
                str += htmlTableRow ( "Boundaries", QString::number(Vect_get_num_primitives(&Map, GV_BOUNDARY)) );
                str += htmlTableRow ( "Centroids", QString::number(Vect_get_num_primitives(&Map, GV_CENTROID)) );
                if ( is3d ) 
                {
                    str += htmlTableRow ( "Faces", QString::number( Vect_get_num_primitives(&Map, GV_FACE) ) );
                    str += htmlTableRow ( "Kernels", QString::number( Vect_get_num_primitives(&Map, GV_KERNEL) ) );
                }
                
                str += htmlTableRow ( "Areas", QString::number(Vect_get_num_areas(&Map)) );
                str += htmlTableRow ( "Islands", QString::number( Vect_get_num_islands(&Map) ) );


                // Box and dimension
                BOUND_BOX box;
                char buffer[100];

                Vect_get_map_box (&Map, &box );
	    
		QgsGrass::setMapset( mGisbase, mLocation, mMapset );
                struct Cell_head window;
                G_get_window (&window);
                int proj = window.proj;

                G_format_northing (box.N, buffer, proj);
		str += htmlTableRow ( "North", QString(buffer) );
                G_format_northing (box.S, buffer, proj);
		str += htmlTableRow ( "South", QString(buffer) );
                G_format_easting (box.E, buffer, proj );
		str += htmlTableRow ( "East", QString(buffer) );
                G_format_easting (box.W, buffer, proj );
		str += htmlTableRow ( "West", QString(buffer) );
                if ( is3d ) 
                {
		    str += htmlTableRow ( "Top", QString::number(box.T) );
		    str += htmlTableRow ( "Bottom", QString::number(box.B) );
                }

		str += htmlTableRow ( "3D", is3d ? "yes" : "no" );

	        str += "</table>";

                // History
                Vect_hist_rewind ( &Map );
                char hbuffer[1001];
		str += "<p>History<br>";
                QRegExp rx ( "^-+$" );
                while ( Vect_hist_read ( hbuffer, 1000, &Map ) != NULL ) {
                    QString row = QString(hbuffer);
                    if ( rx.search ( row ) != -1 ) 
                    { 
                        str += "<hr>";
                    }
                    else
                    {
                        str += row + "<br>";
                    }
                }
            }
            else 
            {
	        str += "</table>";
            }

            Vect_close (&Map);
            return str;
          }
	  break;

	case QgsGrassModel::VectorLayer:
          {
	    QString str = tblStart;
	    str += htmlTableRow("<b>Vector</b>", "<b>" + mMap + "</b>" );
	    str += htmlTableRow("<b>Layer</b>", "<b>" + mLayer + "</b>" );

	    QgsGrass::setLocation( mGisbase, mLocation );
	    
            struct Map_info Map;
            int level = Vect_open_old_head ( &Map, mMap.toLocal8Bit().data(), 
                                             mMapset.toLocal8Bit().data());
            
            if ( level >= 2 ) 
            {
                struct field_info *fi;

                int field = QgsGrassProvider::grassLayer(mLayer);
                if ( field != -1 ) 
                {
                    // Number of features
                    int type = QgsGrassProvider::grassLayerType(mLayer);
                    if (type != -1 )
                    {
	    		str += htmlTableRow("Features", 
                              QString::number(Vect_cidx_get_type_count(&Map, field, type)) );
                    }

		    fi = Vect_get_field ( &Map, field);

                    // Database link
                    if ( fi ) 
                    {
	    		str += htmlTableRow("Driver", QString(fi->driver) );
	    		str += htmlTableRow("Database", QString(fi->database) );
	    		str += htmlTableRow("Table", QString(fi->table) );
	    		str += htmlTableRow("Key column", QString(fi->key) );
                    }
                }
            }            
	    str += "</table>";

            Vect_close (&Map);
            return str;
          }
	  break;
    }
    return QString();
}

QString QgsGrassModelItem::htmlTableRow ( QString s1, QString s2 )
{ 
    QStringList sl (s1);
    sl.append (s2);
    return htmlTableRow (sl);
}

QString QgsGrassModelItem::htmlTableRow ( QStringList list )
{ 
    QString s = "<tr>";
    for ( int i = 0; i < list.size(); i++ )
    {
        s.append ( "<td>" + list.at(i) + "</td>" );
    }
    s.append ( "</tr>" );
    return s;
}

QString QgsGrassModelItem::name() 
{ 
    switch ( mType ) 
    {
	case QgsGrassModel::Location:
	    return mLocation; 
	    break;
	case QgsGrassModel::Mapset:
	    return mMapset;
	    break;
	case QgsGrassModel::Vectors:
	    return "vector";
	    break;
	case QgsGrassModel::Rasters:
	    return "raster";
	    break;
	case QgsGrassModel::Vector:
	case QgsGrassModel::Raster:
	    return mMap;
	    break;
	case QgsGrassModel::VectorLayer:
	    return mLayer; 
	    break;
    }
    return QString();
}

QString QgsGrassModelItem::uri () 
{ 
    switch ( mType ) 
    {
	case QgsGrassModel::VectorLayer:
	    return mGisbase + "/" + mLocation + "/" + mMapset + "/" 
		   + mMap + "/" + mLayer;
	    break;
	case QgsGrassModel::Raster:
	    return mGisbase + "/" + mLocation + "/" + mMapset + "/cellhd/" + mMap;
	    break;
    }
    return QString();
}

QgsGrassModelItem *QgsGrassModelItem::child ( int i ) 
{
    Q_ASSERT(i < mChildren.size());
    //return &(mChildren[i]);
    return mChildren[i];
}

void QgsGrassModelItem::populate()
{
    std::cerr << "QgsGrassModelItem::populate()" << std::endl;

    if ( mPopulated ) return;

    mModel->refreshItem(this);
}

/*********************** MODEL ***********************/

QgsGrassModel::QgsGrassModel ( QObject * parent )
             :QAbstractItemModel ( parent  )
{
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModel()" << std::endl;
    #endif

    // Icons
    QStyle *style = QApplication::style();
    mIconDirectory = QIcon(style->standardPixmap(QStyle::SP_DirClosedIcon));
    mIconDirectory.addPixmap(style->standardPixmap(QStyle::SP_DirOpenIcon), 
	                     QIcon::Normal, QIcon::On);
    QString location = QgsGrass::getDefaultGisdbase() 
		       + "/" + QgsGrass::getDefaultLocation();

    mIconFile = QIcon(style->standardPixmap(QStyle::SP_FileIcon));
    
    mRoot = new QgsGrassModelItem();
    mRoot->mType = QgsGrassModel::Location;
    mRoot->mModel = this;
    mRoot->mGisbase = QgsGrass::getDefaultGisdbase();
    mRoot->mLocation = QgsGrass::getDefaultLocation();
    //mRoot->refresh(); 
    refreshItem(mRoot);
}

QgsGrassModel::~QgsGrassModel() { }

void QgsGrassModel::refresh() 
{ 
    #ifdef QGISDEBUG
    std::cerr << "QgsGrassModel::refresh()" << std::endl;
    #endif
    
    //mRoot->refresh(); 
    refreshItem(mRoot); 
}

QModelIndex QgsGrassModel::index(QgsGrassModelItem *item)
{
    // Item index 
    QModelIndex index;
    if ( item->mParent ) {
	Q_ASSERT ( item->mParent->mChildren.size() > 0 );
	//QVector<QgsGrassModelItem> children = item->mParent->mChildren;
	//int row = (item - &(item->mParent->mChildren.at(0)));
	int row = -1;
	for ( int i = 0; i < item->mParent->mChildren.size(); i++ )
	{
	   if ( item == item->mParent->mChildren[i] )
	   {
	       row = i;
	       break;
	   }
	} 
        Q_ASSERT ( row>=0 );    
	index = createIndex( row, 0, item );
    } else {
	index = QModelIndex();
    }
    return index;
}

void QgsGrassModel::removeItems(QgsGrassModelItem *item, QStringList list)
{
    QModelIndex index = QgsGrassModel::index ( item );
    // Remove items not present in the list
    for (int i = 0; i < item->mChildren.size();) 
    {
	if ( !list.contains(item->mChildren[i]->name()) )
	{
	    //std::cerr << "remove " << item->mChildren[i]->name().ascii() << std::endl;
	    beginRemoveRows( index, i, i );
	    delete item->mChildren[i];
	    item->mChildren.remove(i);
	    endRemoveRows();
	}
	else
	{
	   i++;
	} 
    }
}

void QgsGrassModel::addItems(QgsGrassModelItem *item, QStringList list, int type)
{
    //std::cerr << "QgsGrassModel::addItems" << std::endl;
    QModelIndex index = QgsGrassModel::index ( item );

    // Add new items
    
    for (int i = 0; i < list.size();i++) 
    {
	QString name = list.at(i);
	//std::cerr << "? add " << name.ascii() << std::endl;

	int insertAt = item->mChildren.size();
	for (int i = 0; i < item->mChildren.size();i++) 
	{
	    if ( item->mChildren[i]->name() == name ) 
	    {
		insertAt = -1;
		break;
	    }
	    if ( QString::localeAwareCompare(item->mChildren[i]->name(),name) > 0 )
	    {
		insertAt = i;
		break;
	    }
	}
	
	if ( insertAt >= 0 )
	{
	    std::cerr << "-> add " << name.ascii() << std::endl;
	    beginInsertRows( index, insertAt, insertAt );
	    QgsGrassModelItem *newItem = new QgsGrassModelItem();
	    item->mChildren.insert( insertAt, newItem );
	    //QgsGrassModelItem *newItem = &(item->mChildren[insertAt]);
	    newItem->mType = type;
	    newItem->mParent = item;
	    newItem->copyNames(item);
	    switch ( newItem->mType ) 
	    {
		case QgsGrassModel::Location:
		    newItem->mLocation = name;
		    break;
		case QgsGrassModel::Mapset:
		    newItem->mMapset = name;
		    break;
		case QgsGrassModel::Vectors:
		case QgsGrassModel::Rasters:
		    break;
		case QgsGrassModel::Vector:
		case QgsGrassModel::Raster:
		    newItem->mMap = name;
		    break;
		case QgsGrassModel::VectorLayer:
		    newItem->mLayer = name;
		    break;
	    }
	    
	    endInsertRows();
	}
    }
}

void QgsGrassModel::refreshItem(QgsGrassModelItem *item)
{
    std::cerr << "QgsGrassModel::refreshItem() item->mType = " << item->mType << std::endl;
    

    switch ( item->mType ) 
    {
	case QgsGrassModel::Location:
	  {
	    QStringList list = QgsGrass::mapsets ( item->mGisbase, item->mLocation );
            removeItems(item, list);
            addItems(item, list, QgsGrassModel::Mapset );

	  }
	  break;

	case QgsGrassModel::Mapset:
	  {
	    QStringList vectors = QgsGrass::vectors ( item->mGisbase, 
		                     item->mLocation, item->mMapset );
	    QStringList rasters = QgsGrass::rasters ( item->mGisbase, 
		                     item->mLocation, item->mMapset );

	    QStringList list;
	    if ( vectors.count() > 0 ) list.append("vector");
	    if ( rasters.count() > 0 ) list.append("raster");

            removeItems(item, list);
	    
	    if ( vectors.count() > 0 )
                addItems(item, QStringList("vector"), QgsGrassModel::Vectors );

	    if ( rasters.count() > 0 )
                addItems(item, QStringList("raster"), QgsGrassModel::Rasters );

	  }
	  break;

	case QgsGrassModel::Vectors:
	case QgsGrassModel::Rasters:
	  {
	    QStringList list;
	    int type;
	    if ( item->mType == QgsGrassModel::Vectors )
	    {
	        list = QgsGrass::vectors ( item->mGisbase, item->mLocation, 
			                   item->mMapset );
		type = QgsGrassModel::Vector;
	    }
	    else
	    {
	        list = QgsGrass::rasters ( item->mGisbase, item->mLocation, 
			                   item->mMapset );
		type = QgsGrassModel::Raster;
	    }

            removeItems(item, list);
            addItems(item, list, type );
    
	  }
	  break;

	case QgsGrassModel::Vector:
	  {
	    QStringList list = QgsGrassSelect::vectorLayers ( 
		   QgsGrass::getDefaultGisdbase(), 
		   QgsGrass::getDefaultLocation(),
		   item->mMapset, item->mMap );

            removeItems(item, list);
            addItems(item, list, QgsGrassModel::VectorLayer );
	  }
	    break;

	case QgsGrassModel::Raster:
	    break;

	case QgsGrassModel::VectorLayer:
	    break;
    }
    for ( int i = 0; i < item->mChildren.size(); i++ )
    {
	if ( item->mChildren[i]->mPopulated ) {
	    refreshItem( item->mChildren[i] );
	}
    }

    item->mPopulated = true;
}

QModelIndex QgsGrassModel::index( int row, int column, 
	                        const QModelIndex & parent ) const
{
    //std::cerr << "QgsGrassModel::index row = " << row 
    //                      << " column = " << column << std::endl;
    
    QgsGrassModelItem *item;
    if (!parent.isValid()) { 
	item = mRoot; 
    } else { 
        item = static_cast<QgsGrassModelItem*>(parent.internalPointer());
    }
    //if ( !item->populated() ) refreshItem(item);
    if ( !item->populated() ) item->populate();
    return createIndex ( row, column, item->child(row) );
}

QModelIndex QgsGrassModel::parent ( const QModelIndex & index ) const
{
    //std::cerr << "QgsGrassModel::parent" << std::endl;

    if (!index.isValid()) return QModelIndex();

    QgsGrassModelItem *item =
        static_cast<QgsGrassModelItem*>(index.internalPointer());
    
    QgsGrassModelItem *parentNode = item->mParent;

    if ( parentNode == 0 || parentNode == mRoot) return QModelIndex();

    // parent's row
    QVector<QgsGrassModelItem*> children = parentNode->mParent ? 
    	       parentNode->mParent->mChildren : mRoot->mChildren;
    int row = -1;
    for ( int i = 0; i < children.size(); i++ )
    {
       if ( parentNode == children[i] )
       {
	   row = i;
	   break;
       }
    } 
    Q_ASSERT ( row>=0 );    
    return createIndex(row, 0, parentNode);
}

int QgsGrassModel::rowCount ( const QModelIndex & parent ) const
{
    //std::cerr << "QgsGrassModel::rowCount" << std::endl;
    QgsGrassModelItem *item;
    if (!parent.isValid()) { 
	item = mRoot; 
    } else { 
        item = static_cast<QgsGrassModelItem*>(parent.internalPointer());
    }
    //std::cerr << "name = " << item->name().ascii() << std::endl;
    //std::cerr << "count = " << item->mChildren.size() << std::endl;
    if ( !item->populated() ) item->populate();
    //if ( !item->populated() ) refreshItem(item);
    return item->mChildren.size();
}

int QgsGrassModel::columnCount ( const QModelIndex & parent ) const
{
    //std::cerr << "QgsGrassModel::columnCount" << std::endl;
    return 1;
}

QVariant QgsGrassModel::data ( const QModelIndex &index, int role ) const 
{
    //std::cerr << "QgsGrassModel::data" << std::endl;

    if (!index.isValid()) { return QVariant(); } 
    if (role != Qt::DisplayRole && role != Qt::DecorationRole) return QVariant();

    QgsGrassModelItem *item;
    item = static_cast<QgsGrassModelItem*>(index.internalPointer());

    if ( role == Qt::DecorationRole ) 
    {
	if ( item->type() == QgsGrassModel::Raster ||
	     item->type() == QgsGrassModel::VectorLayer )
	{
	        return mIconFile;
	}
        return mIconDirectory;
    }
    return item->data(role);
}

QString QgsGrassModel::itemName ( const QModelIndex &index)
{
    if (!index.isValid()) { return QString(); } 

    QgsGrassModelItem *item;
    item = static_cast<QgsGrassModelItem*>(index.internalPointer());

    return item->name();
}

QString QgsGrassModel::itemMapset ( const QModelIndex &index)
{
    if (!index.isValid()) { return QString(); } 

    QgsGrassModelItem *item;
    item = static_cast<QgsGrassModelItem*>(index.internalPointer());

    return item->mMapset;
}

QString QgsGrassModel::itemMap ( const QModelIndex &index)
{
    if (!index.isValid()) { return QString(); } 

    QgsGrassModelItem *item;
    item = static_cast<QgsGrassModelItem*>(index.internalPointer());

    return item->mMap;
}

QString QgsGrassModel::itemInfo ( const QModelIndex &index)
{
    if (!index.isValid()) { return QString(); } 

    QgsGrassModelItem *item;
    item = static_cast<QgsGrassModelItem*>(index.internalPointer());

    return item->info();
}

int QgsGrassModel::itemType ( const QModelIndex &index ) const 
{
    if (!index.isValid()) { return QgsGrassModel::None; } 
    QgsGrassModelItem *item;
    item = static_cast<QgsGrassModelItem*>(index.internalPointer());
    return item->type();
}

QString QgsGrassModel::uri ( const QModelIndex &index ) const 
{
    if (!index.isValid()) { return QString(); } 
    QgsGrassModelItem *item;
    item = static_cast<QgsGrassModelItem*>(index.internalPointer());
    return item->uri();
}

void QgsGrassModel::setLocation( const QString &gisbase, const QString &location ) 
{ 
    mGisbase = gisbase;
    mLocation = location;
}

QVariant QgsGrassModel::headerData(int section, 
	Qt::Orientation orientation, int role) const
{
    //std::cerr << "QgsGrassModel::headerData" << std::endl;
    
    //TODO
    //if (orientation == Qt::Horizontal && role == Qt::DisplayRole)

    return QVariant();
}

Qt::ItemFlags QgsGrassModel::flags(const QModelIndex &index) const
{
    //std::cerr << "QgsGrassModel::flags" << std::endl;

    //TODO
    if (!index.isValid())
         return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

