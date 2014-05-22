/***************************************************************************
    qgsrangewidgetfactory.cpp
     --------------------------------------
    Date                 : 5.1.2014
    Copyright            : (C) 2014 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrangewidgetfactory.h"
#include "qgsrangeconfigdlg.h"
#include "qgsrangewidget.h"
#include "qgsvectorlayer.h"


QgsRangeWidgetFactory::QgsRangeWidgetFactory( QString name )
  : QgsEditorWidgetFactory( name )
{
}

QgsEditorWidgetWrapper* QgsRangeWidgetFactory::create( QgsVectorLayer* vl, int fieldIdx, QWidget* editor, QWidget* parent ) const
{
  return new QgsRangeWidget( vl, fieldIdx, editor, parent );
}

QgsEditorConfigWidget* QgsRangeWidgetFactory::configWidget( QgsVectorLayer* vl, int fieldIdx, QWidget* parent ) const
{
  return new QgsRangeConfigDlg( vl, fieldIdx, parent );
}

QgsEditorWidgetConfig QgsRangeWidgetFactory::readConfig( const QDomElement& configElement, QgsVectorLayer* layer, int fieldIdx )
{
  Q_UNUSED( layer );
  Q_UNUSED( fieldIdx );
  QMap<QString, QVariant> cfg;

  cfg.insert( "Style", configElement.attribute( "Style" ) );
  cfg.insert( "Min", configElement.attribute( "Min" ).toInt() );
  cfg.insert( "Max", configElement.attribute( "Max" ).toInt() );
  cfg.insert( "Step", configElement.attribute( "Step" ).toInt() );

  return cfg;
}

void QgsRangeWidgetFactory::writeConfig( const QgsEditorWidgetConfig& config, QDomElement& configElement, const QDomDocument& doc, const QgsVectorLayer* layer, int fieldIdx )
{
  Q_UNUSED( doc );
  Q_UNUSED( layer );
  Q_UNUSED( fieldIdx );

  configElement.setAttribute( "Style", config["Style"].toString() );
  configElement.setAttribute( "Min", config["Min"].toInt() );
  configElement.setAttribute( "Max", config["Max"].toInt() );
  configElement.setAttribute( "Step", config["Step"].toInt() );
}

bool QgsRangeWidgetFactory::supportsField(QgsVectorLayer* vl, int fieldIdx)
{
  switch( vl->pendingFields()[fieldIdx].type() )
  {
    case QVariant::LongLong:
    case QVariant::Double:
    case QVariant::Int:
      return true;

    default:
      return false;
  }
}
