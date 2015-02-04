/***************************************************************************
   qgsscalerangewidget.cpp
    --------------------------------------
   Date                 : 25.04.2014
   Copyright            : (C) 2014 Denis Rouzaud
   Email                : denis.rouzaud@gmail.com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "qgsscalerangewidget.h"
#include "qgsapplication.h"
#include "qgsproject.h"


QgsScaleRangeWidget::QgsScaleRangeWidget( QWidget *parent )
    : QWidget( parent )
    , mCanvas( 0 )
{
  mLayout = new QGridLayout( this );
  mLayout->setContentsMargins( 0, 0, 0, 0 );

  QLabel* minLbl = new QLabel( tr( "Minimum\n(exclusive)" ), this );
  minLbl->setWordWrap( true );
  minLbl->setAlignment( Qt::AlignTop );
  minLbl->setToolTip( tr( "Minimum scale, i.e. maximum scale denominator. "
                          "This limit is exclusive, that means the layer will not be displayed on this scale." ) );
  QLabel* maxLbl = new QLabel( tr( "Maximum\n(inclusive)" ), this );
  maxLbl->setWordWrap( true );
  maxLbl->setAlignment( Qt::AlignTop );
  maxLbl->setToolTip( tr( "Maximum scale, i.e. minimum scale denominator. "
                          "This limit is inclusive, that means the layer will be displayed on this scale." ) );

  mMinimumScaleIconLabel = new QLabel( this );
  mMinimumScaleIconLabel->setPixmap( QgsApplication::getThemePixmap( "/mActionZoomOut.svg" ) );
  mMaximumScaleIconLabel = new QLabel( this );
  mMaximumScaleIconLabel->setPixmap( QgsApplication::getThemePixmap( "/mActionZoomIn.svg" ) );

  mMinimumScaleWidget = new QgsScaleWidget( this );
  mMaximumScaleWidget = new QgsScaleWidget( this );
  mMinimumScaleWidget->setShowCurrentScaleButton( true );
  mMaximumScaleWidget->setShowCurrentScaleButton( true );
  reloadProjectScales();
  // add start, add comprehension of scales by settings fake ordered values
  mMinimumScaleWidget->setScale( 1.0 / 100000 );
  mMaximumScaleWidget->setScale( 1.0 / 1000 );

  mLayout->addWidget( minLbl, 0, 0, 2, 1 );
  mLayout->addWidget( mMinimumScaleIconLabel, 0, 1 );
  mLayout->addWidget( mMinimumScaleWidget, 0, 2 );
  mLayout->addWidget( maxLbl, 0, 3, 2, 1 );
  mLayout->addWidget( mMaximumScaleIconLabel, 0, 4 );
  mLayout->addWidget( mMaximumScaleWidget, 0, 5 );

  mLayout->setColumnStretch( 0, 0 );
  mLayout->setColumnStretch( 1, 0 );
  mLayout->setColumnStretch( 2, 3 );
  mLayout->setColumnStretch( 3, 0 );
  mLayout->setColumnStretch( 4, 0 );
  mLayout->setColumnStretch( 5, 3 );
}

QgsScaleRangeWidget::~QgsScaleRangeWidget()
{
}

void QgsScaleRangeWidget::reloadProjectScales()
{
  bool projectScales = QgsProject::instance()->readBoolEntry( "Scales", "/useProjectScales" );
  if ( projectScales )
  {
    QStringList scalesList = QgsProject::instance()->readListEntry( "Scales", "/ScalesList" );
    mMinimumScaleWidget->updateScales( scalesList );
    mMaximumScaleWidget->updateScales( scalesList );
  }
}

void QgsScaleRangeWidget::setMapCanvas( QgsMapCanvas *mapCanvas )
{
  mMinimumScaleWidget->setMapCanvas( mapCanvas );
  mMaximumScaleWidget->setMapCanvas( mapCanvas );
}

void QgsScaleRangeWidget::setMinimumScale( double scale )
{
  mMinimumScaleWidget->setScale( scale );
}

double QgsScaleRangeWidget::minimumScale()
{
  return mMinimumScaleWidget->scale();
}

void QgsScaleRangeWidget::setMaximumScale( double scale )
{
  mMaximumScaleWidget->setScale( scale );
}

double QgsScaleRangeWidget::maximumScale()
{
  return mMaximumScaleWidget->scale();
}

double QgsScaleRangeWidget::minimumScaleDenom()
{
  return qRound( 1.0 / maximumScale() );
}

double QgsScaleRangeWidget::maximumScaleDenom()
{
  return qRound( 1.0 / minimumScale() );
}

void QgsScaleRangeWidget::setScaleRange( double min, double max )
{
  setMaximumScale( max );
  setMinimumScale( min );
}

