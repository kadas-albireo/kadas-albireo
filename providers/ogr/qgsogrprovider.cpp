/***************************************************************************
           qgsogrprovider.cpp Data provider for OGR supported formats
                    Formerly known as qgsshapefileprovider.cpp  
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
/* $Id$ */

#include "qgsogrprovider.h"

#ifndef WIN32
#include <netinet/in.h>
#endif
#include <iostream>
#include <cfloat>
#include <cassert>

#include <ogrsf_frmts.h>
#include <ogr_geometry.h>
#include <ogr_spatialref.h>
#include <cpl_error.h>
#include "ogr_api.h"//only for a test

#include <qfile.h>
#include <qmessagebox.h>
#include <qmap.h>

//TODO Following ifndef can be removed once WIN32 GEOS support
//    is fixed
#ifndef NOWIN32GEOSXXX
//XXX GEOS support on windows is broken until we can get VC++ to
//    tolerate geos.h without throwing a bunch of type errors. It
//    appears that the windows version of GEOS may be compiled with 
//    MINGW rather than VC++.
#endif 


#include "../../src/qgssearchtreenode.h"
#include "../../src/qgsdataprovider.h"
#include "../../src/qgsfeature.h"
#include "../../src/qgsfield.h"
#include "../../src/qgis.h"


#ifdef WIN32
#define QGISEXTERN extern "C" __declspec( dllexport )
#else
#define QGISEXTERN extern "C"
#endif

static const QString TEXT_PROVIDER_KEY = "ogr";
static const QString TEXT_PROVIDER_DESCRIPTION = "OGR data provider";



QgsOgrProvider::QgsOgrProvider(QString const & uri)
    : QgsVectorDataProvider(uri), ogrDataSource(0), extent_(0), ogrLayer(0), ogrDriver(0), minmaxcachedirty(true)
{
  OGRRegisterAll();

  // set the selection rectangle pointer to 0
  mSelectionRectangle = 0;
  // make connection to the data source
#ifdef QGISDEBUG
  std::cerr << "Data source uri is " << uri.local8Bit() << std::endl;
#endif
  // try to open for update
  ogrDataSource = OGRSFDriverRegistrar::Open((const char *) uri.local8Bit(), TRUE, &ogrDriver);
  if(ogrDataSource == NULL)
  {
    // try to open read-only
    ogrDataSource = OGRSFDriverRegistrar::Open((const char *) uri.local8Bit(),FALSE, &ogrDriver);

    //TODO Need to set a flag or something to indicate that the layer
    //TODO is in read-only mode, otherwise edit ops will fail
    // TODO: capabilities() should now reflect this; need to test.
  }
  if (ogrDataSource != NULL) {
#ifdef QGISDEBUG
    std::cerr << "Data source is valid" << std::endl;
    std::cerr << "OGR Driver was " << ogrDriver->GetName() << std::endl;
#endif
    valid = true;

    ogrDriverName = ogrDriver->GetName();

    ogrLayer = ogrDataSource->GetLayer(0);

    // get the extent_ (envelope) of the layer
#ifdef QGISDEBUG
    std::cerr << "Starting get extent\n";
#endif
    extent_ = new OGREnvelope();
    ogrLayer->GetExtent(extent_);
#ifdef QGISDEBUG
    std::cerr << "Finished get extent\n";
#endif
    // getting the total number of features in the layer
    numberFeatures = ogrLayer->GetFeatureCount();   
    // check the validity of the layer
#ifdef QGISDEBUG
    std::cerr << "checking validity\n";
#endif

    loadFields();

#ifdef QGISDEBUG
    std::cerr << "Done checking validity\n";
#endif
  } else {
    std::cerr << "Data source is invalid" << std::endl;
    const char *er = CPLGetLastErrorMsg();
#ifdef QGISDEBUG
    std::cerr << er << std::endl;
#endif
    valid = false;
  }

  //resize the cache matrix
  minmaxcache=new double*[fieldCount()];
  for(int i=0;i<fieldCount();i++)
  {
    minmaxcache[i]=new double[2];
  }
  // create the geos objects
  geometryFactory = new geos::GeometryFactory();
  assert(geometryFactory!=0);
  // create the reader
  //    std::cerr << "Creating the wktReader\n";
  wktReader = new geos::WKTReader(geometryFactory);

  mNumericalTypes.push_back("Integer");
  mNumericalTypes.push_back("Real");
  mNonNumericalTypes.push_back("String");
}

QgsOgrProvider::~QgsOgrProvider()
{
  for(int i=0;i<fieldCount();i++)
  {
    delete[] minmaxcache[i];
  }
  delete[] minmaxcache;

  OGRDataSource::DestroyDataSource(ogrDataSource);
  ogrDataSource = 0;
  delete extent_;
  extent_ = 0;
  delete geometryFactory;
  delete wktReader;
}

void QgsOgrProvider::setEncoding(const QString& e)
{
    QgsVectorDataProvider::setEncoding(e);
    loadFields();
}

