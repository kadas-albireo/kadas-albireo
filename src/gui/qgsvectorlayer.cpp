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

#include <iostream>
#include <iosfwd>
#include <cfloat>
#include <cstring>
#include <cmath>
#include <sstream>
#include <memory>
#include <vector>
#include <utility>
#include <cassert>
#include <limits>

// for htonl
#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#endif

#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QLibrary>
#include <q3listview.h>
#include <QPainter>
#include <QPolygon>
#include <QString>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>
#include <QWidget>
#include <QPixmap>

#include "qgsapplication.h"
#include "qgisapp.h"
#include "qgsproject.h"
#include "qgsrect.h"
#include "qgspoint.h"
#include "qgsmaptopixel.h"
#include "qgsvectorlayer.h"
#include "qgsidentifyresults.h"
#include "qgsattributetable.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgslegend.h"
#include "qgsvectorlayerproperties.h"
#include "qgsrenderer.h"
#include "qgssinglesymbolrenderer.h"
#include "qgsgraduatedsymbolrenderer.h"
#include "qgscontinuouscolorrenderer.h"
#include "qgsuniquevaluerenderer.h"
#include "qgsrenderitem.h"
#include "qgsproviderregistry.h"
#include "qgsrect.h"
#include "qgslabelattributes.h"
#include "qgslabel.h"
#include "qgscoordinatetransform.h"
#include "qgsmaplayerregistry.h"
#include "qgsattributedialog.h"
#include "qgsattributetabledisplay.h"
#include "qgsdistancearea.h"
#ifdef Q_WS_X11
#include "qgsclipper.h"
#endif
#include "qgssvgcache.h"
#include "qgsspatialrefsys.h"
#include "qgis.h" //for globals
//#include "wkbheader.h"

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
  tabledisplay(0),
  m_renderer(0),
  mLabel(0),
  m_propertiesDialog(0),
  providerKey(providerKey),
  valid(false),
  myLib(0),
  ir(0),                    // initialize the identify results pointer
  updateThreshold(0),       // XXX better default value?
  mMinimumScale(0),
  mMaximumScale(0),
  mScaleDependentRender(false),
  mEditable(false),
  mModified(false),
  mToggleEditingAction(0)
{
  // if we're given a provider type, try to create and bind one to this layer
  if ( ! providerKey.isEmpty() )
  {
    setDataProvider( providerKey );
  }
  if(valid)
  {
    setCoordinateSystem();
  }

  // Default for the popup menu
  popMenu = 0;

  // Get the update threshold from user settings. We
  // do this only on construction to avoid the penality of
  // fetching this each time the layer is drawn. If the user
  // changes the threshold from the preferences dialog, it will
  // have no effect on existing layers
  QSettings settings;
  updateThreshold = settings.readNumEntry("Map/updateThreshold", 1000);
} // QgsVectorLayer ctor



QgsVectorLayer::~QgsVectorLayer()
{
#ifdef QGISDEBUG
  std::cerr << "In QgsVectorLayer destructor" << std::endl;
#endif

  valid=false;

  if(isEditable())
  {
    stopEditing();
  }

  if (tabledisplay)
  {
    tabledisplay->close();
    delete tabledisplay;
  }
  if (m_renderer)
  {
    delete m_renderer;
  }
  if (m_propertiesDialog)
  {
    delete m_propertiesDialog;
  }
  // delete the provider object
  delete dataProvider;
  // delete the popu pmenu
  delete popMenu;
  // delete the provider lib pointer
  delete myLib;

  if ( ir )
  {
    delete ir;
  }

  // Destroy and cached geometries and clear the references to them
  for (std::map<int, QgsGeometry*>::iterator it  = mCachedGeometries.begin(); 
      it != mCachedGeometries.end();
      ++it )
  {
    delete (*it).second;
  }
  mCachedGeometries.clear();

}

QString QgsVectorLayer::storageType() const
{
  if (dataProvider)
  {
    return dataProvider->storageType();
  }
  return 0;
}


QString QgsVectorLayer::capabilitiesString() const
{
  if (dataProvider)
  {
    return dataProvider->capabilitiesString();
  }
  return 0;
}

int QgsVectorLayer::getProjectionSrid()
{
  //delegate to the provider
  if (valid)
  {
#ifdef QGISDEBUG    
    std::cout << "Getting srid from provider..." << std::endl;
#endif
    return dataProvider->getSrid();
  }
  else
  {
    return 0;
  }
}

QString QgsVectorLayer::getProjectionWKT()
{
  //delegate to the provider
  if (valid)
  {
    return dataProvider->getProjectionWKT();
  }
  else
  {
    return NULL;
  }
}

QString QgsVectorLayer::providerType()
{
  return providerKey;
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

  std::vector < QgsField > fields = dataProvider->fields();
  if(!fldName.isEmpty())
  {
    // find the index for this field
    fieldIndex = fldName;
    /*
       for(int i = 0; i < fields.size(); i++)
       {
       if(QString(fields[i].name()) == fldName)
       {
       fieldIndex = i;
       break;
       }
       }
       */
  }
  else
  {
    int fieldsSize = fields.size();
    for (int j = 0; j < fieldsSize; j++)
    {

      QString fldName = fields[j].name();
#ifdef QGISDEBUG
      std::cerr << "Checking field " << fldName.toLocal8Bit().data() << " of " << fields.size() << " total" << std::endl;
#endif
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
    if (fields.size() == 0) return;

    if (idxName.length() > 0)
    {
      fieldIndex = idxName;
    }
    else
    {
      if (idxId.length() > 0)
      {
        fieldIndex = idxId;
      }
      else
      {
        fieldIndex = fields[0].name();
      }
    }

    // set this to be the label field as well
    setLabelField(fieldIndex);
  }
}

void QgsVectorLayer::drawLabels(QPainter * p, QgsRect * viewExtent, QgsMapToPixel * theMapToPixelTransform, QPaintDevice* dst)
{
  drawLabels(p, viewExtent, theMapToPixelTransform, dst, 1.);
}

// NOTE this is a temporary method added by Tim to prevent label clipping
// which was occurring when labeller was called in the main draw loop
// This method will probably be removed again in the near future!
void QgsVectorLayer::drawLabels(QPainter * p, QgsRect * viewExtent, QgsMapToPixel * theMapToPixelTransform, QPaintDevice* dst, double scale)
{
#ifdef QGISDEBUG
  qWarning("Starting draw of labels");
#endif

  if ( /*1 == 1 */ m_renderer && mLabelOn)
  {
    bool projectionsEnabledFlag = projectionsEnabled();
    std::list<int> attributes=m_renderer->classificationAttributes();
    // Add fields required for labels
    mLabel->addRequiredFields ( &attributes );

#ifdef QGISDEBUG
    qWarning("Selecting features based on view extent");
#endif

    int featureCount = 0;
    // select the records in the extent. The provider sets a spatial filter
    // and sets up the selection set for retrieval
    dataProvider->reset();
    dataProvider->select(viewExtent);

    try
    {
      //main render loop
      QgsFeature *fet;

      while((fet = dataProvider->getNextFeature(attributes)))
      {
        // Render label
        if ( fet != 0 )
        {
          if(mDeleted.find(fet->featureId())==mDeleted.end())//don't render labels of deleted features
          {
            bool sel=mSelected.find(fet->featureId()) != mSelected.end();
            mLabel->renderLabel ( p, viewExtent, *mCoordinateTransform, 
                projectionsEnabledFlag,
                theMapToPixelTransform, dst, fet, sel, 0, scale);
          }
        }
        delete fet;
        featureCount++;
      }

      //render labels of not-commited features
      //XXX changed from std::vector to std::list in merge from 0.7 to head (TS)
      for(std::vector<QgsFeature*>::iterator it=mAddedFeatures.begin();it!=mAddedFeatures.end();++it)
      {
        bool sel=mSelected.find((*it)->featureId()) != mSelected.end();
        mLabel->renderLabel ( p, viewExtent, *mCoordinateTransform, projectionsEnabledFlag,
            theMapToPixelTransform, dst, *it, sel, 0, scale);
      }
    }
    catch (QgsCsException &e)
    {
      qDebug("Error projecting label locations, caught in %s, line %d:\n%s",
          __FILE__, __LINE__, e.what());
    }

#ifdef QGISDEBUG
    std::cerr << "Total features processed is " << featureCount << std::endl;
#endif
    // XXX Something in our draw event is triggering an additional draw event when resizing [TE 01/26/06]
    // XXX Calling this will begin processing the next draw event causing image havoc and recursion crashes.
    //qApp->processEvents();

  }
}

QgsRect QgsVectorLayer::inverseProjectRect(const QgsRect& r) const
{
  // Undo a coordinate transformation if one was in effect
  if (projectionsEnabled())
  { 
    try
    {
      QgsPoint p1 = mCoordinateTransform->transform(r.xMin(), r.yMin(), 
          QgsCoordinateTransform::INVERSE);
      QgsPoint p2 = mCoordinateTransform->transform(r.xMax(), r.yMax(), 
          QgsCoordinateTransform::INVERSE);
#ifdef QGISDEBUG
      std::cerr << "Projections are enabled\n";
      std::cerr << "Rectangle was: " << r.stringRep(true).toLocal8Bit().data() << '\n';
      QgsRect tt(p1, p2);
      std::cerr << "Inverse transformed to: " << tt.stringRep(true).toLocal8Bit().data() << '\n';
#endif
      return QgsRect(p1, p2);

    }
    catch (QgsCsException &e)
    {
      qDebug("Inverse transform error in %s line %d:\n%s",
          __FILE__, __LINE__, e.what());
    }
  }
  // fall through for all failures
  return QgsRect(r);
}

unsigned char* QgsVectorLayer::drawLineString(unsigned char* feature, 
    QPainter* p,
    QgsMapToPixel* mtp, 
    bool projectionsEnabledFlag)
{
  unsigned char *ptr = feature + 5;
  unsigned int nPoints = *((int*)ptr);
  ptr = feature + 9;

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
  }

  // Transform the points into map coordinates (and reproject if
  // necessary)

  double oldx = x[0];
  double oldy = y[0];

#ifdef QGISDEBUG 
  //std::cout <<"...WKBLineString start at (" << oldx << ", " << oldy << ")" <<std::endl;
#endif

  transformPoints(x, y, z, mtp, projectionsEnabledFlag);

#if defined(Q_WS_X11)
  // Work around a +/- 32768 limitation on coordinates in X11

  // Look through the x and y coordinates and see if there are any
  // that need trimming. If one is found, there's no need to look at
  // the rest of them so end the loop at that point. 
  for (register int i = 0; i < nPoints; ++i)
    if (std::abs(x[i]) > QgsClipper::maxX ||
        std::abs(y[i]) > QgsClipper::maxY)
    {
      QgsClipper::trimFeature(x, y, true); // true = polyline
      nPoints = x.size(); // trimming may change nPoints.
      break;
    }
#endif

  // Cast points to int and put into the appropriate storage for the
  // call to QPainter::drawFeature(). Apparently QT 4 will allow
  // calls to drawPolyline with a QPointArray holding floats (or
  // doubles?), so this loop may become unnecessary (but may still need
  // the double* versions for the OGR coordinate transformation).
  QPolygon pa(nPoints);
  for (register int i = 0; i < nPoints; ++i)
  {
    // Here we assume that a static cast is as fast as a C style case
    // since type checking is done at compile time rather than
    // runtime.
    pa.setPoint(i, static_cast<int>(x[i] + 0.5),
        static_cast<int>(y[i] + 0.5));
  }
#ifdef QGISDEBUGVERBOSE
  // this is only used for verbose debug output
  for (int i = 0; i < pa.size(); ++i)
    std::cerr << pa.point(i).x() << ", " << pa.point(i).y()
      << '\n';
  std::cerr << '\n';
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
  myColor.setAlpha(transparencyLevelInt);
  myTransparentPen.setColor(myColor);
  p->setPen(myTransparentPen);
  p->drawPolyline(pa);
  //restore the pen
  p->setPen(pen);

  return ptr;
}

