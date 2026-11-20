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

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static QString getSettingsScope(PackageEditorState_AddPads::PadType type,
                                Pad::Function function) noexcept {
  const char* typeStr =
      (type == PackageEditorState_AddPads::PadType::THT) ? "tht" : "smt";
  return QString("add_%1_%2_pad").arg(typeStr, serializeToString(function));
}

static FootprintPad getInitialProperties(
    PackageEditorState_AddPads::PadType type, Pad::Function function,
    const ClientSettings& settings) noexcept {
  // Default properties
  FootprintPad data(
      Uuid::createRandom(), std::nullopt, Point(0, 0), Angle::deg0(),
      Pad::Shape::RoundedRect,  // Commonly used pad shape
      PositiveLength(2500000),  // There is no default/recommended pad size
      PositiveLength(1300000),  // -> choose reasonable multiple of 0.1mm
      UnsignedLimitedRatio(Ratio::fromPercent(100)),  // Rounded pad
      Path(),  // Custom shape outline
      MaskConfig::automatic(),  // Stop mask
      MaskConfig::off(),  // Solder paste
      UnsignedLength(0),  // Copper clearance
      Pad::ComponentSide::Top,  // Default side
      function,  // Supplied by library editor
      PadHoleList{});
  if (type == PackageEditorState_AddPads::PadType::THT) {
    data.getHoles().append(std::make_shared<PadHole>(
        Uuid::createRandom(),
        PositiveLength(800000),  // Commonly used drill diameter
        makeNonEmptyPath(Point())));
  } else {
    data.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(50)));
    data.setWidth(PositiveLength(1500000));
    data.setHeight(PositiveLength(700000));
    data.setSolderPasteConfig(MaskConfig::automatic());
    switch (function) {
      case Pad::Function::ThermalPad:
        data.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(0)));
        data.setWidth(PositiveLength(2000000));
        data.setHeight(PositiveLength(2000000));
        break;
      case Pad::Function::BgaPad:
        data.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(100)));
        data.setWidth(PositiveLength(300000));
        data.setHeight(PositiveLength(300000));
        break;
      case Pad::Function::EdgeConnectorPad:
        data.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(0)));
        data.setSolderPasteConfig(MaskConfig::off());
        break;
      case Pad::Function::TestPad:
        data.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(100)));
        data.setWidth(PositiveLength(700000));
        data.setHeight(PositiveLength(700000));
        data.setSolderPasteConfig(MaskConfig::off());
        break;
      case Pad::Function::LocalFiducial:
      case Pad::Function::GlobalFiducial:
        data.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(100)));
        data.setWidth(PositiveLength(1000000));
        data.setHeight(PositiveLength(1000000));
        data.setCopperClearance(UnsignedLength(500000));
        data.setStopMaskConfig(MaskConfig::manual(*data.getCopperClearance()));
        data.setSolderPasteConfig(MaskConfig::off());
        break;
      default:
        break;
    }
  }

  // Now load client settings.
  if (auto v = settings.get<Angle>("rotation")) {
    data.setRotation(v->mappedTo0_360deg());
  }
  if (auto v = settings.get<Pad::Shape>("shape")) {
    data.setShape(*v);
  }
  if (auto v = settings.get<PositiveLength>("width")) {
    data.setWidth(*v);
  }
  if (auto v = settings.get<PositiveLength>("height")) {
    data.setHeight(*v);
  }
  if (auto v = settings.get<UnsignedLimitedRatio>("radius")) {
    data.setRadius(*v);
  } else if ((*data.getRadius() > Ratio::fromPercent(0)) &&
             (*data.getRadius() < Ratio::fromPercent(100))) {
    data.setRadius(
        FootprintPad::getRecommendedRadius(data.getWidth(), data.getHeight()));
  }
  if (data.getCopperClearance() > 0) {  // Only for fiducials
    if (auto v = settings.get<UnsignedLength>("copper_clearance")) {
      data.setCopperClearance(*v);
      data.setStopMaskConfig(MaskConfig::manual(**v));
    }
  }
  if (type == PackageEditorState_AddPads::PadType::SMT) {
    if (auto v = settings.get<Pad::ComponentSide>("component_side")) {
      data.setComponentSide(*v);
    }
  }
  if (type == PackageEditorState_AddPads::PadType::THT) {
    if (std::shared_ptr<PadHole> hole = data.getHoles().value(0)) {
      if (auto v = settings.get<PositiveLength>("drill_diameter")) {
        hole->setDiameter(*v);
      }
    }
  }
  return data;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState_AddPads::PackageEditorState_AddPads(
    Context& context, PadType type, Pad::Function function) noexcept
  : PackageEditorState(context, getSettingsScope(type, function)),
    mPadType(type),
    mCurrentProperties(getInitialProperties(type, function, mSettings)),
    mCurrentPad(nullptr),
    mCurrentGraphicsItem(nullptr) {
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
  if (mCurrentPad && mCurrentEditCmd) {
    mCurrentEditCmd->rotate(rotation, mCurrentPad->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentPad->getRotation());
    mSettings.set("rotation", mCurrentProperties.getRotation());
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
  if (!mCurrentProperties.setPackagePadUuid(pad)) {
    return;
  }

  emit packagePadChanged(mCurrentProperties.getPackagePadUuid());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setPackagePadUuid(mCurrentProperties.getPackagePadUuid(),
                                       true);
  }
}

