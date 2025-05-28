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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "devicetab.h"

#include "../libraryeditor2.h"
#include "utils/slinthelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DeviceTab::DeviceTab(GuiApplication& app, LibraryEditor2& editor,
                     const FilePath& fp, QObject* parent) noexcept
  : WindowTab(app, parent),
    onDerivedUiDataChanged(*this),
    mEditor(editor),
    mDirPath(fp) {
  // Connect library editor.
  connect(&mEditor, &LibraryEditor2::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mEditor, &LibraryEditor2::aboutToBeDestroyed, this,
          &DeviceTab::closeEnforced);
}

DeviceTab::~DeviceTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

ui::TabData DeviceTab::getUiData() const noexcept {
  return ui::TabData{
      ui::TabType::Device,  // Type
      q2s(QString("TODO")),  // Title
      ui::TabFeatures{},  // Features
      slint::SharedString(),  // Undo text
      slint::SharedString(),  // Redo text
      slint::SharedString(),  // Find term
      nullptr,  // Find suggestions
      nullptr,  // Layers
  };
}

ui::DeviceTabData DeviceTab::getDerivedUiData() const noexcept {
  return ui::DeviceTabData{
      mEditor.getUiIndex(),  // Library index
      q2s(mDirPath.toStr()),  // Path
      slint::SharedString(),  // Name
      slint::SharedString(),  // Description
      slint::SharedString(),  // Keywords
      slint::SharedString(),  // Author
      slint::SharedString(),  // Version
      false,  // Deprecated
      nullptr,  // Categories
      slint::SharedString(),  // Package name
      slint::SharedString(),  // Component name
      nullptr,  // Resources
      nullptr,  // Attributes
      nullptr,  // Parts
      nullptr,  // Pinout
  };
}

void DeviceTab::setDerivedUiData(const ui::DeviceTabData& data) noexcept {
}

void DeviceTab::trigger(ui::TabAction a) noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
