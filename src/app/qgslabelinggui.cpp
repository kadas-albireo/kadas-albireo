/***************************************************************************
  qgslabelinggui.cpp
  Smart labeling for vector layers
  -------------------
         begin                : June 2009
         copyright            : (C) Martin Dobias
         email                : wonder dot sk at gmail dot com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslabelinggui.h"

#include <qgsmapcanvas.h>
#include <qgsvectorlayer.h>
#include <qgsvectordataprovider.h>
#include <qgsmaplayerregistry.h>

#include "qgsdatadefinedbutton.h"
#include "qgslabelengineconfigdialog.h"
#include "qgsexpressionbuilderdialog.h"
#include "qgsexpression.h"
#include "qgsfontutils.h"
#include "qgisapp.h"
#include "qgsmaprenderer.h"
#include "qgsproject.h"
#include "qgssvgcache.h"
#include "qgssymbollayerv2utils.h"
#include "qgscharacterselectdialog.h"
#include "qgssvgselectorwidget.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QTextEdit>
#include <QApplication>
#include <QMessageBox>
#include <QSettings>


QgsLabelingGui::QgsLabelingGui( QgsVectorLayer* layer, QgsMapCanvas* mapCanvas, QWidget* parent )
    : QWidget( parent ), mLayer( layer ), mMapCanvas( mapCanvas )
{
  if ( !layer )
    return;

  setupUi( this );
  mFontSizeUnitWidget->setUnits( QStringList() << tr( "points" ) << tr( "map units" ), 1 );
  mBufferUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );
  mShapeSizeUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );
  mShapeOffsetUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );
  mShapeRadiusUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ) << tr( "% of length" ), 1 );
  mShapeBorderWidthUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );
  mShadowOffsetUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );
  mShadowRadiusUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );
  mPointOffsetUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );
  mLineDistanceUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );
  mRepeatDistanceUnitWidget->setUnits( QStringList() << tr( "mm" ) << tr( "map units" ), 1 );

  mFontLineHeightSpinBox->setClearValue( 1.0 );
  mShapeRotationDblSpnBx->setClearValue( 0.0 );
  mShapeOffsetXSpnBx->setClearValue( 0.0 );
  mShapeOffsetYSpnBx->setClearValue( 0.0 );
  mPointOffsetXSpinBox->setClearValue( 0.0 );
  mPointOffsetYSpinBox->setClearValue( 0.0 );
  mPointAngleSpinBox->setClearValue( 0.0 );
  mFontLetterSpacingSpinBox->setClearValue( 0.0 );
  mFontWordSpacingSpinBox->setClearValue( 0.0 );

  mCharDlg = new QgsCharacterSelectorDialog( this );

  mRefFont = lblFontPreview->font();
  mPreviewSize = 24;

  // main layer label-enabling connections
  connect( chkEnableLabeling, SIGNAL( toggled( bool ) ), mFieldExpressionWidget, SLOT( setEnabled( bool ) ) );
  connect( chkEnableLabeling, SIGNAL( toggled( bool ) ), mLabelingFrame, SLOT( setEnabled( bool ) ) );

  // connections for groupboxes with separate activation checkboxes (that need to honor data defined setting)
  connect( mBufferDrawChkBx, SIGNAL( toggled( bool ) ), this, SLOT( updateUi() ) );
  connect( mShapeDrawChkBx, SIGNAL( toggled( bool ) ), this, SLOT( updateUi() ) );
  connect( mShadowDrawChkBx, SIGNAL( toggled( bool ) ), this, SLOT( updateUi() ) );

  connect( mDirectSymbChkBx, SIGNAL( toggled( bool ) ), this, SLOT( updateUi() ) );
  connect( mFormatNumChkBx, SIGNAL( toggled( bool ) ), this, SLOT( updateUi() ) );
  connect( mScaleBasedVisibilityChkBx, SIGNAL( toggled( bool ) ), this, SLOT( updateUi() ) );
  connect( mFontLimitPixelChkBox, SIGNAL( toggled( bool ) ), this, SLOT( updateUi() ) );

  // preview and basic option connections
  connect( btnTextColor, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( changeTextColor( const QColor& ) ) );
  connect( mFontTranspSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( updatePreview() ) );
  connect( mBufferDrawChkBx, SIGNAL( toggled( bool ) ), this, SLOT( updatePreview() ) );
  connect( btnBufferColor, SIGNAL( colorChanged( const QColor& ) ), this, SLOT( changeBufferColor( const QColor& ) ) );
  connect( spinBufferSize, SIGNAL( valueChanged( double ) ), this, SLOT( updatePreview() ) );
  connect( mBufferTranspSpinBox, SIGNAL( valueChanged( int ) ), this, SLOT( updatePreview() ) );
  connect( mBufferJoinStyleComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updatePreview() ) );
  connect( mBufferTranspFillChbx, SIGNAL( toggled( bool ) ), this, SLOT( updatePreview() ) );

  // internal connections
  connect( mFontTranspSlider, SIGNAL( valueChanged( int ) ), mFontTranspSpinBox, SLOT( setValue( int ) ) );
  connect( mFontTranspSpinBox, SIGNAL( valueChanged( int ) ), mFontTranspSlider, SLOT( setValue( int ) ) );
  connect( mBufferTranspSlider, SIGNAL( valueChanged( int ) ), mBufferTranspSpinBox, SLOT( setValue( int ) ) );
  connect( mBufferTranspSpinBox, SIGNAL( valueChanged( int ) ), mBufferTranspSlider, SLOT( setValue( int ) ) );
  connect( mShapeTranspSlider, SIGNAL( valueChanged( int ) ), mShapeTranspSpinBox, SLOT( setValue( int ) ) );
  connect( mShapeTranspSpinBox, SIGNAL( valueChanged( int ) ), mShapeTranspSlider, SLOT( setValue( int ) ) );
  connect( mShadowOffsetAngleDial, SIGNAL( valueChanged( int ) ), mShadowOffsetAngleSpnBx, SLOT( setValue( int ) ) );
  connect( mShadowOffsetAngleSpnBx, SIGNAL( valueChanged( int ) ), mShadowOffsetAngleDial, SLOT( setValue( int ) ) );
  connect( mShadowTranspSlider, SIGNAL( valueChanged( int ) ), mShadowTranspSpnBx, SLOT( setValue( int ) ) );
  connect( mShadowTranspSpnBx, SIGNAL( valueChanged( int ) ), mShadowTranspSlider, SLOT( setValue( int ) ) );
  connect( mLimitLabelChkBox, SIGNAL( toggled( bool ) ), mLimitLabelSpinBox, SLOT( setEnabled( bool ) ) );

  connect( btnEngineSettings, SIGNAL( clicked() ), this, SLOT( showEngineConfigDialog() ) );

  // set placement methods page based on geometry type
  switch ( layer->geometryType() )
  {
    case QGis::Point:
      stackedPlacement->setCurrentWidget( pagePoint );
      break;
    case QGis::Line:
      stackedPlacement->setCurrentWidget( pageLine );
      break;
    case QGis::Polygon:
      stackedPlacement->setCurrentWidget( pagePolygon );
      break;
    case QGis::NoGeometry:
      break;
    case QGis::UnknownGeometry:
      qFatal( "unknown geometry type unexpected" );
  }

  // show/hide options based upon geometry type
  chkMergeLines->setVisible( layer->geometryType() == QGis::Line );
  mDirectSymbolsFrame->setVisible( layer->geometryType() == QGis::Line );
  mMinSizeFrame->setVisible( layer->geometryType() != QGis::Point );

  // field combo and expression button
  mFieldExpressionWidget->setLayer( mLayer );
  QgsDistanceArea myDa;
  myDa.setSourceCrs( mLayer->crs().srsid() );
  myDa.setEllipsoidalMode( QgisApp::instance()->mapCanvas()->mapSettings().hasCrsTransformEnabled() );
  myDa.setEllipsoid( QgsProject::instance()->readEntry( "Measure", "/Ellipsoid", GEO_NONE ) );
  mFieldExpressionWidget->setGeomCalculator( myDa );

  populateFontCapitalsComboBox();

  // color buttons
  mPreviewBackgroundBtn->setColorDialogTitle( tr( "Select fill color" ) );
  mPreviewBackgroundBtn->setContext( "labelling" );
  btnTextColor->setColorDialogTitle( tr( "Select text color" ) );
  btnTextColor->setContext( "labelling" );
  btnTextColor->setDefaultColor( Qt::black );
  btnBufferColor->setColorDialogTitle( tr( "Select buffer color" ) );
  btnBufferColor->setContext( "labelling" );
  btnBufferColor->setDefaultColor( Qt::white );
  mShapeBorderColorBtn->setColorDialogTitle( tr( "Select border color" ) );
  mShapeBorderColorBtn->setContext( "labelling" );
  mShapeFillColorBtn->setColorDialogTitle( tr( "Select fill color" ) );
  mShapeFillColorBtn->setContext( "labelling" );
  mShadowColorBtn->setColorDialogTitle( tr( "Select shadow color" ) );
  mShadowColorBtn->setContext( "labelling" );
  mShadowColorBtn->setDefaultColor( Qt::black );

  // set up quadrant offset button group
  mQuadrantBtnGrp = new QButtonGroup( this );
  mQuadrantBtnGrp->addButton( mPointOffsetAboveLeft, ( int )QgsPalLayerSettings::QuadrantAboveLeft );
  mQuadrantBtnGrp->addButton( mPointOffsetAbove, ( int )QgsPalLayerSettings::QuadrantAbove );
  mQuadrantBtnGrp->addButton( mPointOffsetAboveRight, ( int )QgsPalLayerSettings::QuadrantAboveRight );
  mQuadrantBtnGrp->addButton( mPointOffsetLeft, ( int )QgsPalLayerSettings::QuadrantLeft );
  mQuadrantBtnGrp->addButton( mPointOffsetOver, ( int )QgsPalLayerSettings::QuadrantOver );
  mQuadrantBtnGrp->addButton( mPointOffsetRight, ( int )QgsPalLayerSettings::QuadrantRight );
  mQuadrantBtnGrp->addButton( mPointOffsetBelowLeft, ( int )QgsPalLayerSettings::QuadrantBelowLeft );
  mQuadrantBtnGrp->addButton( mPointOffsetBelow, ( int )QgsPalLayerSettings::QuadrantBelow );
  mQuadrantBtnGrp->addButton( mPointOffsetBelowRight, ( int )QgsPalLayerSettings::QuadrantBelowRight );
  mQuadrantBtnGrp->setExclusive( true );

  // setup direction symbol(s) button group
  mDirectSymbBtnGrp = new QButtonGroup( this );
  mDirectSymbBtnGrp->addButton( mDirectSymbRadioBtnLR, ( int )QgsPalLayerSettings::SymbolLeftRight );
  mDirectSymbBtnGrp->addButton( mDirectSymbRadioBtnAbove, ( int )QgsPalLayerSettings::SymbolAbove );
  mDirectSymbBtnGrp->addButton( mDirectSymbRadioBtnBelow, ( int )QgsPalLayerSettings::SymbolBelow );
  mDirectSymbBtnGrp->setExclusive( true );

  // upside-down labels button group
  mUpsidedownBtnGrp = new QButtonGroup( this );
  mUpsidedownBtnGrp->addButton( mUpsidedownRadioOff, ( int )QgsPalLayerSettings::Upright );
  mUpsidedownBtnGrp->addButton( mUpsidedownRadioDefined, ( int )QgsPalLayerSettings::ShowDefined );
  mUpsidedownBtnGrp->addButton( mUpsidedownRadioAll, ( int )QgsPalLayerSettings::ShowAll );
  mUpsidedownBtnGrp->setExclusive( true );

  //mShapeCollisionsChkBx->setVisible( false ); // until implemented

  // post updatePlacementWidgets() connections
  connect( chkLineAbove, SIGNAL( toggled( bool ) ), this, SLOT( updatePlacementWidgets() ) );
  connect( chkLineBelow, SIGNAL( toggled( bool ) ), this, SLOT( updatePlacementWidgets() ) );

  // setup point placement button group (assigned enum id currently unused)
  mPlacePointBtnGrp = new QButtonGroup( this );
  mPlacePointBtnGrp->addButton( radAroundPoint, ( int )QgsPalLayerSettings::AroundPoint );
  mPlacePointBtnGrp->addButton( radOverPoint, ( int )QgsPalLayerSettings::OverPoint );
  mPlacePointBtnGrp->setExclusive( true );
  connect( mPlacePointBtnGrp, SIGNAL( buttonClicked( int ) ), this, SLOT( updatePlacementWidgets() ) );

  // setup line placement button group (assigned enum id currently unused)
  mPlaceLineBtnGrp = new QButtonGroup( this );
  mPlaceLineBtnGrp->addButton( radLineParallel, ( int )QgsPalLayerSettings::Line );
  mPlaceLineBtnGrp->addButton( radLineCurved, ( int )QgsPalLayerSettings::Curved );
  mPlaceLineBtnGrp->addButton( radLineHorizontal, ( int )QgsPalLayerSettings::Horizontal );
  mPlaceLineBtnGrp->setExclusive( true );
  connect( mPlaceLineBtnGrp, SIGNAL( buttonClicked( int ) ), this, SLOT( updatePlacementWidgets() ) );

  // setup polygon placement button group (assigned enum id currently unused)
  mPlacePolygonBtnGrp = new QButtonGroup( this );
  mPlacePolygonBtnGrp->addButton( radOverCentroid, ( int )QgsPalLayerSettings::OverPoint );
  mPlacePolygonBtnGrp->addButton( radAroundCentroid, ( int )QgsPalLayerSettings::AroundPoint );
  mPlacePolygonBtnGrp->addButton( radPolygonHorizontal, ( int )QgsPalLayerSettings::Horizontal );
  mPlacePolygonBtnGrp->addButton( radPolygonFree, ( int )QgsPalLayerSettings::Free );
  mPlacePolygonBtnGrp->addButton( radPolygonPerimeter, ( int )QgsPalLayerSettings::Line );
  mPlacePolygonBtnGrp->setExclusive( true );
  connect( mPlacePolygonBtnGrp, SIGNAL( buttonClicked( int ) ), this, SLOT( updatePlacementWidgets() ) );

  // TODO: is this necessary? maybe just use the data defined-only rotation?
  mPointAngleDDBtn->setVisible( false );

  // Global settings group for groupboxes' saved/retored collapsed state
  // maintains state across different dialogs
  foreach ( QgsCollapsibleGroupBox *grpbox, findChildren<QgsCollapsibleGroupBox*>() )
  {
    grpbox->setSettingGroup( QString( "mAdvLabelingDlg" ) );
  }

  connect( groupBox_mPreview,
           SIGNAL( collapsedStateChanged( bool ) ),
           this,
           SLOT( collapseSample( bool ) ) );

  // get rid of annoying outer focus rect on Mac
  mLabelingOptionsListWidget->setAttribute( Qt::WA_MacShowFocusRect, false );

  QSettings settings;

  // reset horiz strech of left side of options splitter (set to 1 for previewing in Qt Designer)
  QSizePolicy policy( mLabelingOptionsListFrame->sizePolicy() );
  policy.setHorizontalStretch( 0 );
  mLabelingOptionsListFrame->setSizePolicy( policy );
  if ( !settings.contains( QString( "/Windows/Labeling/OptionsSplitState" ) ) )
  {
    // set left list widget width on intial showing
    QList<int> splitsizes;
    splitsizes << 115;
    mLabelingOptionsSplitter->setSizes( splitsizes );
  }

  // set up reverse connection from stack to list
  connect( mLabelStackedWidget, SIGNAL( currentChanged( int ) ), this, SLOT( optionsStackedWidget_CurrentChanged( int ) ) );

  // restore dialog, splitters and current tab
  mFontPreviewSplitter->restoreState( settings.value( QString( "/Windows/Labeling/FontPreviewSplitState" ) ).toByteArray() );
  mLabelingOptionsSplitter->restoreState( settings.value( QString( "/Windows/Labeling/OptionsSplitState" ) ).toByteArray() );

  mLabelingOptionsListWidget->setCurrentRow( settings.value( QString( "/Windows/Labeling/Tab" ), 0 ).toInt() );
}

void QgsLabelingGui::init()
{
  // load labeling settings from layer
  QgsPalLayerSettings lyr;
  lyr.readFromLayer( mLayer );

  blockInitSignals( true );

  // enable/disable main options based upon whether layer is being labeled
  chkEnableLabeling->setChecked( lyr.enabled );
  mFieldExpressionWidget->setEnabled( chkEnableLabeling->isChecked() );
  mLabelingFrame->setEnabled( chkEnableLabeling->isChecked() );

  // set the current field or add the current expression to the bottom of the list
  mFieldExpressionWidget->setField( lyr.fieldName );

  // populate placement options
  mCentroidRadioWhole->setChecked( lyr.centroidWhole );
  mCentroidInsideCheckBox->setChecked( lyr.centroidInside );
  switch ( lyr.placement )
  {
    case QgsPalLayerSettings::AroundPoint:
      radAroundPoint->setChecked( true );
      radAroundCentroid->setChecked( true );

      mLineDistanceSpnBx->setValue( lyr.dist );
      mLineDistanceUnitWidget->setUnit( lyr.distInMapUnits ? 1 : 0 );
      mLineDistanceUnitWidget->setMapUnitScale( lyr.distMapUnitScale );
      //spinAngle->setValue( lyr.angle ); // TODO: uncomment when supported
      break;
    case QgsPalLayerSettings::OverPoint:
      radOverPoint->setChecked( true );
      radOverCentroid->setChecked( true );

      mQuadrantBtnGrp->button(( int )lyr.quadOffset )->setChecked( true );
      mPointOffsetXSpinBox->setValue( lyr.xOffset );
      mPointOffsetYSpinBox->setValue( lyr.yOffset );
      mPointOffsetUnitWidget->setUnit( lyr.labelOffsetInMapUnits ? 1 : 0 );
      mPointOffsetUnitWidget->setMapUnitScale( lyr.labelOffsetMapUnitScale );
      mPointAngleSpinBox->setValue( lyr.angleOffset );
      break;
    case QgsPalLayerSettings::Line:
      radLineParallel->setChecked( true );
      radPolygonPerimeter->setChecked( true );
      break;
    case QgsPalLayerSettings::Curved:
      radLineCurved->setChecked( true );
      break;
    case QgsPalLayerSettings::Horizontal:
      radPolygonHorizontal->setChecked( true );
      radLineHorizontal->setChecked( true );
      break;
    case QgsPalLayerSettings::Free:
      radPolygonFree->setChecked( true );
      break;
  }

  if ( lyr.placement == QgsPalLayerSettings::Line || lyr.placement == QgsPalLayerSettings::Curved )
  {
    mLineDistanceSpnBx->setValue( lyr.dist );
    mLineDistanceUnitWidget->setUnit( lyr.distInMapUnits ? 1 : 0 );
    mLineDistanceUnitWidget->setMapUnitScale( lyr.distMapUnitScale );
    chkLineAbove->setChecked( lyr.placementFlags & QgsPalLayerSettings::AboveLine );
    chkLineBelow->setChecked( lyr.placementFlags & QgsPalLayerSettings::BelowLine );
    chkLineOn->setChecked( lyr.placementFlags & QgsPalLayerSettings::OnLine );
    if ( !( lyr.placementFlags & QgsPalLayerSettings::MapOrientation ) )
      chkLineOrientationDependent->setChecked( true );
  }

  // Label repeat distance
  mRepeatDistanceSpinBox->setValue( lyr.repeatDistance );
  mRepeatDistanceUnitWidget->setUnit( lyr.repeatDistanceUnit - 1 );
  mRepeatDistanceUnitWidget->setMapUnitScale( lyr.repeatDistanceMapUnitScale );

  mPrioritySlider->setValue( lyr.priority );
  chkNoObstacle->setChecked( lyr.obstacle );
  chkLabelPerFeaturePart->setChecked( lyr.labelPerPart );
  mPalShowAllLabelsForLayerChkBx->setChecked( lyr.displayAll );
  chkMergeLines->setChecked( lyr.mergeLines );
  mMinSizeSpinBox->setValue( lyr.minFeatureSize );
  mLimitLabelChkBox->setChecked( lyr.limitNumLabels );
  mLimitLabelSpinBox->setValue( lyr.maxNumLabels );

  // direction symbol(s)
  mDirectSymbChkBx->setChecked( lyr.addDirectionSymbol );
  mDirectSymbLeftLineEdit->setText( lyr.leftDirectionSymbol );
  mDirectSymbRightLineEdit->setText( lyr.rightDirectionSymbol );
  mDirectSymbRevChkBx->setChecked( lyr.reverseDirectionSymbol );

  mDirectSymbBtnGrp->button(( int )lyr.placeDirectionSymbol )->setChecked( true );
  mUpsidedownBtnGrp->button(( int )lyr.upsidedownLabels )->setChecked( true );

  // curved label max character angles
  mMaxCharAngleInDSpinBox->setValue( lyr.maxCurvedCharAngleIn );
  // lyr.maxCurvedCharAngleOut must be negative, but it is shown as positive spinbox in GUI
  mMaxCharAngleOutDSpinBox->setValue( qAbs( lyr.maxCurvedCharAngleOut ) );

  wrapCharacterEdit->setText( lyr.wrapChar );
  mFontLineHeightSpinBox->setValue( lyr.multilineHeight );
  mFontMultiLineAlignComboBox->setCurrentIndex(( unsigned int ) lyr.multilineAlign );
  chkPreserveRotation->setChecked( lyr.preserveRotation );

  mPreviewBackgroundBtn->setColor( lyr.previewBkgrdColor );
  mPreviewBackgroundBtn->setDefaultColor( lyr.previewBkgrdColor );
  setPreviewBackground( lyr.previewBkgrdColor );

  mScaleBasedVisibilityChkBx->setChecked( lyr.scaleVisibility );
  mScaleBasedVisibilityMinSpnBx->setValue( lyr.scaleMin );
  mScaleBasedVisibilityMaxSpnBx->setValue( lyr.scaleMax );

  // buffer
  mBufferDrawChkBx->setChecked( lyr.bufferDraw );
  spinBufferSize->setValue( lyr.bufferSize );
  mBufferUnitWidget->setUnit( lyr.bufferSizeInMapUnits ? 1 : 0 );
  mBufferUnitWidget->setMapUnitScale( lyr.bufferSizeMapUnitScale );
  btnBufferColor->setColor( lyr.bufferColor );
  mBufferTranspSpinBox->setValue( lyr.bufferTransp );
  mBufferJoinStyleComboBox->setPenJoinStyle( lyr.bufferJoinStyle );
  mBufferTranspFillChbx->setChecked( !lyr.bufferNoFill );
  comboBufferBlendMode->setBlendMode( lyr.bufferBlendMode );

  mFormatNumChkBx->setChecked( lyr.formatNumbers );
  mFormatNumDecimalsSpnBx->setValue( lyr.decimals );
  mFormatNumPlusSignChkBx->setChecked( lyr.plusSign );

  // set pixel size limiting checked state before unit choice so limiting can be
  // turned on as a default for map units, if minimum trigger value of 0 is used
  mFontLimitPixelChkBox->setChecked( lyr.fontLimitPixelSize );
  mMinPixelLimit = lyr.fontMinPixelSize; // ignored after first settings save
  mFontMinPixelSpinBox->setValue( lyr.fontMinPixelSize == 0 ? 3 : lyr.fontMinPixelSize );
  mFontMaxPixelSpinBox->setValue( lyr.fontMaxPixelSize );
  mFontSizeUnitWidget->setUnit( lyr.fontSizeInMapUnits ? 1 : 0 );
  mFontSizeUnitWidget->setMapUnitScale( lyr.fontSizeMapUnitScale );

  mRefFont = lyr.textFont;
  mFontSizeSpinBox->setValue( lyr.textFont.pointSizeF() );
  btnTextColor->setColor( lyr.textColor );
  mFontTranspSpinBox->setValue( lyr.textTransp );
  comboBlendMode->setBlendMode( lyr.blendMode );

  mFontWordSpacingSpinBox->setValue( lyr.textFont.wordSpacing() );
  mFontLetterSpacingSpinBox->setValue( lyr.textFont.letterSpacing() );

  QgsFontUtils::updateFontViaStyle( mRefFont, lyr.textNamedStyle );
  updateFont( mRefFont );

  // show 'font not found' if substitution has occurred (should come after updateFont())
  mFontMissingLabel->setVisible( !lyr.mTextFontFound );
  if ( !lyr.mTextFontFound )
  {
    QString missingTxt = tr( "%1 not found. Default substituted." );
    QString txtPrepend = tr( "Chosen font" );
    if ( !lyr.mTextFontFamily.isEmpty() )
    {
      txtPrepend = QString( "'%1'" ).arg( lyr.mTextFontFamily );
    }
    mFontMissingLabel->setText( missingTxt.arg( txtPrepend ) );

    // ensure user is sent to 'Text style' section to see notice
    mLabelingOptionsListWidget->setCurrentRow( 0 );
  }

  // shape background
  mShapeDrawChkBx->setChecked( lyr.shapeDraw );
  mShapeTypeCmbBx->blockSignals( true );
  mShapeTypeCmbBx->setCurrentIndex( lyr.shapeType );
  mShapeTypeCmbBx->blockSignals( false );
  mShapeSVGPathLineEdit->setText( lyr.shapeSVGFile );

  mShapeSizeCmbBx->setCurrentIndex( lyr.shapeSizeType );
  mShapeSizeXSpnBx->setValue( lyr.shapeSize.x() );
  mShapeSizeYSpnBx->setValue( lyr.shapeSize.y() );
  mShapeSizeUnitWidget->setUnit( lyr.shapeSizeUnits - 1 );
  mShapeSizeUnitWidget->setMapUnitScale( lyr.shapeSizeMapUnitScale );
  mShapeRotationCmbBx->setCurrentIndex( lyr.shapeRotationType );
  mShapeRotationDblSpnBx->setEnabled( lyr.shapeRotationType != QgsPalLayerSettings::RotationSync );
  mShapeRotationDDBtn->setEnabled( lyr.shapeRotationType != QgsPalLayerSettings::RotationSync );
  mShapeRotationDblSpnBx->setValue( lyr.shapeRotation );
  mShapeOffsetXSpnBx->setValue( lyr.shapeOffset.x() );
  mShapeOffsetYSpnBx->setValue( lyr.shapeOffset.y() );
  mShapeOffsetUnitWidget->setUnit( lyr.shapeOffsetUnits - 1 );
  mShapeOffsetUnitWidget->setMapUnitScale( lyr.shapeOffsetMapUnitScale );
  mShapeRadiusXDbSpnBx->setValue( lyr.shapeRadii.x() );
  mShapeRadiusYDbSpnBx->setValue( lyr.shapeRadii.y() );
  mShapeRadiusUnitWidget->setUnit( lyr.shapeRadiiUnits - 1 );
  mShapeRadiusUnitWidget->setMapUnitScale( lyr.shapeRadiiMapUnitScale );

  mShapeFillColorBtn->setColor( lyr.shapeFillColor );
  mShapeBorderColorBtn->setColor( lyr.shapeBorderColor );
  mShapeBorderWidthSpnBx->setValue( lyr.shapeBorderWidth );
  mShapeBorderWidthUnitWidget->setUnit( lyr.shapeBorderWidthUnits - 1 );
  mShapeBorderWidthUnitWidget->setMapUnitScale( lyr.shapeBorderWidthMapUnitScale );
  mShapePenStyleCmbBx->setPenJoinStyle( lyr.shapeJoinStyle );

  mShapeTranspSpinBox->setValue( lyr.shapeTransparency );
  mShapeBlendCmbBx->setBlendMode( lyr.shapeBlendMode );

  mLoadSvgParams = false;
  on_mShapeTypeCmbBx_currentIndexChanged( lyr.shapeType ); // force update of shape background gui

  // drop shadow
  mShadowDrawChkBx->setChecked( lyr.shadowDraw );
  mShadowUnderCmbBx->setCurrentIndex( lyr.shadowUnder );
  mShadowOffsetAngleSpnBx->setValue( lyr.shadowOffsetAngle );
  mShadowOffsetSpnBx->setValue( lyr.shadowOffsetDist );
  mShadowOffsetUnitWidget->setUnit( lyr.shadowOffsetUnits - 1 );
  mShadowOffsetUnitWidget->setMapUnitScale( lyr.shadowOffsetMapUnitScale );
  mShadowOffsetGlobalChkBx->setChecked( lyr.shadowOffsetGlobal );

  mShadowRadiusDblSpnBx->setValue( lyr.shadowRadius );
  mShadowRadiusUnitWidget->setUnit( lyr.shadowRadiusUnits - 1 );
  mShadowRadiusUnitWidget->setMapUnitScale( lyr.shadowRadiusMapUnitScale );
  mShadowRadiusAlphaChkBx->setChecked( lyr.shadowRadiusAlphaOnly );
  mShadowTranspSpnBx->setValue( lyr.shadowTransparency );
  mShadowScaleSpnBx->setValue( lyr.shadowScale );

  mShadowColorBtn->setColor( lyr.shadowColor );
  mShadowBlendCmbBx->setBlendMode( lyr.shadowBlendMode );

  updatePlacementWidgets();

  // needs to come before data defined setup, so connections work
  blockInitSignals( false );

  // set up data defined toolbuttons
  // do this after other widgets are configured, so they can be enabled/disabled
  populateDataDefinedButtons( lyr );

  enableDataDefinedAlignment( mCoordXDDBtn->isActive() && mCoordYDDBtn->isActive() );

  updateUi(); // should come after data defined button setup
}


QgsLabelingGui::~QgsLabelingGui()
{
  QSettings settings;
  settings.setValue( QString( "/Windows/Labeling/FontPreviewSplitState" ), mFontPreviewSplitter->saveState() );
  settings.setValue( QString( "/Windows/Labeling/OptionsSplitState" ), mLabelingOptionsSplitter->saveState() );
  settings.setValue( QString( "/Windows/Labeling/Tab" ), mLabelingOptionsListWidget->currentRow() );
}

void QgsLabelingGui::blockInitSignals( bool block )
{
  chkLineAbove->blockSignals( block );
  chkLineBelow->blockSignals( block );
  mPlacePointBtnGrp->blockSignals( block );
  mPlaceLineBtnGrp->blockSignals( block );
  mPlacePolygonBtnGrp->blockSignals( block );
}

void QgsLabelingGui::optionsStackedWidget_CurrentChanged( int indx )
{
  mLabelingOptionsListWidget->blockSignals( true );
  mLabelingOptionsListWidget->setCurrentRow( indx );
  mLabelingOptionsListWidget->blockSignals( false );
}

void QgsLabelingGui::collapseSample( bool collapse )
{
  if ( collapse )
  {
    QList<int> splitSizes = mFontPreviewSplitter->sizes();
    if ( splitSizes[0] > groupBox_mPreview->height() )
    {
      int delta = splitSizes[0] - groupBox_mPreview->height();
      splitSizes[0] -= delta;
      splitSizes[1] += delta;
      mFontPreviewSplitter->setSizes( splitSizes );
    }
  }
}

void QgsLabelingGui::apply()
{
  writeSettingsToLayer();
  mFontMissingLabel->setVisible( false );
  QgisApp::instance()->markDirty();
  // trigger refresh
  if ( mMapCanvas )
  {
    mMapCanvas->refresh();
  }
}

void QgsLabelingGui::writeSettingsToLayer()
{
  QgsPalLayerSettings settings = layerSettings();
  settings.writeToLayer( mLayer );
}

QgsPalLayerSettings QgsLabelingGui::layerSettings()
{
  QgsPalLayerSettings lyr;

  lyr.enabled = chkEnableLabeling->isChecked();

  bool isExpression;
  lyr.fieldName = mFieldExpressionWidget->currentField( &isExpression );
  lyr.isExpression = isExpression;

  lyr.dist = 0;
  lyr.placementFlags = 0;

  QWidget* curPlacementWdgt = stackedPlacement->currentWidget();
  lyr.centroidWhole = mCentroidRadioWhole->isChecked();
  lyr.centroidInside = mCentroidInsideCheckBox->isChecked();
  if (( curPlacementWdgt == pagePoint && radAroundPoint->isChecked() )
      || ( curPlacementWdgt == pagePolygon && radAroundCentroid->isChecked() ) )
  {
    lyr.placement = QgsPalLayerSettings::AroundPoint;
    lyr.dist = mLineDistanceSpnBx->value();
    lyr.distInMapUnits = ( mLineDistanceUnitWidget->getUnit() == 1 );
    lyr.distMapUnitScale = mLineDistanceUnitWidget->getMapUnitScale();
  }
  else if (( curPlacementWdgt == pagePoint && radOverPoint->isChecked() )
           || ( curPlacementWdgt == pagePolygon && radOverCentroid->isChecked() ) )
  {
    lyr.placement = QgsPalLayerSettings::OverPoint;
    lyr.quadOffset = ( QgsPalLayerSettings::QuadrantPosition )mQuadrantBtnGrp->checkedId();
    lyr.xOffset = mPointOffsetXSpinBox->value();
    lyr.yOffset = mPointOffsetYSpinBox->value();
    lyr.labelOffsetInMapUnits = ( mPointOffsetUnitWidget->getUnit() == 1 );
    lyr.labelOffsetMapUnitScale = mPointOffsetUnitWidget->getMapUnitScale();
    lyr.angleOffset = mPointAngleSpinBox->value();
  }
  else if (( curPlacementWdgt == pageLine && radLineParallel->isChecked() )
           || ( curPlacementWdgt == pagePolygon && radPolygonPerimeter->isChecked() )
           || ( curPlacementWdgt == pageLine && radLineCurved->isChecked() ) )
  {
    bool curved = ( curPlacementWdgt == pageLine && radLineCurved->isChecked() );
    lyr.placement = ( curved ? QgsPalLayerSettings::Curved : QgsPalLayerSettings::Line );
    lyr.dist = mLineDistanceSpnBx->value();
    lyr.distInMapUnits = ( mLineDistanceUnitWidget->getUnit() == 1 );
    lyr.distMapUnitScale = mLineDistanceUnitWidget->getMapUnitScale();
    if ( chkLineAbove->isChecked() )
      lyr.placementFlags |= QgsPalLayerSettings::AboveLine;
    if ( chkLineBelow->isChecked() )
      lyr.placementFlags |= QgsPalLayerSettings::BelowLine;
    if ( chkLineOn->isChecked() )
      lyr.placementFlags |= QgsPalLayerSettings::OnLine;

    if ( ! chkLineOrientationDependent->isChecked() )
      lyr.placementFlags |= QgsPalLayerSettings::MapOrientation;
  }
  else if (( curPlacementWdgt == pageLine && radLineHorizontal->isChecked() )
           || ( curPlacementWdgt == pagePolygon && radPolygonHorizontal->isChecked() ) )
  {
    lyr.placement = QgsPalLayerSettings::Horizontal;
  }
  else if ( radPolygonFree->isChecked() )
  {
    lyr.placement = QgsPalLayerSettings::Free;
  }
  else
  {
    qFatal( "Invalid settings" );
  }

  lyr.repeatDistance = mRepeatDistanceSpinBox->value();
  lyr.repeatDistanceUnit = static_cast<QgsPalLayerSettings::SizeUnit>( 1 + mRepeatDistanceUnitWidget->getUnit() );
  lyr.repeatDistanceMapUnitScale = mRepeatDistanceUnitWidget->getMapUnitScale();

  lyr.textColor = btnTextColor->color();
  lyr.textFont = mRefFont;
  lyr.textNamedStyle = mFontStyleComboBox->currentText();
  lyr.textTransp = mFontTranspSpinBox->value();
  lyr.blendMode = comboBlendMode->blendMode();
  lyr.previewBkgrdColor = mPreviewBackgroundBtn->color();

  lyr.priority = mPrioritySlider->value();
  lyr.obstacle = chkNoObstacle->isChecked();
  lyr.labelPerPart = chkLabelPerFeaturePart->isChecked();
  lyr.displayAll = mPalShowAllLabelsForLayerChkBx->isChecked();
  lyr.mergeLines = chkMergeLines->isChecked();

  lyr.scaleVisibility = mScaleBasedVisibilityChkBx->isChecked();
  lyr.scaleMin = mScaleBasedVisibilityMinSpnBx->value();
  lyr.scaleMax = mScaleBasedVisibilityMaxSpnBx->value();

  // buffer
  lyr.bufferDraw = mBufferDrawChkBx->isChecked();
  lyr.bufferSize = spinBufferSize->value();
  lyr.bufferColor = btnBufferColor->color();
  lyr.bufferTransp = mBufferTranspSpinBox->value();
  lyr.bufferSizeInMapUnits = ( mBufferUnitWidget->getUnit() == 1 );
  lyr.bufferSizeMapUnitScale = mBufferUnitWidget->getMapUnitScale();
  lyr.bufferJoinStyle = mBufferJoinStyleComboBox->penJoinStyle();
  lyr.bufferNoFill = !mBufferTranspFillChbx->isChecked();
  lyr.bufferBlendMode = comboBufferBlendMode->blendMode();

  // shape background
  lyr.shapeDraw = mShapeDrawChkBx->isChecked();
  lyr.shapeType = ( QgsPalLayerSettings::ShapeType )mShapeTypeCmbBx->currentIndex();
  lyr.shapeSVGFile = mShapeSVGPathLineEdit->text();

  lyr.shapeSizeType = ( QgsPalLayerSettings::SizeType )mShapeSizeCmbBx->currentIndex();
  lyr.shapeSize = QPointF( mShapeSizeXSpnBx->value(), mShapeSizeYSpnBx->value() );
  lyr.shapeSizeUnits = ( QgsPalLayerSettings::SizeUnit )( mShapeSizeUnitWidget->getUnit() + 1 );
  lyr.shapeSizeMapUnitScale = mShapeSizeUnitWidget->getMapUnitScale();
  lyr.shapeRotationType = ( QgsPalLayerSettings::RotationType )( mShapeRotationCmbBx->currentIndex() );
  lyr.shapeRotation = mShapeRotationDblSpnBx->value();
  lyr.shapeOffset = QPointF( mShapeOffsetXSpnBx->value(), mShapeOffsetYSpnBx->value() );
  lyr.shapeOffsetUnits = ( QgsPalLayerSettings::SizeUnit )( mShapeOffsetUnitWidget->getUnit() + 1 );
  lyr.shapeOffsetMapUnitScale = mShapeOffsetUnitWidget->getMapUnitScale();
  lyr.shapeRadii = QPointF( mShapeRadiusXDbSpnBx->value(), mShapeRadiusYDbSpnBx->value() );
  lyr.shapeRadiiUnits = ( QgsPalLayerSettings::SizeUnit )( mShapeRadiusUnitWidget->getUnit() + 1 );
  lyr.shapeRadiiMapUnitScale = mShapeRadiusUnitWidget->getMapUnitScale();

  lyr.shapeFillColor = mShapeFillColorBtn->color();
  lyr.shapeBorderColor = mShapeBorderColorBtn->color();
  lyr.shapeBorderWidth = mShapeBorderWidthSpnBx->value();
  lyr.shapeBorderWidthUnits = ( QgsPalLayerSettings::SizeUnit )( mShapeBorderWidthUnitWidget->getUnit() + 1 );
  lyr.shapeBorderWidthMapUnitScale = mShapeBorderWidthUnitWidget->getMapUnitScale();
  lyr.shapeJoinStyle = mShapePenStyleCmbBx->penJoinStyle();
  lyr.shapeTransparency = mShapeTranspSpinBox->value();
  lyr.shapeBlendMode = mShapeBlendCmbBx->blendMode();

  // drop shadow
  lyr.shadowDraw = mShadowDrawChkBx->isChecked();
  lyr.shadowUnder = ( QgsPalLayerSettings::ShadowType )mShadowUnderCmbBx->currentIndex();
  lyr.shadowOffsetAngle = mShadowOffsetAngleSpnBx->value();
  lyr.shadowOffsetDist = mShadowOffsetSpnBx->value();
  lyr.shadowOffsetUnits = ( QgsPalLayerSettings::SizeUnit )( mShadowOffsetUnitWidget->getUnit() + 1 );
  lyr.shadowOffsetMapUnitScale = mShadowOffsetUnitWidget->getMapUnitScale();
  lyr.shadowOffsetGlobal = mShadowOffsetGlobalChkBx->isChecked();
  lyr.shadowRadius = mShadowRadiusDblSpnBx->value();
  lyr.shadowRadiusUnits = ( QgsPalLayerSettings::SizeUnit )( mShadowRadiusUnitWidget->getUnit() + 1 );
  lyr.shadowRadiusMapUnitScale = mShadowRadiusUnitWidget->getMapUnitScale();
  lyr.shadowRadiusAlphaOnly = mShadowRadiusAlphaChkBx->isChecked();
  lyr.shadowTransparency = mShadowTranspSpnBx->value();
  lyr.shadowScale = mShadowScaleSpnBx->value();
  lyr.shadowColor = mShadowColorBtn->color();
  lyr.shadowBlendMode = mShadowBlendCmbBx->blendMode();

  // format numbers
  lyr.formatNumbers = mFormatNumChkBx->isChecked();
  lyr.decimals = mFormatNumDecimalsSpnBx->value();
  lyr.plusSign = mFormatNumPlusSignChkBx->isChecked();

  // direction symbol(s)
  lyr.addDirectionSymbol = mDirectSymbChkBx->isChecked();
  lyr.leftDirectionSymbol = mDirectSymbLeftLineEdit->text();
  lyr.rightDirectionSymbol = mDirectSymbRightLineEdit->text();
  lyr.reverseDirectionSymbol = mDirectSymbRevChkBx->isChecked();
  lyr.placeDirectionSymbol = ( QgsPalLayerSettings::DirectionSymbols )mDirectSymbBtnGrp->checkedId();

  lyr.upsidedownLabels = ( QgsPalLayerSettings::UpsideDownLabels )mUpsidedownBtnGrp->checkedId();

  lyr.maxCurvedCharAngleIn = mMaxCharAngleInDSpinBox->value();
  // lyr.maxCurvedCharAngleOut must be negative, but it is shown as positive spinbox in GUI
  lyr.maxCurvedCharAngleOut = -mMaxCharAngleOutDSpinBox->value();

  lyr.minFeatureSize = mMinSizeSpinBox->value();
  lyr.limitNumLabels = mLimitLabelChkBox->isChecked();
  lyr.maxNumLabels = mLimitLabelSpinBox->value();
  lyr.fontSizeInMapUnits = ( mFontSizeUnitWidget->getUnit() == 1 );
  lyr.fontSizeMapUnitScale = mFontSizeUnitWidget->getMapUnitScale();
  lyr.fontLimitPixelSize = mFontLimitPixelChkBox->isChecked();
  lyr.fontMinPixelSize = mFontMinPixelSpinBox->value();
  lyr.fontMaxPixelSize = mFontMaxPixelSpinBox->value();
  lyr.wrapChar = wrapCharacterEdit->text();
  lyr.multilineHeight = mFontLineHeightSpinBox->value();
  lyr.multilineAlign = ( QgsPalLayerSettings::MultiLineAlign ) mFontMultiLineAlignComboBox->currentIndex();
  lyr.preserveRotation = chkPreserveRotation->isChecked();

  // data defined labeling
  // text style
  setDataDefinedProperty( mFontDDBtn, QgsPalLayerSettings::Family, lyr );
  setDataDefinedProperty( mFontStyleDDBtn, QgsPalLayerSettings::FontStyle, lyr );
  setDataDefinedProperty( mFontUnderlineDDBtn, QgsPalLayerSettings::Underline, lyr );
  setDataDefinedProperty( mFontStrikeoutDDBtn, QgsPalLayerSettings::Strikeout, lyr );
  setDataDefinedProperty( mFontBoldDDBtn, QgsPalLayerSettings::Bold, lyr );
  setDataDefinedProperty( mFontItalicDDBtn, QgsPalLayerSettings::Italic, lyr );
  setDataDefinedProperty( mFontSizeDDBtn, QgsPalLayerSettings::Size, lyr );
  setDataDefinedProperty( mFontUnitsDDBtn, QgsPalLayerSettings::FontSizeUnit, lyr );
  setDataDefinedProperty( mFontColorDDBtn, QgsPalLayerSettings::Color, lyr );
  setDataDefinedProperty( mFontTranspDDBtn, QgsPalLayerSettings::FontTransp, lyr );
  setDataDefinedProperty( mFontCaseDDBtn, QgsPalLayerSettings::FontCase, lyr );
  setDataDefinedProperty( mFontLetterSpacingDDBtn, QgsPalLayerSettings::FontLetterSpacing, lyr );
  setDataDefinedProperty( mFontWordSpacingDDBtn, QgsPalLayerSettings::FontWordSpacing, lyr );
  setDataDefinedProperty( mFontBlendModeDDBtn, QgsPalLayerSettings::FontBlendMode, lyr );

  // text formatting
  setDataDefinedProperty( mWrapCharDDBtn, QgsPalLayerSettings::MultiLineWrapChar, lyr );
  setDataDefinedProperty( mFontLineHeightDDBtn, QgsPalLayerSettings::MultiLineHeight, lyr );
  setDataDefinedProperty( mFontMultiLineAlignDDBtn, QgsPalLayerSettings::MultiLineAlignment, lyr );
  setDataDefinedProperty( mDirectSymbDDBtn, QgsPalLayerSettings::DirSymbDraw, lyr );
  setDataDefinedProperty( mDirectSymbLeftDDBtn, QgsPalLayerSettings::DirSymbLeft, lyr );
  setDataDefinedProperty( mDirectSymbRightDDBtn, QgsPalLayerSettings::DirSymbRight, lyr );
  setDataDefinedProperty( mDirectSymbPlacementDDBtn, QgsPalLayerSettings::DirSymbPlacement, lyr );
  setDataDefinedProperty( mDirectSymbRevDDBtn, QgsPalLayerSettings::DirSymbReverse, lyr );
  setDataDefinedProperty( mFormatNumDDBtn, QgsPalLayerSettings::NumFormat, lyr );
  setDataDefinedProperty( mFormatNumDecimalsDDBtn, QgsPalLayerSettings::NumDecimals, lyr );
  setDataDefinedProperty( mFormatNumPlusSignDDBtn, QgsPalLayerSettings::NumPlusSign, lyr );

  // text buffer
  setDataDefinedProperty( mBufferDrawDDBtn, QgsPalLayerSettings::BufferDraw, lyr );
  setDataDefinedProperty( mBufferSizeDDBtn, QgsPalLayerSettings::BufferSize, lyr );
  setDataDefinedProperty( mBufferUnitsDDBtn, QgsPalLayerSettings::BufferUnit, lyr );
  setDataDefinedProperty( mBufferColorDDBtn, QgsPalLayerSettings::BufferColor, lyr );
  setDataDefinedProperty( mBufferTranspDDBtn, QgsPalLayerSettings::BufferTransp, lyr );
  setDataDefinedProperty( mBufferJoinStyleDDBtn, QgsPalLayerSettings::BufferJoinStyle, lyr );
  setDataDefinedProperty( mBufferBlendModeDDBtn, QgsPalLayerSettings::BufferBlendMode, lyr );

  // background
  setDataDefinedProperty( mShapeDrawDDBtn, QgsPalLayerSettings::ShapeDraw, lyr );
  setDataDefinedProperty( mShapeTypeDDBtn, QgsPalLayerSettings::ShapeKind, lyr );
  setDataDefinedProperty( mShapeSVGPathDDBtn, QgsPalLayerSettings::ShapeSVGFile, lyr );
  setDataDefinedProperty( mShapeSizeTypeDDBtn, QgsPalLayerSettings::ShapeSizeType, lyr );
  setDataDefinedProperty( mShapeSizeXDDBtn, QgsPalLayerSettings::ShapeSizeX, lyr );
  setDataDefinedProperty( mShapeSizeYDDBtn, QgsPalLayerSettings::ShapeSizeY, lyr );
  setDataDefinedProperty( mShapeSizeUnitsDDBtn, QgsPalLayerSettings::ShapeSizeUnits, lyr );
  setDataDefinedProperty( mShapeRotationTypeDDBtn, QgsPalLayerSettings::ShapeRotationType, lyr );
  setDataDefinedProperty( mShapeRotationDDBtn, QgsPalLayerSettings::ShapeRotation, lyr );
  setDataDefinedProperty( mShapeOffsetDDBtn, QgsPalLayerSettings::ShapeOffset, lyr );
  setDataDefinedProperty( mShapeOffsetUnitsDDBtn, QgsPalLayerSettings::ShapeOffsetUnits, lyr );
  setDataDefinedProperty( mShapeRadiusDDBtn, QgsPalLayerSettings::ShapeRadii, lyr );
  setDataDefinedProperty( mShapeRadiusUnitsDDBtn, QgsPalLayerSettings::ShapeRadiiUnits, lyr );
  setDataDefinedProperty( mShapeTranspDDBtn, QgsPalLayerSettings::ShapeTransparency, lyr );
  setDataDefinedProperty( mShapeBlendModeDDBtn, QgsPalLayerSettings::ShapeBlendMode, lyr );
  setDataDefinedProperty( mShapeFillColorDDBtn, QgsPalLayerSettings::ShapeFillColor, lyr );
  setDataDefinedProperty( mShapeBorderColorDDBtn, QgsPalLayerSettings::ShapeBorderColor, lyr );
  setDataDefinedProperty( mShapeBorderWidthDDBtn, QgsPalLayerSettings::ShapeBorderWidth, lyr );
  setDataDefinedProperty( mShapeBorderUnitsDDBtn, QgsPalLayerSettings::ShapeBorderWidthUnits, lyr );
  setDataDefinedProperty( mShapePenStyleDDBtn, QgsPalLayerSettings::ShapeJoinStyle, lyr );

  // drop shadow
  setDataDefinedProperty( mShadowDrawDDBtn, QgsPalLayerSettings::ShadowDraw, lyr );
  setDataDefinedProperty( mShadowUnderDDBtn, QgsPalLayerSettings::ShadowUnder, lyr );
  setDataDefinedProperty( mShadowOffsetAngleDDBtn, QgsPalLayerSettings::ShadowOffsetAngle, lyr );
  setDataDefinedProperty( mShadowOffsetDDBtn, QgsPalLayerSettings::ShadowOffsetDist, lyr );
  setDataDefinedProperty( mShadowOffsetUnitsDDBtn, QgsPalLayerSettings::ShadowOffsetUnits, lyr );
  setDataDefinedProperty( mShadowRadiusDDBtn, QgsPalLayerSettings::ShadowRadius, lyr );
  setDataDefinedProperty( mShadowRadiusUnitsDDBtn, QgsPalLayerSettings::ShadowRadiusUnits, lyr );
  setDataDefinedProperty( mShadowTranspDDBtn, QgsPalLayerSettings::ShadowTransparency, lyr );
  setDataDefinedProperty( mShadowScaleDDBtn, QgsPalLayerSettings::ShadowScale, lyr );
  setDataDefinedProperty( mShadowColorDDBtn, QgsPalLayerSettings::ShadowColor, lyr );
  setDataDefinedProperty( mShadowBlendDDBtn, QgsPalLayerSettings::ShadowBlendMode, lyr );

  // placement
  setDataDefinedProperty( mCentroidDDBtn, QgsPalLayerSettings::CentroidWhole, lyr );
  setDataDefinedProperty( mPointQuadOffsetDDBtn, QgsPalLayerSettings::OffsetQuad, lyr );
  setDataDefinedProperty( mPointOffsetDDBtn, QgsPalLayerSettings::OffsetXY, lyr );
  setDataDefinedProperty( mPointOffsetUnitsDDBtn, QgsPalLayerSettings::OffsetUnits, lyr );
  setDataDefinedProperty( mLineDistanceDDBtn, QgsPalLayerSettings::LabelDistance, lyr );
  setDataDefinedProperty( mLineDistanceUnitDDBtn, QgsPalLayerSettings::DistanceUnits, lyr );
  // TODO: is this necessary? maybe just use the data defined-only rotation?
  //setDataDefinedProperty( mPointAngleDDBtn, QgsPalLayerSettings::OffsetRotation, lyr );
  setDataDefinedProperty( mMaxCharAngleDDBtn, QgsPalLayerSettings::CurvedCharAngleInOut, lyr );
  setDataDefinedProperty( mRepeatDistanceDDBtn, QgsPalLayerSettings::RepeatDistance, lyr );
  setDataDefinedProperty( mRepeatDistanceUnitDDBtn, QgsPalLayerSettings::RepeatDistanceUnit, lyr );

  // data defined-only
  setDataDefinedProperty( mCoordXDDBtn, QgsPalLayerSettings::PositionX, lyr );
  setDataDefinedProperty( mCoordYDDBtn, QgsPalLayerSettings::PositionY, lyr );
  setDataDefinedProperty( mCoordAlignmentHDDBtn, QgsPalLayerSettings::Hali, lyr );
  setDataDefinedProperty( mCoordAlignmentVDDBtn, QgsPalLayerSettings::Vali, lyr );
  setDataDefinedProperty( mCoordRotationDDBtn, QgsPalLayerSettings::Rotation, lyr );

  // rendering
  setDataDefinedProperty( mScaleBasedVisibilityDDBtn, QgsPalLayerSettings::ScaleVisibility, lyr );
  setDataDefinedProperty( mScaleBasedVisibilityMinDDBtn, QgsPalLayerSettings::MinScale, lyr );
  setDataDefinedProperty( mScaleBasedVisibilityMaxDDBtn, QgsPalLayerSettings::MaxScale, lyr );
  setDataDefinedProperty( mFontLimitPixelDDBtn, QgsPalLayerSettings::FontLimitPixel, lyr );
  setDataDefinedProperty( mFontMinPixelDDBtn, QgsPalLayerSettings::FontMinPixel, lyr );
  setDataDefinedProperty( mFontMaxPixelDDBtn, QgsPalLayerSettings::FontMaxPixel, lyr );
  setDataDefinedProperty( mShowLabelDDBtn, QgsPalLayerSettings::Show, lyr );
  setDataDefinedProperty( mAlwaysShowDDBtn, QgsPalLayerSettings::AlwaysShow, lyr );

  return lyr;
}

void QgsLabelingGui::setDataDefinedProperty( const QgsDataDefinedButton* ddBtn, QgsPalLayerSettings::DataDefinedProperties p, QgsPalLayerSettings& lyr )
{
  const QMap< QString, QString >& map = ddBtn->definedProperty();
  lyr.setDataDefinedProperty( p, map.value( "active" ).toInt(), map.value( "useexpr" ).toInt(), map.value( "expression" ), map.value( "field" ) );
}

void QgsLabelingGui::populateDataDefinedButtons( QgsPalLayerSettings& s )
{
  // don't register enable/disable siblings, since visual feedback from data defined buttons should be enough,
  // and ability to edit layer-level setting should remain enabled regardless

  QString trString = tr( "string " );

  // text style
  mFontDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Family ),
                    QgsDataDefinedButton::String,
                    trString + tr( "[<b>family</b>|<b>family[foundry]</b>],<br>"
                                   "e.g. Helvetica or Helvetica [Cronyx]" ) );

  mFontStyleDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontStyle ),
                         QgsDataDefinedButton::String,
                         trString + tr( "[<b>font style name</b>|<b>Ignore</b>],<br>"
                                        "e.g. Bold Condensed or Light Italic" ) );

  mFontUnderlineDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Underline ),
                             QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );

  mFontStrikeoutDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Strikeout ),
                             QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );

  mFontBoldDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Bold ),
                        QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );

  mFontItalicDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Italic ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );

  mFontSizeDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Size ),
                        QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doublePosDesc() );

  mFontUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontSizeUnit ),
                         QgsDataDefinedButton::String, trString + "[<b>Points</b>|<b>MapUnit</b>]" );

  mFontColorDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Color ),
                         QgsDataDefinedButton::String, QgsDataDefinedButton::colorNoAlphaDesc() );

  mFontTranspDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontTransp ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::intTranspDesc() );

  mFontCaseDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontCase ),
                        QgsDataDefinedButton::String,
                        trString + QString( "[<b>NoChange</b>|<b>Upper</b>|<br>"
                                            "<b>Lower</b>|<b>Capitalize</b>]" ) );

  mFontLetterSpacingDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontLetterSpacing ),
                                 QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleDesc() );
  mFontWordSpacingDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontWordSpacing ),
                               QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleDesc() );

  mFontBlendModeDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontBlendMode ),
                             QgsDataDefinedButton::String, QgsDataDefinedButton::blendModesDesc() );

  // text formatting
  mWrapCharDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::MultiLineWrapChar ),
                        QgsDataDefinedButton::String, QgsDataDefinedButton::anyStringDesc() );
  mFontLineHeightDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::MultiLineHeight ),
                              QgsDataDefinedButton::AnyType, tr( "double [0.0-10.0]" ) );
  mFontMultiLineAlignDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::MultiLineAlignment ),
                                  QgsDataDefinedButton::String, QgsDataDefinedButton::textHorzAlignDesc() );

  mDirectSymbDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::DirSymbDraw ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );
  mDirectSymbDDBtn->registerCheckedWidget( mDirectSymbChkBx );
  mDirectSymbLeftDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::DirSymbLeft ),
                              QgsDataDefinedButton::String, QgsDataDefinedButton::anyStringDesc() );
  mDirectSymbRightDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::DirSymbRight ),
                               QgsDataDefinedButton::String, QgsDataDefinedButton::anyStringDesc() );

  mDirectSymbPlacementDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::DirSymbPlacement ),
                                   QgsDataDefinedButton::String,
                                   trString + "[<b>LeftRight</b>|<b>Above</b>|<b>Below</b>]" );
  mDirectSymbRevDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::DirSymbReverse ),
                             QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );

  mFormatNumDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::NumFormat ),
                         QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );
  mFormatNumDDBtn->registerCheckedWidget( mFormatNumChkBx );
  mFormatNumDecimalsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::NumDecimals ),
                                 QgsDataDefinedButton::AnyType, tr( "int [0-20]" ) );
  mFormatNumPlusSignDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::NumPlusSign ),
                                 QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );

  // text buffer
  mBufferDrawDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::BufferDraw ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );
  mBufferDrawDDBtn->registerCheckedWidget( mBufferDrawChkBx );
  mBufferSizeDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::BufferSize ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doublePosDesc() );
  mBufferUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::BufferUnit ),
                           QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );
  mBufferColorDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::BufferColor ),
                           QgsDataDefinedButton::String, QgsDataDefinedButton::colorNoAlphaDesc() );
  mBufferTranspDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::BufferTransp ),
                            QgsDataDefinedButton::AnyType, QgsDataDefinedButton::intTranspDesc() );
  mBufferJoinStyleDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::BufferJoinStyle ),
                               QgsDataDefinedButton::String, QgsDataDefinedButton::penJoinStyleDesc() );
  mBufferBlendModeDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::BufferBlendMode ),
                               QgsDataDefinedButton::String, QgsDataDefinedButton::blendModesDesc() );

  // background
  mShapeDrawDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeDraw ),
                         QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );
  mShapeDrawDDBtn->registerCheckedWidget( mShapeDrawChkBx );
  mShapeTypeDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeKind ),
                         QgsDataDefinedButton::String,
                         trString + QString( "[<b>Rectangle</b>|<b>Square</b>|<br>"
                                             "<b>Ellipse</b>|<b>Circle</b>|<b>SVG</b>]" ) );
  mShapeSVGPathDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeSVGFile ),
                            QgsDataDefinedButton::String, QgsDataDefinedButton::svgPathDesc() );
  mShapeSizeTypeDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeSizeType ),
                             QgsDataDefinedButton::String,
                             trString + "[<b>Buffer</b>|<b>Fixed</b>]" );
  mShapeSizeXDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeSizeX ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleDesc() );
  mShapeSizeYDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeSizeY ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleDesc() );
  mShapeSizeUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeSizeUnits ),
                              QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );
  mShapeRotationTypeDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeRotationType ),
                                 QgsDataDefinedButton::String,
                                 trString + "[<b>Sync</b>|<b>Offset</b>|<b>Fixed</b>]" );
  mShapeRotationDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeRotation ),
                             QgsDataDefinedButton::AnyType, QgsDataDefinedButton::double180RotDesc() );
  mShapeOffsetDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeOffset ),
                           QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleXYDesc() );
  mShapeOffsetUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeOffsetUnits ),
                                QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );
  mShapeRadiusDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeRadii ),
                           QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleXYDesc() );
  mShapeRadiusUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeRadiiUnits ),
                                QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuPercentDesc() );
  mShapeTranspDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeTransparency ),
                           QgsDataDefinedButton::AnyType, QgsDataDefinedButton::intTranspDesc() );
  mShapeBlendModeDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeBlendMode ),
                              QgsDataDefinedButton::String, QgsDataDefinedButton::blendModesDesc() );
  mShapeFillColorDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeFillColor ),
                              QgsDataDefinedButton::String, QgsDataDefinedButton::colorAlphaDesc() );
  mShapeBorderColorDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeBorderColor ),
                                QgsDataDefinedButton::String, QgsDataDefinedButton::colorAlphaDesc() );
  mShapeBorderWidthDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeBorderWidth ),
                                QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doublePosDesc() );
  mShapeBorderUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeBorderWidthUnits ),
                                QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );
  mShapePenStyleDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShapeJoinStyle ),
                             QgsDataDefinedButton::String, QgsDataDefinedButton::penJoinStyleDesc() );

  // drop shadows
  mShadowDrawDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowDraw ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );
  mShadowDrawDDBtn->registerCheckedWidget( mShadowDrawChkBx );
  mShadowUnderDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowUnder ),
                           QgsDataDefinedButton::String,
                           trString + QString( "[<b>Lowest</b>|<b>Text</b>|<br>"
                                               "<b>Buffer</b>|<b>Background</b>]" ) );
  mShadowOffsetAngleDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowOffsetAngle ),
                                 QgsDataDefinedButton::AnyType, QgsDataDefinedButton::double180RotDesc() );
  mShadowOffsetDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowOffsetDist ),
                            QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doublePosDesc() );
  mShadowOffsetUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowOffsetUnits ),
                                 QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );
  mShadowRadiusDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowRadius ),
                            QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doublePosDesc() );
  mShadowRadiusUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowRadiusUnits ),
                                 QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );
  mShadowTranspDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowTransparency ),
                            QgsDataDefinedButton::AnyType, QgsDataDefinedButton::intTranspDesc() );
  mShadowScaleDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowScale ),
                           QgsDataDefinedButton::AnyType, tr( "int [0-2000]" ) );
  mShadowColorDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowColor ),
                           QgsDataDefinedButton::String, QgsDataDefinedButton::colorNoAlphaDesc() );
  mShadowBlendDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ShadowBlendMode ),
                           QgsDataDefinedButton::String, QgsDataDefinedButton::blendModesDesc() );

  // placement
  mCentroidDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::CentroidWhole ),
                        QgsDataDefinedButton::String,
                        trString + "[<b>Visible</b>|<b>Whole</b>]" );
  mPointQuadOffsetDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::OffsetQuad ),
                               QgsDataDefinedButton::AnyType,
                               tr( "int<br>" ) + QString( "[<b>0</b>=Above Left|<b>1</b>=Above|<b>2</b>=Above Right|<br>"
                                                          "<b>3</b>=Left|<b>4</b>=Over|<b>5</b>=Right|<br>"
                                                          "<b>6</b>=Below Left|<b>7</b>=Below|<b>8</b>=Below Right]" ) );
  mPointOffsetDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::OffsetXY ),
                           QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleXYDesc() );
  mPointOffsetUnitsDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::OffsetUnits ),
                                QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );
  mLineDistanceDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::LabelDistance ),
                            QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doublePosDesc() );
  mLineDistanceUnitDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::DistanceUnits ),
                                QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );
  // TODO: is this necessary? maybe just use the data defined-only rotation?
  //mPointAngleDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::OffsetRotation ),
  //                        QgsDataDefinedButton::AnyType, QgsDataDefinedButton::double180RotDesc() );
  mMaxCharAngleDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::CurvedCharAngleInOut ),
                            QgsDataDefinedButton::AnyType, tr( "double coord [<b>in,out</b> as 20.0-60.0,20.0-95.0]" ) );
  mRepeatDistanceDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::RepeatDistance ),
                              QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doublePosDesc() );
  mRepeatDistanceUnitDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::RepeatDistanceUnit ),
                                  QgsDataDefinedButton::String, QgsDataDefinedButton::unitsMmMuDesc() );

  // data defined-only
  QString ddPlaceInfo = tr( "In edit mode, layer's relevant labeling map tool is:<br>"
                            "&nbsp;&nbsp;Defined attribute field -&gt; <i>enabled</i><br>"
                            "&nbsp;&nbsp;Defined expression -&gt; <i>disabled</i>" );
  mCoordXDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::PositionX ),
                      QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleDesc() );
  mCoordXDDBtn->setUsageInfo( ddPlaceInfo );
  mCoordYDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::PositionY ),
                      QgsDataDefinedButton::AnyType, QgsDataDefinedButton::doubleDesc() );
  mCoordYDDBtn->setUsageInfo( ddPlaceInfo );
  mCoordAlignmentHDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Hali ),
                               QgsDataDefinedButton::String,
                               trString + "[<b>Left</b>|<b>Center</b>|<b>Right</b>]" );
  mCoordAlignmentHDDBtn->setUsageInfo( ddPlaceInfo );
  mCoordAlignmentVDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Vali ),
                               QgsDataDefinedButton::String,
                               trString + QString( "[<b>Bottom</b>|<b>Base</b>|<br>"
                                                   "<b>Half</b>|<b>Cap</b>|<b>Top</b>]" ) );
  mCoordAlignmentVDDBtn->setUsageInfo( ddPlaceInfo );
  mCoordRotationDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Rotation ),
                             QgsDataDefinedButton::AnyType, QgsDataDefinedButton::double180RotDesc() );
  mCoordRotationDDBtn->setUsageInfo( ddPlaceInfo );

  // rendering
  QString ddScaleVisInfo = tr( "Value &lt; 0 represents a scale closer than 1:1, e.g. -10 = 10:1<br>"
                               "Value of 0 disables the specific limit." );
  mScaleBasedVisibilityDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::ScaleVisibility ),
                                    QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );
  mScaleBasedVisibilityDDBtn->registerCheckedWidget( mScaleBasedVisibilityChkBx );
  mScaleBasedVisibilityMinDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::MinScale ),
                                       QgsDataDefinedButton::AnyType, QgsDataDefinedButton::intDesc() );
  mScaleBasedVisibilityMinDDBtn->setUsageInfo( ddScaleVisInfo );
  mScaleBasedVisibilityMaxDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::MaxScale ),
                                       QgsDataDefinedButton::AnyType, QgsDataDefinedButton::intDesc() );
  mScaleBasedVisibilityMaxDDBtn->setUsageInfo( ddScaleVisInfo );

  mFontLimitPixelDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontLimitPixel ),
                              QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );
  mFontLimitPixelDDBtn->registerCheckedWidget( mFontLimitPixelChkBox );
  mFontMinPixelDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontMinPixel ),
                            QgsDataDefinedButton::AnyType, tr( "int [1-1000]" ) );
  mFontMaxPixelDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::FontMaxPixel ),
                            QgsDataDefinedButton::AnyType, tr( "int [1-10000]" ) );

  mShowLabelDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::Show ),
                         QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );

  mAlwaysShowDDBtn->init( mLayer, s.dataDefinedProperty( QgsPalLayerSettings::AlwaysShow ),
                          QgsDataDefinedButton::AnyType, QgsDataDefinedButton::boolDesc() );
}

void QgsLabelingGui::changeTextColor( const QColor &color )
{
  Q_UNUSED( color )
  updatePreview();
}

void QgsLabelingGui::updateFont( QFont font )
{
  // update background reference font
  if ( font != mRefFont )
  {
    mRefFont = font;
  }

  // test if font is actually available
  // NOTE: QgsFontUtils::fontMatchOnSystem may fail here, just crosscheck family
  mFontMissingLabel->setVisible( !QgsFontUtils::fontFamilyMatchOnSystem( mRefFont.family() ) );

  mDirectSymbLeftLineEdit->setFont( mRefFont );
  mDirectSymbRightLineEdit->setFont( mRefFont );

  blockFontChangeSignals( true );
  mFontFamilyCmbBx->setCurrentFont( mRefFont );
  populateFontStyleComboBox();
  int idx = mFontCapitalsComboBox->findData( QVariant(( unsigned int ) mRefFont.capitalization() ) );
  mFontCapitalsComboBox->setCurrentIndex( idx == -1 ? 0 : idx );
  mFontUnderlineBtn->setChecked( mRefFont.underline() );
  mFontStrikethroughBtn->setChecked( mRefFont.strikeOut() );
  blockFontChangeSignals( false );

  // update font name with font face
//  font.setPixelSize( 24 );

  updatePreview();
}

void QgsLabelingGui::blockFontChangeSignals( bool blk )
{
  mFontFamilyCmbBx->blockSignals( blk );
  mFontStyleComboBox->blockSignals( blk );
  mFontCapitalsComboBox->blockSignals( blk );
  mFontUnderlineBtn->blockSignals( blk );
  mFontStrikethroughBtn->blockSignals( blk );
  mFontWordSpacingSpinBox->blockSignals( blk );
  mFontLetterSpacingSpinBox->blockSignals( blk );
}

void QgsLabelingGui::updatePreview()
{
  scrollPreview();
  lblFontPreview->setFont( mRefFont );
  QFont previewFont = lblFontPreview->font();
  double fontSize = mFontSizeSpinBox->value();
  double previewRatio = mPreviewSize / fontSize;
  double bufferSize = 0.0;
  QString grpboxtitle;
  QString sampleTxt = tr( "Text/Buffer sample" );

  if ( mFontSizeUnitWidget->getUnit() == 1 ) // map units
  {
    // TODO: maybe match current map zoom level instead?
    previewFont.setPointSize( mPreviewSize );
    mPreviewSizeSlider->setEnabled( true );
    grpboxtitle = sampleTxt + tr( " @ %1 pts (using map units)" ).arg( mPreviewSize );

    previewFont.setWordSpacing( previewRatio * mFontWordSpacingSpinBox->value() );
    previewFont.setLetterSpacing( QFont::AbsoluteSpacing, previewRatio * mFontLetterSpacingSpinBox->value() );

    if ( mBufferDrawChkBx->isChecked() )
    {
      if ( mBufferUnitWidget->getUnit() == 1 ) // map units
      {
        bufferSize = previewRatio * spinBufferSize->value() / 3.527;
      }
      else // millimeters
      {
        grpboxtitle = sampleTxt + tr( " @ %1 pts (using map units, BUFFER IN MILLIMETERS)" ).arg( mPreviewSize );
        bufferSize = spinBufferSize->value();
      }
    }
  }
  else // in points
  {
    if ( fontSize > 0 )
      previewFont.setPointSize( fontSize );
    mPreviewSizeSlider->setEnabled( false );
    grpboxtitle = sampleTxt;

    if ( mBufferDrawChkBx->isChecked() )
    {
      if ( mBufferUnitWidget->getUnit() == 0 ) // millimeters
      {
        bufferSize = spinBufferSize->value();
      }
      else // map units
      {
        grpboxtitle = sampleTxt + tr( " (BUFFER NOT SHOWN, in map units)" );
      }
    }
  }

  lblFontPreview->setFont( previewFont );
  groupBox_mPreview->setTitle( grpboxtitle );

  QColor prevColor = btnTextColor->color();
  prevColor.setAlphaF(( 100.0 - ( double )( mFontTranspSpinBox->value() ) ) / 100.0 );
  lblFontPreview->setTextColor( prevColor );

  bool bufferNoFill = false;
  if ( mBufferDrawChkBx->isChecked() && bufferSize != 0.0 )
  {
    QColor buffColor = btnBufferColor->color();
    buffColor.setAlphaF(( 100.0 - ( double )( mBufferTranspSpinBox->value() ) ) / 100.0 );

    bufferNoFill = !mBufferTranspFillChbx->isChecked();
    lblFontPreview->setBuffer( bufferSize, buffColor, mBufferJoinStyleComboBox->penJoinStyle(), bufferNoFill );
  }
  else
  {
    lblFontPreview->setBuffer( 0, Qt::white, Qt::BevelJoin, bufferNoFill );
  }
}

void QgsLabelingGui::scrollPreview()
{
  scrollArea_mPreview->ensureVisible( 0, 0, 0, 0 );
}

void QgsLabelingGui::setPreviewBackground( QColor color )
{
  scrollArea_mPreview->widget()->setStyleSheet( QString( "background: rgb(%1, %2, %3);" ).arg( QString::number( color.red() ),
      QString::number( color.green() ),
      QString::number( color.blue() ) ) );
}

void QgsLabelingGui::showEngineConfigDialog()
{
  QgsLabelEngineConfigDialog dlg( this );
  dlg.exec();
}

void QgsLabelingGui::syncDefinedCheckboxFrame( QgsDataDefinedButton* ddBtn, QCheckBox* chkBx, QFrame* f )
{
  if ( ddBtn->isActive() && !chkBx->isChecked() )
  {
    chkBx->setChecked( true );
  }
  f->setEnabled( chkBx->isChecked() );
}

void QgsLabelingGui::updateUi()
{
  // enable/disable inline groupbox-like setups (that need to honor data defined setting)

  syncDefinedCheckboxFrame( mBufferDrawDDBtn, mBufferDrawChkBx, mBufferFrame );
  syncDefinedCheckboxFrame( mShapeDrawDDBtn, mShapeDrawChkBx, mShapeFrame );
  syncDefinedCheckboxFrame( mShadowDrawDDBtn, mShadowDrawChkBx, mShadowFrame );

  syncDefinedCheckboxFrame( mDirectSymbDDBtn, mDirectSymbChkBx, mDirectSymbFrame );
  syncDefinedCheckboxFrame( mFormatNumDDBtn, mFormatNumChkBx, mFormatNumFrame );
  syncDefinedCheckboxFrame( mScaleBasedVisibilityDDBtn, mScaleBasedVisibilityChkBx, mScaleBasedVisibilityFrame );
  syncDefinedCheckboxFrame( mFontLimitPixelDDBtn, mFontLimitPixelChkBox, mFontLimitPixelFrame );
}

void QgsLabelingGui::changeBufferColor( const QColor &color )
{
  Q_UNUSED( color )
  updatePreview();
}

void QgsLabelingGui::updatePlacementWidgets()
{
  QWidget* curWdgt = stackedPlacement->currentWidget();

  bool showLineFrame = false;
  bool showCentroidFrame = false;
  bool showQuadrantFrame = false;
  bool showOffsetFrame = false;
  bool showDistanceFrame = false;
  bool showRotationFrame = false;
  bool showMaxCharAngleFrame = false;

  bool enableMultiLinesFrame = true;

  if (( curWdgt == pagePoint && radAroundPoint->isChecked() )
      || ( curWdgt == pagePolygon && radAroundCentroid->isChecked() ) )
  {
    showCentroidFrame = ( curWdgt == pagePolygon && radAroundCentroid->isChecked() );
    showDistanceFrame = true;
    //showRotationFrame = true; // TODO: uncomment when supported
  }
  else if (( curWdgt == pagePoint && radOverPoint->isChecked() )
           || ( curWdgt == pagePolygon && radOverCentroid->isChecked() ) )
  {
    showCentroidFrame = ( curWdgt == pagePolygon && radOverCentroid->isChecked() );
    showQuadrantFrame = true;
    showOffsetFrame = true;
    showRotationFrame = true;
  }
  else if (( curWdgt == pageLine && radLineParallel->isChecked() )
           || ( curWdgt == pagePolygon && radPolygonPerimeter->isChecked() )
           || ( curWdgt == pageLine && radLineCurved->isChecked() ) )
  {
    showLineFrame = true;
    showDistanceFrame = true;
    //showRotationFrame = true; // TODO: uncomment when supported

    bool offline = chkLineAbove->isChecked() || chkLineBelow->isChecked();
    chkLineOrientationDependent->setEnabled( offline );
    mPlacementDistanceFrame->setEnabled( offline );

    showMaxCharAngleFrame = ( curWdgt == pageLine && radLineCurved->isChecked() );
    // TODO: enable mMultiLinesFrame when supported for curved labels
    enableMultiLinesFrame = !( curWdgt == pageLine && radLineCurved->isChecked() );
  }

  mPlacementLineFrame->setVisible( showLineFrame );
  mPlacementCentroidFrame->setVisible( showCentroidFrame );
  mPlacementQuadrantFrame->setVisible( showQuadrantFrame );
  mPlacementOffsetFrame->setVisible( showOffsetFrame );
  mPlacementDistanceFrame->setVisible( showDistanceFrame );
  mPlacementRotationFrame->setVisible( showRotationFrame );
  mPlacementRepeatDistanceFrame->setVisible( curWdgt == pageLine || ( curWdgt == pagePolygon && radPolygonPerimeter->isChecked() ) );
  mPlacementMaxCharAngleFrame->setVisible( showMaxCharAngleFrame );

  mMultiLinesFrame->setEnabled( enableMultiLinesFrame );
}

void QgsLabelingGui::populateFontCapitalsComboBox()
{
  mFontCapitalsComboBox->addItem( tr( "No change" ), QVariant( 0 ) );
  mFontCapitalsComboBox->addItem( tr( "All uppercase" ), QVariant( 1 ) );
  mFontCapitalsComboBox->addItem( tr( "All lowercase" ), QVariant( 2 ) );
  // Small caps doesn't work right with QPainterPath::addText()
  // https://bugreports.qt-project.org/browse/QTBUG-13965
//  mFontCapitalsComboBox->addItem( tr( "Small caps" ), QVariant( 3 ) );
  mFontCapitalsComboBox->addItem( tr( "Capitalize first letter" ), QVariant( 4 ) );
}

void QgsLabelingGui::populateFontStyleComboBox()
{
  mFontStyleComboBox->clear();
  foreach ( const QString &style, mFontDB.styles( mRefFont.family() ) )
  {
    mFontStyleComboBox->addItem( style );
  }

  int curIndx = 0;
  int stylIndx = mFontStyleComboBox->findText( mFontDB.styleString( mRefFont ) );
  if ( stylIndx > -1 )
  {
    curIndx = stylIndx;
  }

  mFontStyleComboBox->setCurrentIndex( curIndx );
}

void QgsLabelingGui::on_mPreviewSizeSlider_valueChanged( int i )
{
  mPreviewSize = i;
  updatePreview();
}

void QgsLabelingGui::on_mFontSizeSpinBox_valueChanged( double d )
{
  mRefFont.setPointSizeF( d );
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontCapitalsComboBox_currentIndexChanged( int index )
{
  int capitalsindex = mFontCapitalsComboBox->itemData( index ).toUInt();
  mRefFont.setCapitalization(( QFont::Capitalization ) capitalsindex );
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontFamilyCmbBx_currentFontChanged( const QFont& f )
{
  mRefFont.setFamily( f.family() );
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontStyleComboBox_currentIndexChanged( const QString & text )
{
  QgsFontUtils::updateFontViaStyle( mRefFont, text );
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontUnderlineBtn_toggled( bool ckd )
{
  mRefFont.setUnderline( ckd );
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontStrikethroughBtn_toggled( bool ckd )
{
  mRefFont.setStrikeOut( ckd );
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontWordSpacingSpinBox_valueChanged( double spacing )
{
  mRefFont.setWordSpacing( spacing );
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontLetterSpacingSpinBox_valueChanged( double spacing )
{
  mRefFont.setLetterSpacing( QFont::AbsoluteSpacing, spacing );
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontSizeUnitWidget_changed()
{
  int index = mFontSizeUnitWidget->getUnit();
  // disable pixel size limiting for labels defined in points
  if ( index == 0 )
  {
    mFontLimitPixelChkBox->setChecked( false );
  }
  else if ( index == 1 && mMinPixelLimit == 0 )
  {
    // initial minimum trigger value set, turn on pixel size limiting by default
    // for labels defined in map units (ignored after first settings save)
    mFontLimitPixelChkBox->setChecked( true );
  }
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mFontMinPixelSpinBox_valueChanged( int px )
{
  // ensure max font pixel size for map unit labels can't be lower than min
  mFontMaxPixelSpinBox->setMinimum( px );
  mFontMaxPixelSpinBox->update();
}

void QgsLabelingGui::on_mFontMaxPixelSpinBox_valueChanged( int px )
{
  // ensure max font pixel size for map unit labels can't be lower than min
  if ( px < mFontMinPixelSpinBox->value() )
  {
    mFontMaxPixelSpinBox->blockSignals( true );
    mFontMaxPixelSpinBox->setValue( mFontMinPixelSpinBox->value() );
    mFontMaxPixelSpinBox->blockSignals( false );
  }
  mFontMaxPixelSpinBox->setMinimum( mFontMinPixelSpinBox->value() );
}

void QgsLabelingGui::on_mBufferUnitWidget_changed()
{
  updateFont( mRefFont );
}

void QgsLabelingGui::on_mCoordXDDBtn_dataDefinedActivated( bool active )
{
  if ( !active ) //no data defined alignment without data defined position
  {
    enableDataDefinedAlignment( false );
  }
  else if ( mCoordYDDBtn->isActive() )
  {
    enableDataDefinedAlignment( true );
  }
}

void QgsLabelingGui::on_mCoordYDDBtn_dataDefinedActivated( bool active )
{
  if ( !active ) //no data defined alignment without data defined position
  {
    enableDataDefinedAlignment( false );
  }
  else if ( mCoordXDDBtn->isActive() )
  {
    enableDataDefinedAlignment( true );
  }
}

void QgsLabelingGui::on_mShapeTypeCmbBx_currentIndexChanged( int index )
{
  // shape background
  bool isRect = (( QgsPalLayerSettings::ShapeType )index == QgsPalLayerSettings::ShapeRectangle
                 || ( QgsPalLayerSettings::ShapeType )index == QgsPalLayerSettings::ShapeSquare );
  bool isSVG = (( QgsPalLayerSettings::ShapeType )index == QgsPalLayerSettings::ShapeSVG );

  showBackgroundPenStyle( isRect );
  showBackgroundRadius( isRect );

  mShapeSVGPathFrame->setVisible( isSVG );
  // symbology SVG renderer only supports size^2 scaling, so we only use the x size spinbox
  mShapeSizeYLabel->setVisible( !isSVG );
  mShapeSizeYSpnBx->setVisible( !isSVG );
  mShapeSizeYDDBtn->setVisible( !isSVG );
  mShapeSizeXLabel->setText( tr( "Size%1" ).arg( !isSVG ? tr( " X" ) : "" ) );

  // SVG parameter setting doesn't support color's alpha component yet
  mShapeFillColorBtn->setAllowAlpha( !isSVG );
  mShapeFillColorBtn->setButtonBackground();
  mShapeBorderColorBtn->setAllowAlpha( !isSVG );
  mShapeBorderColorBtn->setButtonBackground();

  // configure SVG parameter widgets
  mShapeSVGParamsBtn->setVisible( isSVG );
  if ( isSVG )
  {
    updateSvgWidgets( mShapeSVGPathLineEdit->text() );
  }
  else
  {
    mShapeFillColorLabel->setEnabled( true );
    mShapeFillColorBtn->setEnabled( true );
    mShapeFillColorDDBtn->setEnabled( true );
    mShapeBorderColorLabel->setEnabled( true );
    mShapeBorderColorBtn->setEnabled( true );
    mShapeBorderColorDDBtn->setEnabled( true );
    mShapeBorderWidthLabel->setEnabled( true );
    mShapeBorderWidthSpnBx->setEnabled( true );
    mShapeBorderWidthDDBtn->setEnabled( true );
  }
  // TODO: fix overriding SVG symbol's border width units in QgsSvgCache
  // currently broken, fall back to symbol units only
  mShapeBorderWidthUnitWidget->setVisible( !isSVG );
  mShapeSVGUnitsLabel->setVisible( isSVG );
  mShapeBorderUnitsDDBtn->setEnabled( !isSVG );
}

void QgsLabelingGui::on_mShapeSVGPathLineEdit_textChanged( const QString& text )
{
  updateSvgWidgets( text );
}

void QgsLabelingGui::updateSvgWidgets( const QString& svgPath )
{
  if ( mShapeSVGPathLineEdit->text() != svgPath )
  {
    mShapeSVGPathLineEdit->setText( svgPath );
  }

  QString resolvedPath = QgsSymbolLayerV2Utils::symbolNameToPath( svgPath );
  bool validSVG = !resolvedPath.isNull();

  // draw red text for path field if invalid (path can't be resolved)
  mShapeSVGPathLineEdit->setStyleSheet( QString( !validSVG ? "QLineEdit{ color: rgb(225, 0, 0); }" : "" ) );
  mShapeSVGPathLineEdit->setToolTip( !validSVG ? tr( "File not found" ) : resolvedPath );

  QColor fill, outline;
  double outlineWidth = 0.0;
  bool fillParam = false, outlineParam = false, outlineWidthParam = false;
  if ( validSVG )
  {
    QgsSvgCache::instance()->containsParams( resolvedPath, fillParam, fill, outlineParam, outline, outlineWidthParam, outlineWidth );
  }

  mShapeSVGParamsBtn->setEnabled( validSVG && ( fillParam || outlineParam || outlineWidthParam ) );

  mShapeFillColorLabel->setEnabled( validSVG && fillParam );
  mShapeFillColorBtn->setEnabled( validSVG && fillParam );
  mShapeFillColorDDBtn->setEnabled( validSVG && fillParam );
  if ( mLoadSvgParams && validSVG && fillParam )
    mShapeFillColorBtn->setColor( fill );

  mShapeBorderColorLabel->setEnabled( validSVG && outlineParam );
  mShapeBorderColorBtn->setEnabled( validSVG && outlineParam );
  mShapeBorderColorDDBtn->setEnabled( validSVG && outlineParam );
  if ( mLoadSvgParams && validSVG && outlineParam )
    mShapeBorderColorBtn->setColor( outline );

  mShapeBorderWidthLabel->setEnabled( validSVG && outlineWidthParam );
  mShapeBorderWidthSpnBx->setEnabled( validSVG && outlineWidthParam );
  mShapeBorderWidthDDBtn->setEnabled( validSVG && outlineWidthParam );
  if ( mLoadSvgParams && validSVG && outlineWidthParam )
    mShapeBorderWidthSpnBx->setValue( outlineWidth );

  // TODO: fix overriding SVG symbol's border width units in QgsSvgCache
  // currently broken, fall back to symbol's
  //mShapeBorderWidthUnitWidget->setEnabled( validSVG && outlineWidthParam );
  //mShapeBorderUnitsDDBtn->setEnabled( validSVG && outlineWidthParam );
  mShapeSVGUnitsLabel->setEnabled( validSVG && outlineWidthParam );
}

void QgsLabelingGui::on_mShapeSVGSelectorBtn_clicked()
{
  QgsSvgSelectorDialog svgDlg( this );
  svgDlg.svgSelector()->setSvgPath( mShapeSVGPathLineEdit->text().trimmed() );

  if ( svgDlg.exec() == QDialog::Accepted )
  {
    QString svgPath = svgDlg.svgSelector()->currentSvgPath();
    if ( !svgPath.isEmpty() )
    {
      mShapeSVGPathLineEdit->setText( svgPath );
    }
  }
}

void QgsLabelingGui::on_mShapeSVGParamsBtn_clicked()
{
  QString svgPath = mShapeSVGPathLineEdit->text();
  mLoadSvgParams = true;
  updateSvgWidgets( svgPath );
  mLoadSvgParams = false;
}

void QgsLabelingGui::on_mShapeRotationCmbBx_currentIndexChanged( int index )
{
  mShapeRotationDblSpnBx->setEnabled(( QgsPalLayerSettings::RotationType )index != QgsPalLayerSettings::RotationSync );
  mShapeRotationDDBtn->setEnabled(( QgsPalLayerSettings::RotationType )index != QgsPalLayerSettings::RotationSync );
}

void QgsLabelingGui::on_mPreviewTextEdit_textChanged( const QString & text )
{
  lblFontPreview->setText( text );
  updatePreview();
}

void QgsLabelingGui::on_mPreviewTextBtn_clicked()
{
  mPreviewTextEdit->setText( QString( "Lorem Ipsum" ) );
  updatePreview();
}

void QgsLabelingGui::on_mPreviewBackgroundBtn_colorChanged( const QColor &color )
{
  setPreviewBackground( color );
}

void QgsLabelingGui::on_mDirectSymbLeftToolBtn_clicked()
{
  bool gotChar = false;
  QChar dirSymb = mCharDlg->selectCharacter( &gotChar, mRefFont, mFontDB.styleString( mRefFont ) );

  if ( !gotChar )
    return;

  if ( !dirSymb.isNull() )
    mDirectSymbLeftLineEdit->setText( QString( dirSymb ) );
}

void QgsLabelingGui::on_mDirectSymbRightToolBtn_clicked()
{
  bool gotChar = false;
  QChar dirSymb = mCharDlg->selectCharacter( &gotChar, mRefFont, mFontDB.styleString( mRefFont ) );

  if ( !gotChar )
    return;

  if ( !dirSymb.isNull() )
    mDirectSymbRightLineEdit->setText( QString( dirSymb ) );
}

void QgsLabelingGui::showBackgroundRadius( bool show )
{
  mShapeRadiusLabel->setVisible( show );
  mShapeRadiusXDbSpnBx->setVisible( show );

  mShapeRadiusYDbSpnBx->setVisible( show );

  mShapeRadiusUnitWidget->setVisible( show );

  mShapeRadiusDDBtn->setVisible( show );
  mShapeRadiusUnitsDDBtn->setVisible( show );
}

void QgsLabelingGui::showBackgroundPenStyle( bool show )
{
  mShapePenStyleLabel->setVisible( show );
  mShapePenStyleCmbBx->setVisible( show );

  mShapePenStyleDDBtn->setVisible( show );
}

void QgsLabelingGui::enableDataDefinedAlignment( bool enable )
{
  mCoordAlignmentFrame->setEnabled( enable );
}


