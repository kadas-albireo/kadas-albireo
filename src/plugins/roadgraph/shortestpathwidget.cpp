/***************************************************************************
 *   Copyright (C) 2009 by Sergey Yakushev                                 *
 *   yakushevs@list.ru                                                     *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

/**
 * \file shortestpathwidget.cpp
 * \brief implemetation UI for find shotest path
 */

//qt includes
#include <qcombobox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <qlabel.h>
#include <qlineedit.h>
#include <QToolButton>
#include <QMessageBox>
#include <qgscontexthelp.h>

// Qgis includes
#include <qgsmapcanvas.h>
#include <qgsmaptoolemitpoint.h>
#include <qgisinterface.h>
#include <qgsrubberband.h>
#include <qgsmaptopixel.h>
#include <qgsmaprenderer.h>
#include <qgsfeature.h>
#include <qgsapplication.h>
#include <qgsvectorlayer.h>

#include <qgsgraphdirector.h>
#include <qgsgraphbuilder.h>
#include <qgsgraph.h>
#include <qgsgraphanalyzer.h>

// roadgraph plugin includes
#include "roadgraphplugin.h"
#include "shortestpathwidget.h"
#include "exportdlg.h"
#include "units.h"
#include "settings.h"

//standard includes

RgShortestPathWidget::RgShortestPathWidget( QWidget* theParent, RoadGraphPlugin *thePlugin )   : QDockWidget( theParent ), mPlugin( thePlugin )
{
  setWindowTitle( tr( "Shortest path" ) );
  setObjectName( "ShortestPathDock" );
  setAllowedAreas( Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );

  QWidget *myWidget = new QWidget( this );
  setWidget( myWidget );

  QVBoxLayout *v = new QVBoxLayout( myWidget );
  QHBoxLayout *h = NULL;
  QLabel *l = NULL;

  l = new QLabel( tr( "Start" ), myWidget );
  v->addWidget( l );
  h = new QHBoxLayout();
  mFrontPointLineEdit = new QLineEdit( myWidget );
  mFrontPointLineEdit->setReadOnly( true );
  QToolButton *selectFrontPoint = new QToolButton( myWidget );
  selectFrontPoint->setCheckable( true );
  selectFrontPoint->setIcon( QPixmap( ":/roadgraph/coordinate_capture.png" ) );
  h->addWidget( mFrontPointLineEdit );
  h->addWidget( selectFrontPoint );
  v->addLayout( h );

  l = new QLabel( tr( "Stop" ), myWidget );
  v->addWidget( l );
  h = new QHBoxLayout();
  mBackPointLineEdit = new QLineEdit( myWidget );
  mBackPointLineEdit->setReadOnly( true );
  QToolButton *selectBackPoint = new QToolButton( myWidget );
  selectBackPoint->setCheckable( true );
  selectBackPoint->setIcon( QPixmap( ":/roadgraph/coordinate_capture.png" ) );
  h->addWidget( mBackPointLineEdit );
  h->addWidget( selectBackPoint );
  v->addLayout( h );

  h = new QHBoxLayout();
  l = new QLabel( tr( "Criterion" ), myWidget );
  mCriterionName = new QComboBox( myWidget );
  mCriterionName->insertItem( 0, tr( "Length" ) );
  mCriterionName->insertItem( 1, tr( "Time" ) );
  h->addWidget( l );
  h->addWidget( mCriterionName );
  v->addLayout( h );

  h = new QHBoxLayout();
  l = new QLabel( tr( "Length" ), myWidget );
  mPathCostLineEdit = new QLineEdit( myWidget );
  mPathCostLineEdit->setReadOnly( true );
  h->addWidget( l );
  h->addWidget( mPathCostLineEdit );
  v->addLayout( h );

  h = new QHBoxLayout();
  l = new QLabel( tr( "Time" ), myWidget );
  mPathTimeLineEdit = new QLineEdit( myWidget );
  mPathTimeLineEdit->setReadOnly( true );
  h->addWidget( l );
  h->addWidget( mPathTimeLineEdit );
  v->addLayout( h );

  h = new QHBoxLayout();
  mCalculate = new QPushButton( tr( "Calculate" ), myWidget );
  h->addWidget( mCalculate );
  QPushButton *pbExport = new QPushButton( tr( "Export" ), myWidget );
  h->addWidget( pbExport );

  connect( pbExport, SIGNAL( clicked( bool ) ), this, SLOT( exportPath() ) );

  mClear =  new QPushButton( tr( "Clear" ), myWidget );
  h->addWidget( mClear );
  v->addLayout( h );

  h = new QHBoxLayout();
  QPushButton *helpButton = new QPushButton( tr( "Help" ), this );
  helpButton->setIcon( style()->standardIcon( QStyle::SP_DialogHelpButton ) );
  h->addWidget( helpButton );
  v->addLayout( h );

  v->addStretch();

  mFrontPointMapTool = new QgsMapToolEmitPoint( mPlugin->iface()->mapCanvas() );
  mFrontPointMapTool->setButton( selectFrontPoint );

  mBackPointMapTool  = new QgsMapToolEmitPoint( mPlugin->iface()->mapCanvas() );
  mBackPointMapTool->setButton( selectBackPoint );

  connect( selectFrontPoint, SIGNAL( clicked( bool ) ), this, SLOT( onSelectFrontPoint() ) );
  connect( mFrontPointMapTool, SIGNAL( canvasClicked( const QgsPoint&, Qt::MouseButton ) ),
           this, SLOT( setFrontPoint( const QgsPoint& ) ) );

  connect( selectBackPoint, SIGNAL( clicked( bool ) ), this, SLOT( onSelectBackPoint() ) );
  connect( mBackPointMapTool, SIGNAL( canvasClicked( const QgsPoint&, Qt::MouseButton ) ),
           this, SLOT( setBackPoint( const QgsPoint& ) ) );

  connect( helpButton, SIGNAL( clicked( bool ) ), this, SLOT( helpRequested() ) );
  connect( mCalculate, SIGNAL( clicked( bool ) ), this, SLOT( findingPath() ) );
  connect( mClear, SIGNAL( clicked( bool ) ), this, SLOT( clear() ) );

  mrbFrontPoint = new QgsRubberBand( mPlugin->iface()->mapCanvas(), true );
  mrbFrontPoint->setColor( Qt::green );
  mrbFrontPoint->setWidth( 2 );

  mrbBackPoint = new QgsRubberBand( mPlugin->iface()->mapCanvas(), true );
  mrbBackPoint->setColor( Qt::red );
  mrbBackPoint->setWidth( 2 );

  mrbPath = new QgsRubberBand( mPlugin->iface()->mapCanvas(), false );
  mrbPath->setWidth( 2 );

  connect( mPlugin->iface()->mapCanvas(), SIGNAL( extentsChanged() ), this, SLOT( mapCanvasExtentsChanged() ) );

} //RgShortestPathWidget::RgShortestPathWidget()
RgShortestPathWidget::~RgShortestPathWidget()
{
  delete mFrontPointMapTool;
  delete mBackPointMapTool;

  delete mrbFrontPoint;
  delete mrbBackPoint;
  delete mrbPath;
} //RgShortestPathWidget::~RgShortestPathWidget()