void QgsOgrProvider::loadFields()
{
    //the attribute fields need to be read again when the encoding changes
    attributeFields.clear();
    OGRFeatureDefn* fdef = ogrLayer->GetLayerDefn();
    if(fdef)
    {
      geomType = fdef->GetGeomType();
      for(int i=0;i<fdef->GetFieldCount();++i)
      {
        OGRFieldDefn *fldDef = fdef->GetFieldDefn(i);
        OGRFieldType type = type = fldDef->GetType();
        bool numeric = (type == OFTInteger || type == OFTReal);
        attributeFields.push_back(QgsField(
              mEncoding->toUnicode(fldDef->GetNameRef()), 
              mEncoding->toUnicode(fldDef->GetFieldTypeName(type)),
              fldDef->GetWidth(),
              fldDef->GetPrecision(),
              numeric));
      }
    }
}

QString QgsOgrProvider::getProjectionWKT()
{ 
#ifdef QGISDEBUG 
    std::cerr << "QgsOgrProvider::getProjectionWKT()" << std::endl; 
#endif 
  OGRSpatialReference * mySpatialRefSys = ogrLayer->GetSpatialRef();
  if (mySpatialRefSys == NULL)
  {
#ifdef QGISDEBUG 
    std::cerr << "QgsOgrProvider::getProjectionWKT() --- no wkt found..returning null" << std::endl; 
#endif 
    return NULL;
  }
  else
  {
    // if appropriate, morph the projection from ESRI form
    QString fileName = ogrDataSource->GetName();
#ifdef QGISDEBUG 
    std::cerr << "Data source file name is : " << fileName.local8Bit() << std::endl; 
#endif 
    if(fileName.contains(".shp"))
    {
#ifdef QGISDEBUG 
      std::cerr << "Morphing " << fileName.local8Bit() << " WKT from ESRI" << std::endl; 
#endif 
      // morph it
      mySpatialRefSys->morphFromESRI();
    }
    // get the proj4 text
    char * ppszProj4;
    mySpatialRefSys->exportToProj4   ( &ppszProj4 );
    std::cout << "vvvvvvvvvvvvvvvvv PROJ4 TEXT vvvvvvvvvvvvvvv" << std::endl; 
    std::cout << ppszProj4 << std::endl; 
    std::cout << "^^^^^^^^^^^^^^^^^ PROJ4 TEXT ^^^^^^^^^^^^^^^" << std::endl; 
    char    *pszWKT = NULL;
    mySpatialRefSys->exportToWkt( &pszWKT );
    QString myWKTString = QString(pszWKT);
    OGRFree(pszWKT);  
    return myWKTString;
  }
}


QString QgsOgrProvider::storageType()
{
  // Delegate to the driver loaded in by OGR
  return ogrDriverName;
}


/**
 * Get the first feature resutling from a select operation
 * @return QgsFeature
 */
QgsFeature * QgsOgrProvider::getFirstFeature(bool fetchAttributes)
{
  QgsFeature *f = 0;

  if(valid)
  {
#ifdef QGISDEBUG
    std::cerr << "getting first feature\n";
#endif
    ogrLayer->ResetReading();

    OGRFeature * feat = ogrLayer->GetNextFeature();

    Q_CHECK_PTR( feat  );

    if(feat)
    {
#ifdef QGISDEBUG
      std::cerr << "First feature is not null\n";
#endif
    }
    else
    {
#ifdef QGISDEBUG
      std::cerr << "First feature is null\n";

#endif
      return 0x0;               // so return a null feature indicating that we got a null feature
    }

    // get the feature type name, if any
    OGRFeatureDefn * featureDefinition = feat->GetDefnRef();
    QString featureTypeName =   
      featureDefinition ? QString(featureDefinition->GetName()) : QString("");

    f = new QgsFeature(feat->GetFID(), featureTypeName );

    Q_CHECK_PTR( f );

    if ( ! f )                  // return null if we can't get a new QgsFeature
    {
      delete feat;

      return 0x0;
    }

    size_t geometry_size = feat->GetGeometryRef()->WkbSize();
    f->setGeometryAndOwnership(getGeometryPointer(feat), geometry_size);

    if(fetchAttributes)
    {
      getFeatureAttributes(feat, f);
    }

    delete feat;

  }

  return f;

} // QgsOgrProvider::getFirstFeature()




bool QgsOgrProvider::getNextFeature(QgsFeature &f, bool fetchAttributes)
{
  bool returnValue;
  if(valid){
    //std::cerr << "getting next feature\n";
    // skip features without geometry
    OGRFeature *fet;
    while ((fet = ogrLayer->GetNextFeature()) != NULL) {
      if (fet->GetGeometryRef())
        break;
    }
    if(fet){
      OGRGeometry *geom = fet->GetGeometryRef();

      // get the wkb representation
      unsigned char *feature = new unsigned char[geom->WkbSize()];
      geom->exportToWkb((OGRwkbByteOrder) endian(), feature);
      f.setFeatureId(fet->GetFID());
      f.setGeometryAndOwnership(feature, geom->WkbSize());

      OGRFeatureDefn * featureDefinition = fet->GetDefnRef();
      QString featureTypeName =   
        featureDefinition ? QString(featureDefinition->GetName()) : QString("");
      f.typeName( featureTypeName );

      if(fetchAttributes){
        getFeatureAttributes(fet, &f);
      }
      /*   char *wkt = new char[2 * geom->WkbSize()];
           geom->exportToWkt(&wkt);
           f->setWellKnownText(wkt);
           delete[] wkt;  */
      delete fet;
      returnValue = true;
    }else{
#ifdef QGISDEBUG
      std::cerr << "Feature is null\n";
      f.setValid(false);
      returnValue = false;
#endif
      // probably should reset reading here
      ogrLayer->ResetReading();
    }


  }else{
#ifdef QGISDEBUG    
    std::cerr << "Read attempt on an invalid shapefile data source\n";
#endif
  }
  return returnValue;
}

