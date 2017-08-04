/***************************************************************************
    qgsredlining.cpp - Redlining support
     --------------------------------------
    Date                 : Sep 2015
    Copyright            : (C) 2015 Sandro Mani
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsmaptoolsredlining.h"
#include "qgscircularstringv2.h"
#include "qgscrscache.h"
#include "qgscurvepolygonv2.h"
#include "qgsfeaturepicker.h"
#include "qgsgeometryutils.h"
#include "qgslinestringv2.h"
#include "qgsmapcanvas.h"
#include "qgspallabeling.h"
#include "qgspolygonv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgsrubberband.h"
#include "qgsvectordataprovider.h"
#include "qgsredlining.h"
#include "qgsredlininglayer.h"
#include "nodetool/qgsselectedfeature.h"
#include "nodetool/qgsvertexentry.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSettings>


QgsRedliningBottomBar::QgsRedliningBottomBar( QgsMapTool* tool , QgsRedliningAttribEditor *editor )
    : QgsBottomBar( tool->canvas() ), mTool( tool ), mEditor( editor )
{
  QGridLayout* layout = new QGridLayout();
  setLayout( layout );

  int row = 0;
  if ( editor )
  {
    layout->addWidget( editor, row++, 0, 1, 3 );
    QFrame* line = new QFrame();
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    layout->addWidget( line, row++, 0, 1, 3 );
  }

  QPushButton* undoButton = new QPushButton( tr( "Undo" ) );
  undoButton->setEnabled( false );
  undoButton->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
  undoButton->setIcon( QIcon( ":/images/themes/default/mActionUndo.png" ) );
  connect( undoButton, SIGNAL( clicked( bool ) ), tool, SLOT( undo() ) );
  connect( tool, SIGNAL( canUndo( bool ) ), undoButton, SLOT( setEnabled( bool ) ) );
  layout->addWidget( undoButton, row, 0, 1, 1 );

  QPushButton* redoButton = new QPushButton( tr( "Redo" ) );
  redoButton->setEnabled( false );
  redoButton->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
  redoButton->setIcon( QIcon( ":/images/themes/default/mActionRedo.png" ) );
  connect( redoButton, SIGNAL( clicked( bool ) ), tool, SLOT( redo() ) );
  connect( tool, SIGNAL( canRedo( bool ) ), redoButton, SLOT( setEnabled( bool ) ) );
  layout->addWidget( redoButton, row, 1, 1, 1 );

  QPushButton* closeButton = new QPushButton();
  closeButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Preferred );
  closeButton->setIcon( QIcon( ":/images/themes/default/mIconClose.png" ) );
  closeButton->setToolTip( tr( "Close" ) );
  connect( closeButton, SIGNAL( clicked( bool ) ), this, SLOT( onClose() ) );

  layout->addWidget( closeButton, row, 2, 1, 1 );

  show();
  setFixedWidth( width() );
  updatePosition();
  if ( editor )
  {
    editor->setFocus();
  }
}

void QgsRedliningBottomBar::onClose()
{
  mTool->canvas()->unsetMapTool( mTool );
}
///////////////////////////////////////////////////////////////////////////////

template <class T>
class QgsRedliningMapToolT<T>::AddFeatureCommand : public QgsStateStack::StateChangeCommand
{
  public:
    AddFeatureCommand( QgsStateStack* stateStack, QgsMapToolDrawShape::State* newState, QgsRedliningLayer* layer, const QgsFeature& feature )
        : StateChangeCommand( stateStack, newState, true ), mLayer( layer ), mFeature( feature )
    {}
    void undo() override
    {
      StateChangeCommand::undo();
      mLayer->deleteFeature( mFeature.id() );
      mLayer->triggerRepaint();
    }
    void redo() override
    {
      StateChangeCommand::redo();
      QgsFeatureId newId = mLayer->addFeature( mFeature );
      mFeature.setFeatureId( newId ); // Update feature id so that delete works
      mLayer->triggerRepaint();
    }
  private:
    QgsRedliningLayer* mLayer;
    QgsFeature mFeature;
};

template <class T>
void QgsRedliningMapToolT<T>::init( const QgsFeature* editFeature, QgsRedliningAttribEditor *editor )
{
  T::setSnapPoints( QSettings().value( "/qgis/snapping/enabled", false ).toBool() );
  mStandaloneEditor = 0;
  mBottomBar = new QgsRedliningBottomBar( this, editFeature ? editor : 0 );
  T::getRubberBand()->setIconType( QgsGeometryRubberBand::ICON_FULL_BOX );
  T::getRubberBand()->setIconOutlineColor( Qt::red );
  T::getRubberBand()->setIconFillColor( Qt::white );
  T::setShowInputWidget( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  if ( editFeature )
  {
    mEditMode = true;
    T::editGeometry( editFeature->geometry()->geometry(), mLayer->crs() );
    mLayer->deleteFeature( editFeature->id() );
    mLayer->triggerRepaint();
    if ( editor )
    {
      editor->set( editFeature->attributes(), mLayer->pendingFields() );
    }
  }
  else
  {
    mStandaloneEditor = editor;
  }
}

template <class T>
void QgsRedliningMapToolT<T>::canvasReleaseEvent( QMouseEvent *ev )
{
  if ( mEditMode && ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier )
  {
    QgsFeaturePicker::PickResult result = QgsFeaturePicker::pick( T::canvas(), ev->pos(), T::toMapCoordinates( ev->pos() ), QGis::AnyGeometry );
    if ( result.layer == mLayer )
    {
      QList<QgsFeature> features = QList<QgsFeature>() << createFeature() << result.feature;
      T::mutableState()->status = T::StatusFinished; // Avoid onFinished being called, which adds the feature to the layer
      T::canvas()->setMapTool( new QgsRedliningEditGroupMapTool( T::canvas(), mRedlining, mLayer, features ) );
    }
  }
  else
  {
    T::canvasReleaseEvent( ev );
  }
}

template <class T>
QgsFeature QgsRedliningMapToolT<T>::createFeature() const
{
  QgsAbstractGeometryV2* geom = T::createGeometry( mLayer->crs() );
  if ( !geom || geom->isEmpty() )
  {
    delete geom;
    return QgsFeature();
  }
  const QgsFields& fields = mLayer->pendingFields();
  QgsFeature f( fields );
  QgsAttributes attribs = f.attributes();
  attribs[fields.fieldNameIndex( "flags" )] = mFlags;
  attribs[fields.fieldNameIndex( "size" )] = T::getRubberBand()->outlineWidth();
  attribs[fields.fieldNameIndex( "outline" )] = QgsSymbolLayerV2Utils::encodeColor( T::getRubberBand()->outlineColor() );
  attribs[fields.fieldNameIndex( "fill" )] = QgsSymbolLayerV2Utils::encodeColor( T::getRubberBand()->fillColor() );
  attribs[fields.fieldNameIndex( "outline_style" )] = QgsSymbolLayerV2Utils::encodePenStyle( T::getRubberBand()->lineStyle() );
  attribs[fields.fieldNameIndex( "fill_style" )] = QgsSymbolLayerV2Utils::encodeBrushStyle( T::getRubberBand()->brushStyle() );
  f.setAttributes( attribs );
  f.setGeometry( new QgsGeometry( geom ) );
  return f;
}

template <class T>
void QgsRedliningMapToolT<T>::onFinished()
{
  QgsFeature f( createFeature() );
  if ( !f.geometry() )
  {
    return;
  }
  if ( mStandaloneEditor )
  {
    QDialog dialog;
    dialog.setLayout( new QVBoxLayout() );
    dialog.layout()->addWidget( mStandaloneEditor );
    dialog.setWindowTitle( mStandaloneEditor->windowTitle() );
    QDialogButtonBox* bbox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
    QObject::connect( bbox, SIGNAL( accepted() ), &dialog, SLOT( accept() ) );
    QObject::connect( bbox, SIGNAL( rejected() ), &dialog, SLOT( reject() ) );
    dialog.layout()->addWidget( bbox );
    dialog.show();
    mStandaloneEditor->setFocus();
    if ( dialog.exec() != QDialog::Accepted || !mStandaloneEditor->isValid() )
    {
      dialog.layout()->removeWidget( mStandaloneEditor ); // Remove to avoid QDialog deleting it
      mStandaloneEditor->setParent( 0 );
      T::reset();
      return;
    }
    QgsAttributes attribs = f.attributes();
    mStandaloneEditor->get( attribs, mLayer->pendingFields() );
    mStandaloneEditor->setParent( 0 );
    f.setAttributes( attribs );
    dialog.layout()->removeWidget( mStandaloneEditor ); // Remove to avoid QDialog deleting it
  }
  else if ( mBottomBar->editor() )
  {
    QgsAttributes attribs = f.attributes();
    mBottomBar->editor()->get( attribs, mLayer->pendingFields() );
    f.setAttributes( attribs );
  }
  if ( T::getStatus() == QgsMapToolDrawShape::StatusEditingMoving || T::getStatus() == QgsMapToolDrawShape::StatusEditingReady )
  {
    mLayer->addFeature( f );
    mLayer->triggerRepaint();
  }
  else
  {
    T::mStateStack.push( new AddFeatureCommand( &( T::mStateStack ), T::emptyState(), mLayer, f ) );
  }
}

template<>
QgsRedliningMapToolT<QgsMapToolDrawPolyLine>::QgsRedliningMapToolT( QgsMapCanvas* canvas, QgsRedliningManager* redlining, QgsRedliningLayer* layer, const QString& flags, const QgsFeature* editFeature, QgsRedliningAttribEditor* editor, bool isArea )
    : QgsMapToolDrawPolyLine( canvas, isArea ), mRedlining( redlining ), mLayer( layer ), mFlags( flags )
{
  init( editFeature, editor );
}

const int QgsRedliningPointMapTool::sSizeRatio = 2;

QgsRedliningPointMapTool::QgsRedliningPointMapTool( QgsMapCanvas* canvas, QgsRedliningManager *redlining, QgsRedliningLayer* layer, const QString& symbol, const QgsFeature* editFeature, QgsRedliningAttribEditor* editor )
    : QgsRedliningMapToolT<QgsMapToolDrawPoint>( canvas, redlining, layer, QString( "shape=point,symbol=%1,w=%2*\"size\",h=%2*\"size\",r=0" ).arg( symbol ).arg( sSizeRatio ), editFeature, editor )
{
  if ( symbol == "circle" )
  {
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_CIRCLE );
  }
  else if ( symbol == "rectangle" )
  {
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_FULL_BOX );
  }
  else if ( symbol == "triangle" )
  {
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_FULL_TRIANGLE );
  }
  else
  {
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_NONE );
  }
  mNodeRubberband = new QgsRubberBand( canvas, QGis::Point );
  mNodeRubberband->setBorderColor( Qt::red );
  mNodeRubberband->setFillColor( Qt::white );
  mNodeRubberband->setIcon( QgsRubberBand::ICON_FULL_BOX );
  mNodeRubberband->setIconSize( 10 );
  mNodeRubberband->setWidth( 2 );
  updateNodeRubberband();
  connect( this, SIGNAL( geometryChanged() ), this, SLOT( updateNodeRubberband() ) );
}

void QgsRedliningPointMapTool::updateStyle( int outlineWidth, const QColor& outlineColor, const QColor& fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle )
{
  mRubberBand->setIconOutlineWidth( outlineWidth );
  mRubberBand->setIconOutlineColor( outlineColor );
  mRubberBand->setIconFillColor( fillColor );
  mRubberBand->setIconLineStyle( lineStyle );
  mRubberBand->setIconBrushStyle( brushStyle );
  // canvas()->logicalDpiX() / 25.48: mm -> px
  mRubberBand->setIconSize( sSizeRatio * canvas()->logicalDpiX() / 25.48 * outlineWidth );
  QgsMapToolDrawShape::updateStyle( outlineWidth, outlineColor, fillColor, lineStyle, brushStyle );
}

void QgsRedliningPointMapTool::updateNodeRubberband()
{
  const QgsPointV2* pos = static_cast<const QgsPointV2*>( mRubberBand->geometry() );
  if ( pos )
  {
    if ( mNodeRubberband->size() == 0 )
      mNodeRubberband->addPoint( QgsPoint( pos->x(), pos->y() ) );
    else
    {
      mNodeRubberband->movePoint( 0, QgsPoint( pos->x(), pos->y() ), 0, false );
      mNodeRubberband->movePoint( 1, QgsPoint( pos->x(), pos->y() ) );
    }
  }
  else
  {
    mNodeRubberband->setPoints( QList< QList<QgsPoint> >() );
  }
}

template class QgsRedliningMapToolT<QgsMapToolDrawPoint>;
template class QgsRedliningMapToolT<QgsMapToolDrawRectangle>;
template class QgsRedliningMapToolT<QgsMapToolDrawPolyLine>;
template class QgsRedliningMapToolT<QgsMapToolDrawCircle>;

///////////////////////////////////////////////////////////////////////////////

QgsRedliningEditGroupMapTool::QgsRedliningEditGroupMapTool( QgsMapCanvas* canvas, QgsRedliningManager *redlining, QgsRedliningLayer* layer, const QList<QgsFeature>& features )
    : QgsMapTool( canvas ), mRedlining( redlining ), mLayer( layer ), mDraggingRect( false )
{
  connect( this, SIGNAL( deactivated() ), this, SLOT( deleteLater() ) );
  connect( mLayer, SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
  connect( canvas, SIGNAL( renderComplete( QPainter* ) ), this, SLOT( updateRect() ) );

  mRectItem = new QGraphicsRectItem();
  mRectItem->setPen( QPen( Qt::black, 2, Qt::DashLine ) );
  canvas->scene()->addItem( mRectItem );

  for ( int i = 0, n = features.size(); i < n; ++i )
  {
    addFeatureToSelection( features[i], i == n - 1 );
  }
}

QgsRedliningEditGroupMapTool::~QgsRedliningEditGroupMapTool()
{
  while ( !mItems.isEmpty() )
  {
    removeItemFromSelection( 0, false );
  }
  delete mRectItem;
}

void QgsRedliningEditGroupMapTool::canvasPressEvent( QMouseEvent* e )
{
  if ( e->button() == Qt::LeftButton && ( e->modifiers() & Qt::ControlModifier ) == 0 )
  {
    mMouseMoveLastXY = e->posF();
    if ( mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
    {
      mDraggingRect = true;
      mCanvas->setCursor( Qt::SizeAllCursor );
    }
  }
}

void QgsRedliningEditGroupMapTool::canvasMoveEvent( QMouseEvent *e )
{
  if ( mDraggingRect )
  {
    QPointF newPos = e->posF();
    double dx = newPos.x() - mMouseMoveLastXY.x();
    double dy = newPos.y() - mMouseMoveLastXY.y();
    QgsPoint oldMapPos = toMapCoordinates( mMouseMoveLastXY.toPoint() );
    QgsPoint newMapPos = toMapCoordinates( newPos.toPoint() );
    double mdx = newMapPos.x() - oldMapPos.x();
    double mdy = newMapPos.y() - oldMapPos.y();
    foreach ( const Item& item, mItems )
    {
      double ox, oy;
      item.rubberband->translationOffset( ox, oy );
      ox += mdx;
      oy += mdy;
      item.rubberband->setTranslationOffset( ox, oy );
      if ( item.nodeRubberband )
      {
        item.nodeRubberband->setTranslationOffset( ox, oy );
      }
    }
    mRectItem->moveBy( dx, dy );
  }
  mMouseMoveLastXY = e->posF();
}

void QgsRedliningEditGroupMapTool::canvasReleaseEvent( QMouseEvent * e )
{
  if ( mDraggingRect )
  {
    foreach ( const Item& item, mItems )
    {
      double ox, oy;
      item.rubberband->translationOffset( ox, oy );
      QgsAbstractGeometryV2* geom = item.rubberband->geometry()->clone();
      geom->transform( QTransform::fromTranslate( ox, oy ) );
      item.rubberband->setGeometry( geom );
      item.rubberband->setTranslationOffset( 0, 0 );
      if ( item.nodeRubberband )
      {
        item.nodeRubberband->translationOffset( ox, oy );
        const QgsPoint* p = item.nodeRubberband->getPoint( 0 );
        if ( p )
        {
          QgsPoint newPos = QgsPoint( p->x() + ox, p->y() + oy );
          item.nodeRubberband->movePoint( 0, newPos, false );
          item.nodeRubberband->movePoint( 1, newPos );
        }
        item.nodeRubberband->setTranslationOffset( 0, 0 );
      }
    }
    mDraggingRect = false;
    mCanvas->setCursor( mCursor );
    updateRect();
  }
  else if ( e->button() == Qt::LeftButton )
  {
    QgsRenderContext renderContext = QgsRenderContext::fromMapSettings( canvas()->mapSettings() );
    double radiusmm = QSettings().value( "/Map/searchRadiusMM", QGis::DEFAULT_SEARCH_RADIUS_MM ).toDouble();
    radiusmm = radiusmm > 0 ? radiusmm : QGis::DEFAULT_SEARCH_RADIUS_MM;
    double mapTol = radiusmm * renderContext.scaleFactor() * renderContext.mapToPixel().mapUnitsPerPixel();
    QgsPoint mapPos = toMapCoordinates( e->pos() );

    int selectedItem = -1;
    for ( int i = 0, n = mItems.size(); i < n; ++i )
    {
      if ( mItems[i].rubberband->contains( mapPos, mapTol ) )
      {
        selectedItem = i;
        break;
      }
    }

    if ( e->modifiers() == Qt::ControlModifier && selectedItem >= 0 )
    {
      // CTRL+Clicked a selected item, deselect it
      removeItemFromSelection( selectedItem );
    }
    else
    {
      QgsFeaturePicker::PickResult result = QgsFeaturePicker::pick( canvas(), e->pos(), toMapCoordinates( e->pos() ), QGis::AnyGeometry );
      if ( result.layer == mLayer )
      {
        if (( e->modifiers() & Qt::ControlModifier ) == 0 )
        {
          // Clicked a new item without CTRL item, remove other items
          while ( !mItems.isEmpty() )
          {
            removeItemFromSelection( 0, false );
          }
        }
        // Add new item
        addFeatureToSelection( result.feature );
      }
      else if (( e->modifiers() & Qt::ControlModifier ) == 0 )
      {
        // Empty-clicked without control, quit tool
        deleteLater();
      }
    }
  }
  else if ( e->button() == Qt::RightButton  && !mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
  {
    deleteLater(); // quit tool
  }
}

void QgsRedliningEditGroupMapTool::keyReleaseEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    deleteLater(); // quit tool
  }
}

void QgsRedliningEditGroupMapTool::addFeatureToSelection( const QgsFeature &feature, bool update )
{
  QString flags = feature.attribute( "flags" ).toString();
  int outlineWidth = feature.attribute( "size" ).toInt();
  QColor outlineColor = QgsSymbolLayerV2Utils::decodeColor( feature.attribute( "outline" ).toString() );
  QColor fillColor = QgsSymbolLayerV2Utils::decodeColor( feature.attribute( "fill" ).toString() );
  Qt::PenStyle outlineStyle = QgsSymbolLayerV2Utils::decodePenStyle( feature.attribute( "outline_style" ).toString() );
  Qt::BrushStyle fillStyle = QgsSymbolLayerV2Utils::decodeBrushStyle( feature.attribute( "fill_style" ).toString() );

  const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( mLayer->crs().authid(), canvas()->mapSettings().destinationCrs().authid() );

  Item item;
  item.rubberband = new QgsGeometryRubberBand( canvas(), feature.geometry()->type() );
  item.rubberband->setGeometry( feature.geometry()->geometry()->transformed( *ct ) );
  item.nodeRubberband = 0;
  item.flags = flags;

  QRegExp shapeRe( "\\bshape=(\\w+)\\b" );
  shapeRe.indexIn( flags );
  QString shape = shapeRe.cap( 1 );
  if ( shape == "point" )
  {
    QRegExp symbolRe( "\\bsymbol=(\\w+)\\b" );
    symbolRe.indexIn( flags );
    QString symbol = symbolRe.cap( 1 );
    if ( symbol == "circle" )
    {
      item.rubberband->setIconType( QgsGeometryRubberBand::ICON_CIRCLE );
    }
    else if ( symbol == "rectangle" )
    {
      item.rubberband->setIconType( QgsGeometryRubberBand::ICON_FULL_BOX );
    }
    else if ( symbol == "triangle" )
    {
      item.rubberband->setIconType( QgsGeometryRubberBand::ICON_FULL_TRIANGLE );
    }
    else
    {
      item.rubberband->setIconType( QgsGeometryRubberBand::ICON_NONE );
    }
    // canvas()->logicalDpiX() / 25.48: mm -> px
    item.rubberband->setIconSize( QgsRedliningPointMapTool::sSizeRatio * canvas()->logicalDpiX() / 25.48 * outlineWidth );
    item.rubberband->setIconOutlineWidth( outlineWidth );
    item.rubberband->setIconOutlineColor( outlineColor );
    item.rubberband->setIconFillColor( fillColor );
    item.rubberband->setIconLineStyle( outlineStyle );
    item.rubberband->setIconBrushStyle( fillStyle );
    item.nodeRubberband = new QgsRubberBand( canvas(), QGis::Point );
    item.nodeRubberband->setBorderColor( Qt::red );
    item.nodeRubberband->setFillColor( Qt::white );
    item.nodeRubberband->setIcon( QgsRubberBand::ICON_FULL_BOX );
    item.nodeRubberband->setIconSize( 10 );
    item.nodeRubberband->setWidth( 2 );
    const QgsPointV2* pos = static_cast<const QgsPointV2*>( item.rubberband->geometry() );
    item.nodeRubberband->addPoint( QgsPoint( pos->x(), pos->y() ) );
  }
  else
  {
    item.rubberband->setIconSize( 10 );
    item.rubberband->setIconType( QgsGeometryRubberBand::ICON_FULL_BOX );
    item.rubberband->setIconOutlineColor( Qt::red );
    item.rubberband->setIconFillColor( Qt::white );
    item.rubberband->setOutlineWidth( outlineWidth );
    item.rubberband->setOutlineColor( outlineColor );
    item.rubberband->setFillColor( fillColor );
    item.rubberband->setLineStyle( outlineStyle );
    item.rubberband->setBrushStyle( fillStyle );
  }

  mLayer->deleteFeature( feature.id() );
  mLayer->triggerRepaint();

  mItems.append( item );

  if ( update )
  {
    updateRect();
  }
}

void QgsRedliningEditGroupMapTool::removeItemFromSelection( int itemIndex, bool update )
{
  Item& item = mItems[itemIndex];
  if ( mLayer )
  {
    QgsFeature feature = featureFromItem( item );
    mLayer->addFeature( feature );
  }
  if ( update )
  {
    updateRect();
  }
  mLayer->triggerRepaint();
  connect( canvas(), SIGNAL( mapCanvasRefreshed() ), item.rubberband, SLOT( deleteLater() ) );
  delete item.nodeRubberband;
  mItems.removeAt( itemIndex );
}

QgsFeature QgsRedliningEditGroupMapTool::featureFromItem( const Item &item ) const
{
  const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), mLayer->crs().authid() );
  const QgsFields& fields = mLayer->pendingFields();
  const QgsGeometryRubberBand* rubberBand = item.rubberband;
  QgsFeature f( fields );
  QgsAttributes attribs = f.attributes();
  attribs[fields.fieldNameIndex( "flags" )] = item.flags;
  if ( item.nodeRubberband )   // Is point
  {
    attribs[fields.fieldNameIndex( "size" )] = rubberBand->iconOutlineWidth();
    attribs[fields.fieldNameIndex( "outline" )] = QgsSymbolLayerV2Utils::encodeColor( rubberBand->iconOutlineColor() );
    attribs[fields.fieldNameIndex( "fill" )] = QgsSymbolLayerV2Utils::encodeColor( rubberBand->iconFillColor() );
    attribs[fields.fieldNameIndex( "outline_style" )] = QgsSymbolLayerV2Utils::encodePenStyle( rubberBand->iconLineStyle() );
    attribs[fields.fieldNameIndex( "fill_style" )] = QgsSymbolLayerV2Utils::encodeBrushStyle( rubberBand->iconBrushStyle() );
  }
  else
  {
    attribs[fields.fieldNameIndex( "size" )] = rubberBand->outlineWidth();
    attribs[fields.fieldNameIndex( "outline" )] = QgsSymbolLayerV2Utils::encodeColor( rubberBand->outlineColor() );
    attribs[fields.fieldNameIndex( "fill" )] = QgsSymbolLayerV2Utils::encodeColor( rubberBand->fillColor() );
    attribs[fields.fieldNameIndex( "outline_style" )] = QgsSymbolLayerV2Utils::encodePenStyle( rubberBand->lineStyle() );
    attribs[fields.fieldNameIndex( "fill_style" )] = QgsSymbolLayerV2Utils::encodeBrushStyle( rubberBand->brushStyle() );
  }
  f.setAttributes( attribs );
  f.setGeometry( new QgsGeometry( rubberBand->geometry()->transformed( *ct ) ) );
  return f;
}

void QgsRedliningEditGroupMapTool::updateRect()
{
  if ( mItems.isEmpty() )
  {
    deleteLater(); // quit tool if no items remain
    return;
  }
  int n = mItems.size();
  if ( n == 1 )
  {
    // Just one item, return to individual item editing mode
    Item& item = mItems.front();
    QgsFeature feature = featureFromItem( item );
    mLayer->addFeature( feature );
    mLayer->triggerRepaint();
    connect( canvas(), SIGNAL( mapCanvasRefreshed() ), item.rubberband, SLOT( deleteLater() ) );
    delete item.nodeRubberband;
    mItems.clear();
    deleteLater();
    mRedlining->editFeature( feature );
    return;
  }
  QRectF rect = mItems.front().rubberband->boundingRect().translated( mItems.front().rubberband->pos() );
  for ( int i = 1; i < n; ++i )
  {
    rect = rect.unite( mItems[i].rubberband->boundingRect().translated( mItems[i].rubberband->pos() ) );
  }
  mRectItem->setPos( 0, 0 );
  mRectItem->setRect( rect );
  mRectItem->setVisible( n > 1 );
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningEditTextMapTool::QgsRedliningEditTextMapTool( QgsMapCanvas* canvas, QgsRedliningLayer* layer, const QgsLabelPosition& label, QgsRedliningAttribEditor *editor )
    : QgsMapTool( canvas ), mStatus( StatusReady ), mLayer( layer ), mLabel( label )
{
  connect( mCanvas, SIGNAL( renderComplete( QPainter* ) ), this, SLOT( updateLabelBoundingBox() ) );
  setCursor( Qt::CrossCursor );

  mBottomBar = new QgsRedliningBottomBar( this, editor );

  mRubberBand = new QgsGeometryRubberBand( mCanvas, QGis::Line );
  mRubberBand->setOutlineColor( Qt::red );
  mRubberBand->setOutlineWidth( 1 );
  mRubberBand->setLineStyle( Qt::DashLine );
  mRubberBand->setIconType( QgsGeometryRubberBand::ICON_FULL_BOX );
  mRubberBand->setIconSize( 10 );
  mRubberBand->setIconOutlineColor( Qt::red );
  mRubberBand->setIconOutlineWidth( 2 );
  mRubberBand->setIconFillColor( Qt::white );
  updateRubberband( mLabel.labelRect );

  QgsFeature f;
  mLayer->getFeatures( QgsFeatureRequest( mLabel.featureId ) ).nextFeature( f );

  if ( editor )
  {
    editor->set( f.attributes(), mLayer->pendingFields() );
    connect( editor, SIGNAL( changed() ), this, SLOT( applyEditorChanges() ) );
  }

  State* initialState = new State;
  initialState->pos = *static_cast<QgsPointV2*>( f.geometry()->geometry() );
  mStateStack = new QgsStateStack( initialState, this );
  connect( mStateStack, SIGNAL( stateChanged() ), this, SLOT( update() ) );
  connect( mStateStack, SIGNAL( canUndoChanged( bool ) ), this, SIGNAL( canUndo( bool ) ) );
  connect( mStateStack, SIGNAL( canRedoChanged( bool ) ), this, SIGNAL( canRedo( bool ) ) );
}

QgsRedliningEditTextMapTool::~QgsRedliningEditTextMapTool()
{
  delete mRubberBand;
  delete mBottomBar;
}

void QgsRedliningEditTextMapTool::canvasPressEvent( QMouseEvent *e )
{
  mPressPos = toMapCoordinates( e->pos() );

  const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
  QList<QgsLabelPosition> labelPositions = labelingResults ? labelingResults->labelsAtPosition( mPressPos ) : QList<QgsLabelPosition>();

  bool labelClicked = false;
  foreach ( const QgsLabelPosition& labelPos, labelPositions )
  {
    if ( labelPos.layerID == mLayer->id() && labelPos.featureId == mLabel.featureId )
    {
      labelClicked = true;
      break;
    }
  }

  if ( !labelClicked && e->button() == Qt::RightButton )
  {
    canvas()->unsetMapTool( this );
  }
  else if ( labelClicked && e->button() == Qt::RightButton )
  {
    showContextMenu( e );
  }
  else if ( labelClicked )
  {
    mStatus = StatusMoving;
  }
}

void QgsRedliningEditTextMapTool::showContextMenu( QMouseEvent *e )
{
  QMenu menu;
  QAction* actionDelete = menu.addAction( tr( "Delete" ) );
  if ( menu.exec( e->globalPos() ) == actionDelete )
  {
    mLayer->deleteFeature( mLabel.featureId );
    mLayer->triggerRepaint();
    canvas()->unsetMapTool( this );
  }
}

void QgsRedliningEditTextMapTool::canvasMoveEvent( QMouseEvent *e )
{
  if ( mStatus == StatusMoving )
  {
    QgsPoint p = toMapCoordinates( e->pos() );
    mRubberBand->setTranslationOffset( p.x() - mPressPos.x(), p.y() - mPressPos.y() );
    mRubberBand->updatePosition();
  }
}

void QgsRedliningEditTextMapTool::canvasReleaseEvent( QMouseEvent */*e*/ )
{
  if ( mStatus == StatusMoving )
  {
    mStatus = StatusReady;
    double dx, dy;
    mRubberBand->translationOffset( dx, dy );
    QgsFeature f;
    mLayer->getFeatures( QgsFeatureRequest( mLabel.featureId ) ).nextFeature( f );

    QgsPointV2* layerPos = static_cast<QgsPointV2*>( f.geometry()->geometry() );
    QgsPoint mapPos = toMapCoordinates( mLayer, QgsPoint( layerPos->x(), layerPos->y() ) );
    mapPos.setX( mapPos.x() + dx );
    mapPos.setY( mapPos.y() + dy );

    State* newState = new State( *static_cast<const State*>( mStateStack->state() ) );
    newState->pos = QgsPointV2( toLayerCoordinates( mLayer, mapPos ) );
    mStateStack->updateState( newState );
  }
}

