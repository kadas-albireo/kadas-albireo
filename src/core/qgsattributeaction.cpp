/***************************************************************************
                               qgsattributeaction.cpp

 A class that stores and controls the managment and execution of actions
 associated. Actions are defined to be external programs that are run
 with user-specified inputs that can depend on the value of layer
 attributes.

                             -------------------
    begin                : Oct 24 2004
    copyright            : (C) 2004 by Gavin Macaulay
    email                : gavin at macaulay dot co dot nz

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QList>

#include <QStringList>
#include <QDomElement>

#include "qgsattributeaction.h"
#include "qgspythonrunner.h"
#include "qgsrunprocess.h"
#include "qgsvectorlayer.h"

void QgsAttributeAction::addAction( QgsAction::ActionType type, QString name, QString action, bool capture )
{
  mActions << QgsAction( type, name, action, capture );
}

void QgsAttributeAction::doAction( int index, const QgsAttributeMap &attributes,
                                   int defaultValueIndex, void ( *executePython )( const QString & ) )
{
  if ( index < 0 || index >= size() )
    return;

  const QgsAction &action = at( index );

  if ( !action.runable() )
    return;

  // A couple of extra options for running the action may be
  // useful. For example,
  // - run the action inside a terminal (on unix)
  // - capture the stdout from the process and display in a dialog
  //   box
  //
  // The capture stdout one is partially implemented. It just needs
  // the UI and the code in this function to select on the
  // action.capture() return value.

  // The QgsRunProcess instance created by this static function
  // deletes itself when no longer needed.
  QString expandedAction = expandAction( action.action(), attributes, defaultValueIndex );
  if ( action.type() == QgsAction::GenericPython )
  {
    if ( executePython )
    {
      // deprecated
      executePython( expandedAction );
    }
    else if ( smPythonExecute )
    {
      // deprecated
      smPythonExecute( expandedAction );
    }
    else
    {
      QgsPythonRunner::run( expandedAction );
    }
  }
  else
  {
    QgsRunProcess::create( expandedAction, action.capture() );
  }
}

QString QgsAttributeAction::expandAction( QString action, const QgsAttributeMap &attributes,
    uint clickedOnValue )
{
  // This function currently replaces all %% characters in the action
  // with the value from values[clickedOnValue].second, and then
  // searches for all strings that go %attribite_name, where
  // attribute_name is found in values[x].first, and replaces any that
  // it finds by values[s].second.

  // Additional substitutions could include symbols for $CWD, $HOME,
  // etc (and their OSX and Windows equivalents)

  // This function will potentially fall apart if any of the
  // substitutions produce text that could match another
  // substitution. May be better to adopt a two pass approach - identify
  // all matches and their substitutions and then do a second pass
  // for the actual substitutions.

  QString expanded_action;
  if ( attributes.contains( clickedOnValue ) )
    expanded_action = action.replace( "%%", attributes[clickedOnValue].toString() );
  else
    expanded_action = action;

  const QgsFieldMap &fields = mLayer->pendingFields();

  for ( int i = 0; i < 4; i++ )
  {
    for ( QgsAttributeMap::const_iterator it = attributes.begin(); it != attributes.end(); it++ )
    {
      QgsFieldMap::const_iterator fit = fields.find( it.key() );
      if ( fit == fields.constEnd() )
        continue;

      QString to_replace;
      switch ( i )
      {
        case 0: to_replace = "[%" + fit->name() + "]"; break;
        case 1: to_replace = "[%" + mLayer->attributeDisplayName( it.key() ) + "]"; break;
        case 2: to_replace = "%" + fit->name(); break;
        case 3: to_replace = "%" + mLayer->attributeDisplayName( it.key() ); break;
      }

      expanded_action = expanded_action.replace( to_replace, it.value().toString() );
    }
  }

  return expanded_action;
}

bool QgsAttributeAction::writeXML( QDomNode& layer_node, QDomDocument& doc ) const
{
  QDomElement aActions = doc.createElement( "attributeactions" );

  for ( int i = 0; i < mActions.size(); i++ )
  {
    QDomElement actionSetting = doc.createElement( "actionsetting" );
    actionSetting.setAttribute( "type", mActions[i].type() );
    actionSetting.setAttribute( "name", mActions[i].name() );
    actionSetting.setAttribute( "action", mActions[i].action() );
    actionSetting.setAttribute( "capture", mActions[i].capture() );
    aActions.appendChild( actionSetting );
  }
  layer_node.appendChild( aActions );

  return true;
}

bool QgsAttributeAction::readXML( const QDomNode& layer_node )
{
  mActions.clear();

  QDomNode aaNode = layer_node.namedItem( "attributeactions" );

  if ( !aaNode.isNull() )
  {
    QDomNodeList actionsettings = aaNode.childNodes();
    for ( unsigned int i = 0; i < actionsettings.length(); ++i )
    {
      QDomElement setting = actionsettings.item( i ).toElement();
      addAction(( QgsAction::ActionType ) setting.attributeNode( "type" ).value().toInt(),
                setting.attributeNode( "name" ).value(),
                setting.attributeNode( "action" ).value(),
                setting.attributeNode( "capture" ).value().toInt() != 0 );
    }
  }
  return true;
}

void ( *QgsAttributeAction::smPythonExecute )( const QString & ) = 0;

void QgsAttributeAction::setPythonExecute( void ( *runPython )( const QString & ) )
{
  smPythonExecute = runPython;
}
