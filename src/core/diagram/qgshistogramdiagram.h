/***************************************************************************
    qgshistogramdiagram.h
    ---------------------
    begin                : August 2012
    copyright            : (C) 2012 by Matthias Kuhn
    email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSHISTOGRAMDIAGRAM_H
#define QGSHISTOGRAMDIAGRAM_H

#define DIAGRAM_NAME_HISTOGRAM "Histogram"

#include "qgsdiagram.h"
#include "qgsfeature.h"
#include <QPen>
#include <QBrush>

class QPainter;
class QPointF;
class QgsDiagramSettings;
class QgsDiagramInterpolationSettings;

class QgsRenderContext;


class CORE_EXPORT QgsHistogramDiagram: public QgsDiagram
{
  public:
    QgsHistogramDiagram();
    ~QgsHistogramDiagram();

    virtual QgsDiagram* clone() const override;

    void renderDiagram( const QgsFeature& feature, QgsRenderContext& c, const QgsDiagramSettings& s, const QPointF& position ) override;

    QSizeF diagramSize( const QgsAttributes& attributes, const QgsRenderContext& c, const QgsDiagramSettings& s ) override;
    QSizeF diagramSize( const QgsFeature& feature, const QgsRenderContext& c, const QgsDiagramSettings& s, const QgsDiagramInterpolationSettings& is ) override;
    QString diagramName() const override { return DIAGRAM_NAME_HISTOGRAM; }

  private:
    QBrush mCategoryBrush;
    QPen   mPen;
    double mScaleFactor;
};


#endif // QGSHISTOGRAMDIAGRAM_H
