/***************************************************************************
    qgsrulebasedrendererv2.h - Rule-based renderer (symbology-ng)
    ---------------------
    begin                : May 2010
    copyright            : (C) 2010 by Martin Dobias
    email                : wonder.sk at gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSRULEBASEDRENDERERV2_H
#define QGSRULEBASEDRENDERERV2_H

#include "qgsfield.h"
#include "qgsfeature.h"
#include "qgis.h"

#include "qgsrendererv2.h"

class QgsExpression;

class QgsCategorizedSymbolRendererV2;
class QgsGraduatedSymbolRendererV2;

/**
When drawing a vector layer with rule-based renderer, it goes through
the rules and draws features with symbols from rules that match.
 */
class CORE_EXPORT QgsRuleBasedRendererV2 : public QgsFeatureRendererV2
{
  public:


    // TODO: use QVarLengthArray instead of QList

    enum FeatureFlags { FeatIsSelected = 1, FeatDrawMarkers = 2 };

    // feature for rendering: QgsFeature and some flags
    struct FeatureToRender
    {
      FeatureToRender( QgsFeature& _f, int _flags ) : feat( _f ), flags( _flags ) {}
      QgsFeature feat;
      int flags; // selected and/or draw markers
    };

    // rendering job: a feature to be rendered with a particular symbol
    // (both f, symbol are _not_ owned by this class)
    struct RenderJob
    {
      RenderJob( FeatureToRender& _ftr, QgsSymbolV2* _s ) : ftr( _ftr ), symbol( _s ) {}
      FeatureToRender& ftr;
      QgsSymbolV2* symbol;
    };

    // render level: a list of jobs to be drawn at particular level
    // (jobs are owned by this class)
    struct RenderLevel
    {
      RenderLevel( int z ): zIndex( z ) {}
      ~RenderLevel() { foreach ( RenderJob* j, jobs ) delete j; }
      int zIndex;
      QList<RenderJob*> jobs;
    };

    // rendering queue: a list of rendering levels
    typedef QList<RenderLevel> RenderQueue;

    class Rule;
    typedef QList<Rule*> RuleList;

    /**
      This class keeps data about a rules for rule-based renderer.
      A rule consists of a symbol, filter expression and range of scales.
      If filter is empty, it matches all features.
      If scale range has both values zero, it matches all scales.
      If one of the min/max scale denominators is zero, there is no lower/upper bound for scales.
      A rule matches if both filter and scale range match.
     */
    class CORE_EXPORT Rule
    {
      public:
        //! Constructor takes ownership of the symbol
        Rule( QgsSymbolV2* symbol, int scaleMinDenom = 0, int scaleMaxDenom = 0, QString filterExp = QString(),
              QString label = QString(), QString description = QString() );
        //Rule( const Rule& other );
        ~Rule();
        QString dump( int offset = 0 ) const;
        QSet<QString> usedAttributes();
        QgsSymbolV2List symbols();
        //! @note not available in python bindings
        QgsLegendSymbolList legendSymbolItems();
        bool isFilterOK( QgsFeature& f ) const;
        bool isScaleOK( double scale ) const;

        QgsSymbolV2* symbol() { return mSymbol; }
        QString label() const { return mLabel; }
        bool dependsOnScale() const { return mScaleMinDenom != 0 || mScaleMaxDenom != 0; }
        int scaleMinDenom() const { return mScaleMinDenom; }
        int scaleMaxDenom() const { return mScaleMaxDenom; }
        QgsExpression* filter() const { return mFilter; }
        QString filterExpression() const { return mFilterExp; }
        QString description() const { return mDescription; }

        //! set a new symbol (or NULL). Deletes old symbol.
        void setSymbol( QgsSymbolV2* sym );
        void setLabel( QString label ) { mLabel = label; }
        void setScaleMinDenom( int scaleMinDenom ) { mScaleMinDenom = scaleMinDenom; }
        void setScaleMaxDenom( int scaleMaxDenom ) { mScaleMaxDenom = scaleMaxDenom; }
        void setFilterExpression( QString filterExp ) { mFilterExp = filterExp; initFilter(); }
        void setDescription( QString description ) { mDescription = description; }

        //! clone this rule, return new instance
        Rule* clone() const;

        void toSld( QDomDocument& doc, QDomElement &element, QgsStringMap props );
        static Rule* createFromSld( QDomElement& element, QGis::GeometryType geomType );

        QDomElement save( QDomDocument& doc, QgsSymbolV2Map& symbolMap );

        //! prepare the rule for rendering and its children (build active children array)
        bool startRender( QgsRenderContext& context, const QgsVectorLayer *vlayer );
        //! get all used z-levels from this rule and children
        QSet<int> collectZLevels();
        //! assign normalized z-levels [0..N-1] for this rule's symbol for quick access during rendering
        //! @note not available in python bindings
        void setNormZLevels( const QMap<int, int>& zLevelsToNormLevels );

        bool renderFeature( FeatureToRender& featToRender, QgsRenderContext& context, RenderQueue& renderQueue );

