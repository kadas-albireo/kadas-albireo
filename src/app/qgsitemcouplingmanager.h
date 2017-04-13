/***************************************************************************
    qgsitemcouplingmanager.h
     --------------------------------------
    Date                 : Apr 2017
    Copyright            : (C) 2017 Sandro Mani / Sourcepole AG
    Email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSITEMCOUPLINGMANAGER_H
#define QGSITEMCOUPLINGMANAGER_H

#include <QObject>

class QDomDocument;
class QgsAnnotationItem;
class QgsMapLayer;


class APP_EXPORT QgsItemCoupling : public QObject
{
    Q_OBJECT
  public:
    QgsItemCoupling( QgsMapLayer* layer, QgsAnnotationItem* annotation );
    const QgsMapLayer* layer() const { return mLayer; }
    const QgsAnnotationItem* annotation() const { return mAnnotation; }
  private:
    QgsMapLayer* mLayer;
    QgsAnnotationItem* mAnnotation;
  signals:
    void couplingDissolved();
  private slots:
    void layerRemoved();
    void annotationRemoved();
};


class APP_EXPORT QgsItemCouplingManager : public QObject
{
    Q_OBJECT
  public:
    QgsItemCouplingManager( QObject* parent = 0 ) : QObject( parent ) {}
    ~QgsItemCouplingManager();
    void addCoupling( QgsMapLayer* layer, QgsAnnotationItem* annotation );
    void clear();
    void writeXml( QDomDocument &doc ) const;
    void readXml( const QDomDocument& doc );

  private:
    QList<QgsItemCoupling*> mCouplings;

  private slots:
    void removeCoupling();
};

#endif // QGSITEMCOUPLINGMANAGER_H
