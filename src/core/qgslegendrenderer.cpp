#include "qgslegendrenderer.h"

#include "qgscomposerlegenditem.h"
#include "qgslegendmodel.h"
#include "qgsmaplayerregistry.h"
#include "qgssymbolv2.h"
#include "qgsvectorlayer.h"

#include <QPainter>



QgsLegendRenderer::QgsLegendRenderer( QgsLegendModel* legendModel, const QgsLegendSettings& settings )
    : mLegendModel( legendModel )
    , mSettings( settings )
{
}

QSizeF QgsLegendRenderer::minimumSize()
{
  return paintAndDetermineSize( 0 );
}

void QgsLegendRenderer::drawLegend( QPainter* painter )
{
  paintAndDetermineSize( painter );
}


QSizeF QgsLegendRenderer::paintAndDetermineSize( QPainter* painter )
{
  QSizeF size( 0, 0 );
  QStandardItem* rootItem = mLegendModel->invisibleRootItem();
  if ( !rootItem ) return size;

  QList<Atom> atomList = createAtomList( rootItem, mSettings.splitLayer() );

  setColumns( atomList );

  qreal maxColumnWidth = 0;
  if ( mSettings.equalColumnWidth() )
  {
    foreach ( Atom atom, atomList )
    {
      maxColumnWidth = qMax( atom.size.width(), maxColumnWidth );
    }
  }

  //calculate size of title
  QSizeF titleSize = drawTitle();
  //add title margin to size of title text
  titleSize.rwidth() += mSettings.boxSpace() * 2.0;
  double columnTop = mSettings.boxSpace() + titleSize.height() + mSettings.style( QgsComposerLegendStyle::Title ).margin( QgsComposerLegendStyle::Bottom );

  QPointF point( mSettings.boxSpace(), columnTop );
  bool firstInColumn = true;
  double columnMaxHeight = 0;
  qreal columnWidth = 0;
  int column = 0;
  foreach ( Atom atom, atomList )
  {
    if ( atom.column > column )
    {
      // Switch to next column
      if ( mSettings.equalColumnWidth() )
      {
        point.rx() += mSettings.columnSpace() + maxColumnWidth;
      }
      else
      {
        point.rx() += mSettings.columnSpace() + columnWidth;
      }
      point.ry() = columnTop;
      columnWidth = 0;
      column++;
      firstInColumn = true;
    }
    if ( !firstInColumn )
    {
      point.ry() += spaceAboveAtom( atom );
    }

    QSizeF atomSize = drawAtom( atom, painter, point );
    columnWidth = qMax( atomSize.width(), columnWidth );

    point.ry() += atom.size.height();
    columnMaxHeight = qMax( point.y() - columnTop, columnMaxHeight );

    firstInColumn = false;
  }
  point.rx() += columnWidth + mSettings.boxSpace();

  size.rheight() = columnTop + columnMaxHeight + mSettings.boxSpace();
  size.rwidth() = point.x();
  if ( !mSettings.title().isEmpty() )
  {
    size.rwidth() = qMax( titleSize.width(), size.width() );
  }

  // override the size if it was set by the user
  if ( mLegendSize.isValid() )
  {
    qreal w = qMax( size.width(), mLegendSize.width() );
    qreal h = qMax( size.height(), mLegendSize.height() );
    size = QSizeF( w, h );
  }

  // Now we have set the correct total item width and can draw the title centered
  if ( !mSettings.title().isEmpty() )
  {
    if ( mSettings.titleAlignment() == Qt::AlignLeft )
    {
      point.rx() = mSettings.boxSpace();
    }
    else if ( mSettings.titleAlignment() == Qt::AlignHCenter )
    {
      point.rx() = size.width() / 2;
    }
    else
    {
      point.rx() = size.width() - mSettings.boxSpace();
    }
    point.ry() = mSettings.boxSpace();
    drawTitle( painter, point, mSettings.titleAlignment(), size.width() );
  }

  return size;
}





