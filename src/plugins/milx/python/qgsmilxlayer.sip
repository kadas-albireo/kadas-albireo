class QgsMilXItem
{
%TypeHeaderCode
#include <qgsmilxlayer.h>
%End
  public:
    enum ControlPointState {
      HAVE_CONTROL_POINTS,
      NEED_CONTROL_POINTS_AND_INDICES,
      NEED_CONTROL_POINT_INDICES,
    };
    static bool validateMssString(const QString& mssString, QString &adjustedMssString /Out/, QString& messages /Out/);

    QgsMilXItem();
    void initialize(const QString& mssString, const QString& militaryName, const QList<QgsPoint> &points, ControlPointState controlPointState );
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

    QgsMilXLayer( QgsLayerTreeViewMenuProvider *menuProvider, const QString& name = "MilX" );
    // This second variant is just because for some reason SIP mishandles QgsLayerTreeViewMenuProvider and the first variant is unusable from python...
    QgsMilXLayer( QgisInterface* iface, const QString& name = "MilX" );
    ~QgsMilXLayer();
    void addItem( QgsMilXItem* item /Transfer/ );
    QgsMilXItem* takeItem( int idx );
    const QList<QgsMilXItem*>& items() const;
    QgsLegendSymbologyList legendSymbologyItems( const QSize& iconSize );
    void exportToMilxly( QDomElement &milxDocumentEl, const QString &versionTag, int dpi, QStringList& exportMessages );
    bool importMilxly( QDomElement &milxLayerEl, const QString &fileMssVer, int dpi, QString &errorMsg, QStringList& importMessages );
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
