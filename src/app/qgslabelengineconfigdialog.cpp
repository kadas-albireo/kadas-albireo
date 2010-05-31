#include "qgslabelengineconfigdialog.h"

#include "qgspallabeling.h"

QgsLabelEngineConfigDialog::QgsLabelEngineConfigDialog( QgsPalLabeling* lbl, QWidget* parent )
    : QDialog( parent ), mLBL( lbl )
{
  setupUi( this );

  connect( buttonBox, SIGNAL( accepted() ), this, SLOT( onOK() ) );

  // search method
  cboSearchMethod->setCurrentIndex( mLBL->searchMethod() );

  // candidate numbers
  int candPoint, candLine, candPolygon;
  mLBL->numCandidatePositions( candPoint, candLine, candPolygon );
  spinCandPoint->setValue( candPoint );
  spinCandLine->setValue( candLine );
  spinCandPolygon->setValue( candPolygon );

  chkShowCandidates->setChecked( mLBL->isShowingCandidates() );

  chkShowAllLabels->setChecked( mLBL->isShowingAllLabels() );
}


void QgsLabelEngineConfigDialog::onOK()
{
  // save
  mLBL->setSearchMethod(( QgsPalLabeling::Search ) cboSearchMethod->currentIndex() );

  mLBL->setNumCandidatePositions( spinCandPoint->value(),
                                  spinCandLine->value(),
                                  spinCandPolygon->value() );

  mLBL->setShowingCandidates( chkShowCandidates->isChecked() );

  mLBL->setShowingAllLabels( chkShowAllLabels->isChecked() );

  accept();
}
