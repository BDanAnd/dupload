/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2012 by Belov Nikita <null@deltaz.org>
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

#include "dplayermpc.h"

dPlayerMpc::dPlayerMpc( dUpload *d ) : m_dupload( d )
{
	connect( dSettings::instance(), &dSettings::settingsChanged, this, &dPlayerMpc::update );

	m_watcher = new QFileSystemWatcher();
	connect( m_watcher, &QFileSystemWatcher::directoryChanged, this, &dPlayerMpc::directoryChanged );

	update();
}

dPlayerMpc::~dPlayerMpc()
{
	m_watcher->deleteLater();
}

void dPlayerMpc::update()
{
	m_enabled = dSettings::instance()->get( "enableMpc", false );

	if ( !m_enabled )
		return;

	if ( m_dir == dSettings::instance()->get< QString >( "mpcScreensPath" ) )
		return;

	m_dir = dSettings::instance()->get< QString >( "mpcScreensPath" );

	if ( m_watcher->directories().size() )
		m_watcher->removePaths( m_watcher->directories() );
	m_watcher->addPath( m_dir );

	m_files = QDir( m_dir ).entryList( QDir::Files | QDir::NoSymLinks | QDir::CaseSensitive, QDir::Name );
}

void dPlayerMpc::directoryChanged( const QString &dir )
{
	if ( !m_enabled )
		return;

	auto files = QDir( m_dir ).entryList( QDir::Files | QDir::NoSymLinks | QDir::CaseSensitive, QDir::Name );

	// It's slowly code.
	QStringList delta12, delta21; // 12 is for m_files and files, 21 is for files and m_files

	foreach( const QString &dir, m_files )
		if ( !files.contains( dir ) )
			delta12.append( dir );

	foreach( const QString &dir, files )
		if ( !m_files.contains( dir ) )
			delta21.append( dir );

	if ( !delta12.count() && delta21.count() == 1 )
	{
		auto timer = new QTimer();
		timer->setSingleShot( true );

		QString filename = dir + "/" + delta21.first();
		int *size = new int;
		*size = -1;

		// It's ugly code, but what can I do?
		connect( timer, &QTimer::timeout, [=]()
			{
				QFile file( filename );
				if ( *size == -1 || *size != file.size() )
				{
					*size = file.size();
					timer->start( 100 );

					return;
				}

				if ( timer->interval() != 500 )
				{
					timer->start( 500 );
					return;
				}

				m_dupload->changed( filename );
				timer->deleteLater();
				delete size;
			}
		);

		timer->start( 100 );
	}

	m_files = files;
}
