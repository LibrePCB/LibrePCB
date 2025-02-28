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
#include "../../../graphics/graphicsscene.h"
#include "../../../widgets/graphicsview.h"
#include "../../../widgets/positivelengthedit.h"
#include "../../../widgets/unsignedlengthedit.h"
#include "../../../widgets/unsignedlimitedratioedit.h"
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

PackageEditorState_AddPads::PackageEditorState_AddPads(
    Context& context, PadType type, FootprintPad::Function function) noexcept
  : PackageEditorState(context),
    mPadType(type),
    mCurrentPad(nullptr),
    mCurrentGraphicsItem(nullptr),
    mPackagePadComboBox(nullptr),
    mLastPad(
        Uuid::createRandom(), std::nullopt, Point(0, 0), Angle::deg0(),
        FootprintPad::Shape::RoundedRect,  // Commonly used pad shape
        PositiveLength(2500000),  // There is no default/recommended pad size
        PositiveLength(1300000),  // -> choose reasonable multiple of 0.1mm
        UnsignedLimitedRatio(Ratio::fromPercent(100)),  // Rounded pad
        Path(),  // Custom shape outline
        MaskConfig::automatic(),  // Stop mask
        MaskConfig::off(),  // Solder paste
        UnsignedLength(0),  // Copper clearance
        FootprintPad::ComponentSide::Top,  // Default side
        function,  // Supplied by library editor
        PadHoleList{}) {
  if (mPadType == PadType::SMT) {
    mLastPad.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(50)));
    mLastPad.setWidth(PositiveLength(1500000));
    mLastPad.setHeight(PositiveLength(700000));
    mLastPad.setSolderPasteConfig(MaskConfig::automatic());
    switch (function) {
      case FootprintPad::Function::ThermalPad:
        mLastPad.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(0)));
        mLastPad.setWidth(PositiveLength(2000000));
        mLastPad.setHeight(PositiveLength(2000000));
        break;
      case FootprintPad::Function::BgaPad:
        mLastPad.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(100)));
        mLastPad.setWidth(PositiveLength(300000));
        mLastPad.setHeight(PositiveLength(300000));
        break;
      case FootprintPad::Function::EdgeConnectorPad:
        mLastPad.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(0)));
        mLastPad.setSolderPasteConfig(MaskConfig::off());
        break;
      case FootprintPad::Function::TestPad:
        mLastPad.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(100)));
        mLastPad.setWidth(PositiveLength(700000));
        mLastPad.setHeight(PositiveLength(700000));
        mLastPad.setSolderPasteConfig(MaskConfig::off());
        break;
      case FootprintPad::Function::LocalFiducial:
      case FootprintPad::Function::GlobalFiducial:
        mLastPad.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(100)));
        mLastPad.setWidth(PositiveLength(1000000));
        mLastPad.setHeight(PositiveLength(1000000));
        mLastPad.setCopperClearance(UnsignedLength(500000));
        mLastPad.setStopMaskConfig(
            MaskConfig::manual(*mLastPad.getCopperClearance()));
        mLastPad.setSolderPasteConfig(MaskConfig::off());
        break;
      default:
        break;
    }
  } else {
    mLastPad.getHoles().append(std::make_shared<PadHole>(
        Uuid::createRandom(),
        PositiveLength(800000),  // Commonly used drill diameter
        makeNonEmptyPath(Point())));
  }
  applyRecommendedRoundedRectRadius();
}

