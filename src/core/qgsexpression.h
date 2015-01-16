/***************************************************************************
                               qgsexpression.h
                             -------------------
    begin                : August 2011
    copyright            : (C) 2011 Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSEXPRESSION_H
#define QGSEXPRESSION_H

#include <QMetaType>
#include <QStringList>
#include <QVariant>
#include <QList>
#include <QDomDocument>

#include "qgsfield.h"
#include "qgsdistancearea.h"

class QgsFeature;
class QgsGeometry;
class QgsOgcUtils;
class QgsVectorLayer;
class QgsVectorDataProvider;

class QDomElement;

/**
Class for parsing and evaluation of expressions (formerly called "search strings").
The expressions try to follow both syntax and semantics of SQL expressions.

Usage:
\code{.cpp}
  QgsExpression exp("gid*2 > 10 and type not in ('D','F'));
  if (exp.hasParserError())
  {
    // show error message with parserErrorString() and exit
  }
  QVariant result = exp.evaluate(feature, fields);
  if (exp.hasEvalError())
  {
    // show error message with evalErrorString()
  }
  else
  {
    // examine the result
  }
\endcode

Possible QVariant value types:
- invalid (null)
- int
- double
- string
- geometry

Similarly to SQL, this class supports three-value logic: true/false/unknown.
Unknown value may be a result of operations with missing data (NULL). Please note
that NULL is different value than zero or an empty string. For example
3 > NULL returns unknown.

There is no special (three-value) 'boolean' type: true/false is represented as
1/0 integer, unknown value is represented the same way as NULL values: invalid QVariant.

For better performance with many evaluations you may first call prepare(fields) function
to find out indices of columns and then repeatedly call evaluate(feature).

Type conversion: operators and functions that expect arguments to be of particular
type automatically convert the arguments to that type, e.g. sin('2.1') will convert
the argument to a double, length(123) will first convert the number to a string.
Explicit conversion can be achieved with toint, toreal, tostring functions.
If implicit or explicit conversion is invalid, the evaluation returns an error.
Comparison operators do numeric comparison in case both operators are numeric (int/double)
or they can be converted to numeric types.

Arithmetic operators do integer arithmetics if both operands are integer. That is
2+2 yields integer 4, but 2.0+2 returns real number 4.0. There are also two versions of
division and modulo operators: 1.0/2 returns 0.5 while 1/2 returns 0. */
class CORE_EXPORT QgsExpression
{
  public:
    QgsExpression( const QString& expr );
    ~QgsExpression();

    //! Returns true if an error occurred when parsing the input expression
    bool hasParserError() const { return !mParserErrorString.isNull(); }
    //! Returns parser error
    QString parserErrorString() const { return mParserErrorString; }

    class Node;

    //! Returns root node of the expression. Root node is null is parsing has failed
    const Node* rootNode() const { return mRootNode; }

    //! Get the expression ready for evaluation - find out column indexes.
    bool prepare( const QgsFields &fields );

    /**
     * Get list of columns referenced by the expression.
     * @note if the returned list contains the QgsFeatureRequest::AllAttributes constant then
     * all attributes from the layer are required for evaluation of the expression.
     * QgsFeatureRequest::setSubsetOfAttributes automatically handles this case.
     */
    QStringList referencedColumns() const;

    //! Returns true if the expression uses feature geometry for some computation
    bool needsGeometry() const;

    // evaluation

    //! Evaluate the feature and return the result
    //! @note prepare() should be called before calling this method
    QVariant evaluate( const QgsFeature* f = NULL );

    //! Evaluate the feature and return the result
    //! @note prepare() should be called before calling this method
    //! @note available in python bindings as evaluatePrepared
    inline QVariant evaluate( const QgsFeature& f ) { return evaluate( &f ); }

    //! Evaluate the feature and return the result
    //! @note this method does not expect that prepare() has been called on this instance
    QVariant evaluate( const QgsFeature* f, const QgsFields& fields );

