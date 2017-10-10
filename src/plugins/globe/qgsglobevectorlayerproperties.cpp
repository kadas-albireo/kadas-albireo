/***************************************************************************
    qgsglobevectorlayerpropertiespage.cpp
     --------------------------------------
    Date                 : 9.7.2013
    Copyright            : (C) 2013 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qgsproject.h>
#include <qgsvectorlayer.h>
#include "qgsglobevectorlayerproperties.h"

#include <osgEarth/Version>
#include <QStandardItemModel>

Q_DECLARE_METATYPE( QgsGlobeVectorLayerConfig* )

QgsGlobeVectorLayerConfig* QgsGlobeVectorLayerConfig::getConfig( QgsVectorLayer* layer )
{
  QgsGlobeVectorLayerConfig* layerConfig = layer->property( "globe-config" ).value<QgsGlobeVectorLayerConfig*>();
  if ( !layerConfig )
  {
    layerConfig = new QgsGlobeVectorLayerConfig( layer );
    layer->setProperty( "globe-config", QVariant::fromValue<QgsGlobeVectorLayerConfig*>( layerConfig ) );
  }
  return layerConfig;
}

///////////////////////////////////////////////////////////////////////////////

QgsGlobeVectorLayerPropertiesPage::QgsGlobeVectorLayerPropertiesPage( QgsVectorLayer* layer, QWidget *parent )
    : QgsVectorLayerPropertiesPage( parent )
    , mLayer( layer )
{
  setupUi( this );

  // Populate combo boxes
  comboBoxRenderingMode->addItem( tr( "Rasterized" ), QgsGlobeVectorLayerConfig::RenderingModeRasterized );
  comboBoxRenderingMode->addItem( tr( "Model 2.5D" ), QgsGlobeVectorLayerConfig::RenderingModeModel25D );
  comboBoxRenderingMode->addItem( tr( "Model 3D" ), QgsGlobeVectorLayerConfig::RenderingModeModel3D );
  comboBoxRenderingMode->addItem( tr( "Model (Advanced)" ), QgsGlobeVectorLayerConfig::RenderingModeModelAdvanced );
  comboBoxRenderingMode->setItemData( 0, tr( "Rasterize the layer to a texture, and drape it on the terrain" ), Qt::ToolTipRole );
  comboBoxRenderingMode->setItemData( 1, tr( "Render features as 2D models with optional extrusion height" ), Qt::ToolTipRole );
  comboBoxRenderingMode->setItemData( 2, tr( "Render features as full 3D models" ), Qt::ToolTipRole );
  comboBoxRenderingMode->setItemData( 3, tr( "Render features as models, advanced mode" ), Qt::ToolTipRole );
  comboBoxRenderingMode->setCurrentIndex( -1 );

  comboBoxAltitudeClamping->addItem( tr( "At terrain height" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN ) );
  comboBoxAltitudeClamping->addItem( tr( "Relative to terrain" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_RELATIVE_TO_TERRAIN ) );
  comboBoxAltitudeClamping->addItem( tr( "Relative to MSL" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_ABSOLUTE ) );
  comboBoxAltitudeClamping->setItemData( 0, tr( "Ignore original feature height and project all feature vertices to terrain" ), Qt::ToolTipRole );
  comboBoxAltitudeClamping->setItemData( 1, tr( "Interpret the height of the feature as relative to the terrain" ), Qt::ToolTipRole );
  comboBoxAltitudeClamping->setItemData( 2, tr( "Interpret the height of the feature as relative to the MSL" ), Qt::ToolTipRole );
  comboBoxAltitudeClamping->setCurrentIndex( -1 );

  comboBoxAltitudeBinding->addItem( tr( "Vertex" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::BINDING_VERTEX ) );
  comboBoxAltitudeBinding->addItem( tr( "Centroid" ), static_cast<int>( osgEarth::Symbology::AltitudeSymbol::BINDING_CENTROID ) );
  comboBoxAltitudeBinding->setItemData( 0, tr( "Clamp every vertex independently" ), Qt::ToolTipRole );
  comboBoxAltitudeBinding->setItemData( 1, tr( "Clamp to the centroid of the entire geometry" ), Qt::ToolTipRole );
  comboBoxAltitudeBinding->setCurrentIndex( -1 );

  // Connect signals (setCurrentIndex(-1) above ensures the signal is called when the current values are set below)
  connect( comboBoxRenderingMode, SIGNAL( currentIndexChanged( int ) ), this, SLOT( showRenderingModeWidget( int ) ) );
  connect( comboBoxAltitudeClamping, SIGNAL( currentIndexChanged( int ) ), this, SLOT( onAltitudeClampingChanged( int ) ) );

  // Set values
  QgsGlobeVectorLayerConfig* layerConfig = QgsGlobeVectorLayerConfig::getConfig( mLayer );

  comboBoxRenderingMode->setCurrentIndex( comboBoxRenderingMode->findData( static_cast<int>( layerConfig->renderingMode ) ) );

  comboBoxAltitudeClamping->setCurrentIndex( comboBoxAltitudeClamping->findData( static_cast<int>( layerConfig->altitudeClamping ) ) );
  comboBoxAltitudeBinding->setCurrentIndex( comboBoxAltitudeBinding->findData( static_cast<int>( layerConfig->altitudeBinding ) ) );

  spinBoxAltitudeOffset->setValue( layerConfig->verticalOffset );
  spinBoxAltitudeScale->setValue( layerConfig->verticalScale );
  spinBoxAltitudeResolution->setValue( layerConfig->clampingResolution );

  groupBoxExtrusion->setChecked( layerConfig->extrusionEnabled );
  labelExtrusionHeight->setText( layerConfig->extrusionHeight );
  checkBoxExtrusionFlatten->setChecked( layerConfig->extrusionFlatten );
  spinBoxExtrusionWallGradient->setValue( layerConfig->extrusionWallGradient );

#if OSGEARTH_VERSION_LESS_THAN(2, 7, 0)
  groupBoxLabelingEnabled->setChecked( layerConfig->labelingEnabled );
  checkBoxLabelingDeclutter->setChecked( layerConfig->labelingDeclutter );
#else
#pragma message("TODO: labeling broken with osgEarth 2.7")
  groupBoxLabelingEnabled->setChecked( false );
  checkBoxLabelingDeclutter->setChecked( false );
  groupBoxLabelingEnabled->setVisible( false );
  checkBoxLabelingDeclutter->setVisible( false );
#endif

  checkBoxLighting->setChecked( layerConfig->lightingEnabled );
}

void QgsGlobeVectorLayerPropertiesPage::apply()
{
  QgsGlobeVectorLayerConfig* layerConfig = QgsGlobeVectorLayerConfig::getConfig( mLayer );

  layerConfig->renderingMode = static_cast<QgsGlobeVectorLayerConfig::RenderingMode>( comboBoxRenderingMode->itemData( comboBoxRenderingMode->currentIndex() ).toInt() );
  layerConfig->altitudeClamping = static_cast<osgEarth::Symbology::AltitudeSymbol::Clamping>( comboBoxAltitudeClamping->itemData( comboBoxAltitudeClamping->currentIndex() ).toInt() );
  layerConfig->altitudeBinding = static_cast<osgEarth::Symbology::AltitudeSymbol::Binding>( comboBoxAltitudeBinding->itemData( comboBoxAltitudeBinding->currentIndex() ).toInt() );

  layerConfig->verticalOffset = spinBoxAltitudeOffset->value();
  layerConfig->verticalScale = spinBoxAltitudeScale->value();
  layerConfig->clampingResolution = spinBoxAltitudeResolution->value();

  layerConfig->extrusionEnabled = groupBoxExtrusion->isChecked();
  layerConfig->extrusionHeight = labelExtrusionHeight->text();
  layerConfig->extrusionFlatten = checkBoxExtrusionFlatten->isChecked();
  layerConfig->extrusionWallGradient = spinBoxExtrusionWallGradient->value();

  layerConfig->labelingEnabled = groupBoxLabelingEnabled->isChecked();
  layerConfig->labelingDeclutter = checkBoxLabelingDeclutter->isChecked();

  layerConfig->lightingEnabled = checkBoxLighting->isChecked();

  emit layerSettingsChanged( mLayer );
}

void QgsGlobeVectorLayerPropertiesPage::onAltitudeClampingChanged( int index )
{
  QgsGlobeVectorLayerConfig::RenderingMode mode = static_cast<QgsGlobeVectorLayerConfig::RenderingMode>( comboBoxRenderingMode->currentData().toInt() );
  if ( mode == QgsGlobeVectorLayerConfig::RenderingModeModelAdvanced )
  {
    osgEarth::Symbology::AltitudeSymbol::Clamping clamping = static_cast<osgEarth::Symbology::AltitudeSymbol::Clamping>( comboBoxAltitudeClamping->itemData( index ).toInt() );

    bool terrainClamping = clamping == osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN || clamping == osgEarth::Symbology::AltitudeSymbol::CLAMP_RELATIVE_TO_TERRAIN;
    labelAltitudeBinding->setVisible( terrainClamping );
    comboBoxAltitudeBinding->setVisible( terrainClamping );
    labelAltitudeResolution->setVisible( terrainClamping );
    spinBoxAltitudeResolution->setVisible( terrainClamping );
  }
}

void QgsGlobeVectorLayerPropertiesPage::showRenderingModeWidget( int index )
{
  stackedWidgetRenderingMode->setCurrentIndex( index != 0 );
  QgsGlobeVectorLayerConfig::RenderingMode mode = static_cast<QgsGlobeVectorLayerConfig::RenderingMode>( comboBoxRenderingMode->currentData().toInt() );
  QStandardItem* clampTerrainItem = qobject_cast<QStandardItemModel*>( comboBoxAltitudeClamping->model() )->item( 0 );
  if ( mode == QgsGlobeVectorLayerConfig::RenderingModeModel25D )
  {
    comboBoxAltitudeClamping->setCurrentIndex( comboBoxAltitudeClamping->findData( static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_TO_TERRAIN ) ) );
    comboBoxAltitudeBinding->setCurrentIndex( comboBoxAltitudeBinding->findData( osgEarth::Symbology::AltitudeSymbol::BINDING_VERTEX ) );
    spinBoxAltitudeResolution->setValue( 0 );
    spinBoxAltitudeOffset->setValue( 0.001 ); // To avoid Z finding when no extrusion is enabled
    spinBoxAltitudeScale->setValue( 1. );
    groupBoxAltitude->setVisible( false );

    groupBoxExtrusion->setVisible( true );
    checkBoxExtrusionFlatten->setChecked( true );
    checkBoxExtrusionFlatten->setVisible( false );

    checkBoxLighting->setChecked( true );
    checkBoxLighting->setVisible( false );
  }
  else if ( mode == QgsGlobeVectorLayerConfig::RenderingModeModel3D )
  {
    clampTerrainItem->setFlags( clampTerrainItem->flags() & ~( Qt::ItemIsEnabled | Qt::ItemIsSelectable ) );
    comboBoxAltitudeClamping->setCurrentIndex( comboBoxAltitudeClamping->findData( static_cast<int>( osgEarth::Symbology::AltitudeSymbol::CLAMP_ABSOLUTE ) ) );
    comboBoxAltitudeBinding->setCurrentIndex( comboBoxAltitudeBinding->findData( osgEarth::Symbology::AltitudeSymbol::BINDING_CENTROID ) );
    comboBoxAltitudeBinding->setVisible( false );
    labelAltitudeBinding->setVisible( false );
    spinBoxAltitudeResolution->setValue( 0 );
    spinBoxAltitudeResolution->setVisible( false );
    labelAltitudeResolution->setVisible( false );
    groupBoxAltitude->setVisible( true );

    groupBoxExtrusion->setChecked( false );
    groupBoxExtrusion->setVisible( false );

    checkBoxLighting->setChecked( true );
    checkBoxLighting->setVisible( false );
  }
  else if ( mode == QgsGlobeVectorLayerConfig::RenderingModeModelAdvanced )
  {
    clampTerrainItem->setFlags( clampTerrainItem->flags() | ( Qt::ItemIsEnabled | Qt::ItemIsSelectable ) );
    comboBoxAltitudeBinding->setVisible( true );
    labelAltitudeBinding->setVisible( true );
    spinBoxAltitudeResolution->setVisible( true );
    labelAltitudeResolution->setVisible( true );
    spinBoxAltitudeOffset->setVisible( true );
    spinBoxAltitudeScale->setVisible( true );
    groupBoxExtrusion->setVisible( true );
    checkBoxLighting->setVisible( true );
  }
}

///////////////////////////////////////////////////////////////////////////////

QgsGlobeLayerPropertiesFactory::QgsGlobeLayerPropertiesFactory( QObject *parent )
    : QObject( parent )
{
  connect( QgsProject::instance(), SIGNAL( readMapLayer( QgsMapLayer*, QDomElement ) ), this, SLOT( readGlobeVectorLayerConfig( QgsMapLayer*, QDomElement ) ) );
  connect( QgsProject::instance(), SIGNAL( writeMapLayer( QgsMapLayer*, QDomElement&, QDomDocument& ) ), this, SLOT( writeGlobeVectorLayerConfig( QgsMapLayer*, QDomElement&, QDomDocument& ) ) );
}

QgsVectorLayerPropertiesPage* QgsGlobeLayerPropertiesFactory::createVectorLayerPropertiesPage( QgsVectorLayer* layer, QWidget* parent )
{
  QgsGlobeVectorLayerPropertiesPage* propsPage = new QgsGlobeVectorLayerPropertiesPage( layer, parent );
  connect( propsPage, SIGNAL( layerSettingsChanged( QgsMapLayer* ) ), this, SIGNAL( layerSettingsChanged( QgsMapLayer* ) ) );
  return propsPage;
}

QListWidgetItem* QgsGlobeLayerPropertiesFactory::createVectorLayerPropertiesItem( QgsVectorLayer* layer, QListWidget* view )
{
  Q_UNUSED( layer );
  return new QListWidgetItem( QIcon( ":/globe/icon.svg" ), tr( "Globe" ), view );
}

void QgsGlobeLayerPropertiesFactory::readGlobeVectorLayerConfig( QgsMapLayer* mapLayer, const QDomElement& elem )
{
  if ( dynamic_cast<QgsVectorLayer*>( mapLayer ) )
  {
    QgsVectorLayer* vLayer = static_cast<QgsVectorLayer*>( mapLayer );
    QgsGlobeVectorLayerConfig* config = QgsGlobeVectorLayerConfig::getConfig( vLayer );

    QDomElement globeElem = elem.firstChildElement( "globe" );
    if ( !globeElem.isNull() )
    {
      QDomElement renderingModeElem = globeElem.firstChildElement( "renderingMode" );
      config->renderingMode = static_cast<QgsGlobeVectorLayerConfig::RenderingMode>( renderingModeElem.attribute( "mode" ).toInt() );

      QDomElement modelRenderingElem = globeElem.firstChildElement( "modelRendering" );
      if ( !modelRenderingElem.isNull() )
      {
        QDomElement altitudeElem = modelRenderingElem.firstChildElement( "altitude" );
        config->altitudeClamping = static_cast<osgEarth::Symbology::AltitudeSymbol::Clamping>( altitudeElem.attribute( "clamping" ).toInt() );
        config->altitudeBinding = static_cast<osgEarth::Symbology::AltitudeSymbol::Binding>( altitudeElem.attribute( "binding" ).toInt() );
        config->verticalOffset = altitudeElem.attribute( "verticalOffset" ).toFloat();
        config->verticalScale = altitudeElem.attribute( "verticalScale" ).toFloat();
        config->clampingResolution = altitudeElem.attribute( "clampingResolution" ).toFloat();

        QDomElement extrusionElem = modelRenderingElem.firstChildElement( "extrusion" );
        config->extrusionEnabled = extrusionElem.attribute( "enabled" ).toInt() == 1;
        config->extrusionHeight = extrusionElem.attribute( "height", QString( "10" ) ).trimmed();
        if ( config->extrusionHeight.isEmpty() )
          config->extrusionHeight = "10";
        config->extrusionFlatten = extrusionElem.attribute( "flatten" ).toInt() == 1;
        config->extrusionWallGradient = extrusionElem.attribute( "wall-gradient" ).toDouble();

        QDomElement labelingElem = modelRenderingElem.firstChildElement( "labeling" );
        config->labelingEnabled = labelingElem.attribute( "enabled", "0" ).toInt() == 1;
        config->labelingField = labelingElem.attribute( "field" );
        config->labelingDeclutter = labelingElem.attribute( "declutter", "1" ).toInt() == 1;

        config->lightingEnabled = modelRenderingElem.attribute( "lighting", "1" ).toInt() == 1;
      }
    }
  }
}

void QgsGlobeLayerPropertiesFactory::writeGlobeVectorLayerConfig( QgsMapLayer* mapLayer, QDomElement& elem, QDomDocument& doc )
{
  if ( dynamic_cast<QgsVectorLayer*>( mapLayer ) )
  {
    QgsVectorLayer* vLayer = static_cast<QgsVectorLayer*>( mapLayer );
    QgsGlobeVectorLayerConfig* config = QgsGlobeVectorLayerConfig::getConfig( vLayer );

    QDomElement globeElem = doc.createElement( "globe" );

    QDomElement renderingModeElem = doc.createElement( "renderingMode" );
    renderingModeElem.setAttribute( "mode", config->renderingMode );
    globeElem.appendChild( renderingModeElem );

    QDomElement modelRenderingElem = doc.createElement( "modelRendering" );

    QDomElement altitudeElem = doc.createElement( "altitude" );
    altitudeElem.setAttribute( "clamping", config->altitudeClamping );
    altitudeElem.setAttribute( "binding", config->altitudeBinding );
    altitudeElem.setAttribute( "verticalOffset", config->verticalOffset );
    altitudeElem.setAttribute( "verticalScale", config->verticalScale );
    altitudeElem.setAttribute( "clampingResolution", config->clampingResolution );
    modelRenderingElem.appendChild( altitudeElem );

    QDomElement extrusionElem = doc.createElement( "extrusion" );
    extrusionElem.setAttribute( "enabled", config->extrusionEnabled );
    extrusionElem.setAttribute( "height", config->extrusionHeight );
    extrusionElem.setAttribute( "flatten", config->extrusionFlatten );
    extrusionElem.setAttribute( "wall-gradient", config->extrusionWallGradient );
    modelRenderingElem.appendChild( extrusionElem );

    QDomElement labelingElem = doc.createElement( "labeling" );
    labelingElem.setAttribute( "enabled", config->labelingEnabled );
    labelingElem.setAttribute( "field", config->labelingField );
    labelingElem.setAttribute( "declutter", config->labelingDeclutter );
    modelRenderingElem.appendChild( labelingElem );

    modelRenderingElem.setAttribute( "lighting", config->lightingEnabled );

    globeElem.appendChild( modelRenderingElem );

    elem.appendChild( globeElem );
  }
}
