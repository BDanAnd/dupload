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

#include "black_and_white.h"
#include <QPushButton>
#include <QVBoxLayout>

QString dFilterBlackAndWhite::name() const
{
	return "Black & White";
}

void dFilterBlackAndWhite::setPixmap( const QPixmap &pixmap ) const
{
	m_original = &pixmap;
}

QWidget * dFilterBlackAndWhite::getWidget() const
{
	QWidget *w = new QWidget;

	slider = new QSlider( Qt::Horizontal );
	slider->setTickPosition( QSlider::TicksBothSides );
	slider->setTickInterval( 15 );
	slider->setSingleStep( 1 );
	slider->setRange( 0, 255 );
	slider->setValue( 127 );
	connect(slider, &QSlider::valueChanged, this, &dFilterBlackAndWhite::applyFilter);

	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget( slider );

	w->setLayout( layout );

	return w;
}

#include<QDebug>

QPixmap dFilterBlackAndWhite::applyFilter() const
{
	int threshold = slider->value();
	qInfo() << threshold;

	QImage image = m_original->toImage();
	for ( int i = 0; i < image.height(); i++ )
	{
		uchar *scan = image.scanLine( i );
		int depth = 4;
		for ( int j = 0; j < image.width(); j++ )
		{
			QRgb *rgbpixel = reinterpret_cast<QRgb*>( scan + j * depth );
			int gray = qGray( *rgbpixel );
			if (gray >= threshold )
			{
				*rgbpixel = QColor( Qt::white ).rgba();
			}
			else
			{
				*rgbpixel = QColor( Qt::black ).rgba();
			}
		}
	}
	return QPixmap::fromImage( image );
}
