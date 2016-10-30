/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "symboleditorstate_drawpolygonbase.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/geometry/cmd/cmdpolygonedit.h>
#include <librepcb/common/geometry/cmd/cmdpolygonsegmentedit.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>
#include <librepcb/common/graphics/polygongraphicsitem.h>
#include "../symboleditorwidget.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SymbolEditorState_DrawPolygonBase::SymbolEditorState_DrawPolygonBase(const Context& context, Mode mode) noexcept :
    SymbolEditorState(context), mMode(mode), mCurrentPolygon(nullptr),
    mCurrentGraphicsItem(nullptr), mLastLayerName(GraphicsLayer::sSymbolOutlines),
    mLastLineWidth(250000), mLastAngle(0), mLastFill(false),
    mLastGrabArea(mode != Mode::LINE)
{
}

SymbolEditorState_DrawPolygonBase::~SymbolEditorState_DrawPolygonBase() noexcept
{
    Q_ASSERT(mSegmentEditCmds.empty());
    Q_ASSERT(mEditCmd.isNull());
    Q_ASSERT(mCurrentPolygon == nullptr);
    Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool SymbolEditorState_DrawPolygonBase::entry() noexcept
{
    mContext.graphicsScene.setSelectionArea(QPainterPath()); // clear selection
    mContext.graphicsView.setCursor(Qt::CrossCursor);

    // populate command toolbar
    mContext.commandToolBar.addLabel(tr("Layer:"));
    std::unique_ptr<GraphicsLayerComboBox> layerComboBox(new GraphicsLayerComboBox());
    layerComboBox->setLayers(mContext.layerProvider.getSchematicGeometryElementLayers());
    layerComboBox->setCurrentLayer(mLastLayerName);
    connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
            this, &SymbolEditorState_DrawPolygonBase::layerComboBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(layerComboBox));

    mContext.commandToolBar.addLabel(tr("Line Width:"), 10);
    std::unique_ptr<QDoubleSpinBox> lineWidthSpinBox(new QDoubleSpinBox());
    lineWidthSpinBox->setMinimum(0);
    lineWidthSpinBox->setMaximum(100);
    lineWidthSpinBox->setSingleStep(0.1);
    lineWidthSpinBox->setDecimals(6);
    lineWidthSpinBox->setValue(mLastLineWidth.toMm());
    connect(lineWidthSpinBox.get(),
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &SymbolEditorState_DrawPolygonBase::lineWidthSpinBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(lineWidthSpinBox));

    if (mMode != Mode::RECT) {
        mContext.commandToolBar.addLabel(tr("Angle:"), 10);
        std::unique_ptr<QDoubleSpinBox> angleSpinBox(new QDoubleSpinBox());
        angleSpinBox->setMinimum(-360);
        angleSpinBox->setMaximum(360);
        angleSpinBox->setSingleStep(30);
        angleSpinBox->setDecimals(6);
        angleSpinBox->setValue(mLastAngle.toDeg());
        connect(angleSpinBox.get(),
                static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this, &SymbolEditorState_DrawPolygonBase::angleSpinBoxValueChanged);
        mContext.commandToolBar.addWidget(std::move(angleSpinBox));
    }

    if (mMode != Mode::LINE) {
        std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
        fillCheckBox->setChecked(mLastFill);
        connect(fillCheckBox.get(), &QCheckBox::toggled,
                this, &SymbolEditorState_DrawPolygonBase::fillCheckBoxCheckedChanged);
        mContext.commandToolBar.addWidget(std::move(fillCheckBox));
    }

    if (mMode != Mode::LINE) {
        std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
        grabAreaCheckBox->setChecked(mLastGrabArea);
        connect(grabAreaCheckBox.get(), &QCheckBox::toggled,
                this, &SymbolEditorState_DrawPolygonBase::grabAreaCheckBoxCheckedChanged);
        mContext.commandToolBar.addWidget(std::move(grabAreaCheckBox));
    }

    return true;
}

bool SymbolEditorState_DrawPolygonBase::exit() noexcept
{
    if (mCurrentPolygon && (!abort())) {
        return false;
    }

    // cleanup command toolbar
    mContext.commandToolBar.clear();

    mContext.graphicsView.setCursor(Qt::ArrowCursor);
    return true;
}

/*****************************************************************************************
 *  Event Handlers
 ****************************************************************************************/

bool SymbolEditorState_DrawPolygonBase::processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept
{
    if (mCurrentPolygon) {
        Point currentPos = Point::fromPx(e.scenePos(), getGridInterval());
        return updateCurrentPosition(currentPos);
    } else {
        return true;
    }
}

bool SymbolEditorState_DrawPolygonBase::processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept
{
    Point currentPos = Point::fromPx(e.scenePos(), getGridInterval());
    if (mCurrentPolygon) {
        if (currentPos == mSegmentStartPos) {
            return abort();
        } else if ((currentPos == mCurrentPolygon->getStartPos()) || (mMode == Mode::RECT)) {
            return addNextSegment(currentPos) && abort();
        } else {
            return addNextSegment(currentPos);
        }
    } else {
        return start(currentPos);
    }
}

bool SymbolEditorState_DrawPolygonBase::processGraphicsSceneLeftMouseButtonDoubleClicked(QGraphicsSceneMouseEvent& e) noexcept
{
    return processGraphicsSceneLeftMouseButtonPressed(e); // handle like single click
}

