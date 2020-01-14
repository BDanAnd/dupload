/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2020 by Bogomolov Danila
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

#ifndef DFILTERBLACKANDWHITE_H
#define DFILTERBLACKANDWHITE_H

#include <QObject>
#include <QtPlugin>
#include <QSlider>
#include "dfilterinterface.h"

class dFilterBlackAndWhite : public QObject, dFilterInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA( IID dFilterInterface_iid )
	Q_INTERFACES( dFilterInterface )

public:
	QString name() const override;
	void setPixmap( const QPixmap &pixmap ) const override;
	QPixmap applyFilter() const override;
	QWidget * getWidget() const override;

private:
	mutable QSlider *slider;
};

#endif // DFILTERBLACKANDWHITE_H

