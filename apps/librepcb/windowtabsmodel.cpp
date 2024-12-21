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

void WindowTabsModel::addTab(std::shared_ptr<ProjectEditor> prj,
                             ui::TabType type, int objIndex,
                             const QString& title) noexcept {
  auto t = std::make_shared<WindowTab>(mApp, prj, type, objIndex, title);
  connect(t.get(), &WindowTab::cursorCoordinatesChanged, this,
          &WindowTabsModel::cursorCoordinatesChanged);
  connect(t.get(), &WindowTab::requestRepaint, this,
          &WindowTabsModel::requestRepaint);
  mItems.append(t);
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
  if (std::shared_ptr<WindowTab> t = getTab(index)) {
    t->activate();
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t WindowTabsModel::row_count() const {
  return mItems.size();
}

std::optional<ui::Tab> WindowTabsModel::row_data(std::size_t i) const {
  if (std::shared_ptr<WindowTab> s = mItems.value(i)) {
    return s->getUiData();
  } else {
    return std::nullopt;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
