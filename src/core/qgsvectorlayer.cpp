/***************************************************************************
                               qgsvectorlayer.cpp
  This class implements a generic means to display vector layers. The features
  and attributes are read from the data store using a "data provider" plugin.
  QgsVectorLayer can be used with any data store for which an appropriate
  plugin is available.
                              -------------------
          begin                : Oct 29, 2003
          copyright            : (C) 2003 by Gary E.Sherman
          email                : sherman at mrcc.com
 
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*  $Id$ */

#include <cassert>
#include <cfloat>
#include <cstring>
#include <cmath>
#include <iosfwd>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <utility>

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QString>

#include "qgsvectorlayer.h"

// renderers
#include "qgscontinuouscolorrenderer.h"
#include "qgsgraduatedsymbolrenderer.h"
#include "qgsrenderer.h"
#include "qgssinglesymbolrenderer.h"
#include "qgsuniquevaluerenderer.h"

#include "qgsattributeaction.h"

#include "qgis.h" //for globals
#include "qgsapplication.h"
#include "qgscoordinatetransform.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsgeometry.h"
#include "qgsgeometryvertexindex.h"
#include "qgslabel.h"
#include "qgslogger.h"
#include "qgsmaptopixel.h"
#include "qgspoint.h"
#include "qgsproviderregistry.h"
#include "qgsrect.h"
#include "qgssinglesymbolrenderer.h"
#include "qgsspatialrefsys.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorfilewriter.h"

#ifdef Q_WS_X11
#include "qgsclipper.h"
#endif

#ifdef TESTPROVIDERLIB
#include <dlfcn.h>
#endif


static const char * const ident_ = "$Id$";

// typedef for the QgsDataProvider class factory
typedef QgsDataProvider * create_it(const QString* uri);



QgsVectorLayer::QgsVectorLayer(QString vectorLayerPath,
    QString baseName,
    QString providerKey)
: QgsMapLayer(VECTOR, baseName, vectorLayerPath),
  mUpdateThreshold(0),       // XXX better default value?
  mProviderKey(providerKey),
  mEditable(false),
  mModified(false),
  mRenderer(0),
  mLabel(0),
  mLabelOn(false)
{
  mActions = new QgsAttributeAction;
  
  // if we're given a provider type, try to create and bind one to this layer
  if ( ! mProviderKey.isEmpty() )
  {
    setDataProvider( mProviderKey );
  }
  if(mValid)
  {
    setCoordinateSystem();
    
    // add single symbol renderer as default
    QgsSingleSymbolRenderer *renderer = new QgsSingleSymbolRenderer(vectorType());
    setRenderer(renderer);

    // Get the update threshold from user settings. We
    // do this only on construction to avoid the penality of
    // fetching this each time the layer is drawn. If the user
    // changes the threshold from the preferences dialog, it will
    // have no effect on existing layers
    // TODO: load this setting somewhere else [MD]
    //QSettings settings;
    //mUpdateThreshold = settings.readNumEntry("Map/updateThreshold", 1000);
    
  }
} // QgsVectorLayer ctor



QgsVectorLayer::~QgsVectorLayer()
{
  QgsDebugMsg("In QgsVectorLayer destructor");

  mValid=false;

  if (mRenderer)
  {
    delete mRenderer;
  }
  // delete the provider object
  delete mDataProvider;
  
  delete mLabel;

  // Destroy any cached geometries and clear the references to them
  deleteCachedGeometries();

  delete mActions;
}

QString QgsVectorLayer::storageType() const
{
  if (mDataProvider)
  {
    return mDataProvider->storageType();
  }
  return 0;
}


QString QgsVectorLayer::capabilitiesString() const
{
  if (mDataProvider)
  {
    return mDataProvider->capabilitiesString();
  }
  return 0;
}

QString QgsVectorLayer::dataComment() const
{
  if (mDataProvider)
  {
    return mDataProvider->dataComment();
  }
  return QString();
}


QString QgsVectorLayer::providerType() const
{
  return mProviderKey;
}

/**
 * sets the preferred display field based on some fuzzy logic
 */
void QgsVectorLayer::setDisplayField(QString fldName)
{
  // If fldName is provided, use it as the display field, otherwise
  // determine the field index for the feature column of the identify
  // dialog. We look for fields containing "name" first and second for
  // fields containing "id". If neither are found, the first field
  // is used as the node.
  QString idxName="";
  QString idxId="";

  if(!fldName.isEmpty())
  {
    mDisplayField = fldName;
  }
  else
  {
    QgsFieldMap fields = mDataProvider->fields();
    int fieldsSize = fields.size();

    for (QgsFieldMap::iterator it = fields.begin(); it != fields.end(); ++it)
    {

      QString fldName = it.value().name();
      QgsDebugMsg("Checking field " + fldName + " of " + QString::number(fieldsSize) + " total");
      
      // Check the fields and keep the first one that matches.
      // We assume that the user has organized the data with the
      // more "interesting" field names first. As such, name should
      // be selected before oldname, othername, etc.
      if (fldName.find("name", false) > -1)
      {
        if(idxName.isEmpty())
        {
          idxName = fldName;
        }
      }
      if (fldName.find("descrip", false) > -1)
      {
        if(idxName.isEmpty())
        {
          idxName = fldName;
        }
      }
      if (fldName.find("id", false) > -1)
      {
        if(idxId.isEmpty())
        {
          idxId = fldName;
        }
      }
    }

    //if there were no fields in the dbf just return - otherwise qgis segfaults!
    if (fieldsSize == 0)
      return;

    if (idxName.length() > 0)
    {
      mDisplayField = idxName;
    }
    else
    {
      if (idxId.length() > 0)
      {
        mDisplayField = idxId;
      }
      else
      {
        mDisplayField = fields[0].name();
      }
    }

  }
}

void QgsVectorLayer::drawLabels(QPainter * p, QgsRect & viewExtent, QgsMapToPixel * theMapToPixelTransform, QgsCoordinateTransform* ct)
{
  drawLabels(p, viewExtent, theMapToPixelTransform, ct, 1.);
}

// NOTE this is a temporary method added by Tim to prevent label clipping
// which was occurring when labeller was called in the main draw loop
// This method will probably be removed again in the near future!
void QgsVectorLayer::drawLabels(QPainter * p, QgsRect & viewExtent, QgsMapToPixel * theMapToPixelTransform, QgsCoordinateTransform* ct, double scale)
{
  QgsDebugMsg("Starting draw of labels");

  if (mRenderer && mLabelOn)
  {
    QgsAttributeList attributes = mRenderer->classificationAttributes();

    // Add fields required for labels
    mLabel->addRequiredFields( attributes );

    QgsDebugMsg("Selecting features based on view extent");

    int featureCount = 0;
    // select the records in the extent. The provider sets a spatial filter
    // and sets up the selection set for retrieval
    mDataProvider->reset();
    mDataProvider->select(viewExtent);

    try
    {
      QgsFeature fet;

      // main render loop
      while(mDataProvider->getNextFeature(fet, true, attributes))
      {
        // don't render labels of deleted features
        if (!mDeletedFeatureIds.contains(fet.featureId()))
        {
	  if(mRenderer->willRenderFeature(&fet))
	    {
	      bool sel = mSelectedFeatureIds.contains(fet.featureId());
	      mLabel->renderLabel ( p, viewExtent, ct, 
              theMapToPixelTransform, fet, sel, 0, scale);
	    }
        }
        featureCount++;
      }

      // render labels of not-commited features
      for (QgsFeatureList::iterator it = mAddedFeatures.begin(); it != mAddedFeatures.end(); ++it)
      {
	if(mRenderer->willRenderFeature(&(*it)))
	  {
	    bool sel = mSelectedFeatureIds.contains((*it).featureId());
	    mLabel->renderLabel ( p, viewExtent, ct, theMapToPixelTransform, *it, sel, 0, scale);
	  }
      }
    }
    catch (QgsCsException &e)
    {
      QgsLogger::critical("Error projecting label locations, caught in " + QString(__FILE__) + ", line " +QString(__LINE__));
    }

#ifdef QGISDEBUG
    QgsLogger::debug("Total features processed", featureCount, 1, __FILE__, __FUNCTION__, __LINE__);
#endif
    
    // XXX Something in our draw event is triggering an additional draw event when resizing [TE 01/26/06]
    // XXX Calling this will begin processing the next draw event causing image havoc and recursion crashes.
    //qApp->processEvents();

  }
}


unsigned char* QgsVectorLayer::drawLineString(unsigned char* feature, 
    QPainter* p,
    QgsMapToPixel* mtp,
    QgsCoordinateTransform* ct,
    bool drawingToEditingCanvas)
{
  unsigned char *ptr = feature + 5;
  unsigned int wkbType = *((int*)(feature+1));
  unsigned int nPoints = *((int*)ptr);
  ptr = feature + 9;
  
  bool hasZValue = (wkbType == QGis::WKBLineString25D);

  std::vector<double> x(nPoints);
  std::vector<double> y(nPoints);
  std::vector<double> z(nPoints, 0.0);
  
  // Extract the points from the WKB format into the x and y vectors. 
  for (register unsigned int i = 0; i < nPoints; ++i)
  {
    x[i] = *((double *) ptr);
    ptr += sizeof(double);
    y[i] = *((double *) ptr);
    ptr += sizeof(double);
  
    if (hasZValue) // ignore Z value
      ptr += sizeof(double);
  }

  // Transform the points into map coordinates (and reproject if
  // necessary)

  transformPoints(x, y, z, mtp, ct);

#if defined(Q_WS_X11)
  // Work around a +/- 32768 limitation on coordinates in X11

  // Look through the x and y coordinates and see if there are any
  // that need trimming. If one is found, there's no need to look at
  // the rest of them so end the loop at that point. 
  for (register unsigned int i = 0; i < nPoints; ++i)
    if (std::abs(x[i]) > QgsClipper::maxX ||
        std::abs(y[i]) > QgsClipper::maxY)
    {
      QgsClipper::trimFeature(x, y, true); // true = polyline
      nPoints = x.size(); // trimming may change nPoints.
      break;
    }
#endif

  // set up QPolygonF class with transformed points
  QPolygonF pa(nPoints);
  for (register unsigned int i = 0; i < nPoints; ++i)
  {
    pa[i].setX(x[i]);
    pa[i].setY(y[i]);
  }
  
#ifdef QGISDEBUGVERBOSE
  // this is only used for verbose debug output
  for (int i = 0; i < pa.size(); ++i)
    {
      QgsDebugMsgLevel("pa" + QString::number(pa.point(i).x()), 2);
      QgsDebugMsgLevel("pa" + QString::number(pa.point(i).y()), 2);
    }
#endif

  // The default pen gives bevelled joins between segements of the
  // polyline, which is good enough for the moment.
  //preserve a copy of the pen before we start fiddling with it
  QPen pen = p->pen(); // to be kept original
  //
  // experimental alpha transparency
  // 255 = opaque
  //
  QPen myTransparentPen = p->pen(); // store current pen
  QColor myColor = myTransparentPen.color();
  myColor.setAlpha(mTransparencyLevel);
  myTransparentPen.setColor(myColor);
  p->setPen(myTransparentPen);
  p->drawPolyline(pa);

  // draw vertex markers if in editing mode, but only to the main canvas
  if (
      (mEditable) &&
      (drawingToEditingCanvas)
     )
  {
      std::vector<double>::const_iterator xIt;
      std::vector<double>::const_iterator yIt;
      for(xIt = x.begin(), yIt = y.begin(); xIt != x.end(); ++xIt, ++yIt)
	{
	  drawVertexMarker((int)(*xIt), (int)(*yIt), *p);
	}
    }

  //restore the pen
  p->setPen(pen);
  
  return ptr;
}