    //! Evaluate the feature and return the result
    //! @note this method does not expect that prepare() has been called on this instance
    //! @note not available in python bindings
    inline QVariant evaluate( const QgsFeature& f, const QgsFields& fields ) { return evaluate( &f, fields ); }

    //! Returns true if an error occurred when evaluating last input
    bool hasEvalError() const { return !mEvalErrorString.isNull(); }
    //! Returns evaluation error
    QString evalErrorString() const { return mEvalErrorString; }
    //! Set evaluation error (used internally by evaluation functions)
    void setEvalErrorString( QString str ) { mEvalErrorString = str; }

    //! Set the number for $rownum special column
    void setCurrentRowNumber( int rowNumber ) { mRowNumber = rowNumber; }
    //! Return the number used for $rownum special column
    int currentRowNumber() { return mRowNumber; }

    //! Assign a special column
    static void setSpecialColumn( const QString& name, QVariant value );
    //! Unset a special column
    static void unsetSpecialColumn( const QString& name );
    //! Return the value of the given special column or a null QVariant if undefined
    static QVariant specialColumn( const QString& name );
    //! Check whether a special column exists
    //! @note added in 2.2
    static bool hasSpecialColumn( const QString& name );

    static bool isValid( const QString& text, const QgsFields& fields, QString &errorMessage );

    void setScale( double scale ) { mScale = scale; }

    double scale() { return mScale; }

    //! Alias for dump()
    const QString expression() const
    {
      if ( !mExp.isNull() )
        return mExp;
      else
        return dump();
    }

    //! Return the expression string that represents this QgsExpression.
    QString dump() const;

    //! Return calculator used for distance and area calculations
    //! (used by internal functions)
    QgsDistanceArea *geomCalculator() { initGeomCalculator(); return mCalc; }

    //! Sets the geometry calculator used in evaluation of expressions,
    // instead of the default.
    void setGeomCalculator( const QgsDistanceArea &calc );

    /** This function currently replaces each expression between [% and %]
       in the string with the result of its evaluation on the feature
       passed as argument.

       Additional substitutions can be passed through the substitutionMap
       parameter
       @param action
       @param feat
       @param layer
       @param substitutionMap
       @param distanceArea optional QgsDistanceArea. If specified, the QgsDistanceArea is used for distance
       and area conversion
    */
    static QString replaceExpressionText( const QString &action, const QgsFeature *feat,
                                          QgsVectorLayer *layer,
                                          const QMap<QString, QVariant> *substitutionMap = 0,
                                          const QgsDistanceArea* distanceArea = 0
                                        );

    /**Attempts to evaluate a text string as an expression to a resultant double
     * value.
     * @param text text to evaluate as expression
     * @param fallbackValue value to return if text can not be evaluated as a double
     * @returns evaluated double value, or fallback value
     * @note added in QGIS 2.7
    */
    static double evaluateToDouble( const QString& text, const double fallbackValue );

    /**
     * @brief list of unary operators
     * @note if any change is made here, the definition of QgsExpression::UnaryOperatorText[] must be adapted.
     */
    enum UnaryOperator
    {
      uoNot,
      uoMinus,
    };

    /**
     * @brief list of binary operators
     * @note if any change is made here, the definition of QgsExpression::BinaryOperatorText[] must be adapted.
     */
    enum BinaryOperator
    {
      // logical
      boOr,
      boAnd,

      // comparison
      boEQ,  // =
      boNE,  // <>
      boLE,  // <=
      boGE,  // >=
      boLT,  // <
      boGT,  // >
      boRegexp,
      boLike,
      boNotLike,
      boILike,
      boNotILike,
      boIs,
      boIsNot,

      // math
      boPlus,
      boMinus,
      boMul,
      boDiv,
      boIntDiv,
      boMod,
      boPow,

