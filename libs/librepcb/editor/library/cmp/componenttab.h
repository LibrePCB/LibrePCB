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

#ifndef LIBREPCB_EDITOR_COMPONENTTAB_H
#define LIBREPCB_EDITOR_COMPONENTTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../windowtab.h"

#include <librepcb/core/fileio/filepath.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;

namespace editor {

class LibraryEditor2;

/*******************************************************************************
 *  Class ComponentTab
 ******************************************************************************/

/**
 * @brief The ComponentTab class
 */
class ComponentTab final : public WindowTab {
  Q_OBJECT

public:
  // Signals
  Signal<ComponentTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  ComponentTab() = delete;
  ComponentTab(const ComponentTab& other) = delete;
  explicit ComponentTab(LibraryEditor2& editor, std::unique_ptr<Component> cmp,
                        bool wizardMode, QObject* parent = nullptr) noexcept;
  ~ComponentTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept;
  ui::TabData getUiData() const noexcept override;
  ui::ComponentTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::ComponentTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  ComponentTab& operator=(const ComponentTab& rhs) = delete;

private:
  LibraryEditor2& mEditor;
  std::unique_ptr<Component> mComponent;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
