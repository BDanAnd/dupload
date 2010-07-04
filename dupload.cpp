/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2009-2010 by Belov Nikita <null@deltaz.org>
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

#include "dupload.h"

dUpload::dUpload( const QString &file, QWidget *parent ) : QWidget( parent )
{
	ui.setupUi( this );
	ui.progress->setVisible( false );
	move( qApp->desktop()->geometry().center() - rect().center() );

	m_icon.addFile( ":/dUpload.ico" );
	setWindowIcon( m_icon );

	droparea = new dropArea();
	connect( droparea, SIGNAL( changed( const QString & ) ), this, SLOT( changed( const QString & ) ) );
	connect( droparea, SIGNAL( clicked() ), this, SLOT( clicked() ) );
	ui.layout->addWidget( droparea, 0, 0 );

	m_netman = new QNetworkAccessManager();
	connect( m_netman, SIGNAL( finished( QNetworkReply * ) ), this, SLOT( finished( QNetworkReply * ) ) );

	m_trayicon = new dTrayIcon( this );

#if defined( Q_WS_WIN )
	TCHAR winUserName[UNLEN + 1];
	DWORD winUserNameSize = sizeof( winUserName );
	GetUserName( winUserName, &winUserNameSize );
	#if defined( UNICODE )
		m_userlogin = QString::fromUtf16( winUserName );
	#else
		m_userlogin = QString::fromLocal8Bit( winUserName );
	#endif
#elif defined( Q_WS_MAC ) || defined( Q_OS_FREEBSD ) || defined( Q_OS_LINUX ) || defined( Q_OS_UNIX )
	m_userlogin = QString( getpwuid( geteuid() )->pw_name );
#else
	m_userlogin = "unknown";
#endif

	dExternal::instance( this );

	dGlobalHotKey::instance()->shortcut( "Alt+B" );
	dGlobalHotKey::instance()->shortcut( "Alt+N" );
	dGlobalHotKey::instance()->shortcut( "Alt+V" );
	dGlobalHotKey::instance()->shortcut( "Alt+S" );
	connect( dGlobalHotKey::instance(), SIGNAL( hotKeyPressed( quint32 ) ), this, SLOT( hotKeyPressed( quint32 ) ) );

	if ( QFileInfo( file ).isFile() )
		changed( file );
}

dUpload::~dUpload()
{
}

void dUpload::notify( const QString &m )
{
	if ( m_trayicon )
		m_trayicon->message( m, 3 );
}

void dUpload::show()
{
	setVisible( true );

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
	#endif

	#if defined( Q_WS_MAC )
		raise();
	#endif
}

void dUpload::show( Qt::WindowFlags flags )
{
	setWindowFlags( flags );
	if ( QtWin::isCompositionEnabled() )
		QtWin::extendFrameIntoClientArea( this );
	show();
}

void dUpload::changed( const QString &file )
{
	QFile f( file );
	if ( !f.open( QIODevice::ReadOnly ) )
		return;

	QString filename = file.section( "/", -1 );
	QString type = file.section( ".", -1 ).toLower();
	if ( type == "jpg" )
		type = "jpeg";

	load( f.readAll(), type, filename );

	f.close();
}

void dUpload::load( const QByteArray &arr, const QString &type, const QString &filename )
{
	droparea->lock();
	droparea->setVisible( false );
	ui.progress->setValue( 0 );
	ui.progress->setVisible( true );

	QByteArray data;
	QNetworkRequest request( QUrl( "http://i.deltaz.org/upload.php" ) );

	QString author = "------------TSrVsleyvjLUMocQf0NMuB\r\nContent-Disposition: form-data; name=\"author\"\r\n\r\n" + m_userlogin + "\r\n";
	QString begin = "------------TSrVsleyvjLUMocQf0NMuB\r\nContent-Disposition: form-data; name=\"image\"; filename=\"" + filename + "\"\r\nContent-Type: image/" + type + "\r\n\r\n";
	QString end = "\r\n------------TSrVsleyvjLUMocQf0NMuB--\r\n";

	data.append( author.toUtf8() );
	data.append( begin.toUtf8() );
	data.append( arr );
	data.append( end.toUtf8() );

	request.setRawHeader( "User-Agent", QString( "iDelta" ).toUtf8() );
	request.setRawHeader( "Accept", QString( "text/html" ).toUtf8() ); 
	request.setRawHeader( "Content-Length", QString::number( data.length() ).toUtf8() );
	request.setRawHeader( "Content-Type", QString( "multipart/form-data; boundary=----------TSrVsleyvjLUMocQf0NMuB" ).toUtf8() );
	
	QNetworkReply* netr = m_netman->post( request, data );
	connect( netr, SIGNAL( uploadProgress( qint64, qint64 ) ), this, SLOT( progress( qint64, qint64 ) ) );
#if defined( DTASKBARACTIVE )
	dTaskBar::instance()->setProgressState( this->winId(), 1 );
#endif // DTASKBARACTIVE
}

