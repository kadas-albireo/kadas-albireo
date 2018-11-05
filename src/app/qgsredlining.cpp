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
#include "qgsbottombar.h"
#include "qgscolorbuttonv2.h"
#include "qgscrscache.h"
#include "qgsribbonapp.h"
#include "qgslayertreemodel.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaptooladdfeature.h"
#include "qgsmaptoolsredlining.h"
#include "qgsredlininglayer.h"
#include "qgsrubberband.h"
#include "qgssymbollayerv2utils.h"
#include "qgsproject.h"

#include <QMenu>
#include <QSettings>
#include <QShortcut>

QgsRedliningLabelEditor::QgsRedliningLabelEditor() : QgsRedliningAttribEditor( tr( "Label" ) )
{
  ui.setupUi( this );

  QFont font;
  font.fromString( QSettings().value( "/Redlining/font", font.toString() ).toString() );
  ui.toolButtonBold->setChecked( font.bold() );
  ui.toolButtonItalic->setChecked( font.italic() );
  ui.spinBoxFontSize->setValue( font.pointSize() );
  ui.spinBoxRotation->setValue( QSettings().value( "/Redlining/fontrotation", 0 ).toDouble() );
  // Set only family to make the text in the fontComboBox appear normal
  QFont fontComboFont;
  fontComboFont.setFamily( font.family() );
  ui.fontComboBox->setCurrentFont( fontComboFont );

  connect( ui.lineEditText, SIGNAL( textChanged( QString ) ), this, SIGNAL( changed() ) );
  connect( ui.fontComboBox, SIGNAL( currentFontChanged( QFont ) ), this, SIGNAL( changed() ) );
  connect( ui.toolButtonBold, SIGNAL( toggled( bool ) ), this, SIGNAL( changed() ) );
  connect( ui.toolButtonItalic, SIGNAL( toggled( bool ) ), this, SIGNAL( changed() ) );
  connect( ui.spinBoxFontSize, SIGNAL( valueChanged( int ) ), this, SIGNAL( changed() ) );
  connect( ui.spinBoxRotation, SIGNAL( valueChanged( double ) ), this, SIGNAL( changed() ) );
  connect( this, SIGNAL( changed() ), this, SLOT( saveFont() ) );
}

void QgsRedliningLabelEditor::set( const QgsAttributes &attribs, const QgsFields &fields )
{
  blockSignals( true );
  QString text = attribs[fields.fieldNameIndex( "text" )].toString();
  QMap<QString, QString> flagsMap = QgsRedliningLayer::deserializeFlags( attribs[fields.fieldNameIndex( "flags" )].toString() );

  QFont font;
  font.fromString( QSettings().value( "/Redlining/font", font.toString() ).toString() );
  QString rotation = QSettings().value( "/Redlining/fontrotation", 0 ).toString();
  ui.lineEditText->setText( text );
  ui.spinBoxFontSize->setValue( flagsMap.value( "fontSize", QString( "%1" ).arg( font.pointSize() ) ).toInt() );
  ui.toolButtonItalic->setChecked( flagsMap.value( "italic", QString( "%1" ).arg( font.italic() ) ).toInt() );
  ui.toolButtonBold->setChecked( flagsMap.value( "bold", QString( "%1" ).arg( font.bold() ) ).toInt() );
  ui.spinBoxRotation->setValue( flagsMap.value( "rotation", rotation ).toDouble() );
  // Set only family to make the text in the fontComboBox appear normal
  font.setFamily( flagsMap.value( "family", font.family() ) );
  ui.fontComboBox->setCurrentFont( font );
  blockSignals( false );

  saveFont();
}

void QgsRedliningLabelEditor::get( QgsAttributes &attribs, const QgsFields &fields ) const
{
  QMap<QString, QString> flagsMap = QgsRedliningLayer::deserializeFlags( attribs[fields.fieldNameIndex( "flags" )].toString() );
  flagsMap["family"] = ui.fontComboBox->currentFont().family();
  flagsMap["italic"] = QString( "%1" ).arg( ui.toolButtonItalic->isChecked() );
  flagsMap["bold"] = QString( "%1" ).arg( ui.toolButtonBold->isChecked() );
  flagsMap["rotation"] = QString( "%1" ).arg( ui.spinBoxRotation->value() );
  flagsMap["fontSize"] = QString( "%1" ).arg( ui.spinBoxFontSize->value() );

  attribs[fields.fieldNameIndex( "text" )] = ui.lineEditText->text();
  attribs[fields.fieldNameIndex( "flags" )] = QgsRedliningLayer::serializeFlags( flagsMap );
}

