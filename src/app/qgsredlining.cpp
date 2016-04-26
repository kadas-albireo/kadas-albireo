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

#include "qgsredlining.h"
#include "qgisapp.h"
#include "qgscolorbuttonv2.h"
#include "qgsribbonapp.h"
#include "qgslayertreemodel.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptooladdfeature.h"
#include "qgsmaptoolsredlining.h"
#include "qgsredlininglayer.h"
#include "qgsredliningtextdialog.h"
#include "qgsrubberband.h"
#include "qgssymbollayerv2utils.h"
#include "qgsproject.h"

#include <QMenu>
#include <QSettings>
#include <QToolBar>


class QgsRedlining::LabelEditor : public QgsRedliningAttributeEditor
{
  public:
    QString getName() const override { return tr( "label" ); }

    bool exec( QgsFeature& feature, QStringList& changedAttributes ) override
    {
      QMap<QString, QString> flagsMap = QgsRedliningLayer::deserializeFlags( feature.attribute( "flags" ).toString() );
      QFont font;
      font.fromString( QSettings().value( "/Redlining/font", font.toString() ).toString() );
      font.setFamily( flagsMap.value( "family", font.family() ) );
      font.setPointSize( flagsMap.value( "fontSize", QString( "%1" ).arg( font.pointSize() ) ).toInt() );
      font.setItalic( flagsMap.value( "italic", QString( "%1" ).arg( font.italic() ) ).toInt() );
      font.setBold( flagsMap.value( "bold", QString( "%1" ).arg( font.bold() ) ).toInt() );
      double rotation = flagsMap.value( "rotation" ).toDouble();

      QgsRedliningTextDialog textDialog( feature.attribute( "text" ).toString(), font, rotation );
      if ( textDialog.exec() != QDialog::Accepted || textDialog.currentText().isEmpty() )
      {
        return false;
      }
      feature.setAttribute( "text", textDialog.currentText() );
      font = textDialog.currentFont();
      flagsMap["family"] = font.family();
      flagsMap["italic"] = QString( "%1" ).arg( font.italic() );
      flagsMap["bold"] = QString( "%1" ).arg( font.bold() );
      flagsMap["rotation"] = QString( "%1" ).arg( textDialog.rotation() );
      flagsMap["fontSize"] = QString( "%1" ).arg( font.pointSize() );

      feature.setAttribute( "flags", QgsRedliningLayer::serializeFlags( flagsMap ) );
      changedAttributes.append( "text" );
      changedAttributes.append( "flags" );
      return true;
    }
};


