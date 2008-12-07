/***************************************************************************
                         qgssinglesymbolrenderer.cpp  -  description
                             -------------------
    begin                : Oct 2003
    copyright            : (C) 2003 by Marco Hugentobler
    email                : mhugent@geo.unizh.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id: qgsgraduatedsymbolrenderer.cpp 5371 2006-04-25 01:52:13Z wonder $ */

#include "qgis.h"
#include "qgslogger.h"
#include "qgsfeature.h"
#include "qgsgraduatedsymbolrenderer.h"
#include "qgssymbol.h"
#include "qgssymbologyutils.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"
#include <math.h>
#include <QDomNode>
#include <QDomElement>
#include <QImage>
#include <QPainter>


QgsGraduatedSymbolRenderer::QgsGraduatedSymbolRenderer( QGis::GeometryType type, Mode mode )
{
  mGeometryType = type;
}

QgsGraduatedSymbolRenderer::QgsGraduatedSymbolRenderer( const QgsGraduatedSymbolRenderer& other )
{
  mMode = other.mMode;
  mGeometryType = other.mGeometryType;
  mClassificationField = other.mClassificationField;
  const QList<QgsSymbol*> s = other.symbols();
  for ( QList<QgsSymbol*>::const_iterator it = s.begin(); it != s.end(); ++it )
  {
    addSymbol( new QgsSymbol( **it ) );
  }
  updateSymbolAttributes();
}

QgsGraduatedSymbolRenderer& QgsGraduatedSymbolRenderer::operator=( const QgsGraduatedSymbolRenderer & other )
{
  if ( this != &other )
  {
    mMode = other.mMode;
    mGeometryType = other.mGeometryType;
    mClassificationField = other.mClassificationField;
    removeSymbols();
    const QList<QgsSymbol*> s = other.symbols();
    for ( QList<QgsSymbol*>::const_iterator it = s.begin(); it != s.end(); ++it )
    {
      addSymbol( new QgsSymbol( **it ) );
    }
    updateSymbolAttributes();
  }

  return *this;
}

QgsGraduatedSymbolRenderer::~QgsGraduatedSymbolRenderer()
{

}


QgsGraduatedSymbolRenderer::Mode QgsGraduatedSymbolRenderer::mode() const
{
  //mode is only really used to be able to reinstate
  //the graduated dialog properties properly, so we
  //dont do anything else besides accessors and mutators in
  //this class
  return mMode;
}

void QgsGraduatedSymbolRenderer::setMode( QgsGraduatedSymbolRenderer::Mode theMode )
{
  //mode is only really used to be able to reinstate
  //the graduated dialog properties properly, so we
  //dont do anything else besides accessors and mutators in
  //this class
  mMode = theMode;
}

const QList<QgsSymbol*> QgsGraduatedSymbolRenderer::symbols() const
{
  return mSymbols;
}

void QgsGraduatedSymbolRenderer::removeSymbols()
{
  //free the memory first
  for ( QList<QgsSymbol*>::iterator it = mSymbols.begin(); it != mSymbols.end(); ++it )
  {
    delete *it;
  }

  //and remove the pointers then
  mSymbols.clear();
  updateSymbolAttributes();
}

bool QgsGraduatedSymbolRenderer::willRenderFeature( QgsFeature *f )
{
  return ( symbolForFeature( f ) != 0 );
}

void QgsGraduatedSymbolRenderer::renderFeature( QPainter * p, QgsFeature & f, QImage* img, bool selected, double widthScale, double rasterScaleFactor )
{
  QgsSymbol* theSymbol = symbolForFeature( &f );
  if ( !theSymbol )
  {
    if ( img && mGeometryType == QGis::Point )
    {
      img->fill( 0 );
    }
    else if ( mGeometryType != QGis::Point )
    {
      p->setPen( Qt::NoPen );
      p->setBrush( Qt::NoBrush );
    }
    return;
  }

  //set the qpen and qpainter to the right values
  // Point
  if ( img && mGeometryType == QGis::Point )
  {
    double fieldScale = 1.0;
    double rotation = 0.0;

    if ( theSymbol->scaleClassificationField() >= 0 )
    {
      //first find out the value for the scale classification attribute
      const QgsAttributeMap& attrs = f.attributeMap();
      fieldScale = sqrt( fabs( attrs[theSymbol->scaleClassificationField()].toDouble() ) );
      QgsDebugMsg( QString( "Feature has field scale factor %1" ).arg( fieldScale ) );
    }
    if ( theSymbol->rotationClassificationField() >= 0 )
    {
      const QgsAttributeMap& attrs = f.attributeMap();
      rotation = attrs[theSymbol->rotationClassificationField()].toDouble();
      QgsDebugMsg( QString( "Feature has rotation factor %1" ).arg( rotation ) );
    }
    *img = theSymbol->getPointSymbolAsImage( widthScale, selected, mSelectionColor, fieldScale, rotation, rasterScaleFactor );
  }

  // Line, polygon
  if ( mGeometryType != QGis::Point )
  {
    if ( !selected )
    {
      QPen pen = theSymbol->pen();
      pen.setWidthF( widthScale * pen.widthF() );
      p->setPen( pen );

      if ( mGeometryType == QGis::Polygon )
      {
        QBrush brush = theSymbol->brush();
        scaleBrush( brush, rasterScaleFactor ); //scale brush content for printout
        p->setBrush( brush );
      }
    }
    else
    {
      QPen pen = theSymbol->pen();
      pen.setWidthF( widthScale * pen.widthF() );

      if ( mGeometryType == QGis::Polygon )
      {
        QBrush brush = theSymbol->brush();
        scaleBrush( brush, rasterScaleFactor ); //scale brush content for printout
        brush.setColor( mSelectionColor );
        p->setBrush( brush );
      }
      else //dont draw outlines in selection colour for polys otherwise they appear merged
      {
        pen.setColor( mSelectionColor );
      }
      p->setPen( pen );
    }
  }
}

