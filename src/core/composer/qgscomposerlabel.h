/***************************************************************************
                         qgscomposerlabel.h
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
#ifndef QGSCOMPOSERLABEL_H
#define QGSCOMPOSERLABEL_H

#include "qgscomposeritem.h"
#include <QFont>

class QgsVectorLayer;
class QgsFeature;
class QgsDistanceArea;

/** \ingroup MapComposer
 * A label that can be placed onto a map composition.
 */
class CORE_EXPORT QgsComposerLabel: public QgsComposerItem
{
    Q_OBJECT
  public:
    QgsComposerLabel( QgsComposition *composition );
    ~QgsComposerLabel();

    /** return correct graphics item type. */
    virtual int type() const { return ComposerLabel; }

    /** \brief Reimplementation of QCanvasItem::paint*/
    void paint( QPainter* painter, const QStyleOptionGraphicsItem* itemStyle, QWidget* pWidget );

    /**resizes the widget such that the text fits to the item. Keeps top left point*/
    void adjustSizeToText();

    QString text() { return mText; }
    void setText( const QString& text );

    int htmlState() { return mHtmlState; }
    void setHtmlState( int state );

    /**Returns the text as it appears on screen (with replaced data field) */
    QString displayText() const;

    /** Sets the current feature, the current layer and a list of local variable substitutions for evaluating expressions */
    void setExpressionContext( QgsFeature* feature, QgsVectorLayer* layer, QMap<QString, QVariant> substitutions = ( QMap<QString, QVariant>() ) );

    QFont font() const;
    void setFont( const QFont& f );
    /** Accessor for the vertical alignment of the label
     * @returns Qt::AlignmentFlag
     */
    Qt::AlignmentFlag vAlign() const { return mVAlignment; }
    /** Accessor for the horizontal alignment of the label
     * @returns Qt::AlignmentFlag
     */
    Qt::AlignmentFlag hAlign() const { return mHAlignment; }
    /** Mutator for the horizontal alignment of the label
     * @param a alignment
     * @returns void
     */
    void setHAlign( Qt::AlignmentFlag a ) {mHAlignment = a;}
    /** Mutator for the vertical alignment of the label
     * @param a alignment
     * @returns void
     */
    void setVAlign( Qt::AlignmentFlag a ) { mVAlignment = a; }
    //!brief Accessor for the margin of the label
    double margin() { return mMargin; }
    //!brief Mutator for the margin of the label
    void setMargin( double m ) { mMargin = m; }

    /**Sets text color
        */
    void setFontColor( const QColor& c ) { mFontColor = c; }
    /**Get font color
        */
    QColor fontColor() const { return mFontColor; }

    /** stores state in Dom element
       * @param elem is Dom element corresponding to 'Composer' tag
       * @param doc document
       */
    bool writeXML( QDomElement& elem, QDomDocument & doc ) const;

    /** sets state from Dom document
       * @param itemElem is Dom element corresponding to 'ComposerLabel' tag
       * @param doc document
       */
    bool readXML( const QDomElement& itemElem, const QDomDocument& doc );

    //Overriden to contain part of label's text
    virtual QString displayName() const;

  public slots:
    void refreshExpressionContext();


  private slots:
    void loadingHtmlFinished( bool );

  private:
    // Text
    QString mText;

    // Html state
    int mHtmlState;
    double mHtmlUnitsToMM;
    double htmlUnitsToMM(); //calculate scale factor
    bool mHtmlLoaded;

    /**Helper function to calculate x/y shift for adjustSizeToText() depending on rotation, current size and alignment*/
    void itemShiftAdjustSize( double newWidth, double newHeight, double& xShift, double& yShift ) const;

    // Font
    QFont mFont;

    // Border between text and fram (in mm)
    double mMargin;

    // Font color
    QColor mFontColor;

    // Horizontal Alignment
    Qt::AlignmentFlag mHAlignment;

    // Vertical Alignment
    Qt::AlignmentFlag mVAlignment;

    /**Replaces replace '$CURRENT_DATE<(FORMAT)>' with the current date (e.g. $CURRENT_DATE(d 'June' yyyy)*/
    void replaceDateText( QString& text ) const;

    QgsFeature* mExpressionFeature;
    QgsVectorLayer* mExpressionLayer;
    QMap<QString, QVariant> mSubstitutions;
    QgsDistanceArea* mDistanceArea;
};

#endif


