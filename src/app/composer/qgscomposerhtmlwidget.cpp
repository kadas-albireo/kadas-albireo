#include "qgscomposerhtmlwidget.h"
#include "qgscomposerframe.h"
#include "qgscomposeritemwidget.h"
#include "qgscomposerhtml.h"
#include <QFileDialog>

QgsComposerHtmlWidget::QgsComposerHtmlWidget( QgsComposerHtml* html, QgsComposerFrame* frame ): mHtml( html ), mFrame( frame )
{
  setupUi( this );
  mResizeModeComboBox->addItem( tr( "Use existing frames" ), QgsComposerMultiFrame::UseExistingFrames );
  mResizeModeComboBox->addItem( tr( "Extend to next page" ), QgsComposerMultiFrame::ExtendToNextPage );

  //embed widget for general options
  if ( mFrame )
  {
    //add widget for general composer item properties
    QgsComposerItemWidget* itemPropertiesWidget = new QgsComposerItemWidget( this, mFrame );
    mToolBox->addItem( itemPropertiesWidget, tr( "General options" ) );
  }
}

QgsComposerHtmlWidget::QgsComposerHtmlWidget()
{
}

QgsComposerHtmlWidget::~QgsComposerHtmlWidget()
{
}

void QgsComposerHtmlWidget::blockSignals( bool block )
{
  mUrlLineEdit->blockSignals( block );
  mFileToolButton->blockSignals( block );
  mResizeModeComboBox->blockSignals( block );
}

void QgsComposerHtmlWidget::on_mUrlLineEdit_editingFinished()
{
  if ( mHtml )
  {
    mHtml->setUrl( QUrl( mUrlLineEdit->text() ) );
    mHtml->update();
  }
}

void QgsComposerHtmlWidget::on_mFileToolButton_clicked()
{
  QString file = QFileDialog::getOpenFileName( this, tr( "Select HTML document" ), QString(), "HTML (*.html)" );
  if ( !file.isEmpty() )
  {
    QUrl url = QUrl::fromLocalFile( file );
    mHtml->setUrl( url );
    mUrlLineEdit->setText( url.toString() );
  }
}

void QgsComposerHtmlWidget::on_mResizeModeComboBox_currentIndexChanged( int index )
{
  if ( !mHtml )
  {
    return;
  }

  mHtml->setResizeMode(( QgsComposerMultiFrame::ResizeMode )mResizeModeComboBox->itemData( index ).toInt() );
}

void QgsComposerHtmlWidget::setGuiElementValues()
{
  if ( !mHtml )
  {
    return;
  }

  blockSignals( true );
  mUrlLineEdit->setText( mHtml->url().toString() );
  mResizeModeComboBox->setCurrentIndex( mResizeModeComboBox->findData( mHtml->resizeMode() ) );
  blockSignals( false );
}