        //! only tell whether a feature will be rendered without actually rendering it
        //! @note added in 1.9
        bool willRenderFeature( QgsFeature& feat );

        //! tell which symbols will be used to render the feature
        //! @note added in 1.9
        QgsSymbolV2List symbolsForFeature( QgsFeature& feat );

        void stopRender( QgsRenderContext& context );

        static Rule* create( QDomElement& ruleElem, QgsSymbolV2Map& symbolMap );

        RuleList& children() { return mChildren; }
        Rule* parent() { return mParent; }

        //! add child rule, take ownership, sets this as parent
        void appendChild( Rule* rule ) { mChildren.append( rule ); rule->mParent = this; }
        //! add child rule, take ownership, sets this as parent
        void insertChild( int i, Rule* rule ) { mChildren.insert( i, rule ); rule->mParent = this; }
        //! delete child rule
        void removeChild( Rule* rule ) { mChildren.removeAll( rule ); delete rule; }
        //! delete child rule
        void removeChildAt( int i ) { Rule* rule = mChildren[i]; mChildren.removeAt( i ); delete rule; }
        //! take child rule out, set parent as null
        void takeChild( Rule* rule ) { mChildren.removeAll( rule ); rule->mParent = NULL; }
        //! take child rule out, set parent as null
        Rule* takeChildAt( int i ) { Rule* rule = mChildren.takeAt( i ); rule->mParent = NULL; return rule; }

      protected:
        void initFilter();

        Rule* mParent; // parent rule (NULL only for root rule)
        QgsSymbolV2* mSymbol;
        int mScaleMinDenom, mScaleMaxDenom;
        QString mFilterExp, mLabel, mDescription;
        bool mElseRule;
        RuleList mChildren;

        // temporary
        QgsExpression* mFilter;
        // temporary while rendering
        QList<int> mSymbolNormZLevels;
        RuleList mActiveChildren;
    };

    /////

    static QgsFeatureRendererV2* create( QDomElement& element );

    //! Constructs the renderer from given tree of rules (takes ownership)
    QgsRuleBasedRendererV2( QgsRuleBasedRendererV2::Rule* root );
    //! Constructor for convenience. Creates a root rule and adds a default rule with symbol (takes ownership)
    QgsRuleBasedRendererV2( QgsSymbolV2* defaultSymbol );

    ~QgsRuleBasedRendererV2();

    //! return symbol for current feature. Should not be used individually: there could be more symbols for a feature
    virtual QgsSymbolV2* symbolForFeature( QgsFeature& feature );

    virtual bool renderFeature( QgsFeature& feature, QgsRenderContext& context, int layer = -1, bool selected = false, bool drawVertexMarker = false );

    virtual void startRender( QgsRenderContext& context, const QgsVectorLayer *vlayer );

    virtual void stopRender( QgsRenderContext& context );

    virtual QList<QString> usedAttributes();

    virtual QgsFeatureRendererV2* clone();

    virtual void toSld( QDomDocument& doc, QDomElement &element ) const;

    static QgsFeatureRendererV2* createFromSld( QDomElement& element, QGis::GeometryType geomType );

    virtual QgsSymbolV2List symbols();

    //! store renderer info to XML element
    virtual QDomElement save( QDomDocument& doc );

    //! return a list of symbology items for the legend
    virtual QgsLegendSymbologyList legendSymbologyItems( QSize iconSize );

    //! return a list of item text / symbol
    //! @note: this method was added in version 1.5
    //! @note not available in python bindings
    virtual QgsLegendSymbolList legendSymbolItems();

    //! for debugging
    virtual QString dump();

    //! return whether the renderer will render a feature or not.
    //! Must be called between startRender() and stopRender() calls.
    //! @note added in 1.9
    virtual bool willRenderFeature( QgsFeature& feat );

    //! return list of symbols used for rendering the feature.
    //! For renderers that do not support MoreSymbolsPerFeature it is more efficient
    //! to use symbolForFeature()
    //! @note added in 1.9
    virtual QgsSymbolV2List symbolsForFeature( QgsFeature& feat );

    //! returns bitwise OR-ed capabilities of the renderer
    //! \note added in 2.0
    virtual int capabilities() { return MoreSymbolsPerFeature; }

    /////

    Rule* rootRule() { return mRootRule; }

    //////

    //! take a rule and create a list of new rules based on the categories from categorized symbol renderer
    static void refineRuleCategories( Rule* initialRule, QgsCategorizedSymbolRendererV2* r );
    //! take a rule and create a list of new rules based on the ranges from graduated symbol renderer
    static void refineRuleRanges( Rule* initialRule, QgsGraduatedSymbolRendererV2* r );
    //! take a rule and create a list of new rules with intervals of scales given by the passed scale denominators
    static void refineRuleScales( Rule* initialRule, QList<int> scales );

  protected:
    //! the root node with hierarchical list of rules
    Rule* mRootRule;

    // temporary
    RenderQueue mRenderQueue;
    QList<FeatureToRender> mCurrentFeatures;
};

#endif // QGSRULEBASEDRENDERERV2_H
