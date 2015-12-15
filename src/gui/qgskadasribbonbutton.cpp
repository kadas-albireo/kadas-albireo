#include "qgskadasribbonbutton.h"
#include <QDragEnterEvent>
#include <QPainter>

QgsKadasRibbonButton::QgsKadasRibbonButton( QWidget* parent ): QToolButton( parent )
{
}

QgsKadasRibbonButton::~QgsKadasRibbonButton()
{
}

void QgsKadasRibbonButton::paintEvent( QPaintEvent* e )
{
  QPainter p( this );
  p.setRenderHint( QPainter::Antialiasing );

  QPalette plt;

  //background
  p.setBrush( QBrush( palette().color( QPalette::Window ) ) );
  if ( isChecked() )
  {
    QPen checkedPen( palette().color( QPalette::Dark ) );
    //QPen checkedPen( Qt::red );
    checkedPen.setWidth( 3 );
    p.setPen( checkedPen );
  }
  else if ( underMouse() )
  {
    QPen underMousePen( palette().color( QPalette::Highlight ) );
    underMousePen.setWidth( 2 );
    p.setPen( underMousePen );
  }
  else if ( hasFocus() )
  {
    p.setPen( palette().color( QPalette::Highlight ) );
  }
  else
  {
    p.setPen( QColor( 255, 255, 255, 0 ) );
  }
  p.drawRoundedRect( 0, 0, width(), height(), 5, 5 );

  int iconBottomY = 0;
  bool smallIcon = false;
  if ( iconSize().height() <= 20 )
  {
    smallIcon = true;
  }

  if ( smallIcon )
  {
    iconBottomY = height() / 2.0 - 5.0;
  }
  else
  {
    iconBottomY = height() / 2.0 + 5;
  }

  //icon
  QIcon buttonIcon = icon();
  if ( !buttonIcon.isNull() )
  {
    QSize iSize = iconSize();
    QPixmap pixmap = buttonIcon.pixmap( iSize, QIcon::Normal, QIcon::On );
    int pixmapY = iconBottomY - iSize.height();
    int pixmapX = width() / 2.0 - iSize.width() / 2.0;
    p.drawPixmap( pixmapX, pixmapY, pixmap );
  }

  //text
  QString buttonText = text();
  if ( !buttonText.isEmpty() )
  {
    QFontMetricsF fm( font() );
    p.setFont( font() );
    p.setPen( QPen( palette().color( QPalette::Text ) ) );

    QStringList rawRextLines = buttonText.split( "\n" );
    // Insert additional line breaks where exceeds button width
    QStringList textLines;
    int maxWidth = width() - 10;
    foreach ( const QString& line, rawRextLines )
    {
      if ( fm.width( line ) > maxWidth )
      {
        QString newline;
        // Measure widths at whitespace pos, fit as many words into a line as possible
        foreach ( const QString& word, line.split( " " ) )
        {
          if ( fm.width( newline + " " + word ) > maxWidth )
          {
            if ( !newline.isEmpty() )
            {
              textLines.append( newline );
            }
            newline = word;
          }
          else
          {
            newline += QString( " " ) + word;
          }
        }
        textLines.append( newline );
      }
      else
      {
        textLines.append( line );
      }
    }

    for ( int i = 0; i < textLines.size(); ++i )
    {
      QString textLine = textLines.at( i );
      double textWidth = fm.width( textLine );
      double textHeight = fm.boundingRect( textLine ).height();
      int textX = ( width() - textWidth ) / 2.0;
      int textY = iconBottomY + textHeight * ( i + 1 ) /*+ i * 1*/;
      if ( smallIcon )
      {
        textY += 10;
      }
      p.drawText( textX, textY, textLine );
    }
  }
}

void QgsKadasRibbonButton::enterEvent( QEvent* event )
{
  update();
  QAbstractButton::enterEvent( event );
}

void QgsKadasRibbonButton::leaveEvent( QEvent* event )
{
  update();
  QAbstractButton::leaveEvent( event );
}

void QgsKadasRibbonButton::focusInEvent( QFocusEvent* event )
{
  update();
  QAbstractButton::focusInEvent( event );
}

void QgsKadasRibbonButton::focusOutEvent( QFocusEvent* event )
{
  update();
  QAbstractButton::focusOutEvent( event );
}


void QgsKadasRibbonButton::mousePressEvent( QMouseEvent* event )
{
  QAbstractButton::mousePressEvent( event );
  event->ignore();
}

void QgsKadasRibbonButton::mouseMoveEvent( QMouseEvent* event )
{
  event->ignore();
}