unsigned char* QgsVectorLayer::drawPolygon(unsigned char* feature, 
    QPainter* p, 
    QgsMapToPixel* mtp, 
    QgsCoordinateTransform* ct,
    bool drawingToEditingCanvas)
{
  typedef std::pair<std::vector<double>, std::vector<double> > ringType;
  typedef ringType* ringTypePtr;
  typedef std::vector<ringTypePtr> ringsType;

  // get number of rings in the polygon
  unsigned int numRings = *((int*)(feature + 1 + sizeof(int)));

  if ( numRings == 0 )  // sanity check for zero rings in polygon
    return feature + 9;

  unsigned int wkbType = *((int*)(feature+1));
  
  bool hasZValue = (wkbType == QGis::WKBPolygon25D);
  
  int total_points = 0;

  // A vector containing a pointer to a pair of double vectors.The
  // first vector in the pair contains the x coordinates, and the
  // second the y coordinates.
  ringsType rings;

  // Set pointer to the first ring
  unsigned char* ptr = feature + 1 + 2 * sizeof(int); 

  for (register unsigned int idx = 0; idx < numRings; idx++)
  {
    unsigned int nPoints = *((int*)ptr);

    ringTypePtr ring = new ringType(std::vector<double>(nPoints),
        std::vector<double>(nPoints));
    ptr += 4;

    // create a dummy vector for the z coordinate
    std::vector<double> zVector(nPoints, 0.0);
    // Extract the points from the WKB and store in a pair of
    // vectors.
    /*
#ifdef QGISDEBUG
std::cerr << "Points for ring " << idx << " ("
<< nPoints << " points)\n";
#endif
*/
    for (register unsigned int jdx = 0; jdx < nPoints; jdx++)
    {
      ring->first[jdx] = *((double *) ptr);
      ptr += sizeof(double);
      ring->second[jdx] = *((double *) ptr);
      ptr += sizeof(double);
      
      if (hasZValue)
        ptr += sizeof(double);
      
      /*
#ifdef QGISDEBUG
std::cerr << jdx << ": " 
<< ring->first[jdx] << ", " << ring->second[jdx] << '\n';
#endif
*/
    }
    // If ring has fewer than two points, what is it then?
    // Anyway, this check prevents a crash
    if (nPoints < 1) 
    {
      QgsDebugMsg("Ring has only " + QString::number(nPoints) + " points! Skipping this ring.");
      continue;
    }

    transformPoints(ring->first, ring->second, zVector, mtp, ct);

#if defined(Q_WS_X11)
    // Work around a +/- 32768 limitation on coordinates in X11

    // Look through the x and y coordinates and see if there are any
    // that need trimming. If one is found, there's no need to look at
    // the rest of them so end the loop at that point. 
    for (register unsigned int i = 0; i < nPoints; ++i)
    {
      if (std::abs(ring->first[i]) > QgsClipper::maxX ||
          std::abs(ring->second[i]) > QgsClipper::maxY)
      {
        QgsClipper::trimFeature(ring->first, ring->second, false);
        /*
#ifdef QGISDEBUG
std::cerr << "Trimmed points (" << ring->first.size() << ")\n";
for (int i = 0; i < ring->first.size(); ++i)
std::cerr << i << ": " << ring->first[i] 
<< ", " << ring->second[i] << '\n';
#endif
*/
        break;
      }
      //std::cout << "POLYGONTRANSFORM: " << ring->first[i] << ", " << ring->second[i] << std::endl; 
    }

#endif

    // Don't bother keeping the ring if it has been trimmed out of
    // existence.
    if (ring->first.size() == 0)
      delete ring;
    else
    {
      rings.push_back(ring);
      total_points += ring->first.size();
    }
  }

  // Now we draw the polygons

  // use painter paths for drawing polygons with holes
  // when adding polygon to the path they invert the area
  // this means that adding inner rings to the path creates
  // holes in outer ring
  QPainterPath path; // OddEven fill rule by default
  
  // Only try to draw polygons if there is something to draw
  if (total_points > 0)
  {
    // Store size here and use it in the loop to avoid penalty of
    // multiple calls to size()
    int numRings = rings.size();
    for (register int i = 0; i < numRings; ++i)
    {
      // Store the pointer in a variable with a short name so as to make
      // the following code easier to type and read.
      ringTypePtr r = rings[i];
      // only do this once to avoid penalty of additional calls
      unsigned ringSize = r->first.size();

      // Transfer points to the array of QPointF
      QPolygonF pa(ringSize);
      for (register unsigned int j = 0; j != ringSize; ++j)
      {
        pa[j].setX(r->first[j]);
        pa[j].setY(r->second[j]);
      }

      path.addPolygon(pa);

      // Tidy up the pointed to pairs of vectors as we finish with them
      delete rings[i];
    }

#ifdef QGISDEBUGVERBOSE
    // this is only for verbose debug output -- no optimzation is 
    // needed :)
    QgsDebugMsg("Pixel points are:");
    for (int i = 0; i < pa.size(); ++i)
      {
	QgsDebugMsgLevel("i" + QString::number(i), 2);
	QgsDebugMsgLevel("pa[i].x()" + QString::number(pa[i].x()), 2);
	QgsDebugMsgLevel("pa[i].y()" + QString::number(pa[i].y()), 2);
      }
    std::cerr << "Ring positions are:\n";
    QgsDebugMsg("Ring positions are:");
    for (int i = 0; i < ringDetails.size(); ++i)
      {
	QgsDebugMsgLevel("ringDetails[i].first" + QString::number(ringDetails[i].first), 2);
	QgsDebugMsgLevel("ringDetails[i].second" + QString::number(ringDetails[i].second), 2);
      }
    QgsDebugMsg("Outer ring point is " + QString::number(outerRingPt.x()) + ", " + QString::number(outerRingPt.y()));
#endif

    /*
    // A bit of code to aid in working out what values of
    // QgsClipper::minX, etc cause the X11 zoom bug.
    int largestX  = -std::numeric_limits<int>::max();
    int smallestX = std::numeric_limits<int>::max();
    int largestY  = -std::numeric_limits<int>::max();
    int smallestY = std::numeric_limits<int>::max();

    for (int i = 0; i < pa.size(); ++i)
    {
    largestX  = std::max(largestX,  pa.point(i).x());
    smallestX = std::min(smallestX, pa.point(i).x());
    largestY  = std::max(largestY,  pa.point(i).y());
    smallestY = std::min(smallestY, pa.point(i).y());
    }
    std::cerr << "Largest  X coordinate was " << largestX  << '\n';
    std::cerr << "Smallest X coordinate was " << smallestX << '\n';
    std::cerr << "Largest  Y coordinate was " << largestY  << '\n';
    std::cerr << "Smallest Y coordinate was " << smallestY << '\n';
    */

    //preserve a copy of the brush and pen before we start fiddling with it
    QBrush brush = p->brush(); //to be kept as original
    QPen pen = p->pen(); // to be kept original
    //
    // experimental alpha transparency
    // 255 = opaque
    //
    QBrush myTransparentBrush = p->brush();
    QColor myColor = brush.color();
    myColor.setAlpha(mTransparencyLevel);
    myTransparentBrush.setColor(myColor);
    QPen myTransparentPen = p->pen(); // store current pen
    myColor = myTransparentPen.color();
    myColor.setAlpha(mTransparencyLevel);
    myTransparentPen.setColor(myColor);
    
    p->setBrush(myTransparentBrush);
    p->setPen (myTransparentPen);
    
    //
    // draw the polygon
    // 
    p->drawPath(path);
    

    // draw vertex markers if in editing mode, but only to the main canvas
    if (
        (mEditable) &&
        (drawingToEditingCanvas)
       )
      {
	for(int i = 0; i < path.elementCount(); ++i)
	  {
            const QPainterPath::Element & e = path.elementAt(i);
	    drawVertexMarker((int)e.x, (int)e.y, *p);
	  }
      }

    //
    //restore brush and pen to original
    //
    p->setBrush ( brush );
    p->setPen ( pen );
  
  } // totalPoints > 0
  
  return ptr;
}


bool QgsVectorLayer::draw(QPainter * p,
                          QgsRect & viewExtent,
                          QgsMapToPixel * theMapToPixelTransform,
                          QgsCoordinateTransform* ct,
                          bool drawingToEditingCanvas)
{
  draw ( p, viewExtent, theMapToPixelTransform, ct, drawingToEditingCanvas, 1., 1.);

  return TRUE; // Assume success always
}

