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

#ifndef LIBREPCB_CORE_THEME_H
#define LIBREPCB_CORE_THEME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/sexpression.h"
#include "../types/uuid.h"
#include "themecolor.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ColorRole;

/*******************************************************************************
 *  Class Theme
 ******************************************************************************/

/**
 * @brief Theme class as used by ::librepcb::WorkspaceSettingsItem_Themes
 */
class Theme final {
  Q_DECLARE_TR_FUNCTIONS(Theme)

public:
  // Constructors / Destructor
  explicit Theme(const Uuid& uuid = Uuid::createRandom(),
                 const QString& name = "Unnamed") noexcept;
  Theme(const Uuid& uuid, const QString& name, const Theme& copyFrom) noexcept;
  Theme(const Theme& other) noexcept;
  ~Theme() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const QString& getName() const noexcept { return mName; }
  const QList<ThemeColor>& getColors() const noexcept { return mColors; }
  const ThemeColor& getColor(const ColorRole& role) const noexcept;
  const ThemeColor& getColor(const QString& role) const noexcept;

  // Setters
  void setName(const QString& name) noexcept;
  void setColors(const QList<ThemeColor>& colors) noexcept;

  // General Methods
  void restoreDefaults() noexcept;
  void load(const SExpression& root);
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Theme& rhs) const noexcept;
  bool operator!=(const Theme& rhs) const noexcept { return !(*this == rhs); }
  Theme& operator=(const Theme& rhs) noexcept;

private:  // Methods
  void addColor(const ColorRole& role, const char* category,
                const QColor& primary, const QColor& secondary) noexcept;
  SExpression& addNode(const QString& name) noexcept;

private:  // Data
  QMap<QString, SExpression> mNodes;

  Uuid mUuid;
  QString mName;
  QList<ThemeColor> mColors;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
