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

#ifndef LIBREPCB_EDITOR_BOARDCLIPBOARDDATABUILDER_H
#define LIBREPCB_EDITOR_BOARDCLIPBOARDDATABUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include "boardclipboarddata.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class BoardGraphicsScene;

/*******************************************************************************
 *  Class BoardClipboardDataBuilder
 ******************************************************************************/

/**
 * @brief The BoardClipboardDataBuilder class
 */
class BoardClipboardDataBuilder final {
public:
  // Constructors / Destructor
  BoardClipboardDataBuilder() = delete;
  BoardClipboardDataBuilder(const BoardClipboardDataBuilder& other) = delete;
  explicit BoardClipboardDataBuilder(BoardGraphicsScene& scene) noexcept;
  ~BoardClipboardDataBuilder() noexcept;

  // General Methods
  std::unique_ptr<BoardClipboardData> generate(
      const Point& cursorPos) const noexcept;

  // Operator Overloadings
  BoardClipboardDataBuilder& operator=(const BoardClipboardDataBuilder& rhs) =
      delete;

private:  // Data
  BoardGraphicsScene& mScene;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
