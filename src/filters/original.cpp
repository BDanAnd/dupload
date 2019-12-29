/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2015 by Belov Nikita <null@deltaz.org>
 *                2018-2019 by Bogomolov Danila
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

#include "original.h"

QString dFilterOriginal::name() const
{
	return "Original";
}

QPixmap dFilterOriginal::applyFilter( const QPixmap &pixmap ) const
{
	return pixmap;
}
