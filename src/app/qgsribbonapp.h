#ifndef QGSRIBBONAPP_H
#define QGSRIBBONAPP_H

#include "ui_qgsribbonappbase.h"
#include <QSslError>

class QgsClipboard;
class QgsDecorationItem;
class QgsLayerTreeView;
class QgsLayerTreeMapCanvasBridge;
class QgsMessageBar;
class QNetworkReply;
class QgsVBSCoordinateDisplayer;
class QgsVBSHillshadeTool;
class QgsVBSSlopeTool;
class QgsVBSViewshedTool;
class QgsVectorLayerTools;

class QgsRibbonApp: public QWidget, private Ui::QgsRibbonAppBase
{
    Q_OBJECT
  public:
    QgsRibbonApp( QWidget * parent = 0, Qt::WindowFlags f = 0 );
    ~QgsRibbonApp();

    QgsMapCanvas* mapCanvas() { return mMapCanvas; }
    /** Return the messageBar object which allows displaying unobtrusive messages to the user.*/
    QgsMessageBar *messageBar() { return mInfoBar; }
    /** Get timeout for timed messages: default of 5 seconds */
    int messageTimeout();
    //! mark project dirty
    void markDirty();

    QgsVectorLayerTools* vectorLayerTools() { return mVectorLayerTools; };

    //! Returns a pointer to the internal clipboard
    QgsClipboard* clipboard() { return mInternalClipboard; }

    Ui::QgsRibbonAppBase* getUi() { return this; }

  public slots:
    //! open the properties dialog for the currently selected layer
    void layerProperties();
    //! show the attribute table for the currently selected layer
    void attributeTable();

    //! starts/stops editing mode of the current layer
    void toggleEditing();
    //! starts/stops editing mode of a layer
    bool toggleEditing( QgsMapLayer *layer, bool allowCancel = true );
    //! Remove layers from the map and legend
    void removeLayer();

    /** Save edits of a layer
     * @param leaveEditable leave the layer in editing mode when done
     * @param triggerRepaint send layer signal to repaint canvas when done
     */
    void saveEdits( QgsMapLayer *layer, bool leaveEditable = true, bool triggerRepaint = true );
    //! copies selected features on the active layer to the clipboard
    /**
       \param layerContainingSelection  The layer that the selection will be taken from
                                        (defaults to the active layer on the legend)
     */
    void editCopy( QgsMapLayer *layerContainingSelection = 0 );

    /**Deletes the selected attributes for the currently selected vector layer*/
    void deleteSelected( QgsMapLayer *layer = 0, QWidget *parent = 0, bool promptConfirmation = false );

  signals:
    void newProject();

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
    void saveMapAsImage();
    void kmlExport();
    void zoomToPrevious();
    void zoomToNext();
    void pin( bool enabled );
    void profile( bool enabled );
    void distance( bool enabled );
    void area( bool enabled );
    void azimuth( bool enabled );
    void circle( bool enabled );
    void slope( bool enabled );
    void hillshade( bool enabled );
    void viewshed( bool enabled );

    //! project was read
    void readProject( const QDomDocument & );
    void on_mLayerTreeViewButton_clicked();
    void on_mZoomInButton_clicked();
    void on_mZoomOutButton_clicked();
    void layerTreeViewDoubleClicked( const QModelIndex& index );

    void pinActionToggled( bool enabled );
    void userScale();
    void showScale( double theScale );

    void setNonEditMapTool();
    void mapToolChanged( QgsMapTool* newTool, QgsMapTool* oldTool );

#ifndef QT_NO_OPENSSL
    void namConfirmSslErrors( const QUrl &url, const QList<QSslError> &errors, bool* ok );
#endif
    void namRequestTimedOut( QNetworkReply *reply );
    void renderDecorationItems( QPainter *p );
    void projectReadDecorationItems();

  private:
    void setActionToButton( QAction* action, QToolButton* button );

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
    //! initialize network manager
    void namSetup();
    void namUpdate();
    void initLayerTreeView();
    void createDecorations();

    void showLayerProperties( QgsMapLayer *ml );
    QgsMapLayer *activeLayer();

    /** Alerts user when commit errors occured */
    void commitError( QgsVectorLayer *vlayer );

    void performDrag( const QIcon* icon = 0 );

    void restoreFavoriteButton( QToolButton* button );

    void configureButtons();

    void fileNew( bool thePromptToSaveFlag, bool forceBlank = false );
    //! Create a new file from a template project
    void fileNewFromDefaultTemplate();
    bool fileNewFromTemplate( QString fileName );

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

    QList<QgsDecorationItem*> mDecorationItems;

    QButtonGroup* mToggleButtonGroup;

    QgsVBSSlopeTool* mSlopeTool;
    QgsVBSHillshadeTool* mHillshadeTool;
    QgsVBSViewshedTool* mViewshedTool;

    class Tools
    {
      public:

        Tools()
            : mZoomIn( 0 )
            , mZoomOut( 0 )
            , mPan( 0 )
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

    //! Flag to indicate an edits save/rollback for active layer is in progress
    bool mSaveRollbackInProgress;

    QgsVectorLayerTools* mVectorLayerTools;

    /** QGIS-internal vector feature clipboard */
    QgsClipboard* mInternalClipboard;
};

#endif // QGSRIBBONAPP_H