unsigned char* QgsVectorLayer::drawPolygon(unsigned char* feature, 
    QPainter* p, 
    QgsMapToPixel* mtp, 
    bool projectionsEnabledFlag)
{
  typedef std::pair<std::vector<double>, std::vector<double> > ringType;
  typedef ringType* ringTypePtr;
  typedef std::vector<ringTypePtr> ringsType;

  // get number of rings in the polygon
  unsigned int numRings = *((int*)(feature + 1 + sizeof(int)));

  if ( numRings == 0 )  // sanity check for zero rings in polygon
    return feature + 9;

  int total_points = 0;

  // A vector containing a pointer to a pair of double vectors.The
  // first vector in the pair contains the x coordinates, and the
  // second the y coordinates.
  ringsType rings;

  // Set pointer to the first ring
  unsigned char* ptr = feature + 1 + 2 * sizeof(int); 

  for (register unsigned int idx = 0; idx < numRings; idx++)
  {
    int nPoints = *((int*)ptr);

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
#ifdef QGISDEBUG 
      std::cout << "Ring has only " << nPoints << " points! Skipping this ring." << std::endl;
#endif 
      continue;
    }

    // Transform the points into map coordinates (and reproject if
    // necessary)
    double oldx = ring->first[0];
    double oldy = ring->second[0];

#ifdef QGISDEBUG 
    //std::cout <<"...WKBLineString start at (" << oldx << ", " << oldy << ")" <<std::endl;
#endif

    transformPoints(ring->first, ring->second, zVector, mtp, projectionsEnabledFlag);

#if defined(Q_WS_X11)
    // Work around a +/- 32768 limitation on coordinates in X11

    // Look through the x and y coordinates and see if there are any
    // that need trimming. If one is found, there's no need to look at
    // the rest of them so end the loop at that point. 
    for (register int i = 0; i < nPoints; ++i)
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

  // Only try to draw polygons if there is something to draw
  if (total_points > 0)
  {
    int ii = 0;
    QPoint outerRingPt;

    // Stores the start index of each ring (first) and the number
    // of points in each ring (second).
    typedef std::vector<std::pair<unsigned int, unsigned int> > ringDetailType;
    ringDetailType ringDetails;

    // Need to copy the polygon vertices into a QPointArray for the
    // QPainter::drawpolygon() call. The size is the sum of points in
    // the polygon plus one extra point for each ring except for the
    // first ring.

    // Store size here and use it in the loop to avoid penalty of
    // multiple calls to size()
    int numRings = rings.size();
    QPolygon pa(total_points + numRings - 1);
    for (register int i = 0; i < numRings; ++i)
    {
      // Store the pointer in a variable with a short name so as to make
      // the following code easier to type and read.
      ringTypePtr r = rings[i];
      // only do this once to avoid penalty of additional calls
      unsigned ringSize = r->first.size();

      // Store the start index of this ring, and the number of
      // points in the ring.
      ringDetails.push_back(std::make_pair(ii, ringSize));

      // Transfer points to the QPointArray
      for (register int j = 0; j != ringSize; ++j)
        pa.setPoint(ii++, static_cast<int>(r->first[j] + 0.5),
            static_cast<int>(r->second[j] + 0.5));

      // Store the last point of the first ring, and insert it at
      // the end of all other rings. This makes all the other rings
      // appear as holes in the first ring.
      if (i == 0)
      {
        outerRingPt.setX(pa.point(ii-1).x());
        outerRingPt.setY(pa.point(ii-1).y());
      }
      else
        pa.setPoint(ii++, outerRingPt);

      // Tidy up the pointed to pairs of vectors as we finish with them
      delete rings[i];
    }


#ifdef QGISDEBUGVERBOSE
    // this is only for verbose debug output -- no optimzation is 
    // needed :)
    std::cerr << "Pixel points are:\n";
    for (int i = 0; i < pa.size(); ++i)
      std::cerr << i << ": " << pa.point(i).x() << ", " 
        << pa.point(i).y() << '\n';
    std::cerr << "Ring positions are:\n";
    for (int i = 0; i < ringDetails.size(); ++i)
      std::cerr << ringDetails[i].first << ", "
        << ringDetails[i].second << "\n";      
    std::cerr << "Outer ring point is " << outerRingPt.x()
      << ", " << outerRingPt.y() << '\n';
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
    myColor.setAlpha(transparencyLevelInt);
    myTransparentBrush.setColor(myColor);
    QPen myTransparentPen = p->pen(); // store current pen
    myColor = myTransparentPen.color();
    myColor.setAlpha(transparencyLevelInt);
    myTransparentPen.setColor(myColor);
    //
    // draw the polygon fill
    // 
    p->setBrush(myTransparentBrush);
    p->setPen ( Qt::NoPen ); // no boundary
    p->drawPolygon( pa );

    // draw the polygon outline. Draw each ring as a separate
    // polygon to avoid the lines associated with the outerRingPt.
    p->setBrush ( Qt::NoBrush );
    p->setPen (myTransparentPen);

    ringDetailType::const_iterator ri = ringDetails.begin();

    for (; ri != ringDetails.end(); ++ri)
      p->drawPolygon(pa, FALSE, ri->first, ri->second);
    
    //
    //restore brush and pen to original
    //
    p->setBrush ( brush );
    p->setPen ( pen );
  }

  return ptr;
}


void QgsVectorLayer::draw(QPainter * p, QgsRect * viewExtent, QgsMapToPixel * theMapToPixelTransform, QPaintDevice* dst)
{
  draw ( p, viewExtent, theMapToPixelTransform, dst, 1., 1.);
}

void QgsVectorLayer::draw(QPainter * p, QgsRect * viewExtent, QgsMapToPixel * theMapToPixelTransform, 
    QPaintDevice* dst, double widthScale, double symbolScale)
{
  if ( /*1 == 1 */ m_renderer)
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
    QPixmap marker;
    /*Scale factor of the marker image*/
    double markerScaleFactor=1.;

    // select the records in the extent. The provider sets a spatial filter
    // and sets up the selection set for retrieval
#ifdef QGISDEBUG
    qWarning("Selecting features based on view extent");
#endif
    dataProvider->reset();

    // Destroy all cached geometries and clear the references to them
#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::draw: Destroying all cached geometries"
      << "." << std::endl;
#endif

    // TODO: This area has suspect memory management
    for (std::map<int, QgsGeometry*>::iterator it  = mCachedGeometries.begin(); 
        it != mCachedGeometries.end();
        ++it )
    {
#ifdef QGISDEBUG
      //      std::cout << "QgsVectorLayer::draw: deleting cached geometry ID "
      //                << (*it).first
      //                << "." << std::endl;
#endif
      delete (*it).second;
    }
#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::draw: Clearing all cached geometries"
      << "." << std::endl;
#endif
    mCachedGeometries.clear();

    dataProvider->select(viewExtent);
    int featureCount = 0;
    //  QgsFeature *ftest = dataProvider->getFirstFeature();
#ifdef QGISDEBUG
    qWarning("Starting draw of features");
#endif
    QgsFeature *fet;

    bool projectionsEnabledFlag = projectionsEnabled();
    std::list<int> attributes=m_renderer->classificationAttributes();

    mDrawingCancelled=false; //pressing esc will change this to true

    /*
       QTime t;
       t.start();
       */

    try
    {
      while (fet = dataProvider->getNextFeature(attributes, updateThreshold))
        //      while((fet = dataProvider->getNextFeature(attributes)))
      {

#ifdef QGISDEBUG
        //      std::cout << "QgsVectorLayer::draw: got " << fet->featureId() << std::endl; 
#endif

        // XXX Something in our draw event is triggering an additional draw event when resizing [TE 01/26/06]
        // XXX Calling this will begin processing the next draw event causing image havoc and recursion crashes.
        //qApp->processEvents(); //so we can trap for esc press
        if (mDrawingCancelled) return;
        // If update threshold is greater than 0, check to see if
        // the threshold has been exceeded
        if(updateThreshold > 0)
        {
          //copy the drawing buffer every updateThreshold elements
          if(0 == featureCount % updateThreshold)
#if QT_VERSION < 0x040000
            bitBlt(dst,0,0,p->device(),0,0,-1,-1,Qt::CopyROP,false);
#else
          // TODO: Double check if this is appropriate for Qt4 - probably better to use QPainter::drawPixmap().
          bitBlt(dst,0,0,p->device(),0,0,-1,-1,Qt::AutoColor);
#endif
        }

        if (fet == 0)
        {
#ifdef QGISDEBUG
          std::cerr << "get next feature returned null\n";
#endif
        }
        else
        {
          // Cache this for the use of (e.g.) modifying the feature's geometry.
          //          mCachedGeometries[fet->featureId()] = fet->geometryAndOwnership();

          if (mDeleted.find(fet->featureId())==mDeleted.end())
          {
            // not deleted.

            // check to see if the feature has an uncommitted modification.
            // if so, substitute the modified geometry

            //#ifdef QGISDEBUG
            //	  std::cerr << "QgsVectorLayer::draw: Looking for modified geometry for " << fet->featureId() << std::endl;
            //#endif
            if (mChangedGeometries.find(fet->featureId()) != mChangedGeometries.end())
            {
#ifdef QGISDEBUG
              std::cerr << "QgsVectorLayer::draw: Found modified geometry for " << fet->featureId() << std::endl;
#endif
              // substitute the modified geometry for the committed version
              fet->setGeometry( mChangedGeometries[ fet->featureId() ] );
            }

            // Cache this for the use of (e.g.) modifying the feature's uncommitted geometry.
            mCachedGeometries[fet->featureId()] = fet->geometryAndOwnership();

            bool sel=mSelected.find(fet->featureId()) != mSelected.end();
            m_renderer->renderFeature(p, fet, &marker, &markerScaleFactor, 
                sel, widthScale );

            double scale = markerScaleFactor * symbolScale;
            drawFeature(p,fet,theMapToPixelTransform,&marker, scale, 
                projectionsEnabledFlag);

            //test for geos performance
            //geos::Geometry* g=fet->geosGeometry();
            //delete g;
            ++featureCount;
            delete fet;
          }
        }

      }

      //std::cerr << "Time to draw was " << t.elapsed() << '\n';

      //also draw the not yet commited features
      std::vector<QgsFeature*>::iterator it = mAddedFeatures.begin();
      for(; it != mAddedFeatures.end(); ++it)
      {
        bool sel=mSelected.find((*it)->featureId()) != mSelected.end();
        m_renderer->renderFeature(p, *it, &marker, &markerScaleFactor, 
            sel, widthScale);
        double scale = markerScaleFactor * symbolScale;
        drawFeature(p,*it,theMapToPixelTransform,&marker,scale, 
            projectionsEnabledFlag);
      }
    }
    catch (QgsCsException &cse)
    {
      QString msg("Failed to transform a point while drawing a feature of type '"
          + fet->typeName() + "'. Ignoring this feature.");
      msg += cse.what();
      qWarning(msg.toLocal8Bit().data());
    }

#ifdef QGISDEBUG
    std::cerr << "Total features processed is " << featureCount << std::endl;
#endif
    // XXX Something in our draw event is triggering an additional draw event when resizing [TE 01/26/06]
    // XXX Calling this will begin processing the next draw event causing image havoc and recursion crashes.
    //qApp->processEvents();
  }
  else
  {
#ifdef QGISDEBUG
    qWarning("Warning, QgsRenderer is null in QgsVectorLayer::draw()");
#endif
  }
}


