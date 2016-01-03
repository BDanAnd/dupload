/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2015 by Belov Nikita <null@deltaz.org>
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

#include "dfilter_monochrome.h"

dFilterMonochrome::dFilterMonochrome() : dFilterAbstract()
{
}

dFilterMonochrome::~dFilterMonochrome()
{
}

QString dFilterMonochrome::name()
{
	return "Monochrome";
}

QPixmap dFilterMonochrome::applyFilter(const QPixmap &pixmap)
{
	QImage image = pixmap.toImage();
	for (int i = 0; i < image.height(); i++) {
		uchar *scan = image.scanLine(i);
		int depth = 4;
		for (int j = 0; j < image.width(); j++) {
			QRgb *rgbpixel = reinterpret_cast<QRgb*>(scan + j * depth);
			int gray = qGray(*rgbpixel);
			*rgbpixel = QColor(gray, gray, gray).rgba();
		}
	}
	return QPixmap::fromImage(image);
}
