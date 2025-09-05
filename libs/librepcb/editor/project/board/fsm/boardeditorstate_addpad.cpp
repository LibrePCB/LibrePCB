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
#include "boardeditorstate_addpad.h"

#include "../../../undostack.h"
#include "../../cmd/cmdboardnetsegmentadd.h"
#include "../../cmd/cmdboardnetsegmentaddelements.h"
#include "../../cmd/cmdboardnetsegmentedit.h"
#include "../../cmd/cmdboardnetsegmentremove.h"
#include "../../cmd/cmdboardpadedit.h"
#include "../boardgraphicsscene.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/items/bi_netsegment.h>
#include <librepcb/core/project/board/items/bi_pad.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditorState_AddPad::BoardEditorState_AddPad(
    const Context& context, PadType type, Pad::Function function) noexcept
  : BoardEditorState(context),
    mPadType(type),
    mIsUndoCmdActive(false),
    mCurrentProperties(
        Uuid::createRandom(),  // UUID is not relevant here
        Point(0, 0),  // Position
        Angle::deg0(),  // Rotation
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
        PadHoleList{},  // Holes
        false  // Locked
        ),
    mCurrentNetSignal(std::nullopt),
    mCurrentPad(nullptr) {
  if (mPadType == PadType::SMT) {
    mCurrentProperties.setRadius(UnsignedLimitedRatio(Ratio::fromPercent(50)));
    mCurrentProperties.setWidth(PositiveLength(1500000));
    mCurrentProperties.setHeight(PositiveLength(700000));
    mCurrentProperties.setSolderPasteConfig(MaskConfig::automatic());
    switch (function) {
      case Pad::Function::ThermalPad:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(0)));
        mCurrentProperties.setWidth(PositiveLength(2000000));
        mCurrentProperties.setHeight(PositiveLength(2000000));
        break;
      case Pad::Function::BgaPad:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(100)));
        mCurrentProperties.setWidth(PositiveLength(300000));
        mCurrentProperties.setHeight(PositiveLength(300000));
        break;
      case Pad::Function::EdgeConnectorPad:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(0)));
        mCurrentProperties.setSolderPasteConfig(MaskConfig::off());
        break;
      case Pad::Function::TestPad:
        mCurrentProperties.setRadius(
            UnsignedLimitedRatio(Ratio::fromPercent(100)));
        mCurrentProperties.setWidth(PositiveLength(700000));
        mCurrentProperties.setHeight(PositiveLength(700000));
        mCurrentProperties.setSolderPasteConfig(MaskConfig::off());
        break;
      case Pad::Function::LocalFiducial:
      case Pad::Function::GlobalFiducial:
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

BoardEditorState_AddPad::~BoardEditorState_AddPad() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

bool BoardEditorState_AddPad::entry() noexcept {
  Q_ASSERT(mIsUndoCmdActive == false);

  // Add a new via
  const Point pos = mAdapter.fsmMapGlobalPosToScenePos(QCursor::pos())
                        .mappedToGrid(getGridInterval());
  if (!start(pos)) return false;

  mAdapter.fsmToolEnter(*this);
  mAdapter.fsmSetFeatures(BoardEditorFsmAdapter::Feature::Rotate);
  mAdapter.fsmSetViewCursor(Qt::CrossCursor);
  return true;
}

