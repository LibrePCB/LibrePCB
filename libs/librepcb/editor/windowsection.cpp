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
#include "windowsection.h"

#include "hometab.h"
#include "library/createlibrarytab.h"
#include "library/downloadlibrarytab.h"
#include "utils/deriveduiobjectlistview.h"
#include "windowtab.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WindowSection::WindowSection(GuiApplication& app, QObject* parent) noexcept
  : QObject(parent),
    onUiDataChanged(*this),
    mApp(app),
    mTabs(new UiObjectList<WindowTab, ui::TabData>()),
    mUiData{
        mTabs,  // Tabs
        std::make_shared<DerivedUiObjectList<TabList, CreateLibraryTab,
                                             ui::CreateLibraryTabData>>(mTabs),
        std::make_shared<DerivedUiObjectList<TabList, DownloadLibraryTab,
                                             ui::DownloadLibraryTabData>>(
            mTabs),
        -1,  // Current tab index
        false,  // Highlight
    } {
}

WindowSection::~WindowSection() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WindowSection::setUiData(const ui::WindowSectionData& data) noexcept {
  setCurrentTab(data.current_tab_index);
}

void WindowSection::setHomeTabVisible(bool visible) noexcept {
  const bool hasHomeTab =
      std::dynamic_pointer_cast<HomeTab>(mTabs->value(0)) != nullptr;
  if (visible && (!hasHomeTab)) {
    addTab(std::make_shared<HomeTab>(mApp), 0);
  } else if ((!visible) && hasHomeTab) {
    removeTab(0);
  }
}

void WindowSection::addTab(std::shared_ptr<WindowTab> tab, int index) noexcept {
  // Connect closeRequested() asynchronously to avoid complex nested function
  // calls while the tab object is getting destroyed.
  connect(tab.get(), &WindowTab::closeRequested, this,
          &WindowSection::tabCloseRequested, Qt::QueuedConnection);
  connect(tab.get(), &WindowTab::panelPageRequested, this,
          &WindowSection::panelPageRequested);
  connect(tab.get(), &WindowTab::statusBarMessageChanged, this,
          &WindowSection::statusBarMessageChanged);
  if (index < 0) {
    index = mTabs->count();
  } else {
    index = qBound(0, index, mTabs->count());
  }
  mTabs->insert(index, tab);
  setCurrentTab(index);
}

std::shared_ptr<WindowTab> WindowSection::removeTab(int index) noexcept {
  if (auto tab = mTabs->takeAt(index)) {
    int currentIndex = mUiData.current_tab_index;
    if (index < currentIndex) {
      --currentIndex;
    }
    setCurrentTab(std::min(currentIndex, mTabs->count() - 1));
    disconnect(tab.get(), &WindowTab::closeRequested, this,
               &WindowSection::tabCloseRequested);
    disconnect(tab.get(), &WindowTab::panelPageRequested, this,
               &WindowSection::panelPageRequested);
    disconnect(tab.get(), &WindowTab::statusBarMessageChanged, this,
               &WindowSection::statusBarMessageChanged);
    return tab;
  } else {
    return nullptr;
  }
}

void WindowSection::triggerTab(int index, ui::TabAction a) noexcept {
  if (auto t = mTabs->value(index)) {
    t->trigger(a);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WindowSection::setCurrentTab(int index) noexcept {
  if (mUiData.current_tab_index == index) {
    return;
  }

  for (auto t : *mTabs) {
    t->deactivate();
  }
  if (std::shared_ptr<WindowTab> t = mTabs->value(index)) {
    t->activate();
  }

  mUiData.current_tab_index = index;
  onUiDataChanged.notify();
}

void WindowSection::highlight() noexcept {
  mUiData.highlight = true;
  onUiDataChanged.notify();

  QTimer::singleShot(1000, this, [this]() {
    mUiData.highlight = false;
    onUiDataChanged.notify();
  });
}

void WindowSection::tabCloseRequested() noexcept {
  const WindowTab* tab = static_cast<const WindowTab*>(sender());
  if (auto index = mTabs->indexOf(tab)) {
    if (!dynamic_cast<const HomeTab*>(tab)) {
      removeTab(*index);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
