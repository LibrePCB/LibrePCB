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
#include "createlibrarytabsmodel.h"

#include "createlibrarytab.h"
#include "windowsection.h"
#include "windowsectionsmodel.h"
#include "windowtabsmodel.h"

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

CreateLibraryTabsModel::CreateLibraryTabsModel(
    std::shared_ptr<WindowSectionsModel> sections, QObject* parent) noexcept
  : QObject(parent), mSections(sections) {
  auto t = new QTimer();
  t->setInterval(1000);
  connect(t, &QTimer::timeout, this, [this](){
    row_changed(0x00010002);
  });
  t->start();
}

CreateLibraryTabsModel::~CreateLibraryTabsModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t CreateLibraryTabsModel::row_count() const {
  return 0x00010002;
}

std::optional<ui::CreateLibraryTabData> CreateLibraryTabsModel::row_data(
    std::size_t tabId) const {
  const int sectionId = tabId & 0xFFFF0000;
  if (auto section = mSections->getSection(sectionId)) {
    if (auto tab = section->getTabById(tabId)) {
      if (auto ptr = std::dynamic_pointer_cast<CreateLibraryTab>(tab)) {
        return ptr->getUiData();
      }
    }
  }
  return std::nullopt;
}

void CreateLibraryTabsModel::set_row_data(
    size_t tabId, const ui::CreateLibraryTabData& data) {
  const int sectionId = tabId & 0xFFFF0000;
  if (auto section = mSections->getSection(sectionId)) {
    if (auto tab = section->getTabById(tabId)) {
      if (auto ptr = std::dynamic_pointer_cast<CreateLibraryTab>(tab)) {
        ptr->setUiData(data);
        return;
      }
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
