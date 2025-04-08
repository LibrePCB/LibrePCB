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

#ifndef LIBREPCB_EDITOR_MAINWINDOW_H
#define LIBREPCB_EDITOR_MAINWINDOW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"
#include "utils/uiobjectlist.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GuiApplication;
class MainWindowTestAdapter;
class ProjectReadmeRenderer;
class WindowSection;
class WindowTab;

/*******************************************************************************
 *  Class MainWindow
 ******************************************************************************/

/**
 * @brief The MainWindow class
 */
class MainWindow final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  MainWindow() = delete;
  MainWindow(const MainWindow& other) = delete;
  explicit MainWindow(GuiApplication& app,
                      slint::ComponentHandle<ui::AppWindow> win, int id,
                      QObject* parent = nullptr) noexcept;
  ~MainWindow() noexcept;

  // General Methods
  int getId() const noexcept { return mId; }
  bool isCurrentWindow() const noexcept;
  void makeCurrentWindow() noexcept;
  void showPanelPage(ui::PanelPage page) noexcept;
  void popUpNotifications() noexcept;

  // Operator Overloadings
  MainWindow& operator=(const MainWindow& rhs) = delete;

signals:
  void aboutToClose();

private:
  slint::CloseRequestResponse closeRequested() noexcept;
  void triggerAsync(ui::Action a) noexcept;
  bool trigger(ui::Action a) noexcept;
  void splitSection(int index) noexcept;
  void updateHomeTabSection() noexcept;
  void addTab(std::shared_ptr<WindowTab> tab) noexcept;
  template <typename T>
  bool switchToTab() noexcept;

  const int mId;
  const QString mSettingsPrefix;
  GuiApplication& mApp;
  slint::ComponentHandle<ui::AppWindow> mWindow;
  QWidget* mWidget;
  std::shared_ptr<UiObjectList<WindowSection, ui::WindowSectionData>> mSections;
  std::unique_ptr<ProjectReadmeRenderer> mProjectPreviewRenderer;
  std::unique_ptr<MainWindowTestAdapter> mTestAdapter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
