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
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class PackagePad;

namespace editor {

class CmdFootprintPadEdit;
class FootprintPadGraphicsItem;
class PackagePadComboBox;

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
  QSet<EditorWidgetBase::Feature> getAvailableFeatures()
      const noexcept override;

  // Event Handlers
  virtual bool processGraphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processGraphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  virtual bool processRotate(const Angle& rotation) noexcept override;

  // Operator Overloadings
  PackageEditorState_AddPads& operator=(const PackageEditorState_AddPads& rhs) =
      delete;

private:  // Methods
  bool startAddPad(const Point& pos) noexcept;
  bool finishAddPad(const Point& pos) noexcept;
  bool abortAddPad() noexcept;
  void selectNextFreePadInComboBox() noexcept;
  void packagePadComboBoxCurrentPadChanged(std::optional<Uuid> pad) noexcept;
  void boardSideSelectorCurrentSideChanged(
      FootprintPad::ComponentSide side) noexcept;
  void shapeSelectorCurrentShapeChanged(FootprintPad::Shape shape,
                                        const UnsignedLimitedRatio& radius,
                                        bool customRadius) noexcept;
  void widthEditValueChanged(const PositiveLength& value) noexcept;
  void heightEditValueChanged(const PositiveLength& value) noexcept;
  void drillDiameterEditValueChanged(const PositiveLength& value) noexcept;
  void fiducialClearanceEditValueChanged(const UnsignedLength& value) noexcept;
  void radiusEditValueChanged(const UnsignedLimitedRatio& value) noexcept;
  void pressFitCheckedChanged(bool value) noexcept;
  void applyRecommendedRoundedRectRadius() noexcept;

signals:
  void requestRadiusInputEnabled(bool enabled);
  void requestRadius(const UnsignedLimitedRatio& radius);

private:  // Types / Data
  PadType mPadType;
  std::unique_ptr<CmdFootprintPadEdit> mEditCmd;
  std::shared_ptr<FootprintPad> mCurrentPad;
  std::shared_ptr<FootprintPadGraphicsItem> mCurrentGraphicsItem;
  PackagePadComboBox* mPackagePadComboBox;

  // parameter memory
  FootprintPad mLastPad;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
