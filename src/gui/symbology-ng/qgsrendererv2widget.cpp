/***************************************************************************
    qgsrendererv2widget.cpp
    ---------------------
    begin                : November 2009
    copyright            : (C) 2009 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsrendererv2widget.h"
#include "qgssymbolv2.h"
#include "qgsvectorlayer.h"
#include "qgscolordialog.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QMenu>

#include "qgssymbollevelsv2dialog.h"
#include "qgsexpressionbuilderdialog.h"


QgsRendererV2Widget::QgsRendererV2Widget( QgsVectorLayer* layer, QgsStyleV2* style )
    : QWidget(), mLayer( layer ), mStyle( style )
{
  contextMenu = new QMenu( "Renderer Options " );

  mCopyAction = contextMenu->addAction( tr( "Copy" ), this, SLOT( copy() ) );
  mCopyAction->setShortcut( QKeySequence( QKeySequence::Copy ) );
  mPasteAction = contextMenu->addAction( tr( "Paste" ), this, SLOT( paste() ) );
  mPasteAction->setShortcut( QKeySequence( QKeySequence::Paste ) );

  contextMenu->addSeparator();
  contextMenu->addAction( tr( "Change color" ), this, SLOT( changeSymbolColor() ) );
  contextMenu->addAction( tr( "Change transparency" ), this, SLOT( changeSymbolTransparency() ) );
  contextMenu->addAction( tr( "Change output unit" ), this, SLOT( changeSymbolUnit() ) );

  if ( mLayer && mLayer->geometryType() == QGis::Line )
  {
    contextMenu->addAction( tr( "Change width" ), this, SLOT( changeSymbolWidth() ) );
  }
  else if ( mLayer && mLayer->geometryType() == QGis::Point )
  {
    contextMenu->addAction( tr( "Change size" ), this, SLOT( changeSymbolSize() ) );
  }
}

void QgsRendererV2Widget::contextMenuViewCategories( const QPoint & )
{
  contextMenu->exec( QCursor::pos() );
}

void QgsRendererV2Widget::changeSymbolColor()
{
  QList<QgsSymbolV2*> symbolList = selectedSymbols();
  if ( symbolList.size() < 1 )
  {
    return;
  }

  QColor color = QgsColorDialogV2::getColor( symbolList.at( 0 )->color(), this, "Change Symbol Color", true );
  if ( color.isValid() )
  {
    QList<QgsSymbolV2*>::iterator symbolIt = symbolList.begin();
    for ( ; symbolIt != symbolList.end(); ++symbolIt )
    {
      ( *symbolIt )->setColor( color );
    }
    refreshSymbolView();
  }
}

void QgsRendererV2Widget::changeSymbolTransparency()
{
  QList<QgsSymbolV2*> symbolList = selectedSymbols();
  if ( symbolList.size() < 1 )
  {
    return;
  }

  bool ok;
  double oldTransparency = ( 1 - symbolList.at( 0 )->alpha() ) * 100; // convert to percents
  double transparency = QInputDialog::getDouble( this, tr( "Transparency" ), tr( "Change symbol transparency [%]" ), oldTransparency, 0.0, 100.0, 0, &ok );
  if ( ok )
  {
    QList<QgsSymbolV2*>::iterator symbolIt = symbolList.begin();
    for ( ; symbolIt != symbolList.end(); ++symbolIt )
    {
      ( *symbolIt )->setAlpha( 1 - transparency / 100 );
    }
    refreshSymbolView();
  }
}

void QgsRendererV2Widget::changeSymbolUnit()
{
  QList<QgsSymbolV2*> symbolList = selectedSymbols();
  if ( symbolList.size() < 1 )
  {
    return;
  }

  bool ok;
  int currentUnit = ( symbolList.at( 0 )->outputUnit() == QgsSymbolV2::MM ) ? 0 : 1;
  QString item = QInputDialog::getItem( this, tr( "Symbol unit" ), tr( "Select symbol unit" ), QStringList() << tr( "Millimeter" ) << tr( "Map unit" ), currentUnit, false, &ok );
  if ( ok )
  {
    QgsSymbolV2::OutputUnit unit = ( item.compare( tr( "Millimeter" ) ) == 0 ) ? QgsSymbolV2::MM : QgsSymbolV2::MapUnit;

    QList<QgsSymbolV2*>::iterator symbolIt = symbolList.begin();
    for ( ; symbolIt != symbolList.end(); ++symbolIt )
    {
      ( *symbolIt )->setOutputUnit( unit );
    }
    refreshSymbolView();
  }
}

void QgsRendererV2Widget::changeSymbolWidth()
{
  QList<QgsSymbolV2*> symbolList = selectedSymbols();
  if ( symbolList.size() < 1 )
  {
    return;
  }

  bool ok;
  double width = QInputDialog::getDouble( this, tr( "Width" ), tr( "Change symbol width" ), dynamic_cast<QgsLineSymbolV2*>( symbolList.at( 0 ) )->width(), 0.0, 999999, 1, &ok );
  if ( ok )
  {
    QList<QgsSymbolV2*>::iterator symbolIt = symbolList.begin();
    for ( ; symbolIt != symbolList.end(); ++symbolIt )
    {
      dynamic_cast<QgsLineSymbolV2*>( *symbolIt )->setWidth( width );
    }
    refreshSymbolView();
  }
}

void QgsRendererV2Widget::changeSymbolSize()
{
  QList<QgsSymbolV2*> symbolList = selectedSymbols();
  if ( symbolList.size() < 1 )
  {
    return;
  }

  bool ok;
  double size = QInputDialog::getDouble( this, tr( "Size" ), tr( "Change symbol size" ), dynamic_cast<QgsMarkerSymbolV2*>( symbolList.at( 0 ) )->size(), 0.0, 999999, 1, &ok );
  if ( ok )
  {
    QList<QgsSymbolV2*>::iterator symbolIt = symbolList.begin();
    for ( ; symbolIt != symbolList.end(); ++symbolIt )
    {
      dynamic_cast<QgsMarkerSymbolV2*>( *symbolIt )->setSize( size );
    }
    refreshSymbolView();
  }
}

void QgsRendererV2Widget::showSymbolLevelsDialog( QgsFeatureRendererV2* r )
{
  QgsLegendSymbolList symbols = r->legendSymbolItems();

  QgsSymbolLevelsV2Dialog dlg( symbols, r->usingSymbolLevels(), this );

  if ( dlg.exec() )
  {
    r->setUsingSymbolLevels( dlg.usingLevels() );
  }
}


////////////

//#include <QAction>
#include "qgsfield.h"
#include <QMenu>

QgsRendererV2DataDefinedMenus::QgsRendererV2DataDefinedMenus( QMenu* menu, QgsVectorLayer* layer, QString rotationField, QString sizeScaleField, QgsSymbolV2::ScaleMethod scaleMethod )
    : QObject( menu ), mLayer( layer )
{
  mRotationMenu = new QMenu( tr( "Rotation field" ) );
  mSizeScaleMenu = new QMenu( tr( "Size scale field" ) );

  mRotationAttributeActionGroup = new QActionGroup( mRotationMenu );
  mSizeAttributeActionGroup = new QActionGroup( mSizeScaleMenu );
  mSizeMethodActionGroup = new QActionGroup( mSizeScaleMenu );

  populateMenu( mRotationMenu, rotationField, mRotationAttributeActionGroup );
  populateMenu( mSizeScaleMenu, sizeScaleField, mSizeAttributeActionGroup );

  mSizeScaleMenu->addSeparator();

  QAction* aScaleByArea = new QAction( tr( "Scale area" ), mSizeMethodActionGroup );
  QAction* aScaleByDiameter = new QAction( tr( "Scale diameter" ), mSizeMethodActionGroup );

  aScaleByArea->setCheckable( true );
  aScaleByDiameter->setCheckable( true );

  if ( scaleMethod == QgsSymbolV2::ScaleDiameter )
  {
    aScaleByDiameter->setChecked( true );
  }
  else
  {
    aScaleByArea->setChecked( true );
  }

  mSizeScaleMenu->addActions( mSizeMethodActionGroup->actions() );

  menu->addMenu( mRotationMenu );
  menu->addMenu( mSizeScaleMenu );

  connect( mSizeMethodActionGroup, SIGNAL( triggered( QAction* ) ), this, SLOT( scaleMethodSelected( QAction* ) ) );
  connect( mRotationAttributeActionGroup, SIGNAL( triggered( QAction* ) ), this, SLOT( rotationFieldSelected( QAction* ) ) );
  connect( mSizeAttributeActionGroup, SIGNAL( triggered( QAction* ) ), this, SLOT( sizeScaleFieldSelected( QAction* ) ) );
}

QgsRendererV2DataDefinedMenus::~QgsRendererV2DataDefinedMenus()
{
  delete mSizeMethodActionGroup;
  delete mSizeAttributeActionGroup;
  delete mRotationAttributeActionGroup;
  delete mRotationMenu;
  delete mSizeScaleMenu;
}

void QgsRendererV2DataDefinedMenus::populateMenu( QMenu* menu, QString fieldName, QActionGroup *actionGroup )
{
  QAction* aExpr = new QAction( tr( "- expression -" ), actionGroup );
  aExpr->setCheckable( true );
  menu->addAction( aExpr );
  menu->addSeparator();
  QAction* aNo = new QAction( tr( "- no field -" ), actionGroup );
  aNo->setCheckable( true );
  menu->addAction( aNo );
  menu->addSeparator();

  bool hasField = false;
  const QgsFields & flds = mLayer->pendingFields();
  for ( int idx = 0; idx < flds.count(); ++idx )
  {
    const QgsField& fld = flds[idx];
    if ( fld.type() == QVariant::Int || fld.type() == QVariant::Double )
    {
      QAction* a = new QAction( fld.name(), actionGroup );
      a->setCheckable( true );
      if ( fieldName == fld.name() )
      {
        a->setChecked( true );
        hasField = true;
      }
      menu->addAction( a );
    }
  }

  if ( !hasField )
  {
    if ( fieldName.isEmpty() )
    {
      aNo->setChecked( true );
    }
    else
    {
      aExpr->setChecked( true );
      aExpr->setText( tr( "- expression -" ) + fieldName );
    }
  }

}

void QgsRendererV2DataDefinedMenus::rotationFieldSelected( QAction* a )
{
  if ( a == NULL )
    return;

  QString fldName = a->text();
#if 0
  updateMenu( mRotationAttributeActionGroup, fldName );
#endif
  if ( fldName == tr( "- no field -" ) )
  {
    fldName = QString();
  }
  else if ( fldName.startsWith( tr( "- expression -" ) ) )
  {
    QString expr( fldName );
    expr.replace( 0, tr( "- expression -" ).length(), "" );
    QgsExpressionBuilderDialog dialog( mLayer, expr );
    if ( !dialog.exec() ) return;
    fldName = dialog.expressionText();
    Q_ASSERT( !QgsExpression( fldName ).hasParserError() );
    a->setText( tr( "- expression -" ) + fldName );
  }

  emit rotationFieldChanged( fldName );
}

void QgsRendererV2DataDefinedMenus::sizeScaleFieldSelected( QAction* a )
{
  if ( a == NULL )
    return;

  QString fldName = a->text();
#if 0
  updateMenu( mSizeAttributeActionGroup, fldName );
#endif
  if ( fldName == tr( "- no field -" ) )
  {
    fldName = QString();
  }
  else if ( fldName.startsWith( tr( "- expression -" ) ) )
  {
    QString expr( fldName );
    expr.replace( 0, tr( "- expression -" ).length(), "" );
    QgsExpressionBuilderDialog dialog( mLayer, expr );
    if ( !dialog.exec() ) return;
    fldName = dialog.expressionText();
    Q_ASSERT( !QgsExpression( fldName ).hasParserError() );
    a->setText( tr( "- expression -" ) + fldName );
  }

  emit sizeScaleFieldChanged( fldName );
}

void QgsRendererV2DataDefinedMenus::scaleMethodSelected( QAction* a )
{
  if ( a == NULL )
    return;

  if ( a->text() == tr( "Scale area" ) )
  {
    emit scaleMethodChanged( QgsSymbolV2::ScaleArea );
  }
  else if ( a->text() == tr( "Scale diameter" ) )
  {
    emit scaleMethodChanged( QgsSymbolV2::ScaleDiameter );
  }
}
#if 0 // MK: is there any reason for this?
void QgsRendererV2DataDefinedMenus::updateMenu( QActionGroup* actionGroup, QString fieldName )
{
  foreach ( QAction* a, actionGroup->actions() )
  {
    a->setChecked( a->text() == fieldName );
  }
}
#endif
