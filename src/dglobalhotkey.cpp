/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2009-2010, 2012, 2015 by Belov Nikita <null@deltaz.org>
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

#include "dglobalhotkey.h"

#include <QDebug>

#ifdef Q_WS_X11
    #include <X11/Xlib.h>
    #include <xcb/xcb.h>
#endif

dGlobalHotKey::dGlobalHotKey()
{
    QAbstractEventDispatcher::instance()->installNativeEventFilter( this );
}

dGlobalHotKey::~dGlobalHotKey()
{
}

#if defined( Q_WS_X11 )

static int ( *original_x_errhandler )( Display *display, XErrorEvent *event );
static int qxt_x_errhandler( Display *display, XErrorEvent *event )
{
	Q_UNUSED( display );
	switch ( event->error_code )
	{
		case BadAccess:
		case BadValue:
		case BadWindow:
			if ( event->request_code == 33 || event->request_code == 34 )
				dGlobalHotKey::instance()->error = true;
		default:
			return 0;
	}
}

#endif

#if defined( Q_WS_MAC )

typedef QPair< uint, uint > Identifier;
static QMap< quint32, EventHotKeyRef > keyRefs;
static QHash< Identifier, quint32 > keyIDs;
static quint32 hotKeySerial = 0;
static bool qxt_mac_handler_installed = false;

OSStatus qxt_mac_handle_hot_key( EventHandlerCallRef /* nextHandler */, EventRef event, void *data )
{
    Q_UNUSED( data );
    QAbstractEventDispatcher::instance()->filterEvent( ( void * ) event );

    return noErr;
}

#endif

bool dGlobalHotKey::nativeEventFilter( const QByteArray &eventType, void *e, long * )
{
	#if defined( Q_WS_WIN )
		MSG *m = ( MSG * ) e;

		if ( m->message == WM_HOTKEY )
			dGlobalHotKey::instance()->hotKeyPressed( id( HIWORD( m->lParam ), LOWORD( m->lParam ) ) );
	#elif defined( Q_WS_X11 )
        xcb_key_press_event_t *kev = 0;
        if (eventType == "xcb_generic_event_t") {
            xcb_generic_event_t *ev = static_cast<xcb_generic_event_t *>(e);
            if ((ev->response_type & 127) == XCB_KEY_PRESS) {
                kev = static_cast<xcb_key_press_event_t *>(e);
            }
        }

        if (kev != 0) {
            unsigned int keycode = kev->detail;
            unsigned int keystate = 0;
            if(kev->state & XCB_MOD_MASK_1)
                keystate |= Mod1Mask;
            if(kev->state & XCB_MOD_MASK_CONTROL)
                keystate |= ControlMask;
            if(kev->state & XCB_MOD_MASK_4)
                keystate |= Mod4Mask;
            if(kev->state & XCB_MOD_MASK_SHIFT)
                keystate |= ShiftMask;

            dGlobalHotKey::instance()->hotKeyPressed( id( keycode, ( keystate & ( ShiftMask | ControlMask | Mod1Mask | Mod4Mask ) ) ) );
        }
	#elif defined( Q_WS_MAC )
		EventRef event = ( EventRef ) e;
		if ( GetEventClass( event ) == kEventClassKeyboard && GetEventKind( event ) == kEventHotKeyPressed )
		{
			EventHotKeyID keyID;
			GetEventParameter( event, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof( keyID ), NULL, &keyID );
			Identifier key_id = keyIDs.key( keyID.id );
			dGlobalHotKey::instance()->hotKeyPressed( id( key_id.second ^ key_id.first ) );
		}
	#endif

	return false;
}

dGlobalHotKey *dGlobalHotKey::instance()
{
	static dGlobalHotKey *me = new dGlobalHotKey();

	return me;
}

