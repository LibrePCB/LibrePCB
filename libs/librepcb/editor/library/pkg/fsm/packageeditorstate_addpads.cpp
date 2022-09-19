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

#include "../../../editorcommandset.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../../cmd/cmdfootprintpadedit.h"
#include "../boardsideselectorwidget.h"
#include "../footprintgraphicsitem.h"
#include "../footprintpadgraphicsitem.h"
#include "../packageeditorwidget.h"
#include "../packagepadcombobox.h"

#include <librepcb/core/library/pkg/footprint.h>
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
        Uuid::createRandom(), tl::nullopt, Point(0, 0), Angle::deg0(),
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
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_AddPads::entry() noexcept {
  // populate command toolbar
  EditorCommandSet& cmd = EditorCommandSet::instance();

  // package pad
  mContext.commandToolBar.addLabel(tr("Package Pad:"));
  mPackagePadComboBox = new PackagePadComboBox();
  std::unique_ptr<PackagePadComboBox> packagePadComboBox(mPackagePadComboBox);
  connect(packagePadComboBox.get(), &PackagePadComboBox::currentPadChanged,
          this,
          &PackageEditorState_AddPads::packagePadComboBoxCurrentPadChanged);
  packagePadComboBox->setPads(mContext.package.getPads());
  mContext.commandToolBar.addWidget(std::move(packagePadComboBox));
  mContext.commandToolBar.addSeparator();
  selectNextFreePadInComboBox();

  // board side
  if (mPadType == PadType::SMT) {
    std::unique_ptr<BoardSideSelectorWidget> boardSideSelector(
        new BoardSideSelectorWidget());
    boardSideSelector->setCurrentBoardSide(mLastPad.getBoardSide());
    boardSideSelector->addAction(cmd.layerUp.createAction(
        boardSideSelector.get(), boardSideSelector.get(),
        &BoardSideSelectorWidget::setBoardSideTop));
    boardSideSelector->addAction(cmd.layerDown.createAction(
        boardSideSelector.get(), boardSideSelector.get(),
        &BoardSideSelectorWidget::setBoardSideBottom));
    connect(boardSideSelector.get(),
            &BoardSideSelectorWidget::currentBoardSideChanged, this,
            &PackageEditorState_AddPads::boardSideSelectorCurrentSideChanged);
    mContext.commandToolBar.addWidget(std::move(boardSideSelector));
    mContext.commandToolBar.addSeparator();
  }

  // Shape.
  std::unique_ptr<QActionGroup> shapeActionGroup(
      new QActionGroup(&mContext.commandToolBar));
  QAction* aShapeRound =
      cmd.thtShapeRound.createAction(shapeActionGroup.get(), this, [this]() {
        shapeSelectorCurrentShapeChanged(FootprintPad::Shape::ROUND);
      });
  aShapeRound->setIcon(QIcon(":/img/command_toolbars/shape_round.png"));
  aShapeRound->setCheckable(true);
  aShapeRound->setChecked(mLastPad.getShape() == FootprintPad::Shape::ROUND);
  aShapeRound->setActionGroup(shapeActionGroup.get());
  QAction* aShapeRect = cmd.thtShapeRectangular.createAction(
      shapeActionGroup.get(), this, [this]() {
        shapeSelectorCurrentShapeChanged(FootprintPad::Shape::RECT);
      });
  aShapeRect->setIcon(QIcon(":/img/command_toolbars/shape_rect.png"));
  aShapeRect->setCheckable(true);
  aShapeRect->setChecked(mLastPad.getShape() == FootprintPad::Shape::RECT);
  aShapeRect->setActionGroup(shapeActionGroup.get());
  QAction* aShapeOctagon = cmd.thtShapeOctagonal.createAction(
      shapeActionGroup.get(), this, [this]() {
        shapeSelectorCurrentShapeChanged(FootprintPad::Shape::OCTAGON);
      });
  aShapeOctagon->setIcon(QIcon(":/img/command_toolbars/shape_octagon.png"));
  aShapeOctagon->setCheckable(true);
  aShapeOctagon->setChecked(mLastPad.getShape() ==
                            FootprintPad::Shape::OCTAGON);
  aShapeOctagon->setActionGroup(shapeActionGroup.get());
  mContext.commandToolBar.addActionGroup(std::move(shapeActionGroup));
  mContext.commandToolBar.addSeparator();

  // width
  mContext.commandToolBar.addLabel(tr("Width:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtWidth(new PositiveLengthEdit());
  QPointer<PositiveLengthEdit> edtWidthPtr = edtWidth.get();
  edtWidth->configure(getDefaultLengthUnit(), LengthEditBase::Steps::generic(),
                      "package_editor/add_pads/width");
  edtWidth->setValue(mLastPad.getWidth());
  edtWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtWidth.get(), edtWidth.get(), &PositiveLengthEdit::stepUp));
  edtWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtWidth.get(), edtWidth.get(), &PositiveLengthEdit::stepDown));
  connect(edtWidth.get(), &PositiveLengthEdit::valueChanged, this,
          &PackageEditorState_AddPads::widthEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtWidth));

  // height
  mContext.commandToolBar.addLabel(tr("Height:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
  QPointer<PositiveLengthEdit> edtHeightPtr = edtHeight.get();
  edtHeight->configure(getDefaultLengthUnit(), LengthEditBase::Steps::generic(),
                       "package_editor/add_pads/height");
  edtHeight->setValue(mLastPad.getHeight());
  edtHeight->addAction(cmd.sizeIncrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepUp));
  edtHeight->addAction(cmd.sizeDecrease.createAction(
      edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepDown));
  connect(edtHeight.get(), &PositiveLengthEdit::valueChanged, this,
          &PackageEditorState_AddPads::heightEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtHeight));

  // drill diameter
  QPointer<UnsignedLengthEdit> edtDrillDiameterPtr;
  if (mPadType == PadType::THT) {
    mContext.commandToolBar.addLabel(tr("Drill Diameter:"), 10);
    std::unique_ptr<UnsignedLengthEdit> edtDrillDiameter(
        new UnsignedLengthEdit());
    edtDrillDiameterPtr = edtDrillDiameter.get();
    edtDrillDiameter->configure(getDefaultLengthUnit(),
                                LengthEditBase::Steps::drillDiameter(),
                                "package_editor/add_pads/drill_diameter");
    edtDrillDiameter->setValue(mLastPad.getDrillDiameter());
    edtDrillDiameter->addAction(cmd.drillIncrease.createAction(
        edtDrillDiameter.get(), edtDrillDiameter.get(),
        &PositiveLengthEdit::stepUp));
    edtDrillDiameter->addAction(cmd.drillDecrease.createAction(
        edtDrillDiameter.get(), edtDrillDiameter.get(),
        &PositiveLengthEdit::stepDown));
    connect(edtDrillDiameter.get(), &UnsignedLengthEdit::valueChanged, this,
            &PackageEditorState_AddPads::drillDiameterEditValueChanged);
    mContext.commandToolBar.addWidget(std::move(edtDrillDiameter));
  }

  // Avoid creating pads with a drill diameter larger than its size!
  // See https://github.com/LibrePCB/LibrePCB/issues/946.
  if (edtWidthPtr && edtHeightPtr && edtDrillDiameterPtr) {
    connect(edtWidthPtr.data(), &PositiveLengthEdit::valueChanged, this,
            [edtDrillDiameterPtr](const PositiveLength& value) {
              if (edtDrillDiameterPtr &&
                  (value < edtDrillDiameterPtr->getValue())) {
                edtDrillDiameterPtr->setValue(positiveToUnsigned(value));
              }
            });
    connect(edtHeightPtr.data(), &PositiveLengthEdit::valueChanged, this,
            [edtDrillDiameterPtr](const PositiveLength& value) {
              if (edtDrillDiameterPtr &&
                  (value < edtDrillDiameterPtr->getValue())) {
                edtDrillDiameterPtr->setValue(positiveToUnsigned(value));
              }
            });
    connect(edtDrillDiameterPtr.data(), &UnsignedLengthEdit::valueChanged, this,
            [edtWidthPtr, edtHeightPtr](const UnsignedLength& value) {
              if (edtWidthPtr && (value > edtWidthPtr->getValue())) {
                edtWidthPtr->setValue(PositiveLength(*value));
              }
              if (edtHeightPtr && (value > edtHeightPtr->getValue())) {
                edtHeightPtr->setValue(PositiveLength(*value));
              }
            });
  }

  Point pos =
      mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos(), true, true);
  if (!startAddPad(pos)) {
    return false;
  }
  mContext.graphicsView.setCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_AddPads::exit() noexcept {
  if (mCurrentPad && !abortAddPad()) {
    return false;
  }

  // cleanup command toolbar
  mPackagePadComboBox = nullptr;
  mContext.commandToolBar.clear();

  mContext.graphicsView.unsetCursor();
  return true;
}

