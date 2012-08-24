/* **************************************************************************
                qgsrastertransparency.cpp -  description
                       -------------------
begin                : Mon Nov 30 2007
copyright            : (C) 2007 by Peter J. Ersts
email                : ersts@amnh.org

****************************************************************************/

/* **************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsrastertransparency.h"
#include "qgis.h"
#include "qgslogger.h"

#include <QDomDocument>
#include <QDomElement>

QgsRasterTransparency::QgsRasterTransparency()
{

}

/**
  Accessor for transparentSingleValuePixelList
*/
QList<QgsRasterTransparency::TransparentSingleValuePixel> QgsRasterTransparency::transparentSingleValuePixelList() const
{
  return mTransparentSingleValuePixelList;
}

/**
  Accessor for transparentThreeValuePixelList
*/
QList<QgsRasterTransparency::TransparentThreeValuePixel> QgsRasterTransparency::transparentThreeValuePixelList() const
{
  return mTransparentThreeValuePixelList;
}

/**
  Reset to the transparency list to a single value
*/
void QgsRasterTransparency::initializeTransparentPixelList( double theValue )
{
  //clear the existing list
  mTransparentSingleValuePixelList.clear();

  //add the initial value
  TransparentSingleValuePixel myTransparentSingleValuePixel;
  myTransparentSingleValuePixel.min = theValue;
  myTransparentSingleValuePixel.max = theValue;
  myTransparentSingleValuePixel.percentTransparent = 100.0;
  mTransparentSingleValuePixelList.append( myTransparentSingleValuePixel );
}

/**
  Reset to the transparency list to a single value
*/
void QgsRasterTransparency::initializeTransparentPixelList( double theRedValue, double theGreenValue, double theBlueValue )
{
  //clearn the existing list
  mTransparentThreeValuePixelList.clear();

  //add the initial values
  TransparentThreeValuePixel myTransparentThreeValuePixel;
  myTransparentThreeValuePixel.red = theRedValue;
  myTransparentThreeValuePixel.green = theGreenValue;
  myTransparentThreeValuePixel.blue = theBlueValue;
  myTransparentThreeValuePixel.percentTransparent = 100.0;
  mTransparentThreeValuePixelList.append( myTransparentThreeValuePixel );
}


/**
  Mutator for transparentSingleValuePixelList, replaces the whole list
*/
void QgsRasterTransparency::setTransparentSingleValuePixelList( QList<QgsRasterTransparency::TransparentSingleValuePixel> theNewList )
{
  mTransparentSingleValuePixelList = theNewList;
}

/**
  Mutator for transparentThreeValuePixelList, replaces the whole list
*/
void QgsRasterTransparency::setTransparentThreeValuePixelList( QList<QgsRasterTransparency::TransparentThreeValuePixel> theNewList )
{
  mTransparentThreeValuePixelList = theNewList;
}

/**
  Searches through the transparency list, if a match is found, the global transparency value is scaled
  by the stored transparency value.
  @param theValue the needle to search for in the transparency hay stack
  @param theGlobalTransparency  the overal transparency level for the layer
*/
int QgsRasterTransparency::alphaValue( double theValue, int theGlobalTransparency ) const
{
  //if NaN return 0, transparent
  if ( theValue != theValue )
  {
    return 0;
  }

  //Search through the transparency list looking for a match
  bool myTransparentPixelFound = false;
  TransparentSingleValuePixel myTransparentPixel = {0, 0, 100};
  for ( int myListRunner = 0; myListRunner < mTransparentSingleValuePixelList.count(); myListRunner++ )
  {
    myTransparentPixel = mTransparentSingleValuePixelList[myListRunner];
    if ( theValue >= myTransparentPixel.min && theValue <= myTransparentPixel.max )
    {
      myTransparentPixelFound = true;
      break;
    }
  }

  //if a match was found use the stored transparency percentage
  if ( myTransparentPixelFound )
  {
    return ( int )(( float )theGlobalTransparency *( 1.0 - ( myTransparentPixel.percentTransparent / 100.0 ) ) );
  }

  return theGlobalTransparency;
}

/**
  Searches through the transparency list, if a match is found, the global transparency value is scaled
  by the stored transparency value.
  @param theRedValue the red portion of the needle to search for in the transparency hay stack
  @param theGreenValue  the green portion of the needle to search for in the transparency hay stack
  @param theBlueValue the green portion of the needle to search for in the transparency hay stack
  @param theGlobalTransparency  the overal transparency level for the layer
*/
int QgsRasterTransparency::alphaValue( double theRedValue, double theGreenValue, double theBlueValue, int theGlobalTransparency ) const
{
  //if NaN return 0, transparent
  if ( theRedValue != theRedValue || theGreenValue != theGreenValue || theBlueValue != theBlueValue )
  {
    return 0;
  }

  //Search through the transparency list looking for a match
  bool myTransparentPixelFound = false;
  TransparentThreeValuePixel myTransparentPixel = {0, 0, 0, 100};
  for ( int myListRunner = 0; myListRunner < mTransparentThreeValuePixelList.count(); myListRunner++ )
  {
    myTransparentPixel = mTransparentThreeValuePixelList[myListRunner];
    if ( myTransparentPixel.red == theRedValue )
    {
      if ( myTransparentPixel.green == theGreenValue )
      {
        if ( myTransparentPixel.blue == theBlueValue )
        {
          myTransparentPixelFound = true;
          break;
        }
      }
    }
  }

  //if a match was found use the stored transparency percentage
  if ( myTransparentPixelFound )
  {
    return ( int )(( float )theGlobalTransparency *( 1.0 - ( myTransparentPixel.percentTransparent / 100.0 ) ) );
  }

  return theGlobalTransparency;
}