      // strings
      boConcat,
    };
    enum SpatialOperator
    {
      soBbox,
      soIntersects,
      soContains,
      soCrosses,
      soEquals,
      soDisjoint,
      soOverlaps,
      soTouches,
      soWithin,
    };

    static const char* BinaryOperatorText[];
    static const char* UnaryOperatorText[];

    typedef QVariant( *FcnEval )( const QVariantList& values, const QgsFeature* f, QgsExpression* parent );


    /**
      * A abstract base class for defining QgsExpression functions.
      */
    class CORE_EXPORT Function
    {
      public:
        Function( QString fnname, int params, QString group, QString helpText = QString(), bool usesGeometry = false, QStringList referencedColumns = QStringList(), bool lazyEval = false )
            : mName( fnname ), mParams( params ), mUsesGeometry( usesGeometry ), mGroup( group ), mHelpText( helpText ), mReferencedColumns( referencedColumns ), mLazyEval( lazyEval ) {}
        /** The name of the function. */
        QString name() { return mName; }
        /** The number of parameters this function takes. */
        int params() { return mParams; }
        /** Does this function use a geometry object. */
        bool usesgeometry() { return mUsesGeometry; }

        /** True if this function should use lazy evaluation.  Lazy evaluation functions take QgsExpression::Node objects
         * rather than the node results when called.  You can use node->eval(parent, feature) to evaluate the node and return the result
         * Functions are non lazy default and will be given the node return value when called **/
        bool lazyEval() { return mLazyEval; }

        virtual QStringList referencedColumns() const { return mReferencedColumns; }

        /** The group the function belongs to. */
        QString group() { return mGroup; }
        /** The help text for the function. */
        QString helptext() { return mHelpText.isEmpty() ? QgsExpression::helptext( mName ) : mHelpText; }

        virtual QVariant func( const QVariantList& values, const QgsFeature* f, QgsExpression* parent ) = 0;

        bool operator==( const Function& other ) const
        {
          if ( QString::compare( mName, other.mName, Qt::CaseInsensitive ) == 0 )
            return true;

          return false;
        }

      private:
        QString mName;
        int mParams;
        bool mUsesGeometry;
        QString mGroup;
        QString mHelpText;
        QStringList mReferencedColumns;
        bool mLazyEval;
    };

    class StaticFunction : public Function
    {
      public:
        StaticFunction( QString fnname, int params, FcnEval fcn, QString group, QString helpText = QString(), bool usesGeometry = false, QStringList referencedColumns = QStringList(), bool lazyEval = false )
            : Function( fnname, params, group, helpText, usesGeometry, referencedColumns, lazyEval ), mFnc( fcn ) {}

        virtual QVariant func( const QVariantList& values, const QgsFeature* f, QgsExpression* parent ) override
        {
          return mFnc( values, f, parent );
        }

      private:
        FcnEval mFnc;
    };

    static const QList<Function*> &Functions();
    static QList<Function*> gmFunctions;

    static QStringList gmBuiltinFunctions;
    static const QStringList &BuiltinFunctions();

    static bool registerFunction( Function* function );
    static bool unregisterFunction( QString name );

    // tells whether the identifier is a name of existing function
    static bool isFunctionName( QString name );

    // return index of the function in Functions array
    static int functionIndex( QString name );

    /**  Returns the number of functions defined in the parser
      *  @return The number of function defined in the parser.
      */
    static int functionCount();

    /**
     * Returns a list of special Column definitions
     */
    static QList<Function*> specialColumns();

    //! return quoted column reference (in double quotes)
    static QString quotedColumnRef( QString name );
    //! return quoted string (in single quotes)
    static QString quotedString( QString text );

    //////

    class Visitor; // visitor interface is defined below

    enum NodeType
    {
      ntUnaryOperator,
      ntBinaryOperator,
      ntInOperator,
      ntFunction,
      ntLiteral,
      ntColumnRef,
      ntCondition
    };

