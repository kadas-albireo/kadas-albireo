/***************************************************************************
  qgswmsprovider.cpp  -  QGIS Data provider for 
                         OGC Web Map Service layers
                             -------------------
    begin                : 17 Mar, 2005
    copyright            : (C) 2005 by Brendan Morley
    email                : morb at ozemail dot com dot au
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

#include <fstream>
#include <iostream>

#include "qgsrect.h"
#include "qgshttptransaction.h"

#include "qgswmsprovider.h"

#include <q3network.h>
#include <q3http.h>
#include <q3url.h>
#include <qmessagebox.h>

#include <qglobal.h>
#if QT_VERSION >= 0x040000
#include <Q3Picture>
#endif

#ifdef QGISDEBUG
#include <qfile.h>
#endif

#ifdef WIN32
#define QGISEXTERN extern "C" __declspec( dllexport )
#else
#define QGISEXTERN extern "C"
#endif


static QString WMS_KEY = "wms";
static QString WMS_DESCRIPTION = "OGC Web Map Service version 1.3 data provider";


QgsWmsProvider::QgsWmsProvider(QString const & uri)
  : QgsRasterDataProvider(uri),
    httpuri(uri),
    httpproxyhost(0),
    httpproxyport(80),
    httpcapabilitiesresponse(0),
    cachedImage(0),
    cachedViewExtent(0),
    cachedPixelWidth(0),
    cachedPixelHeight(0)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider: constructing with uri '" << uri.toLocal8Bit().data() << "'." << std::endl;
#endif
  
  // assume this is a valid layer until we determine otherwise
  valid = true;
  
  /* GetCapabilities routine.
     Once proven, we'll move to its own function
  */   
  //httpuri = "http://ims.cr.usgs.gov:80/servlet/com.esri.wms.Esrimap/USGS_EDC_Trans_BTS_Roads?SERVICE=WMS&REQUEST=GetCapabilities";

  //httpuri = "http://www.ga.gov.au/bin/getmap.pl?dataset=national&";
  

  getServerCapabilities();
  

  //httpuri = "http://www.ga.gov.au/bin/getmap.pl?dataset=national&Service=WMS&Version=1.1.0&Request=GetMap&"
  //      "BBox=130,-40,160,-10&SRS=EPSG:4326&Width=400&Height=400&Layers=railways&Format=image/png";

  //httpuri = "http://www.ga.gov.au/bin/getmap.pl?dataset=national&";
        
/*
  // 302-redirects to:        
  uri = "http://www.ga.gov.au/bin/mapserv40?"
        "map=/public/http/www/docs/map/national/national.map&"
        "map_logo_status=off&map_coast_status=off&Service=WMS&Version=1.1.0&"
        "Request=GetMap&BBox=130,-40,160,-10&SRS=EPSG:4326&Width=800&"
        "Height=800&Layers=railways&Format=image/png";
*/
  
//  downloadMapURI(uri);


#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider: exiting constructor." << std::endl;
#endif
  
}

QgsWmsProvider::~QgsWmsProvider()
{

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider: deconstructing." << std::endl;
#endif

  // Dispose of any cached image as created by draw()
  if (cachedImage)
  {
    delete cachedImage;
  }

}


std::vector<QgsWmsLayerProperty> QgsWmsProvider::supportedLayers()
{

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::supportedLayers: Entering." << std::endl;
#endif

  // Allow the provider to collect the capabilities first.
  retrieveServerCapabilities();

  return layersSupported;

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::supportedLayers: Exiting." << std::endl;
#endif

}





size_t QgsWmsProvider::layerCount() const
{
    return 1;                   // XXX properly return actual number of layers
} // QgsWmsProvider::layerCount()




void QgsWmsProvider::addLayers(QStringList const &  layers,
                               QStringList const &  styles)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::addLayers: Entering" <<
                  " with layer list of " << layers.join(", ").toLocal8Bit().data() <<
                   " and style list of " << styles.join(", ").toLocal8Bit().data() <<
               std::endl;
#endif

  // TODO: Make activeSubLayers a std::map in order to avoid duplicates

  activeSubLayers += layers;
  activeSubStyles += styles;

  // Set the visibility of these new layers on by default
  for ( QStringList::const_iterator it  = layers.begin(); 
        it != layers.end(); 
        ++it ) 
  {

    activeSubLayerVisibility[*it] = TRUE;
 
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::addLayers: set visibility of layer '" << (*it).toLocal8Bit().data() << "' to TRUE." << std::endl;
#endif
  }

  // now that the layers have changed, the extent will as well.
  calculateExtent();

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::addLayers: Exiting." << std::endl;
#endif
}
      

void QgsWmsProvider::setLayerOrder(QStringList const &  layers)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::setLayerOrder: Entering." << std::endl;
#endif
  
  activeSubLayers = layers;
  
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::setLayerOrder: Exiting." << std::endl;
#endif
}


void QgsWmsProvider::setSubLayerVisibility(QString const & name, bool vis)
{
  activeSubLayerVisibility[name] = vis;
}


void QgsWmsProvider::setImageEncoding(QString const & mimeType)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::setImageEncoding: Setting image encoding to " << mimeType.toLocal8Bit().data() << "." <<
               std::endl;
#endif
  imageMimeType = mimeType;
}