void RgShortestPathWidget::mapCanvasExtentsChanged()
{
  // update rubberbands
  if ( mFrontPointLineEdit->text().length() > 0 )
    setFrontPoint( mFrontPoint );
  if ( mBackPointLineEdit->text().length() > 0 )
    setBackPoint( mBackPoint );
}

void RgShortestPathWidget::onSelectFrontPoint()
{
  mPlugin->iface()->mapCanvas()->setMapTool( mFrontPointMapTool );
}

void RgShortestPathWidget::setFrontPoint( const QgsPoint& pt )
{
  mPlugin->iface()->mapCanvas()->unsetMapTool( mFrontPointMapTool );
  mFrontPointLineEdit->setText( QString( "(" ) + QString().setNum( pt.x() ) + QString( "," ) +
                                QString().setNum( pt.y() ) + QString( ")" ) );
  mFrontPoint = pt;

  double mupp = mPlugin->iface()->mapCanvas()->getCoordinateTransform()->mapUnitsPerPixel() * 2;

  mrbFrontPoint->reset( true );
  mrbFrontPoint->addPoint( QgsPoint( pt.x() - mupp, pt.y() - mupp ), false );
  mrbFrontPoint->addPoint( QgsPoint( pt.x() + mupp, pt.y() - mupp ), false );
  mrbFrontPoint->addPoint( QgsPoint( pt.x() + mupp, pt.y() + mupp ), false );
  mrbFrontPoint->addPoint( QgsPoint( pt.x() - mupp, pt.y() + mupp ), true );
  mrbFrontPoint->show();
} //RgShortestPathWidget::setFrontPoint( const QgsPoint& pt )