void QgsRedliningLabelEditor::saveFont()
{
  QFont font;
  font.setFamily( ui.fontComboBox->currentFont().family() );
  font.setItalic( ui.toolButtonItalic->isChecked() );
  font.setBold( ui.toolButtonBold->isChecked() );
  font.setPointSize( ui.spinBoxFontSize->value() );
  QSettings().setValue( "/Redlining/font", font.toString() );
  QSettings().setValue( "/Redlining/fontrotation", ui.spinBoxRotation->value() );
}


QgsRedlining::QgsRedlining( QgisApp *app, const RedliningUi& ui )
    : QgsRedliningManager( app )
    , mApp( app )
    , mUi( ui )
{
  QAction* actionNewMarker = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Marker" ), this );

  mActionNewPoint = new QAction( QIcon( ":/images/themes/default/redlining_point.svg" ), tr( "Point" ), this );
  mActionNewPoint->setCheckable( true );
  connect( mActionNewPoint, SIGNAL( triggered( bool ) ), this, SLOT( setPointTool( bool ) ) );

  mActionNewSquare = new QAction( QIcon( ":/images/themes/default/redlining_square.svg" ), tr( "Square" ), this );
  mActionNewSquare->setCheckable( true );
  connect( mActionNewSquare, SIGNAL( triggered( bool ) ), this, SLOT( setSquareTool( bool ) ) );

  mActionNewTriangle = new QAction( QIcon( ":/images/themes/default/redlining_triangle.svg" ), tr( "Triangle" ), this );
  mActionNewTriangle->setCheckable( true );
  connect( mActionNewTriangle, SIGNAL( triggered( bool ) ), this, SLOT( setTriangleTool( bool ) ) );

  mActionNewLine = new QAction( QIcon( ":/images/themes/default/redlining_line.svg" ), tr( "Line" ), this );
  mActionNewLine->setCheckable( true );
  connect( mActionNewLine, SIGNAL( triggered( bool ) ), this, SLOT( setLineTool( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_L ), app ), SIGNAL( activated() ), mActionNewLine, SLOT( trigger() ) );
  mActionNewRectangle = new QAction( QIcon( ":/images/themes/default/redlining_rectangle.svg" ), tr( "Rectangle" ), this );
  mActionNewRectangle->setCheckable( true );
  connect( mActionNewRectangle, SIGNAL( triggered( bool ) ), this, SLOT( setRectangleTool( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_R ), app ), SIGNAL( activated() ), mActionNewRectangle, SLOT( trigger() ) );
  mActionNewPolygon = new QAction( QIcon( ":/images/themes/default/redlining_polygon.svg" ), tr( "Polygon" ), this );
  mActionNewPolygon->setCheckable( true );
  connect( mActionNewPolygon, SIGNAL( triggered( bool ) ), this, SLOT( setPolygonTool( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_P ), app ), SIGNAL( activated() ), mActionNewPolygon, SLOT( trigger() ) );
  mActionNewCircle = new QAction( QIcon( ":/images/themes/default/redlining_circle.svg" ), tr( "Circle" ), this );
  mActionNewCircle->setCheckable( true );
  connect( mActionNewCircle, SIGNAL( triggered( bool ) ), this, SLOT( setCircleTool( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_C ), app ), SIGNAL( activated() ), mActionNewCircle, SLOT( trigger() ) );
  mActionNewText = new QAction( QIcon( ":/images/themes/default/redlining_text.svg" ), tr( "Text" ), this );
  mActionNewText->setCheckable( true );
  connect( mActionNewText, SIGNAL( triggered( bool ) ), this, SLOT( setTextTool( bool ) ) );
  connect( new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_D, Qt::CTRL + Qt::Key_T ), app ), SIGNAL( activated() ), mActionNewText, SLOT( trigger() ) );

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

  mUi.spinBoxSize->setRange( 1, 100 );
  mUi.spinBoxSize->setValue( QSettings().value( "/Redlining/size", 1 ).toInt() );
  connect( mUi.spinBoxSize, SIGNAL( valueChanged( int ) ), this, SLOT( saveOutlineWidth() ) );

  mUi.colorButtonOutlineColor->setAllowAlpha( true );
  mUi.colorButtonOutlineColor->setProperty( "settings_key", "outline_color" );
  QColor initialOutlineColor = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/Redlining/outline_color", "0,0,0,255" ).toString() );
  mUi.colorButtonOutlineColor->setColor( initialOutlineColor );
  connect( mUi.colorButtonOutlineColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );

  mUi.comboOutlineStyle->setProperty( "settings_key", "outline_style" );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::NoPen ), QString(), static_cast<int>( Qt::NoPen ) );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::SolidLine ), QString(), static_cast<int>( Qt::SolidLine ) );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashLine ), QString(), static_cast<int>( Qt::DashLine ) );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DashDotLine ), QString(), static_cast<int>( Qt::DashDotLine ) );
  mUi.comboOutlineStyle->addItem( createOutlineStyleIcon( Qt::DotLine ), QString(), static_cast<int>( Qt::DotLine ) );
  mUi.comboOutlineStyle->setCurrentIndex( QSettings().value( "/Redlining/outline_style", "1" ).toInt() );
  connect( mUi.comboOutlineStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveStyle() ) );

  mUi.colorButtonFillColor->setAllowAlpha( true );
  mUi.colorButtonFillColor->setProperty( "settings_key", "fill_color" );
  QColor initialFillColor = QgsSymbolLayerV2Utils::decodeColor( QSettings().value( "/Redlining/fill_color", "255,0,0,255" ).toString() );
  mUi.colorButtonFillColor->setColor( initialFillColor );
  connect( mUi.colorButtonFillColor, SIGNAL( colorChanged( QColor ) ), this, SLOT( saveColor() ) );

  mUi.comboFillStyle->setProperty( "settings_key", "fill_style" );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::NoBrush ), QString(), static_cast<int>( Qt::NoBrush ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::SolidPattern ), QString(), static_cast<int>( Qt::SolidPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::HorPattern ), QString(), static_cast<int>( Qt::HorPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::VerPattern ), QString(), static_cast<int>( Qt::VerPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::BDiagPattern ), QString(), static_cast<int>( Qt::BDiagPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::DiagCrossPattern ), QString(), static_cast<int>( Qt::DiagCrossPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::FDiagPattern ), QString(), static_cast<int>( Qt::FDiagPattern ) );
  mUi.comboFillStyle->addItem( createFillStyleIcon( Qt::CrossPattern ), QString(), static_cast<int>( Qt::CrossPattern ) );
  mUi.comboFillStyle->setCurrentIndex( QSettings().value( "/Redlining/fill_style", "1" ).toInt() );
  connect( mUi.comboFillStyle, SIGNAL( currentIndexChanged( int ) ), this, SLOT( saveStyle() ) );

  connect( QgsProject::instance(), SIGNAL( readProject( QDomDocument ) ), this, SLOT( readProject( QDomDocument ) ) );
  connect( QgsProject::instance(), SIGNAL( writeProject( QDomDocument& ) ), this, SLOT( writeProject( QDomDocument& ) ) );
  connect( QgsMapLayerRegistry::instance(), SIGNAL( layerWillBeRemoved( QString ) ), this, SLOT( checkLayerRemoved( QString ) ) );

  connect( this, SIGNAL( featureStyleChanged() ), this, SLOT( updateToolRubberbandStyle() ) );
}

