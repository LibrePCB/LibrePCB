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

#include "../../../undostack.h"
#include "../../cmd/cmdfootprintpadedit.h"
#include "../footprintgraphicsitem.h"
#include "../footprintpadgraphicsitem.h"

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
    mCurrentProperties(
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
        PadHoleList{}),
    mCurrentPad(nullptr),
    mCurrentGraphicsItem(nullptr) {
  if (mPadType == PadType::SMT) {
    mCurrentProperties.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(50)));
    mCurrentProperties.setWidth(PositiveLength(1500000));
    mCurrentProperties.setHeight(PositiveLength(700000));
    mCurrentProperties.setSolderPasteConfig(MaskConfig::automatic());
    switch (function) {
      case FootprintPad::Function::ThermalPad:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(0)));
        mCurrentProperties.setWidth(PositiveLength(2000000));
        mCurrentProperties.setHeight(PositiveLength(2000000));
        break;
      case FootprintPad::Function::BgaPad:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(100)));
        mCurrentProperties.setWidth(PositiveLength(300000));
        mCurrentProperties.setHeight(PositiveLength(300000));
        break;
      case FootprintPad::Function::EdgeConnectorPad:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(0)));
        mCurrentProperties.setSolderPasteConfig(MaskConfig::off());
        break;
      case FootprintPad::Function::TestPad:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(100)));
        mCurrentProperties.setWidth(PositiveLength(700000));
        mCurrentProperties.setHeight(PositiveLength(700000));
        mCurrentProperties.setSolderPasteConfig(MaskConfig::off());
        break;
      case FootprintPad::Function::LocalFiducial:
      case FootprintPad::Function::GlobalFiducial:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(100)));
        mCurrentProperties.setWidth(PositiveLength(1000000));
        mCurrentProperties.setHeight(PositiveLength(1000000));
        mCurrentProperties.setCopperClearance(UnsignedLength(500000));
        mCurrentProperties.setStopMaskConfig(
            MaskConfig::manual(*mCurrentProperties.getCopperClearance()));
        mCurrentProperties.setSolderPasteConfig(MaskConfig::off());
        break;
      default:
        break;
    }
  } else {
    mCurrentProperties.getHoles().append(std::make_shared<PadHole>(
        Uuid::createRandom(),
        PositiveLength(800000),  // Commonly used drill diameter
        makeNonEmptyPath(Point())));
  }
  applyRecommendedRoundedRectRadius();
}

