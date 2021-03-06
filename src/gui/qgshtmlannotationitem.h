/***************************************************************************
                              qgshtmlannotationitem.h
                              ------------------------
  begin                : February 9, 2010
  copyright            : (C) 2010 by Marco Hugentobler
  email                : marco dot hugentobler at hugis dot net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSHTMLANNOTATIONITEM_H
#define QGSHTMLANNOTATIONITEM_H

#include "qgsannotationitem.h"
#include "qgsfeature.h"
#include <QObject>
#include <QWebView>
#include <QWebFrame>

class QGraphicsProxyWidget;

/**An annotation item that embedds a designer form showing the feature attribute*/
class GUI_EXPORT QgsHtmlAnnotationItem: public QgsAnnotationItem
{
    Q_OBJECT
    QGS_ANNOTATION_ITEM( QgsHtmlAnnotationItem, "HtmlAnnotationItem" )
  public:
    QgsHtmlAnnotationItem( QgsMapCanvas* canvas, QgsVectorLayer* vlayer = 0, bool hasFeature = false, int feature = 0 );
    ~QgsHtmlAnnotationItem();

    virtual QgsHtmlAnnotationItem* clone( QgsMapCanvas *canvas ) override { return new QgsHtmlAnnotationItem( canvas, this ); }

    void paint( QPainter * painter ) override;

    //! paint function called by map canvas
    void paint( QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 ) override;

    QSizeF minimumFrameSize() const override;

    /**Reimplemented from QgsAnnotationItem*/
    void setMapPosition( const QgsPoint& pos, const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) override;

    void setHTMLPage( const QString& htmlFile );
    QString htmlPage() const { return mHtmlFile; }

    void writeXML( QDomDocument& doc ) const override;
    void readXML( const QDomDocument& doc, const QDomElement& itemElem ) override;

    QgsVectorLayer* vectorLayer() const { return mVectorLayer; }

  protected:
    QgsHtmlAnnotationItem( QgsMapCanvas* canvas, QgsHtmlAnnotationItem* source );

  private slots:
    /**Sets a feature for the current map position and updates the dialog*/
    void setFeatureForMapPosition();
    /**Sets visibility status based on mVectorLayer visibility*/
    void updateVisibility();

    void javascript();

  private:
    QGraphicsProxyWidget* mWidgetContainer;
    QWebView* mWebView;
    /**Associated vectorlayer (or 0 if attributes are not supposed to be replaced)*/
    QgsVectorLayer* mVectorLayer;
    /**True if the item is related to a vector feature*/
    bool mHasAssociatedFeature;
    /**Associated feature*/
    QgsFeatureId mFeatureId;
    QgsFeature mFeature;
    QString mHtmlFile;
    QString mHtmlSource;

    QString replaceText( QString displayText, QgsVectorLayer *layer, QgsFeature &feat );
    void _showItemEditor() override;
};

#endif // QGSHTMLANNOTATIONITEM_H
