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

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CategoryTreeModel::CategoryTreeModel(const WorkspaceLibraryDb& db,
                                     const WorkspaceSettings& ws,
                                     Filters filters,
                                     const std::optional<Uuid>& hiddenCategory,
                                     QObject* parent) noexcept
  : QObject(parent),
    mDb(db),
    mSettings(ws),
    mFilters(filters),
    mHiddenCategory(hiddenCategory),
    mIcon(q2s(QIcon(":/bi/folder.svg").pixmap(32))) {
  connect(&mDb, &WorkspaceLibraryDb::scanSucceeded, this,
          &CategoryTreeModel::refresh, Qt::QueuedConnection);
  connect(&mSettings.libraryLocaleOrder, &WorkspaceSettingsItem::edited, this,
          &CategoryTreeModel::refresh, Qt::QueuedConnection);
  refresh();
}

CategoryTreeModel::~CategoryTreeModel() noexcept {
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t CategoryTreeModel::row_count() const {
  return mItems.size();
}

std::optional<ui::TreeViewItemData> CategoryTreeModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CategoryTreeModel::refresh() noexcept {
  try {
    mItems.clear();
    mItems.push_back(ui::TreeViewItemData{
        0,  // Level
        mIcon,  // Icon
        q2s(tr("Root Category")),  // Text
        slint::SharedString(),  // Hint
        "null",  // User data
        false,  // Is project file or folder
        false,  // Has children
        false,  // Expanded
        false,  // Supports pinning
        false,  // Pinned
        ui::TreeViewItemAction::None,  // Action
    });

    if (mFilters.testFlag(Filter::CmpCat)) {
      loadChilds<ComponentCategory>(std::nullopt, 1);  // can throw
    } else if (mFilters.testFlag(Filter::PkgCat)) {
      loadChilds<PackageCategory>(std::nullopt, 1);  // can throw
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to refresh CategoryTreeModel:" << e.getMsg();
  }
  notify_reset();
}

template <typename T>
void CategoryTreeModel::loadChilds(const std::optional<Uuid>& parent,
                                   int level) {
  QVector<std::pair<Uuid, QString>> childs;
  for (auto uuid : mDb.getChilds<T>(parent)) {  // can throw
    if (uuid == mHiddenCategory) continue;

    const FilePath fp = mDb.getLatest<T>(uuid);  // can throw

    QString name;
    mDb.getTranslations<T>(fp, mSettings.libraryLocaleOrder.get(),
                           &name);  // can throw

    childs.append(std::make_pair(uuid, name));
  }

  Toolbox::sortNumeric(
      childs,
      [](const QCollator& collator, const std::pair<Uuid, QString>& lhs,
         const std::pair<Uuid, QString>& rhs) {
        return collator(lhs.second, rhs.second);
      });

  for (const auto& pair : childs) {
    mItems.push_back(ui::TreeViewItemData{
        level,  // Level
        mIcon,  // Icon
        q2s(pair.second.isEmpty() ? pair.first.toStr() : pair.second),  // Text
        slint::SharedString(),  // Hint
        q2s(pair.first.toStr()),  // User data
        false,  // Is project file or folder
        false,  // Has children
        false,  // Expanded
        false,  // Supports pinning
        false,  // Pinned
        ui::TreeViewItemAction::None,  // Action
    });
    loadChilds<T>(pair.first, level + 1);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
