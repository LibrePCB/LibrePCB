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

#ifndef LIBREPCB_CREATELIBRARYTABSMODEL_H
#define LIBREPCB_CREATELIBRARYTABSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <QtCore>
#include <QtGui>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

class WindowTabsModel;

/*******************************************************************************
 *  Class CreateLibraryTabsModel
 ******************************************************************************/

/**
 * @brief The CreateLibraryTabsModel class
 */
class CreateLibraryTabsModel : public QObject,
                               public slint::Model<ui::CreateLibraryTabData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  CreateLibraryTabsModel() = delete;
  CreateLibraryTabsModel(const CreateLibraryTabsModel& other) = delete;
  explicit CreateLibraryTabsModel(std::shared_ptr<WindowTabsModel> tabs,
                                  QObject* parent = nullptr) noexcept;
  virtual ~CreateLibraryTabsModel() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::CreateLibraryTabData> row_data(
      std::size_t i) const override;
  void set_row_data(size_t i, const ui::CreateLibraryTabData& data) override;

  // Operator Overloadings
  CreateLibraryTabsModel& operator=(const CreateLibraryTabsModel& rhs) = delete;

private:
  std::shared_ptr<WindowTabsModel> mModel;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
