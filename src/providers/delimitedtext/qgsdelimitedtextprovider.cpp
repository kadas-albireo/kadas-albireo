/***************************************************************************
  qgsdelimitedtextprovider.cpp -  Data provider for delimted text
  -------------------
          begin                : 2004-02-27
          copyright            : (C) 2004 by Gary E.Sherman
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

#include "qgsdelimitedtextprovider.h"

#include <cfloat>
#include <iostream>

#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QStringList>
#include <QMessageBox>
#include <QSettings>
#include <QRegExp>
#include <q3url.h>

#include <ogrsf_frmts.h>

#include "qgsdataprovider.h"
#include "qgsencodingfiledialog.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgsrect.h"
#include "qgis.h"
#include "qgsmessageviewer.h"

#ifdef WIN32
#define QGISEXTERN extern "C" __declspec( dllexport )
#else
#define QGISEXTERN extern "C"
#endif


static const QString TEXT_PROVIDER_KEY = "delimitedtext";
static const QString TEXT_PROVIDER_DESCRIPTION = "Delimited text data provider";



QgsDelimitedTextProvider::QgsDelimitedTextProvider(QString const &uri)
    : QgsVectorDataProvider(uri), 
      mMinMaxCacheDirty(true),
      mShowInvalidLines(true)
{
  // Get the file name and mDelimiter out of the uri
  mFileName = uri.left(uri.find("?"));
  // split the string up on & to get the individual parameters
  QStringList parameters = QStringList::split("&", uri.mid(uri.find("?")));
#ifdef QGISDEBUG
  std::cerr << "Parameter count after split on &" << parameters.
    size() << std::endl;
#endif
  // get the individual parameters and assign values
  QStringList temp = parameters.grep("delimiter=");
  mDelimiter = temp.size()? temp[0].mid(temp[0].find("=") + 1) : "";
  temp = parameters.grep("xField=");
  mXField = temp.size()? temp[0].mid(temp[0].find("=") + 1) : "";
  temp = parameters.grep("yField=");
  mYField = temp.size()? temp[0].mid(temp[0].find("=") + 1) : "";
  // Decode the parts of the uri. Good if someone entered '=' as a delimiter, for instance.
  Q3Url::decode(mFileName);
  Q3Url::decode(mDelimiter);
  Q3Url::decode(mXField);
  Q3Url::decode(mYField);
#ifdef QGISDEBUG
  std::cerr << "Data source uri is " << (const char *)uri.toLocal8Bit().data() << std::endl;
  std::cerr << "Delimited text file is: " << (const char *)mFileName.toLocal8Bit().data() << std::endl;
  std::cerr << "Delimiter is: " << (const char *)mDelimiter.toLocal8Bit().data() << std::endl;
  std::cerr << "xField is: " << (const char *)mXField.toLocal8Bit().data() << std::endl;
  std::cerr << "yField is: " << (const char *)mYField.toLocal8Bit().data() << std::endl;
#endif
  // Set the selection rectangle to null
  mSelectionRectangle = 0;
  // assume the layer is invalid until proven otherwise
  mValid = false;
  if (!mFileName.isEmpty() && !mDelimiter.isEmpty() && !mXField.isEmpty() &&
      !mYField.isEmpty())
  {
    // check to see that the file exists and perform some sanity checks
    if (QFile::exists(mFileName))
    {
      // Open the file and get number of rows, etc. We assume that the
      // file has a header row and process accordingly. Caller should make
      // sure the the delimited file is properly formed.
      mFile = new QFile(mFileName);
      if (mFile->open(QIODevice::ReadOnly))
      {
        mStream = new QTextStream(mFile);
        QString line;
        mNumberFeatures = 0;
        int xyCount = 0;
        int lineNumber = 0;
        // set the initial extent
        mExtent = new QgsRect();
        //commented out by Tim for now - setMinimal needs to be merged in from 0.7 branch
        //mExtent->setMinimal(); // This defeats normalization
        bool firstPoint = true;
        while (!mStream->atEnd())
        {
          lineNumber++;
          line = mStream->readLine(); // line of text excluding '\n', default local 8 bit encoding.
          if (mNumberFeatures++ == 0)
          {
            // Get the fields from the header row and store them in the 
            // fields vector
#ifdef QGISDEBUG
            std::
              cerr << "Attempting to split the input line: " << (const char *)line.toLocal8Bit().data() <<
              " using delimiter " << (const char *)mDelimiter.toLocal8Bit().data() << std::endl;
#endif
            QStringList fieldList =
              QStringList::split(QRegExp(mDelimiter), line, true);
#ifdef QGISDEBUG
            std::cerr << "Split line into " 
                      << fieldList.size() << " parts" << std::endl;
#endif
            // We don't know anything about a text based field other
            // than its name. All fields are assumed to be text
            int fieldPos = 0;
            for (QStringList::Iterator it = fieldList.begin();
                 it != fieldList.end(); ++it)
            {
              QString field = *it;
              if (field.length() > 0)
              {
                attributeFields.push_back(QgsField(*it, "Text"));
                fieldPositions[*it] = fieldPos++;
                // check to see if this field matches either the x or y field 
                if (mXField == *it)
                {
#ifdef QGISDEBUG
                  std::cerr << "Found x field " << (const char *)(*it).toLocal8Bit().data() << std::endl;
#endif
                  xyCount++;
                }
                if (mYField == *it)
                {
#ifdef QGISDEBUG
                  std::cerr << "Found y field " << (const char *)(*it).toLocal8Bit().data() << std::endl;
#endif
                  xyCount++;
                }
#ifdef QGISDEBUG
                std::cerr << "Adding field: " << (const char *)(*it).toLocal8Bit().data() << std::endl;
#endif

              }
            }
#ifdef QGISDEBUG
            std::
              cerr << "Field count for the delimited text file is " <<
              attributeFields.size() << std::endl;
#endif
          }
          else
          {
            // examine the x,y and update extents
            //  std::cout << line << std::endl; 
            // split the line on the delimiter
            QStringList parts =
              QStringList::split(QRegExp(mDelimiter), line, true);
            //if(parts.size() == attributeFields.size())
            //{
            //  // we can populate attributes if required
            //  fieldsMatch = true;
            //}else
            //{
            //  fieldsMatch = false;
            //}
            /*
               std::cout << "Record hit line " << lineNumber << ": " <<
               parts[fieldPositions[mXField]] << ", " <<
               parts[fieldPositions[mYField]] << std::endl;
             */
            // Get the x and y values, first checking to make sure they
            // aren't null.
            QString sX = parts[fieldPositions[mXField]];
            QString sY = parts[fieldPositions[mYField]];
            //std::cout << "x ,y " << sX << ", " << sY << std::endl; 
            bool xOk = true;
            bool yOk = true;
            double x = sX.toDouble(&xOk);
            double y = sY.toDouble(&yOk);

            if (xOk && yOk)
            {
              if (!firstPoint)
              {
                if (x > mExtent->xMax())
                {
                  mExtent->setXmax(x);
                }
                if (x < mExtent->xMin())
                {
                  mExtent->setXmin(x);
                }
                if (y > mExtent->yMax())
                {
                  mExtent->setYmax(y);
                }
                if (y < mExtent->yMin())
                {
                  mExtent->setYmin(y);
                }
              }
              else
              { // Extent for the first point is just the first point
                mExtent->set(x,y,x,y);
                firstPoint = false;
              }
            }
          }
        }
        reset();
        mNumberFeatures--;

        if (xyCount == 2)
        {
#ifdef QGISDEBUG
          std::cerr << "Data store is valid" << std::endl;
          std::cerr << "Number of features " << mNumberFeatures << std::endl;
          std::cerr << "Extents " << (const char *)mExtent->stringRep().toLocal8Bit().data() << std::endl;
#endif
          mValid = true;
        }
        else
        {
          std::
            cerr << "Data store is invalid. Specified x,y fields do not match\n"
            << "those in the database (xyCount=" << xyCount << ")" << std::endl;
        }
      }
