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

#ifndef LIBREPCB_EDITOR_GRAPHICSSCENECURSOR_H
#define LIBREPCB_EDITOR_GRAPHICSSCENECURSOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/length.h>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Struct GraphicsSceneCursor
 ******************************************************************************/

/**
 * @brief Appearance of the overlay cursor drawn by
 * librepcb::editor::GraphicsScene
 *
 * Describes the visibility of the various scene cursor overlay parts (see
 * GraphicsScene::setSceneCursor()). This includes a crosshair and/or a small
 * "snapped to item" circle, and the radius of an optional clearance circle.
 */
struct GraphicsSceneCursor {
  bool cross = false;
  bool circle = false;

  /// If set, draw a clearance circle with this real-world radius.
  /// `std::nullopt` means no clearance circle is drawn.
  std::optional<UnsignedLength> clearanceRadius = std::nullopt;

  bool operator==(const GraphicsSceneCursor& rhs) const noexcept = default;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
