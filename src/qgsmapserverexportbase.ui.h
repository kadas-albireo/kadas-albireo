/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/
#include <QFileDialog>

void QgsMapserverExportBase::exportLayersOnly()
{
    // disable inputs if only layer objects are being written
    grpMap->setEnabled(!chkExpLayersOnly->isChecked());
    grpWeb->setEnabled(!chkExpLayersOnly->isChecked());
}

void QgsMapserverExportBase::chooseMapFile()
{
QString s = QFileDialog::getSaveFileName(
                    this,
                    "Choose a filename for the exported map file",
                    "./",
                    "Mapserver files (*.map)" );
txtMapFilePath->setText(s);
}


void QgsMapserverExportBase::showHelp()
{

}
