/***************************************************************************
                         qgscomposerlegendwidget.h
                         -------------------------
    begin                : July 2008
    copyright            : (C) 2008 by Marco Hugentobler
    email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSCOMPOSERLEGENDWIDGET_H
#define QGSCOMPOSERLEGENDWIDGET_H

#include "ui_qgscomposerlegendwidgetbase.h"
#include "qgscomposeritemwidget.h"
#include <QWidget>
#include <QItemDelegate>

class QgsComposerLegend;


/** \ingroup MapComposer
 * A widget for setting properties relating to a composer legend.
 */
class QgsComposerLegendWidget: public QgsComposerItemBaseWidget, private Ui::QgsComposerLegendWidgetBase
{
    Q_OBJECT

  public:
    QgsComposerLegendWidget( QgsComposerLegend* legend );
    ~QgsComposerLegendWidget();

    /**Updates the legend layers and groups*/
    void updateLegend();

    QgsComposerLegend* legend() { return mLegend; }

  public slots:

    void on_mWrapCharLineEdit_textChanged( const QString& text );
    void on_mTitleLineEdit_textChanged( const QString& text );
    void on_mTitleAlignCombo_currentIndexChanged( int index );
    void on_mColumnCountSpinBox_valueChanged( int c );
    void on_mSplitLayerCheckBox_toggled( bool checked );
    void on_mEqualColumnWidthCheckBox_toggled( bool checked );
    void on_mSymbolWidthSpinBox_valueChanged( double d );
    void on_mSymbolHeightSpinBox_valueChanged( double d );
    void on_mWmsLegendWidthSpinBox_valueChanged( double d );
    void on_mWmsLegendHeightSpinBox_valueChanged( double d );
    void on_mTitleSpaceBottomSpinBox_valueChanged( double d );
    void on_mGroupSpaceSpinBox_valueChanged( double d );
    void on_mLayerSpaceSpinBox_valueChanged( double d );
    void on_mSymbolSpaceSpinBox_valueChanged( double d );
    void on_mIconLabelSpaceSpinBox_valueChanged( double d );
    void on_mTitleFontButton_clicked();
    void on_mGroupFontButton_clicked();
    void on_mLayerFontButton_clicked();
    void on_mItemFontButton_clicked();
    void on_mFontColorButton_colorChanged( const QColor& newFontColor );
    void on_mBoxSpaceSpinBox_valueChanged( double d );
    void on_mColumnSpaceSpinBox_valueChanged( double d );
    void on_mCheckBoxAutoUpdate_stateChanged( int state );
    void on_mMapComboBox_currentIndexChanged( int index );

    //item manipulation
    void on_mMoveDownToolButton_clicked();
    void on_mMoveUpToolButton_clicked();
    void on_mRemoveToolButton_clicked();
    void on_mAddToolButton_clicked();
    void on_mEditPushButton_clicked();
    void on_mCountToolButton_clicked( bool checked );
    void on_mFilterByMapToolButton_clicked( bool checked );
    void resetLayerNodeToDefaults();
    void on_mUpdateAllPushButton_clicked();
    void on_mAddGroupToolButton_clicked();

    void selectedChanged( const QModelIndex & current, const QModelIndex & previous );

    void setCurrentNodeStyleFromAction();

  protected:
    void showEvent( QShowEvent * event ) override;

  private slots:
    /**Sets GUI according to state of mLegend*/
    void setGuiElements();

  private:
    QgsComposerLegendWidget();
    void blockAllSignals( bool b );
    void refreshMapComboBox();


    QgsComposerLegend* mLegend;
};

#endif

