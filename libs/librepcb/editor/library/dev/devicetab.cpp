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

#include <librepcb/core/library/dev/device.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DeviceTab::DeviceTab(LibraryEditor2& editor, std::unique_ptr<Device> dev,
                     bool wizardMode, QObject* parent) noexcept
  : WindowTab(editor.getApp(), parent),
    onDerivedUiDataChanged(*this),
    mEditor(editor),
    mDevice(std::move(dev)) {
  // Connect library editor.
  mEditor.registerTab(*this);
  connect(&mEditor, &LibraryEditor2::uiIndexChanged, this,
          [this]() { onDerivedUiDataChanged.notify(); });
  connect(&mEditor, &LibraryEditor2::aboutToBeDestroyed, this,
          &DeviceTab::closeEnforced);
}

DeviceTab::~DeviceTab() noexcept {
  mEditor.unregisterTab(*this);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

FilePath DeviceTab::getDirectoryPath() const noexcept {
  return mDevice->getDirectory().getAbsPath();
}

ui::TabData DeviceTab::getUiData() const noexcept {
  return ui::TabData{
      ui::TabType::Device,  // Type
      q2s(QString("TODO")),  // Title
      ui::TabFeatures{},  // Features
      false,  // Read-only
      false,  // Unsaved changes
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
      q2s(mDevice->getDirectory().getAbsPath().toStr()),  // Path
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
      ui::RuleCheckData{
          ui::RuleCheckType::None,
          ui::RuleCheckState::NotRunYet,
          nullptr,
          0,
          0,
          slint::SharedString(),
          true,
      },
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
