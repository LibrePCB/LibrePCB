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

#ifndef LIBREPCB_CORE_BOARDZONEDATA_H
#define LIBREPCB_CORE_BOARDZONEDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/zone.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class BoardZoneData
 ******************************************************************************/

/**
 * @brief The BoardZoneData class
 */
class BoardZoneData final {
public:
  // Constructors / Destructor
  BoardZoneData() = delete;
  BoardZoneData(const BoardZoneData& other) noexcept;
  BoardZoneData(const Uuid& uuid, const BoardZoneData& other) noexcept;
  BoardZoneData(const Uuid& uuid, const QSet<const Layer*>& layers,
                Zone::Rules rules, const Path& outline, bool locked) noexcept;
  explicit BoardZoneData(const SExpression& node);
  ~BoardZoneData() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const QSet<const Layer*>& getLayers() const noexcept { return mLayers; }
  Zone::Rules getRules() const noexcept { return mRules; }
  const Path& getOutline() const noexcept { return mOutline; }
  bool isLocked() const noexcept { return mLocked; }

  // Setters
  bool setLayers(const QSet<const Layer*>& layers);
  bool setRules(Zone::Rules rules) noexcept;
  bool setOutline(const Path& outline) noexcept;
  bool setLocked(bool locked) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const BoardZoneData& rhs) const noexcept;
  bool operator!=(const BoardZoneData& rhs) const noexcept {
    return !(*this == rhs);
  }
  BoardZoneData& operator=(const BoardZoneData& rhs) = default;

private:  // Methods
  void checkLayers(const QSet<const Layer*>& layers);

private:  // Data
  Uuid mUuid;
  QSet<const Layer*> mLayers;
  Zone::Rules mRules;
  Path mOutline;
  bool mLocked;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
