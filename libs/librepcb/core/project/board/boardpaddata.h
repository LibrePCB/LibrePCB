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

#ifndef LIBREPCB_CORE_BOARDPADDATA_H
#define LIBREPCB_CORE_BOARDPADDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/pad.h"
#include "../../serialization/serializableobjectlist.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BoardPadData
 ******************************************************************************/

/**
 * @brief The BoardPadData class represents a pad in a board
 */
class BoardPadData final : public Pad {
  Q_DECLARE_TR_FUNCTIONS(BoardPadData)

public:
  // Signals
  enum class Event {
    // Base class properties
    UuidChanged,
    PositionChanged,
    RotationChanged,
    ShapeChanged,
    WidthChanged,
    HeightChanged,
    RadiusChanged,
    CustomShapeOutlineChanged,
    StopMaskConfigChanged,
    SolderPasteConfigChanged,
    CopperClearanceChanged,
    ComponentSideChanged,
    FunctionChanged,
    HolesEdited,
    // Derived class properties
    LockedChanged,
  };
  Signal<BoardPadData, Event> onEdited;
  typedef Slot<BoardPadData, Event> OnEditedSlot;

  // Constructors / Destructor
  BoardPadData() = delete;
  BoardPadData(const BoardPadData& other) noexcept;
  BoardPadData(const Uuid& uuid, const BoardPadData& other) noexcept;
  BoardPadData(const Uuid& uuid, const Point& pos, const Angle& rot,
               Shape shape, const PositiveLength& width,
               const PositiveLength& height, const UnsignedLimitedRatio& radius,
               const Path& customShapeOutline, const MaskConfig& autoStopMask,
               const MaskConfig& autoSolderPaste,
               const UnsignedLength& copperClearance, ComponentSide side,
               Function function, const PadHoleList& holes,
               bool locked) noexcept;
  explicit BoardPadData(const SExpression& node);
  ~BoardPadData() noexcept;

  // Getters
  using Pad::getHoles;
  PadHoleList& getHoles() noexcept { return mHoles; }
  bool isLocked() const noexcept { return mLocked; }

  // Setters
  bool setPosition(const Point& pos) noexcept;
  bool setRotation(const Angle& rot) noexcept;
  bool setShape(Shape shape) noexcept;
  bool setWidth(const PositiveLength& width) noexcept;
  bool setHeight(const PositiveLength& height) noexcept;
  bool setRadius(const UnsignedLimitedRatio& radius) noexcept;
  bool setCustomShapeOutline(const Path& outline) noexcept;
  bool setStopMaskConfig(const MaskConfig& config) noexcept;
  bool setSolderPasteConfig(const MaskConfig& config) noexcept;
  bool setCopperClearance(const UnsignedLength& clearance) noexcept;
  bool setComponentSide(ComponentSide side) noexcept;
  bool setFunction(Function function) noexcept;
  bool setLocked(bool locked) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const BoardPadData& rhs) const noexcept;
  bool operator!=(const BoardPadData& rhs) const noexcept {
    return !(*this == rhs);
  }
  BoardPadData& operator=(const BoardPadData& rhs) noexcept;

private:  // Methods
  void holesEdited(const PadHoleList& list, int index,
                   const std::shared_ptr<const PadHole>& hole,
                   PadHoleList::Event event) noexcept;

private:  // Data
  bool mLocked;

  // Slots
  PadHoleList::OnEditedSlot mHolesEditedSlot;
};

/*******************************************************************************
 *  Class BoardPadDataList
 ******************************************************************************/

struct BoardPadDataListNameProvider {
  static constexpr const char* tagname = "pad";
};
using BoardPadDataList =
    SerializableObjectList<BoardPadData, BoardPadDataListNameProvider,
                           BoardPadData::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
