/***************************************************************************
                         qgsattributedialog.cpp  -  description
                             -------------------
    begin                : October 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsattributedialog.h"

#include "qgsattributeform.h"
#include "qgshighlight.h"

#include <QSettings>
#include <QGridLayout>


QgsAttributeDialog::QgsAttributeDialog( QgsVectorLayer* vl, QgsFeature* thepFeature, bool featureOwner, QgsDistanceArea myDa, QWidget* parent, bool showDialogButtons )
    : QDialog( parent )
    , mHighlight( 0 )
{
  QgsAttributeEditorContext context;
  context.setDistanceArea( myDa );
  init( vl, thepFeature, context, parent );

  if ( !showDialogButtons )
    mAttributeForm->hideButtonBox();

  if ( featureOwner )
    delete thepFeature;
}

QgsAttributeDialog::QgsAttributeDialog( QgsVectorLayer* vl, QgsFeature* thepFeature, bool featureOwner, QWidget* parent, bool showDialogButtons, QgsAttributeEditorContext context )
    : QDialog( parent )
    , mHighlight( 0 )
{
  init( vl, thepFeature, context, parent );

  if ( !showDialogButtons )
    mAttributeForm->hideButtonBox();

  if ( featureOwner )
    delete thepFeature;
}

QgsAttributeDialog::~QgsAttributeDialog()
{
  if ( mHighlight )
  {
    mHighlight->hide();
    delete mHighlight;
  }

  saveGeometry();
}

void QgsAttributeDialog::saveGeometry()
{
  QSettings().setValue( mSettingsPath + "geometry", QDialog::saveGeometry() );
}

void QgsAttributeDialog::restoreGeometry()
{
  QDialog::restoreGeometry( QSettings().value( mSettingsPath + "geometry" ).toByteArray() );
}

void QgsAttributeDialog::setHighlight( QgsHighlight* h )
{
  delete mHighlight;

  mHighlight = h;
}

void QgsAttributeDialog::accept()
{
  mAttributeForm->save();
  QDialog::accept();
}

void QgsAttributeDialog::show( bool autoDelete )
{
  if ( autoDelete )
    setAttribute( Qt::WA_DeleteOnClose );

  QDialog::show();
  raise();
  activateWindow();
}

void QgsAttributeDialog::init( QgsVectorLayer* layer, QgsFeature* feature, QgsAttributeEditorContext& context, QWidget* parent )
{
  setWindowTitle( tr( "Feature Attributes" ) );
  setLayout( new QGridLayout() );
  layout()->setMargin( 0 );
  mAttributeForm = new QgsAttributeForm( layer, *feature, context, parent );
  layout()->addWidget( mAttributeForm );
  QDialogButtonBox* buttonBox = mAttributeForm->findChild<QDialogButtonBox*>();
  connect( buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  restoreGeometry();
}