void QgsVectorLayer::draw(QPainter * p,
                          QgsRect & viewExtent,
                          QgsMapToPixel * theMapToPixelTransform,
                          QgsCoordinateTransform* ct,
                          bool drawingToEditingCanvas,
                          double widthScale,
                          double symbolScale)
{
  if (mRenderer)
  {
    // painter is active (begin has been called
    /* Steps to draw the layer
       1. get the features in the view extent by SQL query
       2. read WKB for a feature
       3. transform
       4. draw
       */

    QPen pen;
    /*Pointer to a marker image*/
    QImage marker;
    /*Scale factor of the marker image*/
    double markerScaleFactor=1.;

    if(mEditable)
    {
      // Destroy all cached geometries and clear the references to them
      deleteCachedGeometries();
    }

    mDataProvider->reset();
    mDataProvider->select(viewExtent);
    mDataProvider->updateFeatureCount();
    int totalFeatures = mDataProvider->featureCount();
    int featureCount = 0;
    QgsFeature fet;

    QgsAttributeList attributes = mRenderer->classificationAttributes();

    try
    {
      while (mDataProvider->getNextFeature(fet, true, attributes, mUpdateThreshold))
      {
        // XXX Something in our draw event is triggering an additional draw event when resizing [TE 01/26/06]
        // XXX Calling this will begin processing the next draw event causing image havoc and recursion crashes.
        //qApp->processEvents(); //so we can trap for esc press
        //if (mDrawingCancelled) return;
        // If update threshold is greater than 0, check to see if
        // the threshold has been exceeded
        if(mUpdateThreshold > 0)
        {
          // signal progress in drawing
          if(0 == featureCount % mUpdateThreshold)
            emit drawingProgress(featureCount, totalFeatures);
        }

        if (mEditable)
        {
          // Cache this for the use of (e.g.) modifying the feature's geometry.
          //          mCachedGeometries[fet->featureId()] = fet->geometryAndOwnership();

          if (mDeletedFeatureIds.contains(fet.featureId()))
          {
            continue; //dont't draw feature marked as deleted
          }

          if (mChangedGeometries.contains(fet.featureId()))
          {
            // substitute the committed geometry with the modified one
            fet.setGeometry( mChangedGeometries[ fet.featureId() ] );
          }
            
          // Cache this for the use of (e.g.) modifying the feature's uncommitted geometry.
          mCachedGeometries[fet.featureId()] = *fet.geometry();
        }
                
        // check if feature is selected
        // only show selections of the current layer
        // TODO: create a mechanism to let layer know whether it's current layer or not [MD]
        bool sel;
        if (
            /*(mLegend->currentLayer() == this) && TODO!*/
            (mSelectedFeatureIds.contains(fet.featureId()))
           )
        {
          sel = TRUE;
        }
        else
        {
          sel = FALSE;
        }

        mRenderer->renderFeature(p, fet, &marker, &markerScaleFactor, sel, widthScale );

        double scale = markerScaleFactor * symbolScale;
        drawFeature(p,fet,theMapToPixelTransform,ct, &marker, scale, drawingToEditingCanvas);

        ++featureCount;
      }

      // also draw the not yet commited features
      if (mEditable)
      {
        QgsFeatureList::iterator it = mAddedFeatures.begin();
        for(; it != mAddedFeatures.end(); ++it)
        {
          bool sel = mSelectedFeatureIds.contains((*it).featureId());
          mRenderer->renderFeature(p, *it, &marker, &markerScaleFactor, 
              sel, widthScale);
          double scale = markerScaleFactor * symbolScale;
    
          if (mChangedGeometries.contains((*it).featureId()))
          {
            (*it).setGeometry( mChangedGeometries[(*it).featureId() ] );
          }
          
          // give a deep copy of the geometry to mCachedGeometry because it will be erased at each redraw
          mCachedGeometries.insert((*it).featureId(), QgsGeometry(*((*it).geometry())) );
          drawFeature(p,*it,theMapToPixelTransform,ct, &marker,scale, drawingToEditingCanvas);
        }
      }

    }
    catch (QgsCsException &cse)
    {
      QString msg("Failed to transform a point while drawing a feature of type '"
          + fet.typeName() + "'. Ignoring this feature.");
      msg += cse.what();
      QgsLogger::warning(msg);
    }
    QgsDebugMsg("Total features processed is " + QString::number(featureCount));
    // XXX Something in our draw event is triggering an additional draw event when resizing [TE 01/26/06]
    // XXX Calling this will begin processing the next draw event causing image havoc and recursion crashes.
    //qApp->processEvents();
  }
  else
  {
    QgsLogger::warning("QgsRenderer is null in QgsVectorLayer::draw()");
  }
}

void QgsVectorLayer::cacheGeometries() 
{ 
  if(mDataProvider)
  {
    QgsFeature f;
    
    mDataProvider->reset();
    
    while (mDataProvider->getNextFeature(f))
    {
      mCachedGeometries.insert(f.featureId(), *f.geometry());
    }
  }
}

void QgsVectorLayer::deleteCachedGeometries()
{
  // Destroy any cached geometries
  mCachedGeometries.clear();
}

void QgsVectorLayer::drawVertexMarker(int x, int y, QPainter& p)
{
  //todo: let the user configure the size and appearance of the marker 
  int size = 15;
  int m = (size-1)/2;
  p.drawLine(x-m, y+m, x+m, y-m);
  p.drawLine(x-m, y-m, x+m, y+m);
}

void QgsVectorLayer::select(int number, bool emitSignal)
{
  mSelectedFeatureIds.insert(number);
  
  if (emitSignal)
  {
    emit selectionChanged();
  }
}

void QgsVectorLayer::select(QgsRect & rect, bool lock)
{
  // normalize the rectangle
  rect.normalize();
  
  if (lock == false)
  {
    removeSelection(FALSE); // don't emit signal
  }
  
  mDataProvider->select(rect, true);

  QgsFeature fet;

  while (mDataProvider->getNextFeature(fet))
  {
    // don't select deleted features
    if (!mDeletedFeatureIds.contains(fet.featureId()))
    {
      select(fet.featureId(), FALSE); // don't emit signal
    }
  }

  // also test the not commited features
  for(QgsFeatureList::iterator it = mAddedFeatures.begin(); it != mAddedFeatures.end(); ++it)
  {
    if((*it).geometry()->intersects(rect))
    {
      select((*it).featureId(), FALSE); // don't emit signal
    }
  }

  emit selectionChanged();
}

