/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2019-2020 by Bogomolov Danila
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

#ifndef DFILTERINTERFACE_H
#define DFILTERINTERFACE_H

#include <QString>
#include <QtGui/QPixmap>
#include <QtPlugin>

class dFilterInterface
{
public:
	virtual ~dFilterInterface() {}

	virtual QString name() const = 0;
	virtual void setPixmap( const QPixmap &pixmap ) const = 0;
	virtual QWidget * getWidget() const = 0;
	virtual QPixmap applyFilter() const = 0;

protected:
	mutable const QPixmap *m_original;
};

#define dFilterInterface_iid "org.deltaz.dUpload.dFilterInterface/1.0"

Q_DECLARE_INTERFACE( dFilterInterface, dFilterInterface_iid )

Q_DECLARE_METATYPE( dFilterInterface * )

#endif // DFILTERINTERFACE_H
