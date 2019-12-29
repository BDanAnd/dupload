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

#ifndef DFILTER_H
#define DFILTER_H

#include <QtWidgets/QWidget>
#include <QDir>

#include "dupload.h"
#include "ui_dfilter.h"

class dFilter : public QWidget
{
	Q_OBJECT

public:
	dFilter( dUpload *d );
	~dFilter();

protected:
	void keyPressEvent( QKeyEvent *event );
	void resizeEvent( QResizeEvent *event );

private:
	void loadFilters();
	void filterActivated( QListWidgetItem *current, QListWidgetItem * );

	Ui::dFilterClass ui;
	dUpload *m_dupload;
	QPixmap m_original;
	QPixmap m_current;
	QDir filters_dir;
};

#endif // DFILTER_H
