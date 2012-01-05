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

#include "qgsmaptoolvertexedit.h"
#include "qgsfeature.h"
#include "qgsvertexmarker.h"
#include "qgsgeometryvalidator.h"
#include "qgsmapcanvas.h"
#include "qgslogger.h"

#include <QRect>
#include <QRubberBand>

class QgsRubberBand;

class VertexEntry
{
    bool mSelected;
    QgsPoint mPoint;
    int mEquals;
    bool mInRubberBand;
    int mRubberBandNr;
    int mIndex;
    int mOriginalIndex;
    int mPenWidth;
    QString mToolTip;
    QgsVertexMarker::IconType mType;
    QgsVertexMarker *mMarker;
    QgsMapCanvas *mCanvas;
    QgsMapLayer *mLayer;
  public:
    VertexEntry( QgsMapCanvas *canvas, QgsMapLayer *layer, QgsPoint p, int originalIndex, QString tooltip = QString::null, QgsVertexMarker::IconType type = QgsVertexMarker::ICON_BOX, int penWidth = 2 );
    ~VertexEntry();

    QgsPoint point() const { return mPoint; }
    int equals() const { return mEquals; }
    bool isSelected() const { return mSelected; }
    bool isInRubberBand() const { return mInRubberBand; }

    void setCenter( QgsPoint p );
    void moveCenter( double x, double y );
    void setEqual( int index ) { mEquals = index; }
    void setSelected( bool selected = true );
    void setInRubberBand( bool inRubberBand = true ) { mInRubberBand = inRubberBand; }
    int rubberBandNr() const { return mRubberBandNr; }
    int index() { return mIndex; }

    void setRubberBandValues( bool inRubberBand, int rubberBandNr, int indexInRubberBand );
    void update();
};

/**
 * Set representing set of vertex numbers
 */
typedef QSet<int> Vertexes;

/**
 * Constant representing zero value for distance. It's 0 because of error in double counting.
 */
const static double ZERO_TOLERANCE = 0.000000001;

/**
 * Class that supports feature which is selected/
 */
class SelectionFeature: public QObject
{
    Q_OBJECT

  public:
    SelectionFeature();
    ~SelectionFeature();

    /**
     * Setting selected feature
     * @param featureId id of feature which was selected
     * @param vlayer vector layer in which feature is selected
     * @param rubberBand rubber band which displays feature
     * @param canvas mapCanvas on which we are working
     * @param feature feature with which we work this parameter is not mandatory if it's not filled feature will be loaded
     */
    void setSelectedFeature( QgsFeatureId featureId, QgsVectorLayer* vlayer, QgsRubberBand* rubberBand, QgsMapCanvas* canvas, QgsFeature* feature = NULL );

    /**
     * Function to select vertex with number
     * @param vertexNr number of vertex which is to be selected
     */
    void selectVertex( int vertexNr );

    /**
     * Function to deselect vertex with number
     * @param vertexNr number of vertex which is to be deselected
     */
    void deselectVertex( int vertexNr );

    /**
     * Deselects all vertexes of selected feature
     */
    void deselectAllVertexes();

    /**
     * Deletes all selected vertexes
     */
    void deleteSelectedVertexes();

    /**
     * Moves selected vertex
     * @param changeX change in X coordinate
     * @param changeY change in Y coordinate
     */
    void moveSelectedVertexes( double changeX, double changeY );

    /**
     * Inverts selection of vertex with number
     * @param vertexNr number of vertex which is to be inverted
     * @param invert flag if vertex selection should be inverted or not
     */
    void invertVertexSelection( int vertexNr, bool invert = true );

    /**
     * Updates vertex markers position accoording to changed feature geometry
     * @param canvas map canvas we are working with
     */
    void updateVertexMarkersPosition();

    /**
     * Tells if vertex is selected
     * @param vertexNr number of vertex for which we are getting info
     * @return true if vertex is selected, false otherwise
     */
    bool isSelected( int vertexNr );

    /**
     * Getting feature Id of feature selected
     * @return feature id of selected feature
     */
    QgsFeatureId featureId();

    /**
     * Getting vertex map of vertexes
     * @return currently used vertex map
     */
    QList<VertexEntry*> &vertexMap();

    /**
     * Getting currently edited feature
     * @return selected feature
     */
    QgsFeature* feature();

    /**
     * Updates whole selection object from the selected object
     */
    void updateFromFeature();

    /**
     * Clears data about vertexes if they are in rubber band for moving etc.
     */
    void cleanRubberBandsData();

    /**
     * Getter for getting vector layer which selection is working
     * @return used vector layer
     */
    QgsVectorLayer *vlayer();

  public slots:
    void addError( QgsGeometry::Error );
    void validationFinished();

  private:
    /**
     * Deletes whole vertex map.
     */
    void deleteVertexMap();

    /**
     * Creates vertex map when
     */
    void createVertexMap();

    /**
     *  Creates vertex map for polygon type feature
     */
    void createVertexMapPolygon();

