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
#include "symboleditorstate_drawcircle.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/geometry/circle.h>
#include <librepcb/common/geometry/cmd/cmdcircleedit.h>
#include <librepcb/common/widgets/graphicslayercombobox.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/sym/symbolgraphicsitem.h>
#include <librepcb/common/graphics/circlegraphicsitem.h>
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

SymbolEditorState_DrawCircle::SymbolEditorState_DrawCircle(const Context& context) noexcept :
    SymbolEditorState(context), mCurrentCircle(nullptr), mCurrentGraphicsItem(nullptr),
    mLastLayerName(GraphicsLayer::sSymbolOutlines), mLastLineWidth(250000), mLastFill(false),
    mLastGrabArea(true)
{
}

SymbolEditorState_DrawCircle::~SymbolEditorState_DrawCircle() noexcept
{
    Q_ASSERT(mEditCmd.isNull());
    Q_ASSERT(mCurrentCircle == nullptr);
    Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool SymbolEditorState_DrawCircle::entry() noexcept
{
    mContext.graphicsScene.setSelectionArea(QPainterPath()); // clear selection
    mContext.graphicsView.setCursor(Qt::CrossCursor);

    // populate command toolbar
    mContext.commandToolBar.addLabel(tr("Layer:"));
    std::unique_ptr<GraphicsLayerComboBox> layerComboBox(new GraphicsLayerComboBox());
    layerComboBox->setLayers(mContext.layerProvider.getSchematicGeometryElementLayers());
    layerComboBox->setCurrentLayer(*mLastLayerName);
    connect(layerComboBox.get(), &GraphicsLayerComboBox::currentLayerChanged,
            this, &SymbolEditorState_DrawCircle::layerComboBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(layerComboBox));

    mContext.commandToolBar.addLabel(tr("Line Width:"), 10);
    std::unique_ptr<QDoubleSpinBox> lineWidthSpinBox(new QDoubleSpinBox());
    lineWidthSpinBox->setMinimum(0);
    lineWidthSpinBox->setMaximum(100);
    lineWidthSpinBox->setSingleStep(0.1);
    lineWidthSpinBox->setDecimals(6);
    lineWidthSpinBox->setValue(mLastLineWidth->toMm());
    connect(lineWidthSpinBox.get(),
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &SymbolEditorState_DrawCircle::lineWidthSpinBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(lineWidthSpinBox));

    std::unique_ptr<QCheckBox> fillCheckBox(new QCheckBox(tr("Fill")));
    fillCheckBox->setChecked(mLastFill);
    connect(fillCheckBox.get(), &QCheckBox::toggled,
            this, &SymbolEditorState_DrawCircle::fillCheckBoxCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(fillCheckBox));

    std::unique_ptr<QCheckBox> grabAreaCheckBox(new QCheckBox(tr("Grab Area")));
    grabAreaCheckBox->setChecked(mLastGrabArea);
    connect(grabAreaCheckBox.get(), &QCheckBox::toggled,
            this, &SymbolEditorState_DrawCircle::grabAreaCheckBoxCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(grabAreaCheckBox));

    return true;
}

bool SymbolEditorState_DrawCircle::exit() noexcept
{
    if (mCurrentCircle && (!abortAddCircle())) {
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

bool SymbolEditorState_DrawCircle::processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept
{
    if (mCurrentCircle) {
        Point currentPos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
        return updateCircleDiameter(currentPos);
    } else {
        return true;
    }
}

bool SymbolEditorState_DrawCircle::processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept
{
    Point currentPos = Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    if (mCurrentCircle) {
        return finishAddCircle(currentPos);
    } else {
        return startAddCircle(currentPos);
    }
}

bool SymbolEditorState_DrawCircle::processAbortCommand() noexcept
{
    if (mCurrentCircle) {
        return abortAddCircle();
    } else {
        return false;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SymbolEditorState_DrawCircle::startAddCircle(const Point& pos) noexcept
{
    try {
        mContext.undoStack.beginCmdGroup(tr("Add symbol circle"));
        mCurrentCircle = new Circle(Uuid::createRandom(), mLastLayerName, mLastLineWidth,
                                    mLastFill, mLastGrabArea, pos, PositiveLength(1));
        mContext.undoStack.appendToCmdGroup(new CmdCircleInsert(
            mContext.symbol.getCircles(), std::shared_ptr<Circle>(mCurrentCircle)));
        mEditCmd.reset(new CmdCircleEdit(*mCurrentCircle));
        mCurrentGraphicsItem = mContext.symbolGraphicsItem.getCircleGraphicsItem(*mCurrentCircle);
        Q_ASSERT(mCurrentGraphicsItem);
        mCurrentGraphicsItem->setSelected(true);
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        mCurrentGraphicsItem = nullptr;
        mCurrentCircle = nullptr;
        mEditCmd.reset();
        return false;
    }
}

bool SymbolEditorState_DrawCircle::updateCircleDiameter(const Point& pos) noexcept
{
    Point delta = pos - mCurrentCircle->getCenter();
    Length diameter = delta.getLength() * 2;
    if (diameter < 1) {diameter = 1;} // diameter must be greater than zero!
    mEditCmd->setDiameter(PositiveLength(diameter), true);
    return true;
}

bool SymbolEditorState_DrawCircle::finishAddCircle(const Point& pos) noexcept
{
    if (pos == mCurrentCircle->getCenter()) {
        return abortAddCircle();
    }

    try {
        updateCircleDiameter(pos);
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mCurrentCircle = nullptr;
        mContext.undoStack.appendToCmdGroup(mEditCmd.take());
        mContext.undoStack.commitCmdGroup();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

bool SymbolEditorState_DrawCircle::abortAddCircle() noexcept
{
    try {
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mCurrentCircle = nullptr;
        mEditCmd.reset();
        mContext.undoStack.abortCmdGroup();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

void SymbolEditorState_DrawCircle::layerComboBoxValueChanged(const QString& layerName) noexcept
{
    if (layerName.isEmpty()) {
        return;
    }
    mLastLayerName = layerName;
    if (mEditCmd) {
        mEditCmd->setLayerName(mLastLayerName, true);
    }
}

void SymbolEditorState_DrawCircle::lineWidthSpinBoxValueChanged(double value) noexcept
{
    mLastLineWidth = Length::fromMm(value);
    if (mEditCmd) {
        mEditCmd->setLineWidth(mLastLineWidth, true);
    }
}

void SymbolEditorState_DrawCircle::fillCheckBoxCheckedChanged(bool checked) noexcept
{
    mLastFill = checked;
    if (mEditCmd) {
        mEditCmd->setIsFilled(mLastFill, true);
    }
}

void SymbolEditorState_DrawCircle::grabAreaCheckBoxCheckedChanged(bool checked) noexcept
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
