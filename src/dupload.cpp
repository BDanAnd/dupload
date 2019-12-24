/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2009-2010, 2012-2013, 2015 by Belov Nikita <null@deltaz.org>
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
#include "ddesktopmanager.h"

#ifdef Q_OS_WIN
#include <QtWinExtras>
#include "shobjidl.h"
#endif
#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

dUpload::dUpload( const QString &file, QWidget *parent ) : QWidget( parent )
{
	ui.setupUi( this );
	ui.progress->setVisible( false );
	move( qApp->desktop()->screenGeometry().center() - rect().center() );

	m_icon.addFile( ":/dUpload.ico" );
	setWindowIcon( m_icon );

	droparea = new dropArea( this );
	connect( droparea, SIGNAL( changed( const QString &, const QString & ) ), this, SLOT( changed( const QString &, const QString & ) ) );
	connect( droparea, SIGNAL( clicked() ), this, SLOT( clicked() ) );
	ui.layout->addWidget( droparea, 0, 0 );

	QAbstractEventDispatcher::instance()->installNativeEventFilter( this );
	QNetworkProxyFactory::setUseSystemConfiguration( true );

	m_netman = new QNetworkAccessManager();
	connect( m_netman, SIGNAL( finished( QNetworkReply * ) ), this, SLOT( finished( QNetworkReply * ) ) );

	m_trayicon = new dTrayIcon( this );

	m_webcam_creating = false;

	//new dPlayerMpc( this );

#ifdef Q_OS_WIN
	m_dll = new dDll();
	connect( dSettings::instance(), &dSettings::settingsChanged, m_dll, &dDll::updateSettings );
#endif

	m_file = new dFile();

	setUserlogin( QString() );

	dExternal::instance( this );

	dGlobalHotKey::instance()->shortcut("Alt+B");
	dGlobalHotKey::instance()->shortcut("Alt+N");
	dGlobalHotKey::instance()->shortcut("Alt+V");
	dGlobalHotKey::instance()->shortcut("Alt+S");
	dGlobalHotKey::instance()->shortcut("Alt+E");
	dGlobalHotKey::instance()->shortcut("Alt+A");
	dGlobalHotKey::instance()->shortcut("Ctrl+Shift+S");
	dGlobalHotKey::instance()->shortcut("Ctrl+Shift+W");
	dGlobalHotKey::instance()->shortcut("Ctrl+Shift+F");
	dGlobalHotKey::instance()->shortcut("Ctrl+Shift+Alt+S");
	dGlobalHotKey::instance()->shortcut("Ctrl+Shift+Alt+F");

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

void dUpload::showLast()
{
	if ( !m_logged )
	{
		notify( "You're not authorized" );
		return;
	}

	new dLast( this );
}

bool dUpload::authorized()
{
	return m_logged;
}

const QString &dUpload::userlogin()
{
	return m_userlogin;
}

void dUpload::setUserlogin( const QString &userlogin )
{
	if ( userlogin.isEmpty() )
	{
		m_logged = false;

#if defined( Q_WS_WIN )
		TCHAR winUserName[UNLEN + 1];
		DWORD winUserNameSize = sizeof( winUserName );
		GetUserName( winUserName, &winUserNameSize );
		#if defined( UNICODE )
			m_userlogin = QString::fromUtf16( ( ushort * ) winUserName );
		#else
			m_userlogin = QString::fromLocal8Bit( winUserName );
		#endif
#elif defined( Q_WS_MAC ) || defined( Q_OS_FREEBSD ) || defined( Q_OS_LINUX ) || defined( Q_OS_UNIX )
		m_userlogin = QTextCodec::codecForLocale()->toUnicode( getpwuid( geteuid() )->pw_name );
#else
		m_userlogin = "unknown";
#endif
	}
	else
	{
		m_logged = true;

		m_userlogin = userlogin;
	}

	m_trayicon->updateToolTip();
}

const QString &dUpload::passkey()
{
	return m_passkey;
}

void dUpload::setPasskey( const QString &passkey )
{
	m_passkey = passkey;
}

void dUpload::setLink( const QString &link )
{
	m_link = link;
}

void dUpload::aeroBackground()
{
#ifdef Q_OS_WIN
	if ( QtWin::isCompositionEnabled() )
	{
		QtWin::extendFrameIntoClientArea( this, -1, -1, -1, -1 );
		setAttribute( Qt::WA_TranslucentBackground, true );
		setAttribute( Qt::WA_NoSystemBackground, false );
		setStyleSheet( "dUpload { background: transparent; }" );
	}
	else
	{
		QtWin::resetExtendedFrame( this );
		setAttribute( Qt::WA_TranslucentBackground, false );
		setStyleSheet( QString( "dUpload { background: %1; }" ).arg( QtWin::realColorizationColor().name() ) );
	}
#endif
}

void dUpload::show()
{
	aeroBackground();

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
		raise();
	#endif
}

void dUpload::show( Qt::WindowFlags flags )
{
	setWindowFlags( flags );
	show();

#ifdef Q_OS_WIN
	m_dll->changeHWND( ( HWND )winId() );
#endif
}

void dUpload::changed( const QString &file, const QString &gallery )
{
	QFile f( file );
	if ( !f.open( QIODevice::ReadOnly ) )
		return;

	QString filename = file.section( "/", -1 );
	QString type = file.section( ".", -1 ).toLower();
	if ( type == "jpg" )
		type = "jpeg";

	load( f.readAll(), type, filename, gallery );

	f.close();
}

void dUpload::load( const QByteArray &arr, const QString &type, const QString &filename, const QString &gallery, bool autoConvert )
{
	droparea->lock();
	droparea->setVisible( false );
	ui.progress->setValue( 0 );
	ui.progress->setVisible( true );

	if ( dSettings::instance()->get( "s2fEnabled", false ) )
	{
		QString filePath = m_file->save( arr, type );

		finished( ( QNetworkReply * )&filePath );

		return;
	}

	QByteArray data;
	QNetworkRequest request( QUrl( QString( "http://%1/upload.php" ).arg( dSettings::instance()->get< QString >( "serverAddress" ) ) ) );

	QString author;
	if ( gallery.isEmpty() )
	{
		author = "------------TSrVsleyvjLUMocQf0NMuB\r\nContent-Disposition: form-data; name=\"author\"\r\n\r\n" + m_userlogin + "\r\n";
	}
	else
	{
		author = "------------TSrVsleyvjLUMocQf0NMuB\r\nContent-Disposition: form-data; name=\"p\"\r\n\r\n" + m_passkey + "\r\n";
		author += "------------TSrVsleyvjLUMocQf0NMuB\r\nContent-Disposition: form-data; name=\"gallery\"\r\n\r\n" + gallery + "\r\n";
	}

	

	QString begin = "------------TSrVsleyvjLUMocQf0NMuB\r\nContent-Disposition: form-data; name=\"image\"; filename=\"" + filename + "\"\r\nContent-Type: image/" + type + "\r\n\r\n";
	QString end = "\r\n------------TSrVsleyvjLUMocQf0NMuB--\r\n";

	data.append( author.toUtf8() );
	if ( autoConvert )
	{
		data.append( QString( "------------TSrVsleyvjLUMocQf0NMuB\r\nContent-Disposition: form-data; name=\"autoConvert\"\r\n\r\n\r\n" ).toUtf8() );
	}
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
	dTaskBar::instance()->setProgressState( (HWND)this->winId(), 1 );
#endif // DTASKBARACTIVE
}

void dUpload::sendFromClipboard( int type, const QString &gallery )
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
	else if ( type == 2 )
	{
		QApplication::clipboard()->image().save( &buffer, "PNG" );
		load( arr, "png", QString::number( QDateTime::currentDateTime().toTime_t() ) + ".png", QString(), true );
	}
}