void RgShortestPathWidget::onSelectBackPoint()
{
  mPlugin->iface()->mapCanvas()->setMapTool( mBackPointMapTool );
}

void RgShortestPathWidget::setBackPoint( const QgsPoint& pt )
{
  mPlugin->iface()->mapCanvas()->unsetMapTool( mBackPointMapTool );

  mBackPoint = pt;
  mBackPointLineEdit->setText( QString( "(" ) + QString().setNum( pt.x() ) + QString( "," ) +
                               QString().setNum( pt.y() ) + QString( ")" ) );

  double mupp = mPlugin->iface()->mapCanvas()->getCoordinateTransform()->mapUnitsPerPixel() * 2;

  mrbBackPoint->reset( true );
  mrbBackPoint->addPoint( QgsPoint( pt.x() - mupp, pt.y() - mupp ), false );
  mrbBackPoint->addPoint( QgsPoint( pt.x() + mupp, pt.y() - mupp ), false );
  mrbBackPoint->addPoint( QgsPoint( pt.x() + mupp, pt.y() + mupp ), false );
  mrbBackPoint->addPoint( QgsPoint( pt.x() - mupp, pt.y() + mupp ), true );
  mrbBackPoint->show();
}

bool RgShortestPathWidget::getPath( QgsGraph* shortestTree, QgsPoint& p1, QgsPoint& p2 )
{
  if ( mFrontPointLineEdit->text().isNull() || mBackPointLineEdit->text().isNull() )
  {
    QMessageBox::critical( this, tr( "Point not selected" ), tr( "First, select start and stop points." ) );
    return false;
  }

  QgsGraphBuilder builder(
    mPlugin->iface()->mapCanvas()->mapRenderer()->destinationCrs(),
    mPlugin->iface()->mapCanvas()->mapRenderer()->hasCrsTransformEnabled(),
    mPlugin->topologyToleranceFactor() );
  {
    const QgsGraphDirector *director = mPlugin->director();
    if ( director == NULL )
    {
      QMessageBox::critical( this, tr( "Plugin isn't configured" ), tr( "Plugin isn't configured!" ) );
      return false;
    }
    connect( director, SIGNAL( buildProgress( int, int ) ), mPlugin->iface()->mainWindow(), SLOT( showProgress( int, int ) ) );
    connect( director, SIGNAL( buildMessage( QString ) ), mPlugin->iface()->mainWindow(), SLOT( showStatusMessage( QString ) ) );

    QVector< QgsPoint > points;
    QVector< QgsPoint > tiedPoint;

    points.push_back( mFrontPoint );
    points.push_back( mBackPoint );
    director->makeGraph( &builder, points, tiedPoint );

    p1 = tiedPoint[ 0 ];
    p2 = tiedPoint[ 1 ];
    // not need
    delete director;
  }

  if ( p1 == QgsPoint( 0.0, 0.0 ) )
  {
    QMessageBox::critical( this, tr( "Tie point failed" ), tr( "Start point doesn't tie to the road!" ) );
    return false;
  }
  if ( p2 == QgsPoint( 0.0, 0.0 ) )
  {
    QMessageBox::critical( this, tr( "Tie point failed" ), tr( "Stop point doesn't tie to the road!" ) );
    return false;
  }

  QgsGraph *graph = builder.graph();

  QVector< int > pointIdx( 0, 0 );
  QVector< double > pointCost( 0, 0.0 );

  int startVertexIdx = graph->findVertex( p1 );

  int criterionNum = 0;
  if ( mCriterionName->currentIndex() > 0 )
    criterionNum = 1;

  QgsGraphAnalyzer::shortestpath( graph, startVertexIdx, criterionNum, pointIdx, pointCost, shortestTree );

  delete graph;

  if ( shortestTree->findVertex( p2 ) == -1 )
  {
    QMessageBox::critical( this, tr( "Path not found" ), tr( "Path not found" ) );
    return false;
  }
  return true;
}