bool QgsRasterTransparency::isEmpty( double nodataValue ) const
{
  return (
           ( mTransparentSingleValuePixelList.isEmpty() ||
             ( mTransparentSingleValuePixelList.size() == 1 &&
               ( doubleNear( mTransparentSingleValuePixelList.at( 0 ).min, nodataValue ) ||
                 doubleNear( mTransparentSingleValuePixelList.at( 0 ).max, nodataValue ) ||
                 ( nodataValue > mTransparentSingleValuePixelList.at( 0 ).min &&
                   nodataValue < mTransparentSingleValuePixelList.at( 0 ).max ) ) ) )
           &&
           ( mTransparentThreeValuePixelList.isEmpty() ||
             ( mTransparentThreeValuePixelList.size() == 1 &&
               doubleNear( mTransparentThreeValuePixelList.at( 0 ).red, nodataValue ) &&
               doubleNear( mTransparentThreeValuePixelList.at( 0 ).green, nodataValue ) &&
               doubleNear( mTransparentThreeValuePixelList.at( 0 ).blue, nodataValue ) ) ) );
}

void QgsRasterTransparency::writeXML( QDomDocument& doc, QDomElement& parentElem ) const
{
  QDomElement rasterTransparencyElem = doc.createElement( "rasterTransparency" );
  if ( mTransparentSingleValuePixelList.count() > 0 )
  {
    QDomElement singleValuePixelListElement = doc.createElement( "singleValuePixelList" );
    QList<QgsRasterTransparency::TransparentSingleValuePixel>::const_iterator it = mTransparentSingleValuePixelList.constBegin();
    for ( ; it != mTransparentSingleValuePixelList.constEnd(); ++it )
    {
      QDomElement pixelListElement = doc.createElement( "pixelListEntry" );
      //pixelListElement.setAttribute( "pixelValue", QString::number( it->pixelValue, 'f' ) );
      pixelListElement.setAttribute( "min", QString::number( it->min, 'f' ) );
      pixelListElement.setAttribute( "max", QString::number( it->max, 'f' ) );
      pixelListElement.setAttribute( "percentTransparent", QString::number( it->percentTransparent ) );
      singleValuePixelListElement.appendChild( pixelListElement );
    }
    rasterTransparencyElem.appendChild( singleValuePixelListElement );

  }
  if ( mTransparentThreeValuePixelList.count() > 0 )
  {
    QDomElement threeValuePixelListElement = doc.createElement( "threeValuePixelList" );
    QList<QgsRasterTransparency::TransparentThreeValuePixel>::const_iterator it = mTransparentThreeValuePixelList.constBegin();
    for ( ; it != mTransparentThreeValuePixelList.constEnd(); ++it )
    {
      QDomElement pixelListElement = doc.createElement( "pixelListEntry" );
      pixelListElement.setAttribute( "red", QString::number( it->red, 'f' ) );
      pixelListElement.setAttribute( "green", QString::number( it->green, 'f' ) );
      pixelListElement.setAttribute( "blue", QString::number( it->blue, 'f' ) );
      pixelListElement.setAttribute( "percentTransparent", QString::number( it->percentTransparent ) );
      threeValuePixelListElement.appendChild( pixelListElement );
    }
    rasterTransparencyElem.appendChild( threeValuePixelListElement );
  }
  parentElem.appendChild( rasterTransparencyElem );
}

void QgsRasterTransparency::readXML( const QDomElement& elem )
{
  if ( elem.isNull() )
  {
    return;
  }

  mTransparentSingleValuePixelList.clear();
  mTransparentThreeValuePixelList.clear();
  QDomElement currentEntryElem;

  QDomElement singlePixelListElem = elem.firstChildElement( "singleValuePixelList" );
  if ( !singlePixelListElem.isNull() )
  {
    QDomNodeList entryList = singlePixelListElem.elementsByTagName( "pixelListEntry" );
    TransparentSingleValuePixel sp;
    for ( int i = 0; i < entryList.size(); ++i )
    {
      currentEntryElem = entryList.at( i ).toElement();
      sp.percentTransparent = currentEntryElem.attribute( "percentTransparent" ).toDouble();
      // Backward compoatibility < 1.9 : pixelValue (before ranges)
      if ( currentEntryElem.hasAttribute( "pixelValue" ) )
      {
        sp.min = sp.max = currentEntryElem.attribute( "pixelValue" ).toDouble();
      }
      else
      {
        sp.min = currentEntryElem.attribute( "min" ).toDouble();
        sp.max = currentEntryElem.attribute( "max" ).toDouble();
      }
      mTransparentSingleValuePixelList.append( sp );
    }
  }
  QDomElement threeValuePixelListElem = elem.firstChildElement( "threeValuePixelList" );
  if ( !threeValuePixelListElem.isNull() )
  {
    QDomNodeList entryList = threeValuePixelListElem.elementsByTagName( "pixelListEntry" );
    TransparentThreeValuePixel tp;
    for ( int i = 0; i < entryList.size(); ++i )
    {
      currentEntryElem = entryList.at( i ).toElement();
      tp.red = currentEntryElem.attribute( "red" ).toDouble();
      tp.green = currentEntryElem.attribute( "green" ).toDouble();
      tp.blue = currentEntryElem.attribute( "blue" ).toDouble();
      tp.percentTransparent = currentEntryElem.attribute( "percentTransparent" ).toDouble();
      mTransparentThreeValuePixelList.append( tp );
    }
  }
}