void dUpload::progress( qint64 received, qint64 total )
{
	ui.progress->setMaximum( total );
	ui.progress->setValue( received );
	m_trayicon->message( QString::number( received ) + " bytes of " + QString::number( total ), 1 );

#if defined( DTASKBARACTIVE )
	dTaskBar::instance()->setProgressValue( (HWND)this->winId(), received, total );
#endif // DTASKBARACTIVE
}

void dUpload::finished( QNetworkReply *reply )
{
	droparea->lock( false );
	droparea->setVisible( true );
	ui.progress->setVisible( false );

	QString r;
	if ( !dSettings::instance()->get( "s2fEnabled", false ) )
	{
		#if defined( DTASKBARACTIVE )
		dTaskBar::instance()->setProgressState( (HWND)this->winId() );
		#endif // DTASKBARACTIVE

		r = reply->readAll();
	}
	else
	{
		r = *( QString * )reply;

		if ( r == "E:IGNORE" )
			return;
	}
	
	if ( r.startsWith( "E:" ) )
	{
		droparea->settext( r );
		m_trayicon->message( r, 2 );
	}
	else if ( !r.isEmpty() )
	{
		m_filename = r;

		if ( !dSettings::instance()->get( "s2fEnabled", false ) )
		{
			m_link = QString( "http://%1/%2" ).arg( dSettings::instance()->get< QString >( "serverAddress" ) ).arg( m_filename );
		}
		else
		{
			m_link = m_filename;
		}

		QApplication::clipboard()->setText( m_link );
		droparea->settext( "OK" );
		droparea->setToolTip( "Click here for copy link to clipboard\n" + m_link );
		m_trayicon->message( m_link );

		QFile last( QApplication::applicationDirPath() + "/last.txt" );
		if ( last.exists() && last.open( QIODevice::Append | QIODevice::Text ) )
		{
			QTextStream out( &last );
			out << m_link << endl;

			last.close();
		}

		emit finished();
	}
	else
	{
		droparea->settext( "E:0" );
		m_trayicon->message( "E:0", 2 );
	}

	if ( !dSettings::instance()->get( "s2fEnabled", false ) )
	{
		reply->deleteLater();
	}
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
#if defined( Q_WS_WIN ) || defined( Q_WS_MAC )
	quint32 key = event->nativeVirtualKey();
#elif defined( Q_WS_X11 )
	quint32 key = event->nativeScanCode();
#endif

	if ( key == nativeKeycode( 'T' ) || key == nativeKeycode( 'P' ) )
	{
		if ( !m_filename.isEmpty() )
				new dPreview( m_filename );
	}
	else if ( key == nativeKeycode( 'H' ) )
	{
		hide();
	}
	else if ( key == nativeKeycode( 'B' ) )
	{
		sendFromClipboard();
	}
	else if ( key == nativeKeycode( 'N' ) )
	{
		sendFromClipboard( 1 );
	}
	else if ( key == nativeKeycode( 'W' ) )
	{
		show( windowFlags() ^ Qt::WindowStaysOnTopHint );
	}
	else if ( key == nativeKeycode( 'R' ) )
	{
		dExternal::instance( this )->userRegister();
	}
	else if ( key == nativeKeycode( 'A' ) )
	{
		dExternal::instance( this )->userAuth();
	}
	else if ( key == nativeKeycode( 'D' ) )
	{
		setPasskey();
		setUserlogin();
		notify( "You're logged out" );
	}
	else if ( key == nativeKeycode( 'L' ) )
	{
		showLast();
	}
	else if ( key == nativeKeycode( 'S' ) )
	{
		dSettings::instance()->show();
	}
}

