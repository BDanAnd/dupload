/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2013 by Belov Nikita <null@deltaz.org>
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

#include "dpainter.h"

dPainter::dPainter( QWidget * /*parent*/ )
{
	m_currentAction.type = ACTION_NONE;
	m_currentAction.count = 0;
	m_opacity = 1.0;

	m_transparentLayer = new QLabel( this );
}

dPainter::~dPainter()
{
	while ( m_actions.size() )
		delete m_actions.pop().data;
}

void dPainter::drawPoint( const QPoint &point )
{
	m_currentAction.type = ACTION_POINT;
	m_currentAction.data = new QPoint( point );
	m_currentAction.opacity = m_opacity;
	m_currentAction.pen = m_pen;
	m_currentAction.count = 1;

	m_actions.push( m_currentAction );

	draw( &m_pixmap );
	QLabel::setPixmap( m_pixmap );

	m_currentAction.type = ACTION_NONE;
	m_currentAction.count = 0;
}

void dPainter::drawPath( const QPainterPath &path, bool end )
{
	m_currentAction.type = ACTION_PATH;
	m_currentAction.data = new QPainterPath( path );
	m_currentAction.opacity = m_opacity;
	m_currentAction.pen = m_pen;
	m_currentAction.count++;

	m_actions.push( m_currentAction );

	draw( &m_transparentPixmap );

	if ( end )
	{
		m_currentAction.count = 0;
		finalize();
	}

	m_currentAction.type = ACTION_NONE;
}

double dPainter::opacity()
{
	return m_opacity;
}

void dPainter::setOpacity( double opacity )
{
	m_opacity = opacity;
}

const QPen &dPainter::pen()
{
	return m_pen;
}

void dPainter::setPen( const QPen &pen )
{
	m_pen = pen;
}

const QPixmap &dPainter::pixmap()
{
	return m_pixmap;
}

void dPainter::setPixmap( const QPixmap &pixmap )
{
	m_pixmap = m_pixmap_orig = pixmap;
	QLabel::setPixmap( m_pixmap );

	m_transparentPixmap = QPixmap( m_pixmap.size() );
	m_transparentPixmap.fill( Qt::transparent );

	m_transparentLayer->setPixmap( m_transparentPixmap );
}

void dPainter::undo()
{
	if ( !m_actions.size() )
		return;

	for ( int count = m_actions.last().count; count > 0; count-- )
		delete m_actions.pop().data;

	m_pixmap = m_pixmap_orig;

	foreach ( m_currentAction, m_actions )
	{
		if ( m_currentAction.count == 1 )
			finalize();

		draw( &m_transparentPixmap, false );
	}

	finalize();

	m_currentAction.type = ACTION_NONE;
	m_currentAction.count = 0;
}

void dPainter::draw( QPaintDevice *device, bool visible )
{
	QPainter painter( device );

	bool transparentPixmap = device == &m_transparentPixmap;

	if ( transparentPixmap )
		painter.setCompositionMode( QPainter::CompositionMode_Source );
		
	painter.setOpacity( m_currentAction.opacity );
	painter.setPen( m_currentAction.pen );
	painter.setRenderHint( QPainter::Antialiasing );
	painter.setRenderHint( QPainter::HighQualityAntialiasing );

	if ( m_currentAction.type == ACTION_POINT )
	{
		painter.drawPoint( *( QPoint * )m_currentAction.data );
	}
	else if ( m_currentAction.type == ACTION_PATH )
	{
		painter.drawPath( *( QPainterPath * )m_currentAction.data );
	}

	if ( transparentPixmap && visible )
		m_transparentLayer->setPixmap( m_transparentPixmap );
}

void dPainter::finalize()
{
	QPainter painter( &m_pixmap );

	painter.drawPixmap( 0, 0, m_transparentPixmap );
	QLabel::setPixmap( m_pixmap );

	m_transparentPixmap.fill( Qt::transparent );
	m_transparentLayer->setPixmap( m_transparentPixmap );
}
