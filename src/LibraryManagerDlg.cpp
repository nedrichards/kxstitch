/********************************************************************************
 *	Copyright (C) 2010 by Stephen Allewell					*
 *	stephen@mirramar.adsl24.co.uk						*
 *										*
 *	This program is free software; you can redistribute it and/or modify	*
 *	it under the terms of the GNU General Public License as published by	*
 *	the Free Software Foundation; either version 2 of the License, or	*
 *	(at your option) any later version.					*
 ********************************************************************************/


#include "LibraryManagerDlg.h"

#include <QClipboard>
#include <QDir>
#include <QFileInfoList>
#include <QMimeData>

#include <KInputDialog>
#include <KMessageBox>
#include <KStandardDirs>

#include "LibraryListWidgetItem.h"
#include "LibraryTreeWidgetItem.h"
#include "Pattern.h"

#include "configuration.h"


LibraryManagerDlg::LibraryManagerDlg(QWidget *parent)
	:	KDialog(parent)
{
	setCaption(i18n("Library Manager"));
	setButtons(KDialog::Close | KDialog::Help);
	
	QWidget *widget = new QWidget(this);
	ui.setupUi(widget);
	ui.ZoomOut->setIcon(KIcon("zoom-out"));
	ui.ZoomIn->setIcon(KIcon("zoom-in"));
	QMetaObject::connectSlotsByName(this);
	setMainWidget(widget);
	
	refreshLibraries();
	
	ui.LibraryTree->setContextMenuPolicy(Qt::CustomContextMenu);
	ui.LibraryIcons->setContextMenuPolicy(Qt::CustomContextMenu);
	ui.LibraryIcons->changeIconSize(Configuration::icon_DefaultSize());
	ui.IconSizeSlider->setValue(Configuration::icon_DefaultSize());
}


LibraryManagerDlg::~LibraryManagerDlg()
{
}


void LibraryManagerDlg::setCellSize(double cellWidth, double cellHeight)
{
	ui.LibraryIcons->setCellSize(cellWidth, cellHeight);
}


void LibraryManagerDlg::slotButtonClicked(int button)
{
	KDialog::slotButtonClicked(button);
}


void LibraryManagerDlg::on_LibraryTree_customContextMenuRequested(const QPoint &position)
{
	m_contextMenu.clear();
	m_contextMenu.addAction(i18n("New Category"), this, SLOT(newCategory()));
	if (ui.LibraryTree->itemAt(position))
	{
		m_contextMenu.addAction(i18n("Add to Export List"), this, SLOT(addLibraryToExportList()));
		m_contextMenu.addAction(i18n("Properties..."), this, SLOT(libraryProperties()));
		if (QApplication::clipboard()->mimeData()->hasFormat("application/kxstitch"))
		{
			m_contextMenu.addAction(i18n("Paste"), this, SLOT(pasteFromClipboard()));
		}
	}
	m_contextMenu.popup(QCursor::pos());
}


void LibraryManagerDlg::on_LibraryIcons_customContextMenuRequested(const QPoint &position)
{
	m_contextMenu.clear();
	if (ui.LibraryIcons->itemAt(position))
	{
		m_contextMenu.addAction(i18n("Properties..."), this, SLOT(patternProperties()));
		m_contextMenu.addAction(i18n("Add to Export List"), this, SLOT(addPatternToExportList()));
		m_contextMenu.addAction(i18n("Copy"), this, SLOT(copyToClipboard()));
		m_contextMenu.addAction(i18n("Delete"), this, SLOT(deleteLibraryPattern()));
		m_contextMenu.popup(position);
	}
	else
	{
		if (QApplication::clipboard()->mimeData()->hasFormat("application/kxstitch") && ui.LibraryTree->selectedItems().count() == 1)
		{
			m_contextMenu.addAction(i18n("Paste"), this, SLOT(pasteFromClipboard()));
			m_contextMenu.popup(QCursor::pos());
		}
	}
}


void LibraryManagerDlg::on_LibraryTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *)
{
	ui.LibraryIcons->clear();
	LibraryTreeWidgetItem *libraryTreeWidgetItem = dynamic_cast<LibraryTreeWidgetItem *>(current);
	for (LibraryPattern *libraryPattern = libraryTreeWidgetItem->first() ; libraryPattern ; libraryPattern = libraryTreeWidgetItem->next())
	{
		LibraryListWidgetItem *libraryListWidgetItem = new LibraryListWidgetItem(ui.LibraryIcons, libraryPattern);
		libraryPattern->setLibraryListWidgetItem(libraryListWidgetItem);
	}
}


void LibraryManagerDlg::on_IconSizeSlider_valueChanged(int size)
{
	ui.LibraryIcons->changeIconSize(size);
}


