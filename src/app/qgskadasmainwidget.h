#ifndef QGSKADASMAINWIDGET_H
#define QGSKADASMAINWIDGET_H

#include "ui_qgskadasmainwidgetbase.h"

class QgsLayerTreeView;
class QgsLayerTreeMapCanvasBridge;
class QgsMessageBar;
class QgsVBSCoordinateDisplayer;

class QgsKadasMainWidget: public QWidget, private Ui::QgsKadasMainWidgetBase
{
    Q_OBJECT
  public:
    QgsKadasMainWidget( QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsKadasMainWidget();

    QgsMapCanvas* mapCanvas() { return mMapCanvas; }
    /** Return the messageBar object which allows displaying unobtrusive messages to the user.*/
    QgsMessageBar *messageBar() { return mInfoBar; }
    /** Get timeout for timed messages: default of 5 seconds */
    int messageTimeout();
    //! mark project dirty
    void markDirty();

  public slots:
    //! open the properties dialog for the currently selected layer
    void layerProperties();
    //! show the attribute table for the currently selected layer
    void attributeTable();

    //! starts/stops editing mode of the current layer
    void toggleEditing();
    //! starts/stops editing mode of a layer
    bool toggleEditing( QgsMapLayer *layer, bool allowCancel = true );

  protected:
    virtual void resizeEvent( QResizeEvent* event );
    virtual void mousePressEvent( QMouseEvent* event );
    virtual void mouseMoveEvent( QMouseEvent* event );
    virtual void dropEvent( QDropEvent* event );
    virtual void dragEnterEvent( QDragEnterEvent* event );

  private slots:
    void addToFavorites();
    void fileNew();
    void open();
    bool save();
    void pin( bool enabled );
    void profile( bool enabled );
    void distance( bool enabled );
    void area( bool enabled );
    void azimuth( bool enabled );
    void circle( bool enabled );

    //! project was read
    void readProject( const QDomDocument & );
    void on_mLayerTreeViewButton_clicked();
    void on_mZoomInButton_clicked();
    void on_mZoomOutButton_clicked();
    void layerTreeViewDoubleClicked( const QModelIndex& index );

    void pinActionToggled( bool enabled );
    void userScale();

  private:
    void setActionToButton( QAction* action, QAbstractButton* button );

    bool addProject( const QString& projectFile );
    //! check to see if file is dirty and if so, prompt the user th save it
    bool saveDirty();
    //! Save project. Returns true if the user selected a file to save to, false if not.
    bool fileSave();
    /** add this file to the recently opened/saved projects list
     *  pass settings by reference since creating more than one
     * instance simultaneously results in data loss.
     */
    void saveRecentProjectPath( QString projectPath, QSettings & settings );
    //! Update project menu with the current list of recently accessed projects
    void updateRecentProjectPaths();
    //! clear out any stuff from project
    void closeProject();
    void removeAllLayers();
    void createCanvasTools();
    /** Activates or deactivates actions depending on the current maplayer type.
    Is called from the legend when the current legend item has changed*/
    void activateDeactivateLayerRelatedActions( QgsMapLayer *layer );
    void initMapCanvas();
    void initLayerTreeView();

    void showLayerProperties( QgsMapLayer *ml );
    QgsMapLayer *activeLayer();

    /** Alerts user when commit errors occured */
    void commitError( QgsVectorLayer *vlayer );

    void performDrag( const QIcon* icon = 0 );

    void restoreFavoriteButton( QAbstractButton* button );

    //! list of recently opened/saved project files
    QStringList mRecentProjectPaths;
    //! Helper class that connects layer tree with map canvas
    QgsLayerTreeMapCanvasBridge* mLayerTreeCanvasBridge;

    QgsMapTool* mNonEditMapTool;

    //! a bar to display warnings in a non-blocker manner
    QgsMessageBar* mInfoBar;

    QgsVBSCoordinateDisplayer* mCoordinateDisplayer;

    QPoint mDragStartPos;
    QString mDragStartActionName;

    class Tools
    {
      public:

        Tools()
            : mZoomIn( 0 )
            , mZoomOut( 0 )
            , mPan( 0 )
#ifdef HAVE_TOUCH
            , mTouch( 0 )
#endif
            , mIdentify( 0 )
            , mFeatureAction( 0 )
            , mMeasureDist( 0 )
            , mMeasureArea( 0 )
            , mMeasureAngle( 0 )
            , mAddFeature( 0 )
            , mMoveFeature( 0 )
            , mOffsetCurve( 0 )
            , mReshapeFeatures( 0 )
            , mSplitFeatures( 0 )
            , mSplitParts( 0 )
            , mSelect( 0 )
            , mSelectFeatures( 0 )
            , mSelectPolygon( 0 )
            , mSelectFreehand( 0 )
            , mSelectRadius( 0 )
            , mVertexAdd( 0 )
            , mVertexMove( 0 )
            , mVertexDelete( 0 )
            , mAddRing( 0 )
            , mFillRing( 0 )
            , mAddPart( 0 )
            , mSimplifyFeature( 0 )
            , mDeleteRing( 0 )
            , mDeletePart( 0 )
            , mNodeTool( 0 )
            , mRotatePointSymbolsTool( 0 )
            , mAnnotation( 0 )
            , mFormAnnotation( 0 )
            , mHtmlAnnotation( 0 )
            , mSvgAnnotation( 0 )
            , mTextAnnotation( 0 )
            , mPinLabels( 0 )
            , mShowHideLabels( 0 )
            , mMoveLabel( 0 )
            , mRotateFeature( 0 )
            , mRotateLabel( 0 )
            , mChangeLabelProperties( 0 )
        {}

        QgsMapTool *mZoomIn;
        QgsMapTool *mZoomOut;
        QgsMapTool *mPan;
#ifdef HAVE_TOUCH
        QgsMapTool *mTouch;
#endif
        QgsMapTool *mIdentify;
        QgsMapTool *mFeatureAction;
        QgsMapTool *mMeasureDist;
        QgsMapTool *mMeasureArea;
        QgsMapTool *mMeasureCircle;
        QgsMapTool *mMeasureHeightProfile;
        QgsMapTool *mMeasureAngle;
        QgsMapTool *mAddFeature;
        QgsMapTool *mCircularStringCurvePoint;
        QgsMapTool *mCircularStringRadius;
        QgsMapTool *mMoveFeature;
        QgsMapTool *mOffsetCurve;
        QgsMapTool *mReshapeFeatures;
        QgsMapTool *mSplitFeatures;
        QgsMapTool *mSplitParts;
        QgsMapTool *mSelect;
        QgsMapTool *mSelectFeatures;
        QgsMapTool *mSelectPolygon;
        QgsMapTool *mSelectFreehand;
        QgsMapTool *mSelectRadius;
        QgsMapTool *mVertexAdd;
        QgsMapTool *mVertexMove;
        QgsMapTool *mVertexDelete;
        QgsMapTool *mAddRing;
        QgsMapTool *mFillRing;
        QgsMapTool *mAddPart;
        QgsMapTool *mSimplifyFeature;
        QgsMapTool *mDeleteRing;
        QgsMapTool *mDeletePart;
        QgsMapTool *mNodeTool;
        QgsMapTool *mRotatePointSymbolsTool;
        QgsMapTool *mAnnotation;
        QgsMapTool *mFormAnnotation;
        QgsMapTool *mHtmlAnnotation;
        QgsMapTool *mSvgAnnotation;
        QgsMapTool *mTextAnnotation;
        QgsMapTool *mPinLabels;
        QgsMapTool *mShowHideLabels;
        QgsMapTool *mMoveLabel;
        QgsMapTool *mRotateFeature;
        QgsMapTool *mRotateLabel;
        QgsMapTool *mChangeLabelProperties;
        QgsMapTool *mMapToolPinAnnotation;
    } mMapTools;
};

#endif // QGSKADASMAINWIDGET_H
