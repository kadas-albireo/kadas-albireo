/***************************************************************************
 *  qgsvbssearchbox.h                                                      *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVBSSEARCHBOX_H
#define QGSVBSSEARCHBOX_H

#include "qgsvbssearchprovider.h"
#include <QList>
#include <QTimer>
#include <QWidget>


class QCheckBox;
class QToolButton;
class QgsCoordinateReferenceSystem;
class QgsMapCanvas;
class QgsMapToolDrawShape;
class QgsPoint;
class QgsRectangle;
class QgsRubberBand;

class GUI_EXPORT QgsVBSSearchBox : public QWidget
{
    Q_OBJECT

  public:
    QgsVBSSearchBox( QWidget* parent = 0 );
    ~QgsVBSSearchBox();

    void init( QgsMapCanvas* canvas );

    void addSearchProvider( QgsVBSSearchProvider* provider );
    void removeSearchProvider( QgsVBSSearchProvider* provider );

  public slots:
    void clearSearch();

  private:
    class LineEdit;
    class TreeWidget;

    enum FilterType { FilterRect, FilterPoly, FilterCircle };

    enum EntryType { EntryTypeCategory, EntryTypeResult };
    static const int sEntryTypeRole;
    static const int sCatNameRole;
    static const int sCatCountRole;
    static const int sResultDataRole;

    QgsMapCanvas* mMapCanvas;
    QgsRubberBand* mRubberBand;
    QgsMapToolDrawShape* mFilterTool;
    QList<QgsVBSSearchProvider*> mSearchProviders;
    QTimer mTimer;
    LineEdit* mSearchBox;
    QToolButton* mFilterButton;
    QToolButton* mSearchButton;
    QToolButton* mClearButton;
    TreeWidget* mTreeWidget;
    int mNumRunningProviders;

    bool eventFilter( QObject* obj, QEvent* ev ) override;
    void createRubberBand();
    void cancelSearch();

  private slots:
    void textChanged();
    void startSearch();
    void searchResultFound( QgsVBSSearchProvider::SearchResult result );
    void searchProviderFinished();
    void resultSelected();
    void resultActivated();
    void clearFilter();
    void setFilterTool();
    void filterToolFinished();
};

#endif // QGSVBSSEARCHBOX_H