/**
 * Get the next feature resutling from a select operation
 * Return 0 if there are no features in the selection set
 * @return QgsFeature
 */
QgsFeature *QgsOgrProvider::getNextFeature(bool fetchAttributes)
{
   if(!valid)
   {
       std::cerr << "Read attempt on an invalid shapefile data source\n";
       return 0;
   }
   
   OGRFeature* fet;
   OGRGeometry* geom;
   QgsFeature *f = 0;
   while((fet = ogrLayer->GetNextFeature()) != NULL)
   {
     
     if (fet->GetGeometryRef())
       {
	   geom = fet->GetGeometryRef();
	   // get the wkb representation
	   unsigned char *feature = new unsigned char[geom->WkbSize()];
	   geom->exportToWkb((OGRwkbByteOrder) endian(), feature);
	   OGRFeatureDefn * featureDefinition = fet->GetDefnRef();
	   QString featureTypeName =   featureDefinition ? QString(featureDefinition->GetName()) : QString("");

	   f = new QgsFeature(fet->GetFID(), featureTypeName);
	   f->setGeometryAndOwnership(feature, geom->WkbSize());
	   if(fetchAttributes  /*|| !mAttributeFilter.isEmpty()*/)
	   {
	       getFeatureAttributes(fet, f);
	   
        // filtering by attribute
        // TODO: improve, speed up!
/*        if (!mAttributeFilter.isEmpty() && !mAttributeFilter.tree()->checkAgainst(f->attributeMap()))
        {
          delete fet;
          continue;
        }    */
	   }
    
	   if(mUseIntersect)
	   {
	       geos::Geometry *geosGeom = 0;
	       geosGeom=f->geosGeometry();
	       assert(geosGeom != 0);
         
	       char *sWkt = new char[2 * mSelectionRectangle->WkbSize()];
	       mSelectionRectangle->exportToWkt(&sWkt);  
	       geos::Geometry *geosRect = wktReader->read(sWkt);
	       assert(geosRect != 0);
	       if(geosGeom->intersects(geosRect))
	       {
#ifdef QGISDEBUG
		   qWarning("intersection found");
#endif
		   delete[] sWkt;
		   delete geosGeom;
		   break;
	       }
	       else
	       {
#ifdef QGISDEBUG
		   qWarning("no intersection found");
#endif
		   delete[] sWkt;
		   delete geosGeom;
		   delete f;
		   f=0;
	       }
	   }
	   else
	   {
	       break;
	   }
       }
	   delete fet;
   }
   return f;
}

/**
 * Get the next feature resutling from a select operation
 * Return 0 if there are no features in the selection set
 * @return QgsFeature
 */
/*QgsFeature *QgsOgrProvider::getNextFeature(bool fetchAttributes)
{

  QgsFeature *f = 0;
  if(valid){
    OGRFeature *fet;
    OGRGeometry *geom;
    while ((fet = ogrLayer->GetNextFeature()) != NULL) {
      if (fet->GetGeometryRef())
      {
        if(mUseIntersect)
        {
          geom  =  fet->GetGeometryRef();
          char *wkt = new char[2 * geom->WkbSize()];
          geom->exportToWkt(&wkt);
          geos::Geometry *geosGeom = wktReader->read(wkt);
          assert(geosGeom != 0);
          std::cerr << "Geometry type of geos object is : " << geosGeom->getGeometryType() << std::endl; 
          // get the selection rectangle and create a geos geometry from it
          char *sWkt = new char[2 * mSelectionRectangle->WkbSize()];
          mSelectionRectangle->exportToWkt(&sWkt);
          std::cerr << "Passing " << sWkt << " to goes\n";    
          geos::Geometry *geosRect = wktReader->read(sWkt);
          assert(geosRect != 0);
          std::cerr << "About to apply intersects function\n";
          // test the geometry
          if(geosGeom->intersects(geosRect))
          {
            std::cerr << "Intersection found\n";
            break;
          }
          //XXX For some reason deleting these on win32 causes segfault
          //XXX Someday I'll figure out why...
          //delete[] wkt;  
          //delete[] sWkt;  
        }
        else
        {
          break;
        }
      }
    }
    if(fet){
      geom = fet->GetGeometryRef();

      // get the wkb representation
      unsigned char *feature = new unsigned char[geom->WkbSize()];
      geom->exportToWkb((OGRwkbByteOrder) endian(), feature);

      OGRFeatureDefn * featureDefinition = fet->GetDefnRef();
      QString featureTypeName =   
        featureDefinition ? QString(featureDefinition->GetName()) : QString("");

      f = new QgsFeature(fet->GetFID(), featureTypeName);
      f->setGeometryAndOwnership(feature, geom->WkbSize());

      if(fetchAttributes){
        getFeatureAttributes(fet, f);
      }
      delete fet;
    }else{
#ifdef QGISDEBUG
      std::cerr << "Feature is null\n";
#endif
      // probably should reset reading here
      ogrLayer->ResetReading();
    }


  }else{
#ifdef QGISDEBUG    
    std::cerr << "Read attempt on an invalid shapefile data source\n";
#endif
  }
  return f;
}*/