void QgsRedliningEditTextMapTool::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace )
  {
    mLayer->deleteFeature( mLabel.featureId );
    canvas()->unsetMapTool( this );
  }
  else if ( e->key() == Qt::Key_Escape )
  {
    canvas()->unsetMapTool( this );
  }
  else if ( e->key() == Qt::Key_Z && e->modifiers() == Qt::CTRL )
  {
    undo();
  }
  else if ( e->key() == Qt::Key_Y && e->modifiers() == Qt::CTRL )
  {
    redo();
  }
}

void QgsRedliningEditTextMapTool::updateStyle( int /*outlineWidth*/, const QColor& outlineColor, const QColor& fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle )
{
  const QgsFields& fields = mLayer->pendingFields();
  QgsAttributeMap attribs;
  attribs[fields.indexFromName( "outline" )] = QgsSymbolLayerV2Utils::encodeColor( outlineColor );
  attribs[fields.indexFromName( "fill" )] = QgsSymbolLayerV2Utils::encodeColor( fillColor );
  attribs[fields.indexFromName( "outline_style" )] = QgsSymbolLayerV2Utils::encodePenStyle( lineStyle );
  attribs[fields.indexFromName( "fill_style" )] = QgsSymbolLayerV2Utils::encodeBrushStyle( brushStyle );

  mLayer->changeAttributes( mLabel.featureId, attribs );
  mLayer->triggerRepaint();
}

