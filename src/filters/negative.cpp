/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2018-2019 by Bogomolov Danila
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

#include "negative.h"

QString dFilterNegative::name() const
{
	return "Negative";
}

QPixmap dFilterNegative::applyFilter( const QPixmap &pixmap ) const
{
	QImage image = pixmap.toImage();
	for ( int i = 0; i < image.height(); i++ )
	{
		uchar *scan = image.scanLine( i );
		int depth = 4;
		for ( int j = 0; j < image.width(); j++ )
		{
			QRgb *rgbpixel = reinterpret_cast<QRgb*>( scan + j * depth );
			*rgbpixel = QColor( 255 - qRed( *rgbpixel ), 255 - qGreen( *rgbpixel ), 255 - qBlue( *rgbpixel ) ).rgba();
		}
	}
	return QPixmap::fromImage( image );
}
