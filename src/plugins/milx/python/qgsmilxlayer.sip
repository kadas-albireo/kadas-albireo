class QgsMilXItem
{
%TypeHeaderCode
#include <qgsmilxlayer.h>
%End
  public:
    static bool validateMssString(const QString& mssString, QString &adjustedMssString /Out/, QString& messages /Out/);

    QgsMilXItem();
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

class QgsMilXLayer : public QgsPluginLayer
{
%TypeHeaderCode
#include <qgsmilxlayer.h>
%End
  public:
    static QString layerTypeKey();

    QgsMilXLayer( const QString& name = "MilX" );
    ~QgsMilXLayer();
    void addItem( QgsMilXItem* item /Transfer/ );
    QgsMilXItem* takeItem( int idx );
    const QList<QgsMilXItem*>& items() const;
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

class QgsMilXLayerType : public QgsPluginLayerType
{
%TypeHeaderCode
#include <qgsmilxlayer.h>
%End
  public:
    QgsMilXLayerType();
    QgsPluginLayer* createLayer();
    bool showLayerProperties( QgsPluginLayer* /*layer*/ );
};
