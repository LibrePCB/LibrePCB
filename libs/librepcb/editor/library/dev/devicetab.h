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

#ifndef LIBREPCB_EDITOR_DEVICETAB_H
#define LIBREPCB_EDITOR_DEVICETAB_H

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

class Device;

namespace editor {

class LibraryEditor2;

/*******************************************************************************
 *  Class DeviceTab
 ******************************************************************************/

/**
 * @brief The DeviceTab class
 */
class DeviceTab final : public WindowTab {
  Q_OBJECT

public:
  // Signals
  Signal<DeviceTab> onDerivedUiDataChanged;

  // Constructors / Destructor
  DeviceTab() = delete;
  DeviceTab(const DeviceTab& other) = delete;
  explicit DeviceTab(LibraryEditor2& editor, std::unique_ptr<Device> dev,
                     bool wizardMode, QObject* parent = nullptr) noexcept;
  ~DeviceTab() noexcept;

  // General Methods
  FilePath getDirectoryPath() const noexcept;
  ui::TabData getUiData() const noexcept override;
  ui::DeviceTabData getDerivedUiData() const noexcept;
  void setDerivedUiData(const ui::DeviceTabData& data) noexcept;
  void trigger(ui::TabAction a) noexcept override;

  // Operator Overloadings
  DeviceTab& operator=(const DeviceTab& rhs) = delete;

private:
  LibraryEditor2& mEditor;
  std::unique_ptr<Device> mDevice;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
