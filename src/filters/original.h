/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2015 by Belov Nikita <null@deltaz.org>
 *                2018 by Bogomolov Danila
 *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************
*****************************************************************************/

#ifndef DFILTERORIGINAL_H
#define DFILTERORIGINAL_H

#include <QObject>
#include <QtPlugin>
#include "dfilterinterface.h"

class dFilterOriginal : public QObject, dFilterInterface
{
    Q_OBJECT
	Q_PLUGIN_METADATA( IID dFilterInterface_iid )
	Q_INTERFACES(dFilterInterface)

public:
	QString name() const override;
	QPixmap applyFilter(const QPixmap &pixmap) const override;
};

#endif // DFILTERORIGINAL_H
