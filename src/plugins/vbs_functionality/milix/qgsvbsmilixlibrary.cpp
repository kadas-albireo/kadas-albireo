/***************************************************************************
 *  qgsvbsmilixlibrary.cpp                                                 *
 *  -------------------                                                    *
 *  begin                : Sep 29, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbsmilixlibrary.h"
#include "qgsvbsmaptoolmilix.h"
#include "qgslogger.h"
#include "qgisinterface.h"
#include "qgsmapcanvas.h"
#include <QAction>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPainter>
#include "Client/VBSMilixClient.hpp"

static const int SymbolXmlRole = Qt::UserRole + 1;
static const int SymbolPointCountRole = Qt::UserRole + 2;

QgsVBSMilixLibrary::QgsVBSMilixLibrary( QgisInterface* iface, QgsVBSMilixManager* manager, QWidget *parent )
    : QDialog( parent ), mIface( iface )
{
  setLayout( new QVBoxLayout( this ) );

  QListWidget* listViewSymbols = new QListWidget( this );
  listViewSymbols->setViewMode( QListView::IconMode );
  layout()->addWidget( listViewSymbols );
  connect( listViewSymbols, SIGNAL( itemClicked( QListWidgetItem* ) ), this, SLOT( createMapTool( QListWidgetItem* ) ) );

  QDialogButtonBox* buttonBox = new QDialogButtonBox( this );
  buttonBox->addButton( QDialogButtonBox::Close );
  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
  connect( buttonBox, SIGNAL( rejected() ), this, SLOT( reject() ) );
  connect( this, SIGNAL( finished( int ) ), this, SLOT( unsetMilixTool() ) );

  layout()->addWidget( buttonBox );

  populateSymbols( listViewSymbols );
}

void QgsVBSMilixLibrary::populateSymbols( QListWidget* listViewSymbols )
{
  setCursor( Qt::WaitCursor );
  QStringList symbols;
  if ( !VBSMilixClient::getSymbols( symbols ) )
  {
    QgsDebugMsg( QString( "Failed to query symbols: %1" ).arg( VBSMilixClient::lastError() ) );
    unsetCursor();
    return;
  }
  foreach ( const QString& symbolXml, symbols )
  {
    QPixmap pixmap;
    QString name;
    bool hasVariablePoints = false;
    int minNumPoints = -1;
    if ( VBSMilixClient::getSymbol( symbolXml, pixmap, name, hasVariablePoints, minNumPoints ) )
    {
      QListWidgetItem* item = new QListWidgetItem( pixmap, QString( "%1 (%2)" ).arg( name ).arg( minNumPoints ) );
      item->setData( SymbolXmlRole, symbolXml );
      item->setData( SymbolPointCountRole, hasVariablePoints ? -1 : minNumPoints );
      listViewSymbols->addItem( item );
    }
    else
    {
      QgsDebugMsg( QString( "Failed to query symbol %1: %2" ).arg( symbolXml ).arg( VBSMilixClient::lastError() ) );
    }
  }

  unsetCursor();
}

void QgsVBSMilixLibrary::createMapTool( QListWidgetItem* item )
{
  QString symbolXml = item->data( SymbolXmlRole ).toString();
  int pointCount = item->data( SymbolPointCountRole ).toInt();
#pragma message("TODO: render a pointer into the preview pixmap")
  mCurTool = new QgsVBSMapToolMilix( mIface->mapCanvas(), mManager, symbolXml, pointCount, item->icon().pixmap( item->icon().actualSize( QSize( 128, 128 ) ) ) );
  connect( mCurTool.data(), SIGNAL( deactivated() ), mCurTool.data(), SLOT( deleteLater() ) );
  mIface->mapCanvas()->setMapTool( mCurTool.data() );
}

void QgsVBSMilixLibrary::unsetMilixTool()
{
  delete mCurTool.data();
}