QgsFeature *QgsOgrProvider::getNextFeature(std::list<int> const& attlist, int featureQueueSize)
{
  QgsFeature *f = 0; 
  if(valid)
  {
    // skip features without geometry
    OGRFeature *fet;
    while ((fet = ogrLayer->GetNextFeature()) != NULL) {

      if (fet->GetGeometryRef())
      {
        if(mUseIntersect)
        {
          // test this geometry to see if it should be
          // returned 
#ifdef QGISDEBUG2 
          std::cerr << "Testing geometry using intersect" << std::endl; 
#endif 
        }
        else
        {
#ifdef QGISDEBUG2 
          std::cerr << "Testing geometry using mbr" << std::endl; 
#endif 
          break;
        }
      }
    }
    if(fet)
    {
      OGRGeometry *geom = fet->GetGeometryRef();
      // get the wkb representation
      unsigned char *feature = new unsigned char[geom->WkbSize()];
      geom->exportToWkb((OGRwkbByteOrder) endian(), feature);
      OGRFeatureDefn * featureDefinition = fet->GetDefnRef();
      QString featureTypeName =   
        featureDefinition ? QString(featureDefinition->GetName()) : QString("");

      f = new QgsFeature(fet->GetFID(), featureTypeName);
      f->setGeometryAndOwnership(feature, geom->WkbSize());
      for(std::list<int>::const_iterator it=attlist.begin();it!=attlist.end();++it)
      {
        getFeatureAttribute(fet,f,*it);
      }
      delete fet;
      //delete [] feature;
    }
    else
    {
#ifdef QGISDEBUG
      std::cerr << "Feature is null\n";
#endif  
      // probably should reset reading here
      ogrLayer->ResetReading();
    }
  }
  else
  {
#ifdef QGISDEBUG    
    std::cerr << "Read attempt on an invalid shapefile data source\n";
#endif    
  }
  return f;
}

/**
 * Select features based on a bounding rectangle. Features can be retrieved
 * with calls to getFirstFeature and getNextFeature.
 * @param mbr QgsRect containing the extent to use in selecting features
 * @param useIntersect If true, an intersect test will be used in selecting
 * features. In OGR, this is a two pass affair. The mUseIntersect value is
 * stored. If true, a secondary filter (using GEOS) is applied to each
 * feature in the getNextFeature function.
 */
void QgsOgrProvider::select(QgsRect *rect, bool useIntersect)
{
  mUseIntersect = useIntersect;
  // spatial query to select features
  std::cerr << "Selection rectangle is " << *rect << std::endl;
  OGRGeometry *filter = 0;
  filter = new OGRPolygon();
  QString wktExtent = QString("POLYGON ((%1))").arg(rect->asPolygon());
  const char *wktText = (const char *)wktExtent;

  if(useIntersect)
  {
    // store the selection rectangle for use in filtering features during
    // an identify and display attributes
    delete mSelectionRectangle;
    mSelectionRectangle = new OGRPolygon();
    mSelectionRectangle->importFromWkt((char **)&wktText);
  }

  // reset the extent for the ogr filter
  wktExtent = QString("POLYGON ((%1))").arg(rect->asPolygon());
  wktText = (const char *)wktExtent;

  OGRErr result = ((OGRPolygon *) filter)->importFromWkt((char **)&wktText);
  //TODO - detect an error in setting the filter and figure out what to
  //TODO   about it. If setting the filter fails, all records will be returned
  if (result == OGRERR_NONE) 
  {
    std::cerr << "Setting spatial filter using " << wktExtent.local8Bit() << std::endl;
    ogrLayer->SetSpatialFilter(filter);
    //ogrLayer->SetSpatialFilterRect(rect->xMin(), rect->yMin(), rect->xMax(), rect->yMax());
  }else{
#ifdef QGISDEBUG    
    std::cerr << "Setting spatial filter failed!" << std::endl;
    assert(result==OGRERR_NONE);
#endif
  }
  
  delete filter;
  
} // QgsOgrProvider::select



/**
 * Identify features within the search radius specified by rect
 * @param rect Bounding rectangle of search radius
 * @return std::vector containing QgsFeature objects that intersect rect
 */
std::vector<QgsFeature>& QgsOgrProvider::identify(QgsRect * rect)
{
  // select the features
  select(rect);
#ifdef WIN32
  //TODO fix this later for win32
  std::vector<QgsFeature> feat;
  return feat;
#endif
}

unsigned char * QgsOgrProvider::getGeometryPointer(OGRFeature *fet){
  OGRGeometry *geom = fet->GetGeometryRef();
  unsigned char *gPtr=0;
  // get the wkb representation
  gPtr = new unsigned char[geom->WkbSize()];

  geom->exportToWkb((OGRwkbByteOrder) endian(), gPtr);
  return gPtr;

}


QgsRect *QgsOgrProvider::extent()
{
  mExtentRect.set(extent_->MinX, extent_->MinY, extent_->MaxX, extent_->MaxY);
  return &mExtentRect;
}


size_t QgsOgrProvider::layerCount() const
{
    return ogrDataSource->GetLayerCount();
} // QgsOgrProvider::layerCount()


/** 
 * Return the feature type
 */
int QgsOgrProvider::geometryType() const
{
  return geomType;
}

/** 
 * Return the feature type
 */
long QgsOgrProvider::featureCount() const
{
  return numberFeatures;
}

/**
 * Return the number of fields
 */
int QgsOgrProvider::fieldCount() const
{
  return attributeFields.size();
}

