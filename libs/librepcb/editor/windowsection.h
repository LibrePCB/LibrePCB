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

#ifndef LIBREPCB_EDITOR_WINDOWSECTION_H
#define LIBREPCB_EDITOR_WINDOWSECTION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"
#include "utils/uiobjectlist.h"

#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class LengthUnit;
class Point;

namespace editor {

class GuiApplication;
class WindowTab;

/*******************************************************************************
 *  Class WindowSection
 ******************************************************************************/

/**
 * @brief The WindowSection class
 */
class WindowSection final : public QObject {
  Q_OBJECT

public:
  // Signals
  Signal<WindowSection> onUiDataChanged;

  // Constructors / Destructor
  WindowSection() = delete;
  WindowSection(const WindowSection& other) = delete;
  explicit WindowSection(GuiApplication& app,
                         QObject* parent = nullptr) noexcept;
  ~WindowSection() noexcept;

  // General Methods
  const ui::WindowSectionData& getUiData() const noexcept { return mUiData; }
  void setUiData(const ui::WindowSectionData& data) noexcept;
  void setHomeTabVisible(bool visible) noexcept;
  void addTab(std::shared_ptr<WindowTab> tab, int index = -1) noexcept;
  std::shared_ptr<WindowTab> removeTab(int index) noexcept;
  void triggerTab(int index, ui::TabAction a) noexcept;
  slint::Image renderScene(float width, float height, int scene) noexcept;
  bool processScenePointerEvent(const QPointF& pos,
                                slint::private_api::PointerEvent e) noexcept;
  bool processSceneScrolled(const QPointF& pos,
                            slint::private_api::PointerScrollEvent e) noexcept;
  bool processSceneKeyEvent(const slint::private_api::KeyEvent& e) noexcept;

  template <typename T>
  bool switchToTab() noexcept {
    for (int i = 0; i < mTabs->count(); ++i) {
      if (std::dynamic_pointer_cast<T>(mTabs->value(i))) {
        setCurrentTab(i);
        highlight();
        return true;
      }
    }
    return false;
  }

  template <typename T>
  bool switchToLibraryElementTab(const FilePath& fp) noexcept {
    for (int i = 0; i < mTabs->count(); ++i) {
      if (auto tab = std::dynamic_pointer_cast<T>(mTabs->at(i))) {
        if (tab->getDirectoryPath() == fp) {
          setCurrentTab(i);
          highlight();
          return true;
        }
      }
    }
    return false;
  }

  template <typename T>
  bool switchToProjectTab(int prjIndex, int objIndex) noexcept {
    for (int i = 0; i < mTabs->count(); ++i) {
      if (auto tab = std::dynamic_pointer_cast<T>(mTabs->at(i))) {
        if ((tab->getProjectIndex() == prjIndex) &&
            (tab->getProjectObjectIndex() == objIndex)) {
          setCurrentTab(i);
          highlight();
          return true;
        }
      }
    }
    return false;
  }

  /**
   * @brief Request to close all tabs
   *
   * If there are unsaved changes in any tabs, this method will ask the user
   * whether the changes should be saved or not. If the user clicks on "cancel"
   * or the changes could not be saved successfully, this method will return
   * false. If there were no unsaved changes or they were successfully saved,
   * the method returns true.
   *
   * @retval true   All tabs are safe to be closed.
   * @retval false  Some tabs still has unsaved changes.
   */
  bool requestCloseAllTabs() noexcept;

  // Operator Overloadings
  WindowSection& operator=(const WindowSection& rhs) = delete;

signals:
  void currentTabChanged();
  void panelPageRequested(ui::PanelPage p);
  void derivedUiDataChanged(std::size_t index);
  void statusBarMessageChanged(const QString& message, int timeoutMs);
  void cursorCoordinatesChanged(const Point& pos, const LengthUnit& unit);

private:
  void setCurrentTab(int index, bool forceUpdate = false) noexcept;
  std::shared_ptr<WindowTab> getCurrentTab() noexcept;
  void highlight() noexcept;
  void tabCloseRequested() noexcept;

  typedef UiObjectList<WindowTab, ui::TabData> TabList;

  GuiApplication& mApp;
  std::shared_ptr<TabList> mTabs;
  ui::WindowSectionData mUiData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
