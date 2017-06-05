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
#include "qgsgeometryutils.h"
#include "qgslinestringv2.h"
#include "qgsmapcanvas.h"
#include "qgspallabeling.h"
#include "qgspolygonv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgsrubberband.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
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
    AddFeatureCommand( QgsStateStack* stateStack, QgsMapToolDrawShape::State* newState, QgsVectorLayer* layer, const QgsFeature& feature )
        : StateChangeCommand( stateStack, newState, true ), mLayer( layer ), mFeature( feature )
    {}
    void undo() override
    {
      StateChangeCommand::undo();
      mLayer->dataProvider()->deleteFeatures( QgsFeatureIds() << mFeature.id() );
      mLayer->triggerRepaint();
    }
    void redo() override
    {
      StateChangeCommand::redo();
      QgsFeatureList flist = QgsFeatureList() << mFeature;
      mLayer->dataProvider()->addFeatures( flist );
      mFeature.setFeatureId( flist.first().id() ); // Update feature id so that delete works
      mLayer->triggerRepaint();
    }
  private:
    QgsVectorLayer* mLayer;
    QgsFeature mFeature;
};

template <class T>
void QgsRedliningMapToolT<T>::init( const QgsFeature* editFeature, QgsRedliningAttribEditor *editor )
{
  T::setSnapPoints( QSettings().value( "/qgis/snapping", false ).toBool() );
  mStandaloneEditor = 0;
  mBottomBar = new QgsRedliningBottomBar( this, editFeature ? editor : 0 );
  T::getRubberBand()->setIconType( QgsGeometryRubberBand::ICON_FULL_BOX );
  T::getRubberBand()->setIconOutlineColor( Qt::red );
  T::getRubberBand()->setIconFillColor( Qt::white );
  T::setShowInputWidget( QSettings().value( "/qgis/showNumericInput", false ).toBool() );
  if ( editFeature )
  {
    T::editGeometry( editFeature->geometry()->geometry(), mLayer->crs() );
    mLayer->dataProvider()->deleteFeatures( QgsFeatureIds() << editFeature->id() );
    mLayer->triggerRepaint();
    if ( editor )
    {
      editor->set( editFeature->attributes(), mLayer->dataProvider()->fields() );
    }
  }
  else
  {
    mStandaloneEditor = editor;
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
  const QgsFields& fields = mLayer->dataProvider()->fields();
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
    mStandaloneEditor->get( attribs, mLayer->dataProvider()->fields() );
    mStandaloneEditor->setParent( 0 );
    f.setAttributes( attribs );
    dialog.layout()->removeWidget( mStandaloneEditor ); // Remove to avoid QDialog deleting it
  }
  else if ( mBottomBar->editor() )
  {
    QgsAttributes attribs = f.attributes();
    mBottomBar->editor()->get( attribs, mLayer->dataProvider()->fields() );
    f.setAttributes( attribs );
  }
  if ( T::getStatus() == QgsMapToolDrawShape::StatusEditingMoving || T::getStatus() == QgsMapToolDrawShape::StatusEditingReady )
  {
    mLayer->dataProvider()->addFeatures( QgsFeatureList() << f );
    mLayer->triggerRepaint();
  }
  else
  {
    T::mStateStack.push( new AddFeatureCommand( &( T::mStateStack ), T::emptyState(), mLayer, f ) );
  }
}

template<>
QgsRedliningMapToolT<QgsMapToolDrawPolyLine>::QgsRedliningMapToolT( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& flags, const QgsFeature* editFeature, QgsRedliningAttribEditor* editor, bool isArea )
    : QgsMapToolDrawPolyLine( canvas, isArea ), mLayer( layer ), mFlags( flags )
{
  init( editFeature, editor );
}

const int QgsRedliningPointMapTool::sSizeRatio = 2;

QgsRedliningPointMapTool::QgsRedliningPointMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QString& symbol, const QgsFeature* editFeature, QgsRedliningAttribEditor* editor )
    : QgsRedliningMapToolT<QgsMapToolDrawPoint>( canvas, layer, QString( "shape=point,symbol=%1,w=%2*\"size\",h=%2*\"size\",r=0" ).arg( symbol ).arg( sSizeRatio ), editFeature, editor )
{
  if ( symbol == "circle" )
  {
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_CIRCLE );
  }
  else if ( symbol == "rectangle" )
  {
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_BOX );
  }
  else if ( symbol == "triangle" )
  {
    mRubberBand->setIconType( QgsGeometryRubberBand::ICON_TRIANGLE );
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

QgsRedliningEditTextMapTool::QgsRedliningEditTextMapTool( QgsMapCanvas* canvas, QgsVectorLayer* layer, const QgsLabelPosition& label, QgsRedliningAttribEditor *editor )
    : QgsMapToolPan( canvas ), mStatus( StatusReady ), mLayer( layer ), mLabel( label )
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
    editor->set( f.attributes(), mLayer->dataProvider()->fields() );
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
    mLayer->dataProvider()->deleteFeatures( QgsFeatureIds() << mLabel.featureId );
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
    mLayer->dataProvider()->deleteFeatures( QgsFeatureIds() << mLabel.featureId );
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

void QgsRedliningEditTextMapTool::updateStyle( int outlineWidth, const QColor& outlineColor, const QColor& fillColor, Qt::PenStyle lineStyle, Qt::BrushStyle brushStyle )
{
  const QgsFields& fields = mLayer->dataProvider()->fields();
  QgsAttributeMap attribs;
  attribs[fields.indexFromName( "size" )] = outlineWidth;
  attribs[fields.indexFromName( "outline" )] = QgsSymbolLayerV2Utils::encodeColor( outlineColor );
  attribs[fields.indexFromName( "fill" )] = QgsSymbolLayerV2Utils::encodeColor( fillColor );
  attribs[fields.indexFromName( "outline_style" )] = QgsSymbolLayerV2Utils::encodePenStyle( lineStyle );
  attribs[fields.indexFromName( "fill_style" )] = QgsSymbolLayerV2Utils::encodeBrushStyle( brushStyle );

  QgsChangedAttributesMap changedAttribs;
  changedAttribs[mLabel.featureId] = attribs;

  mLayer->dataProvider()->changeAttributeValues( changedAttribs );
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

  QgsGeometryMap geomMap;
  geomMap[mLabel.featureId] = QgsGeometry( state->pos.clone() );
  mLayer->dataProvider()->changeGeometryValues( geomMap );

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
  mBottomBar->editor()->get( attrs, mLayer->dataProvider()->fields() );

  QgsAttributeMap attribs;
  for ( int i = 0, n = attrs.size(); i < n; ++i )
  {
    attribs[i] = attrs[i];
  }
  QgsChangedAttributesMap changedAttribs;
  changedAttribs[mLabel.featureId] = attribs;
  mLayer->dataProvider()->changeAttributeValues( changedAttribs );
  mLayer->triggerRepaint();
}