    /**
     *  Creates vertex map for line type feature
     */
    void createVertexMapLine();

    /**
     *  Creates vertex map for ppint type feature
     */
    void createVertexMapPoint();

    /**
     * Updates stored feature to actual one loaded from layer
     */
    void updateFeature();

    /**
     * Validates the geometry
     */
    void validateGeometry( QgsGeometry *g = NULL );

    QgsFeature* mFeature;
    QgsFeatureId mFeatureId;
    bool mFeatureSelected;
    QgsVectorLayer* mVlayer;
    QgsRubberBand* mRubberBand;
    QList<VertexEntry*> mVertexMap;
    QgsMapCanvas* mCanvas;

    QgsGeometryValidator *mValidator;
    QString mTip;
    QList< QgsGeometry::Error > mGeomErrors;
    QList< QgsVertexMarker * > mGeomErrorMarkers;
};

/**A maptool to move/deletes/adds vertices of line or polygon features*/
class QgsMapToolNodeTool: public QgsMapToolVertexEdit
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

    void keyReleaseEvent( QKeyEvent* e );

    //! called when map tool is being deactivated
    void deactivate();

    /**
     * Returns closest vertex to given point from selected feature
     */
    QgsPoint getClosestVertex( QgsPoint point );

  public slots:
    /**
     * Slot to count with layer change
     * @param layer layer to which selection changed
     */
    void currentLayerChanged( QgsMapLayer* layer );

  protected slots:
    /**
     * Processing incoming signal of deleted feature (for deletion of selected feature)
     * @param featureId id of deleted feature
     */
    void featureDeleted( QgsFeatureId featureId );

    /**
     * Processing incoming signal of deleted feature (for deletion of selected feature)
     * @param featureId id of deleted feature
     */
    void layerModified( bool onlyGeometry );

    /**
     * Processing when layers are changed problem when layer is closed
     */
    void layersChanged();

    /**
     * Changed coordinates to markers need to be redrawn to correct position
     */
    void coordinatesChanged();

  private:

    /**
     * Connects signal which are required for correct work with bakground changes
     * @param vlayer vector layer which is emiting these signals
     */
    void connectSignals( QgsVectorLayer* vlayer );

    /**
     * Deletes the rubber band pointers and clears mRubberBands
     */
    void removeRubberBands();

    /**
     * Creating rubber band marker for movin of point
     * @param center coordinates of point to be moved
     * @param vlayer vector layer on which we are working
     * @return rubber band marker
     */
    QgsRubberBand* createRubberBandMarker( QgsPoint center, QgsVectorLayer* vlayer );

    /**
     * Function to check if selected feature exists and is same with original one
     * stored in internal structures
     * @param vlayer vector layer for checking
     * @return if feature is same as one in internal structures
     */
    bool checkCorrectnessOfFeature( QgsVectorLayer* vlayer );

    /**
     * Creates rubberbands for moving points
     */
    void createMovingRubberBands();

    /**
     * Creates rubber bands for ther features when topology editing is enabled
     * @param vlayer vector layer for ehich rubber bands are created
     * @param vertexMap map of vertexes
     * @param vertex currently processed vertex
     */
    void createTopologyRubberBands( QgsVectorLayer* vlayer, const QList<VertexEntry*> &vertexMap, int vertex );

    /** The position of the vertex to move (in map coordinates) to exclude later from snapping*/
    QList<QgsPoint> mExcludePoint;

    /** rubber bands */
    QList<QgsRubberBand*> mRubberBands;

    /** list of topology rubber bands */
    QList<QgsRubberBand*> mTopologyRubberBand;

    /** vertexes of rubberbands which are to be moved */
    QMap<QgsFeatureId, Vertexes*> mTopologyMovingVertexes;

    /** vertexes of features with int id which were already added tu rubber bands */
    QMap<QgsFeatureId, Vertexes*> mTopologyRubberBandVertexes;

    /** object containing selected feature and it's vertexes */
    SelectionFeature* mSelectionFeature;

    /** flag if selection rectangle is active */
    bool mSelectionRectangle;

    /** flag if moving of vertexes is occuring */
    bool mMoving;

    /** flag if click action is still in queue to be processed */
    bool mClicked;

    /** flag if crtl is pressed */
    bool mCtrl;

    /** flag if selection of another feature can occur */
    bool mSelectAnother;

    /** feature id of another feature where user clicked */
    QgsFeatureId mAnother;

    /** stored position of last press down action to count how much vertexes should be moved */
    QgsPoint* mLastCoordinates;

    /** closest vertex to click */
    QgsPoint mClosestVertex;

    /** backup of map coordinates to be able to count change between moves */
    QgsPoint mPosMapCoordBackup;

    /** active rubberband for selecting vertexes */
    QRubberBand* mQRubberBand;

    /** rectangle defining area for selecting vertexes */
    QRect* mRect;

    /** flag that tells that tool is currently updating feature to do not act on change signal */
    bool mChangingGeometry;

    /** flag to tell if edition points */
    bool mIsPoint;
};

#endif
