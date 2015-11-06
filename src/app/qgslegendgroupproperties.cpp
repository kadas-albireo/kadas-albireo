#include "qgslegendgroupproperties.h"
#include "qgslayertreegroup.h"

QgsLegendGroupProperties::QgsLegendGroupProperties( QgsLayerTreeGroup* legendGroup, QWidget * parent, Qt::WindowFlags f ): mLegendGroup( legendGroup )
{
  setupUi( this );
  if ( legendGroup )
  {
    mTitleLineEdit->setText( legendGroup->title() );
    mAbstractTextEdit->setText( legendGroup->abstract() );
    mMetadataCheckBox->setChecked( legendGroup->wmsPublishMetadata() );
    mLegendCheckBox->setChecked( legendGroup->wmsPublishLegend() );
    mCheckCheckbox->setChecked( legendGroup->wmsCheckable() );
  }
  connect( this, SIGNAL( accepted() ), this, SLOT( apply() ) );
}

QgsLegendGroupProperties::~QgsLegendGroupProperties()
{

}

void QgsLegendGroupProperties::apply()
{
  if ( mLegendGroup )
  {
    mLegendGroup->setTitle( mTitleLineEdit->text() );
    mLegendGroup->setAbstract( mAbstractTextEdit->toPlainText() );
    mLegendGroup->setWMSPublishMetadata( mMetadataCheckBox->isChecked() );
    mLegendGroup->setWMSPublishLegend( mLegendCheckBox->isChecked() );
    mLegendGroup->setWMSCheckable( mCheckCheckbox->isChecked() );
  }
}
