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

#ifndef LIBREPCB_GUIAPPLICATION_H
#define LIBREPCB_GUIAPPLICATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {
namespace app {

class LibrariesModel;
class MainWindow;

/*******************************************************************************
 *  Class GuiApplication
 ******************************************************************************/

/**
 * @brief The GuiApplication class
 */
class GuiApplication : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  GuiApplication() = delete;
  GuiApplication(const GuiApplication& other) = delete;
  explicit GuiApplication(Workspace& ws, QObject* parent = nullptr) noexcept;
  virtual ~GuiApplication() noexcept;

  // Getters
  Workspace& getWorkspace() noexcept { return mWorkspace; }
  std::shared_ptr<LibrariesModel> getLibraries() noexcept { return mLibraries; }
  std::shared_ptr<slint::FilterModel<ui::Library>>
      getInstalledLibraries() noexcept {
    return mInstalledLibraries;
  }
  std::shared_ptr<slint::FilterModel<ui::Library>>
      getAvailableLibraries() noexcept {
    return mAvailableLibraries;
  }

  // General Methods
  std::shared_ptr<MainWindow> newWindow() noexcept;
  void exec();

  // Operator Overloadings
  GuiApplication& operator=(const GuiApplication& rhs) = delete;

private:
  Workspace& mWorkspace;
  std::shared_ptr<LibrariesModel> mLibraries;
  std::shared_ptr<slint::FilterModel<ui::Library>> mInstalledLibraries;
  std::shared_ptr<slint::FilterModel<ui::Library>> mAvailableLibraries;
  QList<std::shared_ptr<MainWindow>> mWindows;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
