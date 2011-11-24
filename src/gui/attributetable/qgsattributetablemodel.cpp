/***************************************************************************
     QgsAttributeTableModel.cpp
     --------------------------------------
    Date                 : Feb 2009
    Copyright            : (C) 2009 Vita Cizek
    Email                : weetya (at) gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsattributetablemodel.h"
#include "qgsattributetablefiltermodel.h"

#include "qgsfield.h"
#include "qgsvectorlayer.h"
#include "qgslogger.h"
#include "qgsattributeaction.h"
#include "qgsmapcanvas.h"

#include <QtGui>
#include <QVariant>
#include <limits>

QgsRectangle QgsAttributeTableModel::mCurrentExtent; // static member


QgsAttributeTableModel::QgsAttributeTableModel( QgsVectorLayer *theLayer, QObject *parent )
    : QAbstractTableModel( parent )
{
  mFeat.setFeatureId( std::numeric_limits<int>::min() );
  mFeatureMap.clear();
  mFeatureQueue.clear();

  mLayer = theLayer;
  loadAttributes();

  connect( mLayer, SIGNAL( attributeValueChanged( QgsFeatureId, int, const QVariant& ) ), this, SLOT( attributeValueChanged( QgsFeatureId, int, const QVariant& ) ) );
  connect( mLayer, SIGNAL( featureAdded( QgsFeatureId ) ), this, SLOT( featureAdded( QgsFeatureId ) ) );
  connect( mLayer, SIGNAL( featureDeleted( QgsFeatureId ) ), this, SLOT( featureDeleted( QgsFeatureId ) ) );
  connect( mLayer, SIGNAL( attributeAdded( int ) ), this, SLOT( attributeAdded( int ) ) );
  connect( mLayer, SIGNAL( attributeDeleted( int ) ), this, SLOT( attributeDeleted( int ) ) );

  loadLayer();
}

bool QgsAttributeTableModel::featureAtId( QgsFeatureId fid ) const
{
  QgsDebugMsgLevel( QString( "loading feature %1" ).arg( fid ), 3 );

  if ( fid == std::numeric_limits<int>::min() )
  {
    return false;
  }
  else if ( mFeatureMap.contains( fid ) )
  {
    mFeat = mFeatureMap[ fid ];
    return true;
  }
  else if ( mLayer->featureAtId( fid, mFeat, false, true ) )
  {
    QSettings settings;
    int cacheSize = settings.value( "/qgis/attributeTableRowCache", "10000" ).toInt();

    if ( mFeatureQueue.size() == cacheSize )
    {
      mFeatureMap.remove( mFeatureQueue.dequeue() );
    }

    mFeatureQueue.enqueue( fid );
    mFeatureMap.insert( fid, mFeat );

    return true;
  }
  else
  {
    return false;
  }
}

void QgsAttributeTableModel::featureDeleted( QgsFeatureId fid )
{
  QgsDebugMsgLevel( QString( "deleted fid=%1 => row=%2" ).arg( fid ).arg( idToRow( fid ) ), 3 );

  int row = idToRow( fid );

  beginRemoveRows( QModelIndex(), row, row );
  removeRow( row );
  endRemoveRows();
}

bool QgsAttributeTableModel::removeRows( int row, int count, const QModelIndex &parent )
{
  Q_UNUSED( parent );
  QgsDebugMsgLevel( QString( "remove %2 rows at %1 (rows %3, ids %4)" ).arg( row ).arg( count ).arg( mRowIdMap.size() ).arg( mIdRowMap.size() ), 3 );

  // clean old references
  for ( int i = row; i < row + count; i++ )
  {
    mIdRowMap.remove( mRowIdMap[ i ] );
    mRowIdMap.remove( i );
  }

  // update maps
  int n = mRowIdMap.size() + count;
  for ( int i = row + count; i < n; i++ )
  {
    QgsFeatureId id = mRowIdMap[i];
    mIdRowMap[ id ] -= count;
    mRowIdMap[ i-count ] = id;
    mRowIdMap.remove( i );
  }

#ifdef QGISDEBUG
  QgsDebugMsgLevel( QString( "after removal rows %1, ids %2" ).arg( mRowIdMap.size() ).arg( mIdRowMap.size() ), 4 );
  QgsDebugMsgLevel( "id->row", 4 );
  for ( QHash<QgsFeatureId, int>::iterator it = mIdRowMap.begin(); it != mIdRowMap.end(); ++it )
    QgsDebugMsgLevel( QString( "%1->%2" ).arg( FID_TO_STRING( it.key() ) ).arg( *it ), 4 );

  QHash<QgsFeatureId, int>::iterator idit;

  QgsDebugMsgLevel( "row->id", 4 );
  for ( QHash<int, QgsFeatureId>::iterator it = mRowIdMap.begin(); it != mRowIdMap.end(); ++it )
    QgsDebugMsgLevel( QString( "%1->%2" ).arg( it.key() ).arg( FID_TO_STRING( *it ) ), 4 );
#endif

  Q_ASSERT( mRowIdMap.size() == mIdRowMap.size() );

  return true;
}

void QgsAttributeTableModel::featureAdded( QgsFeatureId fid, bool newOperation )
{
  QgsDebugMsgLevel( QString( "feature %1 added (%2, rows %3, ids %4)" ).arg( fid ).arg( newOperation ).arg( mRowIdMap.size() ).arg( mIdRowMap.size() ), 3 );

  int n = mRowIdMap.size();
  if ( newOperation )
    beginInsertRows( QModelIndex(), n, n );

  mIdRowMap.insert( fid, n );
  mRowIdMap.insert( n, fid );

  if ( newOperation )
    endInsertRows();

  reload( index( rowCount() - 1, 0 ), index( rowCount() - 1, columnCount() ) );
}

void QgsAttributeTableModel::attributeAdded( int idx )
{
  Q_UNUSED( idx );
  QgsDebugMsg( "entered." );
  loadAttributes();
  loadLayer();
  emit modelChanged();
}

void QgsAttributeTableModel::attributeDeleted( int idx )
{
  Q_UNUSED( idx );
  QgsDebugMsg( "entered." );
  loadAttributes();
  loadLayer();
  emit modelChanged();
}

void QgsAttributeTableModel::layerDeleted()
{
  QgsDebugMsg( "entered." );

  beginRemoveRows( QModelIndex(), 0, rowCount() - 1 );
  removeRows( 0, rowCount() );
  endRemoveRows();
}

void QgsAttributeTableModel::attributeValueChanged( QgsFeatureId fid, int idx, const QVariant &value )
{
  if ( mFeatureMap.contains( fid ) )
  {
    mFeatureMap[ fid ].changeAttribute( fieldCol( idx ), value );
  }
  setData( index( idToRow( fid ), fieldCol( idx ) ), value, Qt::EditRole );
}

void QgsAttributeTableModel::loadAttributes()
{
  if ( !mLayer )
  {
    return;
  }

  bool ins = false, rm = false;

  QgsAttributeList attributes;
  for ( QgsFieldMap::const_iterator it = mLayer->pendingFields().constBegin(); it != mLayer->pendingFields().end(); it++ )
  {
    switch ( mLayer->editType( it.key() ) )
    {
      case QgsVectorLayer::Hidden:
        continue;

      case QgsVectorLayer::ValueMap:
        mValueMaps.insert( it.key(), &mLayer->valueMap( it.key() ) );
        break;

      default:
        break;
    }

    attributes << it.key();
  }

  if ( mFieldCount < attributes.size() )
  {
    ins = true;
    beginInsertColumns( QModelIndex(), mFieldCount, attributes.size() - 1 );
  }
  else if ( attributes.size() < mFieldCount )
  {
    rm = true;
    beginRemoveColumns( QModelIndex(), attributes.size(), mFieldCount - 1 );
  }

  mFieldCount = attributes.size();
  mAttributes = attributes;
  mValueMaps.clear();

  if ( ins )
  {
    endInsertColumns();
  }
  else if ( rm )
  {
    endRemoveColumns();
  }
}

void QgsAttributeTableModel::loadLayer()
{
  QgsDebugMsg( "entered." );

  beginRemoveRows( QModelIndex(), 0, rowCount() - 1 );
  removeRows( 0, rowCount() );
  endRemoveRows();

  QSettings settings;
  int behaviour = settings.value( "/qgis/attributeTableBehaviour", 0 ).toInt();

  if ( behaviour == 1 )
  {
    beginInsertRows( QModelIndex(), 0, mLayer->selectedFeatureCount() );
    foreach( QgsFeatureId fid, mLayer->selectedFeaturesIds() )
    {
      featureAdded( fid, false );
    }
    endInsertRows();
  }
  else
  {
    QgsRectangle rect;
    if ( behaviour == 2 )
    {
      // current canvas only
      rect = mCurrentExtent;
    }

    mLayer->select( QgsAttributeList(), rect, false );

    QgsFeature f;
    for ( int i = 0; mLayer->nextFeature( f ); ++i )
    {
      featureAdded( f.id() );
    }
  }

  mFieldCount = mAttributes.size();
}

void QgsAttributeTableModel::swapRows( QgsFeatureId a, QgsFeatureId b )
{
  if ( a == b )
    return;

  int rowA = idToRow( a );
  int rowB = idToRow( b );

  //emit layoutAboutToBeChanged();

  mRowIdMap.remove( rowA );
  mRowIdMap.remove( rowB );
  mRowIdMap.insert( rowA, b );
  mRowIdMap.insert( rowB, a );

  mIdRowMap.remove( a );
  mIdRowMap.remove( b );
  mIdRowMap.insert( a, rowB );
  mIdRowMap.insert( b, rowA );

  //emit layoutChanged();
}

int QgsAttributeTableModel::idToRow( QgsFeatureId id ) const
{
  if ( !mIdRowMap.contains( id ) )
  {
    QgsDebugMsg( QString( "idToRow: id %1 not in the map" ).arg( id ) );
    return -1;
  }

  return mIdRowMap[id];
}

QgsFeatureId QgsAttributeTableModel::rowToId( const int row ) const
{
  if ( !mRowIdMap.contains( row ) )
  {
    QgsDebugMsg( QString( "rowToId: row %1 not in the map" ).arg( row ) );
    // return negative infinite (to avoid collision with newly added features)
    return std::numeric_limits<int>::min();
  }

  return mRowIdMap[row];
}

int QgsAttributeTableModel::fieldIdx( int col ) const
{
  return mAttributes[ col ];
}

int QgsAttributeTableModel::fieldCol( int idx ) const
{
  return mAttributes.indexOf( idx );
}

int QgsAttributeTableModel::rowCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent );
  return mRowIdMap.size();
}

int QgsAttributeTableModel::columnCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent );
  return mFieldCount;
}

QVariant QgsAttributeTableModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( role == Qt::DisplayRole )
  {
    if ( orientation == Qt::Vertical ) //row
    {
      return QVariant( section );
    }
    else
    {
      QString attributeName = mLayer->attributeAlias( mAttributes[section] );
      if ( attributeName.isEmpty() )
      {
        QgsField field = mLayer->pendingFields()[ mAttributes[section] ];
        attributeName = field.name();
      }
      return QVariant( attributeName );
    }
  }
  else
  {
    return QVariant();
  }
}

void QgsAttributeTableModel::sort( int column, Qt::SortOrder order )
{
  QgsAttributeMap row;
  QgsAttributeList attrs;
  QgsFeature f;

  attrs.append( mAttributes[column] );

  emit layoutAboutToBeChanged();
// QgsDebugMsg("SORTing");

  mSortList.clear();
  mLayer->select( attrs, QgsRectangle(), false );
  while ( mLayer->nextFeature( f ) )
  {
    row = f.attributeMap();
    mSortList.append( QgsAttributeTableIdColumnPair( f.id(), row[ mAttributes[column] ] ) );
  }

  if ( order == Qt::AscendingOrder )
    qStableSort( mSortList.begin(), mSortList.end() );
  else
    qStableSort( mSortList.begin(), mSortList.end(), qGreater<QgsAttributeTableIdColumnPair>() );

  // recalculate id<->row maps
  mRowIdMap.clear();
  mIdRowMap.clear();

  int i = 0;
  QList<QgsAttributeTableIdColumnPair>::Iterator it;
  for ( it = mSortList.begin(); it != mSortList.end(); ++it, ++i )
  {
    mRowIdMap.insert( i, it->id() );
    mIdRowMap.insert( it->id(), i );
  }

  // restore selection
  emit layoutChanged();
  //reset();
  emit modelChanged();
}

QVariant QgsAttributeTableModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() || ( role != Qt::TextAlignmentRole && role != Qt::DisplayRole && role != Qt::EditRole ) )
    return QVariant();

  int fieldId = mAttributes[ index.column()];

  QVariant::Type fldType = mLayer->pendingFields()[ fieldId ].type();
  bool fldNumeric = ( fldType == QVariant::Int || fldType == QVariant::Double );

  if ( role == Qt::TextAlignmentRole )
  {
    if ( fldNumeric )
      return QVariant( Qt::AlignRight );
    else
      return QVariant( Qt::AlignLeft );
  }

  // if we don't have the row in current cache, load it from layer first
  QgsFeatureId rowId = rowToId( index.row() );
  if ( mFeat.id() != rowId )
  {
    if ( !featureAtId( rowId ) )
      return QVariant( "ERROR" );
  }

  if ( mFeat.id() != rowId )
    return QVariant( "ERROR" );

  const QVariant &val = mFeat.attributeMap()[ fieldId ];

  if ( val.isNull() )
  {
    // if the value is NULL, show that in table, but don't show "NULL" text in editor
    if ( role == Qt::EditRole )
    {
      return QVariant( fldType );
    }
    else
    {
      QSettings settings;
      return settings.value( "qgis/nullValue", "NULL" );
    }
  }

  if ( role == Qt::DisplayRole && mValueMaps.contains( index.column() ) )
  {
    return mValueMaps[ index.column()]->key( val.toString(), QString( "(%1)" ).arg( val.toString() ) );
  }

  return val.toString();
}

bool QgsAttributeTableModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  if ( !index.isValid() || role != Qt::EditRole || !mLayer->isEditable() )
    return false;

  QgsFeatureId rowId = rowToId( index.row() );
  if ( mFeat.id() == rowId || featureAtId( rowId ) )
  {
    mFeat.changeAttribute( mAttributes[ index.column()], value );
  }

  if ( !mLayer->isModified() )
    return false;

  emit dataChanged( index, index );

  return true;
}

Qt::ItemFlags QgsAttributeTableModel::flags( const QModelIndex &index ) const
{
  if ( !index.isValid() )
    return Qt::ItemIsEnabled;

  Qt::ItemFlags flags = QAbstractItemModel::flags( index );

  if ( mLayer->isEditable() &&
       mLayer->editType( mAttributes[ index.column()] ) != QgsVectorLayer::Immutable )
    flags |= Qt::ItemIsEditable;

  return flags;
}

void QgsAttributeTableModel::reload( const QModelIndex &index1, const QModelIndex &index2 )
{
  mFeat.setFeatureId( std::numeric_limits<int>::min() );
  emit dataChanged( index1, index2 );
}

void QgsAttributeTableModel::resetModel()
{
  reset();
}

void QgsAttributeTableModel::changeLayout()
{
  emit layoutChanged();
}

void QgsAttributeTableModel::incomingChangeLayout()
{
  emit layoutAboutToBeChanged();
}

void QgsAttributeTableModel::executeAction( int action, const QModelIndex &idx ) const
{
  QgsAttributeMap attributes;

  for ( int i = 0; i < mAttributes.size(); i++ )
  {
    attributes.insert( i, data( index( idx.row(), i ), Qt::EditRole ) );
  }

  mLayer->actions()->doAction( action, attributes, fieldIdx( idx.column() ) );
}

QgsFeature QgsAttributeTableModel::feature( QModelIndex &idx )
{
  QgsFeature f;
  QgsAttributeMap attributes;

  f.setFeatureId( rowToId( idx.row() ) );
  for ( int i = 0; i < mAttributes.size(); i++ )
  {
    f.changeAttribute( i, data( index( idx.row(), i ), Qt::EditRole ) );
  }

  return f;
}
