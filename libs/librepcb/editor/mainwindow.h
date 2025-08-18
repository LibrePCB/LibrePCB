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

class FilePath;
class RuleCheckMessage;

namespace editor {

class GuiApplication;
class LibraryEditor;
class MainWindowTestAdapter;
class ProjectEditor;
class ProjectReadmeRenderer;
class SchematicTab;
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
  void addSection(int newIndex, bool makeCurrent) noexcept;
  void addTab(std::shared_ptr<WindowTab> tab, int section = -1, int index = -1,
              bool switchToTab = true, bool switchToSection = true) noexcept;
  std::shared_ptr<WindowTab> removeTab(
      int section, int tab, bool* wasCurrentTab = nullptr,
      bool* wasCurrentSection = nullptr) noexcept;
  void showPanelPage(ui::PanelPage page) noexcept;
  void popUpNotifications() noexcept;
  void showStatusBarMessage(const QString& message, int timeoutMs);
  void highlightErcMessage(ProjectEditor& prjEditor,
                           std::shared_ptr<const RuleCheckMessage> msg,
                           bool zoomTo) noexcept;
  void setCurrentLibrary(int index) noexcept;
  void setCurrentProject(int index) noexcept;

  // Operator Overloadings
  MainWindow& operator=(const MainWindow& rhs) = delete;

signals:
  void aboutToClose();

private:
  slint::CloseRequestResponse closeRequested() noexcept;
  void trigger(ui::Action a) noexcept;
  void triggerSection(int section, ui::WindowSectionAction a) noexcept;
  void triggerTab(int section, int tab, ui::TabAction a) noexcept;
  void triggerLibrary(slint::SharedString path, ui::LibraryAction a) noexcept;
  void triggerLibraryElement(slint::SharedString path,
                             ui::LibraryElementAction a) noexcept;
  void triggerProject(int index, ui::ProjectAction a) noexcept;
  void triggerSchematic(int project, int schematic,
                        ui::SchematicAction a) noexcept;
  void triggerBoard(int project, int board, ui::BoardAction a) noexcept;
  void openLibraryTab(const FilePath& fp, bool wizardMode) noexcept;
  void openComponentCategoryTab(LibraryEditor& editor, const FilePath& fp,
                                bool copyFrom) noexcept;
  void openPackageCategoryTab(LibraryEditor& editor, const FilePath& fp,
                              bool copyFrom) noexcept;
  void openSymbolTab(LibraryEditor& editor, const FilePath& fp,
                     bool copyFrom) noexcept;
  void openPackageTab(LibraryEditor& editor, const FilePath& fp,
                      bool copyFrom) noexcept;
  void openComponentTab(LibraryEditor& editor, const FilePath& fp,
                        bool copyFrom) noexcept;
  void openDeviceTab(LibraryEditor& editor, const FilePath& fp,
                     bool copyFrom) noexcept;
  std::shared_ptr<SchematicTab> openSchematicTab(int projectIndex,
                                                 int index) noexcept;
  void openBoard2dTab(int projectIndex, int index) noexcept;
  void openBoard3dTab(int projectIndex, int index) noexcept;
  void updateHomeTabSection() noexcept;
  template <typename T>
  bool switchToTab() noexcept;
  template <typename T>
  bool switchToLibraryElementTab(const FilePath& fp) noexcept;
  template <typename T>
  std::shared_ptr<T> switchToProjectTab(int prjIndex, int objIndex) noexcept;

  const int mId;
  const QString mSettingsPrefix;
  GuiApplication& mApp;
  slint::ComponentHandle<ui::AppWindow> mWindow;
  QWidget* mWidget;
  std::shared_ptr<UiObjectList<WindowSection, ui::WindowSectionData>> mSections;
  std::unique_ptr<ProjectReadmeRenderer> mProjectPreviewRenderer;
  std::unique_ptr<MainWindowTestAdapter> mTestAdapter;

  friend class MainWindowTestAdapter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
