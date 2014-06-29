/***************************************************************************
    qgscolorbutton.cpp - Button which displays a color
     --------------------------------------
    Date                 : 12-Dec-2006
    Copyright            : (C) 2006 by Tom Elwertowski
    Email                : telwertowski at users dot sourceforge dot net
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscolorbutton.h"
#include "qgscolordialog.h"
#include "qgsapplication.h"
#include "qgslogger.h"
#include "qgssymbollayerv2utils.h"

#include <QPainter>
#include <QSettings>
#include <QTemporaryFile>
#include <QMouseEvent>
#include <QMenu>
#include <QClipboard>
#include <QDrag>

#ifdef Q_OS_WIN
#include <windows.h>
QString QgsColorButton::fullPath( const QString &path )
{
  TCHAR buf[MAX_PATH];
  int len = GetLongPathName( path.toUtf8().constData(), buf, MAX_PATH );

  if ( len == 0 || len > MAX_PATH )
  {
    QgsDebugMsg( QString( "GetLongPathName('%1') failed with %2: %3" )
                 .arg( path ).arg( len ).arg( GetLastError() ) );
    return path;
  }

  QString res = QString::fromUtf8( buf );
  return res;
}
#endif

/*!
  \class QgsColorButton

  \brief A cross platform button subclass for selecting colors. Will open a color chooser dialog when clicked.
  Offers live updates to button from color chooser dialog

  A subclass of QPushButton is needed to draw the button content because
  some platforms such as Mac OS X and Windows XP enforce a consistent
  GUI look by always using the button color of the current style and
  not allowing button backgrounds to be changed on a button by button basis.
  Therefore, a wholely stylesheet-based button is used for the no-text variant.

  This class is a simplified version of QtColorButton, an internal class used
  by Qt Designer to do the same thing.
*/

QgsColorButton::QgsColorButton( QWidget *parent, QString cdt, QColorDialog::ColorDialogOptions cdo )
    : QPushButton( parent )
    , mColorDialogTitle( cdt.isEmpty() ? tr( "Select Color" ) : cdt )
    , mColor( Qt::black )
    , mColorDialogOptions( cdo )
    , mAcceptLiveUpdates( true )
    , mTempPNG( NULL )
    , mColorSet( false )
{
  setAcceptDrops( true );
  connect( this, SIGNAL( clicked() ), this, SLOT( onButtonClicked() ) );
}

QgsColorButton::~QgsColorButton()
{
  if ( mTempPNG.exists() )
    mTempPNG.remove();
}

const QPixmap& QgsColorButton::transpBkgrd()
{
  static QPixmap transpBkgrd;

  if ( transpBkgrd.isNull() )
    transpBkgrd = QgsApplication::getThemePixmap( "/transp-background_8x8.png" );

  return transpBkgrd;
}

void QgsColorButton::onButtonClicked()
{
  //QgsDebugMsg( "entered" );
  QColor newColor;
  QSettings settings;
  if ( mAcceptLiveUpdates && settings.value( "/qgis/live_color_dialogs", false ).toBool() )
  {
    newColor = QgsColorDialog::getLiveColor(
                 color(), this, SLOT( setValidColor( const QColor& ) ),
                 this->parentWidget(), mColorDialogTitle, mColorDialogOptions );
  }
  else
  {
    newColor = QColorDialog::getColor( color(), this->parentWidget(), mColorDialogTitle, mColorDialogOptions );
  }
  setValidColor( newColor );

  // reactivate button's window
  activateWindow();
}

void QgsColorButton::mousePressEvent( QMouseEvent *e )
{
  if ( e->button() == Qt::RightButton )
  {
    showContextMenu( e );
    return;
  }
  else if ( e->button() == Qt::LeftButton )
  {
    mDragStartPosition = e->pos();
  }
  QPushButton::mousePressEvent( e );
}

QMimeData * QgsColorButton::createColorMimeData() const
{
  QMimeData *mimeData = new QMimeData;
  mimeData->setColorData( QVariant( mColor ) );
  mimeData->setText( mColor.name() );
  return mimeData;
}

bool QgsColorButton::colorFromMimeData( const QMimeData * mimeData, QColor& resultColor )
{
  //attempt to read color data directly from mime
  QColor mimeColor = mimeData->colorData().value<QColor>();
  if ( mimeColor.isValid() )
  {
    if ( !( mColorDialogOptions & QColorDialog::ShowAlphaChannel ) )
    {
      //remove alpha channel
      mimeColor.setAlpha( 255 );
    }
    resultColor = mimeColor;
    return true;
  }

  //attempt to intrepret a color from mime text data
  bool hasAlpha = false;
  QColor textColor = QgsSymbolLayerV2Utils::parseColorWithAlpha( mimeData->text(), hasAlpha );
  if ( textColor.isValid() )
  {
    if ( !( mColorDialogOptions & QColorDialog::ShowAlphaChannel ) )
    {
      //remove alpha channel
      textColor.setAlpha( 255 );
    }
    else if ( !hasAlpha )
    {
      //mime color has no explicit alpha component, so keep existing alpha
      textColor.setAlpha( mColor.alpha() );
    }
    resultColor = textColor;
    return true;
  }

  //could not get color from mime data
  return false;
}

