
class QgsPluginLayerType
{
%TypeHeaderCode
#include "qgspluginlayerregistry.h"
%End
  public:

    QgsPluginLayerType( QString name );
    virtual ~QgsPluginLayerType();

    QString name();

    /** return new layer of this type. Return NULL on error */
    virtual QgsPluginLayer* createLayer() /Factory/;

    /** Resturns whether the layer has a properties dialog: 0 = no, 1 = yes, -1 = unknown
      * Note: the yes/no/unknown return is because this function was introduced after
      * showLayerProperties, which was previously used to handle this by returning false
      * if there are no layer properties. Returning -1 by default here does not break
      * plugin layers which do not yet support this method. */
    virtual int hasLayerProperties() const;

    /** show plugin layer properties dialog. Return false if the dialog cannot be shown. */
    virtual bool showLayerProperties( QgsPluginLayer* layer );

};

//=============================================================================

/** \ingroup core
    a registry of plugin layers types
*/
class QgsPluginLayerRegistry
{
%TypeHeaderCode
#include "qgspluginlayerregistry.h"
%End

  public:

    /** means of accessing canonical single instance  */
    static QgsPluginLayerRegistry* instance();

    ~QgsPluginLayerRegistry();

    /** list all known layer types
     *  \note added in v2.1 */
    QStringList pluginLayerTypes();

    /** add plugin layer type (take ownership) and return true on success */
    bool addPluginLayerType( QgsPluginLayerType* pluginLayerType /Transfer/ );

    /** remove plugin layer type and return true on success */
    bool removePluginLayerType( QString typeName );

    /** return plugin layer type metadata or NULL if doesn't exist */
    QgsPluginLayerType* pluginLayerType( QString typeName );

    /** return new layer if corresponding plugin has been found, else return NULL */
    QgsPluginLayer* createLayer( QString typeName ) /Factory/;

  private:

    /** private since instance() creates it */
    QgsPluginLayerRegistry();
};
