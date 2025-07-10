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
#include "libraryelementcategoriesmodel.h"

#include "../utils/slinthelpers.h"
#include "cat/categorytreebuilder.h"

#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryElementCategoriesModel::LibraryElementCategoriesModel(
    const Workspace& ws, Type type, QObject* parent) noexcept
  : QObject(parent), mWs(ws), mType(type) {
  connect(&mWs.getLibraryDb(), &WorkspaceLibraryDb::scanSucceeded, this,
          &LibraryElementCategoriesModel::refresh, Qt::QueuedConnection);
  refresh();
}

LibraryElementCategoriesModel::~LibraryElementCategoriesModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void LibraryElementCategoriesModel::setCategories(
    const QSet<Uuid>& categories) noexcept {
  if (categories != mCategories) {
    mCategories = categories;
    refresh();
  }
}

void LibraryElementCategoriesModel::add(const Uuid& category) noexcept {
  if (!mCategories.contains(category)) {
    mCategories.insert(category);
    emit modified(mCategories);
    refresh();
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t LibraryElementCategoriesModel::row_count() const {
  return mItems.size();
}

std::optional<ui::LibraryElementCategoryData>
    LibraryElementCategoriesModel::row_data(std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void LibraryElementCategoriesModel::set_row_data(
    std::size_t i, const ui::LibraryElementCategoryData& data) noexcept {
  if ((i < mItems.size()) && data.delete_) {
    if (auto uuid = Uuid::tryFromString(s2q(data.uuid))) {
      mCategories.remove(*uuid);
      emit modified(mCategories);
      refresh();
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryElementCategoriesModel::refresh() noexcept {
  mItems.clear();

  try {
    if (mType == Type::PackageCategory) {
      loadItems<CategoryTreeBuilder<PackageCategory>>();
    } else {
      loadItems<CategoryTreeBuilder<ComponentCategory>>();
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to build categories model:" << e.getMsg();
  }

  Toolbox::sortNumeric(
      mItems,
      [](const QCollator& collator, const ui::LibraryElementCategoryData& lhs,
         const ui::LibraryElementCategoryData& rhs) {
        const std::size_t lhsLen = lhs.names ? lhs.names->row_count() : 0;
        const std::size_t rhsLen = rhs.names ? rhs.names->row_count() : 0;
        for (std::size_t i = 0; i < std::max(lhsLen, rhsLen); ++i) {
          const auto s1 = lhs.names->row_data(i);
          const auto s2 = rhs.names->row_data(i);
          if (s1 && s2 && (*s1 != *s2)) {
            return collator(s1->data(), s2->data());
          } else if (s1.has_value() != s2.has_value()) {
            return s2.has_value();
          }
        }
        return false;
      });

  notify_reset();
}

template <typename T>
void LibraryElementCategoriesModel::loadItems() {
  T builder(mWs.getLibraryDb(), mWs.getSettings().libraryLocaleOrder.get(),
            false);
  for (const Uuid& uuid : mCategories) {
    const QStringList names = builder.buildTree(uuid);
    mItems.push_back(ui::LibraryElementCategoryData{
        q2s(uuid.toStr()),  // UUID
        q2s(names),  // Names
        false,  // Delete
    });
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
