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
#include "qgsannotationlayer.h"
#include "qgisapp.h"
#include "qgsclipboard.h"
#include "qgscrscache.h"
#include "qgsfeaturepicker.h"
#include "qgsfeaturestore.h"
#include "qgsgeometryutils.h"
#include "qgslinestringv2.h"
#include "qgsmapcanvas.h"
#include "qgspallabeling.h"
#include "qgspinannotationitem.h"
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  T::connect( this, &T::finished, this, &QgsRedliningMapToolT<T>::onFinished );
#endif

  T::setSnapPoints( QSettings().value( "/Qgis/snapping/enabled", false ).toBool() );
  mStandaloneEditor = 0;
  mBottomBar = new QgsRedliningBottomBar( this, editFeature ? editor : 0 );
  T::getRubberBand()->setIconType( QgsGeometryRubberBand::ICON_FULL_BOX );
  T::getRubberBand()->setIconOutlineColor( Qt::red );
  T::getRubberBand()->setIconFillColor( Qt::white );
  T::setShowInputWidget( QSettings().value( "/Qgis/showNumericInput", false ).toBool() );
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
void QgsRedliningMapToolT<T>::canvasPressEvent( QMouseEvent *ev )
{
  if ( mEditMode && ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier )
  {
    // pass
  }
  else
  {
    T::canvasPressEvent( ev );
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
      QgsFeature editFeature = createFeature();
      if ( mBottomBar->editor() )
      {
        QgsAttributes attribs = editFeature.attributes();
        mBottomBar->editor()->get( attribs, mLayer->pendingFields() );
        editFeature.setAttributes( attribs );
      }
      QList<QgsFeature> features;
      features.append( editFeature );
      QList<QgsLabelPosition> labels;
      if ( result.feature.isValid() )
      {
        features.append( result.feature );
      }
      else if ( result.labelPos.featureId != -1 )
      {
        labels.append( result.labelPos );
      }
      T::mutableState()->status = T::StatusFinished; // Avoid onFinished being called, which adds the feature to the layer
      T::canvas()->setMapTool( new QgsRedliningEditGroupMapTool( T::canvas(), mRedlining, mLayer, features, labels ) );
    }
  }
  else
  {
    T::canvasReleaseEvent( ev );
  }
}

template <class T>
void QgsRedliningMapToolT<T>::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier )
  {
    copy();
  }
  else if ( e->key() == Qt::Key_X && e->modifiers() == Qt::ControlModifier )
  {
    cut();
  }
  else
  {
    T::keyPressEvent( e );
  }
}

template <class T>
void QgsRedliningMapToolT<T>::copy()
{
  QgsFeatureStore featureStore( mLayer->pendingFields(), mLayer->crs() );
  featureStore.addFeature( createFeature() );
  QgisApp::instance()->clipboard()->setStoredFeatures( featureStore );
}

template <class T>
void QgsRedliningMapToolT<T>::cut()
{
  copy();
  T::deleteShape();
}