QgsRedliningLayer* QgsRedlining::getOrCreateLayer()
{
  return QgsMapLayerRegistry::instance()->getOrCreateRedliningLayer();
}

QgsRedliningLayer* QgsRedlining::getLayer() const
{
  return QgsMapLayerRegistry::instance()->getRedliningLayer();
}

void QgsRedlining::editFeature( const QgsFeature& feature )
{
  syncStyleWidgets( feature );
  QString flags = feature.attribute( "flags" ).toString();
  QRegExp shapeRe( "\\bshape=(\\w+)\\b" );
  shapeRe.indexIn( flags );
  QString shape = shapeRe.cap( 1 );
  if ( shape.isEmpty() )
  {
    // Deduce shape from geometry type
    switch ( feature.geometry()->type() )
    {
      case QGis::Point: shape = "point"; break;
      case QGis::Line: shape = "line"; break;
      case QGis::Polygon: shape = "polygon"; break;
      default: break;
    }
  }
  if ( shape == "point" )
  {
    QMap<QString, QAction*> actionMap;
    actionMap.insert( "circle", mActionNewPoint );
    actionMap.insert( "rectangle", mActionNewSquare );
    actionMap.insert( "triangle", mActionNewTriangle );

    QRegExp symbolRe( "\\bsymbol=(\\w+)\\b" );
    symbolRe.indexIn( flags );
    QString symbol = symbolRe.cap( 1 );
    if ( !symbol.isEmpty() )
    {
      setMarkerTool( symbol, true, &feature, actionMap.value( symbol, 0 ) );
    }
    else
    {
      return;
    }
  }
  else if ( shape == "line" )
  {
    setLineTool( true, &feature );
  }
  else if ( shape == "polygon" )
  {
    setPolygonTool( true, &feature );
  }
  else if ( shape == "rectangle" )
  {
    setRectangleTool( true, &feature );
  }
  else if ( shape == "circle" )
  {
    setCircleTool( true, &feature );
  }
}

