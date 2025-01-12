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

#ifndef LIBREPCB_CREATELIBRARYTAB_H
#define LIBREPCB_CREATELIBRARYTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "windowtab.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

class GuiApplication;

/*******************************************************************************
 *  Class CreateLibraryTab
 ******************************************************************************/

/**
 * @brief The CreateLibraryTab class
 */
class CreateLibraryTab final : public WindowTab {
  Q_OBJECT

public:
  // Constructors / Destructor
  CreateLibraryTab() = delete;
  CreateLibraryTab(const CreateLibraryTab& other) = delete;
  explicit CreateLibraryTab(GuiApplication& app,
                            QObject* parent = nullptr) noexcept;
  virtual ~CreateLibraryTab() noexcept;

  // General Methods
  const ui::CreateLibraryTabData& getUiData() const noexcept { return mUiData; }
  void setUiData(const ui::CreateLibraryTabData& data) noexcept;
  void activate() noexcept override;
  void deactivate() noexcept override;

  // Operator Overloadings
  CreateLibraryTab& operator=(const CreateLibraryTab& rhs) = delete;

private:
  ui::CreateLibraryTabData mUiData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