QList<QgsLegendRenderer::Atom> QgsLegendRenderer::createAtomList( QStandardItem* rootItem, bool splitLayer )
{
  QList<Atom> atoms;

  if ( !rootItem ) return atoms;

  Atom atom;

  for ( int i = 0; i < rootItem->rowCount(); i++ )
  {
    QStandardItem* currentLayerItem = rootItem->child( i );
    QgsComposerLegendItem* currentLegendItem = dynamic_cast<QgsComposerLegendItem*>( currentLayerItem );
    if ( !currentLegendItem ) continue;

    QgsComposerLegendItem::ItemType type = currentLegendItem->itemType();
    if ( type == QgsComposerLegendItem::GroupItem )
    {
      // Group subitems
      QList<Atom> groupAtoms = createAtomList( currentLayerItem, splitLayer );

      Nucleon nucleon;
      nucleon.item = currentLegendItem;
      nucleon.size = dynamic_cast<QgsComposerGroupItem*>( currentLegendItem )->draw( mSettings );

      if ( groupAtoms.size() > 0 )
      {
        // Add internal space between this group title and the next nucleon
        groupAtoms[0].size.rheight() += spaceAboveAtom( groupAtoms[0] );
        // Prepend this group title to the first atom
        groupAtoms[0].nucleons.prepend( nucleon );
        groupAtoms[0].size.rheight() += nucleon.size.height();
        groupAtoms[0].size.rwidth() = qMax( nucleon.size.width(), groupAtoms[0].size.width() );
      }
      else
      {
        // no subitems, append new atom
        Atom atom;
        atom.nucleons.append( nucleon );
        atom.size.rwidth() += nucleon.size.width();
        atom.size.rheight() += nucleon.size.height();
        atom.size.rwidth() = qMax( nucleon.size.width(), atom.size.width() );
        groupAtoms.append( atom );
      }
      atoms.append( groupAtoms );
    }
    else if ( type == QgsComposerLegendItem::LayerItem )
    {
      Atom atom;

      if ( currentLegendItem->style() != QgsComposerLegendStyle::Hidden )
      {
        Nucleon nucleon;
        nucleon.item = currentLegendItem;
        nucleon.size = dynamic_cast<QgsComposerLayerItem*>( currentLegendItem )->draw( mSettings );
        atom.nucleons.append( nucleon );
        atom.size.rwidth() = nucleon.size.width();
        atom.size.rheight() = nucleon.size.height();
      }

      QList<Atom> layerAtoms;

      for ( int j = 0; j < currentLegendItem->rowCount(); j++ )
      {
        QgsComposerBaseSymbolItem * symbolItem = dynamic_cast<QgsComposerBaseSymbolItem*>( currentLegendItem->child( j, 0 ) );
        if ( !symbolItem ) continue;

        Nucleon symbolNucleon = drawSymbolItem( symbolItem );

        if ( !mSettings.splitLayer() || j == 0 )
        {
          // append to layer atom
          // the width is not correct at this moment, we must align all symbol labels
          atom.size.rwidth() = qMax( symbolNucleon.size.width(), atom.size.width() );
          // Add symbol space only if there is already title or another item above
          if ( atom.nucleons.size() > 0 )
          {
            // TODO: for now we keep Symbol and SymbolLabel Top margin in sync
            atom.size.rheight() += mSettings.style( QgsComposerLegendStyle::Symbol ).margin( QgsComposerLegendStyle::Top );
          }
          atom.size.rheight() += symbolNucleon.size.height();
          atom.nucleons.append( symbolNucleon );
        }
        else
        {
          Atom symbolAtom;
          symbolAtom.nucleons.append( symbolNucleon );
          symbolAtom.size.rwidth() = symbolNucleon.size.width();
          symbolAtom.size.rheight() = symbolNucleon.size.height();
          layerAtoms.append( symbolAtom );
        }
      }
      layerAtoms.prepend( atom );
      atoms.append( layerAtoms );
    }
  }

  return atoms;
}