QImage* QgsWmsProvider::draw(QgsRect  const & viewExtent, int pixelWidth, int pixelHeight)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::draw: Entering." << std::endl;
#endif

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::draw: pixelWidth = " << pixelWidth << std::endl;
  std::cout << "QgsWmsProvider::draw: pixelHeight = " << pixelHeight << std::endl;

  std::cout << "QgsWmsProvider::draw: viewExtent.xMin() = " << viewExtent.xMin() << std::endl;
  std::cout << "QgsWmsProvider::draw: viewExtent.yMin() = " << viewExtent.yMin() << std::endl;
  std::cout << "QgsWmsProvider::draw: viewExtent.xMax() = " << viewExtent.xMax() << std::endl;
  std::cout << "QgsWmsProvider::draw: viewExtent.yMax() = " << viewExtent.yMax() << std::endl;
#endif

  // Can we reuse the previously cached image?
  if (
      (cachedImage) &&
      (cachedViewExtent == viewExtent) &&
      (cachedPixelWidth == pixelWidth) &&
      (cachedPixelHeight == pixelHeight)
     )
  {
    return cachedImage;
  }

  // Bounding box in WMS format
  QString bbox;
  // Warning: does not work with scientific notation
  bbox = QString("%1,%2,%3,%4").
          arg(viewExtent.xMin(),0,'f').
          arg(viewExtent.yMin(),0,'f').
          arg(viewExtent.xMax(),0,'f').
          arg(viewExtent.yMax(),0,'f');
          
  // Width in WMS format
  QString width;
  width = width.setNum(pixelWidth);

  // Height in WMS format
  QString height;
  height = height.setNum(pixelHeight);

  // Split proxy from the provider-encoded uri  
  //   ( url ( + " " + proxyhost + " " + proxyport) )
  
  QStringList drawuriparts = QStringList::split(" ", httpuri, TRUE);
  
  QString drawuri = drawuriparts.front();
  drawuriparts.pop_front();
  
  if (drawuriparts.count())
  {
    httpproxyhost = drawuriparts.front();
    drawuriparts.pop_front();
    
    if (drawuriparts.count())
    {
      bool * ushortConversionOK;
      httpproxyport = drawuriparts.front().toUShort(ushortConversionOK);
      drawuriparts.pop_front();
      
      if (!(*ushortConversionOK))
      {
        httpproxyport = 80;  // standard HTTP port
      }
    }
    else
    {
      httpproxyport = 80;  // standard HTTP port
    }
  }    
  
  // Calculate active layers that are also visible.

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::draw: Active" <<
                       " layer list of " << activeSubLayers.join(", ").toLocal8Bit().data() <<
                   " and style list of " << activeSubStyles.join(", ").toLocal8Bit().data() <<
               std::endl;
#endif


  QStringList visibleLayers = QStringList();
  QStringList visibleStyles = QStringList();

  QStringList::Iterator it2  = activeSubStyles.begin(); 

  for ( QStringList::Iterator it  = activeSubLayers.begin(); 
                              it != activeSubLayers.end(); 
                            ++it ) 
  {
    if (TRUE == activeSubLayerVisibility.find( *it )->second)
    {
      visibleLayers += *it;
      visibleStyles += *it2;
    }

    ++it2;
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::draw: Visible" <<
                       " layer list of " << visibleLayers.join(", ").toLocal8Bit().data() <<
                   " and style list of " << visibleStyles.join(", ").toLocal8Bit().data() <<
               std::endl;
#endif


  QString layers = visibleLayers.join(",");
  Q3Url::encode( layers );

  QString styles = visibleStyles.join(",");
  Q3Url::encode( styles );


  // compose the URL query string for the WMS server.

  drawuri += "?Service=WMS";
  drawuri += "&";
  drawuri += "Version=1.1.0";
  drawuri += "&";
  drawuri += "Request=GetMap";
  drawuri += "&";
  drawuri += "BBox=" + bbox;
  //drawuri += "&";
  //drawuri += "SRS=EPSG:4326";
  // TODO: for now use the first SRS of the first layer
  if ( layersSupported.size() > 0 && 
       layersSupported[0].crs.size() )
  {
    std::set<QString>::iterator it = layersSupported[0].crs.begin();
    drawuri += "&";
    drawuri += "SRS=" + *it;
  }
  drawuri += "&";
  drawuri += "Width=" + width;
  drawuri += "&";
  drawuri += "Height=" + height;
  drawuri += "&";
  drawuri += "Layers=" + layers;
  drawuri += "&";
  drawuri += "Styles=" + styles;
  drawuri += "&";
  drawuri += "Format=" + imageMimeType;
  drawuri += "&";
  drawuri += "Transparent=true";

  emit setStatus( QString("Test from QgsWmsProvider") );

  
  QgsHttpTransaction http(drawuri, httpproxyhost, httpproxyport);
  
  
  // Do a passthrough for the status bar text
  connect(
          &http, SIGNAL(setStatus        (QString)),
           this,   SLOT(showStatusMessage(QString))
         );
  


  // TODO: Check Content-Type is correct
  QByteArray imagesource = http.getSynchronously();
  
  // TODO: Bail gracefully if we get a "application/vnd.ogc.se_xml" error
  
  if (http.responseContentType() == "application/vnd.ogc.se_xml")
  {
    // We had a Service Exception from the WMS
    
    QMessageBox::critical(0,
      "WMS Service Exception",
      imagesource);

    
  }
  
  
#ifdef QGISDEBUG
    std::cout << "QgsWmsProvider::draw: Response received." << std::endl;
#endif

#ifdef QGISDEBUG
  QFile file( "/tmp/qgis-wmsprovider-draw-raw.png" );
  if ( file.open( QIODevice::WriteOnly ) ) 
  {
    file.writeBlock(imagesource);
    file.close();
  }
#endif

  // Load into the final QImage.

  //bool success = i.loadFromData(imagesource);
  if (cachedImage)
  {
    delete cachedImage;
  }
  cachedImage = new QImage(imagesource);

  // Remember settings for useful caching next time.
  cachedViewExtent = viewExtent;
  cachedPixelWidth = pixelWidth;
  cachedPixelHeight = pixelHeight;

#ifdef QGISDEBUG
  // Get what we can support

// Commented out for now, causes segfaults.
//  supportedFormats();

#endif

  
  
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::draw: Exiting." << std::endl;
#endif


  // TODO: bit depth conversion to the client's expectation
//  return *(i.convertDepth(32));
  return cachedImage;

}


