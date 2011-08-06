/********************************************************************************
 *	Copyright (C) 2010 by Stephen Allewell					*
 *	stephen@mirramar.adsl24.co.uk						*
 *										*
 *	This program is free software; you can redistribute it and/or modify	*
 *	it under the terms of the GNU General Public License as published by	*
 *	the Free Software Foundation; either version 2 of the License, or	*
 *	(at your option) any later version.					*
 ********************************************************************************/


#include <QAction>
#include <QBitmap>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QRubberBand>
#include <QScrollArea>
#include <QStyleOptionRubberBand>

#include <KAction>
#include <KMessageBox>
#include <KXMLGUIFactory>

#include <math.h>

#include "BackgroundImage.h"
#include "Commands.h"
#include "Document.h"
#include "Editor.h"
#include "Floss.h"
#include "MainWindow.h"
#include "Preview.h"
#include "Renderer.h"
#include "Scale.h"


const Editor::mouseEventCallPointer Editor::mousePressEventCallPointers[] =
{
	&Editor::mousePressEvent_Paint,
	&Editor::mousePressEvent_Draw,
	&Editor::mousePressEvent_Erase,
	&Editor::mousePressEvent_Rectangle,
	&Editor::mousePressEvent_FillRectangle,
	&Editor::mousePressEvent_Ellipse,
	&Editor::mousePressEvent_FillEllipse,
	&Editor::mousePressEvent_FillPolyline,
	&Editor::mousePressEvent_Text,
	&Editor::mousePressEvent_Select,
	&Editor::mousePressEvent_Backstitch
};

const Editor::mouseEventCallPointer Editor::mouseMoveEventCallPointers[] =
{
	&Editor::mouseMoveEvent_Paint,
	&Editor::mouseMoveEvent_Draw,
	&Editor::mouseMoveEvent_Erase,
	&Editor::mouseMoveEvent_Rectangle,
	&Editor::mouseMoveEvent_FillRectangle,
	&Editor::mouseMoveEvent_Ellipse,
	&Editor::mouseMoveEvent_FillEllipse,
	&Editor::mouseMoveEvent_FillPolyline,
	&Editor::mouseMoveEvent_Text,
	&Editor::mouseMoveEvent_Select,
	&Editor::mouseMoveEvent_Backstitch
};

const Editor::mouseEventCallPointer Editor::mouseReleaseEventCallPointers[] =
{
	&Editor::mouseReleaseEvent_Paint,
	&Editor::mouseReleaseEvent_Draw,
	&Editor::mouseReleaseEvent_Erase,
	&Editor::mouseReleaseEvent_Rectangle,
	&Editor::mouseReleaseEvent_FillRectangle,
	&Editor::mouseReleaseEvent_Ellipse,
	&Editor::mouseReleaseEvent_FillEllipse,
	&Editor::mouseReleaseEvent_FillPolyline,
	&Editor::mouseReleaseEvent_Text,
	&Editor::mouseReleaseEvent_Select,
	&Editor::mouseReleaseEvent_Backstitch
};

const Editor::renderToolSpecificGraphicsCallPointer Editor::renderToolSpecificGraphics[] =
{
	0,					// Paint
	&Editor::renderRubberBandLine,		// Draw
	0,					// Erase
	&Editor::renderRubberBandRectangle,	// Rectangle
	&Editor::renderRubberBandRectangle,	// Fill Rectangle
	&Editor::renderRubberBandEllipse,	// Ellipse
	&Editor::renderRubberBandEllipse,	// Fill Ellipse
	0,					// Fill Polyline
	0,					// Text
	&Editor::renderRubberBandRectangle,	// Select
	&Editor::renderRubberBandLine		// Backstitch
};


const Stitch::Type stitchMap[][4] =
{
	{
		Stitch::TLQtr,
		Stitch::TRQtr,
		Stitch::BLQtr,
		Stitch::BRQtr
	},
	{
		Stitch::TBHalf,
		Stitch::BTHalf,
		Stitch::BTHalf,
		Stitch::TBHalf
	},
	{
		Stitch::TL3Qtr,
		Stitch::TR3Qtr,
		Stitch::BL3Qtr,
		Stitch::BR3Qtr
	},
	{
		Stitch::Full,
		Stitch::Full,
		Stitch::Full,
		Stitch::Full
	},
	{
		Stitch::TLSmallHalf,
		Stitch::TRSmallHalf,
		Stitch::BLSmallHalf,
		Stitch::BRSmallHalf
	},
	{
		Stitch::TLSmallFull,
		Stitch::TRSmallFull,
		Stitch::BLSmallFull,
		Stitch::BRSmallFull
	}
};


Editor::Editor(QWidget *parent)
	:	QWidget(parent)
{
	m_horizontalScale = new Scale(Qt::Horizontal);
	m_verticalScale = new Scale(Qt::Vertical);

	m_renderer = new Renderer();

	// TODO this will be iniitalised from the document layers, but managed from the editor preferences
	m_layerOrder.append(0);
	m_visibleLayers.append(0);
}


Editor::~Editor()
{
	delete m_renderer;
}


void Editor::setDocument(Document *document)
{
	m_document = document;
	readDocumentSettings();
}


Document *Editor::document()
{
	return m_document;
}


void Editor::setPreview(Preview *preview)
{
	m_preview = preview;
}


Scale *Editor::horizontalScale()
{
	return m_horizontalScale;
}


