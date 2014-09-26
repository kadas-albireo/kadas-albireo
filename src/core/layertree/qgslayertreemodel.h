/***************************************************************************
  qgslayertreemodel.h
  --------------------------------------
  Date                 : May 2014
  Copyright            : (C) 2014 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSLAYERTREEMODEL_H
#define QGSLAYERTREEMODEL_H

#include <QAbstractItemModel>
#include <QFont>
#include <QIcon>

class QgsLayerTreeNode;
class QgsLayerTreeGroup;
class QgsLayerTreeLayer;
class QgsLayerTreeModelLegendNode;
class QgsMapHitTest;
class QgsMapLayer;
class QgsMapSettings;


/**
 * The QgsLayerTreeModel class is model implementation for Qt item views framework.
 * The model can be used in any QTreeView, it is however recommended to use it
 * with QgsLayerTreeView which brings additional functionality specific to layer tree handling.
 *
 * The model listens to the changes in the layer tree and signals the changes as appropriate,
 * so that any view that uses the model is updated accordingly.
 *
 * Behavior of the model can be customized with flags. For example, whether to show legend or
 * whether to allow changes to the layer tree.
 *
 * @see QgsLayerTreeView
 * @note added in 2.4
 */
class CORE_EXPORT QgsLayerTreeModel : public QAbstractItemModel
{
    Q_OBJECT
  public:
    //! Construct a new tree model with given layer tree (root node must not be null pointer).
    //! The root node is not transferred by the model.
    explicit QgsLayerTreeModel( QgsLayerTreeGroup* rootNode, QObject *parent = 0 );
    ~QgsLayerTreeModel();

    // Implementation of virtual functions from QAbstractItemModel