bool BoardEditorState_AddPad::exit() noexcept {
  // Abort the currently active command
  if (!abortCommand(true)) return false;

  mAdapter.fsmSetViewCursor(std::nullopt);
  mAdapter.fsmSetFeatures(BoardEditorFsmAdapter::Features());
  mAdapter.fsmToolLeave();
  return true;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

bool BoardEditorState_AddPad::processGraphicsSceneMouseMoved(
    const GraphicsSceneMouseEvent& e) noexcept {
  BoardGraphicsScene* scene = getActiveBoardScene();
  if (!scene) return false;

  Point pos = e.scenePos.mappedToGrid(getGridInterval());
  return updatePosition(*scene, pos);
}

bool BoardEditorState_AddPad::processGraphicsSceneLeftMouseButtonPressed(
    const GraphicsSceneMouseEvent& e) noexcept {
  const Point pos = e.scenePos.mappedToGrid(getGridInterval());
  finish(pos);
  start(pos);
  return true;
}

bool BoardEditorState_AddPad::processGraphicsSceneLeftMouseButtonDoubleClicked(
    const GraphicsSceneMouseEvent& e) noexcept {
  return processGraphicsSceneLeftMouseButtonPressed(e);
}

bool BoardEditorState_AddPad::processGraphicsSceneRightMouseButtonReleased(
    const GraphicsSceneMouseEvent& e) noexcept {
  Q_UNUSED(e);
  return processRotate(Angle::deg90());
}

bool BoardEditorState_AddPad::processRotate(const Angle& rotation) noexcept {
  if (mCurrentPad && mCurrentEditCmd) {
    mCurrentEditCmd->rotate(rotation, mCurrentPad->getPosition(), true);
    mCurrentProperties.setRotation(mCurrentPad->getRotation());
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Connection to UI
 ******************************************************************************/

QVector<std::pair<Uuid, QString>> BoardEditorState_AddPad::getAvailableNets()
    const noexcept {
  QVector<std::pair<Uuid, QString>> nets;
  for (const NetSignal* net :
       mContext.project.getCircuit().getNetSignals().values()) {
    nets.append(std::make_pair(net->getUuid(), *net->getName()));
  }
  Toolbox::sortNumeric(
      nets,
      [](const QCollator& cmp, const std::pair<Uuid, QString>& lhs,
         const std::pair<Uuid, QString>& rhs) {
        return cmp(lhs.second, rhs.second);
      },
      Qt::CaseInsensitive, false);
  return nets;
}

void BoardEditorState_AddPad::setNet(const std::optional<Uuid>& net) noexcept {
  if (net != mCurrentNetSignal) {
    mCurrentNetSignal = net;
    emit netChanged(mCurrentNetSignal);
  }

  applySelectedNetSignal();
}

void BoardEditorState_AddPad::setComponentSide(
    Pad::ComponentSide side) noexcept {
  if (mCurrentProperties.setComponentSide(side)) {
    emit componentSideChanged(mCurrentProperties.getComponentSide());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setComponentSideAndHoles(
        mCurrentProperties.getComponentSide(), mCurrentProperties.getHoles(),
        true);
  }

  makePadLayerVisible();
}

void BoardEditorState_AddPad::setShape(Pad::Shape shape) noexcept {
  if (mCurrentProperties.setShape(shape)) {
    emit shapeChanged(mCurrentProperties.getShape());
    applyRecommendedRoundedRectRadius();
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setShape(mCurrentProperties.getShape(), true);
  }
}

void BoardEditorState_AddPad::setWidth(const PositiveLength& width) noexcept {
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

void BoardEditorState_AddPad::setHeight(const PositiveLength& height) noexcept {
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

void BoardEditorState_AddPad::setRadius(
    const UnsignedLimitedRatio& radius) noexcept {
  if (mCurrentProperties.setRadius(radius)) {
    emit radiusChanged(mCurrentProperties.getRadius());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setRadius(mCurrentProperties.getRadius(), true);
  }
}

void BoardEditorState_AddPad::setDrillDiameter(
    const PositiveLength& diameter) noexcept {
  if (std::shared_ptr<PadHole> hole = mCurrentProperties.getHoles().value(0)) {
    if (hole->getDiameter() != diameter) {
      hole->setDiameter(diameter);
      emit drillDiameterChanged(diameter);
    }

    if (mCurrentEditCmd) {
      mCurrentEditCmd->setComponentSideAndHoles(
          mCurrentProperties.getComponentSide(), mCurrentProperties.getHoles(),
          true);
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

void BoardEditorState_AddPad::setCopperClearance(
    const UnsignedLength& clearance) noexcept {
  if (mCurrentProperties.setCopperClearance(clearance)) {
    emit copperClearanceChanged(mCurrentProperties.getCopperClearance());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setCopperClearance(mCurrentProperties.getCopperClearance(),
                                        true);
  }
}

void BoardEditorState_AddPad::setStopMaskConfig(
    const MaskConfig& cfg) noexcept {
  if (mCurrentProperties.setStopMaskConfig(cfg)) {
    emit stopMaskConfigChanged(mCurrentProperties.getStopMaskConfig());
  }

  if (mCurrentEditCmd) {
    mCurrentEditCmd->setStopMaskConfig(mCurrentProperties.getStopMaskConfig(),
                                       true);
  }
}

void BoardEditorState_AddPad::setFunction(Pad::Function function) noexcept {
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

bool BoardEditorState_AddPad::start(const Point& pos) noexcept {
  // Discard any temporary changes and release undo stack.
  abortBlockingToolsInOtherEditors();

  Q_ASSERT(mIsUndoCmdActive == false);

  try {
    // Assign new UUIDs to all holes.
    for (PadHole& hole : mCurrentProperties.getHoles()) {
      hole = PadHole(Uuid::createRandom(), hole);
    }

    mContext.undoStack.beginCmdGroup(tr("Add Pad to Board"));
    mIsUndoCmdActive = true;
    CmdBoardNetSegmentAdd* cmdAddSeg =
        new CmdBoardNetSegmentAdd(mContext.board, getCurrentNetSignal());
    mContext.undoStack.appendToCmdGroup(cmdAddSeg);
    BI_NetSegment* netsegment = cmdAddSeg->getNetSegment();
    Q_ASSERT(netsegment);
    mCurrentProperties.setPosition(pos);
    std::unique_ptr<CmdBoardNetSegmentAddElements> cmdAddPad(
        new CmdBoardNetSegmentAddElements(*netsegment));
    mCurrentPad = cmdAddPad->addPad(
        BoardPadData(Uuid::createRandom(), mCurrentProperties));
    Q_ASSERT(mCurrentPad);
    mContext.undoStack.appendToCmdGroup(cmdAddPad.release());
    mCurrentEditCmd.reset(new CmdBoardPadEdit(*mCurrentPad));

    // Highlight all elements of the current netsignal.
    mAdapter.fsmSetHighlightedNetSignals({netsegment->getNetSignal()});

    makePadLayerVisible();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddPad::updatePosition(BoardGraphicsScene& scene,
                                             const Point& pos) noexcept {
  if (mCurrentEditCmd) {
    mCurrentEditCmd->setPosition(pos, true);
    scene.getBoard().triggerAirWiresRebuild();
    return true;
  } else {
    return false;
  }
}

bool BoardEditorState_AddPad::finish(const Point& pos) noexcept {
  Q_ASSERT(mIsUndoCmdActive == true);

  try {
    if (mCurrentEditCmd) {
      mCurrentEditCmd->setPosition(pos, false);
      mContext.undoStack.appendToCmdGroup(mCurrentEditCmd.release());
    }
    mContext.undoStack.commitCmdGroup();
    mIsUndoCmdActive = false;
    mCurrentPad = nullptr;
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    abortCommand(false);
    return false;
  }
}

bool BoardEditorState_AddPad::abortCommand(bool showErrMsgBox) noexcept {
  try {
    // Clear highlighted net signal.
    mAdapter.fsmSetHighlightedNetSignals({});

    // Delete the current edit command
    mCurrentEditCmd.reset();

    // Abort the undo command
    if (mIsUndoCmdActive) {
      mContext.undoStack.abortCmdGroup();
      mIsUndoCmdActive = false;
    }

    // Reset attributes, go back to idle state
    mCurrentPad = nullptr;
    return true;
  } catch (const Exception& e) {
    if (showErrMsgBox) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
    return false;
  }
}

void BoardEditorState_AddPad::applySelectedNetSignal() noexcept {
  NetSignal* netsignal = getCurrentNetSignal();
  if ((mIsUndoCmdActive) && (mCurrentPad) &&
      (netsignal != mCurrentPad->getNetSegment()->getNetSignal())) {
    try {
      mContext.undoStack.appendToCmdGroup(
          new CmdBoardNetSegmentRemove(*mCurrentPad->getNetSegment()));
      std::unique_ptr<CmdBoardNetSegmentEdit> cmdEdit(
          new CmdBoardNetSegmentEdit(*mCurrentPad->getNetSegment()));
      cmdEdit->setNetSignal(netsignal);
      mContext.undoStack.appendToCmdGroup(cmdEdit.release());
      mContext.undoStack.appendToCmdGroup(
          new CmdBoardNetSegmentAdd(*mCurrentPad->getNetSegment()));
    } catch (const Exception& e) {
      QMessageBox::critical(parentWidget(), tr("Error"), e.getMsg());
    }
  }

  // Highlight all elements of the current netsignal.
  mAdapter.fsmSetHighlightedNetSignals({netsignal});
}

NetSignal* BoardEditorState_AddPad::getCurrentNetSignal() const noexcept {
  return mCurrentNetSignal
      ? mContext.project.getCircuit().getNetSignals().value(*mCurrentNetSignal)
      : nullptr;
}

void BoardEditorState_AddPad::applyRecommendedRoundedRectRadius() noexcept {
  if ((*mCurrentProperties.getRadius() > Ratio::fromPercent(0)) &&
      (*mCurrentProperties.getRadius() < Ratio::fromPercent(100))) {
    setRadius(Pad::getRecommendedRadius(mCurrentProperties.getWidth(),
                                        mCurrentProperties.getHeight()));
  }
}

void BoardEditorState_AddPad::makePadLayerVisible() noexcept {
  if (!mCurrentProperties.getHoles().isEmpty()) {
    makeLayerVisible(Theme::Color::sBoardPads);
  } else {
    makeLayerVisible(
        (mCurrentProperties.getComponentSide() == Pad::ComponentSide::Top)
            ? Theme::Color::sBoardCopperTop
            : Theme::Color::sBoardCopperBot);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