Scale *Editor::verticalScale()
{
	return m_verticalScale;
}


void Editor::readDocumentSettings()
{
	m_cellWidth = m_document->property("cellWidth").toInt();
	m_cellHeight = m_document->property("cellHeight").toInt();
	m_cellHorizontalGrouping = m_document->property("cellHorizontalGrouping").toInt();
	m_cellVerticalGrouping = m_document->property("cellVerticalGrouping").toInt();

	m_horizontalScale->setCellSize(m_cellWidth);
	m_horizontalScale->setCellGrouping(m_cellHorizontalGrouping);
	m_horizontalScale->setCellCount(m_document->stitchData().width());
	m_horizontalScale->setClothCount(m_document->property("horizontalClothCount").toDouble());
	m_horizontalScale->setUnits(static_cast<Configuration::EnumEditor_FormatScalesAs::type>(m_document->property("formatScalesAs").toInt()));

	m_verticalScale->setCellSize(m_cellHeight);
	m_verticalScale->setCellGrouping(m_cellVerticalGrouping);
	m_verticalScale->setCellCount(m_document->stitchData().height());
	m_verticalScale->setClothCount(m_document->property("verticalClothCount").toDouble());
	m_verticalScale->setUnits(static_cast<Configuration::EnumEditor_FormatScalesAs::type>(m_document->property("formatScalesAs").toInt()));

	m_renderer->setCellSize(m_cellWidth, m_cellHeight);

	m_horizontalScale->setOffset(pos().x());
	m_verticalScale->setOffset(pos().y());

	resizeEditor(); // this will do an update
}


void Editor::resizeEditor()
{
	this->resize(m_document->stitchData().width()*m_cellWidth + 1, m_document->stitchData().height()*m_cellHeight + 1);
}


void Editor::zoom(double factor)
{
	m_cellWidth = (int)((m_document->property("cellWidth").toDouble())*factor);
	m_cellHeight = (int)((m_document->property("cellHeight").toDouble())*factor);
	m_horizontalScale->setCellSize(m_cellWidth);
	m_verticalScale->setCellSize(m_cellHeight);
	m_renderer->setCellSize(m_cellWidth, m_cellHeight);
	this->resize(m_document->stitchData().width()*m_cellWidth + 1, m_document->stitchData().height()*m_cellHeight + 1);
	emit changedVisibleCells(visibleCells());
}


void Editor::fitToPage()
{
	int oldCellWidth = m_cellWidth;
	int oldCellHeight = m_cellHeight;

	int documentWidth = m_document->stitchData().width();
	int documentHeight = m_document->stitchData().height();
	int documentCellWidth = m_document->property("cellWidth").toInt();
	int documentCellHeight = m_document->property("cellHeight").toInt();

	QRect visibleArea = parentWidget()->contentsRect();
	int visibleWidth = visibleArea.width();
	int visibleHeight = visibleArea.height();

	m_cellWidth = visibleWidth / documentWidth;    // divide the visible width by the number of horizontal cells
	m_cellHeight = visibleHeight / documentHeight; // divide the visible height by the number of vertical cells

	if (m_cellWidth < 2 || m_cellHeight < 2)
	{
		KMessageBox::information(this, "Pattern is to large to display full size.");
		m_cellWidth = oldCellWidth;
		m_cellHeight = oldCellHeight;
	}
	else
	{
		int ratioWidth  = m_cellHeight * documentCellWidth / documentCellHeight;
		int ratioHeight = m_cellWidth * documentCellHeight / documentCellWidth;

		if (ratioHeight > ratioWidth)
			m_cellWidth = ratioWidth;
		else
			m_cellHeight = ratioHeight;

		m_horizontalScale->setCellSize(m_cellWidth);
		m_verticalScale->setCellSize(m_cellHeight);

		this->resize(documentWidth*m_cellWidth + 1, documentHeight*m_cellHeight + 1);
	}
}


void Editor::fitToWidth()
{
	int oldCellWidth = m_cellWidth;
	int oldCellHeight = m_cellHeight;

	int documentWidth = m_document->stitchData().width();
	int documentHeight = m_document->stitchData().height();
	int documentCellWidth = m_document->property("cellWidth").toInt();
	int documentCellHeight = m_document->property("cellHeight").toInt();

	m_cellWidth = parentWidget()->contentsRect().width() / documentWidth;
	m_cellHeight = m_cellWidth * m_document->property("cellHeight").toInt() / m_document->property("cellWidth").toInt();

	if (m_cellHeight < 2 || m_cellWidth < 2)
	{
		KMessageBox::information(this, "Pattern is to large to display full Width.");
		m_cellWidth = oldCellWidth;
		m_cellHeight = oldCellHeight;
	}
	else
	{
		m_horizontalScale->setCellSize(m_cellWidth);
		m_verticalScale->setCellSize(m_cellHeight);

		this->resize(documentWidth*m_cellWidth + 1, documentHeight*m_cellHeight + 1);
	}
}