void QgsVectorLayer::invertSelection()
{
  // copy the ids of selected features to tmp
  QgsFeatureIds tmp;
  for(QgsFeatureIds::iterator iter = mSelectedFeatureIds.begin(); iter != mSelectedFeatureIds.end(); ++iter)
  {
    tmp.insert(*iter);
  }

  removeSelection(FALSE); // don't emit signal

  QgsFeature fet;
  mDataProvider->reset();

  while (mDataProvider->getNextFeature(fet))
  {
    // don't select deleted features
    if (!mDeletedFeatureIds.contains(fet.featureId()))
    {
      select(fet.featureId(), FALSE); // don't emit signal
    }
  }

  // consider also newly added features
  for (QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
  {
    select((*iter).featureId(), FALSE); // don't emit signal
  }

  for(QgsFeatureIds::iterator iter = tmp.begin(); iter != tmp.end(); ++iter)
  {
    mSelectedFeatureIds.remove(*iter);
  }

  emit selectionChanged();
}

void QgsVectorLayer::removeSelection(bool emitSignal)
{
  mSelectedFeatureIds.clear();

  if (emitSignal)
    emit selectionChanged();
}

void QgsVectorLayer::triggerRepaint()
{ 
  emit repaintRequested();
}

QgsVectorDataProvider* QgsVectorLayer::getDataProvider()
{
  return mDataProvider;
}

const QgsVectorDataProvider* QgsVectorLayer::getDataProvider() const
{
  return mDataProvider;
}

void QgsVectorLayer::setProviderEncoding(const QString& encoding)
{
  if(mDataProvider)
  {
    mDataProvider->setEncoding(encoding);
  }
}


const QgsRenderer* QgsVectorLayer::renderer() const
{
  return mRenderer;
}

void QgsVectorLayer::setRenderer(QgsRenderer * r)
{
  if (r != mRenderer)
  {
    delete mRenderer;
    mRenderer = r;
  }
}

QGis::VectorType QgsVectorLayer::vectorType() const
{
  if (mDataProvider)
  {
    int type = mDataProvider->geometryType();
    switch (type)
    {
      case QGis::WKBPoint:
      case QGis::WKBPoint25D:
        return QGis::Point;

      case QGis::WKBLineString:
      case QGis::WKBLineString25D:
        return QGis::Line;

      case QGis::WKBPolygon:
      case QGis::WKBPolygon25D:
        return QGis::Polygon;

      case QGis::WKBMultiPoint:
      case QGis::WKBMultiPoint25D:
        return QGis::Point;

      case QGis::WKBMultiLineString:
      case QGis::WKBMultiLineString25D:
        return QGis::Line;

      case QGis::WKBMultiPolygon:
      case QGis::WKBMultiPolygon25D:
        return QGis::Polygon;
    }
#ifdef QGISDEBUG
    QgsLogger::debug("Warning: Data Provider Geometry type is not recognised, is", type, 1, __FILE__, __FUNCTION__, __LINE__);
#endif

  }
  else
  {
#ifdef QGISDEBUG
    qWarning("warning, pointer to mDataProvider is null in QgsVectorLayer::vectorType()");
#endif

  }

  // We shouldn't get here, and if we have, other things are likely to
  // go wrong. Code that uses the vectorType() return value should be
  // rewritten to cope with a value of QGis::Unknown. To make this
  // need known, the following message is printed every time we get
  // here.
  std::cerr << "WARNING: This code (file " << __FILE__ << ", line "
            << __LINE__ << ") should never be reached. "
            << "Problems may occur...\n";

  return QGis::Unknown;
}

QGis::WKBTYPE QgsVectorLayer::geometryType() const
{
  return (QGis::WKBTYPE)(mGeometryType);
}

QgsRect QgsVectorLayer::boundingBoxOfSelected()
{
  if(mSelectedFeatureIds.size()==0)//no selected features
  {
    return QgsRect(0,0,0,0);
  }

  QgsRect r, retval;
  QgsFeature fet;
  mDataProvider->reset();

  retval.setMinimal();
  while (mDataProvider->getNextFeature(fet))
  {
    if (mSelectedFeatureIds.contains(fet.featureId()))
    {
      if(fet.geometry())
      {
        r=fet.geometry()->boundingBox();
        retval.combineExtentWith(&r);
      }
    }
  }

  // also go through the not commited features
  for(QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
  {
    if(mSelectedFeatureIds.contains((*iter).featureId()))
    {
      r = (*iter).geometry()->boundingBox();
      retval.combineExtentWith(&r);
    }
  }

  if (retval.width() == 0.0 || retval.height() == 0.0)
  {
    // If all of the features are at the one point, buffer the
    // rectangle a bit. If they are all at zero, do something a bit
    // more crude.

    if (retval.xMin() == 0.0 && retval.xMax() == 0.0 &&
        retval.yMin() == 0.0 && retval.yMax() == 0.0)
    {
      retval.set(-1.0, -1.0, 1.0, 1.0);
    }
    else
    {
      const double padFactor = 1e-8;
      double widthPad = retval.xMin() * padFactor;
      double heightPad = retval.yMin() * padFactor;
      double xmin = retval.xMin() - widthPad;
      double xmax = retval.xMax() + widthPad;
      double ymin = retval.yMin() - heightPad;
      double ymax = retval.yMax() + heightPad;
      retval.set(xmin, ymin, xmax, ymax);
    }
  }

  return retval;
}


QgsFeature * QgsVectorLayer::getNextFeature(bool fetchAttributes, bool selected) const
{
  if ( ! mDataProvider )
  {
    QgsLogger::warning(" QgsVectorLayer::getNextFeature() invoked with null mDataProvider");
    return 0;
  }
  
  // TODO: make an interface like the one in QgsVectorDataProvider

  QgsFeature fet;
  if ( selected )
  {
    while (mDataProvider->getNextFeature(fet)) // TODO:
    {
      bool sel = mSelectedFeatureIds.contains(fet.featureId());
      if ( sel ) return new QgsFeature(fet);
    }
    return 0;
  }

  mDataProvider->getNextFeature(fet);
  return new QgsFeature(fet);
} // QgsVectorLayer::getNextFeature



bool QgsVectorLayer::getNextFeature(QgsFeature &feature, bool fetchAttributes) const
{
  if ( ! mDataProvider )
  {
    QgsLogger::warning(" QgsVectorLayer::getNextFeature() invoked with null mDataProvider");
    return false;
  }

  // TODO: make an interface like the one in QgsVectorDataProvider
  
  return mDataProvider->getNextFeature( feature );
} // QgsVectorLayer::getNextFeature



long QgsVectorLayer::featureCount() const
{
  if ( ! mDataProvider )
  {
    QgsLogger::warning(" QgsVectorLayer::featureCount() invoked with null mDataProvider");
    return 0;
  }

  return mDataProvider->featureCount();
} // QgsVectorLayer::featureCount

long QgsVectorLayer::updateFeatureCount() const
{
  if ( ! mDataProvider )
  {
    QgsLogger::warning(" QgsVectorLayer::updateFeatureCount() invoked with null mDataProvider");
    return 0;
  }
  return mDataProvider->updateFeatureCount();
}

void QgsVectorLayer::updateExtents()
{
  if(mDataProvider)
  {
    if(mDeletedFeatureIds.isEmpty())
    {
      // get the extent of the layer from the provider
      QgsRect r = mDataProvider->extent();
      mLayerExtent.setXmin(r.xMin());
      mLayerExtent.setYmin(r.yMin());
      mLayerExtent.setXmax(r.xMax());
      mLayerExtent.setYmax(r.yMax());
    }
    else
    {
      QgsFeature fet;
      QgsRect bb;

      mLayerExtent.setMinimal();
      mDataProvider->reset();
      while (mDataProvider->getNextFeature(fet, false))
      {
        if (!mDeletedFeatureIds.contains(fet.featureId()))
        {
          if (fet.geometry())
          {
            bb = fet.geometry()->boundingBox();
            mLayerExtent.combineExtentWith(&bb);
          }
        }
      }
    }
  }
  else
  {
    QgsLogger::warning(" QgsVectorLayer::updateFeatureCount() invoked with null mDataProvider");
  }

  // also consider the not commited features
  for(QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
  {
    QgsRect bb = iter->geometry()->boundingBox();
    mLayerExtent.combineExtentWith(&bb);
  }

  // Send this (hopefully) up the chain to the map canvas
  emit recalculateExtents();
}

QString QgsVectorLayer::subsetString()
{
  if ( ! mDataProvider )
  {
    QgsLogger::warning(" QgsVectorLayer::subsetString() invoked with null mDataProvider");
    return 0;
  }
  return mDataProvider->subsetString();
}

void QgsVectorLayer::setSubsetString(QString subset)
{
  if ( ! mDataProvider )
  {
    QgsLogger::warning(" QgsVectorLayer::setSubsetString() invoked with null mDataProvider");
    return;
  }
  
  mDataProvider->setSubsetString(subset);
  // get the updated data source string from the provider
  mDataSource = mDataProvider->dataSourceUri();
  updateExtents();
  
  //trigger a recalculate extents request to any attached canvases
  QgsDebugMsg("Subset query changed, emitting recalculateExtents() signal");
  
  // emit the signal  to inform any listeners that the extent of this
  // layer has changed
  emit recalculateExtents();
}


bool QgsVectorLayer::addFeature(QgsFeature& f, bool alsoUpdateExtent)
{
  static int addedIdLowWaterMark = -1;

  if (!mDataProvider)
  {
    return false;
  }
    
  if(!(mDataProvider->capabilities() & QgsVectorDataProvider::AddFeatures))
  {
    return false;
  }

  if(!isEditable())
  {
    return false;
  }

    //assign a temporary id to the feature (use negative numbers)
    addedIdLowWaterMark--;

    QgsDebugMsg("Assigned feature id " + QString::number(addedIdLowWaterMark));

    // Force a feature ID (to keep other functions in QGIS happy,
    // providers will use their own new feature ID when we commit the new feature)
    // and add to the known added features.
    f.setFeatureId(addedIdLowWaterMark);
    mAddedFeatures.append(f);
    
    setModified(TRUE);

    if (alsoUpdateExtent)
    {
      updateExtents();
    }  

    return true;
}


bool QgsVectorLayer::insertVertexBefore(double x, double y, int atFeatureId, QgsGeometryVertexIndex beforeVertex)
{
  if (!mEditable)
  {
    return false;
  }
  
  if (mDataProvider)
  {

    if (!mChangedGeometries.contains(atFeatureId))
    {
      // first time this geometry has changed since last commit
      if(!mCachedGeometries.contains(atFeatureId))
      {
        return false;
      }
      mChangedGeometries[atFeatureId] = mCachedGeometries[atFeatureId];
    }

    mChangedGeometries[atFeatureId].insertVertexBefore(x, y, beforeVertex);

    setModified(TRUE, TRUE); // only geometry was changed

    return true;
  }
  return false;
}


bool QgsVectorLayer::moveVertexAt(double x, double y, int atFeatureId,
    QgsGeometryVertexIndex atVertex)
{
  if (!mEditable)
  {
    return false;
  }
  
  if (mDataProvider)
  {
    if (!mChangedGeometries.contains(atFeatureId))
    {
      // first time this geometry has changed since last commit
      if(!mCachedGeometries.contains(atFeatureId))
      {
        return false;
      }
      mChangedGeometries[atFeatureId] = mCachedGeometries[atFeatureId];
    }

    mChangedGeometries[atFeatureId].moveVertexAt(x, y, atVertex);

    setModified(TRUE, TRUE); // only geometry was changed

    return true;
  }
  return false;
}


bool QgsVectorLayer::deleteVertexAt(int atFeatureId,
    QgsGeometryVertexIndex atVertex)
{
  if (!mEditable)
  {
    return false;
  }

  if (mDataProvider)
  {
    if (!mChangedGeometries.contains(atFeatureId))
    {
      // first time this geometry has changed since last commit
      if(!mCachedGeometries.contains(atFeatureId))
      {
        return false;
      }
      mChangedGeometries[atFeatureId] = mCachedGeometries[atFeatureId];
    }

    mChangedGeometries[atFeatureId].deleteVertexAt(atVertex);

    setModified(TRUE, TRUE); // only geometry was changed

    return true;
  }
  return false;
}


bool QgsVectorLayer::deleteSelectedFeatures()
{
  if(!(mDataProvider->capabilities() & QgsVectorDataProvider::DeleteFeatures))
  {
    return false;
  }

  if(!isEditable())
  {
    return false;
  }

  for(QgsFeatureIds::iterator it = mSelectedFeatureIds.begin(); it != mSelectedFeatureIds.end(); ++it)
  {
    bool noncommited = FALSE;
    // first test, if the feature with this id is a not-commited feature
    for (QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
    {
      if (iter->featureId() == *it)
      {
        noncommited = TRUE;
        
        // Delete the feature itself
        mAddedFeatures.remove(iter);
        
        break;
      }
    }
    
    if (!noncommited)
    {
      mDeletedFeatureIds.insert(*it);
    }
  }

  if(mSelectedFeatureIds.size()>0)
  {
    setModified(TRUE);
    removeSelection(FALSE); // don't emit signal
    triggerRepaint();
    updateExtents();

  }

  return true;
}

QgsLabel * QgsVectorLayer::label()
{
  return mLabel;
}

void QgsVectorLayer::setLabelOn ( bool on )
{
  mLabelOn = on;
}

bool QgsVectorLayer::labelOn ( void )
{
  return mLabelOn;
}



bool QgsVectorLayer::startEditing()
{
  if (!mDataProvider)
  {
    return false;
  }
    
  if(!(mDataProvider->capabilities()&QgsVectorDataProvider::AddFeatures))
  {
    return false;
  }

  // No longer need to cache all geometries
  // instead, mCachedGeometries is refreshed every time the
  // screen is redrawn.
  //cacheGeometries();
  mEditable=true;
  return true;
}

  
bool QgsVectorLayer::readXML_( QDomNode & layer_node )
{
#ifdef QGISDEBUG
  std::cerr << "Datasource in QgsVectorLayer::readXML_: " << mDataSource.toLocal8Bit().data() << std::endl;
#endif
  // process the attribute actions
  mActions->readXML(layer_node);

  //process provider key
  QDomNode pkeyNode = layer_node.namedItem("provider");

  if (pkeyNode.isNull())
  {
    mProviderKey = "";
  }
  else
  {
    QDomElement pkeyElt = pkeyNode.toElement();
    mProviderKey = pkeyElt.text();
  }

  // determine type of vector layer
  if ( ! mProviderKey.isNull() )
  {
    // if the provider string isn't empty, then we successfully
    // got the stored provider
  }
  else if ((mDataSource.find("host=") > -1) &&
      (mDataSource.find("dbname=") > -1))
  {
    mProviderKey = "postgres";
  }
  else
  {
    mProviderKey = "ogr";
  }

  if ( ! setDataProvider( mProviderKey ) )
  {
    return false;
  }

  //read provider encoding
  QDomNode encodingNode = layer_node.namedItem("encoding");
  if( ! encodingNode.isNull() && mDataProvider )
  {
    mDataProvider->setEncoding(encodingNode.toElement().text());
  }

  // get and set the display field if it exists.
  QDomNode displayFieldNode = layer_node.namedItem("displayfield");
  if (!displayFieldNode.isNull())
  {
    QDomElement e = displayFieldNode.toElement();
    setDisplayField(e.text());
  }



  // create and bind a renderer to this layer

  QDomNode singlenode = layer_node.namedItem("singlesymbol");
  QDomNode graduatednode = layer_node.namedItem("graduatedsymbol");
  QDomNode continuousnode = layer_node.namedItem("continuoussymbol");
  QDomNode singlemarkernode = layer_node.namedItem("singlemarker");
  QDomNode graduatedmarkernode = layer_node.namedItem("graduatedmarker");
  QDomNode uniquevaluenode = layer_node.namedItem("uniquevalue");
  QDomNode labelnode = layer_node.namedItem("label");
  QDomNode uniquemarkernode = layer_node.namedItem("uniquevaluemarker");

  //std::auto_ptr<QgsRenderer> renderer; actually the renderer SHOULD NOT be
  //deleted when this function finishes, otherwise the application will
  //crash
  // XXX this seems to be a dangerous implementation; should re-visit design
  QgsRenderer * renderer;

  // XXX Kludge!


  // if we don't have a coordinate transform, get one

  //
  // Im commenting this out - if the layer was serialied in a 
  //  >=0.7 project it should have been validated and have all
  // coord xform info
  //

  //if ( ! coordinateTransform() )
  //{
  //    setCoordinateSystem();
  //}

  if (!singlenode.isNull())
  {
    renderer = new QgsSingleSymbolRenderer(vectorType());
    renderer->readXML(singlenode, *this);
  }
  else if (!graduatednode.isNull())
  {
    renderer = new QgsGraduatedSymbolRenderer(vectorType());
    renderer->readXML(graduatednode, *this);
  }
  else if (!continuousnode.isNull())
  {
    renderer = new QgsContinuousColorRenderer(vectorType());
    renderer->readXML(continuousnode, *this);
  }
  else if (!uniquevaluenode.isNull())
  {
    renderer = new QgsUniqueValueRenderer(vectorType());
    renderer->readXML(uniquevaluenode, *this);
  }

  // Test if labeling is on or off
  QDomElement element = labelnode.toElement();
  int labelOn = element.text().toInt();
  if (labelOn < 1)
  {
    setLabelOn(false);
  }
  else
  {
    setLabelOn(true);
  }

#ifdef QGISDEBUG
  std::cout << "Testing if qgsvectorlayer can call label readXML routine" << std::endl;
#endif

  QDomNode labelattributesnode = layer_node.namedItem("labelattributes");

  if(!labelattributesnode.isNull())
  {
#ifdef QGISDEBUG
    std::cout << "qgsvectorlayer calling label readXML routine" << std::endl;
#endif
    mLabel->readXML(labelattributesnode);
  }

  return mValid;               // should be true if read successfully

} // void QgsVectorLayer::readXML_



bool QgsVectorLayer::setDataProvider( QString const & provider )
{
  // XXX should I check for and possibly delete any pre-existing providers?
  // XXX How often will that scenario occur?

  mProviderKey = provider;     // XXX is this necessary?  Usually already set
  // XXX when execution gets here.

  //XXX - This was a dynamic cast but that kills the Windows
  //      version big-time with an abnormal termination error
  mDataProvider = 
    (QgsVectorDataProvider*)(QgsProviderRegistry::instance()->getProvider(provider,mDataSource));

  if (mDataProvider)
  {
    QgsDebugMsg( "Instantiated the data provider plugin" );

    mValid = mDataProvider->isValid();
    if (mValid)
    {

      // TODO: Check if the provider has the capability to send fullExtentCalculated
      connect(mDataProvider, SIGNAL( fullExtentCalculated() ), 
          this,           SLOT( updateExtents() ) 
          );

      // get the extent
      QgsRect mbr = mDataProvider->extent();

      // show the extent
      QString s = mbr.stringRep();
      QgsDebugMsg("Extent of layer: " +  s);
      // store the extent
      mLayerExtent.setXmax(mbr.xMax());
      mLayerExtent.setXmin(mbr.xMin());
      mLayerExtent.setYmax(mbr.yMax());
      mLayerExtent.setYmin(mbr.yMin());

      // get and store the feature type
      mGeometryType = mDataProvider->geometryType();

      // look at the fields in the layer and set the primary
      // display field using some real fuzzy logic
      setDisplayField();

      if (mProviderKey == "postgres")
      {
        QgsDebugMsg("Beautifying layer name " + name());
        // adjust the display name for postgres layers
        QRegExp reg("\".+\"\\.\"(.+)\"");
        reg.indexIn(name());
        QStringList stuff = reg.capturedTexts();
        QString lName = stuff[1];
        if (lName.length() == 0) // fallback
          lName = name();
        setLayerName(lName);
        QgsDebugMsg("Beautifying layer name " + name());
      }

      // label
      mLabel = new QgsLabel ( mDataProvider->fields() );
      mLabelOn = false;
    }
    else
    {
      QgsDebugMsg("Invalid provider plugin " + QString(mDataSource.ascii()));
      return false;
    }
  }
  else
  {
    QgsDebugMsg( " unable to get data provider" );

    return false;
  }

  return true;

} // QgsVectorLayer:: setDataProvider




/* virtual */ bool QgsVectorLayer::writeXML_( QDomNode & layer_node,
    QDomDocument & document )
{
  // first get the layer element so that we can append the type attribute

  QDomElement mapLayerNode = layer_node.toElement();

  if ( mapLayerNode.isNull() || ("maplayer" != mapLayerNode.nodeName()) )
  {
    qDebug( "QgsVectorLayer::writeXML() can't find <maplayer>" );
    return false;
  }

  mapLayerNode.setAttribute( "type", "vector" );

  // set the geometry type
  mapLayerNode.setAttribute( "geometry", QGis::qgisVectorGeometryType[vectorType()]);

  // add provider node

  QDomElement provider  = document.createElement( "provider" );
  QDomText providerText = document.createTextNode( providerType() );
  provider.appendChild( providerText );
  layer_node.appendChild( provider );

  //provider encoding
  QDomElement encoding = document.createElement("encoding");
  QDomText encodingText = document.createTextNode(mDataProvider->encoding());
  encoding.appendChild( encodingText );
  layer_node.appendChild( encoding );

  //classification field(s)
  QgsAttributeList attributes=mRenderer->classificationAttributes();
  const QgsFieldMap providerFields = mDataProvider->fields();
  for(QgsAttributeList::const_iterator it = attributes.begin(); it != attributes.end(); ++it)
    {
      QDomElement classificationElement = document.createElement("classificationattribute");
      QDomText classificationText = document.createTextNode(providerFields[*it].name());
      classificationElement.appendChild(classificationText);
      layer_node.appendChild(classificationElement);
    }

  // add the display field

  QDomElement dField  = document.createElement( "displayfield" );
  QDomText dFieldText = document.createTextNode( displayField() );
  dField.appendChild( dFieldText );
  layer_node.appendChild( dField );

  // add label node

  QDomElement label  = document.createElement( "label" );
  QDomText labelText = document.createTextNode( "" );

  if ( labelOn() )
  {
    labelText.setData( "1" );
  }
  else
  {
    labelText.setData( "0" );
  }
  label.appendChild( labelText );

  layer_node.appendChild( label );

  // add attribute actions

  mActions->writeXML(layer_node, document);

  // renderer specific settings

  const QgsRenderer * myRenderer = renderer();
  if( myRenderer )
  {
    myRenderer->writeXML(layer_node, document);
  }
  else
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " no renderer\n";

    // XXX return false?
  }

  // Now we get to do all that all over again for QgsLabel

  // XXX Since this is largely a cut-n-paste from the previous, this
  // XXX therefore becomes a candidate to be generalized into a separate
  // XXX function.  I think.

  QgsLabel * myLabel = this->label();

  if ( myLabel )
  {
    std::stringstream labelXML;

    myLabel->writeXML(labelXML);

    QDomDocument labelDOM;

    std::string rawXML;
    std::string temp_str;
    QString     errorMsg;
    int         errorLine;
    int         errorColumn;

    // start with bogus XML header
    rawXML  = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";

    temp_str = labelXML.str();

    rawXML   += temp_str;

#ifdef QGISDEBUG
    std::cout << rawXML << std::endl << std::flush;
#endif
    const char * s = rawXML.c_str(); // debugger probe
    // Use the const char * form of the xml to make non-stl qt happy
    if ( ! labelDOM.setContent( QString::fromUtf8(s), &errorMsg, &errorLine, &errorColumn ) )
    {
      qDebug( ("XML import error at line %d column %d " + errorMsg).toLocal8Bit().data(), errorLine, errorColumn );

      return false;
    }

    // lastChild() because the first two nodes are the <xml> and
    // <!DOCTYPE> nodes; the label node follows that, and is (hopefully)
    // the last node.
    QDomNode labelDOMNode = document.importNode( labelDOM.lastChild(), true );

    if ( ! labelDOMNode.isNull() )
    {
      layer_node.appendChild( labelDOMNode );
    }
    else
    {
      qDebug( "not able to import label DOM node" );

      // XXX return false?
    }

  }

  return true;
} // bool QgsVectorLayer::writeXML_


int QgsVectorLayer::findFreeId()
{
  int freeid=-INT_MAX;
  int fid;
  if(mDataProvider)
  {
    mDataProvider->reset();
    QgsFeature fet;

    //TODO: Is there an easier way of doing this other than iteration?
    //TODO: Also, what about race conditions between this code and a competing mapping client?
    //TODO: Maybe push this to the data provider?
    while (mDataProvider->getNextFeature(fet, false))
    {
      fid = fet.featureId();
      if(fid>freeid)
      {
        freeid=fid;
      }
    }
    
    QgsDebugMsg("freeid is: " + QString::number(freeid+1));
    
    return freeid+1;
  }
  else
  {
    QgsDebugMsg("Error, mDataProvider is 0 in QgsVectorLayer::findFreeId");
    return -1;
  }
}

bool QgsVectorLayer::commitChanges()
{
  if (!mDataProvider)
  {
    return false;
  }
  
  if (!isEditable())
  {
    return false;
  }
  
  /*
     Unfortunately the commits occur in four distinct stages,
     (add features, change attributes, change geometries, delete features)
     so if a stage fails, it's difficult to roll back cleanly.
     Therefore the error messages become a bit complicated to generate.
   */

  // Attempt the commit of new features
  bool addedFeaturesOk = FALSE;
  if (mAddedFeatures.size() > 0)
  {
    if(!mDataProvider->addFeatures(mAddedFeatures))
    {
      QStringList errorStrings;
      errorStrings += tr("Could not commit the added features.");
      errorStrings += tr("No other types of changes will be committed at this time.");

      // TODO: use an error string or something to store error to be retrieved later [MD]
      //QMessageBox::warning(0, tr("Error"), errorStrings.join(" "));
      return FALSE;
    }
    else
    {
      // done, remove features from the list
      mAddedFeatures.clear();
      addedFeaturesOk = TRUE;
    }
  }

  // Attempt the commit of changed attributes
  bool changedAttributesOk = FALSE;
  if (mChangedAttributes.size() > 0)
  {
    if (!mDataProvider->changeAttributeValues(mChangedAttributes))
    {
      QStringList errorStrings;
      errorStrings += tr("Could not commit the changed attributes.");
      if (addedFeaturesOk)
      {
        errorStrings += tr("However, the added features were committed OK.");
      }
      errorStrings += tr("No other types of changes will be committed at this time.");

      // TODO: use an error string or something to store error to be retrieved later [MD]
      //QMessageBox::warning(0, tr("Error"), errorStrings.join(" "));
      return FALSE;
    }
    else
    {
      // Changed attributes committed OK, remove the in-memory changes
      mChangedAttributes.clear();
      changedAttributesOk = TRUE;
    }
  }

  // Attempt the commit of changed geometries
  bool changedGeometriesOk = FALSE;
  if (mChangedGeometries.size() > 0)
  {
    if (!mDataProvider->changeGeometryValues(mChangedGeometries))
    {
      QStringList errorStrings;
      errorStrings += tr("Could not commit the changed geometries.");
      if (addedFeaturesOk)
      {
        errorStrings += tr("However, the added features were committed OK.");
      }
      if (changedAttributesOk)
      {
        errorStrings += tr("However, the changed attributes were committed OK.");
      }
      errorStrings += tr("No other types of changes will be committed at this time.");

      // TODO: use an error string or something to store error to be retrieved later [MD]
      //QMessageBox::warning(0, tr("Error"), errorStrings.join(" "));
      return FALSE;
    }
    else
    {
      // Changed geometries committed OK, remove the in-memory changes
      mChangedGeometries.clear();
      changedGeometriesOk = TRUE;
    }
  }

  // Attempt the commit of deleted features
  bool deletedFeaturesOk = FALSE;
  if (mDeletedFeatureIds.size() > 0)
  {
    if (!mDataProvider->deleteFeatures(mDeletedFeatureIds))
    {
      QStringList errorStrings;
      errorStrings += tr("Could not commit the deleted features.");
      if (addedFeaturesOk)
      {
        errorStrings += tr("However, the added features were committed OK.");
      }
      if (changedAttributesOk)
      {
        errorStrings += tr("However, the changed attributes were committed OK.");
      }
      if (changedGeometriesOk)
      {
        errorStrings += tr("However, the changed geometries were committed OK.");
      }
      errorStrings += tr("No other types of changes will be committed at this time.");

      // TODO: use an error string or something to store error to be retrieved later [MD]
      //QMessageBox::warning(0, tr("Error"), errorStrings.join(" "));
      return FALSE;
    }
    else
    {
      // just in case some of those features are still selected
      for (QgsFeatureIds::iterator it = mDeletedFeatureIds.begin(); it != mDeletedFeatureIds.end(); ++it)
      {
        mSelectedFeatureIds.remove(*it);
      }
      
      // Deleted features committed OK, remove the in-memory changes
      mDeletedFeatureIds.clear();
      deletedFeaturesOk = TRUE;
    }
  }

  deleteCachedGeometries();
  
  mEditable = false;
  setModified(FALSE);
  
  mDataProvider->updateExtents();
  mDataProvider->updateFeatureCount();

  triggerRepaint();
  
  return TRUE;
}

bool QgsVectorLayer::rollBack()
{
  if (!isEditable())
  {
    return false;
  }
  
  if (isModified())
  {
    // roll back changed attributes
    mChangedAttributes.clear();

    // roll back changed geometries
    mChangedGeometries.clear();
  
    // Roll back added features
    // Delete the features themselves before deleting the references to them.
    mAddedFeatures.clear();
  
    // Roll back deleted features
    mDeletedFeatureIds.clear();
  }

  deleteCachedGeometries();

  mEditable = false;
  setModified(FALSE);
  
  triggerRepaint();

  return true;
}

void QgsVectorLayer::setSelectedFeatures(const QgsFeatureIds& ids)
{
  // TODO: check whether features with these ID exist
  mSelectedFeatureIds = ids;
  emit selectionChanged();
}

int QgsVectorLayer::selectedFeatureCount()
{
  return mSelectedFeatureIds.size();
}

const QgsFeatureIds& QgsVectorLayer::selectedFeaturesIds() const
{
  return mSelectedFeatureIds;
}


QgsFeatureList QgsVectorLayer::selectedFeatures()
{
  if (!mDataProvider)
  {
    return QgsFeatureList();
  }
  
  QgsFeatureList features;

  // we don't need to cache features ... it just adds unnecessary time
  // as we don't need to pull *everything* from disk

  // Go through each selected feature ID and determine
  // its current geometry and attributes
  
  for (QgsFeatureIds::iterator it = mSelectedFeatureIds.begin(); it != mSelectedFeatureIds.end(); ++it)
  {
    QgsFeature feat;

    // Pull the original version of the feature from disk or memory
    // as appropriate

    // Check this selected item against the uncommitted added features
    bool selectionIsAddedFeature = FALSE;

    for (QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
    {
      if ( (*it) == (*iter).featureId() )
      {
        feat = QgsFeature(*iter);
        selectionIsAddedFeature = TRUE;
      }
    }

    if (!selectionIsAddedFeature)
    {
      // pull committed version from disk
      feat = *it;

      int row = 0;  //TODO: Get rid of this, but getFeatureAttributes()
                    //      needs it for some reason

      mDataProvider->getFeatureAttributes(*it, row, &feat);

      if (!mChangedGeometries.contains(*it))
      {
         // also pull committed geometry from disk as we will
         // not need to overwrite it later

         if (mDataProvider->capabilities() & QgsVectorDataProvider::SelectGeometryAtId)
         {
           mDataProvider->getFeatureGeometry(*it, &feat);
         }
         else
         {
           // TODO: handle error somehow [MD]
           //QMessageBox::information(0, tr("Cannot retrieve features"),
           //     tr("The provider for the current layer cannot retrieve geometry for the selected features.  This version of the provider does not have this capability."));
         }
       }

     }

     // Transform the feature to the "current" in-memory version

     features.append(QgsFeature(feat, mChangedAttributes, mChangedGeometries));
  } // for each selected

  return features;
}

bool QgsVectorLayer::addFeatures(QgsFeatureList features, bool makeSelected)
{
  if (!mDataProvider)
  { 
    return false;
  }
  
  if(!(mDataProvider->capabilities() & QgsVectorDataProvider::AddFeatures))
  {
    return false;
  }
  
  if (!isEditable())
  {
    return false;
  }

  if (makeSelected)
  {
    mSelectedFeatureIds.clear();
  }

  for (QgsFeatureList::iterator iter = features.begin(); iter != features.end(); ++iter)
  {
    addFeature(*iter);

    if (makeSelected)
    {
      mSelectedFeatureIds.insert((*iter).featureId());
    }
  }

  updateExtents();
  
  if (makeSelected)
  {
    emit selectionChanged();
  }

  return true;
}


bool QgsVectorLayer::copySymbologySettings(const QgsMapLayer& other)
{
  const QgsVectorLayer* vl = dynamic_cast<const QgsVectorLayer*>(&other);

  if(this == vl)//exit if both vectorlayer are the same
  {
    return false;
  }

  if(!vl)
  {
    return false;
  }
  delete mRenderer;

  QgsRenderer* r = vl->mRenderer;
  if(r)
  {
    mRenderer = r->clone();
    return true;
  }
  else
  {
    return false;
  }
}

bool QgsVectorLayer::isSymbologyCompatible(const QgsMapLayer& other) const
{
  //vector layers are symbology compatible if they have the same type, the same sequence of numerical/ non numerical fields and the same field names


  const QgsVectorLayer* otherVectorLayer = dynamic_cast<const QgsVectorLayer*>(&other);
  if(otherVectorLayer)
  {

    if(otherVectorLayer->vectorType() != vectorType())
      {
	return false;
      }

    const QgsFieldMap& fieldsThis = mDataProvider->fields();
    const QgsFieldMap& fieldsOther = otherVectorLayer ->mDataProvider->fields();

    if(fieldsThis.size() != fieldsOther.size())
    {
      return false;
    }

    // TODO: fill two sets with the numerical types for both layers

    uint fieldsThisSize = fieldsThis.size();

    for(uint i = 0; i < fieldsThisSize; ++i)
    {
      if(fieldsThis[i].name() != fieldsOther[i].name())//field names need to be the same
      {
        return false;
      }
      // TODO: compare types of the fields
    }
    return true; //layers are symbology compatible if the code reaches this point
  }
  return false;
}

bool QgsVectorLayer::snapPoint(QgsPoint& point, double tolerance)
{
  if(tolerance<=0||!mDataProvider)
  {
    return false;
  }
  double mindist=tolerance*tolerance;//current minimum distance
  double mindistx=point.x();
  double mindisty=point.y();
  QgsFeature fet;
  QgsPoint vertexFeature;//the closest vertex of a feature
  QgsGeometryVertexIndex vindex;
  double minsquaredist;
  int rb1, rb2; //rubberband indexes (not used in this method)

  QgsRect selectrect(point.x()-tolerance,point.y()-tolerance,point.x()+tolerance,point.y()+tolerance);

  mDataProvider->reset();
  mDataProvider->select(selectrect);

  //go to through the features reported by the spatial filter of the provider
  while (mDataProvider->getNextFeature(fet))
  {
    // if geometry has been changed, use the new geometry
    if(mChangedGeometries.contains(fet.featureId()))
    {
      vertexFeature = mChangedGeometries[fet.featureId()].closestVertex(point, vindex, rb1, rb2, minsquaredist);
    }
    else
    {
      vertexFeature = fet.geometry()->closestVertex(point, vindex, rb1, rb2, minsquaredist);
    }
    if(minsquaredist<mindist)
    {
      mindistx=vertexFeature.x();
      mindisty=vertexFeature.y();
      mindist=minsquaredist;
    }
  }

  //also go through the not commited features
  for(QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter!=mAddedFeatures.end(); ++iter)
  {
    if (mChangedGeometries.contains((*iter).featureId()))
    {
      // use the changed geometry
      vertexFeature = mChangedGeometries[(*iter).featureId()].closestVertex(point, vindex, rb1, rb2, minsquaredist);
    }
    else
    {
      vertexFeature = (*iter).geometry()->closestVertex(point, vindex, rb1, rb2, minsquaredist);
    }
    if(minsquaredist<mindist)
    {
      mindistx=vertexFeature.x();
      mindisty=vertexFeature.y();
      mindist=minsquaredist;
    }
  }

  //and also go through the changed geometries, because the spatial filter of the provider did not consider feature changes
  for(QgsGeometryMap::const_iterator iter = mChangedGeometries.begin(); iter != mChangedGeometries.end(); ++iter)
  {
    vertexFeature = iter.value().closestVertex(point, vindex, rb1, rb2, minsquaredist);
    if(minsquaredist<mindist)
    {
      mindistx=vertexFeature.x();
      mindisty=vertexFeature.y();
      mindist=minsquaredist;
    }
  }

  point.setX(mindistx);
  point.setY(mindisty);

  return true;
}


bool QgsVectorLayer::snapVertexWithContext(QgsPoint& point, QgsGeometryVertexIndex& atVertex,
                                           int& beforeVertexIndex, int& afterVertexIndex,
                                           int& snappedFeatureId, QgsGeometry& snappedGeometry,
                                           double tolerance)
{
  bool vertexFound = false; //flag to check if a meaningful result can be returned
  QgsGeometryVertexIndex atVertexTemp;
  int beforeVertexIndexTemp, afterVertexIndexTemp;

  QgsPoint origPoint = point;

  // Sanity checking
  if (tolerance<=0 || !mDataProvider)
  {
    return false;
  }

  QgsFeature feature;
  QgsPoint minDistSegPoint;  // the closest point
  double testSqrDist;        // the squared distance between 'point' and 'snappedFeature'

  double minSqrDist  = tolerance*tolerance; //current minimum distance

  QgsRect selectrect(point.x()-tolerance, point.y()-tolerance,
                     point.x()+tolerance, point.y()+tolerance);

  mDataProvider->reset();
  mDataProvider->select(selectrect);

  // Go through the committed features
  while (mDataProvider->getNextFeature(feature))
  {
    if (mChangedGeometries.contains(feature.featureId()))
    {
      // ignore for this loop, let the loop below over mChangedGeometries
      // detect the changed geometry instead
      continue;

      // // substitute the modified geometry for the committed version
      // feature->setGeometry(mChangedGeometries[feature->featureId()]);
    }

    minDistSegPoint = feature.geometry()->closestVertex(origPoint, atVertexTemp, beforeVertexIndexTemp, afterVertexIndexTemp, testSqrDist);
    if (
        (testSqrDist < minSqrDist) ||
        (
         // this test will "choose" the polygon that the origPoint is in:
         (testSqrDist == minSqrDist) && 
         (feature.geometry()->contains(&origPoint))
        )
       )
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;

      atVertex          = atVertexTemp;
      beforeVertexIndex = beforeVertexIndexTemp;
      afterVertexIndex = afterVertexIndexTemp;
      snappedFeatureId  = feature.featureId();
      snappedGeometry   = *(feature.geometry());
      vertexFound = true;
      return true;
    }
  }


  // Also go through the new features
  for (QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
  {
    if (mChangedGeometries.contains((*iter).featureId()))
    {
      // ignore for this loop, let the loop below over mChangedGeometries
      // detect the changed geometry instead
      continue;
      // //use the modified geometry
      // minDistSegPoint = mChangedGeometries[(*iter)->featureId()].closestVertex(origPoint, atVertexTemp, beforeVertexIndexTemp, afterVertexIndexTemp, testSqrDist);
    }
    else
    {
    	minDistSegPoint = (*iter).geometry()->closestVertex(origPoint, atVertexTemp, beforeVertexIndexTemp, afterVertexIndexTemp, testSqrDist);
    }
    if (
        (testSqrDist < minSqrDist) ||
        (
         // this test will "choose" the polygon that the origPoint is in:
         (testSqrDist == minSqrDist) && 
         ((*iter).geometry()->contains(&origPoint))
        )
       )
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;

      atVertex      = atVertexTemp;
      beforeVertexIndex = beforeVertexIndexTemp;
      afterVertexIndex = afterVertexIndexTemp;
      snappedFeatureId  =   iter->featureId();
      snappedGeometry   = *(iter->geometry());
      vertexFound = true;
      return true;
    }
  }

  //and also go through the changed geometries, because the spatial filter of the provider did not consider feature changes
  for(QgsGeometryMap::iterator it = mChangedGeometries.begin(); it != mChangedGeometries.end(); ++it)
  {
    minDistSegPoint = it.value().closestVertex(origPoint, atVertexTemp, beforeVertexIndexTemp, afterVertexIndexTemp, testSqrDist);
    if (
        (testSqrDist < minSqrDist) ||
        (
           // this test will "choose" the polygon that the origPoint is in:
        (testSqrDist == minSqrDist) && 
        (it.value().contains(&origPoint))
        )
       )
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;
      atVertex      = atVertexTemp;
      beforeVertexIndex = beforeVertexIndexTemp;
      afterVertexIndex = afterVertexIndexTemp;
      snappedFeatureId  = it.key();
      snappedGeometry   = it.value();
      vertexFound = true;
    }
  }

  if(!vertexFound)
  {
    beforeVertexIndex = -1;
    afterVertexIndex = -1;
    return false;
  }

  return true;
}


bool QgsVectorLayer::snapSegmentWithContext(QgsPoint& point, QgsGeometryVertexIndex& beforeVertex,
                                            int& snappedFeatureId, QgsGeometry& snappedGeometry,
                                            double tolerance)
{
  bool segmentFound = false; //flag to check if a reasonable result can be returned
  QgsGeometryVertexIndex beforeVertexTemp;

  QgsPoint origPoint = point;

  // Sanity checking
  if (tolerance<=0 || !mDataProvider)
  {
    return false;
  }

  QgsFeature feature;
  QgsPoint minDistSegPoint;  // the closest point on the segment
  double testSqrDist;        // the squared distance between 'point' and 'snappedFeature'
  double minSqrDist  = tolerance*tolerance; //current minimum distance

  QgsRect selectrect(point.x()-tolerance, point.y()-tolerance,
                     point.x()+tolerance, point.y()+tolerance);

  mDataProvider->reset();
  mDataProvider->select(selectrect);

  // Go through the committed features
  while (mDataProvider->getNextFeature(feature))
  {
    if (mChangedGeometries.contains(feature.featureId()))
    {
      // ignore for this loop, let the loop below over mChangedGeometries
      // detect the changed geometry instead
      continue;

      // // substitute the modified geometry for the committed version
      // feature->setGeometry( mChangedGeometries[ feature->featureId() ] );
    }

    minDistSegPoint = feature.geometry()->closestSegmentWithContext(origPoint, beforeVertexTemp, testSqrDist);

    if (
        (testSqrDist < minSqrDist) ||
        (
         // this test will "choose" the polygon that the origPoint is in:
         (testSqrDist == minSqrDist) && 
         (feature.geometry()->contains(&origPoint))
        )
       )
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;

      beforeVertex      = beforeVertexTemp;
      snappedFeatureId  = feature.featureId();
      snappedGeometry   = *(feature.geometry());
      segmentFound = true;
    }
    
  }

  // Also go through the new features
  for (QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
  {
    if(mChangedGeometries.contains((*iter).featureId()))
    {
      // ignore for this loop, let the loop below over mChangedGeometries
      // detect the changed geometry instead
      continue;

      // //use the modified geometry
      // minDistSegPoint = mChangedGeometries[(*iter)->featureId()].closestSegmentWithContext(origPoint, beforeVertexTemp, testSqrDist);
    }
    else
    {
      minDistSegPoint = (*iter).geometry()->closestSegmentWithContext(origPoint, beforeVertexTemp, testSqrDist);
    }

    if (
        (testSqrDist < minSqrDist) ||
        (
         // this test will "choose" the polygon that the origPoint is in:
         (testSqrDist == minSqrDist) && 
         ((*iter).geometry()->contains(&origPoint))
        )
       )
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;

      beforeVertex      = beforeVertexTemp;
      snappedFeatureId  =   (*iter).featureId();
      snappedGeometry   = *((*iter).geometry());
      segmentFound = true;
    }
  }

  //and also go through the changed geometries, because the spatial filter of the provider did not consider feature changes
  for(QgsGeometryMap::iterator it = mChangedGeometries.begin(); it != mChangedGeometries.end(); ++it)
  {
    minDistSegPoint = it.value().closestSegmentWithContext(origPoint, beforeVertexTemp, testSqrDist);
    if (
        (testSqrDist < minSqrDist) ||
        (
           // this test will "choose" the polygon that the origPoint is in:
        (testSqrDist == minSqrDist) && 
        (it.value().contains(&origPoint))
        )
       )
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;
      beforeVertex      = beforeVertexTemp;
      snappedFeatureId  = it.key();
      snappedGeometry   = it.value();
      segmentFound = true;
    }
  }

  return segmentFound; 
}