QgsVectorLayer::endian_t QgsVectorLayer::endian()
{
  //     char *chkEndian = new char[4];
  //     memset(chkEndian, '\0', 4);
  //     chkEndian[0] = 0xE8;

  //     int *ce = (int *) chkEndian;
  //     int retVal;
  //     if (232 == *ce)
  //       retVal = NDR;
  //     else
  //       retVal = XDR;

  //     delete [] chkEndian;

  return (htonl(1) == 1) ? QgsVectorLayer::XDR : QgsVectorLayer::NDR ;
}

void QgsVectorLayer::identify(QgsRect * r)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);

  QgsRect pr = inverseProjectRect(*r);
  dataProvider->select(&pr, true);
  int featureCount = 0;
  QgsFeature *fet;

  QgsDistanceArea calc;
  calc.setSourceSRS(mCoordinateTransform->sourceSRS().srsid());

  if ( !isEditable() ) {
    // display features falling within the search radius
    if( !ir )
    {
      // It is necessary to pass topLevelWidget()as parent, but there is no QWidget available.
      //
      // Win32 doesn't like this approach to creating the window and seems
      // to work fine without it [gsherman]
      QWidget *top = 0;
#ifndef WIN32
      foreach (QWidget *w, QApplication::topLevelWidgets())
      {
        if ( typeid(*w) == typeid(QgisApp) )
        {
          top = w;
          break;
        }
      }
#endif
      ir = new QgsIdentifyResults(mActions, top);

      // restore the identify window position and show it
      ir->restorePosition();
    } else {
      ir->raise();
      ir->clear();
      ir->setActions ( mActions );
    }

    while ((fet = dataProvider->getNextFeature(true)))
    {
      featureCount++;

      Q3ListViewItem *featureNode = ir->addNode("foo");
      featureNode->setText(0, fieldIndex);
      std::vector < QgsFeatureAttribute > attr = fet->attributeMap();
      // Do this only once rather than each pass through the loop
      int attrSize = attr.size();
      for (register int i = 0; i < attrSize; i++)
      {
#ifdef QGISDEBUG
        std::cout << attr[i].fieldName().toLocal8Bit().data() << " == " << fieldIndex.toLocal8Bit().data() << std::endl;
#endif
        if (attr[i].fieldName().lower() == fieldIndex)
        {
          featureNode->setText(1, attr[i].fieldValue());
        }
        ir->addAttribute(featureNode, attr[i].fieldName(), attr[i].fieldValue());
      }

      // measure distance or area
      if (vectorType() == QGis::Line)
      {
        double dist = calc.measure(fet->geometry());
        QString str = QString::number(dist/1000, 'f', 3);
        str += " km";
        ir->addAttribute(featureNode, ".Length", str);
      }
      else if (vectorType() == QGis::Polygon)
      {
        double area = calc.measure(fet->geometry());
        QString str = QString::number(area/1000000, 'f', 3);
        str += " km2";
        ir->addAttribute(featureNode, ".Area", str);
      }

      // Add actions 

      QgsAttributeAction::aIter iter = mActions.begin();
      for (register int i = 0; iter != mActions.end(); ++iter, ++i)
      {
        ir->addAction( featureNode, i, tr("action"), iter->name() );
      }

      delete fet;
    }

#ifdef QGISDEBUG
    std::cout << "Feature count on identify: " << featureCount << std::endl;
#endif

    //also test the not commited features //todo: eliminate copy past code
    /*for(std::list<QgsFeature*>::iterator it=mAddedFeatures.begin();it!=mAddedFeatures.end();++it)
      {
      if((*it)->intersects(r))
      {
      featureCount++;
      if (featureCount == 1)
      {
      ir = new QgsIdentifyResults(mActions);
      }

      QListViewItem *featureNode = ir->addNode("foo");
      featureNode->setText(0, fieldIndex);
      std::vector < QgsFeatureAttribute > attr = (*it)->attributeMap();
      for (int i = 0; i < attr.size(); i++)
      {
#ifdef QGISDEBUG
std::cout << attr[i].fieldName() << " == " << fieldIndex << std::endl;
#endif
if (attr[i].fieldName().lower() == fieldIndex)
{
featureNode->setText(1, attr[i].fieldValue());
}
ir->addAttribute(featureNode, attr[i].fieldName(), attr[i].fieldValue());
} 
}
}*/

    ir->setTitle(name() + " - " + QString::number(featureCount) + tr(" features found"));
if (featureCount == 1) 
{
  ir->showAllAttributes();
  ir->setTitle(name() + " - " + tr(" 1 feature found") );
}
if (featureCount == 0)
{
  //QMessageBox::information(0, tr("No features found"), tr("No features were found in the active layer at the point you clicked"));

  ir->setTitle(name() + " - " + tr("No features found") );
  ir->setMessage ( tr("No features found"), tr("No features were found in the active layer at the point you clicked") );
}
ir->show();
} 
else { // Edit attributes 
  // TODO: what to do if more features were selected? - nearest?
  if ( (fet = dataProvider->getNextFeature(true)) ) {
    // Was already changed?
    std::map<int,std::map<QString,QString> >::iterator it = mChangedAttributes.find(fet->featureId());

    std::vector < QgsFeatureAttribute > old;
    if ( it != mChangedAttributes.end() ) {
      std::map<QString,QString> oldattr = (*it).second;
      for( std::map<QString,QString>::iterator ait = oldattr.begin(); ait!=oldattr.end(); ++ait ) {
        old.push_back ( QgsFeatureAttribute ( (*ait).first, (*ait).second ) );
      }
    } else {
      old = fet->attributeMap();
    }
    QgsAttributeDialog ad( &old );

    if ( ad.exec()==QDialog::Accepted ) {
      std::map<QString,QString> attr;
      // Do this only once rather than each pass through the loop
      int oldSize = old.size();
      for(register int i= 0; i < oldSize; ++i) {
        attr.insert ( std::make_pair( old[i].fieldName(), ad.value(i) ) );
      }
      // Remove old if exists
      it = mChangedAttributes.find(fet->featureId());

      if ( it != mChangedAttributes.end() ) { // found
        mChangedAttributes.erase ( it );
      }

      mChangedAttributes.insert ( std::make_pair( fet->featureId(), attr ) );
      mModified=true;
    }
  }
}
QApplication::restoreOverrideCursor();
}

void QgsVectorLayer::table()
{
  if (tabledisplay)
  {
    tabledisplay->raise();
    // Give the table the most recent copy of the actions for this
    // layer.
    tabledisplay->table()->setAttributeActions(mActions);
  }
  else
  {
    // display the attribute table
    QApplication::setOverrideCursor(Qt::waitCursor);
    tabledisplay = new QgsAttributeTableDisplay(this);
    connect(tabledisplay, SIGNAL(deleted()), this, SLOT(invalidateTableDisplay()));
    tabledisplay->table()->fillTable(this);
    tabledisplay->table()->setSorting(true);


    tabledisplay->setTitle(tr("Attribute table - ") + name());
    tabledisplay->show();
    tabledisplay->table()->clearSelection();  //deselect the first row

    // Give the table the most recent copy of the actions for this
    // layer.
    tabledisplay->table()->setAttributeActions(mActions);

    QObject::disconnect(tabledisplay->table(), SIGNAL(selectionChanged()), tabledisplay->table(), SLOT(handleChangedSelections()));

    for (std::set<int>::iterator it = mSelected.begin(); it != mSelected.end(); ++it)
    {
      tabledisplay->table()->selectRowWithId(*it);//todo: avoid that the table gets repainted during each selection
#ifdef QGISDEBUG
      qWarning(("selecting row with id " + QString::number(*it)).toLocal8Bit().data());
#endif

    }

    QObject::connect(tabledisplay->table(), SIGNAL(selectionChanged()), tabledisplay->table(), SLOT(handleChangedSelections()));

    //etablish the necessary connections between the table and the shapefilelayer
    QObject::connect(tabledisplay->table(), SIGNAL(selected(int)), this, SLOT(select(int)));
    QObject::connect(tabledisplay->table(), SIGNAL(selectionRemoved()), this, SLOT(removeSelection()));
    QObject::connect(tabledisplay->table(), SIGNAL(repaintRequested()), this, SLOT(triggerRepaint()));
    QApplication::restoreOverrideCursor();
  }

} // QgsVectorLayer::table

void QgsVectorLayer::select(int number)
{
  mSelected.insert(number);
}

void QgsVectorLayer::select(QgsRect * rect, bool lock)
{
  QApplication::setOverrideCursor(Qt::WaitCursor);
  // normalize the rectangle
  rect->normalize();
  if (tabledisplay)
  {
    QObject::disconnect(tabledisplay->table(), SIGNAL(selectionChanged()), tabledisplay->table(), SLOT(handleChangedSelections()));
    QObject::disconnect(tabledisplay->table(), SIGNAL(selected(int)), this, SLOT(select(int))); //disconnecting because of performance reason
  }

  if (lock == false)
  {
    removeSelection();        //only if ctrl-button is not pressed
    if (tabledisplay)
    {
      tabledisplay->table()->clearSelection();
    }
  }

  QgsRect r = inverseProjectRect(*rect);
  dataProvider->select(&r, true);

  QgsFeature *fet;

  while (fet = dataProvider->getNextFeature(false))
  {
    if(mDeleted.find(fet->featureId())==mDeleted.end())//don't select deleted features
    {
      select(fet->featureId());
      if (tabledisplay)
      {
        tabledisplay->table()->selectRowWithId(fet->featureId());
      }
    }
    delete fet;
  }

  //also test the not commited features
  for(std::vector<QgsFeature*>::iterator it=mAddedFeatures.begin();it!=mAddedFeatures.end();++it)
  {
    if((*it)->geometry()->intersects(rect))
    {
      select((*it)->featureId());
      if (tabledisplay)
      {
        tabledisplay->table()->selectRowWithId((*it)->featureId());
      }
    }
  }

  if (tabledisplay)
  {
    QObject::connect(tabledisplay->table(), SIGNAL(selectionChanged()), tabledisplay->table(), SLOT(handleChangedSelections()));
    QObject::connect(tabledisplay->table(), SIGNAL(selected(int)), this, SLOT(select(int)));  //disconnecting because of performance reason
  }
  triggerRepaint();
  QApplication::restoreOverrideCursor();

  emit selectionChanged();
}

