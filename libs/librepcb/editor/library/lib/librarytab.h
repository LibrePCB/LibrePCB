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

#ifndef LIBREPCB_EDITOR_LIBRARYTAB_H
#define LIBREPCB_EDITOR_LIBRARYTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../windowtab.h"

#include <librepcb/core/types/uuid.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class Library;

namespace editor {

class LibraryEditor2;
class LibraryElementsModel;

/*******************************************************************************
 *  Class LibraryTab
 ******************************************************************************/

/**
 * @brief The LibraryTab class
 */
class LibraryTab final : public WindowTab {
  Q_OBJECT

public:
  // Signals
  Signal<LibraryTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  LibraryTab() = delete;
  LibraryTab(const LibraryTab& other) = delete;
  explicit LibraryTab(GuiApplication& app, LibraryEditor2& editor,
                      QObject* parent = nullptr) noexcept;
  ~LibraryTab() noexcept;

  // General Methods
  int getLibraryIndex() const noexcept;
  ui::TabData getUiData() const noexcept override;
  ui::LibraryTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::LibraryTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  LibraryTab& operator=(const LibraryTab& rhs) = delete;

private:
  LibraryEditor2& mEditor;
  Library& mLibrary;
  std::shared_ptr<LibraryElementsModel> mElementsModel;
  int mIndex = 1;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