void Editor::fitToHeight()
{
	int oldCellWidth = m_cellWidth;
	int oldCellHeight = m_cellHeight;

	int documentWidth = m_document->stitchData().width();
	int documentHeight = m_document->stitchData().height();
	int documentCellWidth = m_document->property("cellWidth").toInt();
	int documentCellHeight = m_document->property("cellHeight").toInt();

	m_cellHeight = parentWidget()->contentsRect().height() / documentHeight;
	m_cellWidth = m_cellHeight * documentCellWidth / documentCellHeight;

	if (m_cellHeight < 2 || m_cellWidth < 2)
	{
		KMessageBox::information(this, "Pattern is to large to display full height.");
		m_cellWidth = oldCellWidth;
		m_cellHeight = oldCellHeight;
	}
	else
	{
		m_horizontalScale->setCellSize(m_cellWidth);
		m_verticalScale->setCellSize(m_cellHeight);

		this->resize(documentWidth*m_cellWidth + 1, documentHeight*m_cellHeight + 1);
	}
}


void Editor::renderStitches(bool show)
{
	m_renderStitches = show;
	update();
}


void Editor::renderBackstitches(bool show)
{
	m_renderBackstitches = show;
	update();
}


void Editor::renderFrenchKnots(bool show)
{
	m_renderFrenchKnots = show;
	update();
}


void Editor::renderGrid(bool show)
{
	m_renderGrid = show;
	update();
}


void Editor::renderBackgroundImages(bool show)
{
	m_renderBackgroundImages = show;
	update();
}


void Editor::renderStitchesAs()
{
	m_renderStitchesAs = static_cast<Configuration::EnumRenderer_RenderStitchesAs::type>(qobject_cast<QAction *>(sender())->data().toInt());
	m_renderer->setRenderStitchesAs(m_renderStitchesAs);
	update();
}


void Editor::renderBackstitchesAs()
{
	m_renderBackstitchesAs = static_cast<Configuration::EnumRenderer_RenderBackstitchesAs::type>(qobject_cast<QAction *>(sender())->data().toInt());
	m_renderer->setRenderBackstitchesAs(m_renderBackstitchesAs);
	update();
}


void Editor::renderKnotsAs()
{
	m_renderKnotsAs = static_cast<Configuration::EnumRenderer_RenderKnotsAs::type>(qobject_cast<QAction *>(sender())->data().toInt());
	m_renderer->setRenderKnotsAs(m_renderKnotsAs);
	update();
}


void Editor::colorHilight(bool set)
{
	m_colorHilight = set;
	update();
}


void Editor::selectTool()
{
	if (m_toolMode == Editor::ToolSelect)
	{
		if (m_rubberBand.isValid())
		{
			QRect r = m_rubberBand;
			m_rubberBand = QRect(0, 0, 0, 0);
			repaint(r);
		}
		emit(selectionMade(false));
	}
	m_toolMode = static_cast<Editor::ToolMode>(qobject_cast<QAction *>(sender())->data().toInt());
}


void Editor::selectStitch()
{
	m_currentStitchType = static_cast<Editor::SelectedStitchType>(qobject_cast<QAction *>(sender())->data().toInt());
}


void Editor::setMaskLayer(bool set)
{
	m_maskLayer = set;
}


void Editor::setMaskStitch(bool set)
{
	m_maskStitch = set;
}


void Editor::setMaskColor(bool set)
{
	m_maskColor = set;
}


void Editor::setMaskBackstitch(bool set)
{
	m_maskBackstitch = set;
}


void Editor::setMaskKnot(bool set)
{
	m_maskKnot = set;
}


void Editor::contextMenuEvent(QContextMenuEvent *e)
{
	MainWindow *mainWindow = qobject_cast<MainWindow *>(topLevelWidget());
	QMenu *context = static_cast<QMenu *>(mainWindow->guiFactory()->container("EditorPopup", mainWindow));
	context->popup(e->globalPos());
	e->accept();
}


void Editor::dragEnterEvent(QDragEnterEvent*)
{
}


void Editor::dragLeaveEvent(QDragLeaveEvent*)
{
}


void Editor::dragMoveEvent(QDragMoveEvent*)
{
}


void Editor::dropEvent(QDropEvent*)
{
}


void Editor::keyPressEvent(QKeyEvent*)
{
}


void Editor::keyReleaseEvent(QKeyEvent*)
{
}


void Editor::mousePressEvent(QMouseEvent *e)
{
	if ((e->buttons() & Qt::LeftButton) && ((m_document->documentPalette().currentIndex() != -1) || (m_toolMode == Editor::ToolSelect)))
		(this->*mousePressEventCallPointers[m_toolMode])(e);
}


void Editor::mouseMoveEvent(QMouseEvent *e)
{
	if ((e->buttons() & Qt::LeftButton) && ((m_document->documentPalette().currentIndex() != -1) || (m_toolMode == Editor::ToolSelect)))
		(this->*mouseMoveEventCallPointers[m_toolMode])(e);
}


void Editor::mouseReleaseEvent(QMouseEvent *e)
{
	if ((m_document->documentPalette().currentIndex() != -1) || (m_toolMode == Editor::ToolSelect))
		(this->*mouseReleaseEventCallPointers[m_toolMode])(e);
}



void Editor::paintEvent(QPaintEvent *e)
{
	static QPoint oldpos = pos();

	QPainter *painter = new QPainter();
	painter->begin(this);

	painter->fillRect(e->rect(), m_document->property("fabricColor").value<QColor>());

	if (m_renderBackgroundImages) renderBackgroundImages(painter, e->rect());
	if (m_renderGrid) renderGrid(painter, e->rect());
	m_renderer->render(painter,
			   m_document,
			   e->rect(),
			   m_layerOrder,
			   m_visibleLayers,
			   m_renderStitches,
			   m_renderBackstitches,
			   m_renderFrenchKnots,
			   (m_colorHilight)?m_document->documentPalette().currentIndex():-1);
	if (renderToolSpecificGraphics[m_toolMode]) (this->*renderToolSpecificGraphics[m_toolMode])(painter, e->rect());

	painter->end();
	delete painter;

	if (pos() != oldpos)
	{
		m_horizontalScale->setOffset(pos().x());
		m_verticalScale->setOffset(pos().y());
		oldpos = pos();
	}

	emit changedVisibleCells(visibleCells());
}


