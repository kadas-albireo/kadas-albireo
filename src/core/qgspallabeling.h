/***************************************************************************
  qgspallabeling.h
  Smart labeling for vector layers
  -------------------
         begin                : June 2009
         copyright            : (C) Martin Dobias
         email                : wonder.sk at gmail.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//Note: although this file is in the core library, it is not part of the stable API
//and might change at any time!

#ifndef QGSPALLABELING_H
#define QGSPALLABELING_H

class QPainter;
class QgsMapRenderer;
class QgsRectangle;
class QgsCoordinateTransform;

#include <QString>
#include <QFont>
#include <QColor>
#include <QList>
#include <QRectF>

namespace pal
{
  class Pal;
  class Layer;
  class LabelPosition;
}

class QgsMapToPixel;
class QgsFeature;
#include "qgspoint.h"

#include "qgsvectorlayer.h" // definition of QgsLabelingEngineInterface

class QgsPalGeometry;

class CORE_EXPORT QgsPalLayerSettings
{
  public:
    QgsPalLayerSettings();
    QgsPalLayerSettings( const QgsPalLayerSettings& s );
    ~QgsPalLayerSettings();

    enum Placement
    {
      AroundPoint, // Point / Polygon
      OverPoint, // Point / Polygon
      Line, // Line / Polygon
      Curved, // Line
      Horizontal, // Polygon
      Free // Polygon
    };

    enum LinePlacementFlags
    {
      OnLine    = 1,
      AboveLine = 2,
      BelowLine = 4,
      MapOrientation = 8
    };

    QString fieldName;
    Placement placement;
    unsigned int placementFlags;
    QFont textFont;
    QColor textColor;
    bool enabled;
    int priority; // 0 = low, 10 = high
    bool obstacle; // whether it's an obstacle
    double dist; // distance from the feature (in mm)
    double vectorScaleFactor; //scale factor painter units->pixels
    double rasterCompressFactor; //pixel resolution scale factor
    int scaleMin, scaleMax; // disabled if both are zero
    double bufferSize; //buffer size (in mm)
    QColor bufferColor;
    bool labelPerPart; // whether to label every feature's part or only the biggest one
    bool mergeLines;
    bool multiLineLabels; //draw labels on multiple lines if they contain '\n'
    double minFeatureSize; // minimum feature size to be labelled (in mm)

    // called from register feature hook
    void calculateLabelSize( QString text, double& labelX, double& labelY );

    // implementation of register feature hook
    void registerFeature( QgsFeature& f, const QgsRenderContext& context );

    void readFromLayer( QgsVectorLayer* layer );
    void writeToLayer( QgsVectorLayer* layer );

    // temporary stuff: set when layer gets prepared
    pal::Layer* palLayer;
    int fieldIndex;
    QFontMetrics* fontMetrics;
    const QgsMapToPixel* xform;
    const QgsCoordinateTransform* ct;
    QgsPoint ptZero, ptOne;
    QList<QgsPalGeometry*> geometries;

  private:
    /**Checks if a feature is larger than a minimum size (in mm)
    @return true if above size, false if below*/
    bool checkMinimumSizeMM( const QgsRenderContext& ct, QgsGeometry* geom, double minSize ) const;
};

class CORE_EXPORT QgsLabelCandidate
{
  public:
    QgsLabelCandidate( QRectF r, double c ): rect( r ), cost( c ) {}

    QRectF rect;
    double cost;
};

class CORE_EXPORT QgsPalLabeling : public QgsLabelingEngineInterface
{
  public:
    QgsPalLabeling();
    ~QgsPalLabeling();

    QgsPalLayerSettings& layer( const char* layerName );

    void numCandidatePositions( int& candPoint, int& candLine, int& candPolygon );
    void setNumCandidatePositions( int candPoint, int candLine, int candPolygon );

    enum Search { Chain, Popmusic_Tabu, Popmusic_Chain, Popmusic_Tabu_Chain, Falp };

    void setSearchMethod( Search s );
    Search searchMethod() const;

    bool isShowingCandidates() const { return mShowingCandidates; }
    void setShowingCandidates( bool showing ) { mShowingCandidates = showing; }
    const QList<QgsLabelCandidate>& candidates() { return mCandidates; }

    bool isShowingAllLabels() const { return mShowingAllLabels; }
    void setShowingAllLabels( bool showing ) { mShowingAllLabels = showing; }

    // implemented methods from labeling engine interface

    //! called when we're going to start with rendering
    virtual void init( QgsMapRenderer* mr );
    //! called to find out whether the layer is used for labeling
    virtual bool willUseLayer( QgsVectorLayer* layer );
    //! hook called when drawing layer before issuing select()
    virtual int prepareLayer( QgsVectorLayer* layer, int& attrIndex, QgsRenderContext& ctx );
    //! hook called when drawing for every feature in a layer
    virtual void registerFeature( QgsVectorLayer* layer, QgsFeature& feat, const QgsRenderContext& context = QgsRenderContext() );
    //! called when the map is drawn and labels should be placed
    virtual void drawLabeling( QgsRenderContext& context );
    //! called when we're done with rendering
    virtual void exit();

    //! called when passing engine among map renderers
    virtual QgsLabelingEngineInterface* clone();

    void drawLabelCandidateRect( pal::LabelPosition* lp, QPainter* painter, const QgsMapToPixel* xform );
    void drawLabel( pal::LabelPosition* label, QPainter* painter, const QgsMapToPixel* xform, bool drawBuffer = false );
    static void drawLabelBuffer( QPainter* p, QString text, const QFont& font, double size, QColor color );

  protected:

    void initPal();


  protected:
    // temporary hashtable of layer settings, being filled during labeling, cleared once labeling's done
    QHash<QgsVectorLayer*, QgsPalLayerSettings> mActiveLayers;
    QgsPalLayerSettings mInvalidLayerSettings;

    QgsMapRenderer* mMapRenderer;
    int mCandPoint, mCandLine, mCandPolygon;
    Search mSearch;

    pal::Pal* mPal;

    // list of candidates from last labeling
    QList<QgsLabelCandidate> mCandidates;
    bool mShowingCandidates;

    bool mShowingAllLabels; // whether to avoid collisions or not
};

#endif // QGSPALLABELING_H
