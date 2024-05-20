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

#ifndef LIBREPCB_CORE_THEMECOLOR_H
#define LIBREPCB_CORE_THEMECOLOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/sexpression.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ThemeColor
 ******************************************************************************/

/**
 * @brief Color settings as used by ::librepcb::Theme
 */
class ThemeColor final {
public:
  // Constructors / Destructor
  ThemeColor() = delete;
  ThemeColor(const QString& identifier, const char* category, const char* name,
             const QString& nameSuffix, const QColor& primary,
             const QColor& secondary) noexcept;
  ThemeColor(const ThemeColor& other) noexcept;
  ~ThemeColor() noexcept;

  // Getters
  const QString& getIdentifier() const noexcept { return mIdentifier; }
  QString getCategoryTr() const noexcept;
  QString getNameTr() const noexcept;
  const QColor& getPrimaryColor() const noexcept { return mPrimary; }
  const QColor& getSecondaryColor() const noexcept { return mSecondary; }
  bool isEdited() const noexcept { return mEdited; }

  // Setters
  void setPrimaryColor(const QColor& color) noexcept;
  void setSecondaryColor(const QColor& color) noexcept;

  // General Methods
  void load(const SExpression& root);
  std::unique_ptr<SExpression> serialize() const;

  // Operator Overloadings
  bool operator==(const ThemeColor& rhs) const noexcept;
  bool operator!=(const ThemeColor& rhs) const noexcept {
    return !(*this == rhs);
  }
  ThemeColor& operator=(const ThemeColor& rhs) noexcept;

private:  // Data
  QString mIdentifier;
  const char* mCategoryNoTr;
  const char* mNameNoTr;
  QString mNameSuffix;
  QColor mPrimary;
  QColor mSecondary;  ///< Null if not applicable
  bool mEdited;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