void Editor::renderBackgroundImages(QPainter *painter, QRect updateRectangle)
{
	QListIterator<BackgroundImage *> backgroundImages = m_document->backgroundImages().backgroundImages();
	while (backgroundImages.hasNext())
	{
		BackgroundImage *background = backgroundImages.next();
		if (background->isVisible())
		{
			QRect r = background->location();
			r.moveTo(r.left()*m_cellWidth, r.top()*m_cellHeight);
			r.setWidth(r.width()*m_cellWidth);
			r.setHeight(r.height()*m_cellHeight);
			if (r.intersects(updateRectangle))
				painter->drawImage(r, background->image());
		}
	}
}


void Editor::renderGrid(QPainter *painter, QRect updateRectangle)
{
	painter->save();

	int gridMaxX = std::min(updateRectangle.right(),(int)(m_document->stitchData().width()*m_cellWidth))+1;
	int gridMaxY = std::min(updateRectangle.bottom(),(int)(m_document->stitchData().height()*m_cellHeight))+1;

	QPen thickLine;
	QPen thinLine;
	thickLine.setWidth(m_document->property("thickLineWidth").toInt());
	thickLine.setColor(m_document->property("thickLineColor").value<QColor>());
	thinLine.setWidth(m_document->property("thinLineWidth").toInt());
	thinLine.setColor(m_document->property("thinLineColor").value<QColor>());

	for (int x = updateRectangle.left() ; x <= gridMaxX ; x++)
	{
		for (int y = updateRectangle.top() ; y <= gridMaxY ; y++)
		{
			if ((y % m_cellHeight) == 0)
			{
				if (y % (m_cellHeight*m_cellVerticalGrouping))
					painter->setPen(thinLine);
				else
					painter->setPen(thickLine);
				painter->drawLine(updateRectangle.left(), y, gridMaxX, y);
			}
		}
		if ((x % m_cellWidth) == 0)
		{
			if (x % (m_cellWidth*m_cellHorizontalGrouping))
				painter->setPen(thinLine);
			else
				painter->setPen(thickLine);
			painter->drawLine(x, updateRectangle.top(), x, gridMaxY);
		}
	}
	painter->restore();
}


void Editor::renderRubberBandLine(QPainter *painter, QRect)
{
	painter->save();
	if (m_rubberBand.isValid())
	{
		QPen pen(m_document->documentPalette().currentFloss()->flossColor());
		pen.setWidth(4);
		if (m_toolMode == ToolBackstitch)
			painter->drawLine(snapToContents(m_cellStart), snapToContents(m_cellEnd));
		else
			painter->drawLine(cellToRect(m_cellStart).center(), cellToRect(m_cellEnd).center());
	}
	painter->restore();
}


void Editor::renderRubberBandRectangle(QPainter *painter, QRect)
{
	painter->save();
	if (m_rubberBand.isValid())
	{
		QStyleOptionRubberBand opt;
		opt.initFrom(this);
		opt.shape = QRubberBand::Rectangle;
		opt.opaque = false;
		opt.rect = m_rubberBand;
		style()->drawControl(QStyle::CE_RubberBand, &opt, painter);
	}
	painter->restore();
}


void Editor::renderRubberBandEllipse(QPainter *painter, QRect)
{
	painter->save();
	if (m_rubberBand.isValid())
	{
		QPainterPath path;
		path.addEllipse(m_rubberBand);
		painter->setBrush(QColor(Qt::cyan));
		painter->setPen(Qt::black);
		painter->drawPath(path);
	}
	painter->restore();
}


void Editor::mousePressEvent_Paint(QMouseEvent *e)
{
	QPoint p = e->pos();
	QRect rect;

	if (m_currentStitchType == StitchFrenchKnot)
	{
		if (QRect(0, 0, m_document->stitchData().width()*m_cellWidth, m_document->stitchData().height()*m_cellHeight).contains(p))
		{
			m_cellStart = m_cellTracking = m_cellEnd = contentsToSnap(p);
			m_document->undoStack().beginMacro("Paint knots");
			m_document->undoStack().push(new UpdateEditorCommand(this));
			m_document->undoStack().push(new UpdatePreviewCommand(m_preview));
			m_document->undoStack().push(new AddKnotCommand(m_document, m_document->layers().currentLayer(), m_cellStart, m_document->documentPalette().currentIndex()));
			rect = QRect(snapToContents(m_cellStart)-(QPoint(m_cellWidth, m_cellHeight)/2), QSize(m_cellWidth, m_cellHeight));
		}
	}
	else
	{
		m_cellStart = m_cellTracking = m_cellEnd = contentsToCell(p);
		m_zoneStart = m_zoneTracking = m_zoneEnd = contentsToZone(p);
		Stitch::Type stitchType = stitchMap[m_currentStitchType][m_zoneStart];
		m_document->undoStack().beginMacro("Paint stitches");
		m_document->undoStack().push(new UpdateEditorCommand(this));
		m_document->undoStack().push(new UpdatePreviewCommand(m_preview));
		m_document->undoStack().push(new AddStitchCommand(m_document, m_document->layers().currentLayer(), m_cellStart, stitchType, m_document->documentPalette().currentIndex()));
		rect = cellToRect(m_cellStart);
	}
	update(rect);
}


