/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2010, 2012-2013 by Belov Nikita <null@deltaz.org>
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

#include <QtGui/QPainterPath>

#include "dhighlighter.h"

dHighlighter::dHighlighter( dUpload *d ) : m_dupload( d )
{
	ui.setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose );
	setAttribute( Qt::WA_QuitOnClose, false );

	QPixmap pixmap = QPixmap::fromImage( QApplication::clipboard()->image() );

	if ( pixmap.isNull() && QApplication::clipboard()->mimeData()->hasUrls() )
		pixmap.load( QApplication::clipboard()->mimeData()->urls()[0].toLocalFile() );

	if ( pixmap.isNull() )
		pixmap = QPixmap::grabWindow( QApplication::desktop()->winId() );

	ui.image->setPixmap( pixmap );

	showMaximized();

	#if defined( Q_WS_X11 )
		XClientMessageEvent xev;
		xev.type = ClientMessage;
		xev.window = winId();
		xev.message_type = XInternAtom( QX11Info::display(), "_NET_ACTIVE_WINDOW", False );
		xev.format = 32;
		xev.data.l[0] = 2;
		xev.data.l[1] = QX11Info::appUserTime();

		XSendEvent( QX11Info::display(), QX11Info::appRootWindow(), False, ( SubstructureNotifyMask | SubstructureRedirectMask ), ( XEvent * ) &xev );
	#else
		activateWindow();
		raise();
	#endif
}

dHighlighter::~dHighlighter()
{
}

void dHighlighter::keyPressEvent( QKeyEvent *event )
{
#if defined( Q_WS_WIN ) || defined( Q_WS_MAC )
	quint32 key = event->nativeVirtualKey();
#elif defined( Q_WS_X11 )
	quint32 key = event->nativeScanCode();
#endif

	if ( key == m_dupload->nativeKeycode( 'B' ) )
	{
		QApplication::clipboard()->setImage( ui.image->pixmap().toImage() );
		m_dupload->sendFromClipboard();
		close();
	}
	else if ( key == m_dupload->nativeKeycode( 'N' ) )
	{
		QApplication::clipboard()->setImage( ui.image->pixmap().toImage() );
		m_dupload->sendFromClipboard( 1 );
		close();
	}
	else if ( key == m_dupload->nativeKeycode( 'Z' ) )
	{
		if ( m_lastPos == QPoint( -1, -1 ) )
		{
			ui.image->undo();
		}
	}
	else if ( key == m_dupload->nativeKeycode( 'C' ) )
	{
		QApplication::clipboard()->setImage( ui.image->pixmap().toImage() );
		close();
	}
	else if ( event->key() == Qt::Key_Escape )
	{
		close();
	}
}

void dHighlighter::mousePressEvent( QMouseEvent *event )
{
	if ( event->button() == Qt::RightButton )
	{
		ui.image->setOpacity( 0.3 );
		ui.image->setPen( QPen( Qt::yellow, 15, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
	}
	else
	{
		ui.image->setOpacity( 1.0 );
		ui.image->setPen( QPen( Qt::red, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin ) );
	}

	m_lastPos = event->pos();
	m_path = QPainterPath( m_lastPos + scrollBarShift() );
}

void dHighlighter::mouseMoveEvent( QMouseEvent *event )
{
	if( !( event->buttons() & Qt::LeftButton || event->buttons() & Qt::RightButton ) || m_lastPos == QPoint( -1, -1 ) )
		return;

	m_path.quadTo( m_lastPos + scrollBarShift(), ( m_lastPos + event->pos() ) / 2 + scrollBarShift() );
	ui.image->drawPath( m_path );

	m_lastPos = event->pos();
}

void dHighlighter::mouseReleaseEvent( QMouseEvent * /*event*/ )
{
	if ( m_lastPos == QPoint( -1, -1 ) )
		return;

	if ( m_path.isEmpty() )
		ui.image->drawPoint( m_lastPos );
	else
		ui.image->drawPath( m_path, true );

	m_lastPos = QPoint( -1, -1 );
}

QPoint dHighlighter::scrollBarShift()
{
	return QPoint( ui.scrollArea->horizontalScrollBar()->value(), ui.scrollArea->verticalScrollBar()->value() );
}
