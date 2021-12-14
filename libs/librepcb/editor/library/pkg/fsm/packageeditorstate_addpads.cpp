/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate_addpads.h"

#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../../cmd/cmdfootprintpadedit.h"
#include "../boardsideselectorwidget.h"
#include "../footprintpadshapeselectorwidget.h"
#include "../packageeditorwidget.h"
#include "../packagepadcombobox.h"

#include <librepcb/core/graphics/graphicsscene.h>
#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/footprintgraphicsitem.h>
#include <librepcb/core/library/pkg/footprintpadgraphicsitem.h>
#include <librepcb/core/library/pkg/package.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_AddPads::PackageEditorState_AddPads(Context& context,
                                                       PadType type) noexcept
  : PackageEditorState(context),
    mPadType(type),
    mCurrentPad(nullptr),
    mCurrentGraphicsItem(nullptr),
    mPackagePadComboBox(nullptr),
    mLastPad(
        Uuid::createRandom(), Point(0, 0), Angle::deg0(),
        FootprintPad::Shape::ROUND,  // Commonly used pad shape
        PositiveLength(2500000),  // There is no default/recommended pad size
        PositiveLength(1300000),  // -> choose reasonable multiple of 0.1mm
        UnsignedLength(800000),  // Commonly used drill diameter
        FootprintPad::BoardSide::THT) {
  if (mPadType == PadType::SMT) {
    mLastPad.setBoardSide(FootprintPad::BoardSide::TOP);  // Default side
    mLastPad.setShape(FootprintPad::Shape::RECT);  // Commonly used SMT shape
    mLastPad.setDrillDiameter(UnsignedLength(0));  // Disable drill
    mLastPad.setWidth(PositiveLength(1500000));  // Same as for THT pads ->
    mLastPad.setHeight(PositiveLength(700000));  // reasonable multiple of 0.1mm
  }
}

