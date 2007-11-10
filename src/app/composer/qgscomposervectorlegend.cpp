/***************************************************************************
                         qgscomposervectorlegend.cpp
                             -------------------
    begin                : January 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscomposervectorlegend.h"
#include "qgscomposermap.h"
#include "qgscontinuouscolorrenderer.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayer.h"
#include "qgsproject.h"
#include "qgsrenderer.h"
#include "qgssymbol.h"
#include "qgsvectorlayer.h"

#include <QFontDialog>
#include <QPainter>
#include <Q3PopupMenu>
#include <QGraphicsScene>


#include <iostream>
#include <vector>

QgsComposerVectorLegend::QgsComposerVectorLegend ( QgsComposition *composition, int id, 
                                              int x, int y, int fontSize )
    : QWidget(composition), QGraphicsRectItem(x,y,10,10,0)
{
  setupUi(this);

  //std::cout << "QgsComposerVectorLegend::QgsComposerVectorLegend()" << std::endl;

  mComposition = composition;
  mId  = id;
  mMapCanvas = mComposition->mapCanvas();

  init();

  // Font and pen
  if(fontSize < 10){
    fontSize = 10;
  }
  mFont.setPointSize ( fontSize );
    
  // Set map to the first available if any
  std::vector<QgsComposerMap*> maps = mComposition->maps();
  if ( maps.size() > 0 ) {
    mMap = maps[0]->id();
  }

  // Calc size and cache
  recalculate();

  // Add to scene
  mComposition->canvas()->addItem(this);
  QGraphicsRectItem::show();
  QGraphicsRectItem::update();

     
  writeSettings();
}

QgsComposerVectorLegend::QgsComposerVectorLegend ( QgsComposition *composition, int id ) 
    : QGraphicsRectItem(0,0,10,10,0)
{
    //std::cout << "QgsComposerVectorLegend::QgsComposerVectorLegend()" << std::endl;

    setupUi(this);

    mComposition = composition;
    mId  = id;
    mMapCanvas = mComposition->mapCanvas();

    init();

    readSettings();

    // Calc size and cache
    recalculate();

    // Add to scene
    mComposition->canvas()->addItem(this);

    QGraphicsRectItem::show();
    QGraphicsRectItem::update();
}

void QgsComposerVectorLegend::init ( void ) 
{
    mSelected = false;
    mNumCachedLayers = 0;
    mTitle = tr("Legend");
    mMap = 0;
    mNextLayerGroup = 1;
    mFrame = true;

    // Cache
    mCacheUpdated = false;

    // Rectangle
    QGraphicsRectItem::setZValue(50);
//    setActive(true);

    // Layers list view
    mLayersListView->setColumnText(0,tr("Layers"));
    mLayersListView->addColumn(tr("Group"));
    mLayersListView->setSorting(-1);
    mLayersListView->setResizeMode(Q3ListView::AllColumns);
    mLayersListView->setSelectionMode(Q3ListView::Extended);

    mLayersPopupMenu = new Q3PopupMenu( );

    mLayersPopupMenu->insertItem( tr("Combine selected layers"), this, SLOT(groupLayers()) );

    connect ( mLayersListView, SIGNAL(clicked(Q3ListViewItem *)), 
                         this, SLOT(layerChanged(Q3ListViewItem *)));

    connect ( mLayersListView, SIGNAL(rightButtonClicked(Q3ListViewItem *, const QPoint &, int)), 
                    this, SLOT( showLayersPopupMenu(Q3ListViewItem *, const QPoint &, int)) );

    // Plot style
    setPlotStyle ( QgsComposition::Preview );
    
    // Preview style
    mPreviewMode = Render;
    mPreviewModeComboBox->insertItem ( tr("Cache"), Cache );
    mPreviewModeComboBox->insertItem ( tr("Render"), Render );
    mPreviewModeComboBox->insertItem ( tr("Rectangle"), Rectangle );
    mPreviewModeComboBox->setCurrentItem ( mPreviewMode );

    connect ( mComposition, SIGNAL(mapChanged(int)), this, SLOT(mapChanged(int)) ); 
}

QgsComposerVectorLegend::~QgsComposerVectorLegend()
{
  //std::cerr << "QgsComposerVectorLegend::~QgsComposerVectorLegend()" << std::endl;
}

#define FONT_WORKAROUND_SCALE 10
QRectF QgsComposerVectorLegend::render ( QPainter *p )
{
  //std::cout << "QgsComposerVectorLegend::render p = " << p << std::endl;

  // Painter can be 0, create dummy to avoid many if below
  QPainter *painter = NULL;
  QPixmap *pixmap = NULL;
  if ( p ) {
    painter = p;
  } else {
    pixmap = new QPixmap(1,1);
    painter = new QPainter( pixmap );
  }

  //std::cout << "mComposition->scale() = " << mComposition->scale() << std::endl;

  // Font size in canvas units
  float titleSize = 25.4 * mComposition->scale() * mTitleFont.pointSizeFloat() / 72;
  float sectionSize = 25.4 * mComposition->scale() * mSectionFont.pointSizeFloat() / 72;
  float size = 25.4 * mComposition->scale() * mFont.pointSizeFloat() / 72;

  //std::cout << "font sizes = " << titleSize << " " << sectionSize << " " << size << std::endl;

  // Create fonts 
  QFont titleFont ( mTitleFont );
  QFont sectionFont ( mSectionFont );
  QFont font ( mFont );

  titleFont.setPointSizeFloat ( titleSize );
  sectionFont.setPointSizeFloat ( sectionSize );
  font.setPointSizeFloat ( size );

  // Not sure about Style Strategy, QFont::PreferMatch?
  titleFont.setStyleStrategy ( (QFont::StyleStrategy) (QFont::PreferOutline | QFont::PreferAntialias) );
  sectionFont.setStyleStrategy ( (QFont::StyleStrategy) (QFont::PreferOutline | QFont::PreferAntialias) );
  font.setStyleStrategy ( (QFont::StyleStrategy) (QFont::PreferOutline | QFont::PreferAntialias) );


  QFontMetrics titleMetrics ( titleFont );
  QFontMetrics sectionMetrics ( sectionFont );
  QFontMetrics metrics ( font );

  if ( plotStyle() == QgsComposition::Postscript) //do we need seperate PostScript rendering settings?
  {
    // Fonts sizes for Postscript rendering
    double psTitleSize = titleMetrics.ascent() * 72.0 / mComposition->resolution(); //What??
    double psSectionSize = sectionMetrics.ascent() * 72.0 / mComposition->resolution();
    double psSize = metrics.ascent() * 72.0 / mComposition->resolution();

    titleFont.setPointSizeFloat ( psTitleSize * FONT_WORKAROUND_SCALE );
    sectionFont.setPointSizeFloat ( psSectionSize * FONT_WORKAROUND_SCALE );
    font.setPointSizeFloat ( psSize * FONT_WORKAROUND_SCALE );
  }
  else
  {
    titleFont.setPointSizeFloat ( titleSize * FONT_WORKAROUND_SCALE );
    sectionFont.setPointSizeFloat ( sectionSize * FONT_WORKAROUND_SCALE );
    font.setPointSizeFloat ( size * FONT_WORKAROUND_SCALE );
  }

  int x, y;

  // Legend title  -if we do this later, we can center it
  y = mMargin + titleMetrics.height();
  painter->setPen ( mPen );
  painter->setFont ( titleFont );


  painter->save(); //Save the painter state so we can undo the scaling later
  painter->scale(1./FONT_WORKAROUND_SCALE, 1./FONT_WORKAROUND_SCALE); //scale the painter to work around the font bug

  painter->drawText( 2 * mMargin * FONT_WORKAROUND_SCALE, y * FONT_WORKAROUND_SCALE, mTitle );

  painter->restore();

  //used to keep track of total width and height - probably should be changed to float/double
  int width = 4 * mMargin + titleMetrics.width ( mTitle ); 
  int height = mMargin + mSymbolSpace + titleMetrics.height(); // mSymbolSpace?

  // Layers
  QgsComposerMap *map = mComposition->map ( mMap ); //Get the map from the composition by ID number
  if ( map ) {
 
    std::map<int,int> doneGroups;
      
    int nlayers = mMapCanvas->layerCount();
    for ( int i = nlayers - 1; i >= 0; i-- ) {
      QgsMapLayer *layer = mMapCanvas->getZpos(i);
//      if ( !layer->visible() ) continue; // skip non-visible layers
      if ( layer->type() != QgsMapLayer::VECTOR ) continue; //skip raster layers

      QString layerId = layer->getLayerID();

//      if( ! layerOn(layerId) ) continue; //does this need to go away?

      int group = layerGroup ( layerId );
      if ( group > 0 ) { 
        if ( doneGroups.find(group) != doneGroups.end() ) {
          continue; 
        } else {
          doneGroups.insert(std::make_pair(group,1));
        }
      }

      // Make list of all layers in the group and count section items
      std::vector<int> groupLayers; // vector of layers
      std::vector<int> itemHeights; // maximum item sizes
      std::vector<QString> itemLabels; // item labels
      int sectionItemsCount = 0;
      QString sectionTitle;


      for ( int j = nlayers - 1; j >= 0; j-- )
      {
        QgsMapLayer *layer2 = mMapCanvas->getZpos(j);
//        if ( !layer2->visible() ) continue;
        if ( layer2->type() != QgsMapLayer::VECTOR ) continue;
	      
        QString layerId2 = layer2->getLayerID();
        if( ! layerOn(layerId2) ) continue;
	    
        int group2 = layerGroup ( layerId2 );
	    
        QgsVectorLayer *vector = dynamic_cast <QgsVectorLayer*> (layer2);
        const QgsRenderer *renderer = vector->renderer();

        if ( (group > 0 && group2 == group) || ( group == 0 && j == i )  ) {
          groupLayers.push_back(j);

          QList<QgsSymbol*> symbols = renderer->symbols();

          if ( sectionTitle.length() == 0 ) {
            sectionTitle = layer2->name();
          }        
		  
          if ( symbols.size() > sectionItemsCount ) {
            sectionItemsCount = symbols.size();
            itemHeights.resize(sectionItemsCount);
            itemLabels.resize(sectionItemsCount); 
          }

          double widthScale = map->widthScale() * mComposition->scale();
          if ( plotStyle() == QgsComposition::Preview && mPreviewMode == Render ) {
            widthScale *= mComposition->viewScale();
          }
		
          double scale = map->symbolScale() * mComposition->scale();

          int icnt = 0;
          for ( QList<QgsSymbol*>::iterator it = symbols.begin(); it != symbols.end(); ++it ) {
		
            QgsSymbol* sym = (*it);
	      
            // height
            if ( itemHeights[icnt] < mSymbolHeight ) { // init first
              itemHeights[icnt] = mSymbolHeight;
            }

            QPixmap pic = QPixmap::fromImage(sym->getPointSymbolAsImage(widthScale, false));

            int h = (int) ( scale * pic.height() );
            if ( h > itemHeights[icnt] ) {
              itemHeights[icnt] = h;
            }

            if ( itemLabels[icnt].length() == 0 ) {
              if ( sym->label().length() > 0 ) {
		            itemLabels[icnt] = sym->label();
              } else {
		            itemLabels[icnt] = sym->lowerValue();
                if (sym->upperValue().length() > 0)
                  itemLabels[icnt] += " - " + sym->upperValue();
              }
            }
		  
            icnt++;
          }
        }
      }
      //std::cout << "group size = " << groupLayers.size() << std::endl;
      //std::cout << "sectionItemsCount = " << sectionItemsCount << std::endl;


      // Section title 
      if ( sectionItemsCount > 1 )
      {
        height += mSymbolSpace;

        x = (int) ( 2*mMargin );
        y = (int) ( height + sectionMetrics.height() );
        painter->setPen ( mPen );
        painter->setFont ( sectionFont );

        painter->save(); //Save the painter state so we can undo the scaling later
        painter->scale(1./FONT_WORKAROUND_SCALE, 1./FONT_WORKAROUND_SCALE); //scale the painter to work around the font bug

        painter->drawText( x * FONT_WORKAROUND_SCALE, y * FONT_WORKAROUND_SCALE, sectionTitle );
        painter->restore();

        int w = 3*mMargin + sectionMetrics.width( sectionTitle );
        if ( w > width ) width = w;
        height += sectionMetrics.height();
        height += (int) (0.7*mSymbolSpace);
      }


      // Draw all layers in group 
      int groupStartHeight = height;
      for ( int j = groupLayers.size()-1; j >= 0; j-- )
      {
	    std::cout << "layer = " << groupLayers[j] << std::endl;

	    int localHeight = groupStartHeight;
	
	    layer = mMapCanvas->getZpos(groupLayers[j]);
	    QgsVectorLayer *vector = dynamic_cast <QgsVectorLayer*> (layer);
	    const QgsRenderer *renderer = vector->renderer();

	    // Get a list of the symbols from the renderer - some renderers can have several symbols
	    QList<QgsSymbol*> symbols = renderer->symbols();


	    int icnt = 0;
	    for ( QList<QgsSymbol*>::iterator it = symbols.begin(); it != symbols.end(); ++it ) {
          localHeight += mSymbolSpace;

          int symbolHeight = itemHeights[icnt];
          QgsSymbol* sym = (*it);
	    
          QPen pen = sym->pen();
          double widthScale = map->widthScale();

          pen.setWidthF( ( widthScale * pen.widthF() ) );
          painter->setPen ( pen );
          painter->setBrush ( sym->brush() );
	    
          if ( vector->vectorType() == QGis::Point ) {
            double scale = map->symbolScale();

            // Get the picture of appropriate size directly from catalogue
            QPixmap pic = QPixmap::fromImage(sym->getPointSymbolAsImage(widthScale,false,sym->color()));

            painter->save();
            painter->scale(scale,scale);
            painter->drawPixmap ( static_cast<int>( (1.*mMargin+mSymbolWidth/2)/scale-pic.width()/2),
                                  static_cast<int>( (1.*localHeight+symbolHeight/2)/scale-1.*pic.height()/2),
                                  pic );
            painter->restore();

          } else if ( vector->vectorType() == QGis::Line ) {
            painter->drawLine ( mMargin, localHeight+mSymbolHeight/2, 
                                mMargin+mSymbolWidth, localHeight+mSymbolHeight/2 );
          } else if ( vector->vectorType() == QGis::Polygon ) {
            //pen.setWidth(0); //use a cosmetic pen to outline the fill box
            pen.setCapStyle(Qt::FlatCap);
            painter->setPen ( pen );
            painter->drawRect ( mMargin, localHeight, mSymbolWidth, mSymbolHeight );
          }

          // Label 
          painter->setPen ( mPen );
          painter->setFont ( font );
          QString lab;
          if ( sectionItemsCount == 1 ) {
            lab = sectionTitle;
          } else { 
            lab = itemLabels[icnt];
          }
	    
          // drawText (x, y w, h, ...) was cutting last letter (the box was too small)
          QRect br = metrics.boundingRect ( lab );
          x = (int) ( 2*mMargin + mSymbolWidth );
          y = (int) ( localHeight + symbolHeight/2 + ( metrics.height()/2 - metrics.descent()) );
  painter->save(); //Save the painter state so we can undo the scaling later
  painter->scale(1./FONT_WORKAROUND_SCALE, 1./FONT_WORKAROUND_SCALE); //scale the painter to work around the font bug

          painter->drawText( x * FONT_WORKAROUND_SCALE, y * FONT_WORKAROUND_SCALE, lab );
  painter->restore();
          int w = 3*mMargin + mSymbolWidth + metrics.width(lab);
          if ( w > width ) width = w;

          localHeight += symbolHeight;
          icnt++;

	    }//End of iterating through the symbols in the renderer

      }
      // add height of section items to the total height
      height = groupStartHeight;
      for ( int j = 0; j < (int)itemHeights.size(); j++ ) {
	    height += mSymbolSpace + itemHeights[j];
      }
      if ( sectionItemsCount > 1 ) { // add more space to separate section from next item
        height += mSymbolSpace;
      } 

    }//End of iterating through the layers
  }//END if(map)

  height += mMargin;

  if(mFrame)
  {
    QPen pen(QColor(0,0,0), 0.5);
    painter->setPen( pen );
    painter->setBrush( QBrush( QColor(255,255,255), Qt::NoBrush));
    painter->setRenderHint(QPainter::Antialiasing, true);//turn on antialiasing
    painter->drawRect ( 0, 0, width, height );
  }

    
//  QGraphicsRectItem::setRect(0, 0, width, height); //BUG! - calling this causes a re-draw, which means we are continuously re-drawing.
    
  if ( !p ) {
    delete painter;
    delete pixmap;
  }

  return QRectF ( 0, 0, width, height);
}

void QgsComposerVectorLegend::cache ( void )
{
    std::cout << "QgsComposerVectorLegend::cache()" << std::endl;

//typical boundingRect size is 15 units wide,
    mCachePixmap.resize ((int)QGraphicsRectItem::rect().width(), (int)QGraphicsRectItem::rect().height() );


    QPainter p(&mCachePixmap);
    
    mCachePixmap.fill(QColor(255,255,255));
    render ( &p );
    p.end();

    mNumCachedLayers = mMapCanvas->layerCount();
    mCacheUpdated = true;    
}

void QgsComposerVectorLegend::paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget )
{
#ifdef QGISDEBUG
  std::cout << "paint mPlotStyle = " << plotStyle() << " mPreviewMode = " << mPreviewMode << std::endl;
#endif

  if ( plotStyle() == QgsComposition::Preview && mPreviewMode == Cache )
  {
    if ( !mCacheUpdated || mMapCanvas->layerCount() != mNumCachedLayers ) //If the cache is out of date, update it.
    {
      cache();
    }
    painter->drawPixmap(0,0, mCachePixmap);
  }
  else if(plotStyle() == QgsComposition::Preview && mPreviewMode == Rectangle)
  {
    QPen pen(QColor(0,0,0), 0.5);
    painter->setPen( pen );
    painter->setBrush( QBrush( QColor(255,255,255), Qt::NoBrush)); //use SolidPattern instead?
    painter->drawRect(QRectF(0, 0, QGraphicsRectItem::rect().width(), QGraphicsRectItem::rect().height()));
  }
  else if ( (plotStyle() == QgsComposition::Preview && mPreviewMode == Render) || 
              plotStyle() == QgsComposition::Print ||
              plotStyle() == QgsComposition::Postscript ) 
  //We're in render preview mode or printing, so do a full render of the legend. 
  {
    painter->save();
    render(painter);
    painter->restore();
  }


  // Draw the "selected highlight" boxes
  if ( mSelected && plotStyle() == QgsComposition::Preview ) {

    painter->setPen( mComposition->selectionPen() );
    painter->setBrush( mComposition->selectionBrush() );
  
    double s = mComposition->selectionBoxSize();

    painter->drawRect(QRectF(0, 0, s, s)); //top left
    painter->drawRect(QRectF(QGraphicsRectItem::rect().width()-s, 0, s, s)); //top right
    painter->drawRect(QRectF(QGraphicsRectItem::rect().width()-s, QGraphicsRectItem::rect().height()-s, s, s)); //bottom right
    painter->drawRect(QRectF(0, QGraphicsRectItem::rect().height()-s, s, s)); //bottom left
  }
}

void QgsComposerVectorLegend::on_mFontButton_clicked ( void ) 
{
  bool result;

  mFont = QFontDialog::getFont(&result, mFont, this );

  if ( result ) {
    recalculate();
    QGraphicsRectItem::update();
    QGraphicsRectItem::scene()->update();
    writeSettings();
  }
}

void QgsComposerVectorLegend::on_mTitleLineEdit_returnPressed ( void )
{
    mTitle = mTitleLineEdit->text();
    recalculate();
    QGraphicsRectItem::update();
    QGraphicsRectItem::scene()->update();
    writeSettings();
}

void QgsComposerVectorLegend::on_mPreviewModeComboBox_activated ( int i )
{
    mPreviewMode = (PreviewMode) i;
    std::cout << "mPreviewMode = " << mPreviewMode << std::endl;
    writeSettings();
}

void QgsComposerVectorLegend::on_mMapComboBox_activated ( int i )
{
    mMap = mMaps[i];
    recalculate();
    QGraphicsRectItem::update();
    QGraphicsRectItem::scene()->update();
    writeSettings();
}

void QgsComposerVectorLegend::mapChanged ( int id )
{
    if ( id != mMap ) return;

    recalculate();
    QGraphicsRectItem::update();
    QGraphicsRectItem::scene()->update();
}

void QgsComposerVectorLegend::on_mFrameCheckBox_stateChanged ( int )
{
    mFrame = mFrameCheckBox->isChecked();

    QGraphicsRectItem::update();
    QGraphicsRectItem::scene()->update();

    writeSettings();
}

void QgsComposerVectorLegend::recalculate ( void ) 
{
    std::cout << "QgsComposerVectorLegend::recalculate" << std::endl;
    
    // Recalculate sizes according to current font size
    
    // Title and section font 
    mTitleFont = mFont;
    mTitleFont.setPointSizeFloat ( 1.4 * mFont.pointSizeFloat());
    mSectionFont = mFont;
    mSectionFont.setPointSizeFloat ( 1.2 * mFont.pointSizeFloat() );
    
    std::cout << "font size = " << mFont.pointSizeFloat() << std::endl;
    std::cout << "title font size = " << mTitleFont.pointSizeFloat() << std::endl;

    // Font size in canvas units
    float size = 25.4 * mComposition->scale() * mFont.pointSizeFloat() / 72;

    mMargin = (int) ( 0.9 * size );
    mSymbolHeight = (int) ( 1.3 * size );
    mSymbolWidth = (int) ( 3.5 * size );
    mSymbolSpace = (int) ( 0.4 * size );

    std::cout << "mMargin = " << mMargin << " mSymbolHeight = " << mSymbolHeight
              << "mSymbolWidth = " << mSymbolWidth << " mSymbolSpace = " << mSymbolSpace << std::endl;
     
    QRectF r = render(0);

    QGraphicsRectItem::setRect(0, 0, r.width(), r.height() );
    
    mCacheUpdated = false;
}

void QgsComposerVectorLegend::setOptions ( void )
{ 
  mTitleLineEdit->setText( mTitle );
    
  // Maps
  mMapComboBox->clear();
  std::vector<QgsComposerMap*> maps = mComposition->maps();

  mMaps.clear();
    
  bool found = false;
  mMapComboBox->insertItem ( "", 0 );
  mMaps.push_back ( 0 );
  for ( int i = 0; i < (int)maps.size(); i++ ) {
    mMapComboBox->insertItem ( maps[i]->name(), i+1 );
    mMaps.push_back ( maps[i]->id() );

    if ( maps[i]->id() == mMap ) {
      found = true;
      mMapComboBox->setCurrentItem ( i+1 );
    }
  }

  if ( ! found ) {
    mMap = 0;
    mMapComboBox->setCurrentItem ( 0 );
  }

  mFrameCheckBox->setChecked ( mFrame );
    
  // Layers
  mLayersListView->clear();

  if ( mMap != 0 ) {
    QgsComposerMap *map = mComposition->map ( mMap );

    if ( map ) {
      int nlayers = mMapCanvas->layerCount();
      for ( int i = 0; i < nlayers; i++ ) {
        QgsMapLayer *layer = mMapCanvas->getZpos(i);
    
//        if ( !layer->visible() ) continue;
        //if ( layer->type() != QgsMapLayer::VECTOR ) continue;

        Q3CheckListItem *li = new Q3CheckListItem ( mLayersListView, layer->name(), Q3CheckListItem::CheckBox );

        QString id = layer->getLayerID();
        li->setText(2, id );

        li->setOn ( layerOn(id) );
    
        int group = layerGroup(id);
        if ( group > 0 ) {
          li->setText(1, QString::number(group) );
        }

        mLayersListView->insertItem ( li );
      }
    }
  }

  mPreviewModeComboBox->setCurrentItem( mPreviewMode );
}

void QgsComposerVectorLegend::setSelected (  bool s ) 
{
    mSelected = s;
    QGraphicsRectItem::update(); // show highlight
}    

bool QgsComposerVectorLegend::selected( void )
{
    return mSelected;
}

void QgsComposerVectorLegend::showLayersPopupMenu ( Q3ListViewItem * lvi, const QPoint & pt, int )
{
    std::cout << "QgsComposerVectorLegend::showLayersPopupMenu" << std::endl;

    mLayersPopupMenu->exec(pt);
}

bool QgsComposerVectorLegend::layerOn ( QString id ) 
{
  std::map<QString,bool>::iterator it = mLayersOn.find(id);

  if(it != mLayersOn.end() ) {
    return ( it->second );
  }

    return true;
}

void QgsComposerVectorLegend::setLayerOn ( QString id, bool on ) 
{
  std::map<QString,bool>::iterator it = mLayersOn.find(id);

  if(it != mLayersOn.end() ) {
    it->second = on;
  } else {
    mLayersOn.insert(std::make_pair(id,on));
  }
}

int QgsComposerVectorLegend::layerGroup ( QString id ) 
{
  std::map<QString,int>::iterator it = mLayersGroups.find(id);

  if(it != mLayersGroups.end() ) {
    return ( it->second );
  }

  return 0;
}

void QgsComposerVectorLegend::setLayerGroup ( QString id, int group ) 
{
  std::map<QString,int>::iterator it = mLayersGroups.find(id);

  if(it != mLayersGroups.end() ) {
    it->second = group;
  } else {
    mLayersGroups.insert(std::make_pair(id,group));
  }
}

void QgsComposerVectorLegend::layerChanged ( Q3ListViewItem *lvi )
{
    std::cout << "QgsComposerVectorLegend::layerChanged" << std::endl;

    if ( lvi == 0 ) return; 
    
    QString id = lvi->text(2);
    Q3CheckListItem *cli = dynamic_cast <Q3CheckListItem *>(lvi);
    setLayerOn ( id, cli->isOn() );

    writeSettings();

    recalculate();
    QGraphicsRectItem::update();
    QGraphicsRectItem::scene()->update();
}

void QgsComposerVectorLegend::groupLayers ( void ) 
{
  std::cout << "QgsComposerVectorLegend::groupLayers" << std::endl;

  Q3ListViewItemIterator it( mLayersListView );
  int count = 0;
  Q3ListViewItem *lastItem = NULL;
  QString id;
  while ( it.current() ) {
    if ( it.current()->isSelected() ) {
      std::cout << "selected: " << it.current()->text(0).toLocal8Bit().data() << " " << it.current()->text(2).toLocal8Bit().data() << std::endl;

      id = it.current()->text(2);
      setLayerGroup ( id, mNextLayerGroup );
      it.current()->setText(1,QString::number(mNextLayerGroup) );
      lastItem = it.current();
      count++;
    }
    ++it;
  }
  if ( count == 1 ) { // one item only
    setLayerGroup ( id, 0 );
    lastItem->setText(1,"" );
  }

  std::cout << "Groups:" << std::endl;

  for ( std::map<QString,int>::iterator it3 = mLayersGroups.begin(); it3 != mLayersGroups.end(); ++it3 ) {
    std::cout << "layer: " << (it3->first).toLocal8Bit().data() << " group: " << it3->second << std::endl;
  }
    
  mNextLayerGroup++;
    
  writeSettings();

  recalculate();
  QGraphicsRectItem::update();
  QGraphicsRectItem::scene()->update();
}    

QWidget *QgsComposerVectorLegend::options ( void )
{
    setOptions ();
    return ( dynamic_cast <QWidget *> (this) ); 
}

bool QgsComposerVectorLegend::writeSettings ( void )  
{
  std::cout << "QgsComposerVectorLegend::writeSettings" << std::endl;
  QString path;
  path.sprintf("/composition_%d/vectorlegend_%d/", mComposition->id(), mId ); 

  QgsProject::instance()->writeEntry( "Compositions", path+"x", mComposition->toMM((int)QGraphicsRectItem::x()) );
  QgsProject::instance()->writeEntry( "Compositions", path+"y", mComposition->toMM((int)QGraphicsRectItem::y()) );

  QgsProject::instance()->writeEntry( "Compositions", path+"map", mMap );

  QgsProject::instance()->writeEntry( "Compositions", path+"title", mTitle );

  QgsProject::instance()->writeEntry( "Compositions", path+"font/size", mFont.pointSize() );
  QgsProject::instance()->writeEntry( "Compositions", path+"font/family", mFont.family() );
  QgsProject::instance()->writeEntry( "Compositions", path+"font/weight", mFont.weight() );
  QgsProject::instance()->writeEntry( "Compositions", path+"font/underline", mFont.underline() );
  QgsProject::instance()->writeEntry( "Compositions", path+"font/strikeout", mFont.strikeOut() );

  QgsProject::instance()->writeEntry( "Compositions", path+"frame", mFrame );

  // Layers: remove all, write new
  path.sprintf("/composition_%d/vectorlegend_%d/layers/", mComposition->id(), mId ); 
  QgsProject::instance()->removeEntry ( "Compositions", path );
    
  if ( mMap != 0 ) {
    QgsComposerMap *map = mComposition->map ( mMap );

    if ( map ) {
      int nlayers = mMapCanvas->layerCount();
      for ( int i = 0; i < nlayers; i++ ) {
        QgsMapLayer *layer = mMapCanvas->getZpos(i);
    
//        if ( !layer->visible() ) continue;

        QString id = layer->getLayerID();
        path.sprintf("/composition_%d/vectorlegend_%d/layers/layer_%s/", mComposition->id(), mId, id.toLocal8Bit().data() ); 
        QgsProject::instance()->writeEntry( "Compositions", path+"on", layerOn(id) );
        QgsProject::instance()->writeEntry( "Compositions", path+"group", layerGroup(id) );
      }
    }
  }

  QgsProject::instance()->writeEntry( "Compositions", path+"previewmode", mPreviewMode );
    
  return true; 
}

bool QgsComposerVectorLegend::readSettings ( void )
{
  bool ok;
  QString path;
  path.sprintf("/composition_%d/vectorlegend_%d/", mComposition->id(), mId );

  double x = mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"x", 0, &ok));
  double y = mComposition->fromMM(QgsProject::instance()->readDoubleEntry( "Compositions", path+"y", 0, &ok));

  QGraphicsRectItem::setPos(x, y);

  mMap = QgsProject::instance()->readNumEntry("Compositions", path+"map", 0, &ok);
  mTitle = QgsProject::instance()->readEntry("Compositions", path+"title", "???", &ok);
     
  mFont.setFamily ( QgsProject::instance()->readEntry("Compositions", path+"font/family", "", &ok) );
  mFont.setPointSize ( QgsProject::instance()->readNumEntry("Compositions", path+"font/size", 10, &ok) );
  mFont.setWeight(  QgsProject::instance()->readNumEntry("Compositions", path+"font/weight", (int)QFont::Normal, &ok) );
  mFont.setUnderline(  QgsProject::instance()->readBoolEntry("Compositions", path+"font/underline", false, &ok) );
  mFont.setStrikeOut(  QgsProject::instance()->readBoolEntry("Compositions", path+"font/strikeout", false, &ok) );

  mFrame = QgsProject::instance()->readBoolEntry("Compositions", path+"frame", true, &ok);

  // Preview mode
  mPreviewMode = (PreviewMode) QgsProject::instance()->readNumEntry("Compositions", path+"previewmode", Render, &ok);
    
  // Layers
  path.sprintf("/composition_%d/vectorlegend_%d/layers/", mComposition->id(), mId );
  QStringList el = QgsProject::instance()->subkeyList ( "Compositions", path );
    
  for ( QStringList::iterator it = el.begin(); it != el.end(); ++it ) {
    int idx = (*it).find('_');

    QString id = (*it).right( (*it).length() - (idx+1) );
  
    path.sprintf("/composition_%d/vectorlegend_%d/layers/layer_%s/", mComposition->id(), mId, id.toLocal8Bit().data() );
    bool on = QgsProject::instance()->readBoolEntry("Compositions", path+"on", true, &ok);
    int group = QgsProject::instance()->readNumEntry("Compositions", path+"group", 0, &ok);
    setLayerOn ( id , on );
    setLayerGroup ( id, group );

    if ( group >= mNextLayerGroup ) mNextLayerGroup = group+1;
  }
    
    
  recalculate();
    
  return true;
}

bool QgsComposerVectorLegend::removeSettings( void )
{
    std::cerr << "QgsComposerVectorLegend::deleteSettings" << std::endl;

    QString path;
    path.sprintf("/composition_%d/vectorlegend_%d", mComposition->id(), mId ); 
    return QgsProject::instance()->removeEntry ( "Compositions", path );
}

bool QgsComposerVectorLegend::writeXML( QDomNode & node, QDomDocument & document, bool temp )
{
    return true;
}

bool QgsComposerVectorLegend::readXML( QDomNode & node )
{
    return true;
}