template <class T>
void QgsRedliningMapToolT<T>::addContextMenuActions( const QgsMapToolDrawShape::EditContext* context, QMenu& menu ) const
{
  T::addContextMenuActions( context, menu );
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
  if ( !menu.isEmpty() )
  {
    menu.addSeparator();
  }
  menu.addAction( QIcon( ":/images/themes/default/mActionEditCopy.png" ), QApplication::translate( "QgsRedliningMapTool", "Copy" ), this, &QgsRedliningMapToolT<T>::copy );
  menu.addAction( QIcon( ":/images/themes/default/mActionEditCut.png" ), QApplication::translate( "QgsRedliningMapTool", "Cut" ), this, &QgsRedliningMapToolT<T>::cut );
#endif
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
    mStandaloneEditor->set( f.attributes(), mLayer->pendingFields() );
    QDialog dialog( T::canvas() );
    dialog.setModal( true );
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
    : QgsRedliningMapToolT<QgsMapToolDrawPoint>( canvas, redlining, layer, QString( "shape=point,symbol=%1,r=0" ).arg( symbol ).arg( sSizeRatio ), editFeature, editor )
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

QgsRedliningEditGroupMapTool::QgsRedliningEditGroupMapTool( QgsMapCanvas* canvas, QgsRedliningManager *redlining, QgsRedliningLayer* layer, const QList<QgsFeature>& features , const QList<QgsLabelPosition> &labels )
    : QgsMapTool( canvas ), mRedlining( redlining ), mLayer( layer ), mDraggingRect( false )
{
  connect( this, SIGNAL( deactivated() ), this, SLOT( deleteLater() ) );
  connect( mLayer, SIGNAL( destroyed( QObject* ) ), this, SLOT( deleteLater() ) );
  connect( canvas, SIGNAL( renderComplete( QPainter* ) ), this, SLOT( updateLabelBoundingBoxes() ) );
  connect( canvas, SIGNAL( renderComplete( QPainter* ) ), this, SLOT( updateRect() ) );

  mRectItem = new QGraphicsRectItem();
  mRectItem->setPen( QPen( Qt::black, 2, Qt::DashLine ) );
  canvas->scene()->addItem( mRectItem );

  for ( int i = 0, n = features.size(); i < n; ++i )
  {
    addFeatureToSelection( features[i], false );
  }
  for ( int i = 0, n = labels.size(); i < n; ++i )
  {
    addLabelToSelection( labels[i] );
  }
  updateRect();
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
  else if ( e->button() == Qt::RightButton && mRectItem->contains( canvas()->mapToScene( e->pos() ) ) )
  {
    QMenu menu;
    menu.addAction( QIcon( ":/images/themes/default/mActionEditCopy.png" ), tr( "Copy" ), this, SLOT( copy() ) );
    menu.addAction( QIcon( ":/images/themes/default/mActionEditCut.png" ), tr( "Cut" ), this, SLOT( cut() ) );
    menu.addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteAll() ) );
    bool points = true;
    for ( const Item& item : mItems )
    {
      if ( QgsRedliningLayer::deserializeFlags( item.flags )["shape"] != "point" )
      {
        points = false;
        break;
      }
    }
    if ( points )
    {
      menu.addAction( QIcon( ":/images/themes/default/pin_red.svg" ), tr( "Convert to pins" ), this, SLOT( convertToPins() ) );
    }
    menu.exec( e->globalPos() );
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
      if ( item.labelFeature != -1 )
      {
        double dx, dy;
        item.rubberband->translationOffset( dx, dy );
        QgsFeature f;
        mLayer->getFeatures( QgsFeatureRequest( item.labelFeature ) ).nextFeature( f );

        QgsPointV2* layerPos = static_cast<QgsPointV2*>( f.geometry()->geometry() );
        QgsPoint mapPos = toMapCoordinates( mLayer, QgsPoint( layerPos->x(), layerPos->y() ) );
        mapPos.setX( mapPos.x() + dx );
        mapPos.setY( mapPos.y() + dy );
        QgsPoint newLayerPos = toLayerCoordinates( mLayer, mapPos );
        mLayer->changeGeometry( item.labelFeature, QgsGeometry( new QgsPointV2( newLayerPos ) ) );
        mLayer->triggerRepaint();
      }
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
        if ( result.feature.isValid() )
        {
          addFeatureToSelection( result.feature );
        }
        else if ( result.labelPos.featureId != -1 )
        {
          addLabelToSelection( result.labelPos );
        }
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

void QgsRedliningEditGroupMapTool::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Escape )
  {
    deleteLater(); // quit tool
  }
  else if ( e->key() == Qt::Key_Delete )
  {
    deleteAll();
  }
  else if ( e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier )
  {
    copy();
  }
  else if ( e->key() == Qt::Key_X && e->modifiers() == Qt::ControlModifier )
  {
    cut();
  }
}

