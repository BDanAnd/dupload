/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2015 by Belov Nikita <null@deltaz.org>
 *                2019 by Bogomolov Danila
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
#include <QPluginLoader>
#include <vector>

#include "ddesktopmanager.h"
#include "dfilter.h"
#include "dfilterinterface.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

dFilter::dFilter( dUpload *d ) : m_dupload( d )
{
	ui.setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose );
	setAttribute( Qt::WA_QuitOnClose, false );

	QPixmap pixmap = QPixmap::fromImage( QApplication::clipboard()->image() );

	if ( pixmap.isNull() && QApplication::clipboard()->mimeData()->hasUrls() )
	{
		pixmap.load( QApplication::clipboard()->mimeData()->urls()[0].toLocalFile() );
	}

	if ( pixmap.isNull() )
	{
		pixmap = dDesktopManager::instance()->makeScreenshot();
	}

	if ( pixmap.isNull() )
	{
		close();
		return;
	}

	m_original = pixmap;
	m_current = m_original;
	ui.image->setPixmap( m_current.scaled( ui.image->width(), ui.image->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );

	ui.filterList->setIconSize( QSize( 200, 200 ) );
	ui.filterList->setViewMode( QListView::IconMode );
	ui.settings->addWidget( new QWidget );

	loadFilters();

	connect( ui.filterList, &QListWidget::currentItemChanged, this, &dFilter::filterActivated );
	//connect( ui.filterList, QOverload<int>::of( &QListWidget::currentRowChanged ), ui.settings, &QStackedWidget::setCurrentIndex );

	QPoint window_pos = dDesktopManager::instance()->getScreenCoord( dDesktopManager::instance()->getPrimaryScreen() );
	move( window_pos );
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

dFilter::~dFilter()
{
}

void dFilter::resizeEvent( QResizeEvent * )
{
	ui.image->setPixmap( m_current.scaled( ui.image->width(), ui.image->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
}

void dFilter::loadFilters()
{
	filters_dir = QDir( qApp->applicationDirPath() );
	filters_dir.cd( "filters" );

	const auto filters_list = filters_dir.entryList( QDir::Files );

	for ( const QString &file_name : filters_list )
	{
		if ( !QLibrary::isLibrary( file_name ) )
		{
			continue;
		}
		QPluginLoader loader( filters_dir.absoluteFilePath( file_name ) );
		auto filter = qobject_cast<dFilterInterface *>( loader.instance() );
		if ( filter )
		{
			filter->setPixmap( m_original );
			ui.settings->addWidget( filter->getWidget() );
			QListWidgetItem *item = new QListWidgetItem( filter->applyFilter(), filter->name() );
			item->setSizeHint( QSize( 240, 150 ) );
			item->setTextAlignment( Qt::AlignCenter );
			item->setData( Qt::UserRole, QVariant::fromValue( filter ) );
			ui.filterList->addItem( item );
		}
	}
}

void dFilter::filterActivated( QListWidgetItem *current, QListWidgetItem * )
{
	ui.settings->setCurrentIndex( ui.filterList->currentRow() + 1 );
	auto filter = current->data( Qt::UserRole ).value<dFilterInterface *>();
	m_current = filter->applyFilter();
	ui.image->setPixmap( m_current.scaled( ui.image->width(), ui.image->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
}

void dFilter::keyPressEvent( QKeyEvent *event )
{
#if defined( Q_WS_WIN ) || defined( Q_WS_MAC )
	quint32 key = event->nativeVirtualKey();
#elif defined( Q_WS_X11 )
	quint32 key = event->nativeScanCode();
#endif

	if ( key == m_dupload->nativeKeycode( 'B' ) )
	{
		QApplication::clipboard()->setImage( m_current.toImage() );
		m_dupload->sendFromClipboard();
		close();
	}
	else if ( key == m_dupload->nativeKeycode( 'N' ) )
	{
		QApplication::clipboard()->setImage( m_current.toImage() );
		m_dupload->sendFromClipboard(1);
		close();
	}
	else if ( key == m_dupload->nativeKeycode( 'A' ) )
	{
		QApplication::clipboard()->setImage( m_current.toImage() );
		m_dupload->sendFromClipboard(2);
		close();
	}
	else if ( key == m_dupload->nativeKeycode( 'C' ) )
	{
		QApplication::clipboard()->setImage( m_current.toImage() );
		close();
	}
	else if ( event->key() == Qt::Key_Escape )
	{
		close();
	}
}

