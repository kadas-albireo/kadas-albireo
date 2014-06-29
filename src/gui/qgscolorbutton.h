/***************************************************************************
    qgscolorbutton.h - Color button
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
#ifndef QGSCOLORBUTTON_H
#define QGSCOLORBUTTON_H

#include <QColorDialog>
#include <QPushButton>
#include <QTemporaryFile>

class QMimeData;

/** \ingroup gui
 * \class QgsColorButton
 * A cross platform button subclass for selecting colors. Will open a color chooser dialog when clicked.
 * Offers live updates to button from color chooser dialog
 * @note inherited base class moved from QToolButton to QPushButton in QGIS 1.9
 */

class GUI_EXPORT QgsColorButton: public QPushButton
{
    Q_OBJECT
    Q_PROPERTY( QString colorDialogTitle READ colorDialogTitle WRITE setColorDialogTitle )
    Q_PROPERTY( bool acceptLiveUpdates READ acceptLiveUpdates WRITE setAcceptLiveUpdates )
    Q_PROPERTY( QColor color READ color WRITE setColor )
    Q_FLAGS( QColorDialog::ColorDialogOptions )
    Q_PROPERTY( QColorDialog::ColorDialogOptions colorDialogOptions READ colorDialogOptions WRITE setColorDialogOptions )

  public:
    /**
     * Construct a new color button.
     *
     * @param parent The parent QWidget for the dialog
     * @param cdt The title to show in the color chooser dialog
     * @param cdo Options for the color chooser dialog
     * @note changed in 1.9
     */
    QgsColorButton( QWidget *parent = 0, QString cdt = "", QColorDialog::ColorDialogOptions cdo = 0 );
    ~QgsColorButton();

    /**
     * Specify the current color. Will emit a colorChanged signal if the color is different to the previous.
     *
     * @param color the new color
     * @note added in 1.9
     */
    void setColor( const QColor &color );
    /**
     * Return the currently selected color.
     *
     * @return the currently selected color
     * @note added in 1.9
     */
    QColor color() const;

    /**
     * Specify the options for the color chooser dialog (e.g. alpha).
     *
     * @param cdo Options for the color chooser dialog
     * @note added in 1.9
     */
    void setColorDialogOptions( QColorDialog::ColorDialogOptions cdo );

    /**
     * Returns the options for the color chooser dialog.
     *
     * @return Options for the color chooser dialog
     * @note added in 1.9
     */
    QColorDialog::ColorDialogOptions colorDialogOptions();

    /**
     * Set the title, which the color chooser dialog will show.
     *
     * @param cdt Title for the color chooser dialog
     * @note added in 1.9
     */
    void setColorDialogTitle( QString cdt );

    /**
     * Returns the title, which the color chooser dialog shows.
     *
     * @return Title for the color chooser dialog
     * @note added in 1.9
     */
    QString colorDialogTitle();

    /**
     * Whether the button accepts live updates from QColorDialog.
     *
     * @note added in 1.9
     */
    bool acceptLiveUpdates() { return mAcceptLiveUpdates; }

    /**
     * Sets whether the button accepts live updates from QColorDialog.
     * Live updates may cause changes that are not undoable on QColorDialog cancel.
     *
     * @note added in 1.9
     */
    void setAcceptLiveUpdates( bool accept ) { mAcceptLiveUpdates = accept; }

  public slots:
    /**
     * Sets the background pixmap for the button based upon set color and transparency.
     * Call directly to update background after adding/removing QColorDialog::ShowAlphaChannel option
     * but the color has not changed, i.e. setColor() wouldn't update button and
     * you want the button to retain the set color's alpha component regardless
     *
     * @note added in 1.9
     */
    void setButtonBackground();

  signals:
    /**
     * Is emitted, whenever a new color is accepted. The color is always valid.
     * In case the new color is the same, no signal is emitted, to avoid infinite loops.
     *
     * @param color New color
     * @note added in 1.9
     */
    void colorChanged( const QColor &color );

  protected:
    void changeEvent( QEvent* e );
    void showEvent( QShowEvent* e );
    static const QPixmap& transpBkgrd();

    /**
     * Reimplemented to detect right mouse button clicks on the color button and allow dragging colors
     */
    void mousePressEvent( QMouseEvent* e );

    /**
     * Reimplemented to allow dragging colors from button
     */
    void mouseMoveEvent( QMouseEvent *e );

    /**
     * Reimplemented to accept dragged colors
     */
    void dragEnterEvent( QDragEnterEvent * e ) ;

    /**
     * Reimplemented to accept dropped colors
     */
    void dropEvent( QDropEvent *e );

  private:
    QString mColorDialogTitle;
    QColor mColor;
    QColorDialog::ColorDialogOptions mColorDialogOptions;
    bool mAcceptLiveUpdates;
    QTemporaryFile mTempPNG;
    bool mColorSet; // added in QGIS 2.1

    QPoint mDragStartPosition;

    /**
     * Shows the color button context menu and handles copying and pasting color values.
     */
    void showContextMenu( QMouseEvent* event );

    /**
     * Creates mime data from the current color. Sets both the mime data's color data, and the
     * mime data's text with the color's hex code.
     * @note added in 2.3
     * @see colorFromMimeData
     */
    QMimeData* createColorMimeData() const;

    /**
     * Attempts to parse mimeData as a color, either via the mime data's color data or by
     * parsing a textual representation of a color.
     * @returns true if mime data could be intrepreted as a color
     * @param mimeData mime data
     * @param resultColor QColor to store evaluated color
     * @note added in 2.3
     * @see createColorMimeData
     */
    bool colorFromMimeData( const QMimeData *mimeData, QColor &resultColor );

#ifdef Q_OS_WIN
    /**
     * Expands a shortened Windows path to its full path name.
     * @returns full path name.
     * @param path a (possibly) shortened Windows path
     * @note added in 2.3
     */
    QString fullPath( const QString &path );
#endif

  private slots:
    void onButtonClicked();

    /**
     * Sets color for button, if valid.
     *
     * @note added in 1.9
     */
    void setValidColor( const QColor& newColor );
};

#endif
