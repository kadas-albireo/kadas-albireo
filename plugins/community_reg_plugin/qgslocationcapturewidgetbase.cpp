/****************************************************************************
** Form implementation generated from reading ui file 'qgslocationcapturewidgetbase.ui'
**
** Created: Sun Apr 24 15:11:25 2005
**      by: The User Interface Compiler ($Id$)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "qgslocationcapturewidgetbase.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include "qgslocationcapturewidgetbase.ui.h"
//Added by qt3to4:
#include <Q3GridLayout>

/*
 *  Constructs a QgsLocationCaptureWidgetBase as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
QgsLocationCaptureWidgetBase::QgsLocationCaptureWidgetBase( QWidget* parent, const char* name, Qt::WFlags fl )
    : QWidget( parent, name, fl )
{
    if ( !name )
	setName( "QgsLocationCaptureWidgetBase" );
    QgsLocationCaptureWidgetBaseLayout = new Q3GridLayout( this, 1, 1, 11, 6, "QgsLocationCaptureWidgetBaseLayout"); 

    qgsMapCanvas = new QgsMapCanvas( this, "qgsMapCanvas" );

    QgsLocationCaptureWidgetBaseLayout->addMultiCellWidget( qgsMapCanvas, 0, 0, 0, 3 );

    pbnZoomIn = new QPushButton( this, "pbnZoomIn" );

    QgsLocationCaptureWidgetBaseLayout->addWidget( pbnZoomIn, 2, 0 );

    pbnPan = new QPushButton( this, "pbnPan" );

    QgsLocationCaptureWidgetBaseLayout->addWidget( pbnPan, 2, 2 );

    pbnZoomOut = new QPushButton( this, "pbnZoomOut" );

    QgsLocationCaptureWidgetBaseLayout->addWidget( pbnZoomOut, 2, 1 );

    pbnCapturePos = new QPushButton( this, "pbnCapturePos" );

    QgsLocationCaptureWidgetBaseLayout->addWidget( pbnCapturePos, 2, 3 );

    pushButton5 = new QPushButton( this, "pushButton5" );

    QgsLocationCaptureWidgetBaseLayout->addMultiCellWidget( pushButton5, 3, 3, 0, 3 );

    lblCapturePos = new QLabel( this, "lblCapturePos" );
    lblCapturePos->setMaximumSize( QSize( 32767, 20 ) );

    QgsLocationCaptureWidgetBaseLayout->addMultiCellWidget( lblCapturePos, 1, 1, 2, 3 );

    lblCurrentPos = new QLabel( this, "lblCurrentPos" );

    QgsLocationCaptureWidgetBaseLayout->addMultiCellWidget( lblCurrentPos, 1, 1, 0, 1 );
    languageChange();
    resize( QSize(451, 370).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( qgsMapCanvas, SIGNAL( xyClickCoordinates(QgsPoint&) ), this, SLOT( qgsMapCanvas_xyClickCoordinates(QgsPoint&) ) );
    connect( qgsMapCanvas, SIGNAL( xyCoordinates(QgsPoint&) ), this, SLOT( qgsMapCanvas_xyCoordinates(QgsPoint&) ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
QgsLocationCaptureWidgetBase::~QgsLocationCaptureWidgetBase()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void QgsLocationCaptureWidgetBase::languageChange()
{
    setCaption( tr( "Form1" ) );
    pbnZoomIn->setText( tr( "ZoomIn" ) );
    pbnPan->setText( tr( "Pan" ) );
    pbnZoomOut->setText( tr( "Zoom Out" ) );
    pbnCapturePos->setText( tr( "Capture Pos" ) );
    pushButton5->setText( tr( "OK" ) );
    lblCapturePos->setText( tr( "Captured Pos:" ) );
    lblCurrentPos->setText( tr( "Current Pos:" ) );
}