#ifdef QGISDEBUG
      std::cerr << "Done checking validity\n";
#endif

      //resize the cache matrix
      mMinMaxCache = new double *[attributeFields.size()];
      for (int i = 0; i < attributeFields.size(); i++)
      {
        mMinMaxCache[i] = new double[2];
      }
    }
    else
      // file does not exist
      std::
        cerr << "Data source " << (const char *)getDataSourceUri().toLocal8Bit().data() << " could not be opened" <<
        std::endl;

  }
  else
  {
    // uri is invalid so the layer must be too...
    std::cerr << "Data source is invalid" << std::endl;

  }
}

QgsDelimitedTextProvider::~QgsDelimitedTextProvider()
{
  mFile->close();
  delete mFile;
  delete mStream;
  for (int i = 0; i < fieldCount(); i++)
  {
    delete mMinMaxCache[i];
  }
  delete[]mMinMaxCache;
}


QString QgsDelimitedTextProvider::storageType()
{
  return "Delimited text file";
}


/**
 * Get the first feature resutling from a select operation
 * @return QgsFeature
 */
QgsFeature * QgsDelimitedTextProvider::getFirstFeature(bool fetchAttributes)
{
    QgsFeature *f = new QgsFeature;

    reset();                    // reset back to first feature

    if ( getNextFeature_( *f, fetchAttributes ) )
    {
        return f;
    }

    delete f;

    return 0x0;
} // QgsDelimitedTextProvider::getFirstFeature(bool fetchAttributes)

