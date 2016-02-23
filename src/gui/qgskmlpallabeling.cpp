#include "qgskmlpallabeling.h"
#include "qgskmlexport.h"
#include "qgspalgeometry.h"

#include "pal/pointset.h"
#include "pal/labelposition.h"

QgsKMLPalLabeling::QgsKMLPalLabeling( QTextStream* outStream, const QgsRectangle& bbox, double scale, QGis::UnitType mapUnits ): mOutStream( outStream )
{
  mSettings = new QgsMapSettings;
  mSettings->setMapUnits( mapUnits );
  mSettings->setExtent( bbox );

  //todo: unify with QgsDXFLabelingEngine
  int dpi = 96;
  double factor = 1000 * dpi / scale / 25.4 * QGis::fromUnitToUnitFactor( mapUnits, QGis::Meters );
  mSettings->setOutputSize( QSize( bbox.width() * factor, bbox.height() * factor ) );
  mSettings->setOutputDpi( dpi );
  mSettings->setCrsTransformEnabled( false );
  init( *mSettings );

  mImage = new QImage( 10, 10, QImage::Format_ARGB32_Premultiplied );
  mImage->setDotsPerMeterX( 96 / 25.4 * 1000 );
  mImage->setDotsPerMeterY( 96 / 25.4 * 1000 );
  mPainter = new QPainter( mImage );
  mRenderContext.setPainter( mPainter );
  mRenderContext.setRendererScale( scale );
  mRenderContext.setExtent( bbox );
  mRenderContext.setScaleFactor( 96.0 / 25.4 );
  mRenderContext.setMapToPixel( QgsMapToPixel( 1.0 / factor, bbox.xMinimum(), bbox.yMinimum(), bbox.height() * factor ) );
}

QgsKMLPalLabeling::~QgsKMLPalLabeling()
{
  delete mPainter;
  delete mImage;
  delete mSettings;
}

void QgsKMLPalLabeling::drawLabel( pal::LabelPosition* label, QgsRenderContext& context, QgsPalLayerSettings& tmpLyr, DrawLabelType drawType, double dpiRatio )
{
  if ( !mOutStream )
  {
    return;
  }

  QgsPalGeometry *g = dynamic_cast< QgsPalGeometry* >( label->getFeaturePart()->getUserGeometry() );
  if ( !g )
  {
    return;
  }

  QColor fontColor = tmpLyr.textColor;

  double labelMidPointX = label->getX() + label->getWidth() / 2.0;
  double labelMidPointY = label->getY() + label->getHeight() / 2.0;

  ( *mOutStream ) << QString( "<Placemark><name>%1</name><Style><IconStyle><scale>0</scale></IconStyle><LabelStyle><color>%2</color></LabelStyle></Style><Point><coordinates>%3,%4</coordinates></Point></Placemark>" )
  .arg( g->text( label->getPartId() ) ).arg( QgsKMLExport::convertColor( fontColor ) )
  .arg( QString::number( labelMidPointX ) ).arg( QString::number( labelMidPointY ) );
  ( *mOutStream ) << "\n";
}
