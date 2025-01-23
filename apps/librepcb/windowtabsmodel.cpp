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
    for (int i = 0; i < mItems.count(); ++i) {
      if (mItems.at(i).get() == t) {
        return i;
      }
    }
    return -1;
  };

  connect(tab.get(), &WindowTab::cursorCoordinatesChanged, this,
          &WindowTabsModel::cursorCoordinatesChanged);
  connect(
      tab.get(), &WindowTab::requestClose, this,
      [this, getTabIndex]() { closeTab(getTabIndex(sender())); },
      Qt::QueuedConnection);
  connect(tab.get(), &WindowTab::requestRepaint, this,
          &WindowTabsModel::requestRepaint);
  connect(tab.get(), &WindowTab::uiDataChanged, this,
          [this, getTabIndex]() { emit uiDataChanged(getTabIndex(sender())); });
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
    return s->getUiData();
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
