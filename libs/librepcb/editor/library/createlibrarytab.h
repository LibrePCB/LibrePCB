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

#ifndef LIBREPCB_EDITOR_CREATELIBRARYTAB_H
#define LIBREPCB_EDITOR_CREATELIBRARYTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "windowtab.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/version.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CreateLibraryTab
 ******************************************************************************/

/**
 * @brief The CreateLibraryTab class
 */
class CreateLibraryTab final : public WindowTab {
  Q_OBJECT

public:
  // Signals
  Signal<CreateLibraryTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  CreateLibraryTab() = delete;
  CreateLibraryTab(const CreateLibraryTab& other) = delete;
  explicit CreateLibraryTab(GuiApplication& app,
                            QObject* parent = nullptr) noexcept;
  ~CreateLibraryTab() noexcept;

  // General Methods
  ui::TabData getUiData() const noexcept override;
  const ui::CreateLibraryTabData& getDerivedUiData() const noexcept {
    return mUiData;
  }
  void setDerivedUiData(const ui::CreateLibraryTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  CreateLibraryTab& operator=(const CreateLibraryTab& rhs) = delete;

private:
  void validate() noexcept;

  ui::CreateLibraryTabData mUiData;
  std::optional<ElementName> mName;
  std::optional<Version> mVersion;
  std::optional<QUrl> mUrl;
  FilePath mDirectory;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