QgsRedlining::QgsRedlining( QgisApp *app, const RedliningUi& ui )
    : QObject( app )
    , mApp( app )
    , mUi( ui )
    , mLayer( 0 )
    , mLayerRefCount( 0 )
{
  QAction* actionNewMarker = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Marker" ), this );

  mActionNewPoint = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Point" ), this );
  mActionNewPoint->setCheckable( true );
  connect( mActionNewPoint, SIGNAL( triggered( bool ) ), this, SLOT( newPoint( bool ) ) );

  mActionNewSquare = new QAction( QIcon( ":/images/themes/default/redlining_square.svg" ), tr( "Square" ), this );
  mActionNewSquare->setCheckable( true );
  connect( mActionNewSquare, SIGNAL( triggered( bool ) ), this, SLOT( newSquare( bool ) ) );

  mActionNewTriangle = new QAction( QIcon( ":/images/themes/default/redlining_triangle.svg" ), tr( "Triangle" ), this );
  mActionNewTriangle->setCheckable( true );
  connect( mActionNewTriangle, SIGNAL( triggered( bool ) ), this, SLOT( newTriangle( bool ) ) );

  mActionNewLine = new QAction( QIcon( ":/images/themes/default/redlining_line.svg" ), tr( "Line" ), this );
  mActionNewLine->setCheckable( true );
  connect( mActionNewLine, SIGNAL( triggered( bool ) ), this, SLOT( newLine( bool ) ) );
  mActionNewRectangle = new QAction( QIcon( ":/images/themes/default/redlining_rectangle.svg" ), tr( "Rectangle" ), this );
  mActionNewRectangle->setCheckable( true );
  connect( mActionNewRectangle, SIGNAL( triggered( bool ) ), this, SLOT( newRectangle( bool ) ) );
  mActionNewPolygon = new QAction( QIcon( ":/images/themes/default/redlining_polygon.svg" ), tr( "Polygon" ), this );
  mActionNewPolygon->setCheckable( true );
  connect( mActionNewPolygon, SIGNAL( triggered( bool ) ), this, SLOT( newPolygon( bool ) ) );
  mActionNewCircle = new QAction( QIcon( ":/images/themes/default/redlining_circle.svg" ), tr( "Circle" ), this );
  mActionNewCircle->setCheckable( true );
  connect( mActionNewCircle, SIGNAL( triggered( bool ) ), this, SLOT( newCircle( bool ) ) );
  mActionNewText = new QAction( QIcon( ":/images/themes/default/redlining_text.svg" ), tr( "Text" ), this );
  mActionNewText->setCheckable( true );
  connect( mActionNewText, SIGNAL( triggered( bool ) ), this, SLOT( newText( bool ) ) );

  mUi.buttonNewObject->setToolTip( tr( "New Object" ) );
  QMenu* menuNewMarker = new QMenu();
  menuNewMarker->addAction( mActionNewPoint );
  menuNewMarker->addAction( mActionNewSquare );
  menuNewMarker->addAction( mActionNewTriangle );
  actionNewMarker->setMenu( menuNewMarker );
  QMenu* menuNewObject = new QMenu();
  menuNewObject->addAction( actionNewMarker );
  menuNewObject->addAction( mActionNewLine );
  menuNewObject->addAction( mActionNewRectangle );
  menuNewObject->addAction( mActionNewPolygon );
  menuNewObject->addAction( mActionNewCircle );
  menuNewObject->addAction( mActionNewText );
  mUi.buttonNewObject->setMenu( menuNewObject );
  mUi.buttonNewObject->setPopupMode( QToolButton::MenuButtonPopup );
  mUi.buttonNewObject->setDefaultAction( mActionNewPoint );

  mActionEditObject = new QAction( QIcon( ":/images/themes/default/mActionNodeTool.png" ), QString(), this );
  mActionEditObject->setToolTip( tr( "Edit Object" ) );
  mActionEditObject->setCheckable( true );
  connect( mActionEditObject, SIGNAL( triggered( bool ) ), this, SLOT( editObject() ) );

  mUi.spinBoxSize->setRange( 1, 100 );
  mUi.spinBoxSize->setValue( QSettings().value( "/Redlining/size", 1 ).toInt() );
  connect( mUi.spinBoxSize, SIGNAL( valueChanged( int ) ), this, SLOT( saveOutlineWidth() ) );

  mUi.colorButtonOutlineColor->setAllowAlpha( true );
  mUi.colorButtonOutlineColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/Redlining/outline_color", "0,0,0,255" ).toString() );
  mUi.colorButtonOutlineColor->setColor( initialOutlineColor );
  connect( mUi.colorButtonOutlineColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );

  mUi.comboOutlineStyle->setProperty( "settings_key", "outline_style" );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::NoPen ), QString(), Qt::NoPen );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::SolidLine ), QString(), Qt::SolidLine );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashLine ), QString(), Qt::DashLine );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashDotLine ), QString(), Qt::DashDotLine );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DotLine ), QString(), Qt::DotLine );
  mUi.comboOutlineStyle->setCurrentIndex( QSettings().value( "/Redlining/outline_style", "1" ).toInt() );
  connect( mUi.comboOutlineStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveStyle() ) );

  mUi.colorButtonFillColor->setAllowAlpha( true );
  mUi.colorButtonFillColor->setProperty( "settings_key", "fill_color" );
  QColor initialFillColor = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/Redlining/fill_color", "255,0,0,255" ).toString() );
  mUi.colorButtonFillColor->setColor( initialFillColor );
  connect( mUi.colorButtonFillColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );

  mUi.comboFillStyle->setProperty( "settings_key", "fill_style" );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::NoBrush ), QString(), Qt::NoBrush );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::SolidPattern ), QString(), Qt::SolidPattern );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::HorPattern ), QString(), Qt::HorPattern );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::VerPattern ), QString(), Qt::VerPattern );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::BDiagPattern ), QString(), Qt::BDiagPattern );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::DiagCrossPattern ), QString(), Qt::DiagCrossPattern );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::FDiagPattern ), QString(), Qt::FDiagPattern );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::CrossPattern ), QString(), Qt::CrossPattern );
  mUi.comboFillStyle->setCurrentIndex( QSettings().value( "/Redlining/fill_style", "1" ).toInt() );
  connect( mUi.comboFillStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveStyle() ) );

  connect( app, SIGNAL( newProject() ), this, SLOT( clearLayer() ) );
  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProject( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProject( QDomDocument& ) ) );
}

