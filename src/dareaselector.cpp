/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2012, 2015 by Belov Nikita <null@deltaz.org>
 *                      2015 by Bogomolov Danila
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

#include "dareaselector.h"
#include "ddesktopmanager.h"

dAreaSelector::dAreaSelector( dUpload *d ) : m_dupload( d )
{
	setAttribute( Qt::WA_DeleteOnClose );
	setAttribute( Qt::WA_QuitOnClose, false );

	m_processSelection = false;
	m_haveSelection = false;

	QScreen *screen = dDesktopManager::instance()->getPrimaryScreenAS();

	m_backgroundPixmap = dDesktopManager::instance()->makeScreenshot( screen );
	if ( m_backgroundPixmap.isNull() )
	{
		close();
		return;
	}
	m_pixmap = m_backgroundPixmap;

	QPainter painter( &m_backgroundPixmap );
	painter.setBrush( QBrush( QColor( 0, 0, 0, 85 ), Qt::SolidPattern ) );
	painter.drawRect( QRect( QPoint( 0, 0 ), m_backgroundPixmap.size() ) );

	resize( m_backgroundPixmap.size() );

	setWindowFlags( Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint );
	setWindowState( Qt::WindowFullScreen );
	setCursor( Qt::CrossCursor );

	move( dDesktopManager::instance()->getScreenCoord( screen ) );
	show();
}

dAreaSelector::~dAreaSelector()
{
}

void dAreaSelector::paintEvent( QPaintEvent * /*event*/ )
{
	QPainter painter( this );
	painter.drawPixmap( QPoint( 0, 0 ), m_backgroundPixmap );

	drawSelection( painter );
}

void dAreaSelector::keyPressEvent( QKeyEvent *event )
{
#if defined( Q_WS_WIN ) || defined( Q_WS_MAC )
	quint32 key = event->nativeVirtualKey();
#elif defined( Q_WS_X11 )
	quint32 key = event->nativeScanCode();
#endif

	if ( key == m_dupload->nativeKeycode( 'B' ) )
	{
		prepareImage();
		m_dupload->sendFromClipboard();
		close();
	}
	else if ( key == m_dupload->nativeKeycode( 'S' ) && m_haveSelection)
	{
		m_selections.append( m_selection );
		m_haveSelection = false;
		update();
	}
	else if ( key == m_dupload->nativeKeycode( 'D' ) && !m_selections.empty() )
	{
		m_selections.pop_back();
		update();
	}
	else if ( key == m_dupload->nativeKeycode( 'N' ) )
	{
		prepareImage();
		m_dupload->sendFromClipboard( 1 );
		close();
	}
	else if ( key == m_dupload->nativeKeycode( 'C' ) )
	{
		prepareImage();
		close();
	}
	else if ( event->key() == Qt::Key_Escape )
	{
		close();
	}
}

void dAreaSelector::mousePressEvent( QMouseEvent *event )
{
	m_startPos = event->pos();
	m_processSelection = true;
	m_haveSelection = true;
}

void dAreaSelector::mouseMoveEvent( QMouseEvent *event )
{
	if( !( event->buttons() & Qt::LeftButton ) || !m_processSelection )
		return;

	m_endPos = event->pos();
	m_selection = QRect( m_startPos, m_endPos ).normalized();

	update();
}

void dAreaSelector::mouseReleaseEvent( QMouseEvent *event )
{
	m_endPos = event->pos();
	m_processSelection = false;
}

void dAreaSelector::drawSelection( QPainter &painter )
{
	for ( QRect selection : m_selections )
		painter.drawPixmap( selection, m_pixmap, selection );

	if (m_haveSelection)
	{
		painter.drawPixmap( m_selection, m_pixmap, m_selection );
		painter.setPen( QPen( QBrush( QColor( 0, 0, 0, 255 ) ), 2 ) );
		painter.drawRect( m_selection );
	}
}

void dAreaSelector::prepareImage()
{
	if ( !m_haveSelection && m_selections.empty() )
	{
		QApplication::clipboard()->setImage( m_pixmap.toImage() );
	}
	else
	{
		if ( m_haveSelection )
		{
			m_selections.append( m_selection );
		}
		m_selection = m_selections[0];
		for ( QRect selection : m_selections )
			m_selection = m_selection.united( selection );

		QPixmap canvas = QPixmap( m_pixmap.size() );
		canvas.fill( Qt::black );
		QPainter p( &canvas );
		for ( QRect selection : m_selections )
			p.drawPixmap( selection, m_pixmap, selection );

		QApplication::clipboard()->setImage( canvas.copy( m_selection ).toImage() );
	}
}