    int rowCount( const QModelIndex &parent = QModelIndex() ) const;
    int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    QModelIndex parent( const QModelIndex &child ) const;
    QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags( const QModelIndex &index ) const;
    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
    Qt::DropActions supportedDropActions() const;
    QStringList mimeTypes() const;
    QMimeData* mimeData( const QModelIndexList& indexes ) const;
    bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );
    bool removeRows( int row, int count, const QModelIndex& parent = QModelIndex() );

    // New stuff

    enum Flag
    {
      // display flags
      ShowLegend                = 0x0001,  //!< Add legend nodes for layer nodes
      ShowSymbology             = 0x0001,  //!< deprecated - use ShowLegend
      ShowRasterPreviewIcon     = 0x0002,  //!< Will use real preview of raster layer as icon (may be slow)

      // behavioral flags
      AllowNodeReorder          = 0x1000,  //!< Allow reordering with drag'n'drop
      AllowNodeRename           = 0x2000,  //!< Allow renaming of groups and layers
      AllowNodeChangeVisibility = 0x4000,  //!< Allow user to set node visibility with a check box
      AllowLegendChangeState    = 0x8000,  //!< Allow check boxes for legend nodes (if supported by layer's legend)
      AllowSymbologyChangeState = 0x8000,  //!< deprecated - use AllowLegendChangeState
    };
    Q_DECLARE_FLAGS( Flags, Flag )

    //! Set OR-ed combination of model flags
    void setFlags( Flags f );
    //! Enable or disable a model flag
    void setFlag( Flag f, bool on = true );
    //! Return OR-ed combination of model flags
    Flags flags() const;
    //! Check whether a flag is enabled
    bool testFlag( Flag f ) const;

    //! Return layer tree node for given index. Returns root node for invalid index.
    //! Returns null pointer if index does not refer to a layer tree node (e.g. it is a legend node)
    QgsLayerTreeNode* index2node( const QModelIndex& index ) const;
    //! Return index for a given node. If the node does not belong to the layer tree, the result is undefined
    QModelIndex node2index( QgsLayerTreeNode* node ) const;
    //! Convert a list of indexes to a list of layer tree nodes.
    //! Indices that do not represent layer tree nodes are skipped.
    //! @arg skipInternal If true, a node is included in the output list only if no parent node is in the list
    QList<QgsLayerTreeNode*> indexes2nodes( const QModelIndexList& list, bool skipInternal = false ) const;

    //! Return legend node for given index. Returns null for invalid index
    //! @note added in 2.6
    static QgsLayerTreeModelLegendNode* index2legendNode( const QModelIndex& index );
    //! Return index for a given legend node. If the legend node does not belong to the layer tree, the result is undefined.
    //! If the legend node is belongs to the tree but it is filtered out, invalid model index is returned.
    //! @note added in 2.6
    QModelIndex legendNode2index( QgsLayerTreeModelLegendNode* legendNode );

    //! Return list of legend nodes attached to a particular layer node
    //! @note added in 2.6
    QList<QgsLayerTreeModelLegendNode*> layerLegendNodes( QgsLayerTreeLayer* nodeLayer );

    //! Return pointer to the root node of the layer tree. Always a non-null pointer.
    QgsLayerTreeGroup* rootGroup() const;
    //! Reset the model and use a new root group node
    //! @note added in 2.6
    void setRootGroup( QgsLayerTreeGroup* newRootGroup );

    //! Force a refresh of legend nodes of a layer node.
    //! Not necessary to call when layer's renderer is changed as the model listens to these events.
    void refreshLayerLegend( QgsLayerTreeLayer* nodeLayer );

    //! Get index of the item marked as current. Item marked as current is underlined.
    QModelIndex currentIndex() const;
    //! Set index of the current item. May be used by view. Item marked as current is underlined.
    void setCurrentIndex( const QModelIndex& currentIndex );

    //! Set font for a particular type of layer tree node. nodeType should come from QgsLayerTreeNode::NodeType enumeration
    void setLayerTreeNodeFont( int nodeType, const QFont& font );
    //! Get font for a particular type of layer tree node. nodeType should come from QgsLayerTreeNode::NodeType enumeration
    QFont layerTreeNodeFont( int nodeType ) const;

    //! Set at what number of legend nodes the layer node should be collapsed. Setting -1 disables the auto-collapse (default).
    void setAutoCollapseLegendNodes( int nodeCount ) { mAutoCollapseLegendNodesCount = nodeCount; }
    //! Return at what number of legend nodes the layer node should be collapsed. -1 means no auto-collapse (default).
    int autoCollapseLegendNodes() const { return mAutoCollapseLegendNodesCount; }

    //! Force only display of legend nodes which are valid for given scale denominator.
    //! Setting value <= 0 will disable the functionality
    //! @note added in 2.6
    void setLegendFilterByScale( double scaleDenominator );
    double legendFilterByScale() const { return mLegendFilterByScale; }

    //! Force only display of legend nodes which are valid for given map settings.
    //! Setting null pointer or invalid map settings will disable the functionality.
    //! Ownership of map settings pointer does not change.
    //! @note added in 2.6
    void setLegendFilterByMap( const QgsMapSettings* settings );
    const QgsMapSettings* legendFilterByMap() const { return mLegendFilterByMapSettings.data(); }

    //! Give the layer tree model hints about the currently associated map view
    //! so that legend nodes that use map units can be scaled currectly
    //! @note added in 2.6
    void setLegendMapViewData( double mapUnitsPerPixel, int dpi, double scale );
    //! Get hints about map view - to be used in legend nodes. Arguments that are not null will receive values.
    //! If there are no valid map view data (from previous call to setLegendMapViewData()), returned values are zeros.
    //! @note added in 2.6
    void legendMapViewData( double *mapUnitsPerPixel, int *dpi, double *scale );

    //! Return true if index represents a legend node (instead of layer node)
    //! @deprecated use index2legendNode()
    Q_DECL_DEPRECATED bool isIndexSymbologyNode( const QModelIndex& index ) const;
    //! Return layer node to which a legend node belongs to. Returns null pointer if index is not a legend node.
    //! @deprecated use index2legendNode()->parent()
    Q_DECL_DEPRECATED QgsLayerTreeLayer* layerNodeForSymbologyNode( const QModelIndex& index ) const;
    //! @deprecated use refreshLayerLegend()
    Q_DECL_DEPRECATED void refreshLayerSymbology( QgsLayerTreeLayer* nodeLayer ) { refreshLayerLegend( nodeLayer ); }
    //! @deprecated use setAutoCollapseLegendNodes()
    Q_DECL_DEPRECATED void setAutoCollapseSymbologyNodes( int nodeCount ) { setAutoCollapseLegendNodes( nodeCount ); }
    //! @deprecated use autoCollapseLegendNodes()
    Q_DECL_DEPRECATED int autoCollapseSymbologyNodes() const { return autoCollapseLegendNodes(); }

  signals:

  protected slots:
    void nodeWillAddChildren( QgsLayerTreeNode* node, int indexFrom, int indexTo );
    void nodeAddedChildren( QgsLayerTreeNode* node, int indexFrom, int indexTo );
    void nodeWillRemoveChildren( QgsLayerTreeNode* node, int indexFrom, int indexTo );
    void nodeRemovedChildren();

    void nodeVisibilityChanged( QgsLayerTreeNode* node );

    void nodeCustomPropertyChanged( QgsLayerTreeNode* node, const QString& key );

    void nodeLayerLoaded();
    void nodeLayerWillBeUnloaded();
    void layerLegendChanged();

    void layerNeedsUpdate();

    void legendNodeDataChanged();

  protected:
    void removeLegendFromLayer( QgsLayerTreeLayer* nodeLayer );
    void addLegendToLayer( QgsLayerTreeLayer* nodeL );

    void connectToLayer( QgsLayerTreeLayer* nodeLayer );
    void disconnectFromLayer( QgsLayerTreeLayer* nodeLayer );

    void connectToLayers( QgsLayerTreeGroup* parentGroup );
    void disconnectFromLayers( QgsLayerTreeGroup* parentGroup );
    void connectToRootNode();
    void disconnectFromRootNode();

    //! emit dataChanged() for layer tree node items
    void recursivelyEmitDataChanged( const QModelIndex& index = QModelIndex() );

    static const QIcon& iconGroup();

    //! Filter nodes from QgsMapLayerLegend according to the current filtering rules
    QList<QgsLayerTreeModelLegendNode*> filterLegendNodes( const QList<QgsLayerTreeModelLegendNode*>& nodes );

  protected:
    //! Pointer to the root node of the layer tree. Not owned by the model
    QgsLayerTreeGroup* mRootNode;
    //! Set of flags for the model
    Flags mFlags;
    //! Active legend nodes for each layer node. May have been filtered.
    //! Owner of legend nodes is still mOriginalLegendNodes !
    QMap<QgsLayerTreeLayer*, QList<QgsLayerTreeModelLegendNode*> > mLegendNodes;
    //! Data structure for storage of legend nodes for each layer.
    //! These are nodes as received from QgsMapLayerLegend
    QMap<QgsLayerTreeLayer*, QList<QgsLayerTreeModelLegendNode*> > mOriginalLegendNodes;
    //! Current index - will be underlined
    QPersistentModelIndex mCurrentIndex;
    //! Minimal number of nodes when legend should be automatically collapsed. -1 = disabled
    int mAutoCollapseLegendNodesCount;

    QFont mFontLayer;
    QFont mFontGroup;

    //! scale denominator for filtering of legend nodes (<= 0 means no filtering)
    double mLegendFilterByScale;

    QScopedPointer<QgsMapSettings> mLegendFilterByMapSettings;
    QScopedPointer<QgsMapHitTest> mLegendFilterByMapHitTest;

    double mLegendMapViewMupp;
    int mLegendMapViewDpi;
    double mLegendMapViewScale;
};

Q_DECLARE_OPERATORS_FOR_FLAGS( QgsLayerTreeModel::Flags )

#endif // QGSLAYERTREEMODEL_H