PackageEditorState_AddPads::~PackageEditorState_AddPads() noexcept {
  Q_ASSERT(!mEditCmd);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_AddPads::entry() noexcept {
  // populate command toolbar
  EditorCommandSet& cmd = EditorCommandSet::instance();

  // package pad
  if (!mLastPad.getFunctionIsFiducial()) {
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
  }

  // board side
  if (mPadType == PadType::SMT) {
    std::unique_ptr<BoardSideSelectorWidget> boardSideSelector(
        new BoardSideSelectorWidget());
    boardSideSelector->setCurrentBoardSide(mLastPad.getComponentSide());
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
      cmd.shapeRound.createAction(shapeActionGroup.get(), this, [this]() {
        shapeSelectorCurrentShapeChanged(
            FootprintPad::Shape::RoundedRect,
            UnsignedLimitedRatio(Ratio::fromPercent(100)), false);
      });
  aShapeRound->setCheckable(true);
  aShapeRound->setChecked(
      (mLastPad.getShape() == FootprintPad::Shape::RoundedRect) &&
      (*mLastPad.getRadius() == Ratio::fromPercent(100)));
  aShapeRound->setActionGroup(shapeActionGroup.get());
  QAction* aShapeRoundedRect =
      cmd.shapeRoundedRect.createAction(shapeActionGroup.get(), this, [this]() {
        shapeSelectorCurrentShapeChanged(
            FootprintPad::Shape::RoundedRect,
            UnsignedLimitedRatio(Ratio::fromPercent(50)), true);
      });
  aShapeRoundedRect->setCheckable(true);
  aShapeRoundedRect->setChecked(
      (mLastPad.getShape() == FootprintPad::Shape::RoundedRect) &&
      (*mLastPad.getRadius() != Ratio::fromPercent(0)) &&
      (*mLastPad.getRadius() != Ratio::fromPercent(100)));
  aShapeRoundedRect->setActionGroup(shapeActionGroup.get());
  QAction* aShapeRect =
      cmd.shapeRect.createAction(shapeActionGroup.get(), this, [this]() {
        shapeSelectorCurrentShapeChanged(
            FootprintPad::Shape::RoundedRect,
            UnsignedLimitedRatio(Ratio::fromPercent(0)), false);
      });
  aShapeRect->setCheckable(true);
  aShapeRect->setChecked(
      (mLastPad.getShape() == FootprintPad::Shape::RoundedRect) &&
      (*mLastPad.getRadius() == Ratio::fromPercent(0)));
  aShapeRect->setActionGroup(shapeActionGroup.get());
  QAction* aShapeOctagon =
      cmd.shapeOctagon.createAction(shapeActionGroup.get(), this, [this]() {
        shapeSelectorCurrentShapeChanged(
            FootprintPad::Shape::RoundedOctagon,
            UnsignedLimitedRatio(Ratio::fromPercent(0)), true);
      });
  aShapeOctagon->setCheckable(true);
  aShapeOctagon->setChecked(mLastPad.getShape() ==
                            FootprintPad::Shape::RoundedOctagon);
  aShapeOctagon->setActionGroup(shapeActionGroup.get());
  mContext.commandToolBar.addActionGroup(std::move(shapeActionGroup));
  mContext.commandToolBar.addSeparator();

  // width / size
  mContext.commandToolBar.addLabel(
      mLastPad.getFunctionIsFiducial() ? tr("Size:") : tr("Width:"), 10);
  std::unique_ptr<PositiveLengthEdit> edtWidth(new PositiveLengthEdit());
  QPointer<PositiveLengthEdit> edtWidthPtr = edtWidth.get();
  edtWidth->configure(getLengthUnit(), LengthEditBase::Steps::generic(),
                      "package_editor/add_pads/width");
  edtWidth->setValue(mLastPad.getWidth());
  edtWidth->addAction(cmd.lineWidthIncrease.createAction(
      edtWidth.get(), edtWidth.get(), &PositiveLengthEdit::stepUp));
  edtWidth->addAction(cmd.lineWidthDecrease.createAction(
      edtWidth.get(), edtWidth.get(), &PositiveLengthEdit::stepDown));
  connect(edtWidth.get(), &PositiveLengthEdit::valueChanged, this,
          &PackageEditorState_AddPads::widthEditValueChanged);
  if (mLastPad.getFunctionIsFiducial()) {
    connect(edtWidth.get(), &PositiveLengthEdit::valueChanged, this,
            &PackageEditorState_AddPads::heightEditValueChanged);
  }
  mContext.commandToolBar.addWidget(std::move(edtWidth));

  // height
  QPointer<PositiveLengthEdit> edtHeightPtr;
  if (!mLastPad.getFunctionIsFiducial()) {
    mContext.commandToolBar.addLabel(tr("Height:"), 10);
    std::unique_ptr<PositiveLengthEdit> edtHeight(new PositiveLengthEdit());
    edtHeightPtr = edtHeight.get();
    edtHeight->configure(getLengthUnit(), LengthEditBase::Steps::generic(),
                         "package_editor/add_pads/height");
    edtHeight->setValue(mLastPad.getHeight());
    edtHeight->addAction(cmd.sizeIncrease.createAction(
        edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepUp));
    edtHeight->addAction(cmd.sizeDecrease.createAction(
        edtHeight.get(), edtHeight.get(), &PositiveLengthEdit::stepDown));
    connect(edtHeight.get(), &PositiveLengthEdit::valueChanged, this,
            &PackageEditorState_AddPads::heightEditValueChanged);
    mContext.commandToolBar.addWidget(std::move(edtHeight));
  }

  // drill diameter
  QPointer<PositiveLengthEdit> edtDrillDiameterPtr;
  if ((mPadType == PadType::THT) && (!mLastPad.getHoles().isEmpty())) {
    mContext.commandToolBar.addLabel(tr("Drill:"), 10);
    std::unique_ptr<PositiveLengthEdit> edtDrillDiameter(
        new PositiveLengthEdit());
    edtDrillDiameterPtr = edtDrillDiameter.get();
    edtDrillDiameter->configure(getLengthUnit(),
                                LengthEditBase::Steps::drillDiameter(),
                                "package_editor/add_pads/drill_diameter");
    edtDrillDiameter->setValue(mLastPad.getHoles().first()->getDiameter());
    edtDrillDiameter->addAction(cmd.drillIncrease.createAction(
        edtDrillDiameter.get(), edtDrillDiameter.get(),
        &PositiveLengthEdit::stepUp));
    edtDrillDiameter->addAction(cmd.drillDecrease.createAction(
        edtDrillDiameter.get(), edtDrillDiameter.get(),
        &PositiveLengthEdit::stepDown));
    connect(edtDrillDiameter.get(), &PositiveLengthEdit::valueChanged, this,
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
                edtDrillDiameterPtr->setValue(value);
              }
            });
    connect(edtHeightPtr.data(), &PositiveLengthEdit::valueChanged, this,
            [edtDrillDiameterPtr](const PositiveLength& value) {
              if (edtDrillDiameterPtr &&
                  (value < edtDrillDiameterPtr->getValue())) {
                edtDrillDiameterPtr->setValue(value);
              }
            });
    connect(edtDrillDiameterPtr.data(), &PositiveLengthEdit::valueChanged, this,
            [edtWidthPtr, edtHeightPtr](const PositiveLength& value) {
              if (edtWidthPtr && (value > edtWidthPtr->getValue())) {
                edtWidthPtr->setValue(value);
              }
              if (edtHeightPtr && (value > edtHeightPtr->getValue())) {
                edtHeightPtr->setValue(value);
              }
            });
  }

  // fiducial clearance
  if (mLastPad.getFunctionIsFiducial()) {
    mContext.commandToolBar.addLabel(tr("Clearance:"), 10);
    std::unique_ptr<UnsignedLengthEdit> edtFiducialClearance(
        new UnsignedLengthEdit());
    edtFiducialClearance->configure(
        getLengthUnit(), LengthEditBase::Steps::generic(),
        "package_editor/add_pads/fiducial_clearance");
    const std::optional<Length> offset =
        mLastPad.getStopMaskConfig().getOffset();
    if (offset && (*offset > 0)) {
      edtFiducialClearance->setValue(UnsignedLength(*offset));
    }
    edtFiducialClearance->addAction(cmd.sizeIncrease.createAction(
        edtFiducialClearance.get(), edtFiducialClearance.get(),
        &PositiveLengthEdit::stepUp));
    edtFiducialClearance->addAction(cmd.sizeDecrease.createAction(
        edtFiducialClearance.get(), edtFiducialClearance.get(),
        &PositiveLengthEdit::stepDown));
    connect(edtFiducialClearance.get(), &UnsignedLengthEdit::valueChanged, this,
            &PackageEditorState_AddPads::fiducialClearanceEditValueChanged);
    mContext.commandToolBar.addWidget(std::move(edtFiducialClearance));
  }

  // Radius.
  mContext.commandToolBar.addLabel(tr("Radius:"), 10);
  std::unique_ptr<UnsignedLimitedRatioEdit> edtRadius(
      new UnsignedLimitedRatioEdit());
  edtRadius->setSingleStep(1.0);  // [%]
  edtRadius->setValue(mLastPad.getRadius());
  edtRadius->setEnabled(aShapeRoundedRect->isChecked() ||
                        aShapeOctagon->isChecked());
  connect(this, &PackageEditorState_AddPads::requestRadiusInputEnabled,
          edtRadius.get(), &UnsignedLimitedRatioEdit::setEnabled);
  connect(this, &PackageEditorState_AddPads::requestRadius, edtRadius.get(),
          &UnsignedLimitedRatioEdit::setValue);
  connect(edtRadius.get(), &UnsignedLimitedRatioEdit::valueChanged, this,
          &PackageEditorState_AddPads::radiusEditValueChanged);
  mContext.commandToolBar.addWidget(std::move(edtRadius));

  // Press-Fit.
  if (mPadType == PadType::THT) {
    std::unique_ptr<QCheckBox> cbxPressFit(new QCheckBox(tr("Press-Fit")));
    cbxPressFit->setChecked(mLastPad.getFunction() ==
                            FootprintPad::Function::PressFitPad);
    connect(cbxPressFit.get(), &QCheckBox::toggled, this,
            &PackageEditorState_AddPads::pressFitCheckedChanged);
    mContext.commandToolBar.addWidget(std::move(cbxPressFit), 10);
  }

  const Point pos = mContext.graphicsView.mapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(mContext.graphicsScene.getGridInterval());
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
    mCurrentPad = std::make_shared<FootprintPad>(
        Uuid::createRandom(), mLastPad.getPackagePadUuid(),
        mLastPad.getPosition(), mLastPad.getRotation(), mLastPad.getShape(),
        mLastPad.getWidth(), mLastPad.getHeight(), mLastPad.getRadius(),
        mLastPad.getCustomShapeOutline(), mLastPad.getStopMaskConfig(),
        mLastPad.getSolderPasteConfig(), mLastPad.getCopperClearance(),
        mLastPad.getComponentSide(), mLastPad.getFunction(), PadHoleList{});
    for (const PadHole& hole : mLastPad.getHoles()) {
      mCurrentPad->getHoles().append(std::make_shared<PadHole>(
          Uuid::createRandom(), hole.getDiameter(), hole.getPath()));
    }
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
    mContext.undoStack.appendToCmdGroup(mEditCmd.release());
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
    std::optional<Uuid> pad;
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
    std::optional<Uuid> pad) noexcept {
  mLastPad.setPackagePadUuid(pad);
  if (mEditCmd) {
    mEditCmd->setPackagePadUuid(mLastPad.getPackagePadUuid(), true);
  }
}