void QgsWmsProvider::getServerCapabilities()
{
  
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::getServerCapabilities: entering." << std::endl;
#endif

  retrieveServerCapabilities();

  // TODO: Return generic server capabilities here

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::getServerCapabilities: exiting." << std::endl;
#endif

}


void QgsWmsProvider::retrieveServerCapabilities()
{

  // TODO: Smarter caching - need to refresh if demanded.

  if ( httpcapabilitiesresponse.isNull() )
  {
  
    QString uri = httpuri + "?SERVICE=WMS&REQUEST=GetCapabilities";

    QgsHttpTransaction http(uri, httpproxyhost, httpproxyport);

    httpcapabilitiesresponse = http.getSynchronously();

  
#ifdef QGISDEBUG
    std::cout << "QgsWmsProvider::getServerCapabilities: Converting to DOM." << std::endl;
#endif
  
    parseCapabilities(httpcapabilitiesresponse, capabilities);

  }
  
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::getServerCapabilities: exiting." << std::endl;
#endif

}

// private
void QgsWmsProvider::downloadCapabilitiesURI(QString const & uri)
{

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::downloadCapabilitiesURI: Entered with '" << uri.toLocal8Bit().data() << "'" << std::endl;
#endif

  QgsHttpTransaction http(uri, httpproxyhost, httpproxyport);

  httpcapabilitiesresponse = http.getSynchronously();

  
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::downloadCapabilitiesURI: Converting to DOM." << std::endl;
#endif
  
  parseCapabilities(httpcapabilitiesresponse, capabilities);


  
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::downloadCapabilitiesURI: exiting." << std::endl;
#endif

}


// private
void QgsWmsProvider::drawTest(QString const & uri)
{

/*

Example URL (?):
"http://www.ga.gov.au/bin/getmap.pl?dataset=national&VERSION=1.3.0&REQUEST=GetMap&LAYERS=d02&STYLES=&CRS=EPSG:3112&BBOX=151,-28,153,-26&WIDTH=400&HEIGHT=400&FORMAT=image/png"

Example URL (works!)
"http://www.ga.gov.au/bin/getmap.pl?dataset=national&Service=WMS&Version=1.1.0&Request=GetMap&BBox=130,-40,160,-10&SRS=EPSG:4326&Width=400&Height=400&Layers=railways&Format=image/png"

*/

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::drawTest: Entered with '" << uri.toLocal8Bit().data() << "'" << std::endl;
#endif
  
  QgsHttpTransaction http(uri, httpproxyhost, httpproxyport);

  http.getSynchronously();

  
#ifdef QGISDEBUG
    std::cout << "QgsWmsProvider::drawTest: Response received." << std::endl;
#endif
//   


  
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::downloadMapURI: exiting." << std::endl;
#endif

}


