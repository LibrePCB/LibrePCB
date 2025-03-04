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
#include "windowtabsmodel.h"

#include "windowtab.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WindowTabsModel::WindowTabsModel(GuiApplication& app, QObject* parent) noexcept
  : QObject(parent), mApp(app), mItems() {
}

WindowTabsModel::~WindowTabsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WindowTabsModel::addTab(std::shared_ptr<WindowTab> tab) noexcept {
  auto getTabIndex = [this](QObject* obj) {
    const WindowTab* t = static_cast<WindowTab*>(obj);
    for (std::size_t i = 0; i < static_cast<std::size_t>(mItems.size()); ++i) {
      if (mItems.at(i).get() == t) {
        return std::make_optional(i);
      }
    }
    return std::optional<std::size_t>();
  };

  connect(tab.get(), &WindowTab::cursorCoordinatesChanged, this,
          &WindowTabsModel::cursorCoordinatesChanged);
  connect(tab.get(), &WindowTab::statusBarMessageChanged, this,
          &WindowTabsModel::statusBarMessageChanged);
  connect(
      tab.get(), &WindowTab::requestClose, this,
      [this, getTabIndex]() {
        if (auto index = getTabIndex(sender())) {
          closeTab(*index);
        }
      },
      Qt::QueuedConnection);
  connect(tab.get(), &WindowTab::baseUiDataChanged, this,
          [this, getTabIndex]() {
            if (auto index = getTabIndex(sender())) {
              row_changed(*index);
            }
          });
  connect(tab.get(), &WindowTab::tabUiDataChanged, this, [this, getTabIndex]() {
    if (auto index = getTabIndex(sender())) {
      emit uiDataChanged(*index);
    }
  });
  mItems.append(tab);
  row_added(mItems.count() - 1, 1);
}

void WindowTabsModel::closeTab(int index) noexcept {
  const int tabCount = static_cast<int>(row_count());
  if ((index >= 0) && (index < tabCount)) {
    mItems.remove(index);
    row_removed(index, 1);
  }
}

void WindowTabsModel::setCurrentTab(int index) noexcept {
  for (auto t : mItems) {
    t->deactivate();
  }
  if (std::shared_ptr<WindowTab> t = mItems.value(index)) {
    t->activate();
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t WindowTabsModel::row_count() const {
  return mItems.size();
}

std::optional<ui::TabData> WindowTabsModel::row_data(std::size_t i) const {
  if (std::shared_ptr<WindowTab> s = mItems.value(i)) {
    return s->getBaseUiData();
  } else {
    return std::nullopt;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