void QgsVectorLayer::drawFeature(QPainter* p,
                                 QgsFeature& fet,
                                 QgsMapToPixel * theMapToPixelTransform,
                                 QgsCoordinateTransform* ct,
                                 QImage * marker,
                                 double markerScaleFactor,
                                 bool drawingToEditingCanvas)
{
  // Only have variables, etc outside the switch() statement that are
  // used in all cases of the statement (otherwise they may get
  // executed, but never used, in a bit of code where performance is
  // critical).

#if defined(Q_WS_X11)
  bool needToTrim = false;
#endif

  QgsGeometry* geom = fet.geometry();
  unsigned char* feature = geom->wkbBuffer();

  QGis::WKBTYPE wkbType = geom->wkbType();

#ifdef QGISDEBUG
  //std::cout <<"Entering drawFeature()" << std::endl;
#endif

  switch (wkbType)
  {
    case QGis::WKBPoint:
    case QGis::WKBPoint25D:
      {
        double x = *((double *) (feature + 5));
        double y = *((double *) (feature + 5 + sizeof(double)));

#ifdef QGISDEBUG 
        //  std::cout <<"...WKBPoint (" << x << ", " << y << ")" <<std::endl;
#endif

        transformPoint(x, y, theMapToPixelTransform, ct);
        //QPointF pt(x - (marker->width()/2),  y - (marker->height()/2));
        QPointF pt(x/markerScaleFactor - (marker->width()/2),  y/markerScaleFactor - (marker->height()/2));

        p->save();
        p->scale(markerScaleFactor,markerScaleFactor);
        p->drawImage(pt, *marker);
        p->restore();

        break;
      }
    case QGis::WKBMultiPoint:
    case QGis::WKBMultiPoint25D:
      {
        unsigned char *ptr = feature + 5;
        unsigned int nPoints = *((int*)ptr);
        ptr += 4;

        p->save();
        p->scale(markerScaleFactor, markerScaleFactor);

        for (register unsigned int i = 0; i < nPoints; ++i)
        {
          ptr += 5;
          double x = *((double *) ptr);
          ptr += sizeof(double);
          double y = *((double *) ptr);
          ptr += sizeof(double);
          
          if (wkbType == QGis::WKBMultiPoint25D) // ignore Z value
            ptr += sizeof(double);

#ifdef QGISDEBUG 
          std::cout <<"...WKBMultiPoint (" << x << ", " << y << ")" <<std::endl;
#endif

          transformPoint(x, y, theMapToPixelTransform, ct);
          //QPointF pt(x - (marker->width()/2),  y - (marker->height()/2));
          QPointF pt(x/markerScaleFactor - (marker->width()/2),  y/markerScaleFactor - (marker->height()/2));
          
#if defined(Q_WS_X11)
          // Work around a +/- 32768 limitation on coordinates in X11
          if (std::abs(x) > QgsClipper::maxX ||
              std::abs(y) > QgsClipper::maxY)
            needToTrim = true;
          else
#endif
          p->drawImage(pt, *marker);
        }
        p->restore();

        break;
      }
    case QGis::WKBLineString:
    case QGis::WKBLineString25D:
      {
        drawLineString(feature,
                       p,
                       theMapToPixelTransform,
                       ct,
                       drawingToEditingCanvas);
        break;
      }
    case QGis::WKBMultiLineString:
    case QGis::WKBMultiLineString25D:
    {
        unsigned char* ptr = feature + 5;
        unsigned int numLineStrings = *((int*)ptr);
        ptr = feature + 9;
        
        for (register unsigned int jdx = 0; jdx < numLineStrings; jdx++)
        {
          ptr = drawLineString(ptr,
                               p,
                               theMapToPixelTransform,
                               ct,
                               drawingToEditingCanvas);
        }
        break;
      }
    case QGis::WKBPolygon:
    case QGis::WKBPolygon25D:
    {
        drawPolygon(feature,
                    p,
                    theMapToPixelTransform,
                    ct,
                    drawingToEditingCanvas);
        break;
      }
    case QGis::WKBMultiPolygon:
    case QGis::WKBMultiPolygon25D:
    {
        unsigned char *ptr = feature + 5;
        unsigned int numPolygons = *((int*)ptr);
        ptr = feature + 9;
        for (register unsigned int kdx = 0; kdx < numPolygons; kdx++)
          ptr = drawPolygon(ptr,
                            p,
                            theMapToPixelTransform, 
                            ct,
                            drawingToEditingCanvas);
        break;
      }
    default:
      QgsDebugMsg("UNKNOWN WKBTYPE ENCOUNTERED");
      break;
  }
}



