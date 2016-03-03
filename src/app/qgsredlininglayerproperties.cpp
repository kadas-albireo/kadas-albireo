/***************************************************************************
                          qgsredlininglayerproperties.cpp
                             -------------------
    begin                : March 2016
    copyright            : (C) 2016 by Sandro Mani
    email                : smani@sourcepole.ch
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsredlininglayerproperties.h"
#include "qgsredlininglayer.h"
#include "qgsmaplayerpropertiesfactory.h"
#include "qgsvectorlayerpropertiespage.h"
#include "qgsproject.h"

#include <QPushButton>

QgsRedliningLayerProperties::QgsRedliningLayerProperties( QgsRedliningLayer *lyr, QWidget * parent, Qt::WindowFlags fl )
    : QgsOptionsDialogBase( "RedliningLayerProperties", parent, fl )
    , layer( lyr )
{
  setupUi( this );
  // QgsOptionsDialogBase handles saving/restoring of geometry, splitter and current tab states,
  // switching vertical tabs between icon/text to icon-only modes (splitter collapsed to left),
  // and connecting QDialogButtonBox's accepted/rejected signals to dialog's accept/reject slots
  initOptionsBase( false );

  connect( buttonBox->button( QDialogButtonBox::Apply ), SIGNAL( clicked() ), this, SLOT( apply() ) );
  connect( this, SIGNAL( accepted() ), this, SLOT( apply() ) );
  connect( this, SIGNAL( rejected() ), this, SLOT( onCancel() ) );

  QSettings settings;
  // if dialog hasn't been opened/closed yet, default to Styles tab, which is used most often
  // this will be read by restoreOptionsBaseUi()
  if ( !settings.contains( QString( "/Windows/RedliningLayerProperties/tab" ) ) )
  {
    settings.setValue( QString( "/Windows/RedliningLayerProperties/tab" ), 0 );
  }

  QString title = QString( tr( "Layer Properties - %1" ) ).arg( layer->name() );
  restoreOptionsBaseUi( title );
} // QgsVectorLayerProperties ctor


QgsRedliningLayerProperties::~QgsRedliningLayerProperties()
{
}


void QgsRedliningLayerProperties::apply()
{
  // apply all plugin dialogs
  foreach ( QgsVectorLayerPropertiesPage* page, mLayerPropertiesPages )
  {
    page->apply();
  }

  layer->triggerRepaint();
  // notify the project we've made a change
  QgsProject::instance()->dirty( true );
}

void QgsRedliningLayerProperties::onCancel()
{
}

void QgsRedliningLayerProperties::addPropertiesPageFactory( QgsMapLayerPropertiesFactory* factory )
{
  QListWidgetItem* item = factory->createVectorLayerPropertiesItem( layer, mOptionsListWidget );
  if ( item )
  {
    mOptionsListWidget->addItem( item );

    QgsVectorLayerPropertiesPage* page = factory->createVectorLayerPropertiesPage( layer, this );
    mLayerPropertiesPages << page;
    mOptionsStackedWidget->addWidget( page );
  }
}
