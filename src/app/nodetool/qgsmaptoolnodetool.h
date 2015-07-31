/***************************************************************************
    qgsmaptoolnodetool.h  - add/move/delete vertex integrated in one tool
    ---------------------
    begin                : April 2009
    copyright            : (C) 2009 by Richard Kostecky
    email                : csf dot kostej at mail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPTOOLNODETOOL_H
#define QGSMAPTOOLNODETOOL_H

#include "qgsmaptooledit.h"
#include "qgsmapcanvassnapper.h"

class QRubberBand;

class QgsGeometryRubberBand;
class QgsVertexEntry;
class QgsSelectedFeature;
class QgsNodeEditor;

/** A maptool to move/deletes/adds vertices of line or polygon features*/
class QgsMapToolNodeTool: public QgsMapToolEdit
{
    Q_OBJECT
  public:
    QgsMapToolNodeTool( QgsMapCanvas* canvas );
    virtual ~QgsMapToolNodeTool();

    void canvasMoveEvent( QMouseEvent * e );

    void canvasDoubleClickEvent( QMouseEvent * e );

    void canvasPressEvent( QMouseEvent * e );

    void canvasReleaseEvent( QMouseEvent * e );

    void keyPressEvent( QKeyEvent* e );

    //! called when map tool is being deactivated
    void deactivate();

  public slots:
    void selectedFeatureDestroyed();

    /*
     * the current layer changed
     */
    void currentLayerChanged( QgsMapLayer *layer );

    /*
     * the current edition state changed
     */
    void editingToggled();

  private:
    /**
     * Deletes the rubber band pointers and clears mRubberBands
     */
    void removeRubberBands();

    /**
     * Disconnects signals and clears objects
     */
    void cleanTool( bool deleteSelectedFeature = true );

    /**
     * Function to check if selected feature exists and is same with original one
     * stored in internal structures
     * @param vlayer vector layer for checking
     * @return if feature is same as one in internal structures
     */
    bool checkCorrectnessOfFeature( QgsVectorLayer* vlayer );

    /**
     * Creates rubber bands for ther features when topology editing is enabled
     */
    void createTopologyRubberBands();

    /**
     * Returns the index of first selected vertex, -1 when all unselected
     */
    int firstSelectedVertex();

    /**
     * Select the specified vertex bounded to current index range, returns the valid selected index
     */
    void safeSelectVertex( int vertexNr );

    /** Extracts a single snapping point from a set of snapping results.
    This is useful for snapping operations that just require a position to snap to and not all the
    snapping results. If the list is empty, the screen coordinates are transformed into map coordinates and returned
    @param snapResults results collected from the snapping operation.
    @return the snapped point in map coordinates*/
    QgsPoint snapPointFromResults( const QList<QgsSnappingResult>& snapResults, const QPoint& screenCoords );

    /** Inserts vertices to the snapped segments of the editing layer.
         This is useful for topological editing if snap to segment is enabled.
         @param snapResults results collected from the snapping operation
         @param editedLayer pointer to the editing layer
         @return 0 in case of success*/
    int insertSegmentVerticesForSnap( const QList<QgsSnappingResult>& snapResults, QgsVectorLayer* editedLayer );

    void selectPolygon( const QgsPoint& layerPoint );
    void selectFeature( QgsFeatureId id );

    /** Snapper object that reads the settings from project and option
    and applies it to the map canvas*/
    QgsMapCanvasSnapper mSnapper;

    /** Rubber bands during node move */
    QMap<QgsFeatureId, QgsGeometryRubberBand*> mMoveRubberBands;

    /** Vertices of features to move */
    QMap<QgsFeatureId, QList< QPair<QgsVertexId, QgsPointV2> > > mMoveVertices;

    /** Object containing selected feature and it's vertexes */
    QgsSelectedFeature *mSelectedFeature;

    /** Dock widget which allows to edit vertices */
    QgsNodeEditor* mNodeEditor;

    /** Flag if moving of vertexes is occuring */
    bool mMoving;

    /** Flag if selection of another feature can occur */
    bool mSelectAnother;

    /** Feature id of another feature where user clicked */
    QgsFeatureId mAnother;

    /** Stored position of last press down action to count how much vertexes should be moved */
    QPoint mPressCoordinates;

    /** Closest vertex to click in map coordinates */
    QgsPoint mClosestMapVertex;

    /** Active rubberband for selecting vertexes */
    QRubberBand *mSelectionRubberBand;

    /** Rectangle defining area for selecting vertexes */
    QRect* mRect;

    /** Flag to tell if edition points */
    bool mIsPoint;

    /** Vertex to deselect on release */
    int mDeselectOnRelease;
};

#endif
