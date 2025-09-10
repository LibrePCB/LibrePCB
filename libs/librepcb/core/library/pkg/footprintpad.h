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

#ifndef LIBREPCB_CORE_FOOTPRINTPAD_H
#define LIBREPCB_CORE_FOOTPRINTPAD_H

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
 *  Class FootprintPad
 ******************************************************************************/

/**
 * @brief The FootprintPad class represents a pad of a footprint
 */
class FootprintPad final : public Pad {
  Q_DECLARE_TR_FUNCTIONS(FootprintPad)

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
    PackagePadUuidChanged,
  };
  Signal<FootprintPad, Event> onEdited;
  typedef Slot<FootprintPad, Event> OnEditedSlot;

  // Constructors / Destructor
  FootprintPad() = delete;
  FootprintPad(const FootprintPad& other) noexcept;
  FootprintPad(const Uuid& uuid, const FootprintPad& other) noexcept;
  FootprintPad(const Uuid& uuid, const std::optional<Uuid>& pkgPadUuid,
               const Point& pos, const Angle& rot, Shape shape,
               const PositiveLength& width, const PositiveLength& height,
               const UnsignedLimitedRatio& radius,
               const Path& customShapeOutline, const MaskConfig& autoStopMask,
               const MaskConfig& autoSolderPaste,
               const UnsignedLength& copperClearance, ComponentSide side,
               Function function, const PadHoleList& holes) noexcept;
  explicit FootprintPad(const SExpression& node);
  ~FootprintPad() noexcept;

  // Getters
  using Pad::getHoles;
  PadHoleList& getHoles() noexcept { return mHoles; }
  const std::optional<Uuid>& getPackagePadUuid() const noexcept {
    return mPackagePadUuid;
  }

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
  bool setPackagePadUuid(const std::optional<Uuid>& pad) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const FootprintPad& rhs) const noexcept;
  bool operator!=(const FootprintPad& rhs) const noexcept {
    return !(*this == rhs);
  }
  FootprintPad& operator=(const FootprintPad& rhs) noexcept;

private:  // Methods
  void holesEdited(const PadHoleList& list, int index,
                   const std::shared_ptr<const PadHole>& hole,
                   PadHoleList::Event event) noexcept;

private:  // Data
  /// The connected package pad
  ///
  /// This is the UUID of the package pad where this footprint pad is
  /// connected to. It can be std::nullopt, which means that the footprint pad
  /// is electrically not connected (e.g. for mechanical-only pads).
  std::optional<Uuid> mPackagePadUuid;

  // Slots
  PadHoleList::OnEditedSlot mHolesEditedSlot;
};

/*******************************************************************************
 *  Class FootprintPadList
 ******************************************************************************/

struct FootprintPadListNameProvider {
  static constexpr const char* tagname = "pad";
};
using FootprintPadList =
    SerializableObjectList<FootprintPad, FootprintPadListNameProvider,
                           FootprintPad::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
