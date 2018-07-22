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
#include "packageeditorstate_addpads.h"
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/graphics/graphicsscene.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/footprintgraphicsitem.h>
#include <librepcb/library/pkg/footprintpadgraphicsitem.h>
#include <librepcb/library/pkg/cmd/cmdfootprintpadedit.h>
#include "../packageeditorwidget.h"
#include "../widgets/packagepadcombobox.h"
#include "../widgets/boardsideselectorwidget.h"
#include "../widgets/footprintpadshapeselectorwidget.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PackageEditorState_AddPads::PackageEditorState_AddPads(Context& context, PadType type) noexcept :
    PackageEditorState(context), mPadType(type), mCurrentPad(nullptr),
    mCurrentGraphicsItem(nullptr), mPackagePadComboBox(nullptr),
    mLastPad(Uuid::createRandom(), Point(0, 0), Angle::deg0(), FootprintPad::Shape::ROUND,
             Length(2540000), Length(1270000), Length(800000),
             FootprintPad::BoardSide::THT)
{
    if (mPadType == PadType::SMT) {
        mLastPad.setBoardSide(FootprintPad::BoardSide::TOP);
        mLastPad.setShape(FootprintPad::Shape::RECT);
        mLastPad.setDrillDiameter(Length(0));
        mLastPad.setWidth(Length(1270000));
        mLastPad.setHeight(Length(635000));
    }
}