    class CORE_EXPORT Node
    {
      public:
        virtual ~Node() {}
        virtual NodeType nodeType() const = 0;
        // abstract virtual eval function
        // errors are reported to the parent
        virtual QVariant eval( QgsExpression* parent, const QgsFeature* f ) = 0;

        // abstract virtual preparation function
        // errors are reported to the parent
        virtual bool prepare( QgsExpression* parent, const QgsFields &fields ) = 0;

        virtual QString dump() const = 0;

        virtual QStringList referencedColumns() const = 0;
        virtual bool needsGeometry() const = 0;

        // support for visitor pattern
        virtual void accept( Visitor& v ) const = 0;
    };

    class CORE_EXPORT NodeList
    {
      public:
        NodeList() {}
        virtual ~NodeList() { qDeleteAll( mList ); }
        void append( Node* node ) { mList.append( node ); }
        int count() { return mList.count(); }
        QList<Node*> list() { return mList; }

        virtual QString dump() const;

      protected:
        QList<Node*> mList;
    };

    class CORE_EXPORT Interval
    {
        // YEAR const value taken from postgres query
        // SELECT EXTRACT(EPOCH FROM interval '1 year')
        static const int YEARS = 31557600;
        static const int MONTHS = 60 * 60 * 24 * 30;
        static const int WEEKS = 60 * 60 * 24 * 7;
        static const int DAY = 60 * 60 * 24;
        static const int HOUR = 60 * 60;
        static const int MINUTE = 60;
      public:
        Interval( double seconds = 0 ) : mSeconds( seconds ), mValid( true ) { }
        ~Interval();
        double years() { return mSeconds / YEARS;}
        double months() { return mSeconds / MONTHS; }
        double weeks() { return mSeconds / WEEKS;}
        double days() { return mSeconds / DAY;}
        double hours() { return mSeconds / HOUR;}
        double minutes() { return mSeconds / MINUTE;}
        double seconds() { return mSeconds; }
        bool isValid() { return mValid; }
        void setValid( bool valid ) { mValid = valid; }
        bool operator==( const QgsExpression::Interval& other ) const;
        static QgsExpression::Interval invalidInterVal();
        static QgsExpression::Interval fromString( QString string );
      private:
        double mSeconds;
        bool mValid;
    };

    class CORE_EXPORT NodeUnaryOperator : public Node
    {
      public:
        NodeUnaryOperator( UnaryOperator op, Node* operand ) : mOp( op ), mOperand( operand ) {}
        ~NodeUnaryOperator() { delete mOperand; }

        UnaryOperator op() const { return mOp; }
        Node* operand() const { return mOperand; }

        virtual NodeType nodeType() const override { return ntUnaryOperator; }
        virtual bool prepare( QgsExpression* parent, const QgsFields &fields ) override;
        virtual QVariant eval( QgsExpression* parent, const QgsFeature* f ) override;
        virtual QString dump() const override;

        virtual QStringList referencedColumns() const override { return mOperand->referencedColumns(); }
        virtual bool needsGeometry() const override { return mOperand->needsGeometry(); }
        virtual void accept( Visitor& v ) const override { v.visit( *this ); }

      protected:
        UnaryOperator mOp;
        Node* mOperand;
    };

    class CORE_EXPORT NodeBinaryOperator : public Node
    {
      public:
        NodeBinaryOperator( BinaryOperator op, Node* opLeft, Node* opRight ) : mOp( op ), mOpLeft( opLeft ), mOpRight( opRight ) {}
        ~NodeBinaryOperator() { delete mOpLeft; delete mOpRight; }

        BinaryOperator op() const { return mOp; }
        Node* opLeft() const { return mOpLeft; }
        Node* opRight() const { return mOpRight; }

