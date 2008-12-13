/***************************************************************************
                         qgscomposer.h
                             -------------------
    begin                : January 2005
    copyright            : (C) 2005 by Radim Blazek
    email                : blazek@itc.it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/* $Id$ */
#ifndef QGSCOMPOSER_H
#define QGSCOMPOSER_H
#include "ui_qgscomposerbase.h"
#include "qgscomposeritem.h"

class QgisApp;
class QgsComposerLabel;
class QgsComposerLegend;
class QgsComposerMap;
class QgsComposerPicture;
class QgsComposerScaleBar;
class QgsComposerView;
class QgsComposition;
class QgsMapCanvas;

class QGridLayout;
class QPrinter;
class QDomNode;
class QDomDocument;
class QMoveEvent;
class QResizeEvent;
class QFile;
class QSizeGrip;

/** \ingroup MapComposer
 * \brief A gui for composing a printable map.
 */
class QgsComposer: public QMainWindow, private Ui::QgsComposerBase
{
    Q_OBJECT

  public:
    QgsComposer( QgisApp *qgis );
    ~QgsComposer();

    //! Set the pixmap / icons on the toolbar buttons
    void setupTheme();

    //! Open and show, set defaults if first time
    void open();

    //! Zoom to full extent of the paper
    void zoomFull();

    //! Return pointer to map canvas
    QgsMapCanvas *mapCanvas( void );

    //! Return pointer to composer view
    QgsComposerView *view( void );

    //! Return current composition
    //QgsComposition *composition(void);

    //! Show composition options in widget
    void showCompositionOptions( QWidget *w );

    //! Restore the window and toolbar state
    void restoreWindowState();

  protected:
    //! Move event
    virtual void moveEvent( QMoveEvent * );

    //! Resize event
    virtual void resizeEvent( QResizeEvent * );

#ifdef Q_WS_MAC
    //! Change event (update window menu on ActivationChange)
    virtual void changeEvent( QEvent * );

    //! Close event (remove window from menu)
    virtual void closeEvent( QCloseEvent * );

    //! Show event (add window to menu)
    virtual void showEvent( QShowEvent * );
#endif

  public slots:
    //! Zoom to full extent of the paper
    void on_mActionZoomAll_activated( void );

    //! Zoom in
    void on_mActionZoomIn_activated( void );

    //! Zoom out
    void on_mActionZoomOut_activated( void );

    //! Refresh view
    void on_mActionRefreshView_activated( void );

    //! Print the composition
    void on_mActionPrint_activated( void );

    //! Print as image
    void on_mActionExportAsImage_activated( void );

    //! Print as SVG
    void on_mActionExportAsSVG_activated( void );

    //! Select item
    void on_mActionSelectMoveItem_activated( void );

    //! Add new map
    void on_mActionAddNewMap_activated( void );

    //! Add new legend
    void on_mActionAddNewLegend_activated( void );

    //! Add new label
    void on_mActionAddNewLabel_activated( void );

    //! Add new scalebar
    void on_mActionAddNewScalebar_activated( void );

    //! Add new picture
    void on_mActionAddImage_activated( void );

    //! Set tool to move item content
    void on_mActionMoveItemContent_activated( void );

    //! Group selected items
    void on_mActionGroupItems_activated( void );

    //! Ungroup selected item group
    void on_mActionUngroupItems_activated( void );

    //! Move selected items one position up
    void on_mActionRaiseItems_activated( void );

    //!Move selected items one position down
    void on_mActionLowerItems_activated( void );

    //!Move selected items to top
    void on_mActionMoveItemsToTop_activated( void );

    //!Move selected items to bottom
    void on_mActionMoveItemsToBottom_activated( void );

    //! Save window state
    void saveWindowState();

    //! Slot for when the help button is clicked
    void on_helpPButton_clicked();

    //! Slot for when the close button is clicked
    void on_closePButton_clicked();

    /**Add a composer map to the item/widget map and creates a configuration widget for it*/
    void addComposerMap( QgsComposerMap* map );

    /**Adds a composer label to the item/widget map and creates a configuration widget for it*/
    void addComposerLabel( QgsComposerLabel* label );

    /**Adds a composer scale bar to the item/widget map and creates a configuration widget for it*/
    void addComposerScaleBar( QgsComposerScaleBar* scalebar );

    /**Adds a composer legend to the item/widget map and creates a configuration widget for it*/
    void addComposerLegend( QgsComposerLegend* legend );

    /**Adds a composer picture to the item/widget map and creates a configuration widget*/
    void addComposerPicture( QgsComposerPicture* picture );

    /**Removes item from the item/widget map and deletes the configuration widget*/
    void deleteItem( QgsComposerItem* item );

    /**Shows the configuration widget for a composer item*/
    void showItemOptions( const QgsComposerItem* i );

    //XML, usually connected with QgsProject::readProject and QgsProject::writeProject

    //! Stores state in Dom node
    void writeXML( QDomDocument& doc );

    //! Sets state from Dom document
    void readXML( const QDomDocument& doc );

    void setSelectionTool();

  private slots:

    //! Raise, unminimize and activate this window
    void activate();

  private:

    /**Establishes the signal slot connection for the class*/
    void connectSlots();

    //! returns new world matrix for canvas view after zoom with factor scaleChange
    QMatrix updateMatrix( double scaleChange );

    //! True if a composer map contains a WMS layer
    bool containsWMSLayer() const;

    //! Displays a warning because of possible min/max size in WMS
    void showWMSPrintingWarning();

    //! Pointer to composer view
    QgsComposerView *mView;

    //! Current composition
    QgsComposition *mComposition;

    //! Printer
    QPrinter *mPrinter;

    //! Pointer to QGIS application
    QgisApp *mQgis;

    //! The composer was opened first time (-> set defaults)
    bool mFirstTime;

    //! Layout
    QGridLayout *mCompositionOptionsLayout;

    //! Layout
    QGridLayout *mItemOptionsLayout;

    //! Size grip
    QSizeGrip *mSizeGrip;

    //! To know which item to show if selection changes
    QMap<QgsComposerItem*, QWidget*> mItemWidgetMap;

#ifdef Q_WS_MAC
    //! Window menu action to select this window
    QAction *mWindowAction;
#endif

    //! Help context id
    static const int context_id = 985715179;

};

#endif
