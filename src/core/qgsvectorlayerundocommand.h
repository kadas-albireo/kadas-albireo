/***************************************************************************
    qgsvectorlayerundocommand.h
    ---------------------
    begin                : June 2009
    copyright            : (C) 2009 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVECTORLAYERUNDOCOMMAND_H
#define QGSVECTORLAYERUNDOCOMMAND_H

#include <QUndoCommand>

#include <QVariant>
#include <QSet>
#include <QList>

#include "qgsfield.h"
#include "qgsfeature.h"

class QgsGeometry;
class QgsGeometryCache;

#include "qgsvectorlayer.h"
#include "qgsvectorlayereditbuffer.h"


class CORE_EXPORT QgsVectorLayerUndoCommand : public QUndoCommand
{
  public:
    QgsVectorLayerUndoCommand( QgsVectorLayerEditBuffer *buffer )
        : QUndoCommand()
        , mBuffer( buffer )
    {}
    inline QgsVectorLayer *layer() { return mBuffer->L; }
    inline QgsGeometryCache *cache() { return mBuffer->L->cache(); }

    virtual int id() const override { return -1; }
    virtual bool mergeWith( const QUndoCommand * ) override { return false; }

  protected:
    QgsVectorLayerEditBuffer* mBuffer;
};


class CORE_EXPORT QgsVectorLayerUndoCommandAddFeature : public QgsVectorLayerUndoCommand
{
  public:
    QgsVectorLayerUndoCommandAddFeature( QgsVectorLayerEditBuffer* buffer, QgsFeature& f );

    virtual void undo() override;
    virtual void redo() override;

  private:
    QgsFeature mFeature;
};


class CORE_EXPORT QgsVectorLayerUndoCommandDeleteFeature : public QgsVectorLayerUndoCommand
{
  public:
    QgsVectorLayerUndoCommandDeleteFeature( QgsVectorLayerEditBuffer* buffer, QgsFeatureId fid );

    virtual void undo() override;
    virtual void redo() override;

  private:
    QgsFeatureId mFid;
    QgsFeature mOldAddedFeature;
};


class CORE_EXPORT QgsVectorLayerUndoCommandChangeGeometry : public QgsVectorLayerUndoCommand
{
  public:
    QgsVectorLayerUndoCommandChangeGeometry( QgsVectorLayerEditBuffer* buffer, QgsFeatureId fid, QgsGeometry* newGeom );
    ~QgsVectorLayerUndoCommandChangeGeometry();

    virtual void undo() override;
    virtual void redo() override;
    virtual int id() const override;
    virtual bool mergeWith( const QUndoCommand * ) override;

  private:
    QgsFeatureId mFid;
    QgsGeometry* mOldGeom;
    mutable QgsGeometry* mNewGeom;
};


class CORE_EXPORT QgsVectorLayerUndoCommandChangeAttribute : public QgsVectorLayerUndoCommand
{
  public:
    QgsVectorLayerUndoCommandChangeAttribute( QgsVectorLayerEditBuffer* buffer, QgsFeatureId fid, int fieldIndex, const QVariant &newValue, const QVariant &oldValue );
    virtual void undo() override;
    virtual void redo() override;

  private:
    QgsFeatureId mFid;
    int mFieldIndex;
    QVariant mOldValue;
    QVariant mNewValue;
    bool mFirstChange;
};


class CORE_EXPORT QgsVectorLayerUndoCommandAddAttribute : public QgsVectorLayerUndoCommand
{
  public:
    QgsVectorLayerUndoCommandAddAttribute( QgsVectorLayerEditBuffer* buffer, const QgsField& field );

    virtual void undo() override;
    virtual void redo() override;

  private:
    QgsField mField;
    int mFieldIndex;
};


class CORE_EXPORT QgsVectorLayerUndoCommandDeleteAttribute : public QgsVectorLayerUndoCommand
{
  public:
    QgsVectorLayerUndoCommandDeleteAttribute( QgsVectorLayerEditBuffer* buffer, int fieldIndex );

    virtual void undo() override;
    virtual void redo() override;

  private:
    int mFieldIndex;
    bool mProviderField;
    int mOriginIndex;
    QgsField mOldField;

    QMap<QgsFeatureId, QVariant> mDeletedValues;
};


#endif