QgsRedliningLayer* QgsRedlining::getOrCreateLayer()
{
  if ( mLayer )
  {
    return mLayer;
  }
  mLayer = new QgsRedliningLayer( tr( "Redlining" ) );
  QgsMapLayerRegistry::instance()->addMapLayer( mLayer, true, true );
  mLayerRefCount = 0;

  // QueuedConnection to delay execution of the slot until the signal-emitting function has exited,
  // since otherwise the undo stack becomes corrupted (featureChanged change inserted before featureAdded change)
  connect( mLayer, SIGNAL( featureAdded( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ), Qt::QueuedConnection );
  connect( mLayer.data(), SIGNAL( destroyed( QObject* ) ), this, SLOT( clearLayer() ) );

  return mLayer;
}

QgsRedliningLayer* QgsRedlining::getLayer() const
{
  return mLayer.data();
}

void QgsRedlining::editFeature( const QgsFeature& feature )
{
  QgsRedliningEditTool* tool = new QgsRedliningEditTool( mApp->mapCanvas(), getOrCreateLayer() );
  connect( this, SIGNAL( featureStyleChanged() ), tool, SLOT( onStyleChanged() ) );
  connect( tool, SIGNAL( featureSelected( QgsFeature ) ), this, SLOT( syncStyleWidgets( QgsFeature ) ) );
  connect( tool, SIGNAL( updateFeatureStyle( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ) );
  tool->selectFeature( feature );
  tool->setUnsetOnMiss( true );
  setTool( tool, mActionEditObject );
}

void QgsRedlining::editLabel( const QgsLabelPosition &labelPos )
{
  QgsRedliningEditTool* tool = new QgsRedliningEditTool( mApp->mapCanvas(), getOrCreateLayer(), new LabelEditor );
  connect( this, SIGNAL( featureStyleChanged() ), tool, SLOT( onStyleChanged() ) );
  connect( tool, SIGNAL( featureSelected( QgsFeature ) ), this, SLOT( syncStyleWidgets( QgsFeature ) ) );
  connect( tool, SIGNAL( updateFeatureStyle( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ) );
  tool->selectLabel( labelPos );
  tool->setUnsetOnMiss( true );
  setTool( tool, mActionEditObject );
}

void QgsRedlining::clearLayer()
{
  mLayer = 0;
  mLayerRefCount = 0;
  deactivateTool();
}

void QgsRedlining::editObject()
{
  QgsRedliningEditTool* tool = new QgsRedliningEditTool( mApp->mapCanvas(), getOrCreateLayer(), new LabelEditor );
  connect( this, SIGNAL( featureStyleChanged() ), tool, SLOT( onStyleChanged() ) );
  connect( tool, SIGNAL( featureSelected( QgsFeature ) ), this, SLOT( syncStyleWidgets( QgsFeature ) ) );
  connect( tool, SIGNAL( updateFeatureStyle( QgsFeatureId ) ), this, SLOT( updateFeatureStyle( QgsFeatureId ) ) );
  setTool( tool, qobject_cast<QAction*>( QObject::sender() ) );
}

void QgsRedlining::newPoint( bool active )
{
  setTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "circle" ), mActionNewPoint, active );
  mUi.buttonNewObject->setDefaultAction( mActionNewPoint );
}

void QgsRedlining::newSquare( bool active )
{
  setTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "rectangle" ), mActionNewSquare, active );
  mUi.buttonNewObject->setDefaultAction( mActionNewSquare );
}