void PackageEditorState_AddPads::boardSideSelectorCurrentSideChanged(
    FootprintPad::ComponentSide side) noexcept {
  mLastPad.setComponentSide(side);
  if (mEditCmd) {
    mEditCmd->setComponentSide(side, true);
  }
}

void PackageEditorState_AddPads::shapeSelectorCurrentShapeChanged(
    FootprintPad::Shape shape, const UnsignedLimitedRatio& radius,
    bool customRadius) noexcept {
  mLastPad.setShape(shape);
  if (mEditCmd) {
    mEditCmd->setShape(shape, true);
  }
  requestRadius(radius);
  requestRadiusInputEnabled(customRadius);
  applyRecommendedRoundedRectRadius();
}

void PackageEditorState_AddPads::widthEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastPad.setWidth(value);
  if (mEditCmd) {
    mEditCmd->setWidth(mLastPad.getWidth(), true);
  }
  applyRecommendedRoundedRectRadius();
}

void PackageEditorState_AddPads::heightEditValueChanged(
    const PositiveLength& value) noexcept {
  mLastPad.setHeight(value);
  if (mEditCmd) {
    mEditCmd->setHeight(mLastPad.getHeight(), true);
  }
  applyRecommendedRoundedRectRadius();
}

void PackageEditorState_AddPads::drillDiameterEditValueChanged(
    const PositiveLength& value) noexcept {
  if (std::shared_ptr<PadHole> hole = mLastPad.getHoles().value(0)) {
    hole->setDiameter(value);
    if (mEditCmd) {
      mEditCmd->setHoles(mLastPad.getHoles(), true);
    }
  }
}