void QgsRedlining::editLabel( const QgsLabelPosition &labelPos )
{
  QgsFeature feature;
  getOrCreateLayer()->getFeatures( QgsFeatureRequest( labelPos.featureId ) ).nextFeature( feature );
  syncStyleWidgets( feature );
  setTool( new QgsRedliningEditTextMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), labelPos, new QgsRedliningLabelEditor ), mActionNewText );
  mUi.spinBoxSize->setEnabled( false );
  mUi.comboFillStyle->setEnabled( false );
  mUi.colorButtonOutlineColor->setEnabled( false );
  mUi.comboOutlineStyle->setEnabled( false );
}

void QgsRedlining::editFeatures( const QList<QgsFeature> &features )
{
  QgsRedliningLayer* layer = getOrCreateLayer();
  QList<QgsLabelPosition> labels;
  QgsFeatureList shapes;

  // Try to find labels for labelfeatures
  const QgsLabelingResults* labelingResults = mApp->mapCanvas()->labelingResults();
  if ( labelingResults )
  {
    const QgsCoordinateTransform* ct = QgsCoordinateTransformCache::instance()->transform( layer->crs().authid(), mApp->mapCanvas()->mapSettings().destinationCrs().authid() );
    foreach ( const QgsFeature& feature, features )
    {
      // Hacky way to detect a label...
      if ( feature.attribute( "text" ).isNull() )
      {
        shapes.append( feature );
        continue;
      }
      QgsPointV2* layerPos = static_cast<QgsPointV2*>( feature.geometry()->geometry() );
      QgsPoint mapPos = ct->transform( layerPos->x(), layerPos->y() );
      foreach ( const QgsLabelPosition& labelPos, labelingResults->labelsAtPosition( mapPos ) )
      {
        if ( labelPos.layerID == layer->id() && labelPos.featureId == feature.id() )
        {
          labels.append( labelPos );
          break;
        }
      }
    }
  }
  int tot = shapes.size() + labels.size();
  if ( tot == 1 )
  {
    if ( !shapes.isEmpty() )
    {
      editFeature( shapes.first() );
    }
    else if ( !labels.isEmpty() )
    {
      editLabel( labels.first() );
    }
  }
  else if ( tot > 1 )
  {
    mApp->mapCanvas()->setMapTool( new QgsRedliningEditGroupMapTool( mApp->mapCanvas(), this, layer, shapes, labels ) );
  }
}

void QgsRedlining::checkLayerRemoved( const QString &layerId )
{
  QgsRedliningLayer* layer = getLayer();
  if ( layer && layer->id() == layerId )
  {
    deactivateTool();
  }
}

void QgsRedlining::setMarkerTool( const QString &shape, bool active, const QgsFeature *editFeature, QAction *action )
{
  setTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), shape, editFeature ), action, active );
}

void QgsRedlining::setLineTool( bool active, const QgsFeature *editFeature )
{
  setTool( new QgsRedliningPolylineMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), false, editFeature ), mActionNewLine, active );
  mUi.colorButtonFillColor->setEnabled( false );
  mUi.comboFillStyle->setEnabled( false );
}

void QgsRedlining::setRectangleTool( bool active, const QgsFeature *editFeature )
{
  setTool( new QgsRedliningRectangleMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), editFeature ), mActionNewRectangle, active );
}

void QgsRedlining::setPolygonTool( bool active, const QgsFeature *editFeature )
{
  setTool( new QgsRedliningPolylineMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), true, editFeature ), mActionNewPolygon, active );
}

void QgsRedlining::setCircleTool( bool active, const QgsFeature *editFeature )
{
  setTool( new QgsRedliningCircleMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), editFeature ), mActionNewCircle, active );
}

