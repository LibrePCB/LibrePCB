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

#ifndef LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDPAD_H
#define LIBREPCB_EDITOR_BOARDEDITORSTATE_ADDPAD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "boardeditorstate.h"

#include <librepcb/core/project/board/items/bi_pad.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class BoardGraphicsScene;
class CmdBoardPadEdit;

/*******************************************************************************
 *  Class BoardEditorState_AddPad
 ******************************************************************************/

/**
 * @brief The "add via" state/tool of the board editor
 */
class BoardEditorState_AddPad final : public BoardEditorState {
  Q_OBJECT

public:
  // Types
  enum class PadType { THT, SMT };

  // Constructors / Destructor
  BoardEditorState_AddPad() = delete;
  BoardEditorState_AddPad(const BoardEditorState_AddPad& other) = delete;
  explicit BoardEditorState_AddPad(const Context& context, PadType type,
                                   Pad::Function function) noexcept;
  virtual ~BoardEditorState_AddPad() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processRotate(const Angle& rotation) noexcept override;

  // Connection to UI
  PadType getType() const noexcept { return mPadType; }
  QVector<std::pair<Uuid, QString>> getAvailableNets() const noexcept;
  std::optional<Uuid> getNet() const noexcept { return mCurrentNetSignal; }
  void setNet(const std::optional<Uuid>& net) noexcept;
  Pad::ComponentSide getComponentSide() const noexcept {
    return mCurrentProperties.getComponentSide();
  }
  void setComponentSide(Pad::ComponentSide side) noexcept;
  Pad::Shape getShape() const noexcept { return mCurrentProperties.getShape(); }
  void setShape(Pad::Shape shape) noexcept;
  const PositiveLength& getWidth() const noexcept {
    return mCurrentProperties.getWidth();
  }
  void setWidth(const PositiveLength& width) noexcept;
  const PositiveLength& getHeight() const noexcept {
    return mCurrentProperties.getHeight();
  }
  void setHeight(const PositiveLength& height) noexcept;
  const UnsignedLimitedRatio& getRadius() const noexcept {
    return mCurrentProperties.getRadius();
  }
  void setRadius(const UnsignedLimitedRatio& radius) noexcept;
  std::optional<PositiveLength> getDrillDiameter() const noexcept {
    if (std::shared_ptr<const PadHole> hole =
            mCurrentProperties.getHoles().value(0)) {
      return hole->getDiameter();
    } else {
      return std::nullopt;
    }
  }
  void setDrillDiameter(const PositiveLength& diameter) noexcept;
  const UnsignedLength& getCopperClearance() const noexcept {
    return mCurrentProperties.getCopperClearance();
  }
  void setCopperClearance(const UnsignedLength& clearance) noexcept;
  const MaskConfig& getStopMaskConfig() const noexcept {
    return mCurrentProperties.getStopMaskConfig();
  }
  void setStopMaskConfig(const MaskConfig& cfg) noexcept;
  Pad::Function getFunction() const noexcept {
    return mCurrentProperties.getFunction();
  }
  bool getFunctionIsFiducial() const noexcept {
    return mCurrentProperties.getFunctionIsFiducial();
  }
  void setFunction(Pad::Function function) noexcept;

  // Operator Overloadings
  BoardEditorState_AddPad& operator=(const BoardEditorState_AddPad& rhs) =
      delete;

signals:
  void netChanged(const std::optional<Uuid>& net);
  void componentSideChanged(Pad::ComponentSide side);
  void shapeChanged(Pad::Shape shape);
  void widthChanged(const PositiveLength& width);
  void heightChanged(const PositiveLength& height);
  void radiusChanged(const UnsignedLimitedRatio& radius);
  void drillDiameterChanged(const PositiveLength& diameter);
  void copperClearanceChanged(const UnsignedLength& clearance);
  void stopMaskConfigChanged(const MaskConfig& cfg);
  void functionChanged(Pad::Function function);

private:  // Methods
  bool start(const Point& pos) noexcept;
  bool updatePosition(BoardGraphicsScene& scene, const Point& pos) noexcept;
  void setNetSignal(NetSignal* netsignal) noexcept;
  bool finish(const Point& pos) noexcept;
  bool abortCommand(bool showErrMsgBox) noexcept;
  void applySelectedNetSignal() noexcept;
  NetSignal* getCurrentNetSignal() const noexcept;
  void applyRecommendedRoundedRectRadius() noexcept;
  void makePadLayerVisible() noexcept;

private:  // Data
  const PadType mPadType;

  // State
  bool mIsUndoCmdActive;

  // Current tool settings
  BoardPadData mCurrentProperties;

  /// The current net signal of the via
  std::optional<Uuid> mCurrentNetSignal;

  // Information about the current pad to place. Only valid if
  // mIsUndoCmdActive == true.
  BI_Pad* mCurrentPad;
  std::unique_ptr<CmdBoardPadEdit> mCurrentEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