void QgsOgrProvider::getFeatureAttribute(OGRFeature * ogrFet, QgsFeature * f, int attindex)
{
  OGRFieldDefn *fldDef = ogrFet->GetFieldDefnRef(attindex);

  if ( ! fldDef )
  {
    qDebug( "%s:%d ogrFet->GetFieldDefnRef(attindex) returns NULL", 
        __FILE__, __LINE__ );
    return;
  }

  QString fld = fldDef->GetNameRef();
  QCString cstr(ogrFet->GetFieldAsString(attindex));
  bool numeric = attributeFields[attindex].isNumeric();

  f->addAttribute(fld, mEncoding->toUnicode(cstr), numeric);
}

/**
 * Fetch attributes for a selected feature
 */
void QgsOgrProvider::getFeatureAttributes(OGRFeature *ogrFet, QgsFeature *f){
  for (int i = 0; i < ogrFet->GetFieldCount(); i++) {
    getFeatureAttribute(ogrFet,f,i);
    // add the feature attributes to the tree
    /*OGRFieldDefn *fldDef = ogrFet->GetFieldDefnRef(i);
      QString fld = fldDef->GetNameRef();
    //    OGRFieldType fldType = fldDef->GetType();
    QString val;

    val = ogrFet->GetFieldAsString(i);
    f->addAttribute(fld, val);*/
  }
}

std::vector<QgsField> const & QgsOgrProvider::fields() const
{
  return attributeFields;
}

void QgsOgrProvider::reset()
{
  // TODO: check whether it supports normal SQL or only that "restricted_sql"
  if (mAttributeFilter.isEmpty())
    ogrLayer->SetAttributeFilter(NULL);
  else
    ogrLayer->SetAttributeFilter(mAttributeFilter.string());

  ogrLayer->SetSpatialFilter(0);
  ogrLayer->ResetReading();
  // Reset the use intersect flag on a provider reset, otherwise only the last
  // selected feature(s) will be displayed when the attribute table
  // is opened. 
  //XXX In a future release, this "feature" can be used to implement
  // the display of only selected records in the attribute table.
  mUseIntersect = false;
}

QString QgsOgrProvider::minValue(int position)
{
  if(position>=fieldCount())
  {
    std::cerr << "Warning: access requested to invalid position in QgsOgrProvider::minValue(..)" << std::endl;
  }
  if(minmaxcachedirty)
  {
    fillMinMaxCash();
  }
  return QString::number(minmaxcache[position][0],'f',2);
}


QString QgsOgrProvider::maxValue(int position)
{
  if(position>=fieldCount())
  {
    std::cerr << "Warning: access requested to invalid position in QgsOgrProvider::maxValue(..)" << std::endl;
  }
  if(minmaxcachedirty)
  {
    fillMinMaxCash();
  }
  return QString::number(minmaxcache[position][1],'f',2);
}

void QgsOgrProvider::fillMinMaxCash()
{
  for(int i=0;i<fieldCount();i++)
  {
    minmaxcache[i][0]=DBL_MAX;
    minmaxcache[i][1]=-DBL_MAX;
  }

  QgsFeature* f=getFirstFeature(true);
  do
  {
    for(int i=0;i<fieldCount();i++)
    {
      double value=(f->attributeMap())[i].fieldValue().toDouble();
      if(value<minmaxcache[i][0])
      {
        minmaxcache[i][0]=value;  
      }  
      if(value>minmaxcache[i][1])
      {
        minmaxcache[i][1]=value;  
      }
    }
    delete f;

  }while(f=getNextFeature(true));

  minmaxcachedirty=false;
}

//TODO - add sanity check for shape file layers, to include cheking to
//       see if the .shp, .dbf, .shx files are all present and the layer
//       actually has features
bool QgsOgrProvider::isValid()
{
  return valid;
}

