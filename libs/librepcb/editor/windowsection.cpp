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
#include "library/cat/componentcategorytab.h"
#include "library/cat/packagecategorytab.h"
#include "library/cmp/componenttab.h"
#include "library/createlibrarytab.h"
#include "library/dev/devicetab.h"
#include "library/downloadlibrarytab.h"
#include "library/lib/librarytab.h"
#include "library/pkg/packagetab.h"
#include "library/sym/symboltab.h"
#include "project/board/board2dtab.h"
#include "project/board/board3dtab.h"
#include "project/schematic/schematictab.h"
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
        std::make_shared<
            DerivedUiObjectList<TabList, LibraryTab, ui::LibraryTabData>>(
            mTabs),
        std::make_shared<DerivedUiObjectList<TabList, ComponentCategoryTab,
                                             ui::CategoryTabData>>(mTabs),
        std::make_shared<DerivedUiObjectList<TabList, PackageCategoryTab,
                                             ui::CategoryTabData>>(mTabs),
        std::make_shared<
            DerivedUiObjectList<TabList, SymbolTab, ui::SymbolTabData>>(mTabs),
        std::make_shared<
            DerivedUiObjectList<TabList, PackageTab, ui::PackageTabData>>(
            mTabs),
        std::make_shared<
            DerivedUiObjectList<TabList, ComponentTab, ui::ComponentTabData>>(
            mTabs),
        std::make_shared<
            DerivedUiObjectList<TabList, DeviceTab, ui::DeviceTabData>>(mTabs),
        std::make_shared<
            DerivedUiObjectList<TabList, SchematicTab, ui::SchematicTabData>>(
            mTabs),
        std::make_shared<
            DerivedUiObjectList<TabList, Board2dTab, ui::Board2dTabData>>(
            mTabs),
        std::make_shared<
            DerivedUiObjectList<TabList, Board3dTab, ui::Board3dTabData>>(
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
  // However, when the close request comes from the project editor rather than
  // from the UI, we have to close it immediately to avoid dangling references.
  connect(tab.get(), &WindowTab::closeEnforced, this,
          &WindowSection::tabCloseRequested);
  connect(tab.get(), &WindowTab::panelPageRequested, this,
          &WindowSection::panelPageRequested);
  connect(tab.get(), &WindowTab::statusBarMessageChanged, this,
          &WindowSection::statusBarMessageChanged);
  connect(tab.get(), &WindowTab::cursorCoordinatesChanged, this,
          &WindowSection::cursorCoordinatesChanged);
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
    const bool forceUpdate = index <= currentIndex;
    if (index < currentIndex) {
      --currentIndex;
    }
    setCurrentTab(std::min(currentIndex, mTabs->count() - 1), forceUpdate);
    disconnect(tab.get(), &WindowTab::closeRequested, this,
               &WindowSection::tabCloseRequested);
    disconnect(tab.get(), &WindowTab::closeEnforced, this,
               &WindowSection::tabCloseRequested);
    disconnect(tab.get(), &WindowTab::panelPageRequested, this,
               &WindowSection::panelPageRequested);
    disconnect(tab.get(), &WindowTab::statusBarMessageChanged, this,
               &WindowSection::statusBarMessageChanged);
    disconnect(tab.get(), &WindowTab::cursorCoordinatesChanged, this,
               &WindowSection::cursorCoordinatesChanged);
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

slint::Image WindowSection::renderScene(float width, float height,
                                        int scene) noexcept {
  if (std::shared_ptr<WindowTab> t = getCurrentTab()) {
    return t->renderScene(width, height, scene);
  }
  return slint::Image();
}

bool WindowSection::processScenePointerEvent(
    const QPointF& pos, slint::private_api::PointerEvent e) noexcept {
  if (std::shared_ptr<WindowTab> t = getCurrentTab()) {
    return t->processScenePointerEvent(pos, e);
  }
  return false;
}

bool WindowSection::processSceneScrolled(
    const QPointF& pos, slint::private_api::PointerScrollEvent e) noexcept {
  if (std::shared_ptr<WindowTab> t = getCurrentTab()) {
    return t->processSceneScrolled(pos, e);
  }
  return false;
}

bool WindowSection::processSceneKeyEvent(
    const slint::private_api::KeyEvent& e) noexcept {
  if (std::shared_ptr<WindowTab> t = getCurrentTab()) {
    return t->processSceneKeyEvent(e);
  }
  return false;
}

bool WindowSection::requestCloseAllTabs() noexcept {
  for (auto tab : *mTabs) {
    if (!tab->requestClose()) {
      return false;
    }
  }
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WindowSection::setCurrentTab(int index, bool forceUpdate) noexcept {
  if ((!forceUpdate) && (mUiData.current_tab_index == index)) {
    return;
  }

  std::shared_ptr<WindowTab> tab = mTabs->value(index);

  for (auto t : *mTabs) {
    t->deactivate();
  }
  if (tab) {
    tab->activate();
  }

  mUiData.current_tab_index = index;
  onUiDataChanged.notify();
  emit currentTabChanged();
}

std::shared_ptr<WindowTab> WindowSection::getCurrentTab() noexcept {
  return mTabs->value(mUiData.current_tab_index);
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
