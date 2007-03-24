/***************************************************************************
      qgsgpxprovider.cpp  -  Data provider for GPS eXchange files
                             -------------------
    begin                : 2004-04-14
    copyright            : (C) 2004 by Lars Luthman
    email                : larsl@users.sourceforge.net

    Partly based on qgsdelimitedtextprovider.cpp, (C) 2004 Gary E. Sherman
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */

#include <algorithm>
#include <cfloat>
#include <iostream>
#include <limits>
#include <cmath>

// Changed #include <qapp.h> to <qapplication.h>. Apparently some
// debian distros do not include the qapp.h wrapper and the compilation
// fails. [gsherman]
#include <QApplication>
#include <QFile>
#include <QTextStream>
#include <QObject>

#include "qgis.h"
#include "qgsapplication.h"
#include "qgsdataprovider.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsgeometry.h"
#include "qgsspatialrefsys.h"
#include "qgsrect.h"
#include "qgsgpxprovider.h"
#include "gpsdata.h"
#include "qgslogger.h"

#ifdef WIN32
#define QGISEXTERN extern "C" __declspec( dllexport )
#else
#define QGISEXTERN extern "C"
#endif


const char* QgsGPXProvider::attr[] = { "name", "elevation", "symbol", "number",
				       "comment", "description", "source", 
				       "url", "url name" };


const QString GPX_KEY = "gpx";

const QString GPX_DESCRIPTION = QObject::tr("GPS eXchange format provider");


QgsGPXProvider::QgsGPXProvider(QString uri) : 
        QgsVectorDataProvider(uri),
        mEditable(false),
        mMinMaxCacheDirty(true)
{
  // assume that it won't work
  mValid = false;
  
  // we always use UTF-8
  mEncoding = QTextCodec::codecForName("utf8");
  
  // get the filename and the type parameter from the URI
  int fileNameEnd = uri.find('?');
  if (fileNameEnd == -1 || uri.mid(fileNameEnd + 1, 5) != "type=") {
    QgsLogger::warning(tr("Bad URI - you need to specify the feature type."));
    return;
  }
  QString typeStr = uri.mid(fileNameEnd + 6);
  mFeatureType = (typeStr == "waypoint" ? WaypointType :
		  (typeStr == "route" ? RouteType : TrackType));
  
  // set up the attributes and the geometry type depending on the feature type
  attributeFields[NameAttr] = QgsField(attr[NameAttr], QVariant::String, "text");
  if (mFeatureType == WaypointType) {
    attributeFields[EleAttr] = QgsField(attr[EleAttr], QVariant::Double, "double");
    attributeFields[SymAttr] = QgsField(attr[SymAttr], QVariant::String, "text");
  }
  else if (mFeatureType == RouteType || mFeatureType == TrackType) {
    attributeFields[NumAttr] = QgsField(attr[NumAttr], QVariant::Int, "int");
  }
  attributeFields[CmtAttr] = QgsField(attr[CmtAttr], QVariant::String, "text");
  attributeFields[DscAttr] = QgsField(attr[DscAttr], QVariant::String, "text");
  attributeFields[SrcAttr] = QgsField(attr[SrcAttr], QVariant::String, "text");
  attributeFields[URLAttr] = QgsField(attr[URLAttr], QVariant::String, "text");
  attributeFields[URLNameAttr] = QgsField(attr[URLNameAttr], QVariant::String, "text");
  mFileName = uri.left(fileNameEnd);

  // set the selection rectangle to null
  mSelectionRectangle = 0;
  
  // resize the cache matrix
  mMinMaxCache=new double*[attributeFields.size()];
  for(int i=0;i<attributeFields.size();i++) {
    mMinMaxCache[i]=new double[2];
  }

  // parse the file
  data = GPSData::getData(mFileName);
  if (data == 0) {
    return;
  }

  mValid = true;
}


QgsGPXProvider::~QgsGPXProvider()
{
  for(uint i=0;i<fieldCount();i++) {
    delete mMinMaxCache[i];
  }
  delete[] mMinMaxCache;
  GPSData::releaseData(mFileName);
}


QString QgsGPXProvider::storageType() const
{
  return tr("GPS eXchange file");
}

int QgsGPXProvider::capabilities() const
{
  return QgsVectorDataProvider::AddFeatures |
         QgsVectorDataProvider::DeleteFeatures |
         QgsVectorDataProvider::ChangeAttributeValues;
}
  