void QgsWmsProvider::parseCapabilities(QByteArray  const & xml, QgsWmsCapabilitiesProperty& capabilitiesProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseCapabilities: entering." << std::endl;

  //test the content of the QByteArray
  QString responsestring(xml);
  qWarning("QgsWmsProvider::parseCapabilities, received the following data: "+responsestring);
  
  QFile file( "/tmp/qgis-wmsprovider-capabilities.xml" );
  if ( file.open( QIODevice::WriteOnly ) ) 
  {
    file.writeBlock(xml);
    file.close();
  }
#endif
  
  // Convert completed document into a DOM
  QString errormsg;
  bool contentsuccess = capabilitiesDOM.setContent(xml, false, &errormsg);

#ifdef QGISDEBUG
  qWarning("errormessage is: "+errormsg);
#endif

  QDomElement docElem = capabilitiesDOM.documentElement();

  // TODO: Assert the docElem.tagName() is "WMS_Capabilities"

  capabilitiesProperty.version = docElem.attribute("version");

  // Start walking through XML.
  QDomNode n = docElem.firstChild();

  while( !n.isNull() ) {
      QDomElement e = n.toElement(); // try to convert the node to an element.
      if( !e.isNull() ) {
          //std::cout << e.tagName() << std::endl; // the node really is an element.

          if      (e.tagName() == "Service")
          {
            std::cout << "  Service." << std::endl;
            parseService(e, capabilitiesProperty.service);
          }
          else if (e.tagName() == "Capability")
          {
            std::cout << "  Capability." << std::endl;
            parseCapability(e, capabilitiesProperty.capability);
          }

      }
      n = n.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseCapabilities: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseService(QDomElement const & e, QgsWmsServiceProperty& serviceProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseService: entering." << std::endl;
#endif
  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          //std::cout << "  " << e1.tagName() << std::endl; // the node really is an element.

          if      (e1.tagName() == "Title")
          {
            serviceProperty.title = e1.text();
          }
          else if (e1.tagName() == "Abstract")
          {
            serviceProperty.abstract = e1.text();
          }
          else if (e1.tagName() == "KeywordList")
          {
            parseKeywordList(e1, serviceProperty.keywordList);
          }
          else if (e1.tagName() == "OnlineResource")
          {
            parseOnlineResource(e1, serviceProperty.onlineResource);
          }
          else if (e1.tagName() == "ContactInformation")
          {
            parseContactInformation(e1, serviceProperty.contactInformation);
          }
          else if (e1.tagName() == "Fees")
          {
            serviceProperty.fees = e1.text();
          }
          else if (e1.tagName() == "AccessConstraints")
          {
            serviceProperty.accessConstraints = e1.text();
          }
          else if (e1.tagName() == "LayerLimit")
          {
            serviceProperty.layerLimit = e1.text().toUInt();
          }
          else if (e1.tagName() == "MaxWidth")
          {
            serviceProperty.maxWidth = e1.text().toUInt();
          }
          else if (e1.tagName() == "MaxHeight")
          {
            serviceProperty.maxHeight = e1.text().toUInt();
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseService: exiting." << std::endl;
#endif

}


void QgsWmsProvider::parseCapability(QDomElement const & e, QgsWmsCapabilityProperty& capabilityProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseCapability: entering." << std::endl;
#endif
  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          //std::cout << "  " << e1.tagName() << std::endl; // the node really is an element.
      
          if (e1.tagName() == "Request")
          {
            parseRequest(e1, capabilityProperty.request);
          }
          else if (e1.tagName() == "Layer")
          {
            parseLayer(e1, capabilityProperty.layer);
          }
      
      }
      n1 = n1.nextSibling();
  }    

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseCapability: exiting." << std::endl;
#endif

}


void QgsWmsProvider::parseContactPersonPrimary(QDomElement const & e, QgsWmsContactPersonPrimaryProperty& contactPersonPrimaryProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseContactPersonPrimary: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "ContactPerson")
          {
            contactPersonPrimaryProperty.contactPerson = e1.text();
          }
          else if (e1.tagName() == "ContactOrganization")
          {
            contactPersonPrimaryProperty.contactOrganization = e1.text();
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseContactPersonPrimary: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseContactAddress(QDomElement const & e, QgsWmsContactAddressProperty& contactAddressProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseContactAddress: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "AddressType")
          {
            contactAddressProperty.addressType = e1.text();
          }
          else if (e1.tagName() == "Address")
          {
            contactAddressProperty.address = e1.text();
          }
          else if (e1.tagName() == "City")
          {
            contactAddressProperty.city = e1.text();
          }
          else if (e1.tagName() == "StateOrProvince")
          {
            contactAddressProperty.stateOrProvince = e1.text();
          }
          else if (e1.tagName() == "PostCode")
          {
            contactAddressProperty.postCode = e1.text();
          }
          else if (e1.tagName() == "Country")
          {
            contactAddressProperty.country = e1.text();
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseContactAddress: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseContactInformation(QDomElement const & e, QgsWmsContactInformationProperty& contactInformationProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseContactInformation: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "ContactPersonPrimary")
          {
            parseContactPersonPrimary(e1, contactInformationProperty.contactPersonPrimary);
          }
          else if (e1.tagName() == "ContactPosition")
          {
            contactInformationProperty.contactPosition = e1.text();
          }
          else if (e1.tagName() == "ContactAddress")
          {
            parseContactAddress(e1, contactInformationProperty.contactAddress);
          }
          else if (e1.tagName() == "ContactVoiceTelephone")
          {
            contactInformationProperty.contactVoiceTelephone = e1.text();
          }
          else if (e1.tagName() == "ContactFacsimileTelephone")
          {
            contactInformationProperty.contactFacsimileTelephone = e1.text();
          }
          else if (e1.tagName() == "ContactElectronicMailAddress")
          {
            contactInformationProperty.contactElectronicMailAddress = e1.text();
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseContactInformation: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseOnlineResource(QDomElement const & e, QgsWmsOnlineResourceAttribute& onlineResourceAttribute)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseOnlineResource: entering." << std::endl;
#endif

  onlineResourceAttribute.xlinkHref = e.attribute("xlink:href");

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseOnlineResource: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseKeywordList(QDomElement  const & e, QStringList& keywordListProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseKeywordList: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "Keyword")
          {
            std::cout << "      Keyword." << std::endl; 
            keywordListProperty += e1.text();
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseKeywordList: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseGet(QDomElement const & e, QgsWmsGetProperty& getProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseGet: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "OnlineResource")
          {
            std::cout << "      OnlineResource." << std::endl;
            parseOnlineResource(e1, getProperty.onlineResource);
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseGet: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parsePost(QDomElement const & e, QgsWmsPostProperty& postProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parsePost: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "OnlineResource")
          {
            std::cout << "      OnlineResource." << std::endl;
            parseOnlineResource(e1, postProperty.onlineResource);
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parsePost: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseHttp(QDomElement const & e, QgsWmsHttpProperty& httpProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseHttp: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "Get")
          {
            std::cout << "      Get." << std::endl;
            parseGet(e1, httpProperty.get);
          }
          else if (e1.tagName() == "Post")
          {
            std::cout << "      Post." << std::endl;
            parsePost(e1, httpProperty.post);
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseHttp: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseDcpType(QDomElement const & e, QgsWmsDcpTypeProperty& dcpType)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseDcpType: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "HTTP")
          {
            std::cout << "      HTTP." << std::endl; 
            parseHttp(e1, dcpType.http);
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseDcpType: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseOperationType(QDomElement const & e, QgsWmsOperationType& operationType)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseOperationType: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "Format")
          {
            std::cout << "      Format." << std::endl; 
            operationType.format += e1.text();
          }
          else if (e1.tagName() == "DCPType")
          {
            std::cout << "      DCPType." << std::endl;
            QgsWmsDcpTypeProperty dcp;
            parseDcpType(e1, dcp);
            operationType.dcpType.push_back(dcp);
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseOperationType: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseRequest(QDomElement const & e, QgsWmsRequestProperty& requestProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseRequest: entering." << std::endl;
#endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "GetMap")
          {
            std::cout << "      GetMap." << std::endl; 
            parseOperationType(e1, requestProperty.getMap);
          }
          else if (e1.tagName() == "GetFeatureInfo")
          {
            std::cout << "      GetFeatureInfo." << std::endl;
            parseOperationType(e1, requestProperty.getFeatureInfo);
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseRequest: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseLegendUrl(QDomElement const & e, QgsWmsLegendUrlProperty& legendUrlProperty)
{
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseLegendUrl: entering." << std::endl;
#endif

  legendUrlProperty.width  = e.attribute("width").toUInt();;
  legendUrlProperty.height = e.attribute("height").toUInt();;

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "Format")
          {
            legendUrlProperty.format = e1.text();
          }
          else if (e1.tagName() == "OnlineResource")
          {
            parseOnlineResource(e1, legendUrlProperty.onlineResource);
          }
      }
      n1 = n1.nextSibling();
  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::parseLegendUrl: exiting." << std::endl;
#endif
}


void QgsWmsProvider::parseStyle(QDomElement const & e, QgsWmsStyleProperty& styleProperty)
{
// #ifdef QGISDEBUG
//  std::cout << "QgsWmsProvider::parseStyle: entering." << std::endl;
// #endif

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          if      (e1.tagName() == "Name")
          {
            styleProperty.name = e1.text();
          }
          else if (e1.tagName() == "Title")
          {
            styleProperty.title = e1.text();
          }
          else if (e1.tagName() == "Abstract")
          {
            styleProperty.abstract = e1.text();
          }
          else if (e1.tagName() == "LegendURL")
          {
            // TODO
          }
          else if (e1.tagName() == "StyleSheetURL")
          {
            // TODO
          }
          else if (e1.tagName() == "StyleURL")
          {
            // TODO
          }
      }
      n1 = n1.nextSibling();
  }

//#ifdef QGISDEBUG
//  std::cout << "QgsWmsProvider::parseStyle: exiting." << std::endl;
//#endif
}


void QgsWmsProvider::parseLayer(QDomElement const & e, QgsWmsLayerProperty& layerProperty)
{
#ifdef QGISDEBUG
//  std::cout << "QgsWmsProvider::parseLayer: entering." << std::endl;
#endif


// TODO: Delete this stanza completely, depending on success of "Inherit things into the sublayer" below.
//  // enforce WMS non-inheritance rules
//  layerProperty.name =        QString::null;
//  layerProperty.title =       QString::null;
//  layerProperty.abstract =    QString::null;
//  layerProperty.keywordList.clear();

  // assume true until we find a child layer
  bool atleaf = TRUE;

  layerProperty.queryable   = e.attribute("queryable").toUInt();
  layerProperty.cascaded    = e.attribute("cascaded").toUInt();
  layerProperty.opaque      = e.attribute("opaque").toUInt();
  layerProperty.noSubsets   = e.attribute("noSubsets").toUInt();
  layerProperty.fixedWidth  = e.attribute("fixedWidth").toUInt();
  layerProperty.fixedHeight = e.attribute("fixedHeight").toUInt();

  QDomNode n1 = e.firstChild();
  while( !n1.isNull() ) {
      QDomElement e1 = n1.toElement(); // try to convert the node to an element.
      if( !e1.isNull() ) {
          //std::cout << "    " << e1.tagName() << std::endl; // the node really is an element.

          if      (e1.tagName() == "Layer")
          {
//            std::cout << "      Nested layer." << std::endl; 

            QgsWmsLayerProperty subLayerProperty;

            // Inherit things into the sublayer
            //   Ref: 7.2.4.8 Inheritance of layer properties
            subLayerProperty.style = layerProperty.style;
            subLayerProperty.crs   = layerProperty.crs;
            // TODO

            parseLayer(e1, subLayerProperty);

            layerProperty.layer.push_back(subLayerProperty);

            atleaf = FALSE;
          }
          else if (e1.tagName() == "Name")
          {
            layerProperty.name = e1.text();
          }
          else if (e1.tagName() == "Title")
          {
            layerProperty.title = e1.text();
          }
          else if (e1.tagName() == "Abstract")
          {
            layerProperty.abstract = e1.text();
          }
          else if (e1.tagName() == "KeywordList")
          {
            parseKeywordList(e1, layerProperty.keywordList);
          }
          else if (
                   (e1.tagName() == "CRS")  ||
                   (e1.tagName() == "SRS")        // legacy from earlier versions of WMS
                  )
          {
            layerProperty.crs.insert(e1.text());
          }
          else if (
                   (e1.tagName() == "EX_GeographicBoundingBox")  ||
                   (e1.tagName() == "LatLonBoundingBox")        // legacy from earlier versions of WMS
                  )
          {
            
//            std::cout << "      LLBB is: '" << e1.attribute("minx") << "'." << std::endl;
//            std::cout << "      LLBB is: '" << e1.attribute("miny") << "'." << std::endl;
//            std::cout << "      LLBB is: '" << e1.attribute("maxx") << "'." << std::endl;
//            std::cout << "      LLBB is: '" << e1.attribute("maxy") << "'." << std::endl;
            
            layerProperty.ex_GeographicBoundingBox = QgsRect( 
                                                e1.attribute("minx").toDouble(),
                                                e1.attribute("miny").toDouble(),
                                                e1.attribute("maxx").toDouble(),
                                                e1.attribute("maxy").toDouble()
                                              );
          }
          else if (e1.tagName() == "BoundingBox")
          {
            // TODO
              QgsWmsBoundingBoxProperty bbox;
              bbox.box = QgsRect ( e1.attribute("minx").toDouble(),
                                   e1.attribute("miny").toDouble(),
                                   e1.attribute("maxx").toDouble(),
                                   e1.attribute("maxy").toDouble()
                                 );
              bbox.crs = e1.attribute("SRS");
              layerProperty.boundingBox.push_back ( bbox );
          }
          else if (e1.tagName() == "Dimension")
          {
            // TODO
          }
          else if (e1.tagName() == "Attribution")
          {
            // TODO
          }
          else if (e1.tagName() == "AuthorityURL")
          {
            // TODO
          }
          else if (e1.tagName() == "Identifier")
          {
            // TODO
          }
          else if (e1.tagName() == "MetadataURL")
          {
            // TODO
          }
          else if (e1.tagName() == "DataURL")
          {
            // TODO
          }
          else if (e1.tagName() == "FeatureListURL")
          {
            // TODO
          }
          else if (e1.tagName() == "Style")
          {
            QgsWmsStyleProperty styleProperty;

            parseStyle(e1, styleProperty);

            layerProperty.style.push_back(styleProperty);
          }
          else if (e1.tagName() == "MinScaleDenominator")
          {
            // TODO
          }
          else if (e1.tagName() == "MaxScaleDenominator")
          {
            // TODO
          }
          // If we got here then it's not in the WMS 1.3 standard

      }
      n1 = n1.nextSibling();
  }

  if (atleaf)
  {
    // We have all the information we need to properly evaluate a layer definition
    // TODO: Save this somewhere

#ifdef QGISDEBUG
//    std::cout << "QgsWmsProvider::parseLayer: A layer definition is complete." << std::endl;

    std::cout << "QgsWmsProvider::parseLayer:   name is: '" << layerProperty.name.toLocal8Bit().data() << "'." << std::endl;
    std::cout << " QgsWmsProvider::parseLayer:  title is: '" << layerProperty.title.toLocal8Bit().data() << "'." << std::endl;
//    std::cout << "QgsWmsProvider::parseLayer:   srs is: '" << layerProperty.srs << "'." << std::endl;
//    std::cout << "QgsWmsProvider::parseLayer:   bbox is: '" << layerProperty.latlonbbox.stringRep() << "'." << std::endl;

    // Store the extent so that it can be combined with others later
    // in calculateExtent()
    // For now use extent in the first SRS defined for the layer
    //extentForLayer[ layerProperty.name ] = layerProperty.ex_GeographicBoundingBox;
    if ( layerProperty.crs.size() > 0 ) 
    {
        // TODO: find the correct bounding box
        if ( layerProperty.boundingBox.size() > 0 ) 
        {
            extentForLayer[ layerProperty.name ] = 
               layerProperty.boundingBox[0].box;
        }
    }

    // Insert into the local class' registry
    layersSupported.push_back(layerProperty);


#endif

  }

#ifdef QGISDEBUG
//  std::cout << "QgsWmsProvider::parseLayer: exiting." << std::endl;
#endif
}


QgsDataSourceURI * QgsWmsProvider::getURI()
{

  QgsDataSourceURI* dsuri;
   
  dsuri = new QgsDataSourceURI;
  
  //TODO
  
  return dsuri;
}


void QgsWmsProvider::setURI(QgsDataSourceURI * uri)
{
  // TODO
} 



QgsRect *QgsWmsProvider::extent()
{
  return &layerExtent;
}

void QgsWmsProvider::reset()
{
  // TODO
} 

bool QgsWmsProvider::isValid()
{
  return valid;
} 


QString QgsWmsProvider::wmsVersion()
{
  // TODO
} 

QStringList QgsWmsProvider::supportedImageEncodings()
{
  return capabilities.capability.request.getMap.format;
} 

  
QStringList QgsWmsProvider::subLayers()
{
  return activeSubLayers;
}


/*
int QgsWmsProvider::capabilities() const
{
    return ( QgsVectorDataProvider::AddFeatures | 
	     QgsVectorDataProvider::DeleteFeatures |
	     QgsVectorDataProvider::ChangeAttributeValues |
	     QgsVectorDataProvider::AddAttributes |
	     QgsVectorDataProvider::DeleteAttributes );
}
*/


void QgsWmsProvider::showStatusMessage(QString const & theMessage)
{

#ifdef QGISDEBUG
//  std::cout << "QgsWmsProvider::showStatusMessage: entered with '" << theMessage << "'." << std::endl;
#endif

    // Pass-through
    // TODO: See if we can connect signal-to-signal.  This is a kludge according to the Qt doc.
    emit setStatus(theMessage);
}


void QgsWmsProvider::calculateExtent()
{

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::calculateExtent: entered." << std::endl;
#endif

  for ( QStringList::Iterator it  = activeSubLayers.begin(); 
                              it != activeSubLayers.end(); 
                            ++it ) 
  {
  
    // This is the extent for the layer name in *it
    QgsRect extent = extentForLayer.find( *it )->second;
    
    if ( it == activeSubLayers.begin() )
    {
      layerExtent = extent;
    }
    else
    {
      layerExtent.combineExtentWith( &extent );
    }
  
#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::calculateExtent: combined extent is '" << 
               layerExtent.stringRep().toLocal8Bit().data() << "' after '" << (*it).toLocal8Bit().data() << "'." << std::endl;
#endif

  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::calculateExtent: exiting with '" << layerExtent.stringRep().toLocal8Bit().data() << "'." << std::endl;
#endif

}

QString QgsWmsProvider::getMetadata()
{

  QString myMetadataQString = "";

  // Server Properties section
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Server Properties:");
  myMetadataQString += "</td></tr>";

  // Use a nested table
  myMetadataQString += "<tr><td bgcolor=\"white\">";
  myMetadataQString += "<table width=\"100%\">";

  // Table header
  myMetadataQString += "<tr><th bgcolor=\"black\">";
  myMetadataQString += "<font color=\"white\">" + tr("Property") + "</font>";
  myMetadataQString += "</th>";
  myMetadataQString += "<th bgcolor=\"black\">";
  myMetadataQString += "<font color=\"white\">" + tr("Value") + "</font>";
  myMetadataQString += "</th><tr>";

  // WMS Version
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("WMS Version");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += capabilities.version;
  myMetadataQString += "</td></tr>";

  // Service Title
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Title");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += capabilities.service.title;
  myMetadataQString += "</td></tr>";

  // Service Abstract
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Abstract");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += capabilities.service.abstract;
  myMetadataQString += "</td></tr>";

  // Service Keywords
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Keywords");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += capabilities.service.keywordList.join("<br />");
  myMetadataQString += "</td></tr>";

  // Service Online Resource
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Online Resource");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += "-";
  myMetadataQString += "</td></tr>";

  // Service Contact Information
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Contact Person");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += capabilities.service.contactInformation.contactPersonPrimary.contactPerson;
  myMetadataQString += "<br />";
  myMetadataQString += capabilities.service.contactInformation.contactPosition;
  myMetadataQString += "<br />";
  myMetadataQString += capabilities.service.contactInformation.contactPersonPrimary.contactOrganization;
  myMetadataQString += "</td></tr>";

  // Service Fees
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Fees");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += capabilities.service.fees;
  myMetadataQString += "</td></tr>";

  // Service Access Constraints
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Access Constraints");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += capabilities.service.accessConstraints;
  myMetadataQString += "</td></tr>";

  // GetMap Request Formats
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Image Formats");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += capabilities.capability.request.getMap.format.join("<br />");
  myMetadataQString += "</td></tr>";

  // Layer Count (as managed by this provider)
  myMetadataQString += "<tr><td bgcolor=\"gray\">";
  myMetadataQString += tr("Layer Count");
  myMetadataQString += "</td>";
  myMetadataQString += "<td bgcolor=\"gray\">";
  myMetadataQString += QString::number( layersSupported.size() );
  myMetadataQString += "</td></tr>";

  // Close the nested table
  myMetadataQString += "</table>";
  myMetadataQString += "</td></tr>";

  // Iterate through layers

  for (int i = 0; i < layersSupported.size(); i++)
  {

    // TODO: Handle nested layers
    QString layerName = layersSupported[i].name;   // for aesthetic convenience

    // Layer Properties section
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Layer Properties: ");
    myMetadataQString += layerName;
    myMetadataQString += "</td></tr>";
  
    // Use a nested table
    myMetadataQString += "<tr><td bgcolor=\"white\">";
    myMetadataQString += "<table width=\"100%\">";
  
    // Table header
    myMetadataQString += "<tr><th bgcolor=\"black\">";
    myMetadataQString += "<font color=\"white\">" + tr("Property") + "</font>";
    myMetadataQString += "</th>";
    myMetadataQString += "<th bgcolor=\"black\">";
    myMetadataQString += "<font color=\"white\">" + tr("Value") + "</font>";
    myMetadataQString += "</th><tr>";
  
    // Layer Selectivity (as managed by this provider)
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Selected");
    myMetadataQString += "</td>";
    myMetadataQString += "<td bgcolor=\"gray\">";
    myMetadataQString += (activeSubLayers.findIndex(layerName) >= 0) ?
                           tr("Yes") : tr("No");
    myMetadataQString += "</td></tr>";
  
    // Layer Visibility (as managed by this provider)
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Visibility");
    myMetadataQString += "</td>";
    myMetadataQString += "<td bgcolor=\"gray\">";
    myMetadataQString += (activeSubLayers.findIndex(layerName) >= 0) ?
                           (
                            (activeSubLayerVisibility.find(layerName)->second) ?
                            tr("Visible") : tr("Hidden")
                           ) :
                           tr("n/a");
    myMetadataQString += "</td></tr>";
  
    // Layer Title
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Title");
    myMetadataQString += "</td>";
    myMetadataQString += "<td bgcolor=\"gray\">";
    myMetadataQString += layersSupported[i].title;
    myMetadataQString += "</td></tr>";
  
    // Layer Abstract
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Abstract");
    myMetadataQString += "</td>";
    myMetadataQString += "<td bgcolor=\"gray\">";
    myMetadataQString += layersSupported[i].abstract;
    myMetadataQString += "</td></tr>";
  


    // Layer Coordinate Reference Systems
    for ( std::set<QString>::const_iterator it  = layersSupported[i].crs.begin(); 
                                            it != layersSupported[i].crs.end(); 
                                          ++it )
    {
      myMetadataQString += "<tr><td bgcolor=\"gray\">";
      myMetadataQString += tr("Available in CRS");
      myMetadataQString += "</td>";
      myMetadataQString += "<td bgcolor=\"gray\">";
      myMetadataQString += (*it);
      myMetadataQString += "</td></tr>";
    }

    // Layer Styles
    for (int j = 0; j < layersSupported[i].style.size(); j++)
    {
      myMetadataQString += "<tr><td bgcolor=\"gray\">";
      myMetadataQString += tr("Available in style");
      myMetadataQString += "</td>";
      myMetadataQString += "<td bgcolor=\"gray\">";
      myMetadataQString += layersSupported[i].style[j].name;
      myMetadataQString += "</td></tr>";

    myMetadataQString += "<table width=\"100%\">";
  
    // Table header
    myMetadataQString += "<tr><th bgcolor=\"black\">";
    myMetadataQString += "<font color=\"white\">" + tr("Property") + "</font>";
    myMetadataQString += "</th>";
    myMetadataQString += "<th bgcolor=\"black\">";
    myMetadataQString += "<font color=\"white\">" + tr("Value") + "</font>";
    myMetadataQString += "</th><tr>";
  
    // Layer Selectivity (as managed by this provider)
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Selected");
    myMetadataQString += "</td>";
    myMetadataQString += "<td bgcolor=\"gray\">";
    myMetadataQString += (activeSubLayers.findIndex(layerName) >= 0) ?
                           tr("Yes") : tr("No");
    myMetadataQString += "</td></tr>";
  
    // Layer Visibility (as managed by this provider)
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Visibility");
    myMetadataQString += "</td>";
    myMetadataQString += "<td bgcolor=\"gray\">";
    myMetadataQString += (activeSubLayers.findIndex(layerName) >= 0) ?
                           (
                            (activeSubLayerVisibility.find(layerName)->second) ?
                            tr("Visible") : tr("Hidden")
                           ) :
                           tr("n/a");
    myMetadataQString += "</td></tr>";
  
    // Layer Title
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Title");
    myMetadataQString += "</td>";
    myMetadataQString += "<td bgcolor=\"gray\">";
    myMetadataQString += layersSupported[i].title;
    myMetadataQString += "</td></tr>";
  
    // Layer Abstract
    myMetadataQString += "<tr><td bgcolor=\"gray\">";
    myMetadataQString += tr("Abstract");
    myMetadataQString += "</td>";
    myMetadataQString += "<td bgcolor=\"gray\">";
    myMetadataQString += layersSupported[i].abstract;
    myMetadataQString += "</td></tr>";
  


    // Layer Coordinate Reference Systems
    for ( std::set<QString>::const_iterator it  = layersSupported[i].crs.begin(); 
                                            it != layersSupported[i].crs.end(); 
                                          ++it )
    {
      myMetadataQString += "<tr><td bgcolor=\"gray\">";
      myMetadataQString += tr("Available in CRS");
      myMetadataQString += "</td>";
      myMetadataQString += "<td bgcolor=\"gray\">";
      myMetadataQString += (*it);
      myMetadataQString += "</td></tr>";
    }

    // Layer Styles
    for (int j = 0; j < layersSupported[i].style.size(); j++)
    {
      myMetadataQString += "<tr><td bgcolor=\"gray\">";
      myMetadataQString += tr("Available in style");
      myMetadataQString += "</td>";
      myMetadataQString += "<td>";

      // Nested table.
      myMetadataQString += "<table width=\"100%\">";

      // Layer Style Name
      myMetadataQString += "<tr><td bgcolor=\"gray\">";
      myMetadataQString += tr("Name");
      myMetadataQString += "</td>";
      myMetadataQString += "<td bgcolor=\"gray\">";
      myMetadataQString += layersSupported[i].style[j].name;
      myMetadataQString += "</td></tr>";

      // Layer Style Title
      myMetadataQString += "<tr><td bgcolor=\"gray\">";
      myMetadataQString += tr("Title");
      myMetadataQString += "</td>";
      myMetadataQString += "<td bgcolor=\"gray\">";
      myMetadataQString += layersSupported[i].style[j].title;
      myMetadataQString += "</td></tr>";

      // Layer Style Abstract
      myMetadataQString += "<tr><td bgcolor=\"gray\">";
      myMetadataQString += tr("Abstract");
      myMetadataQString += "</td>";
      myMetadataQString += "<td bgcolor=\"gray\">";
      myMetadataQString += layersSupported[i].style[j].abstract;
      myMetadataQString += "</td></tr>";

      // Close the nested table
      myMetadataQString += "</table>";
      myMetadataQString += "</td></tr>";
    }

    // Close the nested table
    myMetadataQString += "</table>";
    myMetadataQString += "</td></tr>";
    }

    // Close the nested table
    myMetadataQString += "</table>";
    myMetadataQString += "</td></tr>";

  }

#ifdef QGISDEBUG
  std::cout << "QgsWmsProvider::getMetadata: exiting with '" << myMetadataQString.toLocal8Bit().data() << "'." << std::endl;
#endif

  return myMetadataQString;
}


QString  QgsWmsProvider::name() const
{
    return WMS_KEY;
} //  QgsWmsProvider::name()



QString  QgsWmsProvider::description() const
{
    return WMS_DESCRIPTION;
} //  QgsWmsProvider::description()




   
/**
 * Class factory to return a pointer to a newly created 
 * QgsWmsProvider object
 */
QGISEXTERN QgsWmsProvider * classFactory(const QString *uri)
{
  return new QgsWmsProvider(*uri);
}
/** Required key function (used to map the plugin to a data store type)
*/
QGISEXTERN QString providerKey(){
  return WMS_KEY;
}
/**
 * Required description function 
 */
QGISEXTERN QString description()
{
    return WMS_DESCRIPTION;
} 
/**
 * Required isProvider function. Used to determine if this shared library
 * is a data provider plugin
 */
QGISEXTERN bool isProvider(){
  return true;
}