void QgsColorButton::mouseMoveEvent( QMouseEvent *e )
{
  if ( !( e->buttons() & Qt::LeftButton ) )
  {
    QPushButton::mouseMoveEvent( e );
    return;
  }

  if (( e->pos() - mDragStartPosition ).manhattanLength() < QApplication::startDragDistance() )
  {
    QPushButton::mouseMoveEvent( e );
    return;
  }

  QDrag *drag = new QDrag( this );
  drag->setMimeData( createColorMimeData() );

  //craft a pixmap for the drag icon
  QImage colorImage( 50, 50, QImage::Format_RGB32 );
  QPainter imagePainter;
  imagePainter.begin( &colorImage );
  //start with a light gray background
  imagePainter.fillRect( QRect( 0, 0, 50, 50 ), QBrush( QColor( 200, 200, 200 ) ) );
  //draw rect with white border, filled with current color
  QColor pixmapColor = mColor;
  pixmapColor.setAlpha( 255 );
  imagePainter.setBrush( QBrush( pixmapColor ) );
  imagePainter.setPen( QPen( Qt::white ) );
  imagePainter.drawRect( QRect( 1, 1, 47, 47 ) );
  imagePainter.end();
  //set as drag pixmap
  drag->setPixmap( QPixmap::fromImage( colorImage ) );

  drag->exec( Qt::CopyAction );
  setDown( false );
}

void QgsColorButton::dragEnterEvent( QDragEnterEvent *e )
{
  //is dragged data valid color data?
  QColor mimeColor;
  if ( colorFromMimeData( e->mimeData(), mimeColor ) )
  {
    e->acceptProposedAction();
  }
}

void QgsColorButton::dropEvent( QDropEvent *e )
{
  //is dropped data valid color data?
  QColor mimeColor;
  if ( colorFromMimeData( e->mimeData(), mimeColor ) )
  {
    e->acceptProposedAction();
    setColor( mimeColor );
  }
}

void QgsColorButton::showContextMenu( QMouseEvent *event )
{
  QMenu colorContextMenu;

  QAction* copyColorAction = new QAction( tr( "Copy color" ), 0 );
  colorContextMenu.addAction( copyColorAction );
  QAction* pasteColorAction = new QAction( tr( "Paste color" ), 0 );
  pasteColorAction->setEnabled( false );
  colorContextMenu.addSeparator();
  colorContextMenu.addAction( pasteColorAction );

  QColor clipColor;
  if ( colorFromMimeData( QApplication::clipboard()->mimeData(), clipColor ) )
  {
    pasteColorAction->setEnabled( true );
  }

  QAction* selectedAction = colorContextMenu.exec( event->globalPos( ) );
  if ( selectedAction == copyColorAction )
  {
    //copy color
    QApplication::clipboard()->setMimeData( createColorMimeData() );
  }
  else if ( selectedAction == pasteColorAction )
  {
    //paste color
    setColor( clipColor );
  }

  delete copyColorAction;
  delete pasteColorAction;
}

void QgsColorButton::setValidColor( const QColor& newColor )
{
  if ( newColor.isValid() )
  {
    setColor( newColor );
  }
}

void QgsColorButton::changeEvent( QEvent* e )
{
  if ( e->type() == QEvent::EnabledChange )
  {
    setButtonBackground();
  }
  QPushButton::changeEvent( e );
}

#if 0 // causes too many cyclical updates, but may be needed on some platforms
void QgsColorButton::paintEvent( QPaintEvent* e )
{
  QPushButton::paintEvent( e );

  if ( !mBackgroundSet )
  {
    setButtonBackground();
  }
}
#endif

void QgsColorButton::showEvent( QShowEvent* e )
{
  setButtonBackground();
  QPushButton::showEvent( e );
}

void QgsColorButton::setColor( const QColor &color )
{
  if ( !color.isValid() )
  {
    return;
  }
  QColor oldColor = mColor;
  mColor = color;

  // handle when initially set color is same as default (Qt::black); consider it a color change
  if ( oldColor != mColor || ( mColor == QColor( Qt::black ) && !mColorSet ) )
  {
    setButtonBackground();
    if ( isEnabled() )
    {
      // TODO: May be beneficial to have the option to set color without emitting this signal.
      //       Now done by blockSignals( bool ) where button is used
      emit colorChanged( mColor );
    }
  }
  mColorSet = true;
}

