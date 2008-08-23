/***************************************************************************
                              qgscompositionwidget.h
                             ------------------------
    begin                : June 11 2008
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

#include "ui_qgscompositionwidgetbase.h"

class QgsComposition;

/** \ingroup MapComposer
 * Struct to hold map composer paper properties.
 */
struct QgsCompositionPaper
{
  QgsCompositionPaper( QString name, double width, double height ) {mName = name; mWidth = width; mHeight = height;}
  QString mName;
  double mWidth;
  double mHeight;
};

/** \ingroup MapComposer
  * Input widget for QgsComposition
  */
class QgsCompositionWidget: public QWidget, private Ui::QgsCompositionWidgetBase
{
    Q_OBJECT
  public:
    QgsCompositionWidget( QWidget* parent, QgsComposition* c );
    ~QgsCompositionWidget();

  public slots:
    void on_mPaperSizeComboBox_currentIndexChanged( const QString& text );
    void on_mPaperOrientationComboBox_currentIndexChanged( const QString& text );
    void on_mPaperWidthLineEdit_editingFinished();
    void on_mPaperHeightLineEdit_editingFinished();
    void on_mResolutionLineEdit_textChanged( const QString& text );

  private:
    QgsComposition* mComposition;
    QMap<QString, QgsCompositionPaper> mPaperMap;

    QgsCompositionWidget(); //default constructor is forbidden
    /**Sets width/height to chosen paper format and updates paper item*/
    void applyCurrentPaperSettings();
    /**Applies the current width and height values*/
    void applyWidthHeight();
    /**Makes sure width/height values for custom paper matches the current orientation*/
    void adjustOrientation();
    /**Sets GUI elements to width/height from composition*/
    void displayCompositionWidthHeight();

    void createPaperEntries();
    void insertPaperEntries();
};