void QgsLegendRenderer::setColumns( QList<Atom>& atomList )
{
  if ( mSettings.columnCount() == 0 ) return;

  // Divide atoms to columns
  double totalHeight = 0;
  // bool first = true;
  qreal maxAtomHeight = 0;
  foreach ( Atom atom, atomList )
  {
    //if ( !first )
    //{
    totalHeight += spaceAboveAtom( atom );
    //}
    totalHeight += atom.size.height();
    maxAtomHeight = qMax( atom.size.height(), maxAtomHeight );
    // first  = false;
  }

  // We know height of each atom and we have to split them into columns
  // minimizing max column height. It is sort of bin packing problem, NP-hard.
  // We are using simple heuristic, brute fore appeared to be to slow,
  // the number of combinations is N = n!/(k!*(n-k)!) where n = atomsCount-1
  // and k = columnsCount-1

  double avgColumnHeight = totalHeight / mSettings.columnCount();
  int currentColumn = 0;
  int currentColumnAtomCount = 0; // number of atoms in current column
  double currentColumnHeight = 0;
  double maxColumnHeight = 0;
  double closedColumnsHeight = 0;
  // first = true; // first in column
  for ( int i = 0; i < atomList.size(); i++ )
  {
    Atom atom = atomList[i];
    double currentHeight = currentColumnHeight;
    //if ( !first )
    //{
    currentHeight += spaceAboveAtom( atom );
    //}
    currentHeight += atom.size.height();

    // Recalc average height for remaining columns including current
    avgColumnHeight = ( totalHeight - closedColumnsHeight ) / ( mSettings.columnCount() - currentColumn );
    if (( currentHeight - avgColumnHeight ) > atom.size.height() / 2 // center of current atom is over average height
        && currentColumnAtomCount > 0 // do not leave empty column
        && currentHeight > maxAtomHeight  // no sense to make smaller columns than max atom height
        && currentHeight > maxColumnHeight  // no sense to make smaller columns than max column already created
        && currentColumn < mSettings.columnCount() - 1 ) // must not exceed max number of columns
    {
      // New column
      currentColumn++;
      currentColumnAtomCount = 0;
      closedColumnsHeight += currentColumnHeight;
      currentColumnHeight = atom.size.height();
    }
    else
    {
      currentColumnHeight = currentHeight;
    }
    atomList[i].column = currentColumn;
    currentColumnAtomCount++;
    maxColumnHeight = qMax( currentColumnHeight, maxColumnHeight );

    // first  = false;
  }

  // Alling labels of symbols for each layr/column to the same labelXOffset
  QMap<QString, qreal> maxSymbolWidth;
  for ( int i = 0; i < atomList.size(); i++ )
  {
    for ( int j = 0; j < atomList[i].nucleons.size(); j++ )
    {
      QgsComposerLegendItem* item = atomList[i].nucleons[j].item;
      if ( !item ) continue;
      QgsComposerLegendItem::ItemType type = item->itemType();
      if ( type == QgsComposerLegendItem::SymbologyV2Item ||
           type == QgsComposerLegendItem::RasterSymbolItem )
      {
        QString key = QString( "%1-%2" ).arg(( qulonglong )item->parent() ).arg( atomList[i].column );
        maxSymbolWidth[key] = qMax( atomList[i].nucleons[j].symbolSize.width(), maxSymbolWidth[key] );
      }
    }
  }
  for ( int i = 0; i < atomList.size(); i++ )
  {
    for ( int j = 0; j < atomList[i].nucleons.size(); j++ )
    {
      QgsComposerLegendItem* item = atomList[i].nucleons[j].item;
      if ( !item ) continue;
      QgsComposerLegendItem::ItemType type = item->itemType();
      if ( type == QgsComposerLegendItem::SymbologyV2Item ||
           type == QgsComposerLegendItem::RasterSymbolItem )
      {
        QString key = QString( "%1-%2" ).arg(( qulonglong )item->parent() ).arg( atomList[i].column );
        double space = mSettings.style( QgsComposerLegendStyle::Symbol ).margin( QgsComposerLegendStyle::Right ) +
                       mSettings.style( QgsComposerLegendStyle::SymbolLabel ).margin( QgsComposerLegendStyle::Left );
        atomList[i].nucleons[j].labelXOffset =  maxSymbolWidth[key] + space;
        atomList[i].nucleons[j].size.rwidth() =  maxSymbolWidth[key] + space + atomList[i].nucleons[j].labelSize.width();
      }
    }
  }
}



QSizeF QgsLegendRenderer::drawTitle( QPainter* painter, QPointF point, Qt::AlignmentFlag halignment, double legendWidth )
{
  QSizeF size( 0, 0 );
  if ( mSettings.title().isEmpty() )
  {
    return size;
  }

  QStringList lines = mSettings.splitStringForWrapping( mSettings.title() );
  double y = point.y();

  if ( painter )
  {
    painter->setPen( mSettings.fontColor() );
  }

  //calculate width and left pos of rectangle to draw text into
  double textBoxWidth;
  double textBoxLeft;
  switch ( halignment )
  {
    case Qt::AlignHCenter:
      textBoxWidth = ( qMin( point.x(), legendWidth - point.x() ) - mSettings.boxSpace() ) * 2.0;
      textBoxLeft = point.x() - textBoxWidth / 2.;
      break;
    case Qt::AlignRight:
      textBoxLeft = mSettings.boxSpace();
      textBoxWidth = point.x() - mSettings.boxSpace();
      break;
    case Qt::AlignLeft:
    default:
      textBoxLeft = point.x();
      textBoxWidth = legendWidth - point.x() - mSettings.boxSpace();
      break;
  }

  QFont titleFont = mSettings.style( QgsComposerLegendStyle::Title ).font();

  for ( QStringList::Iterator titlePart = lines.begin(); titlePart != lines.end(); ++titlePart )
  {
    //last word is not drawn if rectangle width is exactly text width, so add 1
    //TODO - correctly calculate size of italicized text, since QFontMetrics does not
    qreal width = mSettings.textWidthMillimeters( titleFont, *titlePart ) + 1;
    qreal height = mSettings.fontAscentMillimeters( titleFont ) + mSettings.fontDescentMillimeters( titleFont );

    QRectF r( textBoxLeft, y, textBoxWidth, height );

    if ( painter )
    {
      mSettings.drawText( painter, r, *titlePart, titleFont, halignment, Qt::AlignVCenter, Qt::TextDontClip );
    }

    //update max width of title
    size.rwidth() = qMax( width, size.rwidth() );

    y += height;
    if ( titlePart != lines.end() )
    {
      y += mSettings.lineSpacing();
    }
  }
  size.rheight() = y - point.y();

  return size;
}


