#include "qgsdatumtransformdialog.h"
#include "qgscoordinatetransform.h"

QgsDatumTransformDialog::QgsDatumTransformDialog( const QString& layerName, const QList< QList< int > >& dt, QWidget* parent, Qt::WindowFlags f ): QDialog( parent, f )
{
  setupUi( this );
  setWindowTitle( tr( "Select datum transformations for layer" ) + " " + layerName );
  QList< QList< int > >::const_iterator it = dt.constBegin();
  for ( ; it != dt.constEnd(); ++it )
  {
    QTreeWidgetItem* item = new QTreeWidgetItem();
    for ( int i = 0; i < 2; ++i )
    {
      if ( i >= it->size() )
      {
        break;
      }

      int nr = it->at( i );
      item->setData( i, Qt::UserRole, nr );
      if ( nr != -1 )
      {
        item->setText( i, QgsCoordinateTransform::datumTransformString( nr ) );
      }
    }
    mDatumTransformTreeWidget->addTopLevelItem( item );
  }
}

QgsDatumTransformDialog::~QgsDatumTransformDialog()
{
}

QgsDatumTransformDialog::QgsDatumTransformDialog(): QDialog()
{
  setupUi( this );
}

QList< int > QgsDatumTransformDialog::selectedDatumTransform()
{
  QList<int> list;
  QTreeWidgetItem * item = mDatumTransformTreeWidget->currentItem();
  if ( item )
  {
    for ( int i = 0; i < 2; ++i )
    {
      int transformNr = item->data( i, Qt::UserRole ).toInt();
      if ( transformNr != -1 )
      {
        list << transformNr;
      }
    }
  }
  return list;
}

bool QgsDatumTransformDialog::rememberSelection() const
{
  return mRememberSelectionCheckBox->isChecked();
}