bool SymbolEditorState_DrawPolygonBase::processAbortCommand() noexcept
{
    if (mCurrentPolygon) {
        return abort();
    } else {
        return false;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolEditorState_DrawPolygonBase::start(const Point& pos) noexcept
{
    try {
        // add polygon
        mContext.undoStack.beginCmdGroup(tr("Add symbol polygon"));
        mCurrentPolygon.reset(new Polygon(mLastLayerName, mLastLineWidth, mLastFill,
                                          mLastGrabArea, pos));
        mContext.undoStack.appendToCmdGroup(
            new CmdPolygonInsert(mContext.symbol.getPolygons(), mCurrentPolygon));
        mEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
        mCurrentGraphicsItem = mContext.symbolGraphicsItem.getPolygonGraphicsItem(*mCurrentPolygon);
        Q_ASSERT(mCurrentGraphicsItem);
        mCurrentGraphicsItem->setSelected(true);

        // add segment(s)
        mSegmentStartPos = pos;
        int count = (mMode == Mode::RECT) ? 4 : 1;
        for (int i = 0; i < count; ++i) {
            std::shared_ptr<PolygonSegment> segment(new PolygonSegment(pos, mLastAngle));
            mContext.undoStack.appendToCmdGroup(
                new CmdPolygonSegmentInsert(mCurrentPolygon->getSegments(), segment));
            mSegmentEditCmds.emplace_back(new CmdPolygonSegmentEdit(*segment));
        }
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        mCurrentGraphicsItem = nullptr;
        mSegmentEditCmds.clear();
        mEditCmd.reset();
        mCurrentPolygon.reset();
        return false;
    }
}

bool SymbolEditorState_DrawPolygonBase::abort() noexcept
{
    try {
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mSegmentEditCmds.clear();
        mEditCmd.reset();
        mCurrentPolygon.reset();
        mContext.undoStack.abortCmdGroup();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

bool SymbolEditorState_DrawPolygonBase::addNextSegment(const Point& pos) noexcept
{
    try {
        // commit current
        updateCurrentPosition(pos);
        mContext.undoStack.appendToCmdGroup(mEditCmd.take());
        for (std::unique_ptr<CmdPolygonSegmentEdit>& cmd : mSegmentEditCmds) {
            mContext.undoStack.appendToCmdGroup(cmd.release());
        }
        mSegmentEditCmds.clear();
        mContext.undoStack.commitCmdGroup();

        // add next
        mSegmentStartPos = pos;
        mContext.undoStack.beginCmdGroup(tr("Add symbol polygon"));
        mEditCmd.reset(new CmdPolygonEdit(*mCurrentPolygon));
        std::shared_ptr<PolygonSegment> segment(new PolygonSegment(pos, mLastAngle));
        mContext.undoStack.appendToCmdGroup(
            new CmdPolygonSegmentInsert(mCurrentPolygon->getSegments(), segment));
        mSegmentEditCmds.emplace_back(
            new CmdPolygonSegmentEdit(*segment));
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        mSegmentEditCmds.clear();
        return false;
    }
}

bool SymbolEditorState_DrawPolygonBase::updateCurrentPosition(const Point& pos) noexcept
{
    if (mMode == Mode::RECT) {
        Q_ASSERT(mSegmentEditCmds.size() == 4);
        Point start = mCurrentPolygon->getStartPos();
        mSegmentEditCmds[0]->setEndPos(Point(pos.getX(), start.getY()), true);
        mSegmentEditCmds[1]->setEndPos(pos, true);
        mSegmentEditCmds[2]->setEndPos(Point(start.getX(), pos.getY()), true);
    } else {
        Q_ASSERT(mSegmentEditCmds.size() == 1);
        mSegmentEditCmds.front()->setEndPos(pos, true);
    }
    return true;
}

void SymbolEditorState_DrawPolygonBase::layerComboBoxValueChanged(const QString& layerName) noexcept
{
    if (layerName.isEmpty()) {
        return;
    }
    mLastLayerName = layerName;
    if (mEditCmd) {
        mEditCmd->setLayerName(mLastLayerName, true);
    }
}

void SymbolEditorState_DrawPolygonBase::lineWidthSpinBoxValueChanged(double value) noexcept
{
    mLastLineWidth = Length::fromMm(value);
    if (mEditCmd) {
        mEditCmd->setLineWidth(mLastLineWidth, true);
    }
}

void SymbolEditorState_DrawPolygonBase::angleSpinBoxValueChanged(double value) noexcept
{
    mLastAngle = Angle::fromDeg(value);
    if (!mSegmentEditCmds.empty()) {
        mSegmentEditCmds.back()->setAngle(mLastAngle, true);
    }
}

void SymbolEditorState_DrawPolygonBase::fillCheckBoxCheckedChanged(bool checked) noexcept
{
    mLastFill = checked;
    if (mEditCmd) {
        mEditCmd->setIsFilled(mLastFill, true);
    }
}

void SymbolEditorState_DrawPolygonBase::grabAreaCheckBoxCheckedChanged(bool checked) noexcept
{
    mLastGrabArea = checked;
    if (mEditCmd) {
        mEditCmd->setIsGrabArea(mLastGrabArea, true);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