void PackageEditorState_AddPads::setComponentSide(
    Pad::ComponentSide side) noexcept {
  if (!mCurrentProperties.setComponentSide(side)) {
    return;
  }

  mSettings.set("component_side", mCurrentProperties.getComponentSide());
  emit componentSideChanged(mCurrentProperties.getComponentSide());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setComponentSide(mCurrentProperties.getComponentSide(),
                                      true);
  }
}

void PackageEditorState_AddPads::setShape(Pad::Shape shape) noexcept {
  if (!mCurrentProperties.setShape(shape)) {
    return;
  }

  mSettings.set("shape", mCurrentProperties.getShape());
  emit shapeChanged(mCurrentProperties.getShape());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setShape(mCurrentProperties.getShape(), true);
  }

  applyRecommendedRoundedRectRadius();
}

void PackageEditorState_AddPads::setWidth(
    const PositiveLength& width) noexcept {
  // Avoid creating pads with a drill larger than width or height.
  if (auto drill = getDrillDiameter()) {
    if (*drill > width) {
      setDrillDiameter(width);
    }
  }

  if (!mCurrentProperties.setWidth(width)) {
    return;
  }

  mSettings.set("width", mCurrentProperties.getWidth());
  emit widthChanged(mCurrentProperties.getWidth());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setWidth(mCurrentProperties.getWidth(), true);
  }

  applyRecommendedRoundedRectRadius();
}

void PackageEditorState_AddPads::setHeight(
    const PositiveLength& height) noexcept {
  // Avoid creating pads with a drill larger than width or height.
  if (auto drill = getDrillDiameter()) {
    if (*drill > height) {
      setDrillDiameter(height);
    }
  }

  if (!mCurrentProperties.setHeight(height)) {
    return;
  }

  mSettings.set("height", mCurrentProperties.getHeight());
  emit heightChanged(mCurrentProperties.getHeight());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setHeight(mCurrentProperties.getHeight(), true);
  }

  applyRecommendedRoundedRectRadius();
}

void PackageEditorState_AddPads::setRadius(
    const UnsignedLimitedRatio& radius) noexcept {
  if (!mCurrentProperties.setRadius(radius)) {
    return;
  }

  mSettings.set("radius", mCurrentProperties.getRadius());
  emit radiusChanged(mCurrentProperties.getRadius());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setRadius(mCurrentProperties.getRadius(), true);
  }
}

void PackageEditorState_AddPads::setDrillDiameter(
    const PositiveLength& diameter) noexcept {
  // Avoid creating pads with a drill larger than width or height.
  if (diameter > mCurrentProperties.getWidth()) {
    setWidth(diameter);
  }
  if (diameter > mCurrentProperties.getHeight()) {
    setHeight(diameter);
  }

  std::shared_ptr<PadHole> hole = mCurrentProperties.getHoles().value(0);
  if ((!hole) || (hole->getDiameter() == diameter)) {
    return;
  }

  hole->setDiameter(diameter);
  mSettings.set("drill_diameter", diameter);
  emit drillDiameterChanged(diameter);

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setHoles(mCurrentProperties.getHoles(), true);
  }
}

void PackageEditorState_AddPads::setCopperClearance(
    const UnsignedLength& clearance) noexcept {
  if (!mCurrentProperties.setCopperClearance(clearance)) {
    return;
  }

  mSettings.set("copper_clearance", mCurrentProperties.getCopperClearance());
  emit copperClearanceChanged(mCurrentProperties.getCopperClearance());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setCopperClearance(mCurrentProperties.getCopperClearance(),
                                        true);
  }
}

void PackageEditorState_AddPads::setStopMaskConfig(
    const MaskConfig& cfg) noexcept {
  if (!mCurrentProperties.setStopMaskConfig(cfg)) {
    return;
  }

  emit stopMaskConfigChanged(mCurrentProperties.getStopMaskConfig());

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setStopMaskConfig(mCurrentProperties.getStopMaskConfig(),
                                       true);
  }
}

void PackageEditorState_AddPads::setFunction(Pad::Function function) noexcept {
  if (!mCurrentProperties.setFunction(function)) {
    return;
  }

  emit functionChanged(mCurrentProperties.getFunction());

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
    mCurrentEditCmd = std::make_unique<CmdFootprintPadEdit>(*mCurrentPad);
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