void QgsRedlining::newTriangle( bool active )
{
  setTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "triangle" ), mActionNewTriangle, active );
  mUi.buttonNewObject->setDefaultAction( mActionNewTriangle );
}

void QgsRedlining::newLine( bool active )
{
  setTool( new QgsRedliningPolylineMapTool( mApp->mapCanvas(), getOrCreateLayer(), false ), mActionNewLine, active );
  mUi.buttonNewObject->setDefaultAction( mActionNewLine );
}

void QgsRedlining::newRectangle( bool active )
{
  setTool( new QgsRedliningRectangleMapTool( mApp->mapCanvas(), getOrCreateLayer() ), mActionNewRectangle, active );
  mUi.buttonNewObject->setDefaultAction( mActionNewRectangle );
}

void QgsRedlining::newPolygon( bool active )
{
  setTool( new QgsRedliningPolylineMapTool( mApp->mapCanvas(), getOrCreateLayer(), true ), mActionNewPolygon, active );
  mUi.buttonNewObject->setDefaultAction( mActionNewPolygon );
}

void QgsRedlining::newCircle( bool active )
{
  setTool( new QgsRedliningCircleMapTool( mApp->mapCanvas(), getOrCreateLayer() ), mActionNewCircle, active );
  mUi.buttonNewObject->setDefaultAction( mActionNewCircle );
}

void QgsRedlining::newText( bool active )
{
  setTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), getOrCreateLayer(), "", new LabelEditor ), mActionNewText, active );
  mUi.buttonNewObject->setDefaultAction( mActionNewText );
}

void QgsRedlining::setTool( QgsMapTool *tool, QAction* action , bool active )
{
  if ( active && ( mApp->mapCanvas()->mapTool() == 0 || mApp->mapCanvas()->mapTool()->action() != action ) )
  {
    tool->setAction( action );
    connect( tool, SIGNAL( deactivated() ), this, SLOT( deactivateTool() ) );
    if ( mLayerRefCount == 0 )
    {
      mApp->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
      mApp->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
      mLayer->startEditing();
    }
    ++mLayerRefCount;
    mApp->mapCanvas()->setMapTool( tool );
    mRedliningTool = tool;
  }
  else if ( !active && mApp->mapCanvas()->mapTool() && mApp->mapCanvas()->mapTool()->action() == action )
  {
    delete tool;
    mApp->mapCanvas()->unsetMapTool( mApp->mapCanvas()->mapTool() );
  }
}

void QgsRedlining::deactivateTool()
{
  if ( mRedliningTool )
  {
    if ( mLayer )
    {
      --mLayerRefCount;
      if ( mLayerRefCount == 0 )
      {
        mLayer->commitChanges();
        if ( mApp->mapCanvas()->currentLayer() == mLayer )
        {
          mApp->layerTreeView()->setCurrentLayer( 0 );
        }
      }
    }
    mRedliningTool.data()->deleteLater();
  }
}

void QgsRedlining::syncStyleWidgets( const QgsFeature& feature )
{
  mUi.spinBoxSize->blockSignals( true );
  mUi.spinBoxSize->setValue( feature.attribute( "size" ).toInt() );
  mUi.spinBoxSize->blockSignals( false );
  mUi.colorButtonOutlineColor->blockSignals( true );
  mUi.colorButtonOutlineColor->setColor( QgsSymbolLayerV2Utils::decodeColor( feature.attribute( "outline" ).toString() ) );
  mUi.colorButtonOutlineColor->blockSignals( false );
  mUi.colorButtonFillColor->blockSignals( true );
  mUi.colorButtonFillColor->setColor( QgsSymbolLayerV2Utils::decodeColor( feature.attribute( "fill" ).toString() ) );
  mUi.colorButtonFillColor->blockSignals( false );
  mUi.comboOutlineStyle->blockSignals( true );
  mUi.comboOutlineStyle->setCurrentIndex( mUi.comboOutlineStyle->findData( QgsSymbolLayerV2Utils::decodePenStyle( feature.attribute( "outline_style" ).toString() ) ) );
  mUi.comboOutlineStyle->blockSignals( false );
  mUi.comboFillStyle->blockSignals( true );
  mUi.comboFillStyle->setCurrentIndex( mUi.comboFillStyle->findData( QgsSymbolLayerV2Utils::decodeBrushStyle( feature.attribute( "fill_style" ).toString() ) ) );
  mUi.comboFillStyle->blockSignals( false );
}

