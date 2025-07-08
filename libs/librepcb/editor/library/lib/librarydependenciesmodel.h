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

#ifndef LIBREPCB_EDITOR_LIBRARYDEPENDENCIESMODEL_H
#define LIBREPCB_EDITOR_LIBRARYDEPENDENCIESMODEL_H

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

class Workspace;

namespace editor {

/*******************************************************************************
 *  Class LibraryDependenciesModel
 ******************************************************************************/

/**
 * @brief The LibraryDependenciesModel class
 */
class LibraryDependenciesModel final
  : public QObject,
    public slint::Model<ui::LibraryDependency> {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryDependenciesModel() = delete;
  LibraryDependenciesModel(const LibraryDependenciesModel& other) = delete;
  explicit LibraryDependenciesModel(const Workspace& ws, const Uuid& libUuid,
                                    QObject* parent = nullptr) noexcept;
  ~LibraryDependenciesModel() noexcept;

  // General Methods
  const QSet<Uuid>& getUuids() const noexcept { return mCheckedUuids; }
  void setUuids(const QSet<Uuid>& uuids) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::LibraryDependency> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::LibraryDependency& data) noexcept override;

  // Operator Overloadings
  LibraryDependenciesModel& operator=(const LibraryDependenciesModel& rhs) =
      delete;

signals:
  void modified(const QSet<Uuid>& uuids);

private:
  void refresh() noexcept;

  const Workspace& mWs;
  const Uuid mLibUuid;
  QSet<Uuid> mCheckedUuids;
  std::vector<ui::LibraryDependency> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
