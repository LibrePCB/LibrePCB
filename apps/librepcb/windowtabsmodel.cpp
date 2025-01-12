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

#include "board2dtab.h"
#include "board3dtab.h"
#include "createlibrarytab.h"
#include "schematictab.h"

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

WindowTabsModel::WindowTabsModel(GuiApplication& app, int sectionId, QObject* parent) noexcept
  : QObject(parent), mApp(app), mSectionId(sectionId), mItems(), mNextId(1) {
}

WindowTabsModel::~WindowTabsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<WindowTab> WindowTabsModel::getTabById(int id) noexcept {
  return mItems.value(mIndex.value(id, -1));
}

void WindowTabsModel::addTab(ui::TabType type,
                             std::shared_ptr<ProjectEditor> prj,
                             int objIndex) noexcept {
  std::shared_ptr<WindowTab> t;
  const int id = mSectionId | mNextId++;
  switch (type) {
    case ui::TabType::CreateLibrary: {
      t = std::make_shared<CreateLibraryTab>(mApp, id, this);
      break;
    }
    case ui::TabType::Schematic: {
      t = std::make_shared<SchematicTab>(mApp, id, prj, objIndex, this);
      break;
    }
    case ui::TabType::Board2d: {
      t = std::make_shared<Board2dTab>(mApp, id, prj, objIndex, this);
      break;
    }
    case ui::TabType::Board3d: {
      t = std::make_shared<Board3dTab>(mApp, id, prj, objIndex, this);
      break;
    }
    default: {
      return;
    }
  }
  connect(t.get(), &WindowTab::cursorCoordinatesChanged, this,
          &WindowTabsModel::cursorCoordinatesChanged);
  connect(t.get(), &WindowTab::requestRepaint, this,
          &WindowTabsModel::requestRepaint);
  mItems.append(t);
  updateIndex();
  row_added(mItems.count() - 1, 1);
}

void WindowTabsModel::closeTab(int index) noexcept {
  const int tabCount = static_cast<int>(row_count());
  if ((index >= 0) && (index < tabCount)) {
    mItems.remove(index);
    updateIndex();
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

void WindowTabsModel::updateIndex() noexcept {
  mIndex.clear();
  for (int i = 0; i < mItems.count(); ++i) {
    mIndex.insert(mItems.at(i)->getId(), i);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