void RgShortestPathWidget::findingPath()
{
  QgsPoint p1, p2;
  QgsGraph path;

  if ( !getPath( &path, p1, p2 ) )
    return;

  mrbPath->reset( false );
  double time = 0.0;
  double cost = 0.0;

  int startVertexIdx = path.findVertex( p1 );
  int stopVertexIdx  = path.findVertex( p2 );
  QList< QgsPoint > p;
  while ( startVertexIdx != stopVertexIdx )
  {
    QgsGraphArcIdList l = path.vertex( stopVertexIdx ).inArc();
    if ( l.empty() )
      break;
    const QgsGraphArc& e = path.arc( l.front() );

    cost += e.property( 0 ).toDouble();
    time += e.property( 1 ).toDouble();

    p.push_front( path.vertex( e.inVertex() ).point() );

    stopVertexIdx = e.outVertex();
  }
  p.push_front( p1 );
  QList< QgsPoint>::iterator it;
  for ( it = p.begin(); it != p.end(); ++it )
  {
    mrbPath->addPoint( *it );
  }

  Unit timeUnit = Unit::byName( mPlugin->timeUnitName() );
  Unit distanceUnit = Unit::byName( mPlugin->distanceUnitName() );

  mPathCostLineEdit->setText( QString().setNum( cost / distanceUnit.multipler() ) + distanceUnit.name() );
  mPathTimeLineEdit->setText( QString().setNum( time / timeUnit.multipler() ) + timeUnit.name() );

  mrbPath->setColor( Qt::red );
}

void RgShortestPathWidget::clear()
{
  mFrontPointLineEdit->setText( QString() );
  mrbFrontPoint->reset( true );
  mBackPointLineEdit->setText( QString() );
  mrbBackPoint->reset( true );
  mrbPath->reset();
  mPathCostLineEdit->setText( QString() );
  mPathTimeLineEdit->setText( QString() );
}

void RgShortestPathWidget::exportPath()
{
  RgExportDlg dlg( this );
  if ( !dlg.exec() )
    return;

  QgsPoint p1, p2;
  QgsGraph path;
  if ( !getPath( &path, p1, p2 ) )
    return;

  QgsVectorLayer *vl = dlg.mapLayer();
  if ( vl == NULL )
    return;

  QgsCoordinateTransform ct( mPlugin->iface()->mapCanvas()->mapRenderer()->destinationCrs(),
                             vl->crs() );

  int startVertexIdx = path.findVertex( p1 );
  int stopVertexIdx  = path.findVertex( p2 );

  QgsPolyline p;
  while ( startVertexIdx != stopVertexIdx )
  {
    QgsGraphArcIdList l = path.vertex( stopVertexIdx ).inArc();
    if ( l.empty() )
      break;
    const QgsGraphArc& e = path.arc( l.front() );
    p.push_front( path.vertex( e.inVertex() ).point() );
    stopVertexIdx = e.outVertex();
  }
  p.push_front( p1 );

  vl->startEditing();
  QgsFeature f;
  f.setGeometry( QgsGeometry::fromPolyline( p ) );
  vl->addFeature( f );
  vl->updateExtents();

  mPlugin->iface()->mapCanvas()->update();
}

void RgShortestPathWidget::helpRequested()
{
  QgsContextHelp::run( metaObject()->className() );
}
