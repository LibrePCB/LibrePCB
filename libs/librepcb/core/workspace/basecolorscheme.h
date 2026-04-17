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

#ifndef LIBREPCB_CORE_BASECOLORSCHEME_H
#define LIBREPCB_CORE_BASECOLORSCHEME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/uuid.h"

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ColorRole;

/*******************************************************************************
 *  Interface ColorScheme
 ******************************************************************************/

class ColorScheme {
public:
  struct Colors {
    QColor primary;
    QColor secondary;
    const ColorRole* role = nullptr;
  };

  virtual const Uuid& getUuid() const noexcept = 0;
  virtual const QString& getName() const noexcept = 0;
  virtual Colors getColors(const ColorRole& role) const noexcept;
  virtual Colors getColors(const QString& role) const noexcept;
  virtual std::optional<Colors> tryGetColors(
      const ColorRole& role) const noexcept;
  virtual std::optional<Colors> tryGetColors(
      const QString& role) const noexcept = 0;
};

/*******************************************************************************
 *  Class BaseColorScheme
 ******************************************************************************/

/**
 * @brief Base (i.e. built-in) color schemes as used by ::librepcb::ColorScheme
 *
 * @attention Do not delete built-in color schemes since user-defined color
 *            schemes may reference them! Any deletion must be done only
 *            during a file format upgrade, with a proper migration routine.
 */
class BaseColorScheme final : public ColorScheme {
public:
  // Constructors / Destructor
  BaseColorScheme() = delete;
  BaseColorScheme(const BaseColorScheme& other) = delete;
  ~BaseColorScheme() noexcept;

  // General Methods
  const Uuid& getUuid() const noexcept override { return mUuid; }
  const QString& getName() const noexcept override { return mName; }
  const QVector<Colors>& getAllColors() const noexcept { return mColors; }
  std::optional<Colors> tryGetColors(
      const QString& role) const noexcept override;

  // Operator Overloadings
  BaseColorScheme& operator=(const BaseColorScheme& rhs) = delete;

  // Schematic Color Schemes
  static const BaseColorScheme& schematicLibrePcbLight() noexcept;
  static const BaseColorScheme& schematicLibrePcbDark() noexcept;
  static const BaseColorScheme& schematicSolarizedDark() noexcept;

  // Board Color Schemes
  static const BaseColorScheme& boardLibrePcbDark() noexcept;

  // 3D View Color Schemes
  static const BaseColorScheme& view3dLibrePcbLight() noexcept;
  static const BaseColorScheme& view3dLibrePcbDark() noexcept;
  static const BaseColorScheme& view3dSolarizedDark() noexcept;

private:  // Methods
  BaseColorScheme(const Uuid& uuid, const QString& name,
                  const QVector<Colors>& colors) noexcept;
  static BaseColorScheme create(
      const char* uuid, const char* name,
      const QVector<BaseColorScheme::Colors>& colors) noexcept;

private:  // Data
  Uuid mUuid;
  QString mName;
  QVector<Colors> mColors;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
