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
#include "categorytreemodel.h"

#include "../utils/slinthelpers.h"

#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

static QString getUserData(const std::optional<Uuid>& uuid) noexcept {
  return uuid ? uuid->toStr() : QString("null");
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CategoryTreeModel::CategoryTreeModel(const WorkspaceLibraryDb& db,
                                     const WorkspaceSettings& ws,
                                     Filters filters, bool showRootCategory,
                                     const std::optional<Uuid>& hiddenCategory,
                                     QObject* parent) noexcept
  : QObject(parent),
    mDb(db),
    mSettings(ws),
    mFilters(filters),
    mShowRootCategory(showRootCategory),
    mHiddenCategory(hiddenCategory),
    mIcon(q2s(QIcon(":/bi/folder.svg").pixmap(32))) {
  connect(&mDb, &WorkspaceLibraryDb::scanSucceeded, this,
          &CategoryTreeModel::refresh, Qt::QueuedConnection);
  connect(&mSettings.libraryLocaleOrder, &WorkspaceSettingsItem::edited, this,
          &CategoryTreeModel::refresh, Qt::QueuedConnection);

  // By default, expand only the root category.
  mExpandedCategories.insert(getUserData(std::nullopt));

  refresh();
}

CategoryTreeModel::~CategoryTreeModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CategoryTreeModel::setFilterTerm(const QString& term) noexcept {
  if (term != mFilterTerm) {
    mFilterTerm = term;
    applyFilter();
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t CategoryTreeModel::row_count() const {
  return mFilteredItems.size();
}

std::optional<ui::TreeViewItemData> CategoryTreeModel::row_data(
    std::size_t i) const {
  return (i < mFilteredItems.size()) ? std::optional(mFilteredItems.at(i))
                                     : std::nullopt;
}

void CategoryTreeModel::set_row_data(
    std::size_t i, const ui::TreeViewItemData& data) noexcept {
  const QString uuid = s2q(data.user_data);
  if ((i >= mFilteredItems.size()) || uuid.isEmpty()) {
    return;
  }
  if ((!mFilteredItems.at(i).expanded) && data.expanded) {
    mExpandedCategories.insert(uuid);
    applyFilter();
  } else if (mFilteredItems.at(i).expanded && (!data.expanded)) {
    mExpandedCategories.remove(uuid);
    applyFilter();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CategoryTreeModel::refresh() noexcept {
  try {
    mItems.clear();
    if (mShowRootCategory) {
      mItems.push_back(Item{
          0,  // Level
          std::nullopt,  // UUID
          tr("Root Category"),  // Name
          {},  // Keywords
          false,  // Visible
      });
    }
    if (mFilters.testFlag(Filter::CmpCat)) {
      loadChildren<ComponentCategory>(std::nullopt,
                                      mShowRootCategory ? 1 : 0);  // can throw
    } else if (mFilters.testFlag(Filter::PkgCat)) {
      loadChildren<PackageCategory>(std::nullopt,
                                    mShowRootCategory ? 1 : 0);  // can throw
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to refresh CategoryTreeModel:" << e.getMsg();
  }

  applyFilter();
}

template <typename T>
void CategoryTreeModel::loadChildren(const std::optional<Uuid>& parent,
                                     int level) {
  QVector<Item> children;
  for (auto uuid : mDb.getChildren<T>(parent)) {  // can throw
    if (uuid == mHiddenCategory) continue;

    Item item{
        level,  // Level
        uuid,  // UUID
        {},  // Name
        {},  // Keywords
        false,  // Visible
    };
    const FilePath fp = mDb.getLatest<T>(uuid);  // can throw
    mDb.getTranslations<T>(fp, mSettings.libraryLocaleOrder.get(), &item.name,
                           nullptr, &item.keywords);  // can throw
    children.append(item);
  }

  Toolbox::sortNumeric(
      children,
      [](const QCollator& collator, const Item& lhs, const Item& rhs) {
        return collator(lhs.name, rhs.name);
      });

  // Note: Recursing immediately after each child is crucial to keep the items
  // ordered by tree structure (each parent directly followed by its children).
  for (const auto& item : children) {
    mItems.append(item);
    loadChildren<T>(item.uuid, level + 1);
  }
}

void CategoryTreeModel::applyFilter() noexcept {
  const QStringList terms =
      mFilterTerm.simplified().split(' ', Qt::SkipEmptyParts);

  // Determine which items are visible. If a filter is active, the expanded
  // state is ignored: All matching items are shown together with all their
  // parents (even if collapsed). If no filter is active, only items within
  // expanded categories are shown.
  if (terms.isEmpty()) {
    QVector<bool> childrenVisible;  // Updated for each item, by level
    for (auto& item : mItems) {
      while (childrenVisible.count() <= item.level) {
        childrenVisible.append(false);
      }
      // The topmost level is always visible, all other levels only if their
      // parent category is expanded.
      item.visible = (item.level == 0) || childrenVisible.at(item.level - 1);
      childrenVisible[item.level] =
          item.visible && mExpandedCategories.contains(getUserData(item.uuid));
    }
  } else {
    auto matchesFilter = [&](const Item& item) {
      return std::all_of(terms.begin(), terms.end(), [&](const QString& term) {
        return item.name.contains(term, Qt::CaseInsensitive) ||
            item.keywords.contains(term, Qt::CaseInsensitive);
      });
    };

    int includeNextLevel = -1;
    for (int i = mItems.count() - 1; i >= 0; --i) {
      const Item& item = mItems.at(i);
      if (item.level == includeNextLevel) {
        mItems[i].visible = true;  // Parent of a visible item -> visible.
        --includeNextLevel;
      } else if (matchesFilter(item)) {
        mItems[i].visible = true;  // Matches the filter -> visible.
        includeNextLevel = item.level - 1;
      } else {
        mItems[i].visible = false;  // Does not match the filter -> hidden.
      }
    }
  }

  mFilteredItems.clear();
  for (int i = 0; i < mItems.count(); ++i) {
    const auto& item = mItems.at(i);
    if (!item.visible) {
      continue;
    }
    const QString userData = getUserData(item.uuid);
    // The previous item is the parent of this item, thus it is expanded.
    if ((!mFilteredItems.empty()) &&
        (item.level == mFilteredItems.back().level + 1)) {
      mFilteredItems.back().expanded = true;
    }
    // While filtering, expanding/collapsing is not possible.
    const bool hasChildren = (terms.isEmpty()) && (i + 1 < mItems.count()) &&
        (mItems.at(i + 1).level == item.level + 1);
    mFilteredItems.push_back(ui::TreeViewItemData{
        item.level,  // Level
        mIcon,  // Icon
        q2s(item.name.isEmpty() ? userData : item.name),  // Text
        {},  // Comment
        {},  // Status tip
        q2s(userData),  // User data
        false,  // Is project file or folder
        hasChildren,  // Has children
        false,  // Expanded
        false,  // Supports pinning
        false,  // Pinned
        ui::TreeViewItemAction::None,  // Action
    });
  }
  notify_reset();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