void QgsVectorLayer::invertSelection()
{
  QApplication::setOverrideCursor(Qt::waitCursor);
  if (tabledisplay)
  {
    QObject::disconnect(tabledisplay->table(), SIGNAL(selectionChanged()), tabledisplay->table(), SLOT(handleChangedSelections()));
    QObject::disconnect(tabledisplay->table(), SIGNAL(selected(int)), this, SLOT(select(int))); //disconnecting because of performance reason
    tabledisplay->hide();
  }


  //copy the ids of selected features to tmp
  std::list<int> tmp;
  for(std::set<int>::iterator iter=mSelected.begin();iter!=mSelected.end();++iter)
  {
    tmp.push_back(*iter);
  }

  removeSelection();
  if (tabledisplay)
  {
    tabledisplay->table()->clearSelection();
  }

  QgsFeature *fet;
  dataProvider->reset();

  while (fet = dataProvider->getNextFeature(true))
  {
    if(mDeleted.find(fet->featureId())==mDeleted.end())//don't select deleted features
    {
      select(fet->featureId());
    }
  }

  for(std::vector<QgsFeature*>::iterator iter=mAddedFeatures.begin();iter!=mAddedFeatures.end();++iter)
  {
    select((*iter)->featureId());
  }

  for(std::list<int>::iterator iter=tmp.begin();iter!=tmp.end();++iter)
  {
    mSelected.erase(*iter);
  }

  if(tabledisplay)
  {
    QProgressDialog progress( tr("Invert Selection..."), tr("Abort"), 0, mSelected.size(), tabledisplay);
    int i=0;
    for(std::set<int>::iterator iter=mSelected.begin();iter!=mSelected.end();++iter)
    {
      ++i;
      progress.setValue(i);
      qApp->processEvents();
      if(progress.wasCanceled())
      {
        //deselect the remaining features if action was canceled
        mSelected.erase(iter,--mSelected.end());
        break;
      }
      tabledisplay->table()->selectRowWithId(*iter);//todo: avoid that the table gets repainted during each selection
    }
  }

  if (tabledisplay)
  {
    QObject::connect(tabledisplay->table(), SIGNAL(selectionChanged()), tabledisplay->table(), SLOT(handleChangedSelections()));
    QObject::connect(tabledisplay->table(), SIGNAL(selected(int)), this, SLOT(select(int)));  //disconnecting because of performance reason
    tabledisplay->show();
  }

  triggerRepaint();
  QApplication::restoreOverrideCursor();

  emit selectionChanged();
}

void QgsVectorLayer::removeSelection()
{
  mSelected.clear();
}

void QgsVectorLayer::triggerRepaint()
{ 
  emit repaintRequested();
}

void QgsVectorLayer::invalidateTableDisplay()
{
  tabledisplay = 0;
}

QgsVectorDataProvider* QgsVectorLayer::getDataProvider()
{
  return dataProvider;
}

void QgsVectorLayer::setProviderEncoding(const QString& encoding)
{
  if(dataProvider)
  {
    dataProvider->setEncoding(encoding);
  }
}


void QgsVectorLayer::showLayerProperties()
{
  // Set wait cursor while the property dialog is created
  // and initialized
  qApp->setOverrideCursor(QCursor(Qt::WaitCursor));


  if (!m_propertiesDialog)
  {
#ifdef QGISDEBUG
    std::cerr << "Creating new QgsVectorLayerProperties object\n";
#endif
    m_propertiesDialog = new QgsVectorLayerProperties(this);
    // Make sure that the UI starts out with the correct display
    // field value
#ifdef QGISDEBUG
    std::cerr << "Setting display field in prop dialog\n";
#endif
    m_propertiesDialog->setDisplayField(displayField());

#ifdef QGISDEBUG
    std::cerr << "Resetting prop dialog\n";
#endif
    m_propertiesDialog->reset();
#ifdef QGISDEBUG
    std::cerr << "Raising prop dialog\n";
#endif
    m_propertiesDialog->raise();
#ifdef QGISDEBUG
    std::cerr << "Showing prop dialog\n";
#endif
    m_propertiesDialog->show();
  }
  else
  {
    m_propertiesDialog->show();
    m_propertiesDialog->raise();
  }

  // restore normal cursor
  qApp->restoreOverrideCursor();
}


const QgsRenderer* QgsVectorLayer::renderer() const
{
  return m_renderer;
}

void QgsVectorLayer::setRenderer(QgsRenderer * r)
{
  if (r != m_renderer)
  {
    delete m_renderer;
    m_renderer = r;
  }
}

QGis::VectorType QgsVectorLayer::vectorType() const
{
  if (dataProvider)
  {
    int type = dataProvider->geometryType();
    switch (type)
    {
      case QGis::WKBPoint:
        return QGis::Point;
      case QGis::WKBLineString:
        return QGis::Line;
      case QGis::WKBPolygon:
        return QGis::Polygon;
      case QGis::WKBMultiPoint:
        return QGis::Point;
      case QGis::WKBMultiLineString:
        return QGis::Line;
      case QGis::WKBMultiPolygon:
        return QGis::Polygon;
    }
  }
  else
  {
#ifdef QGISDEBUG
    qWarning("warning, pointer to dataProvider is null in QgsVectorLayer::vectorType()");
#endif

  }
}

QgsVectorLayerProperties *QgsVectorLayer::propertiesDialog()
{
  return m_propertiesDialog;
}


void QgsVectorLayer::initContextMenu_(QgisApp * app)
{
  myPopupLabel->setText( tr("Vector Layer") );

  popMenu->addAction(tr("&Open attribute table"), app, SLOT(attributeTable()));

  popMenu->addSeparator();

  int cap=dataProvider->capabilities();
  if((cap&QgsVectorDataProvider::AddFeatures)
      ||(cap&QgsVectorDataProvider::DeleteFeatures))
  {
    mToggleEditingAction = popMenu->addAction(tr("Allow Editing"),this,SLOT(toggleEditing()));
    mToggleEditingAction->setCheckable(true);
  }

  if(cap&QgsVectorDataProvider::SaveAsShapefile)
  {
    // add the save as shapefile menu item
    popMenu->addSeparator();
    popMenu->addAction(tr("Save as shapefile..."), this, SLOT(saveAsShapefile()));
  }

} // QgsVectorLayer::initContextMenu_(QgisApp * app)

QgsRect QgsVectorLayer::bBoxOfSelected()
{
  if(mSelected.size()==0)//no selected features
  {
    return QgsRect(0,0,0,0);
  }

  QgsRect r, retval;
  QgsFeature* fet;
  dataProvider->reset();

  retval.setMinimal();
  while ((fet = dataProvider->getNextFeature(false)))
  {
    if (mSelected.find(fet->featureId()) != mSelected.end())
    {
      r=fet->geometry()->boundingBox();
      retval.combineExtentWith(&r);
    }
    delete fet;
  }
  //also go through the not commited features
  for(std::vector<QgsFeature*>::iterator iter=mAddedFeatures.begin();iter!=mAddedFeatures.end();++iter)
  {
    if(mSelected.find((*iter)->featureId())!=mSelected.end())
    {
      r=(*iter)->geometry()->boundingBox();
      retval.combineExtentWith(&r);
    }
  }
  return retval;
}

void QgsVectorLayer::setLayerProperties(QgsVectorLayerProperties * properties)
{
  m_propertiesDialog = properties;
  // Make sure that the UI gets the correct display
  // field value

  if(m_propertiesDialog)
  {
    m_propertiesDialog->setDisplayField(displayField());
  }
}



QgsFeature * QgsVectorLayer::getFirstFeature(bool fetchAttributes, bool selected) const
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::getFirstFeature() invoked with null dataProvider\n";
    return 0x0;
  }

  if ( selected )
  {
    QgsFeature *fet = dataProvider->getFirstFeature(fetchAttributes);
    while(fet)
    {
      bool sel = mSelected.find(fet->featureId()) != mSelected.end();
      if ( sel ) return fet;
      fet = dataProvider->getNextFeature(fetchAttributes);
    }
    return 0;
  }

  return dataProvider->getFirstFeature( fetchAttributes );
} // QgsVectorLayer::getFirstFeature


QgsFeature * QgsVectorLayer::getNextFeature(bool fetchAttributes, bool selected) const
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::getNextFeature() invoked with null dataProvider\n";
    return 0x0;
  }

  if ( selected )
  {
    QgsFeature *fet;
    while ( fet = dataProvider->getNextFeature(fetchAttributes) )
    {
      bool sel = mSelected.find(fet->featureId()) != mSelected.end();
      if ( sel ) return fet;
    }
    return 0;
  }

  return dataProvider->getNextFeature( fetchAttributes );
} // QgsVectorLayer::getNextFeature



bool QgsVectorLayer::getNextFeature(QgsFeature &feature, bool fetchAttributes) const
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::getNextFeature() invoked with null dataProvider\n";
    return false;
  }

  return dataProvider->getNextFeature( feature, fetchAttributes );
} // QgsVectorLayer::getNextFeature



long QgsVectorLayer::featureCount() const
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::featureCount() invoked with null dataProvider\n";
    return 0;
  }

  return dataProvider->featureCount();
} // QgsVectorLayer::featureCount

long QgsVectorLayer::updateFeatureCount() const
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::updateFeatureCount() invoked with null dataProvider\n";
    return 0;
  }
  return dataProvider->updateFeatureCount();
}

void QgsVectorLayer::updateExtents()
{
  if(dataProvider)
  {
    if(mDeleted.size()==0)
    {
      // get the extent of the layer from the provider
      layerExtent.setXmin(dataProvider->extent()->xMin());
      layerExtent.setYmin(dataProvider->extent()->yMin());
      layerExtent.setXmax(dataProvider->extent()->xMax());
      layerExtent.setYmax(dataProvider->extent()->yMax());
    }
    else
    {
      QgsFeature* fet=0;
      QgsRect bb;

      layerExtent.setMinimal();
      dataProvider->reset();
      while(fet=dataProvider->getNextFeature(false))
      {
        if(mDeleted.find(fet->featureId())==mDeleted.end())
        {
          bb=fet->boundingBox();
          layerExtent.combineExtentWith(&bb);
        }
        delete fet;
      }
    }
  }
  else
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::updateFeatureCount() invoked with null dataProvider\n";
  }

  //todo: also consider the not commited features
  for(std::vector<QgsFeature*>::iterator iter=mAddedFeatures.begin();iter!=mAddedFeatures.end();++iter)
  {
    QgsRect bb=(*iter)->boundingBox();
    layerExtent.combineExtentWith(&bb);

  }

  // Send this (hopefully) up the chain to the map canvas
  emit recalculateExtents();
}

QString QgsVectorLayer::subsetString()
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::subsetString() invoked with null dataProvider\n";
    return 0;
  }
  return dataProvider->subsetString();
}
void QgsVectorLayer::setSubsetString(QString subset)
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::setSubsetString() invoked with null dataProvider\n";
  }
  else
  {
    dataProvider->setSubsetString(subset);
    // get the updated data source string from the provider
    dataSource = dataProvider->getDataSourceUri();
    updateExtents();
  }
  //trigger a recalculate extents request to any attached canvases
#ifdef QGISDEBUG
  std::cout << "Subset query changed, emitting recalculateExtents() signal" << std::endl;
#endif
  // emit the signal  to inform any listeners that the extent of this
  // layer has changed
  emit recalculateExtents();
}
int QgsVectorLayer::fieldCount() const
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::fieldCount() invoked with null dataProvider\n";
    return 0;
  }

  return dataProvider->fieldCount();
} // QgsVectorLayer::fieldCount


std::vector<QgsField> const& QgsVectorLayer::fields() const
{
  if ( ! dataProvider )
  {
    std::cerr << __FILE__ << ":" << __LINE__
      << " QgsVectorLayer::fields() invoked with null dataProvider\n";

    static std::vector<QgsField> bogus; // empty, bogus container
    return bogus;
  }

  return dataProvider->fields();
} // QgsVectorLayer::fields()