void QgsRedlining::updateFeatureStyle( const QgsFeatureId &fid )
{
  if ( !mLayer )
  {
    return;
  }
  QgsFeature f;
  if ( !mLayer->getFeatures( QgsFeatureRequest( fid ) ).nextFeature( f ) )
  {
    return;
  }
  const QgsFields& fields = mLayer->pendingFields();
  mLayer->changeAttributeValue( fid, fields.indexFromName( "size" ), mUi.spinBoxSize->value() );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline" ), QgsSymbolLayerV2Utils::encodeColor( mUi.colorButtonOutlineColor->color() ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill" ), QgsSymbolLayerV2Utils::encodeColor( mUi.colorButtonFillColor->color() ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "outline_style" ), QgsSymbolLayerV2Utils::encodePenStyle( static_cast<Qt::PenStyle>( mUi.comboOutlineStyle->itemData( mUi.comboOutlineStyle->currentIndex() ).toInt() ) ) );
  mLayer->changeAttributeValue( fid, fields.indexFromName( "fill_style" ), QgsSymbolLayerV2Utils::encodeBrushStyle( static_cast<Qt::BrushStyle>( mUi.comboFillStyle->itemData( mUi.comboFillStyle->currentIndex() ).toInt() ) ) );
  mLayer->triggerRepaint();
}

void QgsRedlining::saveColor()
{
  QgsColorButtonV2* btn = qobject_cast<QgsColorButtonV2*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + btn->property( "settings_key" ).toString();
  QSettings().setValue( key, QgsSymbolLayerV2Utils::encodeColor( btn->color() ) );
  emit featureStyleChanged();
}

void QgsRedlining::saveOutlineWidth()
{
  QSettings().setValue( "/Redlining/size", mUi.spinBoxSize->value() );
  emit featureStyleChanged();
}

void QgsRedlining::saveStyle()
{
  QComboBox* combo = qobject_cast<QComboBox*>( QObject::sender() );
  QString key = QString( "/Redlining/" ) + combo->property( "settings_key" ).toString();
  QSettings().setValue( key, combo->currentIndex() );
  emit featureStyleChanged();
}

void QgsRedlining::readProject( const QDomDocument& doc )
{
  clearLayer();
  QDomNodeList nl = doc.elementsByTagName( "Redlining" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  getOrCreateLayer()->read( nl.at( 0 ).toElement() );
}

void QgsRedlining::writeProject( QDomDocument& doc )
{
  if ( !mLayer )
  {
    return;
  }

  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.count() < 1 || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement redliningElem = doc.createElement( "Redlining" );
  mLayer->write( redliningElem );
  qgisElem.appendChild( redliningElem );
}

QIcon QgsRedlining::createOutlineStyleIcon( Qt::PenStyle style )
{
  QPixmap pixmap( 16, 16 );
  pixmap.fill( Qt::transparent );
  QPainter painter( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  QPen pen;
  pen.setStyle( style );
  pen.setColor( Qt::black );
  pen.setWidth( 1 );
  painter.setPen( pen );
  painter.drawLine( 0, 8, 16, 8 );
  return pixmap;
}

QIcon QgsRedlining::createFillStyleIcon( Qt::BrushStyle style )
{
  QPixmap pixmap( 16, 16 );
  pixmap.fill( Qt::transparent );
  QPainter painter( &pixmap );
  painter.setRenderHint( QPainter::Antialiasing );
  QBrush brush;
  brush.setStyle( style );
  brush.setColor( Qt::black );
  painter.fillRect( 0, 0, 16, 16, brush );
  return pixmap;
}
