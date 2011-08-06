/***************************************************************************
 *   Copyright (C) 2009 by Stephen Allewell                                *
 *   stephen@mirramar.fsnet.co.uk                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#ifndef __LibraryTreeWidgetItem_H
#define __LibraryTreeWidgetItem_H


#include <QList>
#include <QString>
#include <QStringList>
#include <QTreeWidgetItem>

#include "LibraryFile.h"
#include "LibraryPattern.h"


class QTreeWidget;


class LibraryTreeWidgetItem : public QTreeWidgetItem
{
	public:
		LibraryTreeWidgetItem(QTreeWidget *, const QString &);
		LibraryTreeWidgetItem(LibraryTreeWidgetItem *, const QString &);

		int maxHeight();
		LibraryPattern *findCharacter(int, Qt::KeyboardModifiers);
		LibraryPattern *first();
		LibraryPattern *next();

		void addPath(const QString &);
		QString path();
		QStringList paths();
//		void addPattern(KXStitchDragObject *pattern);
		void addPattern(LibraryPattern *);
//		void deletePattern(PreviewIconViewItem *iconViewItem);

	private:
		LibraryFile *writablePath();

		int			m_libraryFilesIndex;
		QList<LibraryFile *>	m_libraryFiles;
};


#endif
