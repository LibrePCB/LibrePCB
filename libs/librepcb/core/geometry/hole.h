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

#ifndef LIBREPCB_CORE_HOLE_H
#define LIBREPCB_CORE_HOLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"
#include "../serialization/serializableobjectlist.h"
#include "../types/length.h"
#include "../types/maskconfig.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Hole
 ******************************************************************************/

/**
 * @brief The Hole class
 */
class Hole final {
  Q_DECLARE_TR_FUNCTIONS(Hole)

public:
  // Signals
  enum class Event {
    UuidChanged,
    DiameterChanged,
    PathChanged,
    StopMaskConfigChanged,
  };
  Signal<Hole, Event> onEdited;
  typedef Slot<Hole, Event> OnEditedSlot;

  // Constructors / Destructor
  Hole() = delete;
  Hole(const Hole& other) noexcept;
  Hole(const Uuid& uuid, const Hole& other) noexcept;
  Hole(const Uuid& uuid, const PositiveLength& diameter,
       const NonEmptyPath& path, const MaskConfig& stopMaskConfig) noexcept;
  explicit Hole(const SExpression& node);
  ~Hole() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const PositiveLength& getDiameter() const noexcept { return mDiameter; }
  const NonEmptyPath& getPath() const noexcept { return mPath; }
  const MaskConfig& getStopMaskConfig() const noexcept {
    return mStopMaskConfig;
  }
  bool isSlot() const noexcept;
  bool isMultiSegmentSlot() const noexcept;
  bool isCurvedSlot() const noexcept;
  tl::optional<Length> getPreviewStopMaskOffset() const noexcept;

  // Setters
  bool setDiameter(const PositiveLength& diameter) noexcept;
  bool setPath(const NonEmptyPath& path) noexcept;
  bool setStopMaskConfig(const MaskConfig& config) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Hole& rhs) const noexcept;
  bool operator!=(const Hole& rhs) const noexcept { return !(*this == rhs); }
  Hole& operator=(const Hole& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  PositiveLength mDiameter;
  NonEmptyPath mPath;
  MaskConfig mStopMaskConfig;
};

/*******************************************************************************
 *  Class HoleList
 ******************************************************************************/

struct HoleListNameProvider {
  static constexpr const char* tagname = "hole";
};
using HoleList =
    SerializableObjectList<Hole, HoleListNameProvider, Hole::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