QgsSymbol *QgsGraduatedSymbolRenderer::symbolForFeature( const QgsFeature* f )
{
  //first find out the value for the classification attribute
  const QgsAttributeMap& attrs = f->attributeMap();
  double value = attrs[mClassificationField].toDouble();

  QList<QgsSymbol*>::iterator it;
  //find the first render item which contains the feature
  for ( it = mSymbols.begin(); it != mSymbols.end(); ++it )
  {
    if ( value >= ( *it )->lowerValue().toDouble() && value <= ( *it )->upperValue().toDouble() )
    {
      break;
    }
  }

  if ( it == mSymbols.end() )    //only draw features which are covered by a render item
  {
    return 0;
  }
  return ( *it );
}

int QgsGraduatedSymbolRenderer::readXML( const QDomNode& rnode, QgsVectorLayer& vl )
{
  mGeometryType = vl.geometryType();
  QDomNode modeNode = rnode.namedItem( "mode" );
  QString modeValue = modeNode.toElement().text();
  QDomNode classnode = rnode.namedItem( "classificationfield" );
  QString classificationField = classnode.toElement().text();

  QgsVectorDataProvider* theProvider = vl.dataProvider();
  if ( !theProvider )
  {
    return 1;
  }
  if ( modeValue == "Empty" )
  {
    mMode = QgsGraduatedSymbolRenderer::Empty;
  }
  else if ( modeValue == "Quantile" )
  {
    mMode = QgsGraduatedSymbolRenderer::Quantile;
  }
  else //default
  {
    mMode = QgsGraduatedSymbolRenderer::EqualInterval;
  }

  int classificationId = theProvider->fieldNameIndex( classificationField );
  if ( classificationId == -1 )
  {
    return 2; //@todo: handle gracefully in gui situation where user needs to nominate field
  }
  setClassificationField( classificationId );

  QDomNode symbolnode = rnode.namedItem( "symbol" );
  while ( !symbolnode.isNull() )
  {
    QgsSymbol* sy = new QgsSymbol( mGeometryType );
    sy->readXML( symbolnode, &vl );
    addSymbol( sy );

    symbolnode = symbolnode.nextSibling();
  }
  updateSymbolAttributes();
  vl.setRenderer( this );
  return 0;
}

QgsAttributeList QgsGraduatedSymbolRenderer::classificationAttributes() const
{
  QgsAttributeList list( mSymbolAttributes );
  if ( ! list.contains( mClassificationField ) )
  {
    list.append( mClassificationField );
  }
  return list;
}

void QgsGraduatedSymbolRenderer::updateSymbolAttributes()
{
  // This function is only called after changing field specifier in the GUI.
  // Timing is not so important.

  mSymbolAttributes.clear();

  QList<QgsSymbol*>::iterator it;
  for ( it = mSymbols.begin(); it != mSymbols.end(); ++it )
  {
    int rotationField = ( *it )->rotationClassificationField();
    if ( rotationField >= 0 && !( mSymbolAttributes.contains( rotationField ) ) )
    {
      mSymbolAttributes.append( rotationField );
    }
    int scaleField = ( *it )->scaleClassificationField();
    if ( scaleField >= 0 && !( mSymbolAttributes.contains( scaleField ) ) )
    {
      mSymbolAttributes.append( scaleField );
    }
  }
}

QString QgsGraduatedSymbolRenderer::name() const
{
  return "Graduated Symbol";
}

bool QgsGraduatedSymbolRenderer::writeXML( QDomNode & layer_node, QDomDocument & document, const QgsVectorLayer& vl ) const
{
  bool returnval = true;
  QDomElement graduatedsymbol = document.createElement( "graduatedsymbol" );
  layer_node.appendChild( graduatedsymbol );

  //
  // Mode field first ...
  //

  QString modeValue = "";
  if ( mMode == QgsGraduatedSymbolRenderer::Empty )
  {
    modeValue == "Empty";
  }
  else if ( QgsGraduatedSymbolRenderer::Quantile )
  {
    modeValue = "Quantile";
  }
  else //default
  {
    modeValue = "Equal Interval";
  }
  QDomElement modeElement = document.createElement( "mode" );
  QDomText modeText = document.createTextNode( modeValue );
  modeElement.appendChild( modeText );
  graduatedsymbol.appendChild( modeElement );



  //
  // classification field now ...
  //

  QDomElement classificationfield = document.createElement( "classificationfield" );

  const QgsVectorDataProvider* theProvider = vl.dataProvider();
  if ( !theProvider )
  {
    return false;
  }

  QString classificationFieldName;
  if ( vl.pendingFields().contains( mClassificationField ) )
  {
    classificationFieldName = vl.pendingFields()[ mClassificationField ].name();
  }

  QDomText classificationfieldtxt = document.createTextNode( classificationFieldName );
  classificationfield.appendChild( classificationfieldtxt );
  graduatedsymbol.appendChild( classificationfield );
  for ( QList<QgsSymbol*>::const_iterator it = mSymbols.begin(); it != mSymbols.end(); ++it )
  {
    if ( !( *it )->writeXML( graduatedsymbol, document, &vl ) )
    {
      returnval = false;
    }
  }
  return returnval;
}

QgsRenderer* QgsGraduatedSymbolRenderer::clone() const
{
  QgsGraduatedSymbolRenderer* r = new QgsGraduatedSymbolRenderer( *this );
  return r;
}
