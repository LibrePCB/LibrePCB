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

#ifndef LIBREPCB_EDITOR_HOMETAB_H
#define LIBREPCB_EDITOR_HOMETAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "windowtab.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GuiApplication;

/*******************************************************************************
 *  Class HomeTab
 ******************************************************************************/

/**
 * @brief The HomeTab class
 */
class HomeTab final : public WindowTab {
  Q_OBJECT

public:
  // Constructors / Destructor
  HomeTab() = delete;
  HomeTab(const HomeTab& other) = delete;
  explicit HomeTab(GuiApplication& app, QObject* parent = nullptr) noexcept;
  ~HomeTab() noexcept;

  // General Methods
  ui::TabData getUiData() const noexcept override;

  // Operator Overloadings
  HomeTab& operator=(const HomeTab& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
