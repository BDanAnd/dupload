/****************************************************************************
 *  dUpload
 *
 *  Copyright (c) 2012-2013 by Belov Nikita <null@deltaz.org>
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

#include "dsettings.h"

dSettings::dSettings()
{
}

dSettings::~dSettings()
{
	m_settingsDialog->deleteLater();
}

dSettings *dSettings::instance()
{
	static dSettings *me = new dSettings();
	return me;
}

void dSettings::show( int index )
{
	if ( m_settingsDialog )
	{
		m_settingsDialog->activateWindow();
		m_settingsDialog->raise();
		m_settingsUi.tabWidget->setCurrentIndex( index );

		return;
	}

	m_settingsDialog = new QDialog();
	m_settingsUi.setupUi( m_settingsDialog );

	m_settingsUi.tabWidget->setCurrentIndex( index );

	// dialog
	m_settingsDialog->setAttribute( Qt::WA_DeleteOnClose, true );
	m_settingsDialog->setAttribute( Qt::WA_QuitOnClose, false );
	m_settingsDialog->setWindowFlags( m_settingsDialog->windowFlags() ^ Qt::WindowContextHelpButtonHint );
	m_settingsDialog->setWindowFlags( m_settingsDialog->windowFlags() ^ Qt::WindowStaysOnTopHint );

	// media playes names
	m_settingsUi.MPNamesEdit->setText( get< QString >( "MPNames" ) );

	// enable media players checkbox
	connect( m_settingsUi.enableMPCheckBox, &QCheckBox::toggled, [ this ]( bool checked )
		{
			set( "enableMP", checked );

			m_settingsUi.mediaPlayersLabel->setEnabled( checked );
			m_settingsUi.MPNamesEdit->setEnabled( checked );

			if ( m_settingsUi.MPNamesEdit->text().isEmpty() )
			{
				m_settingsUi.MPNamesEdit->setText( "mpc-hc.exe" );
			}

			set( "MPNames", m_settingsUi.MPNamesEdit->text() );
		}
	);

	m_settingsUi.enableMPCheckBox->setChecked( get( "enableMP", false ) );
	m_settingsUi.enableMPCheckBox->toggled( m_settingsUi.enableMPCheckBox->isChecked() );

	// highlighter
	highlighterSettingsLoad();

	// highlighter color choose buttons
	connect( m_settingsUi.highlighterLeftButton, &QPushButton::clicked, this, &dSettings::highlighterSettingsColor );
	connect( m_settingsUi.highlighterRightButton, &QPushButton::clicked, this, &dSettings::highlighterSettingsColor );

	// highlighter width boxes
	connect( m_settingsUi.highlighterLeftWidthBox, static_cast< void ( QDoubleSpinBox::* )( double ) >( &QDoubleSpinBox::valueChanged ), this, &dSettings::highlighterSettingsWidth );
	connect( m_settingsUi.highlighterRightWidthBox, static_cast< void ( QDoubleSpinBox::* )( double ) >( &QDoubleSpinBox::valueChanged ), this, &dSettings::highlighterSettingsWidth );

	// close button
	connect( m_settingsUi.closeButton, &QPushButton::clicked, [ this ]( bool )
		{
			m_settingsDialog->close();
		}
	);

	m_settingsDialog->show();
}

template < class T >
void dSettings::set( const QString &key, const T &value )
{
	m_settings.insert( key, QVariant::fromValue( value ) );

	emit settingsChanged();
}

int dSettings::remove( const QString &key )
{
	return m_settings.remove( key );
}

void dSettings::highlighterSettingsLoad()
{
	m_settingsUi.highlighterLeftButton->setStyleSheet( "background-color: " + get< QColor >( "highlighterLeftButtonColor", Qt::red ).name() );
	m_settingsUi.highlighterRightButton->setStyleSheet( "background-color: " + get< QColor >( "highlighterRightButtonColor", Qt::yellow ).name() );

	m_settingsUi.highlighterLeftOpacityLabel->setText( QString::number( qFloor( get( "highlighterLeftButtonOpacity", 1.0 ) * 255 ) ) );
	m_settingsUi.highlighterRightOpacityLabel->setText( QString::number( qFloor( get( "highlighterRightButtonOpacity", 0.3 ) * 255 ) ) );

	m_settingsUi.highlighterLeftWidthBox->setValue( get( "highlighterLeftButtonWidth", 4.0 ) );
	m_settingsUi.highlighterRightWidthBox->setValue( get( "highlighterRightButtonWidth", 20.0 ) );
}

void dSettings::highlighterSettingsColor( bool /*checked*/ )
{
	QPushButton *button = qobject_cast< QPushButton * >( sender() );

	QString name = button->objectName();
	QString buttonName = ( name == "highlighterLeftButton" ? "left" : "right" );
	bool left = ( name == "highlighterLeftButton" ? true : false );
	
	QColor color = get< QColor >( name + "Color", ( left ? Qt::red : Qt::yellow ) );
	color.setAlphaF( get< qreal >( name + "Opacity", ( left ? 1.0 : 0.3 ) ) );

	color = QColorDialog::getColor( color, m_settingsDialog, QString( "Choose color for %1 button" ).arg( buttonName ), QColorDialog::ShowAlphaChannel );

	if ( !color.isValid() )
		return;

	set( name + "Opacity", color.alphaF() );

	color.setAlphaF( 1.0 );
	set( name + "Color", color );

	highlighterSettingsLoad();
}

void dSettings::highlighterSettingsWidth( double value )
{
	QDoubleSpinBox *box = qobject_cast< QDoubleSpinBox * >( sender() );

	QString name = ( box->objectName() == "highlighterLeftWidthBox" ? "highlighterLeftButtonWidth" : "highlighterRightButtonWidth" );
	set( name, value );

	highlighterSettingsLoad();
}
