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

#ifndef LIBREPCB_EDITOR_COLORSCHEMEMODEL_H
#define LIBREPCB_EDITOR_COLORSCHEMEMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "ui.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UserColorScheme;

namespace editor {

/*******************************************************************************
 *  Class ColorSchemeModel
 ******************************************************************************/

/**
 * @brief The ColorSchemeModel class
 */
class ColorSchemeModel final : public slint::Model<ui::ColorSchemeItemData> {
public:
  // Constructors / Destructor
  ColorSchemeModel() = delete;
  ColorSchemeModel(const ColorSchemeModel& other) = delete;
  explicit ColorSchemeModel(std::shared_ptr<UserColorScheme> scheme) noexcept;
  ~ColorSchemeModel() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::ColorSchemeItemData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::ColorSchemeItemData& data) noexcept override;

  // Operator Overloadings
  ColorSchemeModel& operator=(const ColorSchemeModel& rhs) = delete;

private:
  std::shared_ptr<UserColorScheme> mScheme;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