bool QgsGPXProvider::getNextFeature(QgsFeature& feature,
                                    bool fetchGeometry,
                                    QgsAttributeList attlist,
                                    uint featureQueueSize)
{
  bool result = false;
  
  QgsAttributeList::const_iterator iter;
  
  if (mFeatureType == WaypointType) {
    // go through the list of waypoints and return the first one that is in
    // the bounds rectangle
    for (; mWptIter != data->waypointsEnd(); ++mWptIter) {
      const Waypoint* wpt;
      wpt = &(*mWptIter);
      if (boundsCheck(wpt->lon, wpt->lat)) {
	feature.setFeatureId(wpt->id);
	result = true;
	
	// some wkb voodoo
	char* geo = new char[21];
	std::memset(geo, 0, 21);
	geo[0] = QgsApplication::endian();
	geo[geo[0] == QgsApplication::NDR ? 1 : 4] = QGis::WKBPoint;
	std::memcpy(geo+5, &wpt->lon, sizeof(double));
	std::memcpy(geo+13, &wpt->lat, sizeof(double));
	feature.setGeometryAndOwnership((unsigned char *)geo, sizeof(wkbPoint));
	feature.setValid(true);
	
	// add attributes if they are wanted
	for (iter = attlist.begin(); iter != attlist.end(); ++iter) {
	  switch (*iter) {
	  case 0:
	    feature.addAttribute(0, QVariant(wpt->name));
	    break;
	  case 1:
	    if (wpt->ele == -std::numeric_limits<double>::max())
        feature.addAttribute(1, QVariant());
	    else
        feature.addAttribute(1, QVariant(wpt->ele));
	    break;
	  case 2:
      feature.addAttribute(2, QVariant(wpt->sym));
	    break;
	  case 3:
      feature.addAttribute(3, QVariant(wpt->cmt));
	    break;
	  case 4:
      feature.addAttribute(4, QVariant(wpt->desc));
	    break;
	  case 5:
      feature.addAttribute(5, QVariant(wpt->src));
	    break;
	  case 6:
      feature.addAttribute(6, QVariant(wpt->url));
	    break;
	  case 7:
      feature.addAttribute(7, QVariant(wpt->urlname));
	    break;
	  }
	}
	
	++mWptIter;
	break;
      }
    }
  }
  
  else if (mFeatureType == RouteType) {
    // go through the routes and return the first one that is in the bounds
    // rectangle
    for (; mRteIter != data->routesEnd(); ++mRteIter) {
      const Route* rte;
      rte = &(*mRteIter);
      
      if (rte->points.size() == 0)
	continue;
      const QgsRect& b(*mSelectionRectangle);
      if ((rte->xMax >= b.xMin()) && (rte->xMin <= b.xMax()) &&
	  (rte->yMax >= b.yMin()) && (rte->yMin <= b.yMax())) {
	feature.setFeatureId(rte->id);
	result = true;
	
	// some wkb voodoo
	int nPoints = rte->points.size();
	char* geo = new char[9 + 16 * nPoints];
	std::memset(geo, 0, 9 + 16 * nPoints);
	geo[0] = QgsApplication::endian();
	geo[geo[0] == QgsApplication::NDR ? 1 : 4] = QGis::WKBLineString;
	std::memcpy(geo + 5, &nPoints, 4);
	for (uint i = 0; i < rte->points.size(); ++i) {
	  std::memcpy(geo + 9 + 16 * i, &rte->points[i].lon, sizeof(double));
	  std::memcpy(geo + 9 + 16 * i + 8, &rte->points[i].lat, sizeof(double));
	}
	feature.setGeometryAndOwnership((unsigned char *)geo, 9 + 16 * nPoints);
	feature.setValid(true);
	
	// add attributes if they are wanted
	for (iter = attlist.begin(); iter != attlist.end(); ++iter) {
	  if (*iter == 0)
      feature.addAttribute(0, QVariant(rte->name));
	  else if (*iter == 1) {
	    if (rte->number == std::numeric_limits<int>::max())
        feature.addAttribute(1, QVariant());
	    else
        feature.addAttribute(1, QVariant(rte->number));
	  }
	  else if (*iter == 2)
      feature.addAttribute(2, QVariant(rte->cmt));
	  else if (*iter == 3)
      feature.addAttribute(3, QVariant(rte->desc));
	  else if (*iter == 4)
      feature.addAttribute(4, QVariant(rte->src));
	  else if (*iter == 5)
      feature.addAttribute(5, QVariant(rte->url));
	  else if (*iter == 6)
      feature.addAttribute(6, QVariant(rte->urlname));
	}
	
	++mRteIter;
	break;
      }
    }
  }
  
  else if (mFeatureType == TrackType) {
    // go through the tracks and return the first one that is in the bounds
    // rectangle
    for (; mTrkIter != data->tracksEnd(); ++mTrkIter) {
      const Track* trk;
      trk = &(*mTrkIter);
      
      if (trk->segments.size() == 0)
	continue;
      if (trk->segments[0].points.size() == 0)
	continue;
      const QgsRect& b(*mSelectionRectangle);
      if ((trk->xMax >= b.xMin()) && (trk->xMin <= b.xMax()) &&
	  (trk->yMax >= b.yMin()) && (trk->yMin <= b.yMax())) {
	feature.setFeatureId(trk->id);
	result = true;
	
	// some wkb voodoo
	int nPoints = trk->segments[0].points.size();
	char* geo = new char[9 + 16 * nPoints];
	std::memset(geo, 0, 9 + 16 * nPoints);
	geo[0] = QgsApplication::endian();
	geo[geo[0] == QgsApplication::NDR ? 1 : 4] = QGis::WKBLineString;
	std::memcpy(geo + 5, &nPoints, 4);
	for (int i = 0; i < nPoints; ++i) {
	  std::memcpy(geo + 9 + 16 * i, &trk->segments[0].points[i].lon, sizeof(double));
	  std::memcpy(geo + 9 + 16 * i + 8, &trk->segments[0].points[i].lat, sizeof(double));
	}
	feature.setGeometryAndOwnership((unsigned char *)geo, 9 + 16 * nPoints);
	feature.setValid(true);
	
	// add attributes if they are wanted
	for (iter = attlist.begin(); iter != attlist.end(); ++iter) {
	  if (*iter == 0)
      feature.addAttribute(0, QVariant(trk->name));
	  else if (*iter == 1) {
	    if (trk->number == std::numeric_limits<int>::max())
        feature.addAttribute(1, QVariant());
	    else
        feature.addAttribute(1, QVariant(trk->number));
	  }
	  else if (*iter == 2)
      feature.addAttribute(2, QVariant(trk->cmt));
	  else if (*iter == 3)
      feature.addAttribute(3, QVariant(trk->desc));
	  else if (*iter == 4)
      feature.addAttribute(4, QVariant(trk->src));
	  else if (*iter == 5)
      feature.addAttribute(5, QVariant(trk->url));
	  else if (*iter == 6)
      feature.addAttribute(6, QVariant(trk->urlname));
	}
	
	++mTrkIter;
	break;
      }
    }
  }
  return result;
}


