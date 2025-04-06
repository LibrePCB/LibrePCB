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

  // Operator Overloadings
  WindowSection& operator=(const WindowSection& rhs) = delete;

signals:
  void splitRequested();
  void closeRequested();
  void derivedUiDataChanged(std::size_t index);
  void statusBarMessageChanged(const QString& message, int timeoutMs);

private:
  void setCurrentTab(int index) noexcept;
  void trigger(ui::Action a) noexcept;
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