void QgsRedliningEditGroupMapTool::updateLabelRect( QgsGeometryRubberBand *rubberBand, const QgsLabelPosition &label )
{
  QgsLineStringV2* exterior = new QgsLineStringV2();
  exterior->addVertex( QgsPointV2( label.labelRect.xMinimum(), label.labelRect.yMinimum() ) );
  exterior->addVertex( QgsPointV2( label.labelRect.xMinimum(), label.labelRect.yMaximum() ) );
  exterior->addVertex( QgsPointV2( label.labelRect.xMaximum(), label.labelRect.yMaximum() ) );
  exterior->addVertex( QgsPointV2( label.labelRect.xMaximum(), label.labelRect.yMinimum() ) );
  exterior->addVertex( QgsPointV2( label.labelRect.xMinimum(), label.labelRect.yMinimum() ) );
  QgsPolygonV2* poly = new QgsPolygonV2();
  poly->setExteriorRing( exterior );
  rubberBand->setGeometry( poly );
}

void QgsRedliningEditGroupMapTool::addLabelToSelection( const QgsLabelPosition& label, bool update )
{
  QgsFeature f;
  mLayer->getFeatures( QgsFeatureRequest( label.featureId ) ).nextFeature( f );
  QMap<QString, QString> flags = mLayer->deserializeFlags( f.attribute( "flags" ).toString() );
  if ( !flags["symbol"].isEmpty() )
  {
    // If label associated to a visible shape is selected, add the feature itself to the selection
    addFeatureToSelection( f );
    return;
  }
  QgsGeometryRubberBand* rubberBand = new QgsGeometryRubberBand( mCanvas, QGis::Line );
  rubberBand->setOutlineColor( Qt::red );
  rubberBand->setOutlineWidth( 1 );
  rubberBand->setLineStyle( Qt::DashLine );
  rubberBand->setIconType( QgsGeometryRubberBand::ICON_FULL_BOX );
  rubberBand->setIconSize( 10 );
  rubberBand->setIconOutlineColor( Qt::red );
  rubberBand->setIconOutlineWidth( 2 );
  rubberBand->setIconFillColor( Qt::white );
  rubberBand->setTranslationOffset( 0, 0 );
  updateLabelRect( rubberBand, label );

  Item item;
  item.rubberband = rubberBand;
  item.nodeRubberband = 0;
  item.labelFeature = label.featureId;

  mItems.append( item );

  if ( update )
  {
    updateRect();
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
  item.text = feature.attribute( "text" ).toString();
  item.labelFeature = -1;

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
    item.rubberband->setIconOutlineWidth( 2 );
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
  if ( item.labelFeature != -1 )
  {
    delete item.rubberband;
  }
  else
  {
    if ( mLayer )
    {
      QgsFeature feature = featureFromItem( item );
      mLayer->addFeature( feature );
      mLayer->triggerRepaint();
    }
    connect( canvas(), SIGNAL( mapCanvasRefreshed() ), item.rubberband, SLOT( deleteLater() ) );
  }
  delete item.nodeRubberband;
  mItems.removeAt( itemIndex );
  if ( update )
  {
    updateRect();
  }
}

QgsFeature QgsRedliningEditGroupMapTool::featureFromItem( const Item &item ) const
{
  const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( canvas()->mapSettings().destinationCrs().authid(), mLayer->crs().authid() );
  const QgsFields& fields = mLayer->pendingFields();
  const QgsGeometryRubberBand* rubberBand = item.rubberband;
  QgsFeature f( fields );
  QgsAttributes attribs = f.attributes();
  attribs[fields.fieldNameIndex( "flags" )] = item.flags;
  attribs[fields.fieldNameIndex( "text" )] = item.text;
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

void QgsRedliningEditGroupMapTool::convertToPins()
{
  for ( const Item& item : mItems )
  {
    QgsPinAnnotationItem* pinItem = new QgsPinAnnotationItem( mCanvas );
    pinItem->setMapPosition( *item.nodeRubberband->getPoint( 0 ) );
    pinItem->setName( item.text );
    QgsAnnotationLayer::getLayer( mCanvas, "mapPins", tr( "Pins" ) )->addItem( pinItem );
  }

  deleteAll();
}

void QgsRedliningEditGroupMapTool::copy()
{
  QgsFeatureStore featureStore( mLayer->pendingFields(), mLayer->crs() );
  foreach ( const Item& item, mItems )
  {
    if ( item.labelFeature != -1 )
    {
      QgsFeature f;
      if ( mLayer->getFeatures( QgsFeatureRequest( item.labelFeature ) ).nextFeature( f ) )
      {
        featureStore.addFeature( f );
      }
    }
    else
    {
      featureStore.addFeature( featureFromItem( item ) );
    }
  }
  QgisApp::instance()->clipboard()->setStoredFeatures( featureStore );
}

void QgsRedliningEditGroupMapTool::deleteAll()
{
  foreach ( const Item& item, mItems )
  {
    delete item.rubberband;
    delete item.nodeRubberband;
    if ( item.labelFeature )
    {
      mLayer->deleteFeature( item.labelFeature );
    }
  }
  mItems.clear();
  mLayer->triggerRepaint();
  deleteLater(); // quit tool
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
    if ( item.labelFeature != -1 )
    {
      const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
      QList<QgsLabelPosition> labelPositions = labelingResults ? labelingResults->labelsWithinRect( item.rubberband->geometry()->boundingBox() ) : QList<QgsLabelPosition>();
      foreach ( const QgsLabelPosition& labelPos, labelPositions )
      {
        if ( labelPos.layerID == mLayer->id() && labelPos.featureId == item.labelFeature )
        {
          mRedlining->editLabel( labelPos );
          break;
        }
      }
      delete item.rubberband;
    }
    else
    {
      QgsFeature feature = featureFromItem( item );
      feature.setFeatureId( mLayer->addFeature( feature ) );
      mLayer->triggerRepaint();
      connect( canvas(), SIGNAL( mapCanvasRefreshed() ), item.rubberband, SLOT( deleteLater() ) );
      mRedlining->editFeature( feature );
    }
    delete item.nodeRubberband;
    mItems.clear();
    deleteLater();
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

void QgsRedliningEditGroupMapTool::updateLabelBoundingBoxes()
{
  // Try to find the label again
  const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
  if ( labelingResults )
  {
    foreach ( const Item& item, mItems )
    {
      if ( item.labelFeature != -1 )
      {
        QgsFeature f;
        mLayer->getFeatures( QgsFeatureRequest( item.labelFeature ) ).nextFeature( f );
        QgsPointV2* layerPos = static_cast<QgsPointV2*>( f.geometry()->geometry() );
        QgsPoint mapPos = toMapCoordinates( mLayer, QgsPoint( layerPos->x(), layerPos->y() ) );
        foreach ( const QgsLabelPosition& labelPos, labelingResults->labelsAtPosition( mapPos ) )
        {
          if ( labelPos.layerID == mLayer->id() && labelPos.featureId == item.labelFeature )
          {
            updateLabelRect( item.rubberband, labelPos );
            break;
          }
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsRedliningEditTextMapTool::QgsRedliningEditTextMapTool( QgsMapCanvas* canvas, QgsRedliningManager *redlining, QgsRedliningLayer* layer, const QgsLabelPosition& label, QgsRedliningAttribEditor *editor )
    : QgsMapTool( canvas ), mStatus( StatusReady ), mRedlining( redlining ), mLayer( layer ), mLabel( label )
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
  if ( e->modifiers() != Qt::ControlModifier )
  {
    mPressPos = toMapCoordinates( e->pos() );

    bool textClicked = labelClicked( mPressPos );
    if ( textClicked && e->button() == Qt::RightButton )
    {
      showContextMenu( e );
    }
    else if ( textClicked )
    {
      mStatus = StatusMoving;
    }
  }
}

void QgsRedliningEditTextMapTool::showContextMenu( QMouseEvent *e )
{
  QMenu menu;
  menu.addAction( QIcon( ":/images/themes/default/mActionEditCopy.png" ), tr( "Copy" ), this, SLOT( copy() ) );
  menu.addAction( QIcon( ":/images/themes/default/mActionEditCut.png" ), tr( "Cut" ), this, SLOT( cut() ) );
  menu.addAction( QIcon( ":/images/themes/default/mActionDeleteSelected.svg" ), tr( "Delete" ), this, SLOT( deleteLabel() ) );
  menu.exec( e->globalPos() );
}

void QgsRedliningEditTextMapTool::canvasMoveEvent( QMouseEvent *e )
{
  if ( mStatus == StatusMoving )
  {
    QgsPoint mapPos = toMapCoordinates( e->pos() );
    mRubberBand->setTranslationOffset( mapPos.x() - mPressPos.x(), mapPos.y() - mPressPos.y() );
    mRubberBand->updatePosition();
  }
}

void QgsRedliningEditTextMapTool::canvasReleaseEvent( QMouseEvent *e )
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
  else if ( e->button() == Qt::LeftButton && e->modifiers() == Qt::ControlModifier )
  {
    QgsFeaturePicker::PickResult result = QgsFeaturePicker::pick( canvas(), e->pos(), toMapCoordinates( e->pos() ), QGis::AnyGeometry );
    if ( result.layer == mLayer )
    {
      QList<QgsFeature> features;
      QList<QgsLabelPosition> labels = QList<QgsLabelPosition>() << mLabel;
      if ( result.feature.isValid() )
      {
        features.append( result.feature );
      }
      else if ( result.labelPos.featureId != -1 )
      {
        labels.append( result.labelPos );
      }
      canvas()->setMapTool( new QgsRedliningEditGroupMapTool( canvas(), mRedlining, mLayer, features, labels ) );
    }
  }
  else if ( !labelClicked( toMapCoordinates( e->pos() ) ) )
  {
    canvas()->unsetMapTool( this );
  }
}

void QgsRedliningEditTextMapTool::keyPressEvent( QKeyEvent *e )
{
  if ( e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace )
  {
    deleteLabel();
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
  else if ( e->key() == Qt::Key_C && e->modifiers() == Qt::ControlModifier )
  {
    copy();
  }
  else if ( e->key() == Qt::Key_X && e->modifiers() == Qt::ControlModifier )
  {
    cut();
  }
}

void QgsRedliningEditTextMapTool::copy()
{
  QgsFeatureStore featureStore( mLayer->pendingFields(), mLayer->crs() );
  QgsFeature f;
  if ( mLayer->getFeatures( QgsFeatureRequest( mLabel.featureId ) ).nextFeature( f ) )
  {
    featureStore.addFeature( f );
    QgisApp::instance()->clipboard()->setStoredFeatures( featureStore );
  }
}

void QgsRedliningEditTextMapTool::deleteLabel()
{
  mLayer->deleteFeature( mLabel.featureId );
  mLayer->triggerRepaint();
  canvas()->unsetMapTool( this );
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

bool QgsRedliningEditTextMapTool::labelClicked( const QgsPoint& mapPos ) const
{
  const QgsLabelingResults* labelingResults = mCanvas->labelingResults();
  QList<QgsLabelPosition> labelPositions = labelingResults ? labelingResults->labelsAtPosition( mapPos ) : QList<QgsLabelPosition>();
  bool labelClicked = false;
  foreach ( const QgsLabelPosition& labelPos, labelPositions )
  {
    if ( labelPos.layerID == mLayer->id() && labelPos.featureId == mLabel.featureId )
    {
      labelClicked = true;
      break;
    }
  }
  return labelClicked;
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