bool dGlobalHotKey::shortcut( const QString &s, bool a )
{
	quint32 key, mods;
	native( s, key, mods );

	#if defined( Q_WS_WIN )
		if ( a )
		{
			quint32 modNoRepeat = ( QSysInfo::windowsVersion() >= QSysInfo::WV_WINDOWS7 ? MOD_NOREPEAT : 0 );

			return RegisterHotKey( 0, mods ^ key, mods | modNoRepeat, key );
		}
		else
		{
			return UnregisterHotKey( 0, mods ^ key );
		}
	#elif defined( Q_WS_X11 )
		if ( a )
		{
			Display *display = QX11Info::display();
			Window window = QX11Info::appRootWindow();
			dGlobalHotKey::instance()->error = false;
			original_x_errhandler = XSetErrorHandler( qxt_x_errhandler );
			XGrabKey( display, key, mods, window, True, GrabModeAsync, GrabModeAsync );
			XGrabKey( display, key, mods | Mod2Mask, window, True, GrabModeAsync, GrabModeAsync );
			XSync( display, False );
			XSetErrorHandler( original_x_errhandler );

			return !dGlobalHotKey::instance()->error;
		}
		else
		{
			Display *display = QX11Info::display();
			Window window = QX11Info::appRootWindow();
			dGlobalHotKey::instance()->error = false;
			original_x_errhandler = XSetErrorHandler( qxt_x_errhandler );
			XUngrabKey( display, key, mods, window );
			XUngrabKey( display, key, mods | Mod2Mask, window );
			XSync( display, False );
			XSetErrorHandler( original_x_errhandler );

			return !dGlobalHotKey::instance()->error;
		}
	#elif defined( Q_WS_MAC )
		if ( a )
		{
			if ( !qxt_mac_handler_installed )
			{
				EventTypeSpec t;
				t.eventClass = kEventClassKeyboard;
				t.eventKind = kEventHotKeyPressed;
				InstallApplicationEventHandler( &qxt_mac_handle_hot_key, 1, &t, NULL, NULL );
			}

			EventHotKeyID keyID;
			keyID.signature = 'cute';
			keyID.id = ++hotKeySerial;

			EventHotKeyRef ref = 0;
			bool rv = !RegisterEventHotKey( key, mods, keyID, GetApplicationEventTarget(), 0, &ref );
			if ( rv )
			{
				keyIDs.insert( Identifier( mods, key ), keyID.id );
				keyRefs.insert( keyID.id, ref );
			}

			return rv;
		}
		else
		{
			Identifier id( mods, key );
			if ( !keyIDs.contains( id ) )
				return false;

			EventHotKeyRef ref = keyRefs.take( keyIDs[id] );
			keyIDs.remove( id );
			return !UnregisterEventHotKey( ref );
		}
	#endif
}

void dGlobalHotKey::native( const QString &s, quint32 &k, quint32 &m )
{
	QKeySequence ks( s );
	Qt::KeyboardModifiers allMods = Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier;

	k = nativeKeycode( ks.isEmpty() ? Qt::Key( 0 ) : Qt::Key( ( ks[0] ^ allMods ) & ks[0] ) );
	m = nativeModifiers( ks.isEmpty() ? Qt::KeyboardModifiers( 0 ) : Qt::KeyboardModifiers( ks[0] & allMods ) );
}

quint32 dGlobalHotKey::id( const QString &s )
{
	quint32 key, mods;
	native( s, key, mods );

	// It's a right sequence of parameters.
	return id(key, mods);
}

quint32 dGlobalHotKey::id(quint32 mods, quint32 key)
{
	return (mods << 16) | key;
}

#if defined( Q_WS_WIN )

quint32 dGlobalHotKey::nativeModifiers( Qt::KeyboardModifiers m )
{
	quint32 native = 0;

	if ( m & Qt::ShiftModifier )
		native |= MOD_SHIFT;
	if ( m & Qt::ControlModifier )
		native |= MOD_CONTROL;
	if ( m & Qt::AltModifier )
		native |= MOD_ALT;
	if ( m & Qt::MetaModifier )
		native |= MOD_WIN;

	return native;
}