QSet<EditorWidgetBase::Feature>
    PackageEditorState_AddPads::getAvailableFeatures() const noexcept {
  return {
      EditorWidgetBase::Feature::Abort,
      EditorWidgetBase::Feature::Rotate,
  };
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
    finishAddPad(currentPos);
  }
  return startAddPad(currentPos);
}

bool PackageEditorState_AddPads::processGraphicsSceneRightMouseButtonReleased(
    QGraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool PackageEditorState_AddPads::processRotate(const Angle& rotation) noexcept {
  if (mCurrentPad) {
    mEditCmd->rotate(rotation, mCurrentPad->getPosition(), true);
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
    mLastPad.setPosition(pos);
    mCurrentPad =
        std::make_shared<FootprintPad>(Uuid::createRandom(), mLastPad);
    mContext.undoStack.appendToCmdGroup(new CmdFootprintPadInsert(
        mContext.currentFootprint->getPads(), mCurrentPad));
    mEditCmd.reset(new CmdFootprintPadEdit(*mCurrentPad));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentPad);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentPad.reset();
    mEditCmd.reset();
    return false;
  }
}

bool PackageEditorState_AddPads::finishAddPad(const Point& pos) noexcept {
  try {
    mEditCmd->setPosition(pos, true);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mLastPad = *mCurrentPad;
    mCurrentPad.reset();
    mContext.undoStack.appendToCmdGroup(mEditCmd.take());
    mContext.undoStack.commitCmdGroup();
    selectNextFreePadInComboBox();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(&mContext.editorWidget, tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_AddPads::abortAddPad() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
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

void PackageEditorState_AddPads::selectNextFreePadInComboBox() noexcept {
  if (mContext.currentFootprint && mPackagePadComboBox) {
    tl::optional<Uuid> pad;
    for (const PackagePad& pkgPad : mContext.package.getPads()) {
      bool connected = false;
      for (const FootprintPad& fptPad : mContext.currentFootprint->getPads()) {
        if (fptPad.getPackagePadUuid() == pkgPad.getUuid()) {
          connected = true;
        }
      }
      if (!connected) {
        pad = pkgPad.getUuid();
        break;
      }
    }
    mPackagePadComboBox->setCurrentPad(pad);
  }
}

void PackageEditorState_AddPads::packagePadComboBoxCurrentPadChanged(
    tl::optional<Uuid> pad) noexcept {
  mLastPad.setPackagePadUuid(pad);
  if (mEditCmd) {
    mEditCmd->setPackagePadUuid(mLastPad.getPackagePadUuid(), true);
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
