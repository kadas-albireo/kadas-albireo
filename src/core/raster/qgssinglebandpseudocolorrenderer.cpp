/***************************************************************************
                         qgssinglebandpseudocolorrenderer.cpp
                         ------------------------------------
    begin                : January 2012
    copyright            : (C) 2012 by Marco Hugentobler
    email                : marco at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgssinglebandpseudocolorrenderer.h"
#include "qgsrastershader.h"
#include "qgsrastertransparency.h"
#include "qgsrasterviewport.h"
#include <QDomDocument>
#include <QDomElement>
#include <QImage>

QgsSingleBandPseudoColorRenderer::QgsSingleBandPseudoColorRenderer( QgsRasterInterface* input, int band, QgsRasterShader* shader ):
    QgsRasterRenderer( input, "singlebandpseudocolor" )
    , mShader( shader )
    , mBand( band )
    , mClassificationMin( std::numeric_limits<double>::quiet_NaN() )
    , mClassificationMax( std::numeric_limits<double>::quiet_NaN() )
    , mClassificationMinMaxOrigin( QgsRasterRenderer::MinMaxUnknown )
{
}

QgsSingleBandPseudoColorRenderer::~QgsSingleBandPseudoColorRenderer()
{
  delete mShader;
}

QgsRasterInterface * QgsSingleBandPseudoColorRenderer::clone() const
{
  QgsRasterShader *shader = 0;

  if ( mShader )
  {
    shader = new QgsRasterShader( mShader->minimumValue(), mShader->maximumValue() );

    // Shader function
    const QgsColorRampShader* origColorRampShader = dynamic_cast<const QgsColorRampShader*>( mShader->rasterShaderFunction() );

    if ( origColorRampShader )
    {
      QgsColorRampShader * colorRampShader = new QgsColorRampShader( mShader->minimumValue(), mShader->maximumValue() );

      colorRampShader->setColorRampType( origColorRampShader->colorRampType() );

      colorRampShader->setColorRampItemList( origColorRampShader->colorRampItemList() );
      shader->setRasterShaderFunction( colorRampShader );
    }
  }
  QgsSingleBandPseudoColorRenderer * renderer = new QgsSingleBandPseudoColorRenderer( 0, mBand, shader );

  renderer->setOpacity( mOpacity );
  renderer->setAlphaBand( mAlphaBand );
  renderer->setRasterTransparency( mRasterTransparency ? new QgsRasterTransparency( *mRasterTransparency ) : 0 );

  return renderer;
}

void QgsSingleBandPseudoColorRenderer::setShader( QgsRasterShader* shader )
{
  delete mShader;
  mShader = shader;
}

QgsRasterRenderer* QgsSingleBandPseudoColorRenderer::create( const QDomElement& elem, QgsRasterInterface* input )
{
  if ( elem.isNull() )
  {
    return 0;
  }

  int band = elem.attribute( "band", "-1" ).toInt();
  QgsRasterShader* shader = 0;
  QDomElement rasterShaderElem = elem.firstChildElement( "rastershader" );
  if ( !rasterShaderElem.isNull() )
  {
    shader = new QgsRasterShader();
    shader->readXML( rasterShaderElem );
  }

  QgsSingleBandPseudoColorRenderer* r = new QgsSingleBandPseudoColorRenderer( input, band, shader );
  r->readXML( elem );

  // TODO: add _readXML in superclass?
  r->setClassificationMin( elem.attribute( "classificationMin", "NaN" ).toDouble() );
  r->setClassificationMax( elem.attribute( "classificationMax", "NaN" ).toDouble() );
  r->setClassificationMinMaxOrigin( QgsRasterRenderer::minMaxOriginFromName( elem.attribute( "classificationMinMaxOrigin", "Unknown" ) ) );

  return r;
}

QgsRasterBlock* QgsSingleBandPseudoColorRenderer::block( int bandNo, QgsRectangle  const & extent, int width, int height )
{
  Q_UNUSED( bandNo );

  QgsRasterBlock *outputBlock = new QgsRasterBlock();
  if ( !mInput || !mShader )
  {
    return outputBlock;
  }


  QgsRasterBlock *inputBlock = mInput->block( mBand, extent, width, height );
  if ( !inputBlock || inputBlock->isEmpty() )
  {
    QgsDebugMsg( "No raster data!" );
    delete inputBlock;
    return outputBlock;
  }

  //rendering is faster without considering user-defined transparency
  bool hasTransparency = usesTransparency();

  QgsRasterBlock *alphaBlock = 0;
  if ( mAlphaBand > 0 && mAlphaBand != mBand )
  {
    alphaBlock = mInput->block( mAlphaBand, extent, width, height );
    if ( !alphaBlock || alphaBlock->isEmpty() )
    {
      delete inputBlock;
      delete alphaBlock;
      return outputBlock;
    }
  }
  else if ( mAlphaBand == mBand )
  {
    alphaBlock = inputBlock;
  }

  if ( !outputBlock->reset( QGis::ARGB32_Premultiplied, width, height ) )
  {
    delete inputBlock;
    delete alphaBlock;
    return outputBlock;
  }

  QRgb myDefaultColor = NODATA_COLOR;

  for ( qgssize i = 0; i < ( qgssize )width*height; i++ )
  {
    if ( inputBlock->isNoData( i ) )
    {
      outputBlock->setColor( i, myDefaultColor );
      continue;
    }
    double val = inputBlock->value( i );
    int red, green, blue, alpha;
    if ( !mShader->shade( val, &red, &green, &blue, &alpha ) )
    {
      outputBlock->setColor( i, myDefaultColor );
      continue;
    }

    if ( alpha < 255 )
    {
      // Working with premultiplied colors, so multiply values by alpha
      red *= ( alpha / 255.0 );
      blue *= ( alpha / 255.0 );
      green *= ( alpha / 255.0 );
    }

    if ( !hasTransparency )
    {
      outputBlock->setColor( i, qRgba( red, green, blue, alpha ) );
    }
    else
    {
      //opacity
      double currentOpacity = mOpacity;
      if ( mRasterTransparency )
      {
        currentOpacity = mRasterTransparency->alphaValue( val, mOpacity * 255 ) / 255.0;
      }
      if ( mAlphaBand > 0 )
      {
        currentOpacity *= alphaBlock->value( i ) / 255.0;
      }

      outputBlock->setColor( i, qRgba( currentOpacity * red, currentOpacity * green, currentOpacity * blue, currentOpacity * alpha ) );
    }
  }

  delete inputBlock;
  if ( mAlphaBand > 0 && mBand != mAlphaBand )
  {
    delete alphaBlock;
  }

  return outputBlock;
}

void QgsSingleBandPseudoColorRenderer::writeXML( QDomDocument& doc, QDomElement& parentElem ) const
{
  if ( parentElem.isNull() )
  {
    return;
  }

  QDomElement rasterRendererElem = doc.createElement( "rasterrenderer" );
  _writeXML( doc, rasterRendererElem );
  rasterRendererElem.setAttribute( "band", mBand );
  if ( mShader )
  {
    mShader->writeXML( doc, rasterRendererElem ); //todo: include color ramp items directly in this renderer
  }
  rasterRendererElem.setAttribute( "classificationMin", QString::number( mClassificationMin ) );
  rasterRendererElem.setAttribute( "classificationMax", QString::number( mClassificationMax ) );
  rasterRendererElem.setAttribute( "classificationMinMaxOrigin", QgsRasterRenderer::minMaxOriginName( mClassificationMinMaxOrigin ) );

  parentElem.appendChild( rasterRendererElem );
}

void QgsSingleBandPseudoColorRenderer::legendSymbologyItems( QList< QPair< QString, QColor > >& symbolItems ) const
{
  if ( mShader )
  {
    QgsRasterShaderFunction* shaderFunction = mShader->rasterShaderFunction();
    if ( shaderFunction )
    {
      shaderFunction->legendSymbologyItems( symbolItems );
    }
  }
}

QList<int> QgsSingleBandPseudoColorRenderer::usesBands() const
{
  QList<int> bandList;
  if ( mBand != -1 )
  {
    bandList << mBand;
  }
  return bandList;
}