void dUpload::sendFromClipboard( int type )
{
	if ( droparea->isLocked() )
		return;

	if ( QApplication::clipboard()->mimeData()->hasUrls() )
	{
		changed( QApplication::clipboard()->mimeData()->urls()[0].toLocalFile() );
		return;
	}

	QByteArray arr;
	QBuffer buffer( &arr );
	buffer.open( QIODevice::WriteOnly );

	if ( type == 0 )
	{
		QApplication::clipboard()->image().save( &buffer, "JPEG" );
		load( arr, "jpeg", QString::number( QDateTime::currentDateTime().toTime_t() ) + ".jpg" );
	}
	else if ( type == 1 )
	{
		QApplication::clipboard()->image().save( &buffer, "PNG" );
		load( arr, "png", QString::number( QDateTime::currentDateTime().toTime_t() ) + ".png" );
	}
}

void dUpload::progress( qint64 received, qint64 total )
{
	ui.progress->setMaximum( total );
	ui.progress->setValue( received );
	m_trayicon->message( QString::number( received ) + " bytes of " + QString::number( total ), 1 );

#if defined( DTASKBARACTIVE )
	dTaskBar::instance()->setProgressValue( this->winId(), received, total );
#endif // DTASKBARACTIVE
}

void dUpload::finished( QNetworkReply *reply )
{
#if defined( DTASKBARACTIVE )
	dTaskBar::instance()->setProgressState( this->winId() );
#endif // DTASKBARACTIVE

	QString r = reply->readAll();

	droparea->lock( false );
	droparea->setVisible( true );
	ui.progress->setVisible( false );
	if ( r.startsWith( "E:" ) )
	{
		droparea->settext( r );
		m_trayicon->message( r, 2 );
	}
	else if ( !r.isEmpty() )
	{
		m_filename = r;
		m_link = "http://i.deltaz.org/" + m_filename;

		QApplication::clipboard()->setText( m_link );
		droparea->settext( "OK" );
		droparea->setToolTip( "Click here for copy link to clipboard\n" + m_link );
		m_trayicon->message( m_link );
	}
	else
	{
		droparea->settext( "E:0" );
		m_trayicon->message( "E:0", 2 );
	}

	reply->deleteLater();
}

void dUpload::clicked()
{
	if ( !m_link.isEmpty() )
	{
		QApplication::clipboard()->setText( m_link );
		droparea->settext( "drop\nhere" );
		droparea->setToolTip( "" );

		m_filename.clear();
		m_link.clear();
	}
}

void dUpload::closeEvent( QCloseEvent * /*event*/ )
{
	delete m_netman;
	delete m_trayicon;
#if defined( DTASKBARACTIVE )
	delete dTaskBar::instance();
#endif // DTASKBARACTIVE
}

void dUpload::keyPressEvent( QKeyEvent *event )
{
	#if defined( Q_WS_WIN )
		const quint32 key = event->nativeScanCode();

		const quint32 key_t = 20;
		const quint32 key_p = 25;
		const quint32 key_h = 35;
		const quint32 key_b = 48;
		const quint32 key_n = 49;
		const quint32 key_w = 17;
	#elif defined( Q_WS_X11 )
		const quint32 key = event->nativeScanCode();

		const quint32 key_t = 28;
		const quint32 key_p = 33;
		const quint32 key_h = 43;
		const quint32 key_b = 56;
		const quint32 key_n = 57;
		const quint32 key_w = 25;
	#elif defined( Q_WS_MAC )
		const quint32 key = event->nativeVirtualKey();

		const quint32 key_t = 17;
		const quint32 key_p = 35;
		const quint32 key_h = 4;
		const quint32 key_b = 11;
		const quint32 key_n = 45;
		const quint32 key_w = 13;
	#endif

	switch ( key )
	{
		case key_t:
		case key_p:
			if ( !m_filename.isEmpty() )
				m_preview = new dPreview( m_filename );
			break;

		case key_h:
			hide();
			break;

		case key_b:
			sendFromClipboard();
			break;

		case key_n:
			sendFromClipboard( 1 );
			break;

		case key_w:
			show( windowFlags() ^ Qt::WindowStaysOnTopHint );
			break;
	}
}

void dUpload::hotKeyPressed( quint32 k )
{
	if ( k == dGlobalHotKey::instance()->id( "Alt+B" ) )
		sendFromClipboard();
	else if ( k == dGlobalHotKey::instance()->id( "Alt+N" ) )
		sendFromClipboard( 1 );
	else if ( k == dGlobalHotKey::instance()->id( "Alt+V" ) )
		show();
	else if ( k == dGlobalHotKey::instance()->id( "Alt+S" ) )
		QApplication::clipboard()->setImage( QPixmap::grabWindow( QApplication::desktop()->winId() ).toImage() );
}

void dUpload::mousePressEvent( QMouseEvent *event )
{
	if( event->button() == Qt::LeftButton )
		m_drag_pos = event->globalPos() - frameGeometry().topLeft();
}

void dUpload::mouseMoveEvent( QMouseEvent *event )
{
	if( event->buttons() & Qt::LeftButton )
		move( event->globalPos() - m_drag_pos );
}