quint32 dGlobalHotKey::nativeKeycode( Qt::Key k )
{
	switch ( k )
	{
		case Qt::Key_Escape:
			return VK_ESCAPE;
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
			return VK_TAB;
		case Qt::Key_Backspace:
			return VK_BACK;
		case Qt::Key_Return:
		case Qt::Key_Enter:
			return VK_RETURN;
		case Qt::Key_Insert:
			return VK_INSERT;
		case Qt::Key_Delete:
			return VK_DELETE;
		case Qt::Key_Pause:
			return VK_PAUSE;
		case Qt::Key_Print:
			return VK_PRINT;
		case Qt::Key_Clear:
			return VK_CLEAR;
		case Qt::Key_Home:
			return VK_HOME;
		case Qt::Key_End:
			return VK_END;
		case Qt::Key_Left:
			return VK_LEFT;
		case Qt::Key_Up:
			return VK_UP;
		case Qt::Key_Right:
			return VK_RIGHT;
		case Qt::Key_Down:
			return VK_DOWN;
		case Qt::Key_PageUp:
			return VK_PRIOR;
		case Qt::Key_PageDown:
			return VK_NEXT;
		case Qt::Key_F1:
			return VK_F1;
		case Qt::Key_F2:
			return VK_F2;
		case Qt::Key_F3:
			return VK_F3;
		case Qt::Key_F4:
			return VK_F4;
		case Qt::Key_F5:
			return VK_F5;
		case Qt::Key_F6:
			return VK_F6;
		case Qt::Key_F7:
			return VK_F7;
		case Qt::Key_F8:
			return VK_F8;
		case Qt::Key_F9:
			return VK_F9;
		case Qt::Key_F10:
			return VK_F10;
		case Qt::Key_F11:
			return VK_F11;
		case Qt::Key_F12:
			return VK_F12;
		case Qt::Key_F13:
			return VK_F13;
		case Qt::Key_F14:
			return VK_F14;
		case Qt::Key_F15:
			return VK_F15;
		case Qt::Key_F16:
			return VK_F16;
		case Qt::Key_F17:
			return VK_F17;
		case Qt::Key_F18:
			return VK_F18;
		case Qt::Key_F19:
			return VK_F19;
		case Qt::Key_F20:
			return VK_F20;
		case Qt::Key_F21:
			return VK_F21;
		case Qt::Key_F22:
			return VK_F22;
		case Qt::Key_F23:
			return VK_F23;
		case Qt::Key_F24:
			return VK_F24;
		case Qt::Key_Space:
			return VK_SPACE;
		case Qt::Key_Asterisk:
			return VK_MULTIPLY;
		case Qt::Key_Plus:
			return VK_ADD;
		case Qt::Key_Comma:
			return VK_SEPARATOR;
		case Qt::Key_Minus:
			return VK_SUBTRACT;
		case Qt::Key_Slash:
			return VK_DIVIDE;

		case Qt::Key_0:
		case Qt::Key_1:
		case Qt::Key_2:
		case Qt::Key_3:
		case Qt::Key_4:
		case Qt::Key_5:
		case Qt::Key_6:
		case Qt::Key_7:
		case Qt::Key_8:
		case Qt::Key_9:
			return k;

		case Qt::Key_A:
		case Qt::Key_B:
		case Qt::Key_C:
		case Qt::Key_D:
		case Qt::Key_E:
		case Qt::Key_F:
		case Qt::Key_G:
		case Qt::Key_H:
		case Qt::Key_I:
		case Qt::Key_J:
		case Qt::Key_K:
		case Qt::Key_L:
		case Qt::Key_M:
		case Qt::Key_N:
		case Qt::Key_O:
		case Qt::Key_P:
		case Qt::Key_Q:
		case Qt::Key_R:
		case Qt::Key_S:
		case Qt::Key_T:
		case Qt::Key_U:
		case Qt::Key_V:
		case Qt::Key_W:
		case Qt::Key_X:
		case Qt::Key_Y:
		case Qt::Key_Z:
			return k;

		default:
			return 0;
	}
}