void PackageEditorState_AddPads::fiducialClearanceEditValueChanged(
    const UnsignedLength& value) noexcept {
  mLastPad.setStopMaskConfig(MaskConfig::manual(*value));
  if (mEditCmd) {
    mEditCmd->setStopMaskConfig(mLastPad.getStopMaskConfig(), true);
  }
}

void PackageEditorState_AddPads::radiusEditValueChanged(
    const UnsignedLimitedRatio& value) noexcept {
  mLastPad.setRadius(value);
  if (mEditCmd) {
    mEditCmd->setRadius(mLastPad.getRadius(), true);
  }
}

void PackageEditorState_AddPads::pressFitCheckedChanged(bool value) noexcept {
  mLastPad.setFunction(value ? FootprintPad::Function::PressFitPad
                             : FootprintPad::Function::StandardPad);
  if (mEditCmd) {
    mEditCmd->setFunction(mLastPad.getFunction(), true);
  }
}

void PackageEditorState_AddPads::applyRecommendedRoundedRectRadius() noexcept {
  if ((*mLastPad.getRadius() > Ratio::fromPercent(0)) &&
      (*mLastPad.getRadius() < Ratio::fromPercent(100))) {
    emit requestRadius(FootprintPad::getRecommendedRadius(
        mLastPad.getWidth(), mLastPad.getHeight()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