void Editor::mouseMoveEvent_Paint(QMouseEvent *e)
{
	QPoint p = e->pos();
	QRect rect;

	if (m_currentStitchType == StitchFrenchKnot)
	{
		if (QRect(0, 0, m_document->stitchData().width()*m_cellWidth, m_document->stitchData().height()*m_cellHeight).contains(p))
		{
			m_cellTracking = contentsToSnap(p);
			if (m_cellTracking != m_cellStart)
			{
				m_cellStart = m_cellTracking;
				m_document->undoStack().push(new AddKnotCommand(m_document, m_document->layers().currentLayer(), m_cellStart, m_document->documentPalette().currentIndex()));
				rect = QRect(snapToContents(m_cellStart)-(QPoint(m_cellWidth, m_cellHeight)/2), QSize(m_cellWidth, m_cellHeight));
			}
		}
	}
	else
	{
		m_cellTracking = contentsToCell(p);
		m_zoneTracking = contentsToZone(p);
		if ((m_cellTracking != m_cellStart) || (m_zoneTracking != m_zoneStart))
		{
			m_cellStart = m_cellTracking;
			m_zoneStart = m_zoneTracking;
			Stitch::Type stitchType = stitchMap[m_currentStitchType][m_zoneStart];
			m_document->undoStack().push(new AddStitchCommand(m_document, m_document->layers().currentLayer(), m_cellStart, stitchType, m_document->documentPalette().currentIndex()));
			rect = cellToRect(m_cellStart);
		}
	}
	update(rect);
}


void Editor::mouseReleaseEvent_Paint(QMouseEvent*)
{
	m_document->undoStack().endMacro();
	m_preview->update();
}


void Editor::mousePressEvent_Draw(QMouseEvent *e)
{
	m_cellStart = m_cellTracking = m_cellEnd = contentsToCell(e->pos());
	m_rubberBand = QRect();
}


void Editor::mouseMoveEvent_Draw(QMouseEvent *e)
{
	QPoint p = e->pos();
	QRect updateArea;

	//  qobject_cast<QScrollArea *>(parent())->ensureVisible(p.x(), p.y());

	m_cellTracking = contentsToCell(p);
	updateArea = (cellToRect(m_cellStart).united(cellToRect(m_cellEnd))).normalized();
	m_rubberBand = QRect();
	update(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));

	m_cellEnd = m_cellTracking;
	updateArea = (cellToRect(m_cellStart).united(cellToRect(m_cellEnd))).normalized();
	m_rubberBand = updateArea;
	update(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
}


void Editor::mouseReleaseEvent_Draw(QMouseEvent*)
{
	QBitmap canvas(m_document->stitchData().width(), m_document->stitchData().height());
	QRect rect;
	QPainter painter;

	if (m_cellStart != m_cellEnd)
	{
		canvas.fill(Qt::color0);
		painter.begin(&canvas);
		painter.setPen(QPen(Qt::color1));
		painter.drawLine(m_cellStart, m_cellEnd);
		painter.drawPoint(m_cellStart); // TODO see if this is still necessary, possible controlled by end cap
		painter.drawPoint(m_cellEnd); // TODO see if this is still necessary, possible controlled by end cap
		painter.end();
		processBitmap(canvas, "Draw line");
	}
	m_rubberBand = QRect();
	update();
}


void Editor::mousePressEvent_Erase(QMouseEvent *e)
{
	QPoint p = e->pos();
	QRect rect;

	if (e->modifiers() & Qt::ControlModifier)
	{
		// Erase a backstitch
		m_cellStart = m_cellTracking = m_cellEnd = contentsToSnap(p);
	}
	else
	{
		if (e->modifiers() & Qt::ShiftModifier)
		{
			// Delete french knots
			m_cellStart = m_cellTracking = m_cellEnd = contentsToSnap(p);
			m_document->undoStack().beginMacro("Delete knots");
			m_document->undoStack().push(new UpdateEditorCommand(this));
			m_document->undoStack().push(new UpdatePreviewCommand(m_preview));
			m_document->undoStack().push(new DeleteKnotCommand(m_document, /*(m_maskLayer)?*/m_document->layers().currentLayer()/*:-1*/, m_cellStart, (m_maskColor)?m_document->documentPalette().currentIndex():-1));
			rect = QRect(snapToContents(m_cellStart)-QPoint(m_cellWidth/2, m_cellHeight/2), QSize(m_cellWidth, m_cellHeight));
		}
		else
		{
			m_cellStart = m_cellTracking = m_cellEnd = contentsToCell(p);
			m_zoneStart = m_zoneTracking = m_zoneEnd = contentsToZone(p);
			m_document->undoStack().beginMacro("Delete stitches");
			m_document->undoStack().push(new UpdateEditorCommand(this));
			m_document->undoStack().push(new UpdatePreviewCommand(m_preview));
			m_document->undoStack().push(new DeleteStitchCommand(m_document, /*(m_maskLayer)?*/m_document->layers().currentLayer()/*:-1*/, m_cellStart, m_maskStitch?stitchMap[m_currentStitchType][m_zoneStart]:Stitch::Delete, m_maskColor?m_document->documentPalette().currentIndex():-1));
			rect = cellToRect(m_cellStart);
		}
		update(rect);
	}
}


