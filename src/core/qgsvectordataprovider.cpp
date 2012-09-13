/***************************************************************************
    qgsvectordataprovider.cpp - DataProvider Interface for vector layers
     --------------------------------------
    Date                 : 26-Oct-2004
    Copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QSettings>
#include <QTextCodec>

#include <cfloat> // for DBL_MAX
#include <climits>

#include "qgsvectordataprovider.h"
#include "qgsfeature.h"
#include "qgsfield.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"

QgsVectorDataProvider::QgsVectorDataProvider( QString uri )
    : QgsDataProvider( uri )
    , mCacheMinMaxDirty( true )
    , mFetchFeaturesWithoutGeom( true )
{
  QSettings settings;
  setEncoding( settings.value( "/UI/encoding", "System" ).toString() );
}


QgsVectorDataProvider::~QgsVectorDataProvider()
{
}

QString QgsVectorDataProvider::storageType() const
{
  return "Generic vector file";
}

long QgsVectorDataProvider::updateFeatureCount()
{
  return -1;
}

bool QgsVectorDataProvider::featureAtId( QgsFeatureId featureId,
    QgsFeature& feature,
    bool fetchGeometry,
    QgsAttributeList fetchAttributes )
{
  select( fetchAttributes, QgsRectangle(), fetchGeometry );

  while ( nextFeature( feature ) )
  {
    if ( feature.id() == featureId )
      return true;
  }

  return false;
}

QString QgsVectorDataProvider::dataComment() const
{
  return QString();
}

bool QgsVectorDataProvider::addFeatures( QgsFeatureList &flist )
{
  Q_UNUSED( flist );
  return false;
}

bool QgsVectorDataProvider::deleteFeatures( const QgsFeatureIds &ids )
{
  Q_UNUSED( ids );
  return false;
}

bool QgsVectorDataProvider::addAttributes( const QList<QgsField> &attributes )
{
  Q_UNUSED( attributes );
  return false;
}

bool QgsVectorDataProvider::addAttributes( const QMap<QString, QString> &attributes )
{
  const QList< NativeType > &types = nativeTypes();
  QList< QgsField > list;

  for ( QMap<QString, QString>::const_iterator it = attributes.constBegin(); it != attributes.constEnd(); it++ )
  {
    int i;
    for ( i = 0; i < types.size() && types[i].mTypeName != it.value(); i++ )
      ;

    if ( i == types.size() )
      return false;

    list << QgsField( it.key(), types[i].mType, it.value() );
  }

  return addAttributes( list );
}

bool QgsVectorDataProvider::deleteAttributes( const QgsAttributeIds &attributes )
{
  Q_UNUSED( attributes );
  return false;
}

bool QgsVectorDataProvider::changeAttributeValues( const QgsChangedAttributesMap &attr_map )
{
  Q_UNUSED( attr_map );
  return false;
}

QVariant QgsVectorDataProvider::defaultValue( int fieldId )
{
  Q_UNUSED( fieldId );
  return QVariant();
}

bool QgsVectorDataProvider::changeGeometryValues( QgsGeometryMap &geometry_map )
{
  Q_UNUSED( geometry_map );
  return false;
}

bool QgsVectorDataProvider::createSpatialIndex()
{
  return false;
}

bool QgsVectorDataProvider::createAttributeIndex( int field )
{
  Q_UNUSED( field );
  return true;
}

int QgsVectorDataProvider::capabilities() const
{
  return QgsVectorDataProvider::NoCapabilities;
}


void QgsVectorDataProvider::setEncoding( const QString& e )
{
  QTextCodec* ncodec = QTextCodec::codecForName( e.toLocal8Bit().constData() );
  if ( ncodec )
  {
    mEncoding = ncodec;
  }
  else
  {
    QgsMessageLog::logMessage( tr( "Codec %1 not found. Falling back to system locale" ).arg( e ) );
    mEncoding = QTextCodec::codecForName( "System" );

    if ( !mEncoding )
      mEncoding = QTextCodec::codecForLocale();

    Q_ASSERT( mEncoding );
  }
}

QString QgsVectorDataProvider::encoding() const
{
  if ( mEncoding )
  {
    return mEncoding->name();
  }

  return "";
}

QString QgsVectorDataProvider::capabilitiesString() const
{
  QStringList abilitiesList;

  int abilities = capabilities();

  if ( abilities & QgsVectorDataProvider::AddFeatures )
  {
    abilitiesList += tr( "Add Features" );
    QgsDebugMsg( "Capability: Add Features" );
  }

  if ( abilities & QgsVectorDataProvider::DeleteFeatures )
  {
    abilitiesList += tr( "Delete Features" );
    QgsDebugMsg( "Capability: Delete Features" );
  }

  if ( abilities & QgsVectorDataProvider::ChangeAttributeValues )
  {
    abilitiesList += tr( "Change Attribute Values" );
    QgsDebugMsg( "Capability: Change Attribute Values" );
  }

  if ( abilities & QgsVectorDataProvider::AddAttributes )
  {
    abilitiesList += tr( "Add Attributes" );
    QgsDebugMsg( "Capability: Add Attributes" );
  }

  if ( abilities & QgsVectorDataProvider::DeleteAttributes )
  {
    abilitiesList += tr( "Delete Attributes" );
    QgsDebugMsg( "Capability: Delete Attributes" );
  }

  if ( abilities & QgsVectorDataProvider::CreateSpatialIndex )
  {
    // TODO: Tighten up this test.  See QgsOgrProvider for details.
    abilitiesList += tr( "Create Spatial Index" );
    QgsDebugMsg( "Capability: Create Spatial Index" );
  }

  if ( abilities & QgsVectorDataProvider::SelectAtId )
  {
    abilitiesList += tr( "Fast Access to Features at ID" );
    QgsDebugMsg( "Capability: Select at ID" );
  }

  if ( abilities & QgsVectorDataProvider::ChangeGeometries )
  {
    abilitiesList += tr( "Change Geometries" );
    QgsDebugMsg( "Capability: Change Geometries" );
  }

  return abilitiesList.join( ", " );

}


int QgsVectorDataProvider::fieldNameIndex( const QString& fieldName ) const
{
  const QgsFieldMap &theFields = fields();

  for ( QgsFieldMap::const_iterator it = theFields.constBegin(); it != theFields.constEnd(); ++it )
  {
    if ( QString::compare( it->name(), fieldName, Qt::CaseInsensitive ) == 0 )
    {
      return it.key();
    }
  }
  return -1;
}

QMap<QString, int> QgsVectorDataProvider::fieldNameMap() const
{
  QMap<QString, int> resultMap;

  const QgsFieldMap& theFields = fields();
  QgsFieldMap::const_iterator field_it = theFields.constBegin();
  for ( ; field_it != theFields.constEnd(); ++field_it )
  {
    resultMap.insert( field_it.value().name(), field_it.key() );
  }

  return resultMap;
}

QgsAttributeList QgsVectorDataProvider::attributeIndexes()
{
  uint count = fieldCount();
  QgsAttributeList list;

  for ( uint i = 0; i < count; i++ )
    list.append( i );

  return list;
}

void QgsVectorDataProvider::enableGeometrylessFeatures( bool fetch )
{
  mFetchFeaturesWithoutGeom = fetch;
}

const QList< QgsVectorDataProvider::NativeType > &QgsVectorDataProvider::nativeTypes() const
{
  return mNativeTypes;
}

const QMap<QString, QVariant::Type> &QgsVectorDataProvider::supportedNativeTypes() const
{
  if ( mOldTypeList.size() > 0 )
    return mOldTypeList;

  QgsVectorDataProvider *p = ( QgsVectorDataProvider * )this;

  const QList< QgsVectorDataProvider::NativeType > &types = nativeTypes();

  for ( QList< QgsVectorDataProvider::NativeType >::const_iterator it = types.constBegin(); it != types.constEnd(); it++ )
  {
    p->mOldTypeList.insert( it->mTypeName, it->mType );
  }

  return p->mOldTypeList;
}

bool QgsVectorDataProvider::supportedType( const QgsField &field ) const
{
  int i;
  for ( i = 0; i < mNativeTypes.size(); i++ )
  {
    if ( field.type() == mNativeTypes[i].mType &&
         field.length() >= mNativeTypes[i].mMinLen && field.length() <= mNativeTypes[i].mMaxLen &&
         field.precision() >= mNativeTypes[i].mMinPrec && field.precision() <= mNativeTypes[i].mMaxPrec )
    {
      break;
    }
  }

  return i < mNativeTypes.size();
}

QVariant QgsVectorDataProvider::minimumValue( int index )
{
  if ( !fields().contains( index ) )
  {
    QgsDebugMsg( "Warning: access requested to invalid field index: " + QString::number( index ) );
    return QVariant();
  }

  fillMinMaxCache();

  if ( !mCacheMinValues.contains( index ) )
    return QVariant();

  return mCacheMinValues[index];
}

QVariant QgsVectorDataProvider::maximumValue( int index )
{
  if ( !fields().contains( index ) )
  {
    QgsDebugMsg( "Warning: access requested to invalid field index: " + QString::number( index ) );
    return QVariant();
  }

  fillMinMaxCache();

  if ( !mCacheMaxValues.contains( index ) )
    return QVariant();

  return mCacheMaxValues[index];
}

void QgsVectorDataProvider::uniqueValues( int index, QList<QVariant> &values, int limit )
{
  QgsFeature f;
  QgsAttributeList keys;
  keys.append( index );
  select( keys, QgsRectangle(), false );

  QSet<QString> set;
  values.clear();

  while ( nextFeature( f ) )
  {
    if ( !set.contains( f.attributeMap()[index].toString() ) )
    {
      values.append( f.attributeMap()[index] );
      set.insert( f.attributeMap()[index].toString() );
    }

    if ( limit >= 0 && values.size() >= limit )
      break;
  }
}

void QgsVectorDataProvider::clearMinMaxCache()
{
  mCacheMinMaxDirty = true;
}

void QgsVectorDataProvider::fillMinMaxCache()
{
  if ( !mCacheMinMaxDirty )
    return;

  const QgsFieldMap& flds = fields();
  for ( QgsFieldMap::const_iterator it = flds.begin(); it != flds.end(); ++it )
  {
    if ( it->type() == QVariant::Int )
    {
      mCacheMinValues[it.key()] = QVariant( INT_MAX );
      mCacheMaxValues[it.key()] = QVariant( INT_MIN );
    }
    else if ( it->type() == QVariant::Double )
    {
      mCacheMinValues[it.key()] = QVariant( DBL_MAX );
      mCacheMaxValues[it.key()] = QVariant( -DBL_MAX );
    }
    else
    {
      mCacheMinValues[it.key()] = QVariant();
      mCacheMaxValues[it.key()] = QVariant();
    }
  }

  QgsFeature f;
  QgsAttributeList keys = mCacheMinValues.keys();
  select( keys, QgsRectangle(), false );

  while ( nextFeature( f ) )
  {
    QgsAttributeMap attrMap = f.attributeMap();
    for ( QgsAttributeList::const_iterator it = keys.begin(); it != keys.end(); ++it )
    {
      const QVariant& varValue = attrMap[*it];

      if ( flds[*it].type() == QVariant::Int )
      {
        int value = varValue.toInt();
        if ( value < mCacheMinValues[*it].toInt() )
          mCacheMinValues[*it] = value;
        if ( value > mCacheMaxValues[*it].toInt() )
          mCacheMaxValues[*it] = value;
      }
      else if ( flds[*it].type() == QVariant::Double )
      {
        double value = varValue.toDouble();
        if ( value < mCacheMinValues[*it].toDouble() )
          mCacheMinValues[*it] = value;
        if ( value > mCacheMaxValues[*it].toDouble() )
          mCacheMaxValues[*it] = value;
      }
      else
      {
        QString value = varValue.toString();
        if ( mCacheMinValues[*it].isNull() || value < mCacheMinValues[*it].toString() )
        {
          mCacheMinValues[*it] = value;
        }
        if ( mCacheMaxValues[*it].isNull() || value > mCacheMaxValues[*it].toString() )
        {
          mCacheMaxValues[*it] = value;
        }
      }
    }
  }

  mCacheMinMaxDirty = false;
}

QVariant QgsVectorDataProvider::convertValue( QVariant::Type type, QString value )
{
  QVariant v( value );

  if ( !v.convert( type ) )
    v = QVariant( QString::null );

  return v;
}

static bool _compareEncodings( const QString& s1, const QString& s2 )
{
  return s1.toLower() < s2.toLower();
}

const QStringList &QgsVectorDataProvider::availableEncodings()
{
  if ( smEncodings.isEmpty() )
  {
    foreach ( QString codec, QTextCodec::availableCodecs() )
    {
      smEncodings << codec;
    }
#if 0
    smEncodings << "BIG5";
    smEncodings << "BIG5-HKSCS";
    smEncodings << "EUCJP";
    smEncodings << "EUCKR";
    smEncodings << "GB2312";
    smEncodings << "GBK";
    smEncodings << "GB18030";
    smEncodings << "JIS7";
    smEncodings << "SHIFT-JIS";
    smEncodings << "TSCII";
    smEncodings << "UTF-8";
    smEncodings << "UTF-16";
    smEncodings << "KOI8-R";
    smEncodings << "KOI8-U";
    smEncodings << "ISO8859-1";
    smEncodings << "ISO8859-2";
    smEncodings << "ISO8859-3";
    smEncodings << "ISO8859-4";
    smEncodings << "ISO8859-5";
    smEncodings << "ISO8859-6";
    smEncodings << "ISO8859-7";
    smEncodings << "ISO8859-8";
    smEncodings << "ISO8859-8-I";
    smEncodings << "ISO8859-9";
    smEncodings << "ISO8859-10";
    smEncodings << "ISO8859-11";
    smEncodings << "ISO8859-12";
    smEncodings << "ISO8859-13";
    smEncodings << "ISO8859-14";
    smEncodings << "ISO8859-15";
    smEncodings << "IBM 850";
    smEncodings << "IBM 866";
    smEncodings << "CP874";
    smEncodings << "CP1250";
    smEncodings << "CP1251";
    smEncodings << "CP1252";
    smEncodings << "CP1253";
    smEncodings << "CP1254";
    smEncodings << "CP1255";
    smEncodings << "CP1256";
    smEncodings << "CP1257";
    smEncodings << "CP1258";
    smEncodings << "Apple Roman";
    smEncodings << "TIS-620";
    smEncodings << "System";
#endif
  }

  // Do case-insensitive sorting of encodings
  qSort( smEncodings.begin(), smEncodings.end(), _compareEncodings );

  return smEncodings;
}

void QgsVectorDataProvider::clearErrors()
{
  mErrors.clear();
}

bool QgsVectorDataProvider::hasErrors()
{
  return !mErrors.isEmpty();
}

QStringList QgsVectorDataProvider::errors()
{
  return mErrors;
}

void QgsVectorDataProvider::pushError( QString msg )
{
  mErrors << msg;
}

QStringList QgsVectorDataProvider::smEncodings;