PackageEditorState_AddPads::~PackageEditorState_AddPads() noexcept
{
    Q_ASSERT(mEditCmd.isNull());
    Q_ASSERT(mCurrentPad == nullptr);
    Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool PackageEditorState_AddPads::entry() noexcept
{
    mContext.graphicsScene.setSelectionArea(QPainterPath()); // clear selection
    mContext.graphicsView.setCursor(Qt::CrossCursor);

    // populate command toolbar

    // package pad
    mContext.commandToolBar.addLabel(tr("Package Pad:"));
    mPackagePadComboBox = new PackagePadComboBox();
    std::unique_ptr<PackagePadComboBox> packagePadComboBox(mPackagePadComboBox);
    connect(packagePadComboBox.get(), &PackagePadComboBox::currentPadChanged,
            this, &PackageEditorState_AddPads::packagePadComboBoxCurrentPadChanged);
    packagePadComboBox->setPackage(&mContext.package, mContext.currentFootprint.get());
    mContext.commandToolBar.addWidget(std::move(packagePadComboBox));
    mContext.commandToolBar.addSeparator();

    // board side
    if (mPadType == PadType::SMT) {
        std::unique_ptr<BoardSideSelectorWidget> boardSideSelector(new BoardSideSelectorWidget());
        boardSideSelector->setCurrentBoardSide(mLastPad.getBoardSide());
        connect(boardSideSelector.get(), &BoardSideSelectorWidget::currentBoardSideChanged,
                this, &PackageEditorState_AddPads::boardSideSelectorCurrentSideChanged);
        mContext.commandToolBar.addWidget(std::move(boardSideSelector));
        mContext.commandToolBar.addSeparator();
    }

    // shape
    std::unique_ptr<FootprintPadShapeSelectorWidget> shapeSelector(new FootprintPadShapeSelectorWidget());
    connect(shapeSelector.get(), &FootprintPadShapeSelectorWidget::currentShapeChanged,
            this, &PackageEditorState_AddPads::shapeSelectorCurrentShapeChanged);
    shapeSelector->setCurrentShape(mLastPad.getShape());
    mContext.commandToolBar.addWidget(std::move(shapeSelector));
    mContext.commandToolBar.addSeparator();

    // width
    mContext.commandToolBar.addLabel(tr("Width:"), 10);
    std::unique_ptr<QDoubleSpinBox> widthSpinBox(new QDoubleSpinBox());
    widthSpinBox->setMinimum(0);
    widthSpinBox->setMaximum(999);
    widthSpinBox->setSingleStep(0.1);
    widthSpinBox->setDecimals(6);
    widthSpinBox->setValue(mLastPad.getWidth().toMm());
    connect(widthSpinBox.get(),
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &PackageEditorState_AddPads::widthSpinBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(widthSpinBox));

    // height
    mContext.commandToolBar.addLabel(tr("Height:"), 10);
    std::unique_ptr<QDoubleSpinBox> heightSpinBox(new QDoubleSpinBox());
    heightSpinBox->setMinimum(0);
    heightSpinBox->setMaximum(999);
    heightSpinBox->setSingleStep(0.1);
    heightSpinBox->setDecimals(6);
    heightSpinBox->setValue(mLastPad.getHeight().toMm());
    connect(heightSpinBox.get(),
            static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &PackageEditorState_AddPads::heightSpinBoxValueChanged);
    mContext.commandToolBar.addWidget(std::move(heightSpinBox));

    // drill diameter
    if (mPadType == PadType::THT) {
        mContext.commandToolBar.addLabel(tr("Drill Diameter:"), 10);
        std::unique_ptr<QDoubleSpinBox> drillDiameterSpinBox(new QDoubleSpinBox());
        drillDiameterSpinBox->setMinimum(0);
        drillDiameterSpinBox->setMaximum(100);
        drillDiameterSpinBox->setSingleStep(0.2);
        drillDiameterSpinBox->setDecimals(6);
        drillDiameterSpinBox->setValue(mLastPad.getDrillDiameter().toMm());
        connect(drillDiameterSpinBox.get(),
                static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                this, &PackageEditorState_AddPads::drillDiameterSpinBoxValueChanged);
        mContext.commandToolBar.addWidget(std::move(drillDiameterSpinBox));
    }

    Point pos = mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
    return startAddPad(pos);
}

bool PackageEditorState_AddPads::exit() noexcept
{
    if (mCurrentPad && !abortAddPad()) {
        return false;
    }

    // cleanup command toolbar
    mPackagePadComboBox = nullptr;
    mContext.commandToolBar.clear();

    mContext.graphicsView.setCursor(Qt::ArrowCursor);
    return true;
}

/*****************************************************************************************
 *  Event Handlers
 ****************************************************************************************/

bool PackageEditorState_AddPads::processGraphicsSceneMouseMoved(QGraphicsSceneMouseEvent& e) noexcept
{
    if (mCurrentPad) {
        Point currentPos = Point::fromPx(e.scenePos(), getGridInterval());
        mEditCmd->setPosition(currentPos, true);
        return true;
    } else {
        return false;
    }
}

bool PackageEditorState_AddPads::processGraphicsSceneLeftMouseButtonPressed(QGraphicsSceneMouseEvent& e) noexcept
{
    Point currentPos = Point::fromPx(e.scenePos(), getGridInterval());
    if (mCurrentPad) {
        finishAddPad(currentPos);
    }
    return startAddPad(currentPos);
}

bool PackageEditorState_AddPads::processGraphicsSceneRightMouseButtonReleased(QGraphicsSceneMouseEvent &e) noexcept
{
    Q_UNUSED(e);
    return processRotateCcw();
}

bool PackageEditorState_AddPads::processRotateCw() noexcept
{
    if (mCurrentPad) {
        mEditCmd->rotate(-Angle::deg90(), mCurrentPad->getPosition(), true);
        return true;
    } else {
        return false;
    }
}

bool PackageEditorState_AddPads::processRotateCcw() noexcept
{
    if (mCurrentPad) {
        mEditCmd->rotate(Angle::deg90(), mCurrentPad->getPosition(), true);
        return true;
    } else {
        return false;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool PackageEditorState_AddPads::startAddPad(const Point& pos) noexcept
{
    try {
        mStartPos = pos;
        mContext.undoStack.beginCmdGroup(tr("Add footprint pad"));
        mCurrentPad.reset(new FootprintPad(mLastPad.getPackagePadUuid(), pos,
                                           mLastPad.getRotation(), mLastPad.getShape(),
                                           mLastPad.getWidth(), mLastPad.getHeight(),
                                           mLastPad.getDrillDiameter(),
                                           mLastPad.getBoardSide()));
        mContext.undoStack.appendToCmdGroup(new CmdFootprintPadInsert(
            mContext.currentFootprint->getPads(), mCurrentPad));
        mEditCmd.reset(new CmdFootprintPadEdit(*mCurrentPad));
        mCurrentGraphicsItem = mContext.currentGraphicsItem->getPadGraphicsItem(*mCurrentPad);
        Q_ASSERT(mCurrentGraphicsItem);
        mCurrentGraphicsItem->setSelected(true);
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        mCurrentGraphicsItem = nullptr;
        mCurrentPad = nullptr;
        mEditCmd.reset();
        return false;
    }
}

bool PackageEditorState_AddPads::finishAddPad(const Point& pos) noexcept
{
    if (pos == mStartPos) {
        return abortAddPad();
    }

    try {
        mEditCmd->setPosition(pos, true);
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mLastPad = *mCurrentPad;
        mCurrentPad.reset();
        mContext.undoStack.appendToCmdGroup(mEditCmd.take());
        mContext.undoStack.commitCmdGroup();
        mPackagePadComboBox->updatePads();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

bool PackageEditorState_AddPads::abortAddPad() noexcept
{
    try {
        mCurrentGraphicsItem->setSelected(false);
        mCurrentGraphicsItem = nullptr;
        mLastPad = *mCurrentPad;
        mCurrentPad.reset();
        mEditCmd.reset();
        mContext.undoStack.abortCmdGroup();
        return true;
    } catch (const Exception& e) {
        QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
        return false;
    }
}

void PackageEditorState_AddPads::packagePadComboBoxCurrentPadChanged(PackagePad* pad) noexcept
{
    if (pad) {
        mLastPad.setPackagePadUuid(pad->getUuid());
        if (mEditCmd) {
            mEditCmd->setPackagePadUuid(mLastPad.getPackagePadUuid(), true);
        }
    }
}

void PackageEditorState_AddPads::boardSideSelectorCurrentSideChanged(FootprintPad::BoardSide side) noexcept
{
    mLastPad.setBoardSide(side);
    if (mEditCmd) {
        mEditCmd->setBoardSide(side, true);
    }
}

void PackageEditorState_AddPads::shapeSelectorCurrentShapeChanged(FootprintPad::Shape shape) noexcept
{
    mLastPad.setShape(shape);
    if (mEditCmd) {
        mEditCmd->setShape(shape, true);
    }
}

void PackageEditorState_AddPads::widthSpinBoxValueChanged(double value) noexcept
{
    mLastPad.setWidth(Length::fromMm(value));
    if (mEditCmd) {
        mEditCmd->setWidth(mLastPad.getWidth(), true);
    }
}

void PackageEditorState_AddPads::heightSpinBoxValueChanged(double value) noexcept
{
    mLastPad.setHeight(Length::fromMm(value));
    if (mEditCmd) {
        mEditCmd->setHeight(mLastPad.getHeight(), true);
    }
}

void PackageEditorState_AddPads::drillDiameterSpinBoxValueChanged(double value) noexcept
{
    mLastPad.setDrillDiameter(Length::fromMm(value));
    if (mEditCmd) {
        mEditCmd->setDrillDiameter(mLastPad.getDrillDiameter(), true);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