/**

  insure double value is properly translated into locate endian-ness

*/
static
double
translateDouble_( double d )
{
    union
    {
        double fpval;
        char   char_val[8];
    } from, to;

    // break double into byte sized chunks
    from.fpval = d;

    to.char_val[7] = from.char_val[0];
    to.char_val[6] = from.char_val[1];
    to.char_val[5] = from.char_val[2];
    to.char_val[4] = from.char_val[3];
    to.char_val[3] = from.char_val[4];
    to.char_val[2] = from.char_val[5];
    to.char_val[1] = from.char_val[6];
    to.char_val[0] = from.char_val[7];

    return to.fpval;

} // translateDouble_


bool
QgsDelimitedTextProvider::getNextFeature_( QgsFeature & feature, 
                                           bool getAttributes,
                                           std::list<int> const * desiredAttributes )
{
    // before we do anything else, assume that there's something wrong with
    // the feature
    feature.setValid( false );
    while ( ! mStream->atEnd() )
    {
      QString line = mStream->readLine(); // Default local 8 bit encoding
        // lex the tokens from the current data line
        QStringList tokens = QStringList::split(QRegExp(mDelimiter), line, true);

        bool xOk = false;
        bool yOk = false;

        int xFieldPos = fieldPositions[mXField];
        int yFieldPos = fieldPositions[mYField];

        double x = tokens[xFieldPos].toDouble( &xOk );
        double y = tokens[yFieldPos].toDouble( &yOk );

        if (! (xOk && yOk))
        {
          // Accumulate any lines that weren't ok, to report on them
          // later, and look at the next line in the file, but only if
          // we need to.
          if (mShowInvalidLines)
            mInvalidLines << line;

          continue;
        }

        // Give every valid line in the file an id, even if it's not
        // in the current extent or bounds.
        ++mFid;             // increment to next feature ID

        if (! boundsCheck(x,y))
          continue;

        // at this point, one way or another, the current feature values
        // are valid
           feature.setValid( true );

           feature.setFeatureId( mFid );

           QByteArray  buffer;
           QDataStream s( &buffer, QIODevice::WriteOnly ); // open on buffers's data

           switch ( endian() )
           {
               case QgsDataProvider::NDR :
                   // we're on a little-endian platform, so tell the data
                   // stream to use that
                   s.setByteOrder( QDataStream::LittleEndian );
                   s << (Q_UINT8)1; // 1 is for little-endian
                   break;
               case QgsDataProvider::XDR :
                   // don't change byte order since QDataStream is big endian by default
                   s << (Q_UINT8)0; // 0 is for big-endian
                   break;
               default :
                   qDebug( "%s:%d unknown endian", __FILE__, __LINE__ );
                   //delete [] geometry;
                   return false;
           }

           s << (Q_UINT32)QGis::WKBPoint;
           s << x;
           s << y;

           unsigned char* geometry = new unsigned char[buffer.size()];
           memcpy(geometry, buffer.data(), buffer.size());

           feature.setGeometryAndOwnership( geometry, sizeof(wkbPoint) );

           if ( getAttributes && ! desiredAttributes )
           {
               for (int fi = 0; fi < attributeFields.size(); fi++)
               {
                   feature.addAttribute(attributeFields[fi].name(), tokens[fi]);
               }
           }
           // regardless of whether getAttributes is true or not, if the
           // programmer went through the trouble of passing in such a list of
           // attribute fields, then obviously they want them
           else if ( desiredAttributes )
           {
               for ( std::list<int>::const_iterator i = desiredAttributes->begin();
                     i != desiredAttributes->end();
                     ++i )
               {
                   feature.addAttribute(attributeFields[*i].name(), tokens[*i]);
               }
           }
           // We have a good line, so return
           return true;

    } // ! textStream EOF

    // End of the file. If there are any lines that couldn't be
    // loaded, display them now.

    if (mShowInvalidLines && !mInvalidLines.isEmpty())
    {
      mShowInvalidLines = false;
      QgsMessageViewer lineViewer;
      lineViewer.setMessageAsPlainText(tr("Note: the following lines were not loaded because Qgis was unable to determine values for the x and y coordinates:\n"));
      for (int i = 0; i < mInvalidLines.size(); ++i)
        lineViewer.appendMessage(mInvalidLines.at(i));

      lineViewer.exec();
      // We no longer need these lines.
      mInvalidLines.empty();
    }

    return false;

} // getNextFeature_( QgsFeature & feature )