void Editor::mouseMoveEvent_Erase(QMouseEvent *e)
{
	QPoint p = e->pos();
	QRect rect;

	if (e->modifiers() & Qt::ControlModifier)
	{
		// Erasing a backstitch
		// Don't need to do anything here
	}
	else
	{
		if (e->modifiers() & Qt::ShiftModifier)
		{
			// Delete french knots
			m_cellTracking = contentsToSnap(p);
			if (m_cellTracking != m_cellStart)
			{
				m_cellStart = m_cellTracking;
				m_document->undoStack().push(new DeleteKnotCommand(m_document, /*(m_maskLayer)?*/m_document->layers().currentLayer()/*:-1*/, m_cellStart, (m_maskColor)?m_document->documentPalette().currentIndex():-1));
				rect = QRect(snapToContents(m_cellStart)-QPoint(m_cellWidth/2, m_cellHeight/2), QSize(m_cellWidth, m_cellHeight));
			}
		}
		else
		{
			m_cellTracking = contentsToCell(p);
			m_zoneTracking = contentsToZone(p);
			if ((m_cellTracking != m_cellStart) || (m_zoneTracking != m_zoneStart))
			{
				m_cellStart = m_cellTracking;
				m_zoneStart = m_zoneTracking;
				m_document->undoStack().push(new DeleteStitchCommand(m_document, /*(m_maskLayer)?*/m_document->layers().currentLayer()/*:-1*/, m_cellStart, m_maskStitch?stitchMap[m_currentStitchType][m_zoneStart]:Stitch::Delete, m_maskColor?m_document->documentPalette().currentIndex():-1));
				rect = cellToRect(m_cellStart);
			}
		}
		update(rect);
	}
}


void Editor::mouseReleaseEvent_Erase(QMouseEvent *e)
{
	QPoint p = e->pos();
	QRect rect;

	if (e->modifiers() & Qt::ControlModifier)
	{
		// Erase a backstitch
		m_cellEnd = contentsToSnap(p);
		QListIterator<int> layers(m_document->stitchData().backstitchLayers());
		while (layers.hasNext())
		{
			int layer = layers.next();
			QListIterator<Backstitch *> backstitches = m_document->stitchData().backstitchIterator(layer);
			while (backstitches.hasNext())
			{
				Backstitch *backstitch = backstitches.next();
				if (backstitch->contains(m_cellStart) && backstitch->contains(m_cellEnd))
				{
					if (/*!m_maskLayer || */(m_document->layers().currentLayer() == layer))
					{
						m_document->undoStack().push(new DeleteBackstitchCommand(m_document, layer, backstitch->start, backstitch->end, m_maskColor?m_document->documentPalette().currentIndex():-1));
						update(QRect(snapToContents(backstitch->start), snapToContents(backstitch->end)).normalized().adjusted(-2, -2, 2, 2));
						break;
					}
				}
			}
		}
	}
	else
	{
		m_document->undoStack().endMacro();
		m_preview->update();
	}
	// Nothing needs to be done for french knots or stitches which are handled in mouseMoveEvent_Erase
}


void Editor::mousePressEvent_Rectangle(QMouseEvent *event)
{
	if (m_rubberBand.isValid())
	{
		QRect r = m_rubberBand;
		m_rubberBand = QRect();
		repaint(r);
	}

	m_cellStart = m_cellTracking = m_cellEnd = contentsToCell(event->pos());
	QRect updateArea = cellToRect(m_cellStart);
	m_rubberBand = updateArea;
	repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
}


void Editor::mouseMoveEvent_Rectangle(QMouseEvent *event)
{
	m_cellTracking = contentsToCell(event->pos());
	if (m_cellTracking != m_cellEnd)
	{
		m_cellEnd = m_cellTracking;
		QRect updateArea = (cellToRect(m_cellStart).united(cellToRect(m_cellEnd))).normalized();
		m_rubberBand = updateArea;
		repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
	}
}


