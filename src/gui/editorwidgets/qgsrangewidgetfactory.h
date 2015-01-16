/***************************************************************************
    qgsrangewidgetfactory.h
     --------------------------------------
    Date                 : 5.1.2014
    Copyright            : (C) 2014 Matthias Kuhn
    Email                : matthias dot kuhn at gmx dot ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSRANGEWIDGETFACTORY_H
#define QGSRANGEWIDGETFACTORY_H

#include "qgseditorwidgetfactory.h"

class GUI_EXPORT QgsRangeWidgetFactory : public QgsEditorWidgetFactory
{
  public:
    QgsRangeWidgetFactory( QString name );

    // QgsEditorWidgetFactory interface
  public:
    virtual QgsEditorWidgetWrapper* create( QgsVectorLayer* vl, int fieldIdx, QWidget* editor, QWidget* parent ) const override;
    virtual QgsEditorConfigWidget* configWidget( QgsVectorLayer* vl, int fieldIdx, QWidget* parent ) const override;
    virtual QgsEditorWidgetConfig readConfig( const QDomElement& configElement, QgsVectorLayer* layer, int fieldIdx ) override;
    virtual void writeConfig( const QgsEditorWidgetConfig& config, QDomElement& configElement, QDomDocument& doc, const QgsVectorLayer* layer, int fieldIdx ) override;

  private:
    virtual bool isFieldSupported( QgsVectorLayer *vl, int fieldIdx ) override;
};

#endif // QGSRANGEWIDGETFACTORY_H