/**
  Get the next feature resulting from a select operation
  Return 0 if there are no features in the selection set
 * @return false if unable to get the next feature
 */
bool QgsDelimitedTextProvider::getNextFeature(QgsFeature & feature,
                                              bool fetchAttributes)
{
    return getNextFeature_( feature, fetchAttributes );
} // QgsDelimitedTextProvider::getNextFeature



QgsFeature * QgsDelimitedTextProvider::getNextFeature(bool fetchAttributes)
{
    QgsFeature * f = new QgsFeature;

    if ( getNextFeature( *f, fetchAttributes ) )
    {
        return f;
    }
    
    delete f;

    return 0x0;
} // QgsDelimitedTextProvider::getNextFeature(bool fetchAttributes)



QgsFeature * QgsDelimitedTextProvider::getNextFeature(std::list<int> const & desiredAttributes, int featureQueueSize)
{
    QgsFeature * f = new QgsFeature;

    if ( getNextFeature_( *f, true, &desiredAttributes ) )
    {
        return f;
    }
    
    delete f;

    return 0x0;

} // QgsDelimitedTextProvider::getNextFeature(std::list < int >&attlist)




/**
 * Select features based on a bounding rectangle. Features can be retrieved
 * with calls to getFirstFeature and getNextFeature.
 * @param mbr QgsRect containing the extent to use in selecting features
 */
void QgsDelimitedTextProvider::select(QgsRect * rect, bool useIntersect)
{

  // Setting a spatial filter doesn't make much sense since we have to
  // compare each point against the rectangle.
  // We store the rect and use it in getNextFeature to determine if the
  // feature falls in the selection area
  reset();
  mSelectionRectangle = new QgsRect((*rect));
}


/**
 * Identify features within the search radius specified by rect
 * @param rect Bounding rectangle of search radius
 * @return std::vector containing QgsFeature objects that intersect rect
 */
std::vector < QgsFeature > &QgsDelimitedTextProvider::identify(QgsRect * rect)
{
  // reset the data source since we need to be able to read through
  // all features
  reset();
  std::cerr << "Attempting to identify features falling within " << (const char *)rect->
    stringRep().toLocal8Bit().data() << std::endl;
  // select the features
  select(rect);
#ifdef WIN32
  //TODO fix this later for win32
  std::vector < QgsFeature > feat;
  return feat;
#endif

}