/**
 * Select features based on a bounding rectangle. Features can be retrieved
 * with calls to getFirstFeature and getNextFeature.
 * @param mbr QgsRect containing the extent to use in selecting features
 */
void QgsGPXProvider::select(QgsRect rect, bool useIntersect)
{
  
  // Setting a spatial filter doesn't make much sense since we have to
  // compare each point against the rectangle.
  // We store the rect and use it in getNextFeature to determine if the
  // feature falls in the selection area
  mSelectionRectangle = new QgsRect(rect);
  // Select implies an upcoming feature read so we reset the data source
  reset();
}



// Return the extent of the layer
QgsRect QgsGPXProvider::extent()
{
  return data->getExtent();
}


/** 
 * Return the feature type
 */
QGis::WKBTYPE QgsGPXProvider::geometryType() const
{
  if (mFeatureType == WaypointType)
    return QGis::WKBPoint;
  
  if (mFeatureType == RouteType || mFeatureType == TrackType)
    return QGis::WKBLineString;
  
  return QGis::WKBUnknown;
}


/** 
 * Return the feature type
 */
long QgsGPXProvider::featureCount() const
{
  if (mFeatureType == WaypointType)
    return data->getNumberOfWaypoints();
  if (mFeatureType == RouteType)
    return data->getNumberOfRoutes();
  if (mFeatureType == TrackType)
    return data->getNumberOfTracks();
  return 0;
}


/**
 * Return the number of fields
 */
uint QgsGPXProvider::fieldCount() const
{
  return attributeFields.size();
}