void QgsRedlining::setTextTool( bool active )
{
  setTool( new QgsRedliningPointMapTool( mApp->mapCanvas(), this, getOrCreateLayer(), "", 0, new QgsRedliningLabelEditor ), mActionNewText, active );
  mUi.spinBoxSize->setEnabled( false );
  mUi.comboFillStyle->setEnabled( false );
  mUi.colorButtonOutlineColor->setEnabled( false );
  mUi.comboOutlineStyle->setEnabled( false );
}

void QgsRedlining::setTool( QgsMapTool *tool, QAction* action , bool active )
{
  if ( active )
  {
    tool->setAction( action );
    mUi.buttonNewObject->setDefaultAction( action );
    connect( tool, SIGNAL( deactivated() ), this, SLOT( deactivateTool() ) );
    mApp->layerTreeView()->setCurrentLayer( getOrCreateLayer() );
    mApp->layerTreeView()->setLayerVisible( getOrCreateLayer(), true );
    mApp->mapCanvas()->setMapTool( tool );
    mRedliningTool = tool;
    updateToolRubberbandStyle();
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
    mRedliningTool.data()->deleteLater();
  }
  mUi.spinBoxSize->setEnabled( true );
  mUi.colorButtonFillColor->setEnabled( true );
  mUi.comboFillStyle->setEnabled( true );
  mUi.colorButtonOutlineColor->setEnabled( true );
  mUi.comboOutlineStyle->setEnabled( true );
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
  mUi.comboOutlineStyle->setCurrentIndex( mUi.comboOutlineStyle->findData( static_cast<int>( QgsSymbolLayerV2Utils::decodePenStyle( feature.attribute( "outline_style" ).toString() ) ) ) );
  mUi.comboOutlineStyle->blockSignals( false );
  mUi.comboFillStyle->blockSignals( true );
  mUi.comboFillStyle->setCurrentIndex( mUi.comboFillStyle->findData( static_cast<int>( QgsSymbolLayerV2Utils::decodeBrushStyle( feature.attribute( "fill_style" ).toString() ) ) ) );
  mUi.comboFillStyle->blockSignals( false );
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

void QgsRedlining::updateToolRubberbandStyle()
{
  int outlineWidth = mUi.spinBoxSize->value();
  QColor outlineColor = mUi.colorButtonOutlineColor->color();
  QColor fillColor = mUi.colorButtonFillColor->color();
  Qt::PenStyle lineStyle = static_cast<Qt::PenStyle>( mUi.comboOutlineStyle->itemData( mUi.comboOutlineStyle->currentIndex() ).toInt() );
  Qt::BrushStyle brushStyle = static_cast<Qt::BrushStyle>( mUi.comboFillStyle->itemData( mUi.comboFillStyle->currentIndex() ).toInt() );

  if ( dynamic_cast<QgsMapToolDrawShape*>( mRedliningTool.data() ) )
  {
    static_cast<QgsMapToolDrawShape*>( mRedliningTool.data() )->updateStyle( outlineWidth, outlineColor, fillColor, lineStyle, brushStyle );
  }
  else if ( dynamic_cast<QgsRedliningEditTextMapTool*>( mRedliningTool.data() ) )
  {
    static_cast<QgsRedliningEditTextMapTool*>( mRedliningTool.data() )->updateStyle( outlineWidth, outlineColor, fillColor, lineStyle, brushStyle );
  }
}

void QgsRedlining::readProject( const QDomDocument& doc )
{
  QDomNodeList nl = doc.elementsByTagName( "Redlining" );
  if ( nl.isEmpty() || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QgsMapLayerRegistry::instance()->setRedliningLayer( nl.at( 0 ).toElement().attribute( "layerid" ) );
}

void QgsRedlining::writeProject( QDomDocument& doc )
{
  QgsRedliningLayer* layer = getLayer();
  if ( !layer )
  {
    return;
  }

  QDomNodeList nl = doc.elementsByTagName( "qgis" );
  if ( nl.isEmpty() || nl.at( 0 ).toElement().isNull() )
  {
    return;
  }
  QDomElement qgisElem = nl.at( 0 ).toElement();

  QDomElement redliningElem = doc.createElement( "Redlining" );
  redliningElem.setAttribute( "layerid", layer->id() );
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
  pen.setColor( QColor( 232, 244, 255 ) );
  pen.setWidth( 2 );
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
  brush.setColor( QColor( 232, 244, 255 ) );
  painter.fillRect( 0, 0, 16, 16, brush );
  return pixmap;
}
