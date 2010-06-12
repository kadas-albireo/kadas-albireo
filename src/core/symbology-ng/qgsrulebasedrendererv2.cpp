/***************************************************************************
    qgsrulebasedrendererv2.cpp - Rule-based renderer (symbology-ng)
    ---------------------
    begin                : May 2010
    copyright            : (C) 2010 by Martin Dobias
    email                : wonder.sk at gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrulebasedrendererv2.h"

#include "qgssearchtreenode.h"
#include "qgssymbollayerv2utils.h"
#include "qgsrendercontext.h"
#include "qgsvectorlayer.h"
#include "qgslogger.h"

#include <QSet>

#include <QDomDocument>
#include <QDomElement>



QgsRuleBasedRendererV2::Rule::Rule( QgsSymbolV2* symbol, int scaleMinDenom, int scaleMaxDenom, QString filterExp )
    : mSymbol( symbol ),
    mScaleMinDenom( scaleMinDenom ), mScaleMaxDenom( scaleMaxDenom ),
    mFilterExp( filterExp )
{
  initFilter();
}

QgsRuleBasedRendererV2::Rule::Rule( const QgsRuleBasedRendererV2::Rule& other )
    : mSymbol( NULL )
{
  *this = other;
}

QgsRuleBasedRendererV2::Rule::~Rule()
{
  delete mSymbol;
}

void QgsRuleBasedRendererV2::Rule::initFilter()
{
  if ( !mFilterExp.isEmpty() )
  {
    mFilterParsed.setString( mFilterExp );
    mFilterTree = mFilterParsed.tree(); // may be NULL if there's no input
  }
  else
  {
    mFilterTree = NULL;
  }
}

QString QgsRuleBasedRendererV2::Rule::dump() const
{
  return QString( "RULE - scale [%1,%2] - filter %3 - symbol %4" )
         .arg( mScaleMinDenom ).arg( mScaleMaxDenom )
         .arg( mFilterExp ).arg( mSymbol->dump() );

}

QStringList QgsRuleBasedRendererV2::Rule::needsFields() const
{
  if ( ! mFilterTree )
    return QStringList();

  return mFilterTree->referencedColumns();
}

bool QgsRuleBasedRendererV2::Rule::isFilterOK( const QgsFieldMap& fields, QgsFeature& f ) const
{
  if ( ! mFilterTree )
    return true;

  bool res = mFilterTree->checkAgainst( fields, f.attributeMap() );
  //print "is_ok", res, feature.id(), feature.attributeMap()
  return res;
}

bool QgsRuleBasedRendererV2::Rule::isScaleOK( double scale ) const
{
  if ( mScaleMinDenom == 0 && mScaleMaxDenom == 0 )
    return true;
  if ( mScaleMinDenom != 0 && mScaleMinDenom > scale )
    return false;
  if ( mScaleMaxDenom != 0 && mScaleMaxDenom < scale )
    return false;
  return true;
}

QgsRuleBasedRendererV2::Rule& QgsRuleBasedRendererV2::Rule::operator=( const QgsRuleBasedRendererV2::Rule & other )
{
  if ( this != &other )
  {
    delete mSymbol;
    mSymbol = other.mSymbol->clone();

    mScaleMinDenom = other.mScaleMinDenom;
    mScaleMaxDenom = other.mScaleMaxDenom;
    mFilterExp = other.mFilterExp;
    initFilter();
  }
  return *this;
}

/////////////////////

QgsRuleBasedRendererV2::QgsRuleBasedRendererV2( QgsSymbolV2* defaultSymbol )
    : QgsFeatureRendererV2( "RuleRenderer" )
{
  mDefaultSymbol = defaultSymbol;

  // add the default rule
  mRules << Rule( defaultSymbol->clone() );
}


QgsSymbolV2* QgsRuleBasedRendererV2::symbolForFeature( QgsFeature& feature )
{
  return mCurrentSymbol;
}

void QgsRuleBasedRendererV2::renderFeature( QgsFeature& feature,
    QgsRenderContext& context,
    int layer,
    bool selected,
    bool drawVertexMarker )
{
  for ( QList<Rule*>::iterator it = mCurrentRules.begin(); it != mCurrentRules.end(); ++it )
  {
    Rule* rule = *it;
    if ( rule->isFilterOK( mCurrentFields, feature ) )
    {
      mCurrentSymbol = rule->symbol();
      // will ask for mCurrentSymbol
      QgsFeatureRendererV2::renderFeature( feature, context, layer, selected, drawVertexMarker );
    }
  }
}


void QgsRuleBasedRendererV2::startRender( QgsRenderContext& context, const QgsVectorLayer *vlayer )
{
  double currentScale = context.rendererScale();
  // filter out rules which are not compatible with this scale

  mCurrentRules.clear();
  for ( QList<Rule>::iterator it = mRules.begin(); it != mRules.end(); ++it )
  {
    Rule& rule = *it;
    if ( rule.isScaleOK( currentScale ) )
      mCurrentRules.append( &rule );
  }

  mCurrentFields = vlayer->pendingFields();

  for ( QList<Rule*>::iterator it = mCurrentRules.begin(); it != mCurrentRules.end(); ++it )
  {
    Rule* rule = *it;
    rule->symbol()->startRender( context );
  }
}

void QgsRuleBasedRendererV2::stopRender( QgsRenderContext& context )
{
  for ( QList<Rule*>::iterator it = mCurrentRules.begin(); it != mCurrentRules.end(); ++it )
  {
    Rule* rule = *it;
    rule->symbol()->stopRender( context );
  }

  mCurrentRules.clear();
  mCurrentFields.clear();
}

QList<QString> QgsRuleBasedRendererV2::usedAttributes()
{
  QSet<QString> attrs;
  for ( QList<Rule>::iterator it = mRules.begin(); it != mRules.end(); ++it )
  {
    Rule& rule = *it;
    attrs.unite( rule.needsFields().toSet() );
  }
  return attrs.values();
}

QgsFeatureRendererV2* QgsRuleBasedRendererV2::clone()
{
  QgsSymbolV2* s = mDefaultSymbol->clone();
  QgsRuleBasedRendererV2* r = new QgsRuleBasedRendererV2( s );
  r->mRules = mRules;
  return r;
}

QgsSymbolV2List QgsRuleBasedRendererV2::symbols()
{
  QgsSymbolV2List lst;
  for ( QList<Rule>::iterator it = mRules.begin(); it != mRules.end(); ++it )
  {
    Rule& rule = *it;
    lst.append( rule.symbol() );
  }

  return lst;
}

QDomElement QgsRuleBasedRendererV2::save( QDomDocument& doc )
{
  QDomElement rendererElem = doc.createElement( RENDERER_TAG_NAME );
  rendererElem.setAttribute( "type", "RuleRenderer" );

  QDomElement rulesElem = doc.createElement( "rules" );

  QgsSymbolV2Map symbols;
  symbols["default"] = mDefaultSymbol;

  int i = 0;
  for ( QList<Rule>::iterator it = mRules.begin(); it != mRules.end(); ++i, ++it )
  {
    Rule& rule = *it;
    symbols[QString::number( i )] = rule.symbol();
    QDomElement ruleElem = doc.createElement( "rule" );
    ruleElem.setAttribute( "symbol", i );
    ruleElem.setAttribute( "filter", rule.filterExpression() );
    ruleElem.setAttribute( "scalemindenom", rule.scaleMinDenom() );
    ruleElem.setAttribute( "scalemaxdenom", rule.scaleMaxDenom() );
    rulesElem.appendChild( ruleElem );
  }
  rendererElem.appendChild( rulesElem );

  QDomElement symbolsElem = QgsSymbolLayerV2Utils::saveSymbols( symbols, "symbols", doc );
  rendererElem.appendChild( symbolsElem );

  return rendererElem;
}


QgsFeatureRendererV2* QgsRuleBasedRendererV2::create( QDomElement& element )
{
  // load symbols
  QDomElement symbolsElem = element.firstChildElement( "symbols" );
  if ( symbolsElem.isNull() )
    return NULL;

  QgsSymbolV2Map symbolMap = QgsSymbolLayerV2Utils::loadSymbols( symbolsElem );

  if ( !symbolMap.contains( "default" ) )
  {
    QgsDebugMsg( "default symbol not found!" );
    return NULL;
  }

  QgsRuleBasedRendererV2* r = new QgsRuleBasedRendererV2( symbolMap.take( "default" ) );
  r->mRules.clear();

  QDomElement rulesElem = element.firstChildElement( "rules" );
  QDomElement ruleElem = rulesElem.firstChildElement( "rule" );
  while ( !ruleElem.isNull() )
  {
    QString symbolIdx = ruleElem.attribute( "symbol" );
    if ( symbolMap.contains( symbolIdx ) )
    {
      QString filterExp = ruleElem.attribute( "filter" );
      int scaleMinDenom = ruleElem.attribute( "scalemindenom", "0" ).toInt();
      int scaleMaxDenom = ruleElem.attribute( "scalemaxdenom", "0" ).toInt();
      r->mRules.append( Rule( symbolMap.take( symbolIdx ), scaleMinDenom, scaleMaxDenom, filterExp ) );
    }
    else
    {
      QgsDebugMsg( "symbol for rule " + symbolIdx + " not found! (skipping)" );
    }
    ruleElem = ruleElem.nextSiblingElement( "rule" );
  }

  // delete symbols if there are any more
  QgsSymbolLayerV2Utils::clearSymbolMap( symbolMap );

  return r;
}


int QgsRuleBasedRendererV2::ruleCount()
{
  return mRules.count();
}

QgsRuleBasedRendererV2::Rule& QgsRuleBasedRendererV2::ruleAt( int index )
{
  return mRules[index];
}

void QgsRuleBasedRendererV2::addRule( const QgsRuleBasedRendererV2::Rule& rule )
{
  mRules.append( rule );
}

void QgsRuleBasedRendererV2::insertRule( int index, const QgsRuleBasedRendererV2::Rule& rule )
{
  mRules.insert( index, rule );
}

void QgsRuleBasedRendererV2::updateRuleAt( int index, const QgsRuleBasedRendererV2::Rule& rule )
{
  mRules[index] = rule;
}

void QgsRuleBasedRendererV2::removeRuleAt( int index )
{
  mRules.removeAt( index );
}

#include "qgscategorizedsymbolrendererv2.h"
#include "qgsgraduatedsymbolrendererv2.h"

QList<QgsRuleBasedRendererV2::Rule> QgsRuleBasedRendererV2::refineRuleCategories( QgsRuleBasedRendererV2::Rule& initialRule, QgsCategorizedSymbolRendererV2* r )
{
  QList<Rule> rules;
  foreach( const QgsRendererCategoryV2& cat, r->categories() )
  {
    QString newfilter = QString( "%1 = '%2'" ).arg( r->classAttribute() ).arg( cat.value().toString() );
    QString filter = initialRule.filterExpression();
    if ( filter.isEmpty() )
      filter = newfilter;
    else
      filter = QString( "(%1) AND (%2)" ).arg( filter ).arg( newfilter );
    rules.append( Rule( cat.symbol()->clone(), initialRule.scaleMinDenom(), initialRule.scaleMaxDenom(), filter ) );
  }
  return rules;
}

QList<QgsRuleBasedRendererV2::Rule> QgsRuleBasedRendererV2::refineRuleRanges( QgsRuleBasedRendererV2::Rule& initialRule, QgsGraduatedSymbolRendererV2* r )
{
  QList<Rule> rules;
  foreach( const QgsRendererRangeV2& rng, r->ranges() )
  {
    QString newfilter = QString( "%1 >= '%2' AND %1 <= '%3'" ).arg( r->classAttribute() ).arg( rng.lowerValue() ).arg( rng.upperValue() );
    QString filter = initialRule.filterExpression();
    if ( filter.isEmpty() )
      filter = newfilter;
    else
      filter = QString( "(%1) AND (%2)" ).arg( filter ).arg( newfilter );
    rules.append( Rule( rng.symbol()->clone(), initialRule.scaleMinDenom(), initialRule.scaleMaxDenom(), filter ) );
  }
  return rules;
}

QList<QgsRuleBasedRendererV2::Rule> QgsRuleBasedRendererV2::refineRuleScales( QgsRuleBasedRendererV2::Rule& initialRule, QList<int> scales )
{
  QList<Rule> rules;
  int oldScale = 0;
  foreach( int scale, scales )
  {
    rules.append( Rule( initialRule.symbol()->clone(), oldScale, scale, initialRule.filterExpression() ) );
    oldScale = scale;
  }
  // last rule
  rules.append( Rule( initialRule.symbol()->clone(), oldScale, 0, initialRule.filterExpression() ) );
  return rules;
}
