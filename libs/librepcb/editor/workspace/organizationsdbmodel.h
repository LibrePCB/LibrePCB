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

#ifndef LIBREPCB_EDITOR_ORGANIZATIONSDBMODEL_H
#define LIBREPCB_EDITOR_ORGANIZATIONSDBMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/types/uuid.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class WorkspaceLibraryDb;
class WorkspaceSettings;

namespace editor {

/*******************************************************************************
 *  Class OrganizationsDbModel
 ******************************************************************************/

/**
 * @brief The OrganizationsDbModel class
 */
class OrganizationsDbModel : public QObject,
                             public slint::Model<ui::OrganizationDbData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  OrganizationsDbModel() = delete;
  OrganizationsDbModel(const OrganizationsDbModel& other) = delete;
  explicit OrganizationsDbModel(const WorkspaceLibraryDb& db,
                                const WorkspaceSettings& ws,
                                QObject* parent = nullptr) noexcept;
  virtual ~OrganizationsDbModel() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::OrganizationDbData> row_data(std::size_t i) const override;

  // Operator Overloadings
  OrganizationsDbModel& operator=(const OrganizationsDbModel& rhs) = delete;

private:  // Methods
  void refresh() noexcept;

private:  // Data
  const WorkspaceLibraryDb& mDb;
  const WorkspaceSettings& mSettings;
  std::vector<ui::OrganizationDbData> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
