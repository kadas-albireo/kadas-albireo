class QgsVBSMilixItem
{
%TypeHeaderCode
#include <milix/qgsvbsmilixlayer.h>
%End
  public:
    static bool validateMssString(const QString& mssString, QString &adjustedMssString /Out/, QString& messages /Out/);

    QgsVBSMilixItem();
    void initialize( const QString& mssString, const QString& militaryName, const QList<QgsPoint> &points, const QList<int>& controlPoints = QList<int>(), const QPoint& userOffset = QPoint(), bool queryControlPoints = true );
    const QString& mssString() const;
    const QString& militaryName() const;
    const QList<QgsPoint>& points() const;
    QList<QPoint> screenPoints( const QgsMapToPixel& mapToPixel, const QgsCoordinateTransform *crst ) const;
    const QList<int>& controlPoints() const;
    const QPoint& userOffset() const;
    bool isMultiPoint() const;

    void writeMilx( QDomDocument& doc, QDomElement& graphicListEl, const QString &versionTag, QString &messages ) const;
    void readMilx( const QDomElement& graphicEl, const QString &symbolXml, const QgsCoordinateTransform *crst, int symbolSize );
};

/** \class QgsVBSMilixLayer
    \brief Milix layer
*/
class QgsVBSMilixLayer : public QgsPluginLayer
{
%TypeHeaderCode
#include <milix/qgsvbsmilixlayer.h>
%End
  public:
    static QString layerTypeKey();

    QgsVBSMilixLayer( const QString& name = "MilX" );
    ~QgsVBSMilixLayer();
    void addItem( QgsVBSMilixItem* item /Transfer/ );
    void removeItem( int idx );
    const QList<QgsVBSMilixItem*>& items() const;
    QgsLegendSymbologyList legendSymbologyItems( const QSize& iconSize );
    void exportToMilxly( QDomElement &milxDocumentEl, const QString &versionTag, QStringList& exportMessages );
    bool importMilxly( QDomElement &milxLayerEl, const QString &fileMssVer, QString &errorMsg, QStringList& importMessages );
    bool writeSymbology( QDomNode &/*node*/, QDomDocument& /*doc*/, QString& /*errorMessage*/ ) const;
    bool readSymbology( const QDomNode &/*node*/, QString &/*errorMessage*/ );
    QgsMapLayerRenderer* createMapRenderer( QgsRenderContext& rendererContext );
    QgsRectangle extent();

    bool testPick( const QgsPoint& mapPos, const QgsMapSettings& mapSettings, QVariant& pickResult );
    void handlePick( const QVariant& pick );

  signals:
    void symbolPicked( int symbolIdx );

  protected:
    bool readXml( const QDomNode& layer_node );
    bool writeXml( QDomNode & layer_node, QDomDocument & document );
};

class QgsVBSMilixLayerType : public QgsPluginLayerType
{
%TypeHeaderCode
#include <milix/qgsvbsmilixlayer.h>
%End
  public:
    QgsVBSMilixLayerType();
    QgsPluginLayer* createLayer();
    bool showLayerProperties( QgsPluginLayer* /*layer*/ );
};