void dUpload::hotKeyPressed( quint32 k )
{
	if (k == dGlobalHotKey::instance()->id("Alt+B")) {
		sendFromClipboard();
	} else if (k == dGlobalHotKey::instance()->id("Alt+N")) {
		sendFromClipboard(1);
	} else if (k == dGlobalHotKey::instance()->id("Alt+V")) {
		show();
	} else if (k == dGlobalHotKey::instance()->id("Alt+S")) {
		QImage image = dDesktopManager::instance()->makeScreenshot().toImage();
		if (!image.isNull()) {
			QApplication::clipboard()->setImage(image);
		}
	} else if (k == dGlobalHotKey::instance()->id("Alt+E")) {
		new dHighlighter(this);
	} else if (k == dGlobalHotKey::instance()->id("Alt+A")) {
		QImage image = dDesktopManager::instance()->makeScreenshot().toImage();
		if (!image.isNull()) {
			QApplication::clipboard()->setImage(image);
			sendFromClipboard(2);
		}
	} else if (k == dGlobalHotKey::instance()->id("Ctrl+Shift+S")) {
		if (m_dareaselector) {
			m_dareaselector->activateWindow();
		} else {
			m_dareaselector = new dAreaSelector(this);
		}
	} else if (k == dGlobalHotKey::instance()->id("Ctrl+Shift+W")) {
		if (m_dwebcam) {
			m_dwebcam->activateWindow();
		} else if (!m_webcam_creating) {
			m_webcam_creating = true;
			m_dwebcam = new dWebCam(this);
			m_webcam_creating = false;
		}
	} else if (k == dGlobalHotKey::instance()->id("Ctrl+Shift+F")) {
		new dFilter(this);
	} else if (k == dGlobalHotKey::instance()->id("Ctrl+Shift+Alt+S")) {
		dSettings::instance()->show();
	} else if (k == dGlobalHotKey::instance()->id("Ctrl+Shift+Alt+F")) {
		bool state = !dSettings::instance()->get("s2fEnabled", false);
		dSettings::instance()->set("s2fEnabled", state);

		if (state) {
			notify("Save to file is enabled");
		} else {
			notify("Save to file is disabled");
		}
	}
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

void dUpload::paintEvent( QPaintEvent *event )
{
#ifdef Q_OS_WIN
	if ( QtWin::isCompositionEnabled() )
	{
		QPainter p( this );
		p.setCompositionMode( QPainter::CompositionMode_Clear );
		p.fillRect( 0, 0, width(), height(), QColor() );
	}
#endif
}

bool dUpload::nativeEventFilter( const QByteArray &eventType, void *event, long *result )
{
#ifdef Q_OS_WIN
	if ( eventType == "windows_generic_MSG" )
	{
		MSG *message = ( MSG * )event;

		if ( message->message == WM_COPYDATA )
		{
			bool enableMP = dSettings::instance()->get( "enableMP", false );

			if ( enableMP )
			{
				TCHAR filePath[ MAX_PATH ];

				COPYDATASTRUCT *data = ( COPYDATASTRUCT * )( message->lParam );
				memcpy_s( filePath, MAX_PATH, data->lpData, MAX_PATH );

				QString file = QString::fromWCharArray( filePath ).remove( 0, 4 );

				emit changed( file );
			}

			return enableMP;
		}
	}
#endif

	return false;
}

bool dUpload::event( QEvent *event )
{
#ifdef Q_OS_WIN
	if ( event->type() == QEvent::WinIdChange )
	{
		m_dll->changeHWND( ( HWND )winId() );
	}
	else if ( event->type() == QWinEvent::CompositionChange || event->type() == QWinEvent::ColorizationChange )
	{
		aeroBackground();
	}
#endif

	return QWidget::event( event );
}

quint32 dUpload::nativeKeycode( QChar key )
{
	quint32 k, m;
	dGlobalHotKey::instance()->native( key, k, m );

	return k;
}