#elif defined( Q_WS_X11 )

quint32 dGlobalHotKey::nativeModifiers( Qt::KeyboardModifiers m )
{
	quint32 native = 0;

	if ( m & Qt::ShiftModifier )
		native |= ShiftMask;
	if ( m & Qt::ControlModifier )
		native |= ControlMask;
	if ( m & Qt::AltModifier )
		native |= Mod1Mask;

	return native;
}

quint32 dGlobalHotKey::nativeKeycode( Qt::Key k )
{
	return XKeysymToKeycode( QX11Info::display(), XStringToKeysym( QKeySequence( k ).toString().toLatin1().data() ) );
}

#elif defined( Q_WS_MAC )

quint32 dGlobalHotKey::nativeModifiers( Qt::KeyboardModifiers m )
{
	quint32 native = 0;

	if ( m & Qt::ShiftModifier )
		native |= shiftKey;
	if ( m & Qt::ControlModifier )
		native |= cmdKey;
	if ( m & Qt::AltModifier )
		native |= optionKey;
	if ( m & Qt::MetaModifier )
		native |= controlKey;
	if ( m & Qt::KeypadModifier )
		native |= kEventKeyModifierNumLockMask;
	// Special case for Qt::Key_Backtab ( stealed from kdelibs. I don't know what does it do )
	if ( ( m & ~Qt::KeyboardModifierMask ) == Qt::Key_Backtab )
		mod |= shiftKey;

	return native;
}

quint32 dGlobalHotKey::nativeKeycode( Qt::Key k )
{
	UTF16Char ch;

	if ( k == Qt::Key_Up )
		ch = 0xF700;
	else if ( k == Qt::Key_Down )
		ch = 0xF701;
	else if ( k == Qt::Key_Left )
		ch = 0xF702;
	else if ( k == Qt::Key_Right )
		ch = 0xF703;
	else if ( k >= Qt::Key_F1 && k <= Qt::Key_F35 )
		ch = k - Qt::Key_F1 + 0xF704;
	else if ( k == Qt::Key_Insert )
		ch = 0xF727;
	else if ( k == Qt::Key_Delete )
		ch = 0xF728;
	else if ( k == Qt::Key_Home )
		ch = 0xF729;
	else if ( k == Qt::Key_End )
		ch = 0xF72B;
	else if ( k == Qt::Key_PageUp )
		ch = 0xF72C;
	else if ( k == Qt::Key_PageDown )
		ch = 0xF72D;
	else if ( k == Qt::Key_Print )
		ch = 0xF72E;
	else if ( k == Qt::Key_ScrollLock )
		ch = 0xF72F;
	else if ( k == Qt::Key_Pause )
		ch = 0xF730;
	else if ( k == Qt::Key_SysReq )
		ch = 0xF731;
	else if ( k == Qt::Key_Stop )
		ch = 0xF734;
	else if ( k == Qt::Key_Menu )
		ch = 0xF735;
	else if ( k == Qt::Key_Select )
		ch = 0xF741;
	else if ( k == Qt::Key_Execute )
		ch = 0xF742;
	else if ( k == Qt::Key_Help )
		ch = 0xF746;
	else if ( k == Qt::Key_Mode_switch )
		ch = 0xF747;
	else if ( k == Qt::Key_Escape )
		ch = 27;
	else if ( k == Qt::Key_Return )
		ch = 13;
	else if ( k == Qt::Key_Enter )
		ch = 3;
	else if ( k == Qt::Key_Tab )
		ch = 9;
	else
		ch = k;

#ifdef QT_MAC_USE_COCOA
        TISInputSourceRef layout = TISCopyCurrentKeyboardLayoutInputSource();
        if ( !layout )
            return 0;

		CFDataRef data = static_cast< CFDataRef >( TISGetInputSourceProperty( layout, kTISPropertyUnicodeKeyLayoutData ) );
		const UCKeyboardLayout *ucData = data ? reinterpret_cast< const UCKeyboardLayout * >( CFDataGetBytePtr( data ) ) : 0;

		if ( !ucData )
			return 0;

		//for ( int i = 0; i < 128; i++ )
		//{
			UInt32 tmpState = 0;
			UniChar str[4];
			UniCharCount actualLength = 0;
			OSStatus err = UCKeyTranslate( ucData, k, kUCKeyActionDown, 0, LMGetKbdType(), kUCKeyTranslateNoDeadKeysMask, &tmpState, 4, &actualLength, str );
		//	if ( err == noErr )
		//	{
		//		if ( str[0] && str[0] != kFunctionKeyCharCode )
		//			scancodes.insert(str[0], i);
		//	}
			qDebug() << str[0];
			return str[0];
		}