void LibraryManagerDlg::newCategory()
{
	QString category;
	LibraryTreeWidgetItem *parent = 0;
	if (ui.LibraryTree->selectedItems().count())
		parent = static_cast<LibraryTreeWidgetItem *>(ui.LibraryTree->selectedItems().at(0));
	
	bool ok = false;
	
	while (!ok)
	{
		QTreeWidgetItem *item = 0;
		category = KInputDialog::getText(i18n("Category"), QString(), QString(), &ok, this);
		if (!ok)
			break;
		if (parent)
		{
			for (int i = 0 ; i < parent->childCount() ; ++i)
				if (parent->child(i)->text(0) == category)
					break;
		}
		else
		{
			QList<QTreeWidgetItem *> rootItems = ui.LibraryTree->findItems(category, Qt::MatchExactly);
			if (!rootItems.isEmpty())
			{
				QTreeWidgetItem *rootItem = rootItems.at(0);
				item = rootItem;
			}
		}
		
		if (item)
		{
			KMessageBox::sorry(this, i18n("This category already exists."), i18n("Category Exists"));
			ok = false;
		}
	}
	
	if (ok)
	{
		LibraryTreeWidgetItem *newItem;
		if (parent)
			newItem = new LibraryTreeWidgetItem(parent, category);
		else
			newItem = new LibraryTreeWidgetItem(ui.LibraryTree, category);
		
		QString path;
		QFileInfo fileInfo;
		
		if (parent)
		{
			fileInfo.setFile(parent->path());
			if (!fileInfo.isWritable())
			{
				path.replace(0, path.indexOf("library"), "");
				path = KGlobal::dirs()->saveLocation("appdata", path);
				fileInfo.setFile(path);
			}
		}
		else
		{
			path = KGlobal::dirs()->saveLocation("appdata", "library");
			fileInfo.setFile(path);
		}
		
		if (fileInfo.dir().mkdir(category))
		{
			path += category;
			path += "/";
			newItem->addPath(path);
		}
		
		ui.LibraryTree->setCurrentItem(newItem);
	}
}


void LibraryManagerDlg::addLibraryToExportList()
{
}


void LibraryManagerDlg::libraryProperties()
{
}


void LibraryManagerDlg::pasteFromClipboard()
{
	LibraryTreeWidgetItem *item = static_cast<LibraryTreeWidgetItem *>(ui.LibraryTree->currentItem());
	Pattern *pattern = new Pattern();
	QByteArray data = QApplication::clipboard()->mimeData()->data("application/kxstitch");
	QDataStream stream(&data, QIODevice::ReadOnly);
	stream >> *pattern;
	item->addPattern(new LibraryPattern(pattern));
	on_LibraryTree_currentItemChanged(static_cast<QTreeWidgetItem *>(item), 0);
}


void LibraryManagerDlg::patternProperties()
{
}


void LibraryManagerDlg::addPatternToExportList()
{
}


void LibraryManagerDlg::copyToClipboard()
{
}


void LibraryManagerDlg::deletePattern()
{
}


void LibraryManagerDlg::refreshLibraries()
{
	ui.LibraryTree->clear();
	QStringList libraryDirectories = KGlobal::dirs()->findDirs("appdata", "library");
	QStringListIterator libraryDirectoriesIterator(libraryDirectories);
	while (libraryDirectoriesIterator.hasNext())
	{
		recurseLibraryDirectory(0, libraryDirectoriesIterator.next());
	}
}


void LibraryManagerDlg::recurseLibraryDirectory(LibraryTreeWidgetItem *parent, const QString &path)
{
	LibraryTreeWidgetItem *libraryTreeWidgetItem = 0;
	QDir directory(path);
	const QFileInfoList directoryEntries = directory.entryInfoList();
	QListIterator<QFileInfo> fileInfoListIterator(directoryEntries);
	while (fileInfoListIterator.hasNext())
	{
		QFileInfo fileInfo = fileInfoListIterator.next();
		if (fileInfo.isDir())
		{
			if (fileInfo.fileName() != "." && fileInfo.fileName() != "..")
			{
				libraryTreeWidgetItem = 0;
				QString subPath = QString("%1%2/").arg(path).arg(fileInfo.fileName());
				if (parent)
				{
					int children = parent->childCount();
					int childIndex = 0;
					while (childIndex < children)
					{
						libraryTreeWidgetItem = dynamic_cast<LibraryTreeWidgetItem *>(parent->child(childIndex));
						if (libraryTreeWidgetItem->text(0) == fileInfo.fileName())
							break;
						childIndex++;
					}
				}
				else
				{
					QList<QTreeWidgetItem *> rootNodes = ui.LibraryTree->findItems(fileInfo.fileName(), Qt::MatchExactly, 0);
					if (!rootNodes.isEmpty())
						libraryTreeWidgetItem = dynamic_cast<LibraryTreeWidgetItem *>(rootNodes[0]);
				}

				if (libraryTreeWidgetItem == 0)
				{
					if (parent)
						libraryTreeWidgetItem = new LibraryTreeWidgetItem(parent, fileInfo.fileName());
					else
					{
						libraryTreeWidgetItem = new LibraryTreeWidgetItem(ui.LibraryTree, fileInfo.fileName());
						ui.LibraryTree->sortItems(0, Qt::AscendingOrder);
					}
				}

				libraryTreeWidgetItem->addPath(subPath);
				recurseLibraryDirectory(libraryTreeWidgetItem, subPath);
			}
		}
	}
}
