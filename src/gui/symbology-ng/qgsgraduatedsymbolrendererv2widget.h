/***************************************************************************
    qgsgraduatedsymbolrendererv2widget.h
    ---------------------
    begin                : December 2009
    copyright            : (C) 2009 by Martin Dobias
    email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGRADUATEDSYMBOLRENDERERV2WIDGET_H
#define QGSGRADUATEDSYMBOLRENDERERV2WIDGET_H

#include "qgsgraduatedsymbolrendererv2.h"
#include "qgsrendererv2widget.h"
#include <QStandardItem>
#include <QProxyStyle>

#include "ui_qgsgraduatedsymbolrendererv2widget.h"

class GUI_EXPORT QgsGraduatedSymbolRendererV2Model : public QAbstractItemModel
{
    Q_OBJECT
  public:
    QgsGraduatedSymbolRendererV2Model( QObject * parent = 0 );
    Qt::ItemFlags flags( const QModelIndex & index ) const override;
    Qt::DropActions supportedDropActions() const override;
    QVariant data( const QModelIndex &index, int role ) const override;
    bool setData( const QModelIndex & index, const QVariant & value, int role ) override;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const override;
    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
    int columnCount( const QModelIndex & = QModelIndex() ) const override;
    QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const override;
    QModelIndex parent( const QModelIndex &index ) const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData( const QModelIndexList &indexes ) const override;
    bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent ) override;

    void setRenderer( QgsGraduatedSymbolRendererV2* renderer );

    QgsRendererRangeV2 rendererRange( const QModelIndex &index );
    void addClass( QgsSymbolV2* symbol );
    void addClass( QgsRendererRangeV2 range );
    void deleteRows( QList<int> rows );
    void removeAllRows();
    void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override;
    void updateSymbology();
    void updateLabels();

  signals:
    void rowsMoved();

  private:
    QgsGraduatedSymbolRendererV2* mRenderer;
    QString mMimeFormat;
};

// View style which shows drop indicator line between items
class QgsGraduatedSymbolRendererV2ViewStyle: public QProxyStyle
{
  public:
    QgsGraduatedSymbolRendererV2ViewStyle( QStyle* style = 0 );

    void drawPrimitive( PrimitiveElement element, const QStyleOption * option, QPainter * painter, const QWidget * widget = 0 ) const override;
};

class GUI_EXPORT QgsGraduatedSymbolRendererV2Widget : public QgsRendererV2Widget, private Ui::QgsGraduatedSymbolRendererV2Widget
{
    Q_OBJECT

  public:
    static QgsRendererV2Widget* create( QgsVectorLayer* layer, QgsStyleV2* style, QgsFeatureRendererV2* renderer );

    QgsGraduatedSymbolRendererV2Widget( QgsVectorLayer* layer, QgsStyleV2* style, QgsFeatureRendererV2* renderer );
    ~QgsGraduatedSymbolRendererV2Widget();

    virtual QgsFeatureRendererV2* renderer() override;

  public slots:
    void changeGraduatedSymbol();
    void graduatedColumnChanged( QString field );
    void classifyGraduated();
    void reapplyColorRamp();
    void rangesDoubleClicked( const QModelIndex & idx );
    void rangesClicked( const QModelIndex & idx );
    void changeCurrentValue( QStandardItem * item );

    /**Adds a class manually to the classification*/
    void addClass();
    /**Removes currently selected classes */
    void deleteClasses();
    /**Removes all classes from the classification*/
    void deleteAllClasses();
    /**Toggle the link between classes boundaries */
    void toggleBoundariesLink( bool linked );

    void rotationFieldChanged( QString fldName );
    void sizeScaleFieldChanged( QString fldName );
    void scaleMethodChanged( QgsSymbolV2::ScaleMethod scaleMethod );
    void labelFormatChanged();

    void showSymbolLevels();

    void rowsMoved();
    void modelDataChanged();

  protected:
    void updateUiFromRenderer( bool updateCount = true );
    void connectUpdateHandlers();
    void disconnectUpdateHandlers();
    bool rowsOrdered();

    void updateGraduatedSymbolIcon();

    //! return a list of indexes for the classes under selection
    QList<int> selectedClasses();
    QgsRangeList selectedRanges();

    void changeRangeSymbol( int rangeIdx );
    void changeRange( int rangeIdx );

    void changeSelectedSymbols();

    QList<QgsSymbolV2*> selectedSymbols() override;
    QgsSymbolV2* findSymbolForRange( double lowerBound, double upperBound, const QgsRangeList& ranges ) const;
    void refreshSymbolView() override;

    void keyPressEvent( QKeyEvent* event ) override;

  protected:
    QgsGraduatedSymbolRendererV2* mRenderer;

    QgsSymbolV2* mGraduatedSymbol;

    int mRowSelected;

    QgsRendererV2DataDefinedMenus* mDataDefinedMenus;

    QgsGraduatedSymbolRendererV2Model* mModel;

    QgsRangeList mCopyBuffer;
};


#endif // QGSGRADUATEDSYMBOLRENDERERV2WIDGET_H
