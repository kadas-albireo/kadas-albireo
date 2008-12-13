/***************************************************************************
                            qgscomposerscalebar.h
                             -------------------
    begin                : March 2005
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
#ifndef QGSCOMPOSERSCALEBAR_H
#define QGSCOMPOSERSCALEBAR_H

#include "qgscomposeritem.h"
#include <QObject>
#include <QPen>

class QgsComposerMap;
class QgsScaleBarStyle;
/** \ingroup MapComposer
 * A scale bar item that can be added to a map composition.
 */

class CORE_EXPORT QgsComposerScaleBar: public QObject, public QgsComposerItem
{

    Q_OBJECT

  public:

    QgsComposerScaleBar( QgsComposition* composition );
    ~QgsComposerScaleBar();

    /** \brief Reimplementation of QCanvasItem::paint*/
    void paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget );

    //getters and setters
    int numSegments() const {return mNumSegments;}
    void setNumSegments( int nSegments ) {mNumSegments = nSegments;}

    int numSegmentsLeft() const {return mNumSegmentsLeft;}
    void setNumSegmentsLeft( int nSegmentsLeft ) {mNumSegmentsLeft = nSegmentsLeft;}

    double numUnitsPerSegment() const {return mNumUnitsPerSegment;}
    void setNumUnitsPerSegment( double units );

    double numMapUnitsPerScaleBarUnit() const {return mNumMapUnitsPerScaleBarUnit;}
    void setNumMapUnitsPerScaleBarUnit( double d ) {mNumMapUnitsPerScaleBarUnit = d;}

    QString unitLabeling() const {return mUnitLabeling;}
    void setUnitLabeling( const QString& label ) {mUnitLabeling = label;}

    QFont font() const;

    void setFont( const QFont& font );

    QPen pen() const {return mPen;}
    void setPen( const QPen& pen ) {mPen = pen;}

    QBrush brush() const {return mBrush;}
    void setBrush( const QBrush& brush ) {mBrush = brush;}

    double height() const {return mHeight;}
    void setHeight( double h ) {mHeight = h;}

    void setComposerMap( const QgsComposerMap* map );
    const QgsComposerMap* composerMap() const {return mComposerMap;}

    double labelBarSpace() const {return mLabelBarSpace;}
    void setLabelBarSpace( double space ) {mLabelBarSpace = space;}

    double boxContentSpace() const {return mBoxContentSpace;}
    void setBoxContentSpace( double space ) {mBoxContentSpace = space;}

    double segmentMillimeters() const {return mSegmentMillimeters;}

    /**Apply default settings (scale bar 1/5 of map item width)*/
    void applyDefaultSettings();

    /**Sets style by name
     @param styleName (untranslated) style name. Possibilities are: 'Single Box', 'Double Box', 'Line Ticks Middle', 'Line Ticks Down', 'Line Ticks Up', 'Numeric'*/
    void setStyle( const QString& styleName );

    /**Returns style name*/
    QString style() const;

    /**Returns the x - positions of the segment borders (in item coordinates) and the width
     of the segment*/
    void segmentPositions( QList<QPair<double, double> >& posWidthList ) const;

    /**Sets box size suitable to content*/
    void adjustBoxSize();

    /**Adjusts box size and calls QgsComposerItem::update()*/
    void update();

    /**Returns string of first label (important for drawing, labeling, size calculation*/
    QString firstLabelString() const;

    /** stores state in Dom node
       * @param elem is Dom element corresponding to 'Composer' tag
       * @param temp write template file
       */
    bool writeXML( QDomElement& elem, QDomDocument & doc ) const;

    /** sets state from Dom document
       * @param itemElem is Dom node corresponding to item tag
       */
    bool readXML( const QDomElement& itemElem, const QDomDocument& doc );

  public slots:
    void updateSegmentSize();
    /**Sets mCompositionMap to 0 if the map is deleted*/
    void invalidateCurrentMap();

  protected:

    /**Reference to composer map object*/
    const QgsComposerMap* mComposerMap;
    /**Number of segments on right side*/
    int mNumSegments;
    /**Number of segments on left side*/
    int mNumSegmentsLeft;
    /**Size of a segment (in map units)*/
    double mNumUnitsPerSegment;
    /**Number of map units per scale bar units (e.g. 1000 to have km for a map with m units)*/
    double mNumMapUnitsPerScaleBarUnit;

    /**Labeling of map units*/
    QString mUnitLabeling;
    /**Font*/
    QFont mFont;
    /**Outline*/
    QPen mPen;
    /**Fill*/
    QBrush mBrush;
    /**Height of bars/lines*/
    double mHeight;
    /**Scalebar style*/
    QgsScaleBarStyle* mStyle;

    /**Space between bar and Text labels*/
    double mLabelBarSpace;

    /**Space between content and item box*/
    double mBoxContentSpace;

    /**Width of a segment (in mm)*/
    double mSegmentMillimeters;

    /**Calculates with of a segment in mm and stores it in mSegmentMillimeters*/
    void refreshSegmentMillimeters();
};

#endif //QGSCOMPOSERSCALEBAR_H


