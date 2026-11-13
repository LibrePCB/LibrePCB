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

#ifndef LIBREPCB_EDITOR_CLIENTSETTINGS_H
#define LIBREPCB_EDITOR_CLIENTSETTINGS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <initializer_list>
#include <librepcb/core/serialization/serialization.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/enums.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

namespace editor {

/*******************************************************************************
 *  Class ClientSettings
 ******************************************************************************/

/**
 * @brief Wrapper around `QSettings` providing LibrePCB type conversions
 *
 * Thin wrapper around `QSettings` which allows to load/store values with
 * LibrePCB value types in addition to the plain Qt types.
 */
class ClientSettings final {
public:
  // Constructors / Destructor
  ClientSettings() noexcept;
  ClientSettings(const ClientSettings& other) = delete;

  /**
   * @brief Constructor with groups
   *
   * @param groups  Groups to open with `QSettings::beginGroup()`, e.g.
   *                `ClientSettings settings({"group1", "group2"})`.
   */
  ClientSettings(std::initializer_list<QAnyStringView> groups) noexcept;

  ~ClientSettings() noexcept;

  // General Methods

  /**
   * @brief Set a value of a type implementing serializeToString()
   *
   * The value is converted to a string with ::librepcb::serializeToString()
   * before storing it, i.e. the type must implement this function.
   *
   * @param key    Key to store the value at.
   * @param value  Value to store.
   */
  template <typename T>
  void set(QAnyStringView key, const T& value) noexcept {
    mSettings.setValue(key, serializeToString(value));
  }

  /**
   * @brief Get an angle value, with optional validity checks
   *
   * @param key     Key to load the value from.
   * @param min     Minimum allowed value (no check if nullopt).
   * @param max     Maximum allowed value (no check if nullopt).
   * @param modulo  The value must be a multiple of this (no check if nullopt).
   *
   * @return The angle, or std::nullopt if the key does not exist, contains an
   *         invalid value or does not pass the validity checks.
   */
  std::optional<Angle> get(QAnyStringView key, const std::optional<Angle>& min,
                           const std::optional<Angle>& max,
                           const std::optional<Angle>& modulo) const noexcept;

  /**
   * @brief Get a layer value, with optional validity check
   *
   * @param key       Key to load the value from.
   * @param validSet  Set of allowed layers (no check if nullopt).
   *
   * @return The layer, or std::nullopt if the key does not exist, contains an
   *         unknown layer or the layer is not within the valid set.
   */
  std::optional<const Layer*> get(
      QAnyStringView key,
      const std::optional<QSet<const Layer*>>& validSet) const noexcept;

  /**
   * @brief Get a value of a type implementing deserialize()
   *
   * The value is converted from a string with ::librepcb::deserialize(),
   * i.e. the type must implement this function.
   *
   * @param key  Key to load the value from.
   *
   * @return The deserialized value, or std::nullopt if the key does not exist
   *         or contains an invalid value.
   */
  template <typename T>
  std::optional<T> get(QAnyStringView key) const noexcept {
    const QVariant value = mSettings.value(key);
    if (!value.isValid()) {
      return std::nullopt;  // Key does not exist, no need to parse it.
    }
    return deserialize<T>(value.toString());
  }

  // Operator Overloadings
  ClientSettings& operator=(const ClientSettings& rhs) = delete;

private:
  QSettings mSettings;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
