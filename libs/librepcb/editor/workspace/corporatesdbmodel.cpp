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
#include "corporatesdbmodel.h"

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

CorporatesDbModel::CorporatesDbModel(const WorkspaceLibraryDb& db,
                                     const WorkspaceSettings& ws,
                                     QObject* parent) noexcept
  : QObject(parent), mDb(db), mSettings(ws) {
  connect(&mDb, &WorkspaceLibraryDb::scanSucceeded, this,
          &CorporatesDbModel::refresh, Qt::QueuedConnection);
  connect(&mSettings.libraryLocaleOrder, &WorkspaceSettingsItem::edited, this,
          &CorporatesDbModel::refresh, Qt::QueuedConnection);
  refresh();
}

CorporatesDbModel::~CorporatesDbModel() noexcept {
}

/*******************************************************************************
 *  Implementations
 ******************************************************************************/

std::size_t CorporatesDbModel::row_count() const {
  return mItems.size();
}

std::optional<ui::CorporateDbData> CorporatesDbModel::row_data(
    std::size_t i) const {
  return (i < mItems.size()) ? std::optional(mItems.at(i)) : std::nullopt;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CorporatesDbModel::refresh() noexcept {
  qDebug() << "Refreshing CorporatesDbModel...";

  try {
    mItems.clear();

    QList<WorkspaceLibraryDb::Corporate> corporates =
        mDb.getAllLatestCorporates(
            mSettings.libraryLocaleOrder.get());  // can throw
    for (const auto& corp : corporates) {
      if (corp.pcbProducts.isEmpty()) continue;
      if (corp.priority <= 0) continue;
      auto pcbProducts =
          std::make_shared<slint::VectorModel<ui::CorporatePcbProductDbData>>();
      for (const auto& prod : corp.pcbProducts) {
        pcbProducts->push_back(ui::CorporatePcbProductDbData{
            q2s(prod.uuid.toStr()),  // UUID
            q2s(prod.name),  // Name
            q2s(prod.url.toString()),  // URL
        });
      }
      mItems.push_back(ui::CorporateDbData{
          q2s(corp.uuid.toStr()),  // UUID
          q2s(corp.name),  // Name
          pcbProducts,  // PCB Products}
      });
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to refresh CorporatesDbModel:" << e.getMsg();
  }
  notify_reset();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