bool QgsOgrProvider::addFeature(QgsFeature* f)
{ 
#ifdef QGISDEBUG
  qWarning("in add Feature");
#endif
  bool returnValue = true;
  OGRFeatureDefn* fdef=ogrLayer->GetLayerDefn();
  OGRFeature* feature=new OGRFeature(fdef);
  QGis::WKBTYPE ftype;
  memcpy(&ftype, (f->getGeometry()+1), sizeof(int));
  switch(ftype)
  {
    case QGis::WKBPoint:
      {
        OGRPoint* p=new OGRPoint();
        p->importFromWkb(f->getGeometry(),1+sizeof(int)+2*sizeof(double));
        OGRErr err = feature->SetGeometry(p);
        break;
      }
    case QGis::WKBLineString:
      {
        OGRLineString* l=new OGRLineString();
        int length;
        memcpy(&length,f->getGeometry()+1+sizeof(int),sizeof(int));
        l->importFromWkb(f->getGeometry(),1+2*sizeof(int)+2*length*sizeof(double));
        feature->SetGeometry(l);
        break;
      }
    case QGis::WKBPolygon:
      {
        OGRPolygon* pol=new OGRPolygon();
        int numrings;
        int totalnumpoints=0;
        int numpoints;//number of points in one ring
        unsigned char* ptr=f->getGeometry()+1+sizeof(int);
        memcpy(&numrings,ptr,sizeof(int));
        ptr+=sizeof(int);
        for(int i=0;i<numrings;++i)
        {
          memcpy(&numpoints,ptr,sizeof(int));
          ptr+=sizeof(int);
          totalnumpoints+=numpoints;
          ptr+=(2*sizeof(double));
        }
        pol->importFromWkb(f->getGeometry(),1+2*sizeof(int)+numrings*sizeof(int)+totalnumpoints*2*sizeof(double));
        feature->SetGeometry(pol);
        break;
      }
    case QGis::WKBMultiPoint:
      {
        OGRMultiPoint* multip= new OGRMultiPoint();
        int count;
        //determine how many points
        memcpy(&count,f->getGeometry()+1+sizeof(int),sizeof(int));
        multip->importFromWkb(f->getGeometry(),1+2*sizeof(int)+count*2*sizeof(double));
        feature->SetGeometry(multip);
        break;
      }
    case QGis::WKBMultiLineString:
      {
        OGRMultiLineString* multil=new OGRMultiLineString();
        int numlines;
        memcpy(&numlines,f->getGeometry()+1+sizeof(int),sizeof(int));
        int totalpoints=0;
        int numpoints;//number of point in one line
        unsigned char* ptr=f->getGeometry()+9;
        for(int i=0;i<numlines;++i)
        {
          memcpy(&numpoints,ptr,sizeof(int));
          ptr+=4;
          for(int j=0;j<numpoints;++j)
          {
            ptr+=16;
            totalpoints+=2;
          }
        }
        int size=1+2*sizeof(int)+numlines*sizeof(int)+totalpoints*2*sizeof(double);
        multil->importFromWkb(f->getGeometry(),size);
        feature->SetGeometry(multil);
        break;
      }
    case QGis::WKBMultiPolygon:
      {
        OGRMultiPolygon* multipol=new OGRMultiPolygon();
        int numpolys;
        memcpy(&numpolys,f->getGeometry()+1+sizeof(int),sizeof(int));
        int numrings;//number of rings in one polygon
        int totalrings=0;
        int totalpoints=0;
        int numpoints;//number of points in one ring
        unsigned char* ptr=f->getGeometry()+9;

        for(int i=0;i<numpolys;++i)
        {
          memcpy(&numrings,ptr,sizeof(int));
          ptr+=4;
          for(int j=0;j<numrings;++j)
          {
            totalrings++;
            memcpy(&numpoints,ptr,sizeof(int));
            for(int k=0;k<numpoints;++k)
            {
              ptr+=16;
              totalpoints+=2;
            }
          }
        }
        int size=1+2*sizeof(int)+numpolys*sizeof(int)+totalrings*sizeof(int)+totalpoints*2*sizeof(double);
        multipol->importFromWkb(f->getGeometry(),size);
        feature->SetGeometry(multipol);
        break;
      }
  }
  //add possible attribute information
#ifdef QGISDEBUG
  qWarning("before attribute commit section");
#endif
  for(int i=0;i<f->attributeMap().size();++i)
  {
    QString s=(f->attributeMap())[i].fieldValue();
#ifdef QGISDEBUG
    qWarning("adding attribute: "+s);
#endif
    if(!s.isEmpty())
    {
      if(fdef->GetFieldDefn(i)->GetType()==OFTInteger)
      {
        feature->SetField(i,s.toInt());
#ifdef QGISDEBUG
        qWarning("OFTInteger, attribute value: "+s.toInt());
#endif
      }
      else if(fdef->GetFieldDefn(i)->GetType()==OFTReal)
      {
        feature->SetField(i,s.toDouble());
#ifdef QGISDEBUG
        qWarning("OFTReal, attribute value: "+QString::number(s.toDouble(),'f',3));
#endif
      }
      else if(fdef->GetFieldDefn(i)->GetType()==OFTString)
      {
	  feature->SetField(i,s.ascii());
#ifdef QGISDEBUG
        qWarning("OFTString, attribute value: "+QString(s.ascii()));
#endif
      }
      else
      {
#ifdef QGISDEBUG
        qWarning("no type found");
#endif
      }
    }
  }

  if(ogrLayer->CreateFeature(feature)!=OGRERR_NONE)
  {
    //writing failed
    QMessageBox::warning (0, "Warning", "Writing of the feature failed",
        QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton );
    returnValue = false;
  }
  ++numberFeatures;
  delete feature;
  ogrLayer->SyncToDisk();
  return returnValue;
}

bool QgsOgrProvider::addFeatures(std::list<QgsFeature*> const flist)
{
  bool returnvalue=true;
  for(std::list<QgsFeature*>::const_iterator it=flist.begin();it!=flist.end();++it)
  {
    if(!addFeature(*it))
    {
      returnvalue=false;
    }
  }
  return returnvalue;
}

bool QgsOgrProvider::addAttributes(std::map<QString,QString> const & name)
{
    bool returnvalue=true;

    for(std::map<QString,QString>::const_iterator iter=name.begin();iter!=name.end();++iter)
    {
	if(iter->second=="OFTInteger")
	{
	    OGRFieldDefn fielddefn(iter->first,OFTInteger);
	    if(ogrLayer->CreateField(&fielddefn)!=OGRERR_NONE)
	    {
#ifdef QGISDEBUG
		qWarning("QgsOgrProvider.cpp: writing of the field failed");	
#endif
		returnvalue=false;
	    }
	}
	else if(iter->second=="OFTReal")
	{
	    OGRFieldDefn fielddefn(iter->first,OFTReal);
	    if(ogrLayer->CreateField(&fielddefn)!=OGRERR_NONE)
	    {
#ifdef QGISDEBUG
		qWarning("QgsOgrProvider.cpp: writing of the field failed");
#endif
		returnvalue=false;
	    }
	}
	else if(iter->second=="OFTString")
	{
	    OGRFieldDefn fielddefn(iter->first,OFTString);
	    if(ogrLayer->CreateField(&fielddefn)!=OGRERR_NONE)
	    {
#ifdef QGISDEBUG
		qWarning("QgsOgrProvider.cpp: writing of the field failed");
#endif
		returnvalue=false;
	    }
	}
	else
	{
#ifdef QGISDEBUG
	    qWarning("QgsOgrProvider.cpp: type not found");
#endif
	    returnvalue=false;
	}
    }
    return returnvalue;
}