PackageEditorState_AddPads::~PackageEditorState_AddPads() noexcept {
  Q_ASSERT(mEditCmd.isNull());
  Q_ASSERT(mCurrentPad == nullptr);
  Q_ASSERT(mCurrentGraphicsItem == nullptr);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_AddPads::entry() noexcept {
  mContext.graphicsScene.setSelectionArea(QPainterPath());  // clear selection
  mContext.graphicsView.setCursor(Qt::CrossCursor);

  // populate command toolbar

  // package pad
  mContext.commandToolBar.addLabel(tr("Package Pad:"));
  mPackagePadComboBox = new PackagePadComboBox();
  std::unique_ptr<PackagePadComboBox> packagePadComboBox(mPackagePadComboBox);
  connect(packagePadComboBox.get(), &PackagePadComboBox::currentPadChanged,
          this,
          &PackageEditorState_AddPads::packagePadComboBoxCurrentPadChanged);
  packagePadComboBox->setPackage(&mContext.package,
                                 mContext.currentFootprint.get());
  mContext.commandToolBar.addWidget(std::move(packagePadComboBox));
  mContext.commandToolBar.addSeparator();

  // board side
  if (mPadType == PadType::SMT) {
    std::unique_ptr<BoardSideSelectorWidget> boardSideSelector(
        new BoardSideSelectorWidget());
    boardSideSelector->setCurrentBoardSide(mLastPad.getBoardSide());
    connect(boardSideSelector.get(),
            &BoardSideSelectorWidget::currentBoardSideChanged, this,
            &PackageEditorState_AddPads::boardSideSelectorCurrentSideChanged);
    mContext.commandToolBar.addWidget(std::move(boardSideSelector));
    mContext.commandToolBar.addSeparator();
  }

  // shape
  std::unique_ptr<FootprintPadShapeSelectorWidget> shapeSelector(
      new FootprintPadShapeSelectorWidget());
  connect(shapeSelector.get(),
          &FootprintPadShapeSelectorWidget::currentShapeChanged, this,
          &PackageEditorState_AddPads::shapeSelectorCurrentShapeChanged);
  shapeSelector->setCurrentShape(mLastPad.getShape());
  mContext.commandToolBar.addWidget(std::move(shapeSelector));
  mContext.commandToolBar.addSeparator();

  // width
  mContext.commandToolBar.addLabel(tr("Width:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtWidth(new PositiveLengthEdit());
  edtWidth->configure(getDefaultLengthUnit(), LengthEditBase::Steps::generic(),
                      "package_editor/add_pads/width");
  edtWidth->setValue(mLastPad.getWidth());
  connect(edtWidth.get(), &PositiveLengthEdit::valueChanged, this,
          &PackageEditorState_AddPads::widthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtWidth));

  // height
  mContext.commandToolBar.addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
  edtHeight->configure(getDefaultLengthUnit(), LengthEditBase::Steps::generic(),
                       "package_editor/add_pads/height");
  edtHeight->setValue(mLastPad.getHeight());
  connect(edtHeight.get(), &PositiveLengthEdit::valueChanged, this,
          &PackageEditorState_AddPads::heightEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtHeight));

  // drill diameter
  if (mPadType == PadType::THT) {
    mContext.commandToolBar.addLabel(tr("Drill Diameter:"), 10);
    std::unique_ptr<UnsignedLengthEdit> edtDrillDiameter(
        new UnsignedLengthEdit());
    edtDrillDiameter->configure(getDefaultLengthUnit(),
                                LengthEditBase::Steps::drillDiameter(),
                                "package_editor/add_pads/drill_diameter");
    edtDrillDiameter->setValue(mLastPad.getDrillDiameter());
    connect(edtDrillDiameter.get(), &UnsignedLengthEdit::valueChanged, this,
            &PackageEditorState_AddPads::drillDiameterEditValueChanged);
    mContext.commandToolBar.addWidget(std::move(edtDrillDiameter));
  }

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  return startAddPad(pos);
}

bool PackageEditorState_AddPads::exit() noexcept {
  if (mCurrentPad && !abortAddPad()) {
    return false;
  }

  // cleanup command toolbar
  mPackagePadComboBox = nullptr;
  mContext.commandToolBar.clear();

  mContext.graphicsView.setCursor(Qt::ArrowCursor);
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_AddPads::processGraphicsSceneMouseMoved(
    QGraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentPad) {
    Point currentPos =
        Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
    mEditCmd->setPosition(currentPos, true);
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_AddPads::processGraphicsSceneLeftMouseButtonPressed(
    QGraphicsSceneMouseEvent& e) noexcept {
  Point currentPos =
      Point::fromPx(e.scenePos()).mappedToGrid(getGridInterval());
  if (mCurrentPad) {
    if (!finishAddPad(currentPos)) {
      return false;
    }
  }
  return startAddPad(currentPos);
}

bool PackageEditorState_AddPads::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotateCcw();
}

bool PackageEditorState_AddPads::processRotateCw() noexcept {
  if (mCurrentPad) {
    mEditCmd->rotate(-Angle::deg90(), mCurrentPad->getPosition(), true);
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_AddPads::processRotateCcw() noexcept {
  if (mCurrentPad) {
    mEditCmd->rotate(Angle::deg90(), mCurrentPad->getPosition(), true);
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_AddPads::startAddPad(const Point& pos) noexcept {
  try {
    mContext.undoStack.beginCmdGroup(tr("Add footprint pad"));
    mCurrentPad.reset(new FootprintPad(
        mLastPad.getPackagePadUuid(), pos, mLastPad.getRotation(),
        mLastPad.getShape(), mLastPad.getWidth(), mLastPad.getHeight(),
        mLastPad.getDrillDiameter(), mLastPad.getBoardSide()));
    mContext.undoStack.appendToCmdGroup(new CmdFootprintPadInsert(
        mContext.currentFootprint->getPads(), mCurrentPad));
    mEditCmd.reset(new CmdFootprintPadEdit(*mCurrentPad));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getPadGraphicsItem(*mCurrentPad);
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

bool PackageEditorState_AddPads::finishAddPad(const Point& pos) noexcept {
  try {
    // In LibrePCB 0.1.x, it's not allowed to add unconnected pads or multiple
    // footprint pads connected to the same package pad!
    QString note = tr("Note that each pad can only be added once.");
    auto pkgPad =
        mContext.package.getPads().find(mCurrentPad->getPackagePadUuid());
    if (!pkgPad) {
      throw LogicError(__FILE__, __LINE__,
                       tr("No package pad selected.") % " " % note);
    }
    for (const FootprintPad& pad : mContext.currentFootprint->getPads()) {
      if ((pad.getPackagePadUuid() == pkgPad->getUuid()) &&
          (&pad != mCurrentPad.get())) {
        throw RuntimeError(__FILE__, __LINE__,
                           tr("The pad \"%1\" has been added already.")
                                   .arg(*pkgPad->getName()) %
                               " " % note);
      }
    }

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

bool PackageEditorState_AddPads::abortAddPad() noexcept {
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

void PackageEditorState_AddPads::packagePadComboBoxCurrentPadChanged(
    PackagePad* pad) noexcept {
  if (pad) {
    mLastPad.setPackagePadUuid(pad->getUuid());
    if (mEditCmd) {
      mEditCmd->setPackagePadUuid(mLastPad.getPackagePadUuid(), true);
    }
  }
}

void PackageEditorState_AddPads::boardSideSelectorCurrentSideChanged(
    FootprintPad::BoardSide side) noexcept {
  mLastPad.setBoardSide(side);
  if (mEditCmd) {
    mEditCmd->setBoardSide(side, true);
  }
}

void PackageEditorState_AddPads::shapeSelectorCurrentShapeChanged(
    FootprintPad::Shape shape) noexcept {
  mLastPad.setShape(shape);
  if (mEditCmd) {
    mEditCmd->setShape(shape, true);
  }
}

void PackageEditorState_AddPads::widthEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastPad.setWidth(value);
  if (mEditCmd) {
    mEditCmd->setWidth(mLastPad.getWidth(), true);
  }
}

void PackageEditorState_AddPads::heightEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastPad.setHeight(value);
  if (mEditCmd) {
    mEditCmd->setHeight(mLastPad.getHeight(), true);
  }
}

void PackageEditorState_AddPads::drillDiameterEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastPad.setDrillDiameter(value);
  if (mEditCmd) {
    mEditCmd->setDrillDiameter(mLastPad.getDrillDiameter(), true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