void QgsColorButton::setButtonBackground()
{
  if ( !text().isEmpty() )
  {
    // generate icon pixmap for regular pushbutton
    setFlat( false );

    QPixmap pixmap;
    pixmap = QPixmap( iconSize() );
    pixmap.fill( QColor( 0, 0, 0, 0 ) );

    int iconW = iconSize().width();
    int iconH = iconSize().height();
    QRect rect( 0, 0, iconW, iconH );

    // QPainterPath::addRoundRect has flaws, draw chamfered corners instead
    QPainterPath roundRect;
    int chamfer = 3;
    int inset = 1;
    roundRect.moveTo( chamfer, inset );
    roundRect.lineTo( iconW - chamfer, inset );
    roundRect.lineTo( iconW - inset, chamfer );
    roundRect.lineTo( iconW - inset, iconH - chamfer );
    roundRect.lineTo( iconW - chamfer, iconH - inset );
    roundRect.lineTo( chamfer, iconH - inset );
    roundRect.lineTo( inset, iconH - chamfer );
    roundRect.lineTo( inset, chamfer );
    roundRect.closeSubpath();

    QPainter p;
    p.begin( &pixmap );
    p.setRenderHint( QPainter::Antialiasing );
    p.setClipPath( roundRect );
    p.setPen( Qt::NoPen );
    if ( mColor.alpha() < 255 )
    {
      p.drawTiledPixmap( rect, transpBkgrd() );
    }
    p.setBrush( mColor );
    p.drawRect( rect );
    p.end();

    // set this pixmap as icon
    setIcon( QIcon( pixmap ) );
  }
  else
  {
    // generate temp background image file with checkerboard canvas to be used via stylesheet

    // set flat, or inline spacing (widget margins) needs to be manually calculated and set
    setFlat( true );

    bool useAlpha = ( mColorDialogOptions & QColorDialog::ShowAlphaChannel );

    // in case margins need to be adjusted
    QString margin = QString( "%1px %2px %3px %4px" ).arg( 0 ).arg( 0 ).arg( 0 ).arg( 0 );

    //QgsDebugMsg( QString( "%1 margin: %2" ).arg( objectName() ).arg( margin ) );

    QString bkgrd = QString( " background-color: rgba(%1,%2,%3,%4);" )
                    .arg( mColor.red() )
                    .arg( mColor.green() )
                    .arg( mColor.blue() )
                    .arg( useAlpha ? mColor.alpha() : 255 );

    if ( useAlpha && mColor.alpha() < 255 )
    {
      QPixmap pixmap = transpBkgrd();
      QRect rect( 0, 0, pixmap.width(), pixmap.height() );

      QPainter p;
      p.begin( &pixmap );
      p.setRenderHint( QPainter::Antialiasing );
      p.setPen( Qt::NoPen );
      p.setBrush( mColor );
      p.drawRect( rect );
      p.end();

      if ( mTempPNG.open() )
      {
        mTempPNG.setAutoRemove( false );
        pixmap.save( mTempPNG.fileName(), "PNG" );
        mTempPNG.close();
      }

      QString bgFileName = mTempPNG.fileName();
#ifdef Q_OS_WIN
      //on windows, mTempPNG will use a shortened path for the temporary folder name
      //this does not work with stylesheets, resulting in the whole button disappearing (#10187)
      bgFileName = fullPath( bgFileName );
#endif
      bkgrd = QString( " background-image: url(%1);" ).arg( bgFileName );
    }

    // TODO: get OS-style focus color and switch border to that color when button in focus
    setStyleSheet( QString( "QgsColorButton{"
                            " %1"
                            " background-position: top left;"
                            " background-origin: content;"
                            " background-clip: content;"
                            " padding: 2px;"
                            " margin: %2;"
                            " outline: none;"
                            " border-style: %4;"
                            " border-width: 1px;"
                            " border-color: rgb(%3,%3,%3);"
                            " border-radius: 3px;} "
                            "QgsColorButton:pressed{"
                            " %1"
                            " background-position: top left;"
                            " background-origin: content;"
                            " background-clip: content;"
                            " padding: 1px;"
                            " margin: %2;"
                            " outline: none;"
                            " border-style: inset;"
                            " border-width: 2px;"
                            " border-color: rgb(128,128,128);"
                            " border-radius: 4px;} " )
                   .arg( bkgrd )
                   .arg( margin )
                   .arg( isEnabled() ? "128" : "110" )
                   .arg( isEnabled() ? "outset" : "dotted" ) );
  }
}

QColor QgsColorButton::color() const
{
  return mColor;
}

void QgsColorButton::setColorDialogOptions( QColorDialog::ColorDialogOptions cdo )
{
  mColorDialogOptions = cdo;
}

QColorDialog::ColorDialogOptions QgsColorButton::colorDialogOptions()
{
  return mColorDialogOptions;
}

void QgsColorButton::setColorDialogTitle( QString cdt )
{
  mColorDialogTitle = cdt;
}

QString QgsColorButton::colorDialogTitle()
{
  return mColorDialogTitle;
}