const QgsFieldMap& QgsGPXProvider::fields() const
{
  return attributeFields;
}


void QgsGPXProvider::reset()
{
  if (mFeatureType == WaypointType)
    mWptIter = data->waypointsBegin();
  else if (mFeatureType == RouteType)
    mRteIter = data->routesBegin();
  else if (mFeatureType == TrackType)
    mTrkIter = data->tracksBegin();
}


QString QgsGPXProvider::minValue(uint position)
{
  if (position >= fieldCount()) {
    QgsLogger::warning(tr("Warning: access requested to invalid position "
                       "in QgsGPXProvider::minValue(..)"));
  }
  if (mMinMaxCacheDirty) {
    fillMinMaxCash();
  }
  return QString::number(mMinMaxCache[position][0],'f',2);
}


QString QgsGPXProvider::maxValue(uint position)
{
  if (position >= fieldCount()) {
    QgsLogger::warning(tr("Warning: access requested to invalid position "
                       "in QgsGPXProvider::maxValue(..)"));
  }
  if (mMinMaxCacheDirty) {
    fillMinMaxCash();
  }
  return QString::number(mMinMaxCache[position][1],'f',2);
}


void QgsGPXProvider::fillMinMaxCash()
{
  for(uint i=0;i<fieldCount();i++) {
    mMinMaxCache[i][0]=DBL_MAX;
    mMinMaxCache[i][1]=-DBL_MAX;
  }

  QgsFeature f;
  reset();

  getNextFeature(f, true);
  do {
    for(uint i=0;i<fieldCount();i++) {
      double value=(f.attributeMap())[i].toDouble();
      if(value<mMinMaxCache[i][0]) {
        mMinMaxCache[i][0]=value;  
      }  
      if(value>mMinMaxCache[i][1]) {
        mMinMaxCache[i][1]=value;  
      }
    }
  } while(getNextFeature(f, true));

  mMinMaxCacheDirty=false;
}



bool QgsGPXProvider::isValid()
{
  return mValid;
}


bool QgsGPXProvider::addFeatures(QgsFeatureList & flist)
{
  
  // add all the features
  for (QgsFeatureList::iterator iter = flist.begin(); 
       iter != flist.end(); ++iter) {
    if (!addFeature(*iter))
      return false;
  }
  
  // write back to file
  QFile file(mFileName);
  if (!file.open(QIODevice::WriteOnly))
    return false;
  QTextStream ostr(&file);
  data->writeXML(ostr);
  return true;
}


