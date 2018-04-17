
class QgsPluginLayer : QgsMapLayer
{
%TypeHeaderCode
#include "qgspluginlayer.h"
%End

  public:
    QgsPluginLayer( QString layerType, QString layerName = QString() );

    /** return plugin layer type (the same as used in QgsPluginLayerRegistry) */
    QString pluginLayerType();

    void setExtent( const QgsRectangle &extent );

    //! return a list of symbology items for the legend
    //! (defult implementation returns nothing)
    //! @note Added in v2.1
    virtual QgsLegendSymbologyList legendSymbologyItems( const QSize& iconSize );

    /** Return new instance of QgsMapLayerRenderer that will be used for rendering of given context
     *
     * The default implementation returns map layer renderer which just calls draw().
     * This may work, but it is unsafe for multi-threaded rendering because of the run
     * conditions that may happen (e.g. something is changed in the layer while it is
     * being rendered).
     *
     * @note added in 2.4
     */
    virtual QgsMapLayerRenderer* createMapRenderer( QgsRenderContext& rendererContext ) /Factory/;

    /** Test for mouse pick. */
    virtual bool testPick(const QgsPoint& mapPos, const QgsMapSettings& mapSettings, QVariant& pickResult, QRect &pickResultsExtent );
    /** Handle a pick result. */
    virtual void handlePick(const QVariant& pick);
    /** Get items in extent */
    virtual QVariantList getItems(const QgsRectangle& /*extent*/) const;
    /** Copy or cut the specified items */
    virtual void copyItems(const QVariantList& /*items*/, bool /*cut*/);
    /** Delete the specified items */
    virtual void deleteItems(const QVariantList& /*items*/);

    /** return  the current layer transparency */
    virtual int layerTransparency() const;
    /** set the layer transparency */
    virtual void setLayerTransparency(int /*value*/);

    /** Get attributes at the specified map location */
    class IdentifyResult {
      %TypeHeaderCode
      #include "qgspluginlayer.h"
      %End
    public:
      IdentifyResult(const QString& featureName, const QMap<QString, QVariant>& attributes, QgsAbstractGeometryV2* geom);
      const QString& featureName() const;
      const QMap<QString, QVariant>& attributes() const;
      const QgsGeometry& geometry() const;
    };
    virtual QList<QgsPluginLayer::IdentifyResult> identify( const QgsPoint& /*mapPos*/, const QgsMapSettings& /*mapSettings*/);
};
