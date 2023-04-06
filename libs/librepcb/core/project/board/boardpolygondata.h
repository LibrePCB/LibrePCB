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

#ifndef LIBREPCB_CORE_BOARDPOLYGONDATA_H
#define LIBREPCB_CORE_BOARDPOLYGONDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/path.h"
#include "../../types/uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class BoardPolygonData
 ******************************************************************************/

/**
 * @brief The BoardPolygonData class
 */
class BoardPolygonData final {
public:
  // Constructors / Destructor
  BoardPolygonData() = delete;
  BoardPolygonData(const BoardPolygonData& other) noexcept;
  BoardPolygonData(const Uuid& uuid, const BoardPolygonData& other) noexcept;
  BoardPolygonData(const Uuid& uuid, const Layer& layer,
                   const UnsignedLength& lineWidth, const Path& path, bool fill,
                   bool isGrabArea) noexcept;
  explicit BoardPolygonData(const SExpression& node);
  ~BoardPolygonData() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Layer& getLayer() const noexcept { return *mLayer; }
  const UnsignedLength& getLineWidth() const noexcept { return mLineWidth; }
  const Path& getPath() const noexcept { return mPath; }
  bool isFilled() const noexcept { return mIsFilled; }
  bool isGrabArea() const noexcept { return mIsGrabArea; }

  // Setters
  bool setLayer(const Layer& layer) noexcept;
  bool setLineWidth(const UnsignedLength& width) noexcept;
  bool setPath(const Path& path) noexcept;
  bool setIsFilled(bool isFilled) noexcept;
  bool setIsGrabArea(bool isGrabArea) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const BoardPolygonData& rhs) const noexcept;
  bool operator!=(const BoardPolygonData& rhs) const noexcept {
    return !(*this == rhs);
  }
  BoardPolygonData& operator=(const BoardPolygonData& rhs) = default;

private:  // Data
  Uuid mUuid;
  const Layer* mLayer;
  UnsignedLength mLineWidth;
  Path mPath;
  bool mIsFilled;
  bool mIsGrabArea;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
