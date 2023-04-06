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

#ifndef LIBREPCB_CORE_BOARDHOLEDATA_H
#define LIBREPCB_CORE_BOARDHOLEDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/path.h"
#include "../../types/length.h"
#include "../../types/maskconfig.h"
#include "../../types/uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BoardHoleData
 ******************************************************************************/

/**
 * @brief The BoardHoleData class
 */
class BoardHoleData final {
public:
  // Constructors / Destructor
  BoardHoleData() = delete;
  BoardHoleData(const BoardHoleData& other) noexcept;
  BoardHoleData(const Uuid& uuid, const BoardHoleData& other) noexcept;
  BoardHoleData(const Uuid& uuid, const PositiveLength& diameter,
                const NonEmptyPath& path, const MaskConfig& stopMaskConfig);
  explicit BoardHoleData(const SExpression& node);
  ~BoardHoleData() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const PositiveLength& getDiameter() const noexcept { return mDiameter; }
  const NonEmptyPath& getPath() const noexcept { return mPath; }
  const MaskConfig& getStopMaskConfig() const noexcept {
    return mStopMaskConfig;
  }
  bool isSlot() const noexcept { return mPath->getVertices().count() > 1; }
  bool isMultiSegmentSlot() const noexcept {
    return mPath->getVertices().count() > 2;
  }
  bool isCurvedSlot() const noexcept { return mPath->isCurved(); }

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
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
  bool operator==(const BoardHoleData& rhs) const noexcept;
  bool operator!=(const BoardHoleData& rhs) const noexcept {
    return !(*this == rhs);
  }
  BoardHoleData& operator=(const BoardHoleData& rhs) = default;

private:  // Data
  Uuid mUuid;
  PositiveLength mDiameter;
  NonEmptyPath mPath;
  MaskConfig mStopMaskConfig;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