bool QgsOgrProvider::changeAttributeValues(std::map<int,std::map<QString,QString> > const & attr_map)
{
#ifdef QGISDEBUG
  std::cerr << "QgsOgrProvider::changeAttributeValues()" << std::endl;
#endif
    
  std::map<int,std::map<QString,QString> > am = attr_map; // stupid, but I don't know other way to convince gcc to compile
  for( std::map<int,std::map<QString,QString> >::iterator it=am.begin();it!=am.end();++it)
  {
    long fid = (long) (*it).first;

    OGRFeature *of = ogrLayer->GetFeature ( fid );

    if ( !of ) {
        QMessageBox::warning (0, "Warning", "Cannot read feature, cannot change attributes" );
	return false;
    }

    std::map<QString,QString> attr = (*it).second;

    for( std::map<QString,QString>::iterator it2 = attr.begin(); it2!=attr.end(); ++it2 )
    {
	QString name = (*it2).first;
	QString value = (*it2).second;
		
	int fc = of->GetFieldCount();
	for ( int f = 0; f < fc; f++ ) {
	    OGRFieldDefn *fd = of->GetFieldDefnRef ( f );
	    
	    if ( name.compare( fd->GetNameRef() ) == 0 ) {
		OGRFieldType type = fd->GetType();

#ifdef QGISDEBUG
		std::cerr << "set field " << f << " : " << name.local8Bit() << " to " << value.local8Bit() << std::endl;
#endif
		switch ( type ) {
		    case OFTInteger:
		        of->SetField ( f, value.toInt() );
			break;
		    case OFTReal:
		        of->SetField ( f, value.toDouble() );
			break;
		    case OFTString:
		        of->SetField ( f, value.ascii() );
			break;
		    default:
                        QMessageBox::warning (0, "Warning", "Unknown field type, cannot change attribute" );
			break;
		}

		continue;
	    }	
	}
	ogrLayer->SetFeature ( of );
    }
  }

  ogrLayer->SyncToDisk();

  return true;
}

bool QgsOgrProvider::createSpatialIndex()
{
    //experimental, try to create a spatial index
    QString filename=getDataSourceUri().section('/',-1,-1);//todo: find out the filename from the uri
    QString layername=filename.section('.',0,0);
    QString sql="CREATE SPATIAL INDEX ON "+layername;
    ogrDataSource->ExecuteSQL (sql.ascii(), ogrLayer->GetSpatialFilter(),"");
    //todo: find out, if the .qix file is there
    QString indexname = getDataSourceUri();
    indexname.truncate(getDataSourceUri().length()-filename.length());
    indexname=indexname+layername+".qix";
    QFile indexfile(indexname);
    if(indexfile.exists())
    {
	return true;
    }
    else
    {
	return false;
    }
}

int QgsOgrProvider::capabilities() const
{
  int ability = NoCapabilities;

  // collect abilities reported by OGR
  if (ogrLayer)
  {
    // Whilst the OGR documentation (e.g. at
    // http://www.gdal.org/ogr/classOGRLayer.html#a17) states "The capability
    // codes that can be tested are represented as strings, but #defined
    // constants exists to ensure correct spelling", we always use strings
    // here.  This is because older versions of OGR don't always have all
    // the #defines we want to test for here.

    if (ogrLayer->TestCapability("RandomRead"))
    // TRUE if the GetFeature() method works for this layer.
    {
      // TODO: Perhaps influence if QGIS caches into memory (vs read from disk every time) based on this setting.
    }

    if (ogrLayer->TestCapability("SequentialWrite"))
    // TRUE if the CreateFeature() method works for this layer.
    {
      ability |= QgsVectorDataProvider::AddFeatures;
    }

    if (ogrLayer->TestCapability("RandomWrite"))
    // TRUE if the SetFeature() method is operational on this layer.
    {
      ability |= QgsVectorDataProvider::ChangeAttributeValues;

      // TODO According to http://shapelib.maptools.org/ (Shapefile C Library V1.2)
      // TODO "You can't modify the vertices of existing structures".
      // TODO Need to work out versions of shapelib vs versions of GDAL/OGR
      // TODO And test appropriately.

      // This provider can't change geometries yet anyway (cf. Postgres provider)
      // ability |= QgsVectorDataProvider::ChangeGeometries;
    }

    if (ogrLayer->TestCapability("FastSpatialFilter"))
    // TRUE if this layer implements spatial filtering efficiently.
    // Layers that effectively read all features, and test them with the 
    // OGRFeature intersection methods should return FALSE.
    // This can be used as a clue by the application whether it should build
    // and maintain it's own spatial index for features in this layer.
    {
      // TODO: Perhaps use as a clue by QGIS whether it should build and maintain it's own spatial index for features in this layer.
    }

    if (ogrLayer->TestCapability("FastFeatureCount"))
    // TRUE if this layer can return a feature count
    // (via OGRLayer::GetFeatureCount()) efficiently ... ie. without counting
    // the features. In some cases this will return TRUE until a spatial
    // filter is installed after which it will return FALSE.
    {
      // TODO: Perhaps use as a clue by QGIS whether it should spawn a thread to count features.
    }

    if (ogrLayer->TestCapability("FastGetExtent"))
    // TRUE if this layer can return its data extent 
    // (via OGRLayer::GetExtent()) efficiently ... ie. without scanning
    // all the features. In some cases this will return TRUE until a
    // spatial filter is installed after which it will return FALSE.
    {
      // TODO: Perhaps use as a clue by QGIS whether it should spawn a thread to calculate extent.
    }

    if (ogrLayer->TestCapability("FastSetNextByIndex"))
    // TRUE if this layer can perform the SetNextByIndex() call efficiently.
    {
      // No use required for this QGIS release.
    }

#ifdef QGISDEBUG
  std::cout << "QgsOgrProvider::capabilities: GDAL Version Num is 'GDAL_VERSION_NUM'." << std::endl;
#endif

    if (1)
    {
      // Ideally this should test for Shapefile type and GDAL >= 1.2.6
      // In reality, createSpatialIndex() looks after itself.
      ability |= QgsVectorDataProvider::CreateSpatialIndex;
    }

  }

  return ability;

/*
    return (QgsVectorDataProvider::AddFeatures
	    | QgsVectorDataProvider::ChangeAttributeValues
	    | QgsVectorDataProvider::CreateSpatialIndex);
*/
}