/*
   unsigned char * QgsDelimitedTextProvider::getGeometryPointer(OGRFeature *fet){
   unsigned char *gPtr=0;
// get the wkb representation

//geom->exportToWkb((OGRwkbByteOrder) endian(), gPtr);
return gPtr;

}
*/


// Return the extent of the layer
QgsRect *QgsDelimitedTextProvider::extent()
{
  return new QgsRect(mExtent->xMin(), mExtent->yMin(), mExtent->xMax(),
                     mExtent->yMax());
}

/** 
 * Return the feature type
 */
int QgsDelimitedTextProvider::geometryType() const
{
  return 1;                     // WKBPoint
}

/** 
 * Return the feature type
 */
long QgsDelimitedTextProvider::featureCount() const
{
  return mNumberFeatures;
}

/**
 * Return the number of fields
 */
int QgsDelimitedTextProvider::fieldCount() const
{
  return attributeFields.size();
}

/**
 * Fetch attributes for a selected feature
 */
void QgsDelimitedTextProvider::getFeatureAttributes(int key, QgsFeature * f)
{
  //for (int i = 0; i < ogrFet->GetFieldCount(); i++) {

  //  // add the feature attributes to the tree
  //  OGRFieldDefn *fldDef = ogrFet->GetFieldDefnRef(i);
  //  QString fld = fldDef->GetNameRef();
  //  //    OGRFieldType fldType = fldDef->GetType();
  //  QString val;

  //  val = ogrFet->GetFieldAsString(i);
  //  f->addAttribute(fld, val);
  //}
}

std::vector<QgsField> const & QgsDelimitedTextProvider::fields() const
{
  return attributeFields;
}

void QgsDelimitedTextProvider::reset()
{
  // Reset feature id to 0
  mFid = 0;
  // Skip ahead one line since first record is always assumed to be
  // the header record
  mStream->seek(0);
  mStream->readLine();
  //reset any spatial filters
  if(mSelectionRectangle && mExtent)
    {
      *mSelectionRectangle = *mExtent;
    }
}

QString QgsDelimitedTextProvider::minValue(int position)
{
  if (position >= fieldCount())
  {
    std::
      cerr << "Warning: access requested to invalid position " <<
      "in QgsDelimitedTextProvider::minValue(..)" << std::endl;
  }
  if (mMinMaxCacheDirty)
  {
    fillMinMaxCash();
  }
  return QString::number(mMinMaxCache[position][0], 'f', 2);
}


QString QgsDelimitedTextProvider::maxValue(int position)
{
  if (position >= fieldCount())
  {
    std::
      cerr << "Warning: access requested to invalid position " <<
      "in QgsDelimitedTextProvider::maxValue(..)" << std::endl;
  }
  if (mMinMaxCacheDirty)
  {
    fillMinMaxCash();
  }
  return QString::number(mMinMaxCache[position][1], 'f', 2);
}

void QgsDelimitedTextProvider::fillMinMaxCash()
{
  for (int i = 0; i < fieldCount(); i++)
  {
    mMinMaxCache[i][0] = DBL_MAX;
    mMinMaxCache[i][1] = -DBL_MAX;
  }

  QgsFeature f;
  reset();

  getNextFeature(f, true);
  do
  {
    for (int i = 0; i < fieldCount(); i++)
    {
      double value = (f.attributeMap())[i].fieldValue().toDouble();
      if (value < mMinMaxCache[i][0])
      {
        mMinMaxCache[i][0] = value;
      }
      if (value > mMinMaxCache[i][1])
      {
        mMinMaxCache[i][1] = value;
      }
    }
  }
  while (getNextFeature(f, true));

  mMinMaxCacheDirty = false;
}

//TODO - add sanity check for shape file layers, to include cheking to
//       see if the .shp, .dbf, .shx files are all present and the layer
//       actually has features
bool QgsDelimitedTextProvider::isValid()
{
  return mValid;
}

/** 
 * Check to see if the point is within the selection rectangle
 */
