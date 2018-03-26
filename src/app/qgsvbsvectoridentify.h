/***************************************************************************
    qgsvbsvectoridentify.h
    ----------------------
    begin                : March 2018
    copyright            : (C) 2018 by Sandro Mani
    email                : manisandro@gmail.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSVBSVECTORIDENTIFY_H
#define QGSVBSVECTORIDENTIFY_H

#include <QDialog>
#include <QPointer>

class QgisApp;
class QgsVectorLayer;
class QgsFeature;
class QgsGeometryRubberBand;


class APP_EXPORT QgsVBSVectorIdentifyResultDialog : public QDialog
{
    Q_OBJECT
  public:
    QgsVBSVectorIdentifyResultDialog( const QgsVectorLayer* layer, const QgsFeature& feature, QgisApp* app );
    ~QgsVBSVectorIdentifyResultDialog();

  private:
    QPointer<QgsGeometryRubberBand> mRubberBand;
};


#endif // QGSVBSVECTORIDENTIFY_H