void Editor::mouseReleaseEvent_Rectangle(QMouseEvent*)
{
	m_selectionArea.setTopLeft(m_cellStart);
	m_selectionArea.setBottomRight(m_cellEnd);
	m_selectionArea = m_selectionArea.normalized();

	int x = m_selectionArea.left();
	int y = m_selectionArea.top();
	QPoint cell(x, y);

	m_document->undoStack().beginMacro("Draw rectangle");
	m_document->undoStack().push(new UpdateEditorCommand(this));
	m_document->undoStack().push(new UpdatePreviewCommand(m_preview));
	while (++x <= m_selectionArea.right())
	{
		m_document->undoStack().push(new AddStitchCommand(m_document, m_document->layers().currentLayer(), cell, Stitch::Full, m_document->documentPalette().currentIndex()));
		cell.setX(x);
	}
	while (++y <= m_selectionArea.bottom())
	{
		m_document->undoStack().push(new AddStitchCommand(m_document, m_document->layers().currentLayer(), cell, Stitch::Full, m_document->documentPalette().currentIndex()));
		cell.setY(y);
	}
	while (--x >= m_selectionArea.left())
	{
		m_document->undoStack().push(new AddStitchCommand(m_document, m_document->layers().currentLayer(), cell, Stitch::Full, m_document->documentPalette().currentIndex()));
		cell.setX(x);
	}
	while (--y >= m_selectionArea.top())
	{
		m_document->undoStack().push(new AddStitchCommand(m_document, m_document->layers().currentLayer(), cell, Stitch::Full, m_document->documentPalette().currentIndex()));
		cell.setY(y);
	}
	m_document->undoStack().endMacro();

	QRect updateArea = (cellToRect(m_cellStart).united(cellToRect(m_cellEnd))).normalized();
	m_selectionArea = QRect();	// this will clear the selection area rectangle on the next repaint
	m_rubberBand = QRect();		// this will clear the rubber band rectangle on the next repaint
	update(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
	m_preview->update();
}


void Editor::mousePressEvent_FillRectangle(QMouseEvent *event)
{
	if (m_rubberBand.isValid())
	{
		QRect r = m_rubberBand;
		m_rubberBand = QRect();
		repaint(r);
	}

	m_cellStart = m_cellTracking = m_cellEnd = contentsToCell(event->pos());
	QRect updateArea = cellToRect(m_cellStart);
	m_rubberBand = updateArea;
	repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
}


void Editor::mouseMoveEvent_FillRectangle(QMouseEvent *event)
{
	m_cellTracking = contentsToCell(event->pos());
	if (m_cellTracking != m_cellEnd)
	{
		m_cellEnd = m_cellTracking;
		QRect updateArea = (cellToRect(m_cellStart).united(cellToRect(m_cellEnd))).normalized();
		m_rubberBand = updateArea;
		repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
	}
}


void Editor::mouseReleaseEvent_FillRectangle(QMouseEvent *event)
{
	m_selectionArea.setTopLeft(m_cellStart);
	m_selectionArea.setBottomRight(m_cellEnd);
	m_selectionArea = m_selectionArea.normalized();

	int x = m_selectionArea.left();
	int y = m_selectionArea.top();
	QPoint cell(x, y);

	m_document->undoStack().beginMacro("Fill rectangle");
	m_document->undoStack().push(new UpdateEditorCommand(this));
	m_document->undoStack().push(new UpdatePreviewCommand(m_preview));
	for (int y = m_selectionArea.top() ; y <= m_selectionArea.bottom() ; y++)
	{
		for (int x = m_selectionArea.left() ; x <= m_selectionArea.right() ; x++)
		{
			QPoint cell(x, y);
			m_document->undoStack().push(new AddStitchCommand(m_document, m_document->layers().currentLayer(), cell, Stitch::Full, m_document->documentPalette().currentIndex()));
		}
	}
	m_document->undoStack().endMacro();

	QRect updateArea = (cellToRect(m_cellStart).united(cellToRect(m_cellEnd))).normalized();
	m_selectionArea = QRect();	// this will clear the selection area rectangle on the next repaint
	m_rubberBand = QRect();		// this will clear the rubber band rectangle on the next repaint
	update(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
	m_preview->update();
}


void Editor::mousePressEvent_Ellipse(QMouseEvent*)
{
}


void Editor::mouseMoveEvent_Ellipse(QMouseEvent*)
{
}


void Editor::mouseReleaseEvent_Ellipse(QMouseEvent*)
{
}


void Editor::mousePressEvent_FillEllipse(QMouseEvent *event)
{
	if (m_rubberBand.isValid())
	{
		QRect r = m_rubberBand;
		m_rubberBand = QRect(0, 0, 0, 0);
		repaint(r);
	}

	m_cellStart = m_cellTracking = m_cellEnd = contentsToCell(event->pos());
	QRect updateArea = cellToRect(m_cellStart);
	m_rubberBand = updateArea;
	repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
}


void Editor::mouseMoveEvent_FillEllipse(QMouseEvent *event)
{
	m_cellTracking = contentsToCell(event->pos());
	if (m_cellTracking != m_cellEnd)
	{
		m_cellEnd = m_cellTracking;
		QRect updateArea = (cellToRect(m_cellStart).united(cellToRect(m_cellEnd))).normalized();
		m_rubberBand = updateArea;
		repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
	}
}


void Editor::mouseReleaseEvent_FillEllipse(QMouseEvent*)
{
	QBitmap canvas(m_document->stitchData().width()*2, m_document->stitchData().height()*2);
	QRect rect;
	QPainter painter;

	QPainterPath path;
	QPoint s(m_cellStart.x()*2, m_cellStart.y()*2);
	QPoint e(m_cellEnd.x()*2, m_cellEnd.y()*2);
	path.addEllipse(QRect(s, e).normalized());
	canvas.fill(Qt::color0);
	painter.begin(&canvas);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setPen(QPen(Qt::color1));
	painter.setBrush(Qt::color1);
	painter.drawPath(path);
	painter.end();
	processBitmap(canvas, "Fill ellipse");

	m_rubberBand = QRect();
	m_selectionArea = QRect();

	update();
}


void Editor::mousePressEvent_FillPolyline(QMouseEvent*)
{
}


void Editor::mouseMoveEvent_FillPolyline(QMouseEvent*)
{
}


void Editor::mouseReleaseEvent_FillPolyline(QMouseEvent*)
{
}


void Editor::mousePressEvent_Text(QMouseEvent*)
{
}


void Editor::mouseMoveEvent_Text(QMouseEvent*)
{
}


void Editor::mouseReleaseEvent_Text(QMouseEvent*)
{
}


void Editor::mousePressEvent_Select(QMouseEvent *event)
{
	if (m_rubberBand.isValid())
	{
		QRect r = m_rubberBand;
		m_rubberBand = QRect(0, 0, 0, 0);
		repaint(r);
	}

	m_cellStart = m_cellTracking = m_cellEnd = contentsToCell(event->pos());
	QRect updateArea = cellToRect(m_cellStart);
	m_rubberBand = updateArea;
	repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
}


void Editor::mouseMoveEvent_Select(QMouseEvent *event)
{
	m_cellTracking = contentsToCell(event->pos());
	if (m_cellTracking != m_cellEnd)
	{
		m_cellEnd = m_cellTracking;
		QRect updateArea = (cellToRect(m_cellStart).united(cellToRect(m_cellEnd))).normalized();
		m_rubberBand = updateArea;
		repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
	}
}


void Editor::mouseReleaseEvent_Select(QMouseEvent*)
{
	m_selectionArea.setTopLeft(m_cellStart);
	m_selectionArea.setBottomRight(m_cellEnd);
	m_selectionArea = m_selectionArea.normalized();
	emit(selectionMade(true));
}


void Editor::mousePressEvent_Backstitch(QMouseEvent *e)
{
	m_cellStart = m_cellTracking = m_cellEnd = contentsToSnap(e->pos());
	m_rubberBand = QRect();
}


void Editor::mouseMoveEvent_Backstitch(QMouseEvent *e)
{
	QRect updateArea;
#if 0
	m_cellTracking = contentsToSnap(e->pos());
	updateArea = QRect(snapToContents(m_cellStart), snapToContents(m_cellEnd)).normalized();
	m_rubberBand = QRect();
	repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));

	m_cellEnd = m_cellTracking;
	updateArea = QRect(snapToContents(m_cellStart), snapToContents(m_cellEnd)).normalized();
	m_rubberBand = updateArea;
	repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
#endif
	updateArea = QRect(snapToContents(m_cellStart), snapToContents(m_cellEnd)).normalized();
	m_cellEnd = contentsToSnap(e->pos());
	m_rubberBand = QRect(snapToContents(m_cellStart), snapToContents(m_cellEnd)).normalized();
	updateArea.unite(m_rubberBand);
	repaint(updateArea);
//	repaint(updateArea.adjusted(-m_cellWidth, -m_cellHeight, m_cellWidth, m_cellHeight));
}


