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

#ifndef LIBREPCB_MAINWINDOW_H
#define LIBREPCB_MAINWINDOW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

class GuiApplication;

/*******************************************************************************
 *  Class MainWindow
 ******************************************************************************/

/**
 * @brief The MainWindow class
 */
class MainWindow : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  MainWindow() = delete;
  MainWindow(const MainWindow& other) = delete;
  explicit MainWindow(GuiApplication& app, QObject* parent = nullptr) noexcept;
  virtual ~MainWindow() noexcept;

  // Operator Overloadings
  MainWindow& operator=(const MainWindow& rhs) = delete;

private:
  void menuItemTriggered(ui::MenuItemId id) noexcept;

  GuiApplication& mApp;
  slint::ComponentHandle<ui::AppWindow> mWindow;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
