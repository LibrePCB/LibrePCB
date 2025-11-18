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
#include "organizationsdbmodel.h"

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

OrganizationsDbModel::OrganizationsDbModel(const WorkspaceLibraryDb& db,
                                           const WorkspaceSettings& ws,
                                           QObject* parent) noexcept
  : QObject(parent), mDb(db), mSettings(ws) {
  connect(&mDb, &WorkspaceLibraryDb::scanSucceeded, this,
          &OrganizationsDbModel::refresh, Qt::QueuedConnection);
  connect(&mSettings.libraryLocaleOrder, &WorkspaceSettingsItem::edited, this,
          &OrganizationsDbModel::refresh, Qt::QueuedConnection);
  refresh();
}

OrganizationsDbModel::~OrganizationsDbModel() noexcept {
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t OrganizationsDbModel::row_count() const {
  return mItems.size();
}

std::optional<ui::OrganizationDbData> OrganizationsDbModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OrganizationsDbModel::refresh() noexcept {
  qDebug() << "Refreshing OrganizationsDbModel...";

  try {
    mItems.clear();

    QList<WorkspaceLibraryDb::Organization> organizations =
        mDb.getAllLatestOrganizations(mSettings.libraryLocaleOrder.get(), true,
                                      false);  // can throw
    for (const auto& org : organizations) {
      if (org.pcbDesignRules.isEmpty()) continue;
      if (org.priority <= 0) continue;
      auto designRules = std::make_shared<
          slint::VectorModel<ui::OrganizationPcbDesignRulesDbData>>();
      for (const auto& rules : org.pcbDesignRules) {
        designRules->push_back(ui::OrganizationPcbDesignRulesDbData{
            q2s(rules.uuid.toStr()),  // UUID
            q2s(rules.name),  // Name
            q2s(rules.url.toString()),  // URL
        });
      }
      mItems.push_back(ui::OrganizationDbData{
          q2s(org.uuid.toStr()),  // UUID
          q2s(org.name),  // Name
          org.priority,  // Priority
          designRules,  // PCB design rules
      });
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to refresh OrganizationsDbModel:" << e.getMsg();
  }
  notify_reset();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