        virtual NodeType nodeType() const override { return ntBinaryOperator; }
        virtual bool prepare( QgsExpression* parent, const QgsFields &fields ) override;
        virtual QVariant eval( QgsExpression* parent, const QgsFeature* f ) override;
        virtual QString dump() const override;

        virtual QStringList referencedColumns() const override { return mOpLeft->referencedColumns() + mOpRight->referencedColumns(); }
        virtual bool needsGeometry() const override { return mOpLeft->needsGeometry() || mOpRight->needsGeometry(); }
        virtual void accept( Visitor& v ) const override { v.visit( *this ); }

        int precedence() const;

      protected:
        bool compare( double diff );
        int computeInt( int x, int y );
        double computeDouble( double x, double y );
        QDateTime computeDateTimeFromInterval( QDateTime d, QgsExpression::Interval *i );

        BinaryOperator mOp;
        Node* mOpLeft;
        Node* mOpRight;
    };

    class CORE_EXPORT NodeInOperator : public Node
    {
      public:
        NodeInOperator( Node* node, NodeList* list, bool notin = false ) : mNode( node ), mList( list ), mNotIn( notin ) {}
        virtual ~NodeInOperator() { delete mNode; delete mList; }

        Node* node() const { return mNode; }
        bool isNotIn() const { return mNotIn; }
        NodeList* list() const { return mList; }

        virtual NodeType nodeType() const override { return ntInOperator; }
        virtual bool prepare( QgsExpression* parent, const QgsFields &fields ) override;
        virtual QVariant eval( QgsExpression* parent, const QgsFeature* f ) override;
        virtual QString dump() const override;

        virtual QStringList referencedColumns() const override { QStringList lst( mNode->referencedColumns() ); foreach ( Node* n, mList->list() ) lst.append( n->referencedColumns() ); return lst; }
        virtual bool needsGeometry() const override { bool needs = false; foreach ( Node* n, mList->list() ) needs |= n->needsGeometry(); return needs; }
          virtual void accept( Visitor& v ) const override { v.visit( *this ); }

          protected:
            Node* mNode;
        NodeList* mList;
        bool mNotIn;
    };

    class CORE_EXPORT NodeFunction : public Node
    {
      public:
        NodeFunction( int fnIndex, NodeList* args ) : mFnIndex( fnIndex ), mArgs( args ) {}
        //NodeFunction( QString name, NodeList* args ) : mName(name), mArgs(args) {}
        virtual ~NodeFunction() { delete mArgs; }

        int fnIndex() const { return mFnIndex; }
        NodeList* args() const { return mArgs; }

        virtual NodeType nodeType() const override { return ntFunction; }
        virtual bool prepare( QgsExpression* parent, const QgsFields &fields ) override;
        virtual QVariant eval( QgsExpression* parent, const QgsFeature* f ) override;
        virtual QString dump() const override;

        virtual QStringList referencedColumns() const override;
        virtual bool needsGeometry() const override { bool needs = Functions()[mFnIndex]->usesgeometry(); if ( mArgs ) { foreach ( Node* n, mArgs->list() ) needs |= n->needsGeometry(); } return needs; }
        virtual void accept( Visitor& v ) const override { v.visit( *this ); }

      protected:
        //QString mName;
        int mFnIndex;
        NodeList* mArgs;
    };

    class CORE_EXPORT NodeLiteral : public Node
    {
      public:
        NodeLiteral( QVariant value ) : mValue( value ) {}

        QVariant value() const { return mValue; }

        virtual NodeType nodeType() const override { return ntLiteral; }
        virtual bool prepare( QgsExpression* parent, const QgsFields &fields ) override;
        virtual QVariant eval( QgsExpression* parent, const QgsFeature* f ) override;
        virtual QString dump() const override;

        virtual QStringList referencedColumns() const override { return QStringList(); }
        virtual bool needsGeometry() const override { return false; }
        virtual void accept( Visitor& v ) const override { v.visit( *this ); }

