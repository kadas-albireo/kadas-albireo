/***************************************************************************
    qgscolorschemelist.cpp
    ----------------------
    Date                 : August 2014
    Copyright            : (C) 2014 by Nyall Dawson
    Email                : nyall dot dawson at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscolorschemelist.h"
#include "qgsapplication.h"
#include "qgslogger.h"
#include "qgssymbollayerv2utils.h"
#include <QPainter>
#include <QColorDialog>
#include <QMimeData>
#include <QClipboard>

//For model testing
//#include "modeltest.h"

QgsColorSchemeList::QgsColorSchemeList( QWidget *parent , QgsColorScheme *scheme, const QString context, const QColor baseColor )
    : QTreeView( parent )
    , mScheme( scheme )
{
  mModel = new QgsColorSchemeModel( scheme, context, baseColor, this );
  //for testing:
  //new ModelTest( mModel, this );
  setModel( mModel );

  mSwatchDelegate = new QgsColorSwatchDelegate( this );
  setItemDelegateForColumn( 0, mSwatchDelegate );

  setRootIsDecorated( false );
  setSelectionMode( QAbstractItemView::ExtendedSelection );
  setSelectionBehavior( QAbstractItemView::SelectRows );
  setDragEnabled( true );
  setAcceptDrops( true );
  setDragDropMode( QTreeView::DragDrop );
  setDropIndicatorShown( true );
  setDefaultDropAction( Qt::MoveAction );
}

QgsColorSchemeList::~QgsColorSchemeList()
{

}

void QgsColorSchemeList::setScheme( QgsColorScheme *scheme, const QString context, const QColor baseColor )
{
  mScheme = scheme;
  mModel->setScheme( scheme, context, baseColor );
}

bool QgsColorSchemeList::saveColorsToScheme()
{
  if ( !mScheme || !mScheme->isEditable() )
  {
    return false;
  }

  mScheme->setColors( mModel->colors(), mModel->context(), mModel->baseColor() );
  return true;
}

void QgsColorSchemeList::removeSelection()
{
  QList<int> rows;
  foreach ( const QModelIndex &index, selectedIndexes() )
  {
    rows << index.row();
  }
  //remove duplicates
  QList<int> rowsToRemove =  QList<int>::fromSet( rows.toSet() );

  //remove rows in descending order
  qSort( rowsToRemove.begin(), rowsToRemove.end(), qGreater<int>() );
  foreach ( const int row, rowsToRemove )
  {
    mModel->removeRow( row );
  }
}

void QgsColorSchemeList::addColor( const QColor color, const QString label )
{
  mModel->addColor( color, label );
}

void QgsColorSchemeList::pasteColors()
{
  QgsNamedColorList pastedColors = QgsSymbolLayerV2Utils::colorListFromMimeData( QApplication::clipboard()->mimeData() );

  if ( pastedColors.length() == 0 )
  {
    //no pasted colors
    return;
  }

  //insert pasted colors
  QgsNamedColorList::const_iterator colorIt = pastedColors.constBegin();
  for ( ; colorIt != pastedColors.constEnd(); ++colorIt )
  {
    mModel->addColor(( *colorIt ).first, ( *colorIt ).second );
  }
}

void QgsColorSchemeList::copyColors()
{
  QList<int> rows;
  foreach ( const QModelIndex &index, selectedIndexes() )
  {
    rows << index.row();
  }
  //remove duplicates
  QList<int> rowsToCopy =  QList<int>::fromSet( rows.toSet() );

  QgsNamedColorList colorsToCopy;
  foreach ( const int row, rowsToCopy )
  {
    colorsToCopy << mModel->colors().at( row );
  }

  //copy colors
  QMimeData* mimeData = QgsSymbolLayerV2Utils::colorListToMimeData( colorsToCopy );
  QApplication::clipboard()->setMimeData( mimeData );
}

bool QgsColorSchemeList::importColorsFromGpl( QFile &file )
{
  QgsNamedColorList importedColors;
  bool ok = false;
  importedColors = QgsSymbolLayerV2Utils::importColorsFromGpl( file, ok );
  if ( !ok )
  {
    return false;
  }

  if ( importedColors.length() == 0 )
  {
    //no imported colors
    return false;
  }

  //insert imported colors
  QgsNamedColorList::const_iterator colorIt = importedColors.constBegin();
  for ( ; colorIt != importedColors.constEnd(); ++colorIt )
  {
    mModel->addColor(( *colorIt ).first, ( *colorIt ).second );
  }

  return true;
}

bool QgsColorSchemeList::exportColorsToGpl( QFile &file )
{
  return QgsSymbolLayerV2Utils::saveColorsToGpl( file, QString(), mModel->colors() );
}

//
// QgsColorSchemeModel
//

QgsColorSchemeModel::QgsColorSchemeModel( QgsColorScheme *scheme, const QString context, const QColor baseColor, QObject *parent )
    : QAbstractItemModel( parent )
    , mScheme( scheme )
    , mContext( context )
    , mBaseColor( baseColor )
{
  if ( scheme )
  {
    mColors = scheme->fetchColors( context, baseColor );
  }
}

QgsColorSchemeModel::~QgsColorSchemeModel()
{

}

QModelIndex QgsColorSchemeModel::index( int row, int column, const QModelIndex &parent ) const
{
  if ( column < 0 || column >= columnCount() )
  {
    //column out of bounds
    return QModelIndex();
  }

  if ( !parent.isValid() && row >= 0 && row < mColors.size() )
  {
    //return an index for the composer item at this position
    return createIndex( row, column );
  }

  //only top level supported
  return QModelIndex();
}

QModelIndex QgsColorSchemeModel::parent( const QModelIndex &index ) const
{
  Q_UNUSED( index );

  //all items are top level
  return QModelIndex();
}

int QgsColorSchemeModel::rowCount( const QModelIndex &parent ) const
{
  if ( !parent.isValid() )
  {
    return mColors.size();
  }
  else
  {
    //no children
    return 0;
  }
}

int QgsColorSchemeModel::columnCount( const QModelIndex &parent ) const
{
  Q_UNUSED( parent );
  return 2;
}

QVariant QgsColorSchemeModel::data( const QModelIndex &index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();

  QPair< QColor, QString > namedColor = mColors.at( index.row() );
  switch ( role )
  {
    case Qt::DisplayRole:
    case Qt::EditRole:
      switch ( index.column() )
      {
        case ColorSwatch:
          return namedColor.first;
        case ColorLabel:
          return namedColor.second;
        default:
          return QVariant();
      }

    case Qt::TextAlignmentRole:
      return QVariant( Qt::AlignLeft | Qt::AlignVCenter );

    default:
      return QVariant();
  }
}

Qt::ItemFlags QgsColorSchemeModel::flags( const QModelIndex &index ) const
{
  Qt::ItemFlags flags = QAbstractItemModel::flags( index );

  if ( ! index.isValid() )
  {
    return flags | Qt::ItemIsDropEnabled;
  }

  switch ( index.column() )
  {
    case ColorSwatch:
    case ColorLabel:
      return flags | Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
    default:
      return flags | Qt::ItemIsEnabled | Qt::ItemIsSelectable;
  }
}

bool QgsColorSchemeModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  Q_UNUSED( role );

  if ( !index.isValid() )
    return false;

  if ( index.row() >= mColors.length() )
    return false;

  switch ( index.column() )
  {
    case ColorSwatch:
      mColors[ index.row()].first = value.value<QColor>();
      emit dataChanged( index, index );
      return true;

    case ColorLabel:
      mColors[ index.row()].second = value.toString();
      emit dataChanged( index, index );
      return true;

    default:
      return false;
  }
}

QVariant QgsColorSchemeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  switch ( role )
  {
    case Qt::DisplayRole:
    {
      switch ( section )
      {
        case ColorSwatch:
          return tr( "Color" );
        case ColorLabel:
          return tr( "Label" );
        default:
          return QVariant();
      }
      break;
    }

    case Qt::TextAlignmentRole:
      switch ( section )
      {
        case ColorSwatch:
          return QVariant( Qt::AlignHCenter | Qt::AlignVCenter );
        case ColorLabel:
          return QVariant( Qt::AlignLeft | Qt::AlignVCenter );
        default:
          return QVariant();
      }
    default:
      return QAbstractItemModel::headerData( section, orientation, role );
  }
}

Qt::DropActions QgsColorSchemeModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::CopyAction;
}

QStringList QgsColorSchemeModel::mimeTypes() const
{
  QStringList types;
  types << "text/xml";
  types << "text/plain";
  types << "application/x-color";
  types << "application/x-colorobject-list";
  return types;
}

QMimeData* QgsColorSchemeModel::mimeData( const QModelIndexList &indexes ) const
{
  QgsNamedColorList colorList;

  QModelIndexList::const_iterator indexIt = indexes.constBegin();
  for ( ; indexIt != indexes.constEnd(); ++indexIt )
  {
    if (( *indexIt ).column() > 0 )
      continue;

    colorList << qMakePair( mColors[( *indexIt ).row()].first, mColors[( *indexIt ).row()].second );
  }

  QMimeData* mimeData = QgsSymbolLayerV2Utils::colorListToMimeData( colorList, false );
  return mimeData;
}

bool QgsColorSchemeModel::dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent )
{
  Q_UNUSED( column );

  if ( action == Qt::IgnoreAction )
  {
    return true;
  }

  if ( parent.isValid() )
  {
    return false;
  }

  int beginRow = row != -1 ? row : rowCount( QModelIndex() );
  QgsNamedColorList droppedColors = QgsSymbolLayerV2Utils::colorListFromMimeData( data );

  if ( droppedColors.length() == 0 )
  {
    //no dropped colors
    return false;
  }

  //insert dropped colors
  insertRows( beginRow, droppedColors.length(), QModelIndex() );
  QgsNamedColorList::const_iterator colorIt = droppedColors.constBegin();
  for ( ; colorIt != droppedColors.constEnd(); ++colorIt )
  {
    QModelIndex colorIdx = index( beginRow, 0, QModelIndex() );
    setData( colorIdx, QVariant(( *colorIt ).first ) );
    QModelIndex labelIdx = index( beginRow, 1, QModelIndex() );
    setData( labelIdx, QVariant(( *colorIt ).second ) );
    beginRow++;
  }

  return true;
}

void QgsColorSchemeModel::setScheme( QgsColorScheme *scheme, const QString context, const QColor baseColor )
{
  mScheme = scheme;
  mContext = context;
  mBaseColor = baseColor;
  beginResetModel();
  mColors = scheme->fetchColors( mContext, mBaseColor );
  endResetModel();
}

bool QgsColorSchemeModel::removeRows( int row, int count, const QModelIndex &parent )
{
  if ( parent.isValid() )
  {
    return false;
  }

  if ( row >= mColors.count() )
  {
    return false;
  }

  for ( int i = row + count - 1; i >= row; --i )
  {
    beginRemoveRows( parent, i, i );
    mColors.removeAt( i );
    endRemoveRows();
  }
  return true;
}

bool QgsColorSchemeModel::insertRows( int row, int count, const QModelIndex& parent )
{
  Q_UNUSED( parent );

  beginInsertRows( QModelIndex(), row, row + count - 1 );
  for ( int i = row; i < row + count; ++i )
  {
    QPair< QColor, QString > newColor;
    mColors.insert( i, newColor );
  }
  endInsertRows();
  return true;
}

void QgsColorSchemeModel::addColor( const QColor color, const QString label )
{
  int row = rowCount();
  insertRow( row );
  QModelIndex colorIdx = index( row, 0, QModelIndex() );
  setData( colorIdx, QVariant( color ) );
  QModelIndex labelIdx = index( row, 1, QModelIndex() );
  setData( labelIdx, QVariant( label ) );
}


//
// QgsColorSwatchDelegate
//
QgsColorSwatchDelegate::QgsColorSwatchDelegate( QWidget *parent )
    : QAbstractItemDelegate( parent )
    , mParent( parent )
{

}

void QgsColorSwatchDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  if ( option.state & QStyle::State_Selected )
  {
    painter->setPen( QPen( Qt::NoPen ) );
    if ( option.state & QStyle::State_Active )
    {
      painter->setBrush( QBrush( QPalette().highlight() ) );
    }
    else
    {
      painter->setBrush( QBrush( QPalette().color( QPalette::Inactive,
                                 QPalette::Highlight ) ) );
    }
    painter->drawRect( option.rect );
  }

  QColor color = index.model()->data( index, Qt::DisplayRole ).value<QColor>();
  if ( !color.isValid() )
  {
    return;
  }

  QRect rect = option.rect;
  //center it
  rect.setLeft( option.rect.center().x() - 15 );
  rect.setSize( QSize( 30, 30 ) );
  rect.adjust( 0, 1, 0, 1 );
  //create an icon pixmap
  painter->save();
  painter->setRenderHint( QPainter::Antialiasing );
  painter->setPen( Qt::NoPen );
  if ( color.alpha() < 255 )
  {
    //start with checkboard pattern
    QBrush checkBrush = QBrush( transparentBackground() );
    painter->setBrush( checkBrush );
    painter->drawRoundedRect( rect, 5, 5 );
  }

  //draw semi-transparent color on top
  painter->setBrush( color );
  painter->drawRoundedRect( rect, 5, 5 );
  painter->restore();
}

const QPixmap& QgsColorSwatchDelegate::transparentBackground() const
{
  static QPixmap transpBkgrd;

  if ( transpBkgrd.isNull() )
    transpBkgrd = QgsApplication::getThemePixmap( "/transp-background_8x8.png" );

  return transpBkgrd;
}

QSize QgsColorSwatchDelegate::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
  Q_UNUSED( option );
  Q_UNUSED( index );
  return QSize( 30, 32 );
}

bool QgsColorSwatchDelegate::editorEvent( QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index )
{
  Q_UNUSED( option );
  if ( event->type() == QEvent::MouseButtonDblClick )
  {
    QColor color = index.model()->data( index, Qt::DisplayRole ).value<QColor>();
    QColor newColor = QColorDialog::getColor( color, mParent, tr( "Select color" ), QColorDialog::ShowAlphaChannel );
    if ( !newColor.isValid() )
    {
      return false;
    }

    return model->setData( index, newColor, Qt::EditRole );
  }

  return false;
}
