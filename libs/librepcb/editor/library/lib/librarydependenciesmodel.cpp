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
#include "librarydependenciesmodel.h"

#include "../../utils/slinthelpers.h"

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

LibraryDependenciesModel::LibraryDependenciesModel(const Workspace& ws,
                                                   const Uuid& libUuid,
                                                   QObject* parent) noexcept
  : QObject(parent), mWs(ws), mLibUuid(libUuid) {
  connect(&mWs.getLibraryDb(), &WorkspaceLibraryDb::scanLibraryListUpdated,
          this, &LibraryDependenciesModel::refresh, Qt::QueuedConnection);
  refresh();
}

LibraryDependenciesModel::~LibraryDependenciesModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void LibraryDependenciesModel::setUuids(const QSet<Uuid>& uuids) noexcept {
  if (uuids != mCheckedUuids) {
    mCheckedUuids = uuids;
    refresh();
  }
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t LibraryDependenciesModel::row_count() const {
  return mItems.size();
}

std::optional<ui::LibraryDependency> LibraryDependenciesModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

void LibraryDependenciesModel::set_row_data(
    std::size_t i, const ui::LibraryDependency& data) noexcept {
  if (i < mItems.size()) {
    if (auto uuid = Uuid::tryFromString(s2q(data.uuid))) {
      if ((*uuid != mLibUuid) &&
          (data.checked != mCheckedUuids.contains(*uuid))) {
        if (data.checked) {
          mCheckedUuids.insert(*uuid);
        } else {
          mCheckedUuids.remove(*uuid);
        }
        mItems[i].checked = data.checked;
        notify_row_changed(i);
        emit modified(mCheckedUuids);
      }
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryDependenciesModel::refresh() noexcept {
  mItems.clear();

  try {
    QMultiMap<Version, FilePath> libraries =
        mWs.getLibraryDb().getAll<Library>();  // can throw

    QSet<Uuid> processedLibs;
    foreach (const FilePath& libDir, libraries) {
      Uuid uuid = Uuid::createRandom();
      mWs.getLibraryDb().getMetadata<Library>(libDir, &uuid);  // can throw

      // Do not offer the library itself as a dependency and offer each
      // library only once.
      if ((uuid == mLibUuid) || processedLibs.contains(uuid)) continue;
      processedLibs.insert(uuid);

      QPixmap icon;
      mWs.getLibraryDb().getLibraryMetadata(libDir, &icon);  // can throw

      QString name;
      mWs.getLibraryDb().getTranslations<Library>(
          libDir, mWs.getSettings().libraryLocaleOrder.get(),
          &name);  // can throw

      mItems.push_back(ui::LibraryDependency{
          q2s(uuid.toStr()),  // UUID
          q2s(icon),  // Icon
          q2s(name),  // Name
          mCheckedUuids.contains(uuid),  // Checked
      });
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to fetch libraries:" << e.getMsg();
  }

  Toolbox::sortNumeric(
      mItems,
      [](const QCollator& collator, const ui::LibraryDependency& lhs,
         const ui::LibraryDependency& rhs) {
        return collator(s2q(lhs.name), s2q(rhs.name));
      });

  notify_reset();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
