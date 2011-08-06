/***************************************************************************
                         qgsattributedialog.cpp  -  description
                             -------------------
    begin                : October 2004
    copyright            : (C) 2004 by Marco Hugentobler
    email                : marco.hugentobler@autoform.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgsattributedialog.h"
#include "qgsfield.h"
#include "qgslogger.h"

#include "qgsvectorlayer.h"
#include "qgsvectordataprovider.h"
#include "qgsuniquevaluerenderer.h"
#include "qgssymbol.h"
#include "qgsattributeeditor.h"
#include "qgshighlight.h"
#include "qgssearchstring.h"
#include "qgssearchtreenode.h"
#include "qgspythonrunner.h"

#include "qgisapp.h"

#include <QTableWidgetItem>
#include <QSettings>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDialogButtonBox>
#include <QUiLoader>
#include <QDialog>
#include <QVBoxLayout>
#include <QLineEdit>

int QgsAttributeDialog::smFormCounter = 0;

QgsAttributeDialog::QgsAttributeDialog( QgsVectorLayer *vl, QgsFeature *thepFeature, bool featureOwner )
    : mDialog( 0 )
    , mSettingsPath( "/Windows/AttributeDialog/" )
    , mLayer( vl )
    , mFeature( thepFeature )
    , mFeatureOwner( featureOwner )
    , mHighlight( 0 )
    , mFormNr( -1 )
{
  if ( !mFeature || !vl->dataProvider() )
    return;

  const QgsFieldMap &theFieldMap = vl->pendingFields();
  if ( theFieldMap.isEmpty() )
    return;

  QgsAttributeMap myAttributes = mFeature->attributeMap();

  QDialogButtonBox *buttonBox = NULL;

  if ( !vl->editForm().isEmpty() )
  {
    QFile file( vl->editForm() );

    if ( file.open( QFile::ReadOnly ) )
    {
      QUiLoader loader;

      QFileInfo fi( vl->editForm() );
      loader.setWorkingDirectory( fi.dir() );
      QWidget *myWidget = loader.load( &file, QgisApp::instance() );
      file.close();

      mDialog = qobject_cast<QDialog*>( myWidget );
      buttonBox = myWidget->findChild<QDialogButtonBox*>();
    }
  }

  if ( !mDialog )
  {
    mDialog = new QDialog( QgisApp::instance() );

    QGridLayout *gridLayout;
    QFrame *mFrame;

    mDialog->resize( 447, 343 );
    gridLayout = new QGridLayout( mDialog );
    gridLayout->setSpacing( 6 );
    gridLayout->setMargin( 2 );
    gridLayout->setObjectName( QString::fromUtf8( "gridLayout" ) );
    mFrame = new QFrame( mDialog );
    mFrame->setObjectName( QString::fromUtf8( "mFrame" ) );
    mFrame->setFrameShape( QFrame::NoFrame );
    mFrame->setFrameShadow( QFrame::Plain );

    gridLayout->addWidget( mFrame, 0, 0, 1, 1 );

    buttonBox = new QDialogButtonBox( mDialog );
    buttonBox->setObjectName( QString::fromUtf8( "buttonBox" ) );
    gridLayout->addWidget( buttonBox, 2, 0, 1, 1 );

    //
    //Set up dynamic inside a scroll box
    //
    QVBoxLayout *mypOuterLayout = new QVBoxLayout();
    mypOuterLayout->setContentsMargins( 0, 0, 0, 0 );
    //transfers layout ownership so no need to call delete

    mFrame->setLayout( mypOuterLayout );
    QScrollArea *mypScrollArea = new QScrollArea();
    mypScrollArea->setFrameShape( QFrame::NoFrame );
    mypScrollArea->setFrameShadow( QFrame::Plain );
    //transfers scroll area ownership so no need to call delete
    mypOuterLayout->addWidget( mypScrollArea );
    QFrame *mypInnerFrame = new QFrame();
    mypInnerFrame->setFrameShape( QFrame::NoFrame );
    mypInnerFrame->setFrameShadow( QFrame::Plain );
    //transfers frame ownership so no need to call delete
    mypScrollArea->setWidget( mypInnerFrame );
    mypScrollArea->setWidgetResizable( true );
    QGridLayout *mypInnerLayout = new QGridLayout( mypInnerFrame );

    int index = 0;
    for ( QgsAttributeMap::const_iterator it = myAttributes.begin(); it != myAttributes.end(); ++it )
    {
      const QgsField &field = theFieldMap[it.key()];

      //show attribute alias if available
      QString myFieldName = vl->attributeDisplayName( it.key() );
      int myFieldType = field.type();

      QWidget *myWidget = QgsAttributeEditor::createAttributeEditor( 0, 0, vl, it.key(), it.value() );
      if ( !myWidget )
        continue;

      QLabel * mypLabel = new QLabel();
      mypInnerLayout->addWidget( mypLabel, index, 0 );
      if ( myFieldType == QVariant::Int )
      {
        mypLabel->setText( myFieldName );
      }
      else if ( myFieldType == QVariant::Double )
      {
        mypLabel->setText( myFieldName );
      }
      else if ( myFieldType == QVariant::LongLong )
      {
        mypLabel->setText( myFieldName );
      }
      else //string
      {
        //any special behaviour for string goes here
        mypLabel->setText( myFieldName );
      }

      if ( vl->editType( it.key() ) != QgsVectorLayer::Immutable )
      {
        myWidget->setEnabled( vl->isEditable() );
      }

      mypInnerLayout->addWidget( myWidget, index, 1 );
      mpIndizes << it.key();
      mpWidgets << myWidget;
      ++index;
    }

    // Set focus to first widget in list, to help entering data without moving the mouse.
    if ( mpWidgets.size() > 0 )
    {
      mpWidgets.first()->setFocus( Qt::OtherFocusReason );
    }
  }
  else
  {
    for ( QgsAttributeMap::const_iterator it = myAttributes.begin(); it != myAttributes.end(); ++it )
    {
      const QgsField &field = theFieldMap[it.key()];

      QWidget *myWidget = mDialog->findChild<QWidget*>( field.name() );
      if ( !myWidget )
        continue;

      QgsAttributeEditor::createAttributeEditor( mDialog, myWidget, vl, it.key(), it.value() );

      if ( vl->editType( it.key() ) != QgsVectorLayer::Immutable )
      {
        myWidget->setEnabled( vl->isEditable() );
      }

      mpIndizes << it.key();
      mpWidgets << myWidget;
    }

    foreach( QLineEdit *le, mDialog->findChildren<QLineEdit*>() )
    {
      if ( !le->objectName().startsWith( "expr_" ) )
        continue;

      le->setReadOnly( true );
      QString expr = le->text();
      le->setText( tr( "Error" ) );

      QgsSearchString ss;
      if ( !ss.setString( expr ) )
        continue;

      QgsSearchTreeNode *st = ss.tree();
      if ( !st )
        continue;

      if ( !mFeature->geometry() && st->needsGeometry() )
      {
        QgsFeature f;
        if ( vl->featureAtId( mFeature->id(), f, true, false ) && f.geometry() )
        {
          mFeature->setGeometry( *f.geometry() );
        }
      }

      QgsSearchTreeValue value;
      st->getValue( value, st, vl->pendingFields(), *mFeature );

      if ( !value.isError() )
      {
        if ( value.isNumeric() )
          le->setText( QString::number( value.number() ) );
        else
          le->setText( value.string() );
      }
      else
      {
        le->setText( tr( "Error: %1" ).arg( st->errorMsg() ) );
      }
    }
  }

  if ( mDialog )
  {
    if ( mDialog->objectName().isEmpty() )
      mDialog->setObjectName( "QgsAttributeDialogBase" );

    if ( mDialog->windowTitle().isEmpty() )
      mDialog->setWindowTitle( tr( "Attributes - %1" ).arg( vl->name() ) );
  }

  if ( buttonBox )
  {
    buttonBox->clear();

    if ( vl->isEditable() )
    {
      buttonBox->setStandardButtons( QDialogButtonBox::Ok | QDialogButtonBox::Cancel );
      connect( buttonBox, SIGNAL( accepted() ), mDialog, SLOT( accept() ) );
      connect( buttonBox, SIGNAL( accepted() ), this, SLOT( accept() ) );
    }
    else
    {
      buttonBox->setStandardButtons( QDialogButtonBox::Cancel );
    }

    connect( buttonBox, SIGNAL( rejected() ), mDialog, SLOT( reject() ) );
  }

  QMetaObject::connectSlotsByName( mDialog );

  connect( mDialog, SIGNAL( destroyed() ), this, SLOT( dialogDestroyed() ) );

  if ( !vl->editFormInit().isEmpty() )
  {
#if 0
    // would be nice if only PyQt's QVariant.toPyObject() wouldn't take ownership
    vl->setProperty( "featureForm.dialog", QVariant::fromValue( qobject_cast<QObject*>( mDialog ) ) );
    vl->setProperty( "featureForm.id", QVariant( mpFeature->id() ) );
#endif

    QString module = vl->editFormInit();
    int pos = module.lastIndexOf( "." );
    if ( pos >= 0 )
    {
      QgsPythonRunner::run( QString( "import %1" ).arg( module.left( pos ) ) );
    }

    mFormNr = smFormCounter++;
    QgsPythonRunner::run( QString( "_qgis_featureform_%1 = wrapinstance( %2, QtGui.QDialog )" ).arg( mFormNr ).arg(( unsigned long ) mDialog ) );

    QString expr = QString( "%1(_qgis_featureform_%2,'%3',%4)" ).arg( vl->editFormInit() ).arg( mFormNr ).arg( vl->id() ).arg( mFeature->id() );
    QgsDebugMsg( QString( "running featureForm init: %1" ).arg( expr ) );
    QgsPythonRunner::run( expr );
  }

  restoreGeometry();
}


QgsAttributeDialog::~QgsAttributeDialog()
{
  if ( mHighlight )
  {
    mHighlight->hide();
    delete mHighlight;
  }

  if ( mFeatureOwner )
  {
    delete mFeature;
  }

  saveGeometry();

  if ( mDialog )
  {
    delete mDialog;
  }
}

void QgsAttributeDialog::accept()
{
  if ( !mLayer->isEditable() || !mFeature )
    return;

  //write the new values back to the feature
  QgsAttributeMap myAttributes = mFeature->attributeMap();
  int myIndex = 0;
  for ( QgsAttributeMap::const_iterator it = myAttributes.begin(); it != myAttributes.end(); ++it )
  {
    QVariant value;

    int idx = mpIndizes.value( myIndex );
    if ( QgsAttributeEditor::retrieveValue( mpWidgets.value( myIndex ), mLayer, idx, value ) )
      mFeature->changeAttribute( idx, value );

    ++myIndex;
  }
}

int QgsAttributeDialog::exec()
{
  if ( mDialog )
  {
    return mDialog->exec();
  }
  else
  {
    QgsDebugMsg( "No dialog" );
    return QDialog::Accepted;
  }
}

void QgsAttributeDialog::show()
{
  if ( mDialog )
  {
    mDialog->setAttribute( Qt::WA_DeleteOnClose );
    mDialog->show();
    mDialog->raise();
    mDialog->activateWindow();
    mDialog->installEventFilter( this );
  }
}

void QgsAttributeDialog::saveGeometry()
{
  if ( mDialog )
  {
    QSettings settings;
    settings.setValue( mSettingsPath + "geometry", mDialog->saveGeometry() );
  }
}

void QgsAttributeDialog::restoreGeometry()
{
  if ( mDialog )
  {
    QSettings settings;
    mDialog->restoreGeometry( settings.value( mSettingsPath + "geometry" ).toByteArray() );
  }
}

void QgsAttributeDialog::setHighlight( QgsHighlight *h )
{
  if ( mHighlight )
  {
    delete mHighlight;
  }

  mHighlight = h;
}


void QgsAttributeDialog::dialogDestroyed()
{
#if 0
  mLayer->setProperty( "featureForm.dialog", QVariant() );
  mLayer->setProperty( "featureForm.id", QVariant() );
#endif
  if ( -1 < mFormNr )
  {
    QString expr = QString( "if locals().has_key('_qgis_featureform_%1'): del _qgis_featureform_%1\n" ).arg( mFormNr );
    QgsPythonRunner::run( expr );
  }

  mDialog = NULL;
  deleteLater();
}

bool QgsAttributeDialog::eventFilter( QObject *obj, QEvent *e )
{
  if ( mHighlight && obj == mDialog )
  {
    switch ( e->type() )
    {
      case QEvent::WindowActivate:
        mHighlight->show();
        break;
      case QEvent::WindowDeactivate:
        mHighlight->hide();
        break;
      default:
        break;
    }
  }

  return false;
}
