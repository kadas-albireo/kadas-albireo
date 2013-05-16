/***************************************************************************
    qgscategorizedsymbolrendererv2.cpp
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
#include <algorithm>

#include "qgscategorizedsymbolrendererv2.h"

#include "qgssymbolv2.h"
#include "qgssymbollayerv2utils.h"
#include "qgsvectorcolorrampv2.h"

#include "qgsfeature.h"
#include "qgsvectorlayer.h"
#include "qgslogger.h"

#include <QDomDocument>
#include <QDomElement>
#include <QSettings> // for legend

QgsRendererCategoryV2::QgsRendererCategoryV2()
    : mValue(), mSymbol( 0 ), mLabel()
{
}

QgsRendererCategoryV2::QgsRendererCategoryV2( QVariant value, QgsSymbolV2* symbol, QString label )
    : mValue( value ), mSymbol( symbol ), mLabel( label )
{
}

QgsRendererCategoryV2::QgsRendererCategoryV2( const QgsRendererCategoryV2& cat )
    : mValue( cat.mValue ), mSymbol( 0 ), mLabel( cat.mLabel )
{
  if ( cat.mSymbol )
  {
    mSymbol = cat.mSymbol->clone();
  }
}

QgsRendererCategoryV2::~QgsRendererCategoryV2()
{
  if ( mSymbol ) delete mSymbol;
}

QgsRendererCategoryV2& QgsRendererCategoryV2::operator=( const QgsRendererCategoryV2 & cat )
{
  mValue = cat.mValue;
  mLabel = cat.mLabel;
  mSymbol = 0;
  if ( cat.mSymbol )
  {
    mSymbol = cat.mSymbol->clone();
  }
  return *this;
}

QVariant QgsRendererCategoryV2::value() const
{
  return mValue;
}

QgsSymbolV2* QgsRendererCategoryV2::symbol() const
{
  return mSymbol;
}

QString QgsRendererCategoryV2::label() const
{
  return mLabel;
}

void QgsRendererCategoryV2::setValue( const QVariant &value )
{
  mValue = value;
}

void QgsRendererCategoryV2::setSymbol( QgsSymbolV2* s )
{
  if ( mSymbol == s )
    return;
  delete mSymbol;
  mSymbol = s;
}

void QgsRendererCategoryV2::setLabel( const QString &label )
{
  mLabel = label;
}

QString QgsRendererCategoryV2::dump()
{
  return QString( "%1::%2::%3\n" ).arg( mValue.toString() ).arg( mLabel ).arg( mSymbol->dump() );
}

void QgsRendererCategoryV2::toSld( QDomDocument &doc, QDomElement &element, QgsStringMap props ) const
{
  if ( !mSymbol || props.value( "attribute", "" ).isEmpty() )
    return;

  QString attrName = props[ "attribute" ];

  QDomElement ruleElem = doc.createElement( "se:Rule" );
  element.appendChild( ruleElem );

  QDomElement nameElem = doc.createElement( "se:Name" );
  nameElem.appendChild( doc.createTextNode( mLabel ) );
  ruleElem.appendChild( nameElem );

  QDomElement descrElem = doc.createElement( "se:Description" );
  QDomElement titleElem = doc.createElement( "se:Title" );
  QString descrStr = QString( "%1 is '%2'" ).arg( attrName ).arg( mValue.toString() );
  titleElem.appendChild( doc.createTextNode( !mLabel.isEmpty() ? mLabel : descrStr ) );
  descrElem.appendChild( titleElem );
  ruleElem.appendChild( descrElem );

  // create the ogc:Filter for the range
  QDomElement filterElem = doc.createElement( "ogc:Filter" );
  QString filterFunc = QString( "%1 = '%2'" )
                       .arg( attrName.replace( "\"", "\"\"" ) )
                       .arg( mValue.toString().replace( "'", "''" ) );
  QgsSymbolLayerV2Utils::createFunctionElement( doc, filterElem, filterFunc );
  ruleElem.appendChild( filterElem );

  mSymbol->toSld( doc, ruleElem, props );
}

///////////////////

QgsCategorizedSymbolRendererV2::QgsCategorizedSymbolRendererV2( QString attrName, QgsCategoryList categories )
    : QgsFeatureRendererV2( "categorizedSymbol" ),
    mAttrName( attrName ),
    mCategories( categories ),
    mSourceSymbol( NULL ),
    mSourceColorRamp( NULL ),
    mScaleMethod( QgsSymbolV2::ScaleArea ),
    mRotationFieldIdx( -1 ),
    mSizeScaleFieldIdx( -1 )
{
  for ( int i = 0; i < mCategories.count(); ++i )
  {
    QgsRendererCategoryV2& cat = mCategories[i];
    if ( cat.symbol() == NULL )
    {
      QgsDebugMsg( "invalid symbol in a category! ignoring..." );
      mCategories.removeAt( i-- );
    }
    //mCategories.insert(cat.value().toString(), cat);
  }
}

QgsCategorizedSymbolRendererV2::~QgsCategorizedSymbolRendererV2()
{
  mCategories.clear(); // this should also call destructors of symbols
  delete mSourceSymbol;
  delete mSourceColorRamp;
}

void QgsCategorizedSymbolRendererV2::rebuildHash()
{
  mSymbolHash.clear();

  for ( int i = 0; i < mCategories.count(); ++i )
  {
    QgsRendererCategoryV2& cat = mCategories[i];
    mSymbolHash.insert( cat.value().toString(), cat.symbol() );
  }
}

QgsSymbolV2* QgsCategorizedSymbolRendererV2::symbolForValue( QVariant value )
{
  // TODO: special case for int, double
  QHash<QString, QgsSymbolV2*>::iterator it = mSymbolHash.find( value.toString() );
  if ( it == mSymbolHash.end() )
  {
    if ( mSymbolHash.count() == 0 )
    {
      QgsDebugMsg( "there are no hashed symbols!!!" );
    }
    else
    {
      QgsDebugMsg( "attribute value not found: " + value.toString() );
    }
    return NULL;
  }

  return *it;
}

QgsSymbolV2* QgsCategorizedSymbolRendererV2::symbolForFeature( QgsFeature& feature )
{
  const QgsAttributes& attrs = feature.attributes();
  if ( mAttrNum < 0 || mAttrNum >= attrs.count() )
  {
    QgsDebugMsg( "attribute '" + mAttrName + "' (index " + QString::number( mAttrNum ) + ") required by renderer not found" );
    return NULL;
  }

  // find the right symbol for the category
  QgsSymbolV2* symbol = symbolForValue( attrs[mAttrNum] );
  if ( symbol == NULL )
  {
    // if no symbol found use default one
    return symbolForValue( QVariant( "" ) );
  }

  if ( mRotationFieldIdx == -1 && mSizeScaleFieldIdx == -1 )
    return symbol; // no data-defined rotation/scaling - just return the symbol

  // find out rotation, size scale
  double rotation = 0;
  double sizeScale = 1;
  if ( mRotationFieldIdx != -1 )
    rotation = attrs[mRotationFieldIdx].toDouble();
  if ( mSizeScaleFieldIdx != -1 )
    sizeScale = attrs[mSizeScaleFieldIdx].toDouble();

  // take a temporary symbol (or create it if doesn't exist)
  QgsSymbolV2* tempSymbol = mTempSymbols[attrs[mAttrNum].toString()];

  // modify the temporary symbol and return it
  if ( tempSymbol->type() == QgsSymbolV2::Marker )
  {
    QgsMarkerSymbolV2* markerSymbol = static_cast<QgsMarkerSymbolV2*>( tempSymbol );
    if ( mRotationFieldIdx != -1 )
      markerSymbol->setAngle( rotation );
    if ( mSizeScaleFieldIdx != -1 )
      markerSymbol->setSize( sizeScale * static_cast<QgsMarkerSymbolV2*>( symbol )->size() );
    markerSymbol->setScaleMethod( mScaleMethod );
  }
  else if ( tempSymbol->type() == QgsSymbolV2::Line )
  {
    QgsLineSymbolV2* lineSymbol = static_cast<QgsLineSymbolV2*>( tempSymbol );
    if ( mSizeScaleFieldIdx != -1 )
      lineSymbol->setWidth( sizeScale * static_cast<QgsLineSymbolV2*>( symbol )->width() );
  }

  return tempSymbol;
}

int QgsCategorizedSymbolRendererV2::categoryIndexForValue( QVariant val )
{
  for ( int i = 0; i < mCategories.count(); i++ )
  {
    if ( mCategories[i].value() == val )
      return i;
  }
  return -1;
}

bool QgsCategorizedSymbolRendererV2::updateCategoryValue( int catIndex, const QVariant &value )
{
  if ( catIndex < 0 || catIndex >= mCategories.size() )
    return false;
  mCategories[catIndex].setValue( value );
  return true;
}

bool QgsCategorizedSymbolRendererV2::updateCategorySymbol( int catIndex, QgsSymbolV2* symbol )
{
  if ( catIndex < 0 || catIndex >= mCategories.size() )
    return false;
  mCategories[catIndex].setSymbol( symbol );
  return true;
}

bool QgsCategorizedSymbolRendererV2::updateCategoryLabel( int catIndex, QString label )
{
  if ( catIndex < 0 || catIndex >= mCategories.size() )
    return false;
  mCategories[catIndex].setLabel( label );
  return true;
}

void QgsCategorizedSymbolRendererV2::addCategory( const QgsRendererCategoryV2 &cat )
{
  if ( cat.symbol() == NULL )
  {
    QgsDebugMsg( "invalid symbol in a category! ignoring..." );
  }
  else
  {
    mCategories.append( cat );
  }
}

bool QgsCategorizedSymbolRendererV2::deleteCategory( int catIndex )
{
  if ( catIndex < 0 || catIndex >= mCategories.size() )
    return false;

  mCategories.removeAt( catIndex );
  return true;
}

void QgsCategorizedSymbolRendererV2::deleteAllCategories()
{
  mCategories.clear();
}

void QgsCategorizedSymbolRendererV2::moveCategory( int from, int to )
{
  if ( from < 0 || from >= mCategories.size() || to < 0 || to >= mCategories.size() ) return;
  mCategories.move( from, to );
}

bool valueLessThan( const QgsRendererCategoryV2 &c1, const QgsRendererCategoryV2 &c2 )
{
  return qgsVariantLessThan( c1.value(), c2.value() );
}
bool valueGreaterThan( const QgsRendererCategoryV2 &c1, const QgsRendererCategoryV2 &c2 )
{
  return qgsVariantGreaterThan( c1.value(), c2.value() );
}

void QgsCategorizedSymbolRendererV2::sortByValue( Qt::SortOrder order )
{
  if ( order == Qt::AscendingOrder )
  {
    qSort( mCategories.begin(), mCategories.end(), valueLessThan );
  }
  else
  {
    qSort( mCategories.begin(), mCategories.end(), valueGreaterThan );
  }
}

bool labelLessThan( const QgsRendererCategoryV2 &c1, const QgsRendererCategoryV2 &c2 )
{
  return QString::localeAwareCompare( c1.label(), c2.label() ) < 0;
}

bool labelGreaterThan( const QgsRendererCategoryV2 &c1, const QgsRendererCategoryV2 &c2 )
{
  return !labelLessThan( c1, c2 );
}

void QgsCategorizedSymbolRendererV2::sortByLabel( Qt::SortOrder order )
{
  if ( order == Qt::AscendingOrder )
  {
    qSort( mCategories.begin(), mCategories.end(), labelLessThan );
  }
  else
  {
    qSort( mCategories.begin(), mCategories.end(), labelGreaterThan );
  }
}

void QgsCategorizedSymbolRendererV2::startRender( QgsRenderContext& context, const QgsVectorLayer *vlayer )
{
  // make sure that the hash table is up to date
  rebuildHash();

  // find out classification attribute index from name
  mAttrNum = vlayer ? vlayer->fieldNameIndex( mAttrName ) : -1;

  mRotationFieldIdx  = ( mRotationField.isEmpty()  ? -1 : vlayer->fieldNameIndex( mRotationField ) );
  mSizeScaleFieldIdx = ( mSizeScaleField.isEmpty() ? -1 : vlayer->fieldNameIndex( mSizeScaleField ) );

  QgsCategoryList::iterator it = mCategories.begin();
  for ( ; it != mCategories.end(); ++it )
  {
    it->symbol()->startRender( context, vlayer );

    if ( mRotationFieldIdx != -1 || mSizeScaleFieldIdx != -1 )
    {
      QgsSymbolV2* tempSymbol = it->symbol()->clone();
      tempSymbol->setRenderHints(( mRotationFieldIdx != -1 ? QgsSymbolV2::DataDefinedRotation : 0 ) |
                                 ( mSizeScaleFieldIdx != -1 ? QgsSymbolV2::DataDefinedSizeScale : 0 ) );
      tempSymbol->startRender( context, vlayer );
      mTempSymbols[ it->value().toString()] = tempSymbol;
    }
  }

}

void QgsCategorizedSymbolRendererV2::stopRender( QgsRenderContext& context )
{
  QgsCategoryList::iterator it = mCategories.begin();
  for ( ; it != mCategories.end(); ++it )
    it->symbol()->stopRender( context );

  // cleanup mTempSymbols
  QHash<QString, QgsSymbolV2*>::iterator it2 = mTempSymbols.begin();
  for ( ; it2 != mTempSymbols.end(); ++it2 )
  {
    it2.value()->stopRender( context );
    delete it2.value();
  }
  mTempSymbols.clear();
}

QList<QString> QgsCategorizedSymbolRendererV2::usedAttributes()
{
  QSet<QString> attributes;
  attributes.insert( mAttrName );
  if ( !mRotationField.isEmpty() )
  {
    attributes.insert( mRotationField );
  }
  if ( !mSizeScaleField.isEmpty() )
  {
    attributes.insert( mSizeScaleField );
  }

  QgsCategoryList::const_iterator catIt = mCategories.constBegin();
  for ( ; catIt != mCategories.constEnd(); ++catIt )
  {
    QgsSymbolV2* catSymbol = catIt->symbol();
    if ( catSymbol )
    {
      attributes.unite( catSymbol->usedAttributes() );
    }
  }
  return attributes.toList();
}

QString QgsCategorizedSymbolRendererV2::dump()
{
  QString s = QString( "CATEGORIZED: idx %1\n" ).arg( mAttrName );
  for ( int i = 0; i < mCategories.count(); i++ )
    s += mCategories[i].dump();
  return s;
}

QgsFeatureRendererV2* QgsCategorizedSymbolRendererV2::clone()
{
  QgsCategorizedSymbolRendererV2* r = new QgsCategorizedSymbolRendererV2( mAttrName, mCategories );
  if ( mSourceSymbol )
    r->setSourceSymbol( mSourceSymbol->clone() );
  if ( mSourceColorRamp )
    r->setSourceColorRamp( mSourceColorRamp->clone() );
  r->setUsingSymbolLevels( usingSymbolLevels() );
  r->setRotationField( rotationField() );
  r->setSizeScaleField( sizeScaleField() );
  r->setScaleMethod( scaleMethod() );
  return r;
}

void QgsCategorizedSymbolRendererV2::toSld( QDomDocument &doc, QDomElement &element ) const
{
  QgsStringMap props;
  props[ "attribute" ] = mAttrName;
  if ( !mRotationField.isEmpty() )
    props[ "angle" ] = QString( mRotationField ).append( "\"" ).prepend( "\"" );
  if ( !mSizeScaleField.isEmpty() )
    props[ "scale" ] = QString( mSizeScaleField ).append( "\"" ).prepend( "\"" );

  // create a Rule for each range
  for ( QgsCategoryList::const_iterator it = mCategories.constBegin(); it != mCategories.constEnd(); it++ )
  {
    QgsStringMap catProps( props );
    it->toSld( doc, element, catProps );
  }
}

QgsSymbolV2List QgsCategorizedSymbolRendererV2::symbols()
{
  QgsSymbolV2List lst;
  for ( int i = 0; i < mCategories.count(); i++ )
    lst.append( mCategories[i].symbol() );
  return lst;
}

QgsFeatureRendererV2* QgsCategorizedSymbolRendererV2::create( QDomElement& element )
{
  QDomElement symbolsElem = element.firstChildElement( "symbols" );
  if ( symbolsElem.isNull() )
    return NULL;

  QDomElement catsElem = element.firstChildElement( "categories" );
  if ( catsElem.isNull() )
    return NULL;

  QgsSymbolV2Map symbolMap = QgsSymbolLayerV2Utils::loadSymbols( symbolsElem );
  QgsCategoryList cats;

  QDomElement catElem = catsElem.firstChildElement();
  while ( !catElem.isNull() )
  {
    if ( catElem.tagName() == "category" )
    {
      QVariant value = QVariant( catElem.attribute( "value" ) );
      QString symbolName = catElem.attribute( "symbol" );
      QString label = catElem.attribute( "label" );
      if ( symbolMap.contains( symbolName ) )
      {
        QgsSymbolV2* symbol = symbolMap.take( symbolName );
        cats.append( QgsRendererCategoryV2( value, symbol, label ) );
      }
    }
    catElem = catElem.nextSiblingElement();
  }

  QString attrName = element.attribute( "attr" );

  QgsCategorizedSymbolRendererV2* r = new QgsCategorizedSymbolRendererV2( attrName, cats );

  // delete symbols if there are any more
  QgsSymbolLayerV2Utils::clearSymbolMap( symbolMap );

  // try to load source symbol (optional)
  QDomElement sourceSymbolElem = element.firstChildElement( "source-symbol" );
  if ( !sourceSymbolElem.isNull() )
  {
    QgsSymbolV2Map sourceSymbolMap = QgsSymbolLayerV2Utils::loadSymbols( sourceSymbolElem );
    if ( sourceSymbolMap.contains( "0" ) )
    {
      r->setSourceSymbol( sourceSymbolMap.take( "0" ) );
    }
    QgsSymbolLayerV2Utils::clearSymbolMap( sourceSymbolMap );
  }

  // try to load color ramp (optional)
  QDomElement sourceColorRampElem = element.firstChildElement( "colorramp" );
  if ( !sourceColorRampElem.isNull() && sourceColorRampElem.attribute( "name" ) == "[source]" )
  {
    r->setSourceColorRamp( QgsSymbolLayerV2Utils::loadColorRamp( sourceColorRampElem ) );
  }

  QDomElement rotationElem = element.firstChildElement( "rotation" );
  if ( !rotationElem.isNull() )
    r->setRotationField( rotationElem.attribute( "field" ) );

  QDomElement sizeScaleElem = element.firstChildElement( "sizescale" );
  if ( !sizeScaleElem.isNull() )
  {
    r->setSizeScaleField( sizeScaleElem.attribute( "field" ) );
    r->setScaleMethod( QgsSymbolLayerV2Utils::decodeScaleMethod( sizeScaleElem.attribute( "scalemethod" ) ) );
  }

  // TODO: symbol levels
  return r;
}

QDomElement QgsCategorizedSymbolRendererV2::save( QDomDocument& doc )
{
  QDomElement rendererElem = doc.createElement( RENDERER_TAG_NAME );
  rendererElem.setAttribute( "type", "categorizedSymbol" );
  rendererElem.setAttribute( "symbollevels", ( mUsingSymbolLevels ? "1" : "0" ) );
  rendererElem.setAttribute( "attr", mAttrName );

  // categories
  int i = 0;
  QgsSymbolV2Map symbols;
  QDomElement catsElem = doc.createElement( "categories" );
  QgsCategoryList::const_iterator it = mCategories.constBegin();
  for ( ; it != mCategories.end(); it++ )
  {
    const QgsRendererCategoryV2& cat = *it;
    QString symbolName = QString::number( i );
    symbols.insert( symbolName, cat.symbol() );

    QDomElement catElem = doc.createElement( "category" );
    catElem.setAttribute( "value", cat.value().toString() );
    catElem.setAttribute( "symbol", symbolName );
    catElem.setAttribute( "label", cat.label() );
    catsElem.appendChild( catElem );
    i++;
  }

  rendererElem.appendChild( catsElem );

  // save symbols
  QDomElement symbolsElem = QgsSymbolLayerV2Utils::saveSymbols( symbols, "symbols", doc );
  rendererElem.appendChild( symbolsElem );

  // save source symbol
  if ( mSourceSymbol )
  {
    QgsSymbolV2Map sourceSymbols;
    sourceSymbols.insert( "0", mSourceSymbol );
    QDomElement sourceSymbolElem = QgsSymbolLayerV2Utils::saveSymbols( sourceSymbols, "source-symbol", doc );
    rendererElem.appendChild( sourceSymbolElem );
  }

  // save source color ramp
  if ( mSourceColorRamp )
  {
    QDomElement colorRampElem = QgsSymbolLayerV2Utils::saveColorRamp( "[source]", mSourceColorRamp, doc );
    rendererElem.appendChild( colorRampElem );
  }

  QDomElement rotationElem = doc.createElement( "rotation" );
  rotationElem.setAttribute( "field", mRotationField );
  rendererElem.appendChild( rotationElem );

  QDomElement sizeScaleElem = doc.createElement( "sizescale" );
  sizeScaleElem.setAttribute( "field", mSizeScaleField );
  sizeScaleElem.setAttribute( "scalemethod", QgsSymbolLayerV2Utils::encodeScaleMethod( mScaleMethod ) );
  rendererElem.appendChild( sizeScaleElem );

  return rendererElem;
}

QgsLegendSymbologyList QgsCategorizedSymbolRendererV2::legendSymbologyItems( QSize iconSize )
{
  QSettings settings;
  bool showClassifiers = settings.value( "/qgis/showLegendClassifiers", false ).toBool();

  QgsLegendSymbologyList lst;
  if ( showClassifiers )
  {
    lst << qMakePair( classAttribute(), QPixmap() );
  }

  int count = categories().count();
  for ( int i = 0; i < count; i++ )
  {
    const QgsRendererCategoryV2& cat = categories()[i];
    QPixmap pix = QgsSymbolLayerV2Utils::symbolPreviewPixmap( cat.symbol(), iconSize );
    lst << qMakePair( cat.label(), pix );
  }
  return lst;
}

QgsLegendSymbolList QgsCategorizedSymbolRendererV2::legendSymbolItems()
{
  QSettings settings;
  bool showClassifiers = settings.value( "/qgis/showLegendClassifiers", false ).toBool();

  QgsLegendSymbolList lst;
  if ( showClassifiers )
  {
    lst << qMakePair( classAttribute(), ( QgsSymbolV2* )0 );
  }

  foreach ( const QgsRendererCategoryV2& cat, mCategories )
  {
    lst << qMakePair( cat.label(), cat.symbol() );
  }
  return lst;
}


QgsSymbolV2* QgsCategorizedSymbolRendererV2::sourceSymbol()
{
  return mSourceSymbol;
}
void QgsCategorizedSymbolRendererV2::setSourceSymbol( QgsSymbolV2* sym )
{
  delete mSourceSymbol;
  mSourceSymbol = sym;
}

QgsVectorColorRampV2* QgsCategorizedSymbolRendererV2::sourceColorRamp()
{
  return mSourceColorRamp;
}
void QgsCategorizedSymbolRendererV2::setSourceColorRamp( QgsVectorColorRampV2* ramp )
{
  delete mSourceColorRamp;
  mSourceColorRamp = ramp;
}

void QgsCategorizedSymbolRendererV2::updateSymbols( QgsSymbolV2 * sym )
{
  int i = 0;
  foreach ( QgsRendererCategoryV2 cat, mCategories )
  {
    QgsSymbolV2* symbol = sym->clone();
    symbol->setColor( cat.symbol()->color() );
    updateCategorySymbol( i, symbol );
    ++i;
  }
}

void QgsCategorizedSymbolRendererV2::setScaleMethod( QgsSymbolV2::ScaleMethod scaleMethod )
{
  mScaleMethod = scaleMethod;
  QgsCategoryList::const_iterator catIt = mCategories.constBegin();
  for ( ; catIt != mCategories.constEnd(); ++catIt )
  {
    setScaleMethodToSymbol( catIt->symbol(), scaleMethod );
  }
}