void Editor::mouseReleaseEvent_Backstitch(QMouseEvent *e)
{
	m_rubberBand = QRect();
	m_document->undoStack().push(new AddBackstitchCommand(m_document, m_document->layers().currentLayer(), m_cellStart, m_cellEnd, m_document->documentPalette().currentIndex()));
	update();
	m_preview->update();
}


QPoint Editor::contentsToCell(const QPoint &p) const
{
	return QPoint(p.x()/m_cellWidth, p.y()/m_cellHeight);
}


int Editor::contentsToZone(const QPoint &p) const
{
	QPoint cell(p.x()*2/m_cellWidth, p.y()*2/m_cellHeight);
	int zone = (cell.y()%2)*2 + (cell.x()%2);
	return zone;
}


QPoint Editor::contentsToSnap(const QPoint &p) const
{
	int w = m_document->stitchData().width()*2;
	int h = m_document->stitchData().height()*2;

	int x = (int)(round((double)(w*p.x())/width()));
	int y = (int)(round((double)(h*p.y())/height()));

	return QPoint(x, y);
}


QPoint Editor::snapToContents(const QPoint &p) const
{
	int x = m_cellWidth*p.x()/2;
	int y = m_cellHeight*p.y()/2;

	return QPoint(x, y);
}


QRect Editor::cellToRect(QPoint cell)
{
	int x = cell.x()*m_cellWidth;
	int y = cell.y()*m_cellHeight;
	return QRect(x, y, m_cellWidth, m_cellHeight);
}


void Editor::processBitmap(QBitmap &canvas, const QString &string)
{
	QImage image;
	image = canvas.toImage();
	m_document->undoStack().beginMacro(string);
	m_document->undoStack().push(new UpdateEditorCommand(this));
	m_document->undoStack().push(new UpdatePreviewCommand(m_preview));
	for (int y = 0 ; y < image.height() ; y++)
	{
		for (int x = 0 ; x < image.width() ; x++)
		{
			if (image.pixelIndex(x, y) == 1)
			{
				QPoint cell(x, y);
				m_document->undoStack().push(new AddStitchCommand(m_document, m_document->layers().currentLayer(), cell, Stitch::Full, m_document->documentPalette().currentIndex()));
			}
		}
	}
	m_document->undoStack().endMacro();
}


QRect Editor::selectionArea()
{
	return m_selectionArea;
}


QRect Editor::visibleCells()
{
	QRect cells;

	int left = -pos().x();
	int top = -pos().y();
	QSize viewportSize = dynamic_cast<QScrollArea *>(parent()->parent())->viewport()->size();
	int viewportWidth = viewportSize.width();
	int viewportHeight = viewportSize.height();
	int documentWidth = m_document->stitchData().width();
	int documentHeight = m_document->stitchData().height();

	QPoint topLeft = contentsToCell(QPoint(left, top));
	QPoint bottomRight = contentsToCell(QPoint(left+viewportWidth, top+viewportHeight));
	cells.setCoords(topLeft.x(), topLeft.y(), std::min(bottomRight.x(), documentWidth), std::min(bottomRight.y(), documentHeight));

	return cells;
}