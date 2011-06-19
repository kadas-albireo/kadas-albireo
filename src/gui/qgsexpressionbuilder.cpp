/***************************************************************************
    qgisexpressionbuilder.cpp - A genric expression string builder widget.
     --------------------------------------
    Date                 :  29-May-2011
    Copyright            : (C) 2006 by Nathan Woodrow
    Email                : nathan.woodrow at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsexpressionbuilder.h"
#include "ui_qgsexpressionbuilder.h"

QgsExpressionBuilder::QgsExpressionBuilder(QWidget *parent) :
    QWidget(parent)
    
{
    setupUi(this);
}

QgsExpressionBuilder::~QgsExpressionBuilder()
{
    
}

QString QgsExpressionBuilder::getExpressionString()
{
    return this->txtExpressionString->toPlainText();
}

void QgsExpressionBuilder::setExpressionString(const QString expressionString)
{
    this->txtExpressionString->setPlainText(expressionString);
}

