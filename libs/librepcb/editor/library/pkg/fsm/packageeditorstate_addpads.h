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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORSTATE_ADDPADS_H
#define LIBREPCB_EDITOR_PACKAGEEDITORSTATE_ADDPADS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "packageeditorstate.h"

#include <librepcb/core/library/pkg/footprintpad.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class CmdFootprintPadEdit;
class FootprintPadGraphicsItem;

/*******************************************************************************
 *  Class PackageEditorState_AddPads
 ******************************************************************************/

/**
 * @brief The PackageEditorState_AddPads class
 */
class PackageEditorState_AddPads : public PackageEditorState {
  Q_OBJECT

public:
  // Types
  enum class PadType { THT, SMT };

  // Constructors / Destructor
  PackageEditorState_AddPads() = delete;
  PackageEditorState_AddPads(const PackageEditorState_AddPads& other) = delete;
  explicit PackageEditorState_AddPads(Context& context, PadType type,
                                      FootprintPad::Function function) noexcept;
  virtual ~PackageEditorState_AddPads() noexcept;

  // General Methods
  virtual bool entry() noexcept override;
  virtual bool exit() noexcept override;

  // Event Handlers
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processRotate(const Angle& rotation) noexcept override;

  // Connection to UI
  PadType getType() const noexcept { return mPadType; }
  const std::optional<Uuid>& getPackagePad() const noexcept {
    return mCurrentProperties.getPackagePadUuid();
  }
  void setPackagePad(const std::optional<Uuid>& pad) noexcept;
  FootprintPad::ComponentSide getComponentSide() const noexcept {
    return mCurrentProperties.getComponentSide();
  }
  void setComponentSide(FootprintPad::ComponentSide side) noexcept;
  FootprintPad::Shape getShape() const noexcept {
    return mCurrentProperties.getShape();
  }
  void setShape(FootprintPad::Shape shape) noexcept;
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
  FootprintPad::Function getFunction() const noexcept {
    return mCurrentProperties.getFunction();
  }
  bool getFunctionIsFiducial() const noexcept {
    return mCurrentProperties.getFunctionIsFiducial();
  }
  void setFunction(FootprintPad::Function function) noexcept;

  // Operator Overloadings
  PackageEditorState_AddPads& operator=(const PackageEditorState_AddPads& rhs) =
      delete;

signals:
  void packagePadChanged(const std::optional<Uuid>& pad);
  void componentSideChanged(FootprintPad::ComponentSide side);
  void shapeChanged(FootprintPad::Shape shape);
  void widthChanged(const PositiveLength& width);
  void heightChanged(const PositiveLength& height);
  void radiusChanged(const UnsignedLimitedRatio& radius);
  void drillDiameterChanged(const PositiveLength& diameter);
  void copperClearanceChanged(const UnsignedLength& clearance);
  void stopMaskConfigChanged(const MaskConfig& cfg);
  void functionChanged(FootprintPad::Function function);

private:  // Methods
  bool startAddPad(const Point& pos) noexcept;
  bool finishAddPad(const Point& pos) noexcept;
  bool abortAddPad() noexcept;
  void selectNextFreePackagePad() noexcept;
  void applyRecommendedRoundedRectRadius() noexcept;

private:  // Types / Data
  const PadType mPadType;

  FootprintPad mCurrentProperties;

  std::shared_ptr<FootprintPad> mCurrentPad;
  std::shared_ptr<FootprintPadGraphicsItem> mCurrentGraphicsItem;
  std::unique_ptr<CmdFootprintPadEdit> mCurrentEditCmd;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
