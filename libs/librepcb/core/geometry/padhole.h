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

#ifndef LIBREPCB_CORE_PADHOLE_H
#define LIBREPCB_CORE_PADHOLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"
#include "../serialization/serializableobjectlist.h"
#include "../types/length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PadHole
 ******************************************************************************/

/**
 * @brief The PadHole class
 */
class PadHole {
  Q_DECLARE_TR_FUNCTIONS(PadHole)

public:
  // Signals
  enum class Event {
    UuidChanged,
    DiameterChanged,
    PathChanged,
  };
  Signal<PadHole, Event> onEdited;
  typedef Slot<PadHole, Event> OnEditedSlot;

  // Constructors / Destructor
  PadHole() = delete;
  PadHole(const PadHole& other) noexcept;
  PadHole(const Uuid& uuid, const PadHole& other) noexcept;
  PadHole(const Uuid& uuid, const PositiveLength& diameter,
          const NonEmptyPath& path) noexcept;
  explicit PadHole(const SExpression& node);
  virtual ~PadHole() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const PositiveLength& getDiameter() const noexcept { return mDiameter; }
  const NonEmptyPath& getPath() const noexcept { return mPath; }
  bool isSlot() const noexcept;
  bool isMultiSegmentSlot() const noexcept;
  bool isCurvedSlot() const noexcept;

  // Setters
  bool setDiameter(const PositiveLength& diameter) noexcept;
  bool setPath(const NonEmptyPath& path) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  virtual void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const PadHole& rhs) const noexcept;
  bool operator!=(const PadHole& rhs) const noexcept { return !(*this == rhs); }
  PadHole& operator=(const PadHole& rhs) noexcept;

protected:  // Data
  Uuid mUuid;
  PositiveLength mDiameter;
  NonEmptyPath mPath;
};

/*******************************************************************************
 *  Class PadHoleList
 ******************************************************************************/

struct PadHoleListNameProvider {
  static constexpr const char* tagname = "hole";
};
using PadHoleList =
    SerializableObjectList<PadHole, PadHoleListNameProvider, PadHole::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