void QgsRedliningEditTextMapTool::updateRubberband( const QgsRectangle &rect )
{
  mRubberBand->setTranslationOffset( 0, 0 );
  QgsLineStringV2* geom = new QgsLineStringV2();
  geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMinimum() ) );
  geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMaximum() ) );
  geom->addVertex( QgsPointV2( rect.xMaximum(), rect.yMaximum() ) );
  geom->addVertex( QgsPointV2( rect.xMaximum(), rect.yMinimum() ) );
  geom->addVertex( QgsPointV2( rect.xMinimum(), rect.yMinimum() ) );
  mRubberBand->setGeometry( geom );
}

void QgsRedliningEditTextMapTool::update()
{
  const State* state = static_cast<const State*>( mStateStack->state() );
  mLayer->changeGeometry( mLabel.featureId, QgsGeometry( state->pos.clone() ) );
  mLayer->triggerRepaint();
}

void QgsRedliningEditTextMapTool::updateLabelBoundingBox()
{
  const State* state = static_cast<const State*>( mStateStack->state() );
  QgsPoint mapPos = toMapCoordinates( mLayer, QgsPoint( state->pos.x(), state->pos.y() ) );
  // Try to find the label again
  const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
  if ( labelingResults )
  {
    foreach ( const QgsLabelPosition& labelPos, labelingResults->labelsAtPosition( mapPos ) )
    {
      if ( labelPos.layerID == mLabel.layerID && labelPos.featureId == mLabel.featureId )
      {
        mLabel = labelPos;
        updateRubberband( mLabel.labelRect );
        break;
      }
    }
  }
  // Label disappeared? Should not happen
}

void QgsRedliningEditTextMapTool::applyEditorChanges()
{
  QgsFeature f;
  mLayer->getFeatures( QgsFeatureRequest( mLabel.featureId ) ).nextFeature( f );
  QgsAttributes attrs = f.attributes();
  mBottomBar->editor()->get( attrs, mLayer->pendingFields() );

  QgsAttributeMap attribs;
  for ( int i = 0, n = attrs.size(); i < n; ++i )
  {
    attribs[i] = attrs[i];
  }
  mLayer->changeAttributes( mLabel.featureId, attribs );
  mLayer->triggerRepaint();
}