bool QgsGPXProvider::addFeature(QgsFeature& f)
{
  unsigned char* geo = f.geometry()->wkbBuffer();
  QGis::WKBTYPE wkbType = f.geometry()->wkbType();
  bool success = false;
  GPSObject* obj = NULL;
  const QgsAttributeMap& attrs(f.attributeMap());
  QgsAttributeMap::const_iterator it;
  
  // is it a waypoint?
  if (mFeatureType == WaypointType && geo != NULL && wkbType == QGis::WKBPoint) {
    
    // add geometry
    Waypoint wpt;
    std::memcpy(&wpt.lon, geo+5, sizeof(double));
    std::memcpy(&wpt.lat, geo+13, sizeof(double));
    
    // add waypoint-specific attributes
    for (it = attrs.begin(); it != attrs.end(); ++it) {
      if (attributeFields[it.key()].name() == attr[EleAttr]) {
	bool eleIsOK;
	double ele = it->toDouble(&eleIsOK);
	if (eleIsOK)
	  wpt.ele = ele;
      }
      else if (attributeFields[it.key()].name() == attr[SymAttr]) {
	wpt.sym = it->toString();
      }
    }
    
    GPSData::WaypointIterator iter = data->addWaypoint(wpt);
    success = true;
    obj = &(*iter);
  }
  
  // is it a route?
  if (mFeatureType == RouteType && geo != NULL && wkbType == QGis::WKBLineString) {

    Route rte;
    
    // reset bounds
    rte.xMin = std::numeric_limits<double>::max();
    rte.xMax = -std::numeric_limits<double>::max();
    rte.yMin = std::numeric_limits<double>::max();
    rte.yMax = -std::numeric_limits<double>::max();

    // add geometry
    int nPoints;
    std::memcpy(&nPoints, geo + 5, 4);
    for (int i = 0; i < nPoints; ++i) {
      double lat, lon;
      std::memcpy(&lon, geo + 9 + 16 * i, sizeof(double));
      std::memcpy(&lat, geo + 9 + 16 * i + 8, sizeof(double));
      Routepoint rtept;
      rtept.lat = lat;
      rtept.lon = lon;
      rte.points.push_back(rtept);
      rte.xMin = rte.xMin < lon ? rte.xMin : lon;
      rte.xMax = rte.xMax > lon ? rte.xMax : lon;
      rte.yMin = rte.yMin < lat ? rte.yMin : lat;
      rte.yMax = rte.yMax > lat ? rte.yMax : lat;
    }
    
    // add route-specific attributes
    for (it = attrs.begin(); it != attrs.end(); ++it) {
      if (attributeFields[it.key()].name() == attr[NumAttr]) {
	bool numIsOK;
	long num = it->toInt(&numIsOK);
	if (numIsOK)
	  rte.number = num;
      }
    }
    
    GPSData::RouteIterator iter = data->addRoute(rte);
    success = true;
    obj = &(*iter);
  }
  
  // is it a track?
  if (mFeatureType == TrackType && geo != NULL && wkbType == QGis::WKBLineString) {

    Track trk;
    TrackSegment trkseg;
    
    // reset bounds
    trk.xMin = std::numeric_limits<double>::max();
    trk.xMax = -std::numeric_limits<double>::max();
    trk.yMin = std::numeric_limits<double>::max();
    trk.yMax = -std::numeric_limits<double>::max();

    // add geometry
    int nPoints;
    std::memcpy(&nPoints, geo + 5, 4);
    for (int i = 0; i < nPoints; ++i) {
      double lat, lon;
      std::memcpy(&lon, geo + 9 + 16 * i, sizeof(double));
      std::memcpy(&lat, geo + 9 + 16 * i + 8, sizeof(double));
      Trackpoint trkpt;
      trkpt.lat = lat;
      trkpt.lon = lon;
      trkseg.points.push_back(trkpt);
      trk.xMin = trk.xMin < lon ? trk.xMin : lon;
      trk.xMax = trk.xMax > lon ? trk.xMax : lon;
      trk.yMin = trk.yMin < lat ? trk.yMin : lat;
      trk.yMax = trk.yMax > lat ? trk.yMax : lat;
    }
    
    // add track-specific attributes
    for (it = attrs.begin(); it != attrs.end(); ++it) {
      if (attributeFields[it.key()].name() == attr[NumAttr]) {
	bool numIsOK;
	long num = it->toInt(&numIsOK);
	if (numIsOK)
	  trk.number = num;
      }
    }
    
    trk.segments.push_back(trkseg);
    GPSData::TrackIterator iter = data->addTrack(trk);
    success = true;
    obj = &(*iter);
  }
  
  
  // add common attributes
  if (obj) {
    for (it = attrs.begin(); it != attrs.end(); ++it) {
      QString fieldName = attributeFields[it.key()].name();
      if (fieldName == attr[NameAttr]) {
	      obj->name = it->toString();
      }
      else if (fieldName == attr[CmtAttr]) {
        obj->cmt = it->toString();
      }
      else if (fieldName == attr[DscAttr]) {
        obj->desc = it->toString();
      }
      else if (fieldName == attr[SrcAttr]) {
        obj->src = it->toString();
      }
      else if (fieldName == attr[URLAttr]) {
        obj->url = it->toString();
      }
      else if (fieldName == attr[URLNameAttr]) {
        obj->urlname = it->toString();
      }
    }
  }
    
  return success;
}


bool QgsGPXProvider::deleteFeatures(const QgsFeatureIds & id)
{
  if (mFeatureType == WaypointType)
    data->removeWaypoints(id);
  else if (mFeatureType == RouteType)
    data->removeRoutes(id);
  else if (mFeatureType == TrackType)
    data->removeTracks(id);

  // write back to file
  QFile file(mFileName);
  if (!file.open(QIODevice::WriteOnly))
    return false;
  QTextStream ostr(&file);
  data->writeXML(ostr);
  return true;
}