bool QgsDelimitedTextProvider::boundsCheck(double x, double y)
{
  bool inBounds(true);
  if (mSelectionRectangle)
    inBounds = (((x < mSelectionRectangle->xMax()) &&
                 (x > mSelectionRectangle->xMin())) &&
                ((y < mSelectionRectangle->yMax()) &&
                 (y > mSelectionRectangle->yMin())));
  // QString hit = inBounds?"true":"false";

  // std::cerr << "Checking if " << x << ", " << y << " is in " << 
  //mSelectionRectangle->stringRep().ascii() << ": " << hit.ascii() << std::endl; 
  return inBounds;
}

int QgsDelimitedTextProvider::capabilities() const
{
    return QgsVectorDataProvider::SaveAsShapefile;
}


bool QgsDelimitedTextProvider::saveAsShapefile()
{
  // save the layer as a shapefile
  QString driverName = "ESRI Shapefile";
  OGRSFDriver *poDriver;
  OGRRegisterAll();
  poDriver =
    OGRSFDriverRegistrar::GetRegistrar()->
    GetDriverByName((const char *)driverName.toLocal8Bit().data());
  bool returnValue = true;
  if (poDriver != NULL)
  {
    // get a name for the shapefile
    // Get a file to process, starting at the current directory
    // Set inital dir to last used in delimited text plugin
    QSettings settings;
    QString enc;
    QString shapefileName;
    QString filter =  QString(tr("Shapefiles (*.shp)", 
                                 "The *.shp is used as a file filter "
                                 "in a dialog box");
    QString dirName = settings.readEntry("/Plugin-DelimitedText/text_path", "./");

    QgsEncodingFileDialog* openFileDialog = new QgsEncodingFileDialog(0,
                                                                      tr("Save layer as..."),
                                                                      dirName,
                                                                      filter,
                                                                      QString("UTF-8"));

    // allow for selection of more than one file
    openFileDialog->setMode(QFileDialog::AnyFile);


    if (openFileDialog->exec() == QDialog::Accepted)
    {
        shapefileName = openFileDialog->selectedFile();
        enc = openFileDialog->encoding();
    }
    else
    {
      return returnValue;
    }

    if (!shapefileName.isNull())
    {
      // add the extension if not present
      if (shapefileName.find(".shp") == -1)
      {
        shapefileName += ".shp";
      }
      OGRDataSource *poDS;
      // create the data source
      poDS = poDriver->CreateDataSource((const char *) shapefileName, NULL);
      if (poDS != NULL)
      {
        QTextCodec* saveCodec = QTextCodec::codecForName(enc.toLocal8Bit().data());
        if(!saveCodec)
        {
#ifdef QGISDEBUG
          qWarning("error finding QTextCodec in QgsDelimitedTextProvider::saveAsShapefile()");
#endif
          saveCodec = QTextCodec::codecForLocale();
        }
 
        std::cerr << "created datasource" << std::endl;
        // datasource created, now create the output layer, use utf8() for now.
        OGRLayer *poLayer;
        poLayer =
           poDS->CreateLayer((const char *) (shapefileName.
                             left(shapefileName.find(".shp"))).utf8(), NULL,
                              static_cast < OGRwkbGeometryType > (1), NULL);
        if (poLayer != NULL)
        {
          std::cerr << "created layer" << std::endl;
          // calculate the field lengths
          int *lengths = getFieldLengths();
          // create the fields
          std::cerr << "creating " << attributeFields.
            size() << " fields" << std::endl;
          for (int i = 0; i < attributeFields.size(); i++)
          {
            // check the field length - if > 10 we need to truncate it
            QgsField attrField = attributeFields[i];
            if (attrField.name().length() > 10)
            {
              attrField = attrField.name().left(10);
            }
            // all fields are created as string (for now)
            OGRFieldDefn fld(saveCodec->fromUnicode(attrField.name()), OFTString);
            // set the length for the field -- but we don't know what it is...
            fld.SetWidth(lengths[i]);
            // create the field
            std::cerr << "creating field " << (const char *)attrField.
              name().toLocal8Bit().data() << " width length " << lengths[i] << std::endl;
            if (poLayer->CreateField(&fld) != OGRERR_NONE)
            {
              QMessageBox::warning(0, tr("Error"),
                                   tr("Error creating field ") + attrField.name());
            }
          }
          // read the delimited text file and create the features
          std::cerr << "Done creating fields" << std::endl;
          // read the line
          reset();
          QString line;
          while (!mStream->atEnd())
          {
            line = mStream->readLine(); // line of text excluding '\n'
            // split the line
            QStringList parts =
              QStringList::split(QRegExp(mDelimiter), line, true);

            // create the feature
            OGRFeature *poFeature;

            poFeature = new OGRFeature(poLayer->GetLayerDefn());

            // iterate over the parts and set the fields
            // set limit - we will ignore extra fields on the line
            int limit = attributeFields.size();

            if (parts.size() < limit)
            {

              // this is bad - not enough values where supplied on the line
              // TODO We should inform the user about this...
            }
            else
            {

              for (int i = 0; i < limit; i++)
              {
                if (parts[i] != QString::null)
                {
                  poFeature->SetField(saveCodec->fromUnicode(attributeFields[i].name()).data(),
                                      saveCodec->fromUnicode(parts[i]).data());

                }
                else
                {
                  poFeature->SetField(saveCodec->fromUnicode(attributeFields[i].name()).data(), "");
                }
              }
              std::cerr << "Field values set" << std::endl;
              // create the point
              OGRPoint *poPoint = new OGRPoint();
              QString sX = parts[fieldPositions[mXField]];
              QString sY = parts[fieldPositions[mYField]];
              poPoint->setX(sX.toDouble());
              poPoint->setY(sY.toDouble());
              std::cerr << "Setting geometry" << std::endl;

              poFeature->SetGeometryDirectly(poPoint);
              if (poLayer->CreateFeature(poFeature) != OGRERR_NONE)
              {
                std::cerr << "Failed to create feature in shapefile" << std::
                  endl;
              }
              else
              {
                std::cerr << "Added feature" << std::endl;
              }

              delete poFeature;
            }
          }
          delete poDS;
        }
        else
        {
          QMessageBox::warning(0, tr("Error"), tr("Layer creation failed"));
        }

      }
      else
      {
        QMessageBox::warning(0, tr("Error creating shapefile"),
                             tr("The shapefile could not be created (") +
                             shapefileName + ")");
      }

    }
    //std::cerr << "Saving to " << shapefileName << std::endl; 
  }
  else
  {
    QMessageBox::warning(0, tr("Driver not found"),
                         driverName + tr(" driver is not available"));
    returnValue = false;
  }
  return returnValue;
}



size_t QgsDelimitedTextProvider::layerCount() const
{
    return 1;                   // XXX How to calculate the layers?
} // QgsOgrProvider::layerCount()



int *QgsDelimitedTextProvider::getFieldLengths()
{
  // this function parses the entire data file and calculates the
  // max for each

  // Only do this if we haven't done it already (ie. the vector is
  // empty)
  int *lengths = new int[attributeFields.size()];
  // init the lengths to zero
  for (int il = 0; il < attributeFields.size(); il++)
  {
    lengths[il] = 0;
  }
  if (mValid)
  {
    reset();
    // read the line
    QString line;
    while (!mStream->atEnd())
    {
      line = mStream->readLine(); // line of text excluding '\n'
      // split the line
      QStringList parts = QStringList::split(QRegExp(mDelimiter), line, true);
      // iterate over the parts and update the max value
      for (int i = 0; i < parts.size(); i++)
      {
        if (parts[i] != QString::null)
        {
          // std::cerr << "comparing length for " << parts[i] << " against max len of " << lengths[i] << std::endl; 
          if (parts[i].length() > lengths[i])
          {
            lengths[i] = parts[i].length();
          }
        }

      }
    }
  }
  return lengths;
}





QString  QgsDelimitedTextProvider::name() const
{
    return TEXT_PROVIDER_KEY;
} // ::name()



QString  QgsDelimitedTextProvider::description() const
{
    return TEXT_PROVIDER_DESCRIPTION;
} //  QgsDelimitedTextProvider::name()


/**
 * Class factory to return a pointer to a newly created 
 * QgsDelimitedTextProvider object
 */
QGISEXTERN QgsDelimitedTextProvider *classFactory(const QString *uri)
{
  return new QgsDelimitedTextProvider(*uri);
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