bool QgsVectorLayer::addFeature(QgsFeature* f, bool alsoUpdateExtent)
{
  static int addedIdLowWaterMark = 0;

  if(dataProvider)
  {
    if(!(dataProvider->capabilities() & QgsVectorDataProvider::AddFeatures))
    {
      QMessageBox::information(0, tr("Layer cannot be added to"), 
          tr("The data provider for this layer does not support the addition of features."));
      return false;
    }

    if(!isEditable())
    {
      QMessageBox::information(0, tr("Layer not editable"), 
          tr("The current layer is not editable. Choose 'Allow editing' in the legend item right click menu."));
      return false;
    }

    //set the endian properly
    int end=endian();
    memcpy(f->getGeometry(),&end,1);

    /*    //assign a temporary id to the feature
          int tempid;
          if(mAddedFeatures.size()==0)
          {
          tempid=findFreeId();
          }
          else
          {
          std::list<QgsFeature*>::const_reverse_iterator rit=mAddedFeatures.rbegin();
          tempid=(*rit)->featureId()+1;
          }
#ifdef QGISDEBUG
qWarning("assigned feature id "+QString::number(tempid));
#endif*/

    //assign a temporary id to the feature (use negative numbers)
    addedIdLowWaterMark--;

#ifdef QGISDEBUG
    qWarning("assigned feature id "+QString::number(addedIdLowWaterMark));
#endif

    // Change the fields on the feature to suit the destination
    // in the paste transformation transfer.
    // TODO: Could be done more efficiently for large pastes
    std::map<int, QString> fields = f->fields();

#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::addFeature: about to traverse fields." << std::endl;
#endif
    for (std::map<int, QString>::iterator it  = fields.begin();
        it != fields.end();
        ++it)
    {
#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::addFeature: inspecting field '"
        << (it->second).toLocal8Bit().data()
        << "'." << std::endl;
#endif

    }

    // Force a feature ID (to keep other functions in QGIS happy,
    // providers will use their own new feature ID when we commit the new feature)
    // and add to the known added features.
    f->setFeatureId(addedIdLowWaterMark);
    mAddedFeatures.push_back(f);
    mModified=true;

    //hide and delete the table because it is not up to date any more
    if (tabledisplay)
    {
      tabledisplay->close();
      delete tabledisplay;
      tabledisplay=0;
    }

    if (alsoUpdateExtent)
    {
      updateExtents();
    }  

    return true;
  }
  return false;
}


bool QgsVectorLayer::insertVertexBefore(double x, double y, int atFeatureId,
    QgsGeometryVertexIndex beforeVertex)
{

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::insertVertexBefore: entered with featureId " << atFeatureId << "." << std::endl;
#endif

  if (dataProvider)
  {

    if ( mChangedGeometries.find(atFeatureId) == mChangedGeometries.end() )
    {
      // first time this geometry has changed since last commit

      mChangedGeometries[atFeatureId] = *(mCachedGeometries[atFeatureId]);
#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::insertVertexBefore: first uncommitted change for this geometry." << std::endl;
#endif
    }

    mChangedGeometries[atFeatureId].insertVertexBefore(x, y, beforeVertex);

    mModified=true;

    // TODO - refresh map here?

#ifdef QGISDEBUG
    // TODO
#endif

    return true;
  }
  return false;
}


bool QgsVectorLayer::moveVertexAt(double x, double y, int atFeatureId,
    QgsGeometryVertexIndex atVertex)
{
  //TODO

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::moveVertexAt: entered with featureId " << atFeatureId << "." << std::endl;
#endif

  if (dataProvider)
  {

    if ( mChangedGeometries.find(atFeatureId) == mChangedGeometries.end() )
    {
      // first time this geometry has changed since last commit

      mChangedGeometries[atFeatureId] = *(mCachedGeometries[atFeatureId]);
#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::moveVertexAt: first uncommitted change for this geometry." << std::endl;
#endif
    }

    mChangedGeometries[atFeatureId].moveVertexAt(x, y, atVertex);

    mModified=true;

    // TODO - refresh map here?

#ifdef QGISDEBUG
    // TODO
#endif

    return true;
  }
  return false;
}


bool QgsVectorLayer::deleteVertexAt(int atFeatureId,
    QgsGeometryVertexIndex atVertex)
{
  //TODO

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::deleteVertexAt: entered with featureId " << atFeatureId << "." << std::endl;
#endif

  if (dataProvider)
  {

    if ( mChangedGeometries.find(atFeatureId) == mChangedGeometries.end() )
    {
      // first time this geometry has changed since last commit

      mChangedGeometries[atFeatureId] = *(mCachedGeometries[atFeatureId]);
#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::deleteVertexAt: first uncommitted change for this geometry." << std::endl;
#endif
    }

    // TODO: Hanlde if we delete the only remaining meaningful vertex of a geometry
    mChangedGeometries[atFeatureId].deleteVertexAt(atVertex);

    mModified=true;

    // TODO - refresh map here?

#ifdef QGISDEBUG
    // TODO
#endif

    return true;
  }
  return false;
}


QString QgsVectorLayer::getDefaultValue(const QString& attr,
    QgsFeature* f)
{
  return dataProvider->getDefaultValue(attr, f);
}