PackageEditorState_AddPads::~PackageEditorState_AddPads() noexcept {
  Q_ASSERT(!mCurrentEditCmd);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool PackageEditorState_AddPads::entry() noexcept {
  if (!mCurrentProperties.getFunctionIsFiducial()) {
    selectNextFreePackagePad();
  }

  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!startAddPad(pos)) {
    return false;
  }

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(PackageEditorFsmAdapter::Feature::Rotate);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool PackageEditorState_AddPads::exit() noexcept {
  if (mCurrentPad && !abortAddPad()) {
    return false;
  }

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(PackageEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool PackageEditorState_AddPads::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  if (mCurrentPad) {
    Point currentPos = e.scenePos.mappedToGrid(getGridInterval());
    mCurrentEditCmd->setPosition(currentPos, true);
    return true;
  } else {
    return false;
  }
}

bool PackageEditorState_AddPads::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  Point currentPos = e.scenePos.mappedToGrid(getGridInterval());
  if (mCurrentPad) {
    finishAddPad(currentPos);
  }
  return startAddPad(currentPos);
}

bool PackageEditorState_AddPads::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool PackageEditorState_AddPads::processRotate(const Angle& rotation) noexcept {
  if (mCurrentPad) {
    mCurrentEditCmd->rotate(rotation, mCurrentPad->getPosition(), true);
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

void PackageEditorState_AddPads::setPackagePad(
    const std::optional<Uuid>& pad) noexcept {
  if (mCurrentProperties.setPackagePadUuid(pad)) {
    emit packagePadChanged(mCurrentProperties.getPackagePadUuid());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setPackagePadUuid(mCurrentProperties.getPackagePadUuid(),
                                       true);
  }
}

void PackageEditorState_AddPads::setComponentSide(
    FootprintPad::ComponentSide side) noexcept {
  if (mCurrentProperties.setComponentSide(side)) {
    emit componentSideChanged(mCurrentProperties.getComponentSide());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setComponentSide(mCurrentProperties.getComponentSide(),
                                      true);
  }
}

void PackageEditorState_AddPads::setShape(FootprintPad::Shape shape) noexcept {
  if (mCurrentProperties.setShape(shape)) {
    emit shapeChanged(mCurrentProperties.getShape());
    applyRecommendedRoundedRectRadius();
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setShape(mCurrentProperties.getShape(), true);
  }
}

void PackageEditorState_AddPads::setWidth(
    const PositiveLength& width) noexcept {
  if (mCurrentProperties.setWidth(width)) {
    emit widthChanged(mCurrentProperties.getWidth());
    applyRecommendedRoundedRectRadius();
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setWidth(mCurrentProperties.getWidth(), true);
  }

  // Avoid creating pads with a drill larger than width or height.
  if (auto drill = getDrillDiameter()) {
    if (*drill > width) {
      setDrillDiameter(width);
    }
  }
}

void PackageEditorState_AddPads::setHeight(
    const PositiveLength& height) noexcept {
  if (mCurrentProperties.setHeight(height)) {
    emit heightChanged(mCurrentProperties.getHeight());
    applyRecommendedRoundedRectRadius();
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setHeight(mCurrentProperties.getHeight(), true);
  }

  // Avoid creating pads with a drill larger than width or height.
  if (auto drill = getDrillDiameter()) {
    if (*drill > height) {
      setDrillDiameter(height);
    }
  }
}

void PackageEditorState_AddPads::setRadius(
    const UnsignedLimitedRatio& radius) noexcept {
  if (mCurrentProperties.setRadius(radius)) {
    emit radiusChanged(mCurrentProperties.getRadius());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setRadius(mCurrentProperties.getRadius(), true);
  }
}

void PackageEditorState_AddPads::setDrillDiameter(
    const PositiveLength& diameter) noexcept {
  if (std::shared_ptr<PadHole> hole = mCurrentProperties.getHoles().value(0)) {
    if (hole->getDiameter() != diameter) {
      hole->setDiameter(diameter);
      emit drillDiameterChanged(diameter);
    }

    if (mCurrentEditCmd) {
      mCurrentEditCmd->setHoles(mCurrentProperties.getHoles(), true);
    }

    // Avoid creating pads with a drill larger than width or height.
    if (diameter > mCurrentProperties.getWidth()) {
      setWidth(diameter);
    }
    if (diameter > mCurrentProperties.getHeight()) {
      setHeight(diameter);
    }
  }
}

void PackageEditorState_AddPads::setCopperClearance(
    const UnsignedLength& clearance) noexcept {
  if (mCurrentProperties.setCopperClearance(clearance)) {
    emit copperClearanceChanged(mCurrentProperties.getCopperClearance());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setCopperClearance(mCurrentProperties.getCopperClearance(),
                                        true);
  }
}

void PackageEditorState_AddPads::setStopMaskConfig(
    const MaskConfig& cfg) noexcept {
  if (mCurrentProperties.setStopMaskConfig(cfg)) {
    emit stopMaskConfigChanged(mCurrentProperties.getStopMaskConfig());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setStopMaskConfig(mCurrentProperties.getStopMaskConfig(),
                                       true);
  }
}

void PackageEditorState_AddPads::setFunction(
    FootprintPad::Function function) noexcept {
  if (mCurrentProperties.setFunction(function)) {
    emit functionChanged(mCurrentProperties.getFunction());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setFunction(mCurrentProperties.getFunction(), true);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool PackageEditorState_AddPads::startAddPad(const Point& pos) noexcept {
  try {
    mContext.undoStack.beginCmdGroup(tr("Add footprint pad"));
    mCurrentProperties.setPosition(pos);
    mCurrentPad = std::make_shared<FootprintPad>(
        Uuid::createRandom(), mCurrentProperties.getPackagePadUuid(),
        mCurrentProperties.getPosition(), mCurrentProperties.getRotation(),
        mCurrentProperties.getShape(), mCurrentProperties.getWidth(),
        mCurrentProperties.getHeight(), mCurrentProperties.getRadius(),
        mCurrentProperties.getCustomShapeOutline(),
        mCurrentProperties.getStopMaskConfig(),
        mCurrentProperties.getSolderPasteConfig(),
        mCurrentProperties.getCopperClearance(),
        mCurrentProperties.getComponentSide(), mCurrentProperties.getFunction(),
        PadHoleList{});
    for (const PadHole& hole : mCurrentProperties.getHoles()) {
      mCurrentPad->getHoles().append(std::make_shared<PadHole>(
          Uuid::createRandom(), hole.getDiameter(), hole.getPath()));
    }
    mContext.undoStack.appendToCmdGroup(new CmdFootprintPadInsert(
        mContext.currentFootprint->getPads(), mCurrentPad));
    mCurrentEditCmd.reset(new CmdFootprintPadEdit(*mCurrentPad));
    mCurrentGraphicsItem =
        mContext.currentGraphicsItem->getGraphicsItem(mCurrentPad);
    Q_ASSERT(mCurrentGraphicsItem);
    mCurrentGraphicsItem->setSelected(true);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    mCurrentGraphicsItem.reset();
    mCurrentPad.reset();
    mCurrentEditCmd.reset();
    return false;
  }
}

bool PackageEditorState_AddPads::finishAddPad(const Point& pos) noexcept {
  try {
    mCurrentEditCmd->setPosition(pos, true);
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentProperties = *mCurrentPad;
    mCurrentPad.reset();
    mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    mContext.undoStack.commitCmdGroup();
    selectNextFreePackagePad();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

bool PackageEditorState_AddPads::abortAddPad() noexcept {
  try {
    mCurrentGraphicsItem->setSelected(false);
    mCurrentGraphicsItem.reset();
    mCurrentProperties = *mCurrentPad;
    mCurrentPad.reset();
    mCurrentEditCmd.reset();
    mContext.undoStack.abortCmdGroup();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    return false;
  }
}

void PackageEditorState_AddPads::selectNextFreePackagePad() noexcept {
  if (mContext.currentFootprint) {
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
    setPackagePad(pad);
  }
}

void PackageEditorState_AddPads::applyRecommendedRoundedRectRadius() noexcept {
  if ((*mCurrentProperties.getRadius() > Ratio::fromPercent(0)) &&
      (*mCurrentProperties.getRadius() < Ratio::fromPercent(100))) {
    setRadius(FootprintPad::getRecommendedRadius(
        mCurrentProperties.getWidth(), mCurrentProperties.getHeight()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