void QgsVectorLayer::setCoordinateSystem()
{
  QgsDebugMsg("QgsVectorLayer::setCoordinateSystem ----- Computing Coordinate System");
  
  //
  // Get the layers project info and set up the QgsCoordinateTransform 
  // for this layer
  //
  
  // get SRS directly from provider
  *mSRS = mDataProvider->getSRS();
  
  /*  
  
  // XXX old stuff
  
  int srid = getProjectionSrid();

  if(srid == 0)
  {
    QString mySourceWKT(getProjectionWKT());
    if (mySourceWKT.isNull())
    {
      mySourceWKT=QString("");
    }
    QgsDebugMsg("QgsVectorLayer::setCoordinateSystem --- using wkt " + mySourceWKT);
    mSRS->createFromWkt(mySourceWKT);
  }
  else
  {
    QgsDebugMsg("QgsVectorLayer::setCoordinateSystem --- using srid " + QString::number(srid));
    mSRS->createFromSrid(srid);
  }
  */

  //QgsSpatialRefSys provides a mechanism for FORCE a srs to be valid
  //which is inolves falling back to system, project or user selected
  //defaults if the srs is not properly intialised.
  //we only nee to do that if the srs is not alreay valid
  if (!mSRS->isValid())
  {
    mSRS->validate();
  }

}