QString  QgsOgrProvider::name() const
{
    return TEXT_PROVIDER_KEY;
} // ::name()



QString  QgsOgrProvider::description() const
{
    return TEXT_PROVIDER_DESCRIPTION;
} //  QgsOgrProvider::name()



/**
 * Class factory to return a pointer to a newly created 
 * QgsOgrProvider object
 */
QGISEXTERN QgsOgrProvider * classFactory(const QString *uri)
{
  return new QgsOgrProvider(*uri);
}



/** Required key function (used to map the plugin to a data store type)
*/
QGISEXTERN QString providerKey()
{
  return TEXT_PROVIDER_KEY;
}


/**
 * Required description function 
 */
QGISEXTERN QString description()
{
    return TEXT_PROVIDER_DESCRIPTION;
} 

/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */

QGISEXTERN bool isProvider()
{
  return true;
}

/**Creates an empty data source
@param uri location to store the file(s)
@param format data format (e.g. "ESRI Shapefile"
@param vectortype point/line/polygon or multitypes
@param attributes a list of name/type pairs for the initial attributes
@return true in case of success*/
QGISEXTERN bool createEmptyDataSource(const QString& uri,const QString& format, QGis::WKBTYPE vectortype, \
const std::list<std::pair<QString, QString> >& attributes)
{
    OGRSFDriver* driver;
    OGRRegisterAll();
    driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(format);
    if(driver == NULL)
    {
	return false;
    }

    OGRDataSource* dataSource;
    dataSource = driver->CreateDataSource(uri, NULL);
    if(dataSource == NULL)
    {
	return false;
    }

    //consider spatial reference system
    OGRSpatialReference* reference = NULL;
    QgsSpatialRefSys mySpatialRefSys;
    mySpatialRefSys.validate();
    char* WKT;
    QString myWKT = NULL;
    if(mySpatialRefSys.toOgrSrs().exportToWkt(&WKT)==OGRERR_NONE)
    {
	myWKT=WKT;
    }
    else
    {
#ifdef QGISDEBUG
	qWarning("createEmptyDataSource: export of srs to wkt failed");
#endif
    }
    if( !myWKT.isNull()  &&  myWKT.length() != 0 )
    {
	reference = new OGRSpatialReference(myWKT.local8Bit());
    }

    OGRLayer* layer;	
    layer = dataSource->CreateLayer(uri, reference, (OGRwkbGeometryType)vectortype, NULL);
    if(layer == NULL)
    {
	return false;
    }

    //create the attribute fields
    for(std::list<std::pair<QString, QString> >::const_iterator it= attributes.begin(); it != attributes.end(); ++it)
    {
	if(it->second == "Real")
	{
	    OGRFieldDefn field(it->first, OFTReal);
	    if(layer->CreateField(&field) != OGRERR_NONE)
	    {
#ifdef QGISDEBUG
		qWarning("creation of OFTReal field failed");
#endif
	    }
	}
	else if(it->second == "Integer")
	{
	    OGRFieldDefn field(it->first, OFTInteger);
	    if(layer->CreateField(&field) != OGRERR_NONE)
	    {
#ifdef QGISDEBUG
		qWarning("creation of OFTInteger field failed");
#endif
	    }
	}
	else if(it->second == "String")
	{
	    OGRFieldDefn field(it->first, OFTString);
	    if(layer->CreateField(&field) != OGRERR_NONE)
	    {
#ifdef QGISDEBUG
		qWarning("creation of OFTString field failed");
#endif
	    }
	}
    }

    OGRDataSource::DestroyDataSource(dataSource);
    if(reference)
    {
	reference->Release();
    }
    return true;
}