#else
	KeyboardLayoutRef layout;
	KeyboardLayoutKind layoutKind;
	KLGetCurrentKeyboardLayout( &layout );
	KLGetKeyboardLayoutProperty( layout, kKLKind, const_cast< const void ** >( reinterpret_cast< void ** >( &layoutKind ) ) );

	if ( layoutKind == kKLKCHRKind )
	{
		if ( ch > 255 )
			return 0;

		char *data;
		KLGetKeyboardLayoutProperty( layout, kKLKCHRData, const_cast< const void ** >( reinterpret_cast< void ** >( &data ) ) );
		int ct = *reinterpret_cast< short * >( data + 258 );
		for ( int i = 0; i < ct; i++ )
		{
			char *keyTable = data + 260 + 128 * i;
			for ( int j = 0; j < 128; j++ )
				if ( keyTable[j] == ch )
					return j;
		}

		return 0;
	}

	char *data;
	KLGetKeyboardLayoutProperty( layout, kKLuchrData, const_cast< const void ** >( reinterpret_cast< void ** >( &data ) ) );
	UCKeyboardLayout *header = reinterpret_cast< UCKeyboardLayout * >( data );
	UCKeyboardTypeHeader *table = header->keyboardTypeList;

	for ( quint32 i=0; i < header->keyboardTypeCount; i++ )
	{
		UCKeyStateRecordsIndex *stateRec = 0;
		if ( table[i].keyStateRecordsIndexOffset != 0 )
		{
			stateRec = reinterpret_cast< UCKeyStateRecordsIndex * >( data + table[i].keyStateRecordsIndexOffset );
			if ( stateRec->keyStateRecordsIndexFormat != kUCKeyStateRecordsIndexFormat )
				stateRec = 0;
		}

		UCKeyToCharTableIndex *charTable = reinterpret_cast< UCKeyToCharTableIndex * >( data + table[i].keyToCharTableIndexOffset );
		if ( charTable->keyToCharTableIndexFormat != kUCKeyToCharTableIndexFormat )
			continue;

		for ( quint32 j=0; j < charTable->keyToCharTableCount; j++ )
		{
			UCKeyOutput *keyToChar = reinterpret_cast< UCKeyOutput * >( data + charTable->keyToCharTableOffsets[j] );
			for ( quint32 k=0; k < charTable->keyToCharTableSize; k++ )
			{
				if (keyToChar[k] & kUCKeyOutputTestForIndexMask)
				{
					long idx = keyToChar[k] & kUCKeyOutputGetIndexMask;
					if ( stateRec && idx < stateRec->keyStateRecordCount )
					{
						UCKeyStateRecord *rec = reinterpret_cast< UCKeyStateRecord * >( data + stateRec->keyStateRecordOffsets[idx] );
						if ( rec->stateZeroCharData == ch )
							return k;
					}
				}
				else if ( !( keyToChar[k] & kUCKeyOutputSequenceIndexMask ) && keyToChar[k] < 0xFFFE )
					if ( keyToChar[k] == ch )
						return k;
			}
		}
	}
#endif

	return 0;
}

#endif