bool QgsVectorLayer::commitAttributeChanges(const QgsAttributeIds& deleted,
                            const QgsNewAttributesMap& added,
                            const QgsChangedAttributesMap& changed)
{
  bool returnvalue=true;

  if(mDataProvider)
  {
    if(mDataProvider->capabilities()&QgsVectorDataProvider::DeleteAttributes)
    {
      //delete attributes in all not commited features
      for (QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
      {
        for (QgsAttributeIds::const_iterator it = deleted.begin(); it != deleted.end(); ++it)
        {
          (*iter).deleteAttribute(*it);
        }
      }

      //and then in the provider
      if(!mDataProvider->deleteAttributes(deleted))
      {
        returnvalue=false;
      }
    }

    if(mDataProvider->capabilities()&QgsVectorDataProvider::AddAttributes)
    {
      //add attributes in all not commited features
      // TODO: is it necessary? [MD]
      /*for (QgsFeatureList::iterator iter = mAddedFeatures.begin(); iter != mAddedFeatures.end(); ++iter)
      {
        for (QgsNewAttributesMap::const_iterator it = added.begin(); it != added.end(); ++it)
        {
          (*iter).addAttribute(, QgsFeatureAttribute(it.key(), ""));
        }
    }*/
      
      //and then in the provider
      if(!mDataProvider->addAttributes(added))
      {
        returnvalue=false;
      }
    }

    if(mDataProvider->capabilities()&QgsVectorDataProvider::ChangeAttributeValues)
    {
      //and then those of the commited ones
      if(!mDataProvider->changeAttributeValues(changed))
      {
        returnvalue=false;
      }
    }
  }
  else
  {
    returnvalue=false;
  }
  return returnvalue;
}

// Convenience function to transform the given point
inline void QgsVectorLayer::transformPoint(double& x, 
    double& y, 
    QgsMapToPixel* mtp,
    QgsCoordinateTransform* ct)
{
  // transform the point
  if (ct)
  {
    double z = 0;
    ct->transformInPlace(x, y, z);
  }

  // transform from projected coordinate system to pixel 
  // position on map canvas
  mtp->transformInPlace(x, y);
}

inline void QgsVectorLayer::transformPoints(
    std::vector<double>& x, std::vector<double>& y, std::vector<double>& z,
    QgsMapToPixel* mtp, QgsCoordinateTransform* ct)
{
  // transform the point
  if (ct)
    ct->transformInPlace(x, y, z);

  // transform from projected coordinate system to pixel 
  // position on map canvas
  mtp->transformInPlace(x, y);
}


const QString QgsVectorLayer::displayField() const
{
  return mDisplayField;
}

bool QgsVectorLayer::isEditable() const
{
  return (mEditable && mDataProvider);
}

bool QgsVectorLayer::isModified() const
{
  return mModified;
}

QgsFeatureList& QgsVectorLayer::addedFeatures()
{
  return mAddedFeatures;
}

QgsFeatureIds& QgsVectorLayer::deletedFeatureIds()
{
  return mDeletedFeatureIds;
}

QgsChangedAttributesMap& QgsVectorLayer::changedAttributes()
{
  return mChangedAttributes;
}

void QgsVectorLayer::setModified(bool modified, bool onlyGeometry)
{
  mModified = modified;
  emit wasModified(onlyGeometry);
}

QString QgsVectorLayer::saveAsShapefile(QString path, QString encoding)
{
  return QgsVectorFileWriter::writeVectorLayerAsShapefile(path, encoding, this);
}
