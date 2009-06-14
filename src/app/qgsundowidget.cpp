#include "qgsundowidget.h"

#include "qgsmaplayer.h"
#include "qgsmapcanvas.h"

#include "qgisapp.h"

QgsUndoWidget::QgsUndoWidget( QWidget * parent, QgsMapCanvas * mapCanvas )
    : QDockWidget( parent )
{
  setupUi(this);
  setWidget(dockWidgetContents);
  connect( this->undoButton, SIGNAL( clicked() ),
           this, SLOT( undo( ) ) );

  connect( this->redoButton, SIGNAL( clicked() ),
           this, SLOT( redo( ) ) );
  undoButton->setDisabled( true );
  redoButton->setDisabled( true );
  mMapCanvas = mapCanvas;
  mUndoView = NULL;
  mUndoStack = NULL;
}


void QgsUndoWidget::layerChanged( QgsMapLayer * layer)
{
   if (layer != NULL)
   {
      setUndoStack( layer->undoStack() );
   }
   else
   {
     destroyStack();
   }
   emit undoStackChanged();
}


void QgsUndoWidget::destroyStack()
{
   if (mUndoStack != NULL)
   {
     mUndoStack->clear();
     mUndoStack = NULL;
   }
   if (mUndoView != NULL)
   {
     mUndoView->close();
     mUndoView = NULL;
   }
}

void QgsUndoWidget::undoChanged(bool value)
{
   undoButton->setDisabled( !value );
   emit undoStackChanged();
}

void QgsUndoWidget::redoChanged(bool value)
{
   redoButton->setDisabled( !value );
   emit undoStackChanged();
}


void QgsUndoWidget::indexChanged(int value)
{
   //redoButton->setDisabled( !value );
   //canvas refresh
   mMapCanvas->refresh();
}

void QgsUndoWidget::undo( )
{
  if (mUndoStack)
    mUndoStack->undo();
}

void QgsUndoWidget::redo()
{
  if (mUndoStack)
    mUndoStack->redo();
}

void QgsUndoWidget::setUndoStack(QUndoStack* undoStack)
{
  if (mUndoView != NULL)
  {
     mUndoView->close();
     delete mUndoView;
     mUndoView = NULL;
  }
        
  mUndoStack = undoStack;
        
  mUndoView = new QUndoView(dockWidgetContents);
  mUndoView->setStack(undoStack); 
  mUndoView->setObjectName("undoView");
  gridLayout->addWidget(mUndoView, 0, 0, 1, 2);
  setWidget(dockWidgetContents);
  connect(this->mUndoStack,  SIGNAL(canUndoChanged(bool)), this, SLOT(undoChanged(bool)));
  connect(this->mUndoStack,  SIGNAL(canRedoChanged(bool)), this, SLOT(redoChanged(bool)));
  connect(this->mUndoStack,  SIGNAL(indexChanged(int)), this, SLOT(indexChanged(int)));

  this->undoButton->setDisabled( !mUndoStack->canUndo() );
  this->redoButton->setDisabled( !mUndoStack->canRedo() );
}



void QgsUndoWidget::setupUi(QDockWidget *UndoWidget)
{
	if (UndoWidget->objectName().isEmpty())
		UndoWidget->setObjectName(QString::fromUtf8("UndoWidget"));
	UndoWidget->resize(350, 223);
	UndoWidget->setMinimumSize(QSize(346, 220));
	dockWidgetContents = new QWidget(UndoWidget);
	dockWidgetContents->setObjectName(QString::fromUtf8("dockWidgetContents"));
	gridLayout = new QGridLayout(dockWidgetContents);
	gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
	gridLayout->setContentsMargins(0, 0, 0, 0);
	spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	
	gridLayout->addItem(spacerItem, 0, 0, 1, 1);
	
	undoButton = new QPushButton(dockWidgetContents);
	undoButton->setObjectName(QString::fromUtf8("undoButton"));
  undoButton->setIcon( QgisApp::instance()->getThemeIcon("mActionUndo.png") );
	undoButton->setIconSize(QSize(24, 24));
	
	gridLayout->addWidget(undoButton, 1, 0, 1, 1);
	
	redoButton = new QPushButton(dockWidgetContents);
	redoButton->setObjectName(QString::fromUtf8("redoButton"));
  redoButton->setIcon( QgisApp::instance()->getThemeIcon("mActionRedo.png") );
  redoButton->setIconSize(QSize(24, 24));
	
	gridLayout->addWidget(redoButton, 1, 1, 1, 1);
	
	spacerItem1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);
	
	gridLayout->addItem(spacerItem1, 0, 1, 1, 1);
	
	UndoWidget->setWidget(dockWidgetContents);
	
	retranslateUi(UndoWidget);
	
	QMetaObject::connectSlotsByName(UndoWidget);
} // setupUi

void QgsUndoWidget::retranslateUi(QDockWidget *UndoWidget)
{
  UndoWidget->setWindowTitle(QApplication::translate("UndoWidget", "Undo/Redo", 0, QApplication::UnicodeUTF8));
	undoButton->setText(QApplication::translate("UndoWidget", "Undo", 0, QApplication::UnicodeUTF8));
	redoButton->setText(QApplication::translate("UndoWidget", "Redo", 0, QApplication::UnicodeUTF8));
	Q_UNUSED(UndoWidget);
}