      protected:
        QVariant mValue;
    };

    class CORE_EXPORT NodeColumnRef : public Node
    {
      public:
        NodeColumnRef( QString name ) : mName( name ), mIndex( -1 ) {}

        QString name() const { return mName; }

        virtual NodeType nodeType() const override { return ntColumnRef; }
        virtual bool prepare( QgsExpression* parent, const QgsFields &fields ) override;
        virtual QVariant eval( QgsExpression* parent, const QgsFeature* f ) override;
        virtual QString dump() const override;

        virtual QStringList referencedColumns() const override { return QStringList( mName ); }
        virtual bool needsGeometry() const override { return false; }

        virtual void accept( Visitor& v ) const override { v.visit( *this ); }

      protected:
        QString mName;
        int mIndex;
    };

    class CORE_EXPORT WhenThen
    {
      public:
        WhenThen( Node* whenExp, Node* thenExp ) : mWhenExp( whenExp ), mThenExp( thenExp ) {}
        ~WhenThen() { delete mWhenExp; delete mThenExp; }

        //protected:
        Node* mWhenExp;
        Node* mThenExp;
    };
    typedef QList<WhenThen*> WhenThenList;

    class CORE_EXPORT NodeCondition : public Node
    {
      public:
        NodeCondition( WhenThenList* conditions, Node* elseExp = NULL ) : mConditions( *conditions ), mElseExp( elseExp ) { delete conditions; }
        ~NodeCondition() { delete mElseExp; qDeleteAll( mConditions ); }

        virtual NodeType nodeType() const override { return ntCondition; }
        virtual QVariant eval( QgsExpression* parent, const QgsFeature* f ) override;
        virtual bool prepare( QgsExpression* parent, const QgsFields &fields ) override;
        virtual QString dump() const override;

        virtual QStringList referencedColumns() const override;
        virtual bool needsGeometry() const override;
        virtual void accept( Visitor& v ) const override { v.visit( *this ); }

      protected:
        WhenThenList mConditions;
        Node* mElseExp;
    };

    //////

    /** support for visitor pattern - algorithms dealing with the expressions
        may be implemented without modifying the Node classes */
    class CORE_EXPORT Visitor
    {
      public:
        virtual ~Visitor() {}
        virtual void visit( const NodeUnaryOperator& n ) = 0;
        virtual void visit( const NodeBinaryOperator& n ) = 0;
        virtual void visit( const NodeInOperator& n ) = 0;
        virtual void visit( const NodeFunction& n ) = 0;
        virtual void visit( const NodeLiteral& n ) = 0;
        virtual void visit( const NodeColumnRef& n ) = 0;
        virtual void visit( const NodeCondition& n ) = 0;
    };

    /** entry function for the visitor pattern */
    void acceptVisitor( Visitor& v ) const;

    static QString helptext( QString name );
    static QString group( QString group );

  protected:
    /**
     * Used by QgsOgcUtils to create an empty
     */
    QgsExpression() : mRootNode( 0 ), mRowNumber( 0 ), mCalc( 0 ) {}

    void initGeomCalculator();

    Node* mRootNode;

    QString mParserErrorString;
    QString mEvalErrorString;

    int mRowNumber;
    double mScale;
    QString mExp;

    QgsDistanceArea *mCalc;

    static QMap<QString, QVariant> gmSpecialColumns;
    static QMap<QString, QString> gmSpecialColumnGroups;

    static QHash<QString, QString> gFunctionHelpTexts;
    static QHash<QString, QString> gGroups;

    static void initFunctionHelp();

    friend class QgsOgcUtils;

  private:
    Q_DISABLE_COPY( QgsExpression )  // for now - until we have proper copy constructor / implicit sharing
};

Q_DECLARE_METATYPE( QgsExpression::Interval );
Q_DECLARE_METATYPE( QgsExpression::Node* );

#endif // QGSEXPRESSION_H
