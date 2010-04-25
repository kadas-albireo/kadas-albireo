#ifndef QGSSINGLESYMBOLRENDERERV2_H
#define QGSSINGLESYMBOLRENDERERV2_H

#include "qgsrendererv2.h"

class CORE_EXPORT QgsSingleSymbolRendererV2 : public QgsFeatureRendererV2
{
  public:

    QgsSingleSymbolRendererV2( QgsSymbolV2* symbol );

    virtual ~QgsSingleSymbolRendererV2();

    virtual QgsSymbolV2* symbolForFeature( QgsFeature& feature );

    virtual void startRender( QgsRenderContext& context, const QgsVectorLayer *vlayer );

    virtual void stopRender( QgsRenderContext& context );

    virtual QList<QString> usedAttributes();

    QgsSymbolV2* symbol() const;
    void setSymbol( QgsSymbolV2* s );

    void setRotationField( QString fieldName ) { mRotationField = fieldName; }
    QString rotationField() const { return mRotationField; }

    void setSizeScaleField( QString fieldName ) { mSizeScaleField = fieldName; }
    QString sizeScaleField() const { return mSizeScaleField; }

    virtual QString dump();

    virtual QgsFeatureRendererV2* clone();

    virtual QgsSymbolV2List symbols();

    //! create renderer from XML element
    static QgsFeatureRendererV2* create( QDomElement& element );

    //! store renderer info to XML element
    virtual QDomElement save( QDomDocument& doc );

    //! return a list of symbology items for the legend
    virtual QgsLegendSymbologyList legendSymbologyItems( QSize iconSize );

    //! return a list of item text / symbol
    //! @note: this method was added in version 1.5
    virtual QgsLegendSymbolList legendSymbolItems();

  protected:
    QgsSymbolV2* mSymbol;
    QString mRotationField;
    QString mSizeScaleField;

    // temporary stuff for rendering
    int mRotationFieldIdx, mSizeScaleFieldIdx;
    QgsSymbolV2* mTempSymbol;
    double mOrigSize;
};


#endif // QGSSINGLESYMBOLRENDERERV2_H
