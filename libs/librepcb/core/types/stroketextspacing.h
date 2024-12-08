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

#ifndef LIBREPCB_CORE_STROKETEXTSPACING_H
#define LIBREPCB_CORE_STROKETEXTSPACING_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "ratio.h"

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class StrokeTextSpacing
 ******************************************************************************/

/**
 * @brief Represents the letter- or line spacing configuration of a stroke text
 */
class StrokeTextSpacing final {
  Q_DECLARE_TR_FUNCTIONS(StrokeTextSpacing)

public:
  // Constructors / Destructor
  StrokeTextSpacing(const std::optional<Ratio>& ratio = std::nullopt) noexcept;
  StrokeTextSpacing(const StrokeTextSpacing& other) noexcept;
  ~StrokeTextSpacing() noexcept;

  // Getters
  const std::optional<Ratio>& getRatio() const noexcept { return mRatio; }

  // Operator Overloadings
  bool operator==(const StrokeTextSpacing& rhs) const noexcept;
  bool operator!=(const StrokeTextSpacing& rhs) const noexcept {
    return !(*this == rhs);
  }
  StrokeTextSpacing& operator=(const StrokeTextSpacing& rhs) noexcept;

private:  // Data
  std::optional<Ratio> mRatio;  ///< `nullopt` means automatic (from font)
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