bool QgsGPXProvider::changeAttributeValues(const QgsChangedAttributesMap & attr_map)
{
  QgsChangedAttributesMap::const_iterator aIter = attr_map.begin();
  if (mFeatureType == WaypointType) {
    GPSData::WaypointIterator wIter = data->waypointsBegin();
    for (; wIter != data->waypointsEnd() && aIter != attr_map.end(); ++wIter) {
      if (wIter->id == aIter.key()) {
	changeAttributeValues(*wIter, aIter.value());
	++aIter;
      }
    }
  }
  else if (mFeatureType == RouteType) {
    GPSData::RouteIterator rIter = data->routesBegin();
    for (; rIter != data->routesEnd() && aIter != attr_map.end(); ++rIter) {
      if (rIter->id == aIter.key()) {
	changeAttributeValues(*rIter, aIter.value());
	++aIter;
      }
    }
  }
  if (mFeatureType == TrackType) {
    GPSData::TrackIterator tIter = data->tracksBegin();
    for (; tIter != data->tracksEnd() && aIter != attr_map.end(); ++tIter) {
      if (tIter->id == aIter.key()) {
	changeAttributeValues(*tIter, aIter.value());
	++aIter;
      }
    }
  }

  // write back to file
  QFile file(mFileName);
  if (!file.open(QIODevice::WriteOnly))
    return false;
  QTextStream ostr(&file);
  data->writeXML(ostr);
  return true;
}


void QgsGPXProvider::changeAttributeValues(GPSObject& obj, const QgsAttributeMap& attrs)
{
  QgsAttributeMap::const_iterator aIter;
  
  // TODO: 
  if (attrs.contains(NameAttr))
    obj.name = attrs[NameAttr].toString();
  if (attrs.contains(CmtAttr))
    obj.cmt = attrs[CmtAttr].toString();
  if (attrs.contains(DscAttr))
    obj.desc = attrs[DscAttr].toString();
  if (attrs.contains(SrcAttr))
    obj.src = attrs[SrcAttr].toString();
  if (attrs.contains(URLAttr))
    obj.url = attrs[URLAttr].toString();
  if (attrs.contains(URLNameAttr))
    obj.urlname = attrs[URLNameAttr].toString();
  
  // waypoint-specific attributes
  Waypoint* wpt = dynamic_cast<Waypoint*>(&obj);
  if (wpt != NULL) {
    if (attrs.contains(SymAttr))
      wpt->sym = attrs[SymAttr].toString();
    if (attrs.contains(EleAttr))
    {
      bool eleIsOK;
      double ele = attrs[EleAttr].toDouble(&eleIsOK);
      if (eleIsOK)
        wpt->ele = ele;
    }
  }
  
  // route- and track-specific attributes
  GPSExtended* ext = dynamic_cast<GPSExtended*>(&obj);
  if (ext != NULL) {
    if (attrs.contains(NumAttr))
    {
      bool eleIsOK;
      double ele = attrs[NumAttr].toDouble(&eleIsOK);
      if (eleIsOK)
        wpt->ele = ele;
    }
  }
}


QString QgsGPXProvider::getDefaultValue(const QString& attr, QgsFeature* f)
{
  if (attr == "source")
    return tr("Digitized in QGIS");
  return "";
}


/** 
 * Check to see if the point is within the selection rectangle
 */
bool QgsGPXProvider::boundsCheck(double x, double y)
{
  bool inBounds = (((x <= mSelectionRectangle->xMax()) &&
        (x >= mSelectionRectangle->xMin())) &&
      ((y <= mSelectionRectangle->yMax()) &&
       (y >= mSelectionRectangle->yMin())));
  QString hit = inBounds?"true":"false";
  return inBounds;
}


QString QgsGPXProvider::name() const
{
    return GPX_KEY;
} // QgsGPXProvider::name()



QString QgsGPXProvider::description() const
{
    return GPX_DESCRIPTION;
} // QgsGPXProvider::description()

void QgsGPXProvider::setSRS(const QgsSpatialRefSys& theSRS)
{
}

QgsSpatialRefSys QgsGPXProvider::getSRS()
{
  return QgsSpatialRefSys(); // use default SRS - it's WGS84
}






/**
 * Class factory to return a pointer to a newly created 
 * QgsGPXProvider object
 */
QGISEXTERN QgsGPXProvider * classFactory(const QString *uri) {
  return new QgsGPXProvider(*uri);
}


/** Required key function (used to map the plugin to a data store type)
*/
QGISEXTERN QString providerKey(){
    return GPX_KEY;
}


/**
 * Required description function 
 */
QGISEXTERN QString description(){
    return GPX_DESCRIPTION;
} 


/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */
QGISEXTERN bool isProvider(){
  return true;
}


