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

#ifndef LIBREPCB_LIBRARY_LIBRARIESMODEL_H
#define LIBREPCB_LIBRARY_LIBRARIESMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>

#include <vector>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;
class ApiEndpoint;

namespace editor {
namespace app {

/*******************************************************************************
 *  Class LibrariesModel
 ******************************************************************************/

/**
 * @brief The LibrariesModel class
 */
class LibrariesModel : public QObject, public slint::Model<ui::Library> {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibrariesModel() = delete;
  LibrariesModel(const LibrariesModel& other) = delete;
  explicit LibrariesModel(Workspace& ws, QObject* parent = nullptr) noexcept;
  virtual ~LibrariesModel() noexcept;

  // General Methods
  std::size_t row_count() const override;
  std::optional<ui::Library> row_data(std::size_t i) const override;

  // Operator Overloadings
  LibrariesModel& operator=(const LibrariesModel& rhs) = delete;

private:
  void refreshLocalLibraries() noexcept;
  void refreshRemoteLibraries() noexcept;

  Workspace& mWorkspace;
  QList<std::shared_ptr<ApiEndpoint>> mApiEndpointsInProgress;
  std::vector<ui::Library> mLibs;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