bool QgsVectorLayer::deleteSelectedFeatures()
{
  if(!(dataProvider->capabilities() & QgsVectorDataProvider::DeleteFeatures))
  {
    QMessageBox::information(0, tr("Provider does not support deletion"), 
        tr("Data provider does not support deleting features"));
    return false;
  }

  if(!isEditable())
  {
    QMessageBox::information(0, tr("Layer not editable"), 
        tr("The current layer is not editable. Choose 'Allow editing' in the legend item right click menu"));
    return false;
  }

  for(std::set<int>::iterator it=mSelected.begin();it!=mSelected.end();++it)
  {
    bool notcommitedfeature=false;
    //first test, if the feature with this id is a not-commited feature
    for(std::vector<QgsFeature*>::iterator iter=mAddedFeatures.begin();iter!=mAddedFeatures.end();++iter)
    {
      if((*it)==(*iter)->featureId())
      {
        // Delete the feature itself before deleting the reference to it.
        delete *iter;
        mAddedFeatures.erase(iter);

        notcommitedfeature=true;
        break;
      }
    }
    if(notcommitedfeature)
    {
      break;
    }
    mDeleted.insert(*it);
  }

  if(mSelected.size()>0)
  {
    mModified=true;
    /*      mSelected.clear();*/
    removeSelection();
    triggerRepaint();
    updateExtents();

    //hide and delete the table because it is not up to date any more
    if (tabledisplay)
    {
      tabledisplay->close();
      delete tabledisplay;
      tabledisplay=0;
    }

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

void QgsVectorLayer::toggleEditing()
{
  if(mToggleEditingAction)
  {
    if (mToggleEditingAction->isChecked() ) //checking of the QAction is done before calling this slot
    {
      startEditing();
    }
    else
    {
      stopEditing();
    }
  }
}


void QgsVectorLayer::startEditing()
{
  if(dataProvider)
  {
    if(!(dataProvider->capabilities()&QgsVectorDataProvider::AddFeatures))
    {
      QMessageBox::information(0,tr("Start editing failed"),
                               tr("Provider cannot be opened for editing"),
                               QMessageBox::Ok);
    }
    else
    {
      mEditable=true;
      if(isValid())
      {
        updateItemPixmap();
        if(mToggleEditingAction)
        {
          mToggleEditingAction->setChecked(true);
        }
      }
    }
  }
}

void QgsVectorLayer::stopEditing()
{
  if(dataProvider)
  {
    if(mModified)
    {
      //commit or roll back?
      int commit=QMessageBox::information(0,tr("Stop editing"),tr("Do you want to save the changes?"),tr("&Yes"),tr("&No"),QString::null,0,1);	

      if(commit==0)
      {
        if(!commitChanges())
        {
          QMessageBox::information(0,tr("Error"),tr("Could not commit changes"),QMessageBox::Ok);
        }
        else
        {
          dataProvider->updateExtents();
          //hide and delete the table because it is not up to date any more
          if (tabledisplay)
          {
            tabledisplay->close();
            delete tabledisplay;
            tabledisplay=0;
          }
        }
      }
      else if(commit==1)
      {
        if(!rollBack())
        {
          QMessageBox::information(0,tr("Error"),
              tr("Problems during roll back"),QMessageBox::Ok);
        }
        //hide and delete the table because it is not up to date any more
        if (tabledisplay)
        {
          tabledisplay->close();
          delete tabledisplay;
          tabledisplay=0;
        }
      }
      emit editingStopped(true);
      triggerRepaint();
    }
    else
    {
      emit editingStopped(false);
    }
    mEditable=false;
    mModified=false;
    if(isValid())
    {
      updateItemPixmap();
      if(mToggleEditingAction)
      {
        mToggleEditingAction->setChecked(false);
      }
    }
  }
}

// return state of scale dependent rendering. True if features should
// only be rendered if between mMinimumScale and mMaximumScale
bool QgsVectorLayer::scaleDependentRender()
{
  return mScaleDependentRender;
}


// Return the minimum scale at which the layer is rendered
int QgsVectorLayer::minimumScale()
{
  return mMinimumScale;
}
// Return the maximum scale at which the layer is rendered
int QgsVectorLayer::maximumScale()
{
  return mMaximumScale;
}



bool QgsVectorLayer::readXML_( QDomNode & layer_node )
{
#ifdef QGISDEBUG
  std::cerr << "Datasource in QgsVectorLayer::readXML_: " << dataSource.toLocal8Bit().data() << std::endl;
#endif
  // process the attribute actions
  mActions.readXML(layer_node);

  //process provider key
  QDomNode pkeyNode = layer_node.namedItem("provider");

  if (pkeyNode.isNull())
  {
    providerKey = "";
  }
  else
  {
    QDomElement pkeyElt = pkeyNode.toElement();
    providerKey = pkeyElt.text();
  }

  // determine type of vector layer
  if ( ! providerKey.isNull() )
  {
    // if the provider string isn't empty, then we successfully
    // got the stored provider
  }
  else if ((dataSource.find("host=") > -1) &&
      (dataSource.find("dbname=") > -1))
  {
    providerKey = "postgres";
  }
  else
  {
    providerKey = "ogr";
  }

#ifdef QGISDEBUG
  const char * dataproviderStr = providerKey.ascii(); // debugger probe
#endif

  if ( ! setDataProvider( providerKey ) )
  {
    return false;
  }

  //read provider encoding
  QDomNode encodingNode = layer_node.namedItem("encoding");
  if( ! encodingNode.isNull() && dataProvider )
  {
    dataProvider->setEncoding(encodingNode.toElement().text());
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

  return valid;               // should be true if read successfully

} // void QgsVectorLayer::readXML_



bool QgsVectorLayer::setDataProvider( QString const & provider )
{
  // XXX should I check for and possibly delete any pre-existing providers?
  // XXX How often will that scenario occur?

  providerKey = provider;     // XXX is this necessary?  Usually already set
  // XXX when execution gets here.

  //XXX - This was a dynamic cast but that kills the Windows
  //      version big-time with an abnormal termination error
  dataProvider = 
    (QgsVectorDataProvider*)(QgsProviderRegistry::instance()->getProvider(provider,dataSource));

  if (dataProvider)
  {
    QgsDebug( "Instantiated the data provider plugin" );

    if (dataProvider->isValid())
    {
      valid = true;

      // TODO: Check if the provider has the capability to send fullExtentCalculated
      connect(dataProvider, SIGNAL( fullExtentCalculated() ), 
          this,           SLOT( updateExtents() ) 
          );

      // Connect the repaintRequested chain from the data provider to this map layer
      // in the hope that the map canvas will notice       
      connect(dataProvider, SIGNAL( repaintRequested() ), 
          this,           SLOT( triggerRepaint() ) 
          );

      // get the extent
      QgsRect *mbr = dataProvider->extent();

      // show the extent
      QString s = mbr->stringRep();
#ifdef QGISDEBUG
      std::cout << "Extent of layer: " << s.toLocal8Bit().data() << std::endl;
#endif
      // store the extent
      layerExtent.setXmax(mbr->xMax());
      layerExtent.setXmin(mbr->xMin());
      layerExtent.setYmax(mbr->yMax());
      layerExtent.setYmin(mbr->yMin());

      // get and store the feature type
      geometryType = dataProvider->geometryType();

      // look at the fields in the layer and set the primary
      // display field using some real fuzzy logic
      setDisplayField();

      if (providerKey == "postgres")
      {
#ifdef QGISDEBUG
        std::cout << "Beautifying layer name " << layerName.toLocal8Bit().data() << std::endl;
#endif
        // adjust the display name for postgres layers
        layerName = layerName.mid(layerName.find(".") + 1);
        layerName = layerName.left(layerName.find("(") - 1);   // Take one away, to avoid a trailing space
#ifdef QGISDEBUG
        std::cout << "Beautified name is " << layerName.toLocal8Bit().data() << std::endl;
#endif

      }

      // upper case the first letter of the layer name
      layerName = layerName.left(1).upper() + layerName.mid(1);

      // label
      mLabel = new QgsLabel ( dataProvider->fields() );
      mLabelOn = false;
    }
    else
    {
#ifdef QGISDEBUG
      qDebug( "%s:%d invalid provider plugin %s", 
          __FILE__, __LINE__, dataSource.ascii() );
      return false;
#endif
    }
  }
  else
  {
    QgsDebug( " unable to get data provider" );

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
  QDomText encodingText = document.createTextNode(dataProvider->encoding());
  encoding.appendChild( encodingText );
  layer_node.appendChild( encoding );

  //classification field(s)
  std::list<int> attributes=m_renderer->classificationAttributes();
  const std::vector<QgsField> providerFields = dataProvider->fields();
  for(std::list<int>::const_iterator it = attributes.begin(); it != attributes.end(); ++it)
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

  mActions.writeXML(layer_node, document);

  // renderer specific settings

  const QgsRenderer * myRenderer;
  if( myRenderer = renderer())
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

  QgsLabel * myLabel;

  if ( myLabel = this->label() )
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


/** we wouldn't have to do this if slots were inherited */
void QgsVectorLayer::inOverview( bool b )
{
  QgsMapLayer::inOverview( b );
}

int QgsVectorLayer::findFreeId()
{
  int freeid=-INT_MAX;
  int fid;
  if(dataProvider)
  {
    dataProvider->reset();
    QgsFeature *fet;

    //TODO: Is there an easier way of doing this other than iteration?
    //TODO: Also, what about race conditions between this code and a competing mapping client?
    //TODO: Maybe push this to the data provider?
    while ((fet = dataProvider->getNextFeature(true)))
    {
      fid=fet->featureId();
      if(fid>freeid)
      {
        freeid=fid;
      }
      delete fet;
    }
#ifdef QGISDEBUG
    qWarning(("freeid is: "+QString::number(freeid+1)).toLocal8Bit().data());
#endif
    return freeid+1;
  }
  else
  {
#ifdef QGISDEBUG
    qWarning("Error, dataProvider is 0 in QgsVectorLayer::findFreeId");
#endif
    return -1;
  }
}

bool QgsVectorLayer::commitChanges()
{
  if(dataProvider)
  {
#ifdef QGISDEBUG
    qWarning("in QgsVectorLayer::commitChanges");
#endif
    bool returnvalue=true;

#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::commitChanges: Committing new features"
      << "." << std::endl;

    for(std::vector<QgsFeature*>::iterator it=mAddedFeatures.begin();it!=mAddedFeatures.end();++it)
    {
      std::cout << "QgsVectorLayer::commitChanges: Got: " << (*it)->geometry()->wkt().toLocal8Bit().data()
        << "." << std::endl;
    }

#endif

    // Commit new features
    std::list<QgsFeature*> addedlist;
    for(std::vector<QgsFeature*>::iterator it=mAddedFeatures.begin();it!=mAddedFeatures.end();++it)
    {
      addedlist.push_back(*it);
    }

    if(!dataProvider->addFeatures(addedlist))
      // TODO: Make the Provider accept a pointer to vector instead of a list - more memory efficient
      //    if ( !(dataProvider->addFeatures(mAddedFeatures)) )
    {
      returnvalue=false;
    }

    // Delete the features themselves before deleting the references to them.
    for(std::vector<QgsFeature*>::iterator it=mAddedFeatures.begin();it!=mAddedFeatures.end();++it)
    {
      delete *it;
    }
    mAddedFeatures.clear();

#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::commitChanges: Committing changed attributes"
      << "." << std::endl;
#endif

    // Commit changed attributes
    if( mChangedAttributes.size() > 0 ) 
    {
      if ( !dataProvider->changeAttributeValues ( mChangedAttributes ) ) 
      {
        QMessageBox::warning(0,tr("Warning"),tr("Could not change attributes"));
      }
      mChangedAttributes.clear();
    }

#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::commitChanges: Committing changed geometries"
      << "." << std::endl;
#endif
    // Commit changed geometries
    if( mChangedGeometries.size() > 0 ) 
    {
      if ( !dataProvider->changeGeometryValues ( mChangedGeometries ) ) 
      {
        QMessageBox::warning(0,tr("Error"),tr("Could not commit changes to geometries"));
      }
      mChangedGeometries.clear();
    }


#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::commitChanges: Committing deleted features"
      << "." << std::endl;
#endif

    // Commit deleted features
    if(mDeleted.size()>0)
    {
      std::list<int> deletelist;
      for(std::set<int>::iterator it=mDeleted.begin();it!=mDeleted.end();++it)
      {
        deletelist.push_back(*it);
        mSelected.erase(*it);//just in case the feature is still selected
      }
      if(!dataProvider->deleteFeatures(deletelist))
      {
        returnvalue=false;
      }
    }

    return returnvalue;
  }
  else
  {
    return false;
  }
}

bool QgsVectorLayer::rollBack()
{
  // TODO: Roll back changed features

  // Roll back added features
  // Delete the features themselves before deleting the references to them.
  for(std::vector<QgsFeature*>::iterator it=mAddedFeatures.begin();it!=mAddedFeatures.end();++it)
  {
    delete *it;
  }
  mAddedFeatures.clear();

  // Roll back deleted features
  mDeleted.clear();

  updateExtents();
  return true;
}


std::vector<QgsFeature>* QgsVectorLayer::selectedFeatures()
{
#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::selectedFeatures: entering"
    << "." << std::endl;
#endif

  if (!dataProvider)
  {
    return 0;
  }

  //TODO: Maybe make this a bit more heap-friendly (i.e. see where we can use references instead of copies)
  std::vector<QgsFeature>* features = new std::vector<QgsFeature>;

  for (std::set<int>::iterator it  = mSelected.begin();
      it != mSelected.end();
      ++it)
  {
    // Check this selected item against the committed or changed features
    if ( mCachedGeometries.find(*it) != mCachedGeometries.end() )
    {
#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::selectedFeatures: found a cached geometry: " 
        << std::endl;
#endif

      QgsFeature* f = new QgsFeature();
      int row = 0;  //TODO: Get rid of this

      dataProvider->getFeatureAttributes(*it, row, f);

      // TODO: Should deep-copy here
      f->setGeometry(*mCachedGeometries[*it]);

#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::selectedFeatures: '" << f->geometry()->wkt().toLocal8Bit().data() << "'"
        << "." << std::endl;
#endif

      // TODO: Mutate with uncommitted attributes / geometry

      // TODO: Retrieve details from provider
      /*      features.push_back(
              QgsFeature(mCachedFeatures[*it],
              mChangedAttributes,
              mChangedGeometries)
              );*/

      features->push_back(*f);

#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::selectedFeatures: added to feature vector"
        << "." << std::endl;
#endif

    }

    // Check this selected item against the uncommitted added features
    for (std::vector<QgsFeature*>::iterator iter  = mAddedFeatures.begin();
        iter != mAddedFeatures.end();
        ++iter)
    {
      if ( (*it) == (*iter)->featureId() )
      {
#ifdef QGISDEBUG
        std::cout << "QgsVectorLayer::selectedFeatures: found an added geometry: " 
          << std::endl;
#endif
        features->push_back( **iter );
        break;
      }
    }

#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::selectedFeatures: finished with feature ID " << (*it)
      << "." << std::endl;
#endif

  } // for each selected

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::selectedFeatures: exiting"
    << "." << std::endl;
#endif

  return features;
}

bool QgsVectorLayer::addFeatures(std::vector<QgsFeature*>* features, bool makeSelected)
{
  if (dataProvider)
  {  
    if(!(dataProvider->capabilities() & QgsVectorDataProvider::AddFeatures))
    {
      QMessageBox::information(0, tr("Layer cannot be added to"), 
          tr("The data provider for this layer does not support the addition of features."));
      return false;
    }

    if(!isEditable())
    {
      QMessageBox::information(0, tr("Layer not editable"), 
          tr("The current layer is not editable. Choose 'Allow editing' in the legend item right click menu."));
      return false;
    }


    if (makeSelected)
    {
      mSelected.clear();
    }

    for (std::vector<QgsFeature*>::iterator iter  = features->begin();
        iter != features->end();
        ++iter)
    {
      // TODO: Tidy these next two lines up

      //      QgsFeature f = (*iter);
      //      addFeature(&f, FALSE);

      addFeature(*iter);

      if (makeSelected)
      {
        mSelected.insert((*iter)->featureId());
      }
    }

    updateExtents();
  }  
  return true;
}

QString QgsVectorLayer::layerTypeIconPath()
{
  QString myThemePath = QgsApplication::themePath();
  switch(vectorType())
    {
    case Point:
      return (myThemePath+"/mIconPointLayer.png");
      break;
    case Line:
      return (myThemePath+"/mIconLineLayer.png");
      break;
    case Polygon:
      return (myThemePath+"/mIconPolygonLayer.png");
    default:
      return (myThemePath+"/mIconLayer.png");
    }
}

void QgsVectorLayer::refreshLegend()
{
  if(mLegend && m_renderer)
    {
      std::list< std::pair<QString, QIcon*> > itemList;
      m_renderer->refreshLegend(&itemList);
      if(m_renderer->needsAttributes()) //create an item for each classification field (only one for most renderers)
	{
	  std::list<int> classfieldlist = m_renderer->classificationAttributes();
	  for(std::list<int>::iterator it = classfieldlist.begin(); it!=classfieldlist.end(); ++it)
	    {
	      const QgsField theField = (dataProvider->fields())[*it];
	      QString classfieldname = theField.name();
	      itemList.push_front(std::make_pair(classfieldname, (QIcon*)0));
	    }
	}
      mLegend->changeSymbologySettings(getLayerID(), &itemList);
    }

#if 0
  if(mLegendLayer && m_renderer)
  {
    m_renderer->refreshLegend(mLegendLayer);
  }

  //create an item for each classification field (currently only one for all renderers)
  if(m_renderer)
  {
    if(m_renderer->needsAttributes())
    {
      std::list<int> classfieldlist = m_renderer->classificationAttributes();
      for(std::list<int>::reverse_iterator it = classfieldlist.rbegin(); it!=classfieldlist.rend(); ++it)
      {
        const QgsField theField = (dataProvider->fields())[*it];
        QString classfieldname = theField.name();
        QgsLegendSymbologyItem* item = new QgsLegendSymbologyItem();
        item->setText(0, classfieldname);
        static_cast<QTreeWidgetItem*>(mLegendLayer)->insertChild(0, item);
      }
    }
  }
  if(mLegendLayer)
  {
    //copy the symbology changes for the other layers in the same symbology group
    mLegendLayer->updateLayerSymbologySettings(this);
  }
#endif
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
  delete m_renderer;

  QgsRenderer* r = vl->m_renderer;
  if(r)
  {
    m_renderer = r->clone();
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

    const std::vector<QgsField> fieldsThis = dataProvider->fields();
    const std::vector<QgsField> fieldsOther = otherVectorLayer ->dataProvider->fields();

    if(fieldsThis.size() != fieldsOther.size())
    {
      return false;
    }

    //fill two sets with the numerical types for both layers
    const std::list<QString> numAttThis = dataProvider->numericalTypes();
    std::set<QString> numericalThis; //the set of numerical types for this vector layer
    for(std::list<QString>::const_iterator it = numAttThis.begin(); it != numAttThis.end(); ++it)
    {
      numericalThis.insert(*it);
    }

    const std::list<QString> numAttOther = otherVectorLayer ->dataProvider->numericalTypes();
    std::set<QString> numericalOther; //the set of numerical types for the other vector layer
    for(std::list<QString>::const_iterator it = numAttOther.begin(); it != numAttOther.end(); ++it)
    {
      numericalOther.insert(*it);
    }

    std::set<QString>::const_iterator thisiter;
    std::set<QString>::const_iterator otheriter;
    int fieldsThisSize = fieldsThis.size();

    for(register int i = 0; i < fieldsThisSize; ++i)
    {
      if(fieldsThis[i].name() != fieldsOther[i].name())//field names need to be the same
      {
        return false;
      }
      thisiter = numericalThis.find(fieldsThis[i].name());
      otheriter = numericalOther.find(fieldsOther[i].name());
      if(thisiter == numericalThis.end())
      {
        if(otheriter != numericalOther.end())
        {
          return false;
        }
      }
      else
      {
        if(otheriter == numericalOther.end())
        {
          return false;
        }
      }
    }
    return true; //layers are symbology compatible if the code reaches this point
  }
  return false;
}

bool QgsVectorLayer::snapPoint(QgsPoint& point, double tolerance)
{
  if(tolerance<=0||!dataProvider)
  {
    return false;
  }
  double mindist=tolerance*tolerance;//current minimum distance
  double mindistx=point.x();
  double mindisty=point.y();
  QgsFeature* fet;
  QgsPoint vertexFeature;//the closest vertex of a feature
  double minvertexdist;//the distance between 'point' and 'vertexFeature'

  QgsRect selectrect(point.x()-tolerance,point.y()-tolerance,point.x()+tolerance,point.y()+tolerance);
  selectrect = inverseProjectRect(selectrect);
  dataProvider->reset();
  dataProvider->select(&selectrect);

  //go to through the features reported by the spatial filter of the provider
  while ((fet = dataProvider->getNextFeature(false)))
  {
    if(mChangedGeometries.find(fet->featureId()) != mChangedGeometries.end())//if geometry has been changed, use the new geometry
    {
      vertexFeature = mChangedGeometries[fet->featureId()].closestVertex(point);
    }
    else
    {
      vertexFeature=fet->geometry()->closestVertex(point);
    }
    minvertexdist=vertexFeature.sqrDist(point.x(),point.y());
    if(minvertexdist<mindist)
    {
      mindistx=vertexFeature.x();
      mindisty=vertexFeature.y();
      mindist=minvertexdist;
    }
  }

  //also go through the not commited features
  for(std::vector<QgsFeature*>::iterator iter=mAddedFeatures.begin();iter!=mAddedFeatures.end();++iter)
  {
    if(mChangedGeometries.find((*iter)->featureId()) != mChangedGeometries.end())//use the changed geometry
    {
      vertexFeature = mChangedGeometries[(*iter)->featureId()].closestVertex(point);
    }
    else
    {
      vertexFeature=(*iter)->geometry()->closestVertex(point);
    }
    minvertexdist=vertexFeature.sqrDist(point.x(),point.y());
    if(minvertexdist<mindist)
    {
      mindistx=vertexFeature.x();
      mindisty=vertexFeature.y();
      mindist=minvertexdist;
    }
  }

  //and also go through the changed geometries, because the spatial filter of the provider did not consider feature changes
  for(std::map<int, QgsGeometry>::const_iterator iter = mChangedGeometries.begin(); iter != mChangedGeometries.end(); ++iter)
  {
    vertexFeature = iter->second.closestVertex(point);
    minvertexdist=vertexFeature.sqrDist(point.x(),point.y());
    if(minvertexdist<mindist)
    {
      mindistx=vertexFeature.x();
      mindisty=vertexFeature.y();
      mindist=minvertexdist;
    }
  }

  point.setX(mindistx);
  point.setY(mindisty);

  return true;
}


bool QgsVectorLayer::snapVertexWithContext(QgsPoint& point,
    QgsGeometryVertexIndex& atVertex,
    int& snappedFeatureId,
    QgsGeometry& snappedGeometry,
    double tolerance)
{
  bool vertexFound = false; //flag to check if a meaningful result can be returned
  QgsGeometryVertexIndex atVertexTemp;

  QgsPoint origPoint = point;

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapVertexWithContext: Entering."
    << "." << std::endl;
#endif

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapVertexWithContext: Tolerance: " << tolerance << ", dataProvider = '" << dataProvider
    << "'." << std::endl;
#endif

  // Sanity checking
  if ( tolerance<=0 ||
      !dataProvider)
  {
    // set some default values before we bail
    atVertex = QgsGeometryVertexIndex();
    snappedFeatureId = std::numeric_limits<int>::min();
    snappedGeometry = QgsGeometry();
    return FALSE;
  }

  QgsFeature* feature;
  QgsPoint minDistSegPoint;  // the closest point on the segment
  double testSqrDist;        // the squared distance between 'point' and 'snappedFeature'

  double minSqrDist  = tolerance*tolerance; //current minimum distance

  QgsRect selectrect(point.x()-tolerance, point.y()-tolerance,
      point.x()+tolerance, point.y()+tolerance);

  dataProvider->reset();
  dataProvider->select(&selectrect);

  origPoint = point;

  // Go through the committed features
  while ((feature = dataProvider->getNextFeature(false)))
  {
    if (mChangedGeometries.find(feature->featureId()) != mChangedGeometries.end())
    {
#ifdef QGISDEBUG
      std::cerr << "QgsVectorLayer::snapSegmentWithContext: Found modified geometry for " << feature->featureId() << std::endl;
#endif
      // substitute the modified geometry for the committed version
      feature->setGeometry( mChangedGeometries[ feature->featureId() ] );
    }

    minDistSegPoint = feature->geometry()->closestVertexWithContext(origPoint,
        atVertexTemp,
        testSqrDist);

    if (testSqrDist < minSqrDist)
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;

      atVertex          = atVertexTemp;
      snappedFeatureId  = feature->featureId();
      snappedGeometry   = *(feature->geometry());
      vertexFound = true;

#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::snapVertexWithContext: minSqrDist reduced to: " << minSqrDist
        //                << " and beforeVertex " << beforeVertex
        << "." << std::endl;
#endif

    }
  }


#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapVertexWithContext:  Checking new features."
    << "." << std::endl;
#endif

  // Also go through the new features
  for (std::vector<QgsFeature*>::iterator iter  = mAddedFeatures.begin();
      iter != mAddedFeatures.end();
      ++iter)
  {
    minDistSegPoint = (*iter)->geometry()->closestVertexWithContext(origPoint,
        atVertexTemp,
        testSqrDist);

    if (testSqrDist < minSqrDist)
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;

      atVertex      = atVertexTemp;
      snappedFeatureId  =   (*iter)->featureId();
      snappedGeometry   = *((*iter)->geometry());
      vertexFound = true;
    }
  }

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapVertexWithContext: Finishing"
    << " with feature " << snappedFeatureId
    //                << " and beforeVertex " << beforeVertex
    << "." << std::endl;
#endif

  if(!vertexFound)
  {
    // set some default values before we bail
    atVertex = QgsGeometryVertexIndex();
    snappedFeatureId = std::numeric_limits<int>::min();
    snappedGeometry = QgsGeometry();
    return FALSE;
  }

  return TRUE;
}


bool QgsVectorLayer::snapSegmentWithContext(QgsPoint& point,
    QgsGeometryVertexIndex& beforeVertex,
    int& snappedFeatureId,
    QgsGeometry& snappedGeometry,
    double tolerance)
{
  bool segmentFound = false; //flag to check if a reasonable result can be returned
  QgsGeometryVertexIndex beforeVertexTemp;

  QgsPoint origPoint = point;

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapSegmentWithContext: Entering."
    << "." << std::endl;
#endif

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapSegmentWithContext: Tolerance: " << tolerance << ", dataProvider = '" << dataProvider
    << "'." << std::endl;
#endif

  // Sanity checking
  if ( tolerance<=0 ||
      !dataProvider)
  {
    // set some default values before we bail
    beforeVertex = QgsGeometryVertexIndex();
    snappedFeatureId = std::numeric_limits<int>::min();
    snappedGeometry = QgsGeometry();
    return FALSE;
  }

  QgsFeature* feature;
  QgsPoint minDistSegPoint;  // the closest point on the segment
  double testSqrDist;        // the squared distance between 'point' and 'snappedFeature'

  double minSqrDist  = tolerance*tolerance; //current minimum distance

  QgsRect selectrect(point.x()-tolerance, point.y()-tolerance,
      point.x()+tolerance, point.y()+tolerance);

  dataProvider->reset();
  dataProvider->select(&selectrect);

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapSegmentWithContext:  Checking committed features."
    << "." << std::endl;
#endif

  // Go through the committed features
  while ((feature = dataProvider->getNextFeature(false)))
  {

    if (mChangedGeometries.find(feature->featureId()) != mChangedGeometries.end())
    {
#ifdef QGISDEBUG
      std::cerr << "QgsVectorLayer::snapSegmentWithContext: Found modified geometry for " << feature->featureId() << std::endl;
#endif
      // substitute the modified geometry for the committed version
      feature->setGeometry( mChangedGeometries[ feature->featureId() ] );
    }

    minDistSegPoint = feature->geometry()->closestSegmentWithContext(origPoint, 
        beforeVertexTemp,
        testSqrDist);

#ifdef QGISDEBUG
    qWarning(beforeVertexTemp.toString());
#endif

    if (testSqrDist < minSqrDist)
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;

      beforeVertex      = beforeVertexTemp;
      snappedFeatureId  = feature->featureId();
      snappedGeometry   = *(feature->geometry());
      segmentFound = true;

#ifdef QGISDEBUG
      std::cout << "QgsVectorLayer::snapSegmentWithContext: minSqrDist reduced to: " << minSqrDist
        //                << " and beforeVertex " << beforeVertex
        << "." << std::endl;
#endif

    }
  }


#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapSegmentWithContext:  Checking new features."
    << "." << std::endl;
#endif

  // Also go through the new features
  for (std::vector<QgsFeature*>::iterator iter  = mAddedFeatures.begin();
      iter != mAddedFeatures.end();
      ++iter)
  {
    minDistSegPoint = (*iter)->geometry()->closestSegmentWithContext(origPoint, 
        beforeVertexTemp,
        testSqrDist);

    if (testSqrDist < minSqrDist)
    {
      point = minDistSegPoint;
      minSqrDist = testSqrDist;

      beforeVertex      = beforeVertexTemp;
      snappedFeatureId  =   (*iter)->featureId();
      snappedGeometry   = *((*iter)->geometry());
      segmentFound = true;
    }
  }

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::snapSegmentWithContext: Finishing"
    << " with feature " << snappedFeatureId
    //                << " and beforeVertex " << beforeVertex
    << "." << std::endl;
#endif

  if(!segmentFound)
  {
    // set some default values before we bail
    beforeVertex = QgsGeometryVertexIndex();
    snappedFeatureId = std::numeric_limits<int>::min();
    snappedGeometry = QgsGeometry();
    return FALSE;
  }

  return TRUE;

}


void QgsVectorLayer::drawFeature(QPainter* p, QgsFeature* fet, QgsMapToPixel * theMapToPixelTransform, 
    QPixmap * marker, double markerScaleFactor, bool projectionsEnabledFlag)
{
  // Only have variables, etc outside the switch() statement that are
  // used in all cases of the statement (otherwise they may get
  // executed, but never used, in a bit of code where performance is
  // critical).

#if defined(Q_WS_X11)
  bool needToTrim = false;
#endif

  unsigned char* feature = fet->getGeometry();

  unsigned int wkbType;
  memcpy(&wkbType, (feature+1), sizeof(wkbType));

#ifdef QGISDEBUG
  //std::cout <<"Entering drawFeature()" << std::endl;
#endif

  switch (wkbType)
  {
    case WKBPoint:
      {
        double x = *((double *) (feature + 5));
        double y = *((double *) (feature + 5 + sizeof(double)));

#ifdef QGISDEBUG 
        //  std::cout <<"...WKBPoint (" << x << ", " << y << ")" <<std::endl;
#endif

        transformPoint(x, y, theMapToPixelTransform, projectionsEnabledFlag);

        p->save();
        p->scale(markerScaleFactor,markerScaleFactor);
        p->drawPixmap(static_cast<int>(x-(marker->width()/2)) ,
            static_cast<int>(y-(marker->height()/2) ),
            *marker);
        p->restore();

        break;
      }
    case WKBMultiPoint:
      {
        unsigned char *ptr = feature + 5;
        unsigned int nPoints = *((int*)ptr);
        ptr += 4;

        p->save();
        p->scale(markerScaleFactor, markerScaleFactor);

        for (register int i = 0; i < nPoints; ++i)
        {
          ptr += 5;
          double x = *((double *) ptr);
          ptr += sizeof(double);
          double y = *((double *) ptr);
          ptr += sizeof(double);

#ifdef QGISDEBUG 
          std::cout <<"...WKBMultiPoint (" << x << ", " << y << ")" <<std::endl;
#endif

          transformPoint(x, y, theMapToPixelTransform, projectionsEnabledFlag);

#if defined(Q_WS_X11)
          // Work around a +/- 32768 limitation on coordinates in X11
          if (std::abs(x) > QgsClipper::maxX ||
              std::abs(y) > QgsClipper::maxY)
            needToTrim = true;
          else
#endif
          p->drawPixmap(static_cast<int>(x-(marker->width()/2)) ,
                static_cast<int>(y-(marker->height()/2) ),
                *marker);
        }
        p->restore();

        break;
      }
    case WKBLineString:
      {
        drawLineString(feature, p, theMapToPixelTransform,
            projectionsEnabledFlag);
        break;
      }
    case WKBMultiLineString:
      {
        unsigned char* ptr = feature + 5;
        unsigned int numLineStrings = *((int*)ptr);
        ptr = feature + 9;

        for (register int jdx = 0; jdx < numLineStrings; jdx++)
        {
          ptr = drawLineString(ptr, p, theMapToPixelTransform,
              projectionsEnabledFlag);
        }
        break;
      }
    case WKBPolygon:
      {
        drawPolygon(feature, p, theMapToPixelTransform,
            projectionsEnabledFlag);
        break;
      }
    case WKBMultiPolygon:
      {
        unsigned char *ptr = feature + 5;
        unsigned int numPolygons = *((int*)ptr);
        ptr = feature + 9;
        for (register int kdx = 0; kdx < numPolygons; kdx++)
          ptr = drawPolygon(ptr, p, theMapToPixelTransform, 
              projectionsEnabledFlag);
        break;
      }
    default:
#ifdef QGISDEBUG
      std::cout << "UNKNOWN WKBTYPE ENCOUNTERED\n";
#endif
      break;
  }
}



void QgsVectorLayer::saveAsShapefile()
{
  // call the dataproviders saveAsShapefile method
  dataProvider->saveAsShapefile();
  //  QMessageBox::information(0,"Save As Shapefile", "Someday...");
}
void QgsVectorLayer::setCoordinateSystem()
{
  delete mCoordinateTransform;
  mCoordinateTransform=new QgsCoordinateTransform();
  //slot is defined inthe maplayer superclass
  connect(mCoordinateTransform, SIGNAL(invalidTransformInput()), this, SLOT(invalidTransformInput()));

#ifdef QGISDEBUG
  std::cout << "QgsVectorLayer::setCoordinateSystem ------------------------------------------------start" << std::endl;
  std::cout << "QgsVectorLayer::setCoordinateSystem ----- Computing Coordinate System" << std::endl;
#endif
  //
  // Get the layers project info and set up the QgsCoordinateTransform 
  // for this layer
  //
  int srid = getProjectionSrid();

  if(srid == 0)
  {
    QString mySourceWKT(getProjectionWKT());
    if (mySourceWKT.isNull())
    {
      mySourceWKT=QString("");
    }

#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::setCoordinateSystem --- using wkt\n" << mySourceWKT.toLocal8Bit().data() << std::endl;
#endif
    mCoordinateTransform->sourceSRS().createFromWkt(mySourceWKT);
    //mCoordinateTransform->sourceSRS()->createFromWkt(getProjectionWKT());
  }
  else
  {
#ifdef QGISDEBUG
    std::cout << "QgsVectorLayer::setCoordinateSystem --- using srid " << srid << std::endl;
#endif
    mCoordinateTransform->sourceSRS().createFromSrid(srid);
  }

  //QgsSpatialRefSys provides a mechanism for FORCE a srs to be valid
  //which is inolves falling back to system, project or user selected
  //defaults if the srs is not properly intialised.
  //we only nee to do that if the srs is not alreay valid
  if (!mCoordinateTransform->sourceSRS().isValid())
  {
    mCoordinateTransform->sourceSRS().validate();
  }
  //get the project projections WKT, defaulting to this layer's projection
  //if none exists....
  //First get the SRS for the default projection WGS 84
  //QString defaultWkt = QgsSpatialReferences::instance()->getSrsBySrid("4326")->srText();

  // if no other layers exist, set the output projection to be
  // the same as the input projection, otherwise set the output to the
  // project srs

#ifdef QGISDEBUG
  std::cout << "Layer registry has " << QgsMapLayerRegistry::instance()->count() << " layers " << std::endl;
#endif     
  if (QgsMapLayerRegistry::instance()->count() ==0)
  {
    mCoordinateTransform->destSRS().createFromProj4(
        mCoordinateTransform->sourceSRS().proj4String());
    //first layer added will set the default project level projection
    //TODO remove cast if poss!
    int mySrsId = static_cast<int>(mCoordinateTransform->sourceSRS().srsid());
    if (mySrsId)
    {
      QgsProject::instance()->writeEntry("SpatialRefSys","/ProjectSRSID",mySrsId);
    }
  }
  else 
  {
    mCoordinateTransform->destSRS().createFromSrsId(
        QgsProject::instance()->readNumEntry("SpatialRefSys","/ProjectSRSID",0));
  }
  if (!mCoordinateTransform->destSRS().isValid())
  {
    mCoordinateTransform->destSRS().validate();  
  }


  //initialise the transform - you should do this any time one of the SRS's changes

  mCoordinateTransform->initialise();



}

bool QgsVectorLayer::commitAttributeChanges(const std::set<QString>& deleted,
    const std::map<QString,QString>& added,
    std::map<int,std::map<QString,QString> >& changed)
{
  bool returnvalue=true;

  if(dataProvider)
  {
    if(dataProvider->capabilities()&QgsVectorDataProvider::DeleteAttributes)
    {
      //delete attributes in all not commited features
      for(std::vector<QgsFeature*>::iterator iter=mAddedFeatures.begin();iter!=mAddedFeatures.end();++iter)
      {
        for(std::set<QString>::const_iterator it=deleted.begin();it!=deleted.end();++it)
        {
          (*iter)->deleteAttribute(*it);
        }
      }
      //and then in the provider
      if(!dataProvider->deleteAttributes(deleted))
      {
        returnvalue=false;
      }
    }

    if(dataProvider->capabilities()&QgsVectorDataProvider::AddAttributes)
    {
      //add attributes in all not commited features
      for(std::vector<QgsFeature*>::iterator iter=mAddedFeatures.begin();iter!=mAddedFeatures.end();++iter)
      {
        for(std::map<QString,QString>::const_iterator it=added.begin();it!=added.end();++it)
        {
          (*iter)->addAttribute(it->first);
        }
      }
      //and then in the provider
      if(!dataProvider->addAttributes(added))
      {
        returnvalue=false;
      }
    }

    if(dataProvider->capabilities()&QgsVectorDataProvider::ChangeAttributeValues)
    {
      //change values of the not commited features
      for(std::vector<QgsFeature*>::iterator iter=mAddedFeatures.begin();iter!=mAddedFeatures.end();++iter)
      {
        std::map<int,std::map<QString,QString> >::iterator it=changed.find((*iter)->featureId());
        if(it!=changed.end())
        {
          for(std::map<QString,QString>::const_iterator iterat=it->second.begin();iterat!=it->second.end();++iterat)
          {
            (*iter)->changeAttributeValue(iterat->first,iterat->second);
          }
          changed.erase(it);
        }
      }

      //and then those of the commited ones
      if(!dataProvider->changeAttributeValues(changed))
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
    bool projectionsEnabledFlag)
{
  // transform the point
  if (projectionsEnabledFlag)
  {
    double z = 0;
    mCoordinateTransform->transformInPlace(x, y, z);
  }

  // transform from projected coordinate system to pixel 
  // position on map canvas
  mtp->transformInPlace(x, y);
}

inline void QgsVectorLayer::transformPoints(
    std::vector<double>& x, std::vector<double>& y, std::vector<double>& z,
    QgsMapToPixel* mtp, bool projectionsEnabledFlag)
{
  // transform the point
  if (projectionsEnabledFlag)
    mCoordinateTransform->transformInPlace(x, y, z);

  // transform from projected coordinate system to pixel 
  // position on map canvas
  mtp->transformInPlace(x, y);
}
unsigned int QgsVectorLayer::getTransparency()
{
  return transparencyLevelInt;
}
//should be between 0 and 255
void QgsVectorLayer::setTransparency(int theInt)
{
  transparencyLevelInt=theInt;
} //  QgsRasterLayer::setTransparency(int theInt)