double QgsLegendRenderer::spaceAboveAtom( Atom atom )
{
  if ( atom.nucleons.size() == 0 ) return 0;

  Nucleon nucleon = atom.nucleons.first();

  QgsComposerLegendItem* item = nucleon.item;
  if ( !item ) return 0;

  QgsComposerLegendItem::ItemType type = item->itemType();
  switch ( type )
  {
    case QgsComposerLegendItem::GroupItem:
      return mSettings.style( item->style() ).margin( QgsComposerLegendStyle::Top );
      break;
    case QgsComposerLegendItem::LayerItem:
      return mSettings.style( item->style() ).margin( QgsComposerLegendStyle::Top );
      break;
    case QgsComposerLegendItem::SymbologyV2Item:
    case QgsComposerLegendItem::RasterSymbolItem:
      // TODO: use Symbol or SymbolLabel Top margin
      return mSettings.style( QgsComposerLegendStyle::Symbol ).margin( QgsComposerLegendStyle::Top );
      break;
    default:
      break;
  }
  return 0;
}


// Draw atom and expand its size (using actual nucleons labelXOffset)
QSizeF QgsLegendRenderer::drawAtom( Atom atom, QPainter* painter, QPointF point )
{
  bool first = true;
  QSizeF size = QSizeF( atom.size );
  foreach ( Nucleon nucleon, atom.nucleons )
  {
    QgsComposerLegendItem* item = nucleon.item;
    //QgsDebugMsg( "text: " + item->text() );
    if ( !item ) continue;
    QgsComposerLegendItem::ItemType type = item->itemType();
    if ( type == QgsComposerLegendItem::GroupItem )
    {
      QgsComposerGroupItem* groupItem = dynamic_cast<QgsComposerGroupItem*>( item );
      if ( !groupItem ) continue;
      if ( groupItem->style() != QgsComposerLegendStyle::Hidden )
      {
        if ( !first )
        {
          point.ry() += mSettings.style( groupItem->style() ).margin( QgsComposerLegendStyle::Top );
        }
        groupItem->draw( mSettings, painter, point );
      }
    }
    else if ( type == QgsComposerLegendItem::LayerItem )
    {
      QgsComposerLayerItem* layerItem = dynamic_cast<QgsComposerLayerItem*>( item );
      if ( !layerItem ) continue;
      if ( layerItem->style() != QgsComposerLegendStyle::Hidden )
      {
        if ( !first )
        {
          point.ry() += mSettings.style( layerItem->style() ).margin( QgsComposerLegendStyle::Top );
        }
        layerItem->draw( mSettings, painter, point );
      }
    }
    else if ( type == QgsComposerLegendItem::SymbologyV2Item ||
              type == QgsComposerLegendItem::RasterSymbolItem )
    {
      if ( !first )
      {
        point.ry() += mSettings.style( QgsComposerLegendStyle::Symbol ).margin( QgsComposerLegendStyle::Top );
      }

      Nucleon symbolNucleon = drawSymbolItem( dynamic_cast<QgsComposerBaseSymbolItem*>( item ), painter, point, nucleon.labelXOffset );
      // expand width, it may be wider because of labelXOffset
      size.rwidth() = qMax( symbolNucleon.size.width(), size.width() );
    }
    point.ry() += nucleon.size.height();
    first = false;
  }
  return size;
}


QgsLegendRenderer::Nucleon QgsLegendRenderer::drawSymbolItem( QgsComposerBaseSymbolItem* symbolItem, QPainter* painter, QPointF point, double labelXOffset )
{
  if ( !symbolItem ) return Nucleon();

  QgsComposerBaseSymbolItem::ItemContext ctx;
  ctx.painter = painter;
  ctx.point = point;
  ctx.labelXOffset = labelXOffset;

  QgsComposerBaseSymbolItem::ItemMetrics im = symbolItem->draw( mSettings, painter ? &ctx : 0 );

  Nucleon nucleon;
  nucleon.item = symbolItem;
  nucleon.symbolSize = im.symbolSize;
  nucleon.labelSize = im.labelSize;
  //QgsDebugMsg( QString( "symbol height = %1 label height = %2").arg( symbolSize.height()).arg( labelSize.height() ));
  double width = qMax(( double ) im.symbolSize.width(), labelXOffset ) + im.labelSize.width();
  double height = qMax( im.symbolSize.height(), im.labelSize.height() );
  nucleon.size = QSizeF( width, height );
  return nucleon;
}
