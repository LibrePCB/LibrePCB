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

#ifndef LIBREPCB_CORE_MASKCONFIG_H
#define LIBREPCB_CORE_MASKCONFIG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "length.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class MaskConfig
 ******************************************************************************/

/**
 * @brief The MaskConfig class defines how to add automatic stop mask or
 *        solder paste
 */
class MaskConfig final {
  Q_DECLARE_TR_FUNCTIONS(MaskConfig)

public:
  // Constructors / Destructor
  MaskConfig() = delete;
  MaskConfig(const MaskConfig& other) noexcept;
  ~MaskConfig() noexcept;

  // Getters
  bool isEnabled() const noexcept { return mEnabled; }
  const tl::optional<Length>& getOffset() const noexcept { return mOffset; }

  // Operator Overloadings
  bool operator==(const MaskConfig& rhs) const noexcept;
  bool operator!=(const MaskConfig& rhs) const noexcept {
    return !(*this == rhs);
  }
  MaskConfig& operator=(const MaskConfig& rhs) noexcept;

  // Static Methods
  static MaskConfig off() noexcept { return MaskConfig(false, tl::nullopt); }
  static MaskConfig automatic() noexcept {
    return MaskConfig(true, tl::nullopt);
  }
  static MaskConfig manual(const Length& offset) noexcept {
    return MaskConfig(true, offset);
  }
  static MaskConfig maybe(const tl::optional<Length>& offset) noexcept {
    return MaskConfig(offset.has_value(), offset);
  }

private:  // Methods
  MaskConfig(bool enabled, const tl::optional<Length>& offset) noexcept;

private:  // Data
  bool mEnabled;  ///< Whether an automatic mask is added or not
  tl::optional<Length> mOffset;  ///< `nullopt` means "from design rules"
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
