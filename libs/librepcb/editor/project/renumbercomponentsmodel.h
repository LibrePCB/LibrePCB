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

#ifndef LIBREPCB_EDITOR_RENUMBERCOMPONENTSMODEL_H
#define LIBREPCB_EDITOR_RENUMBERCOMPONENTSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "ui.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentInstance;

namespace editor {

class ProjectEditor;

/*******************************************************************************
 *  Class RenumberComponentsModel
 ******************************************************************************/

/**
 * @brief The RenumberComponentsModel class
 */
class RenumberComponentsModel
  : public QObject,
    public slint::Model<ui::RenumberComponentsItemData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  RenumberComponentsModel() = delete;
  RenumberComponentsModel(const RenumberComponentsModel& other) = delete;
  explicit RenumberComponentsModel(ProjectEditor& editor,
                                   QObject* parent = nullptr) noexcept;
  ~RenumberComponentsModel() noexcept override;

  // General Methods
  void apply(QWidget* parent) noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::RenumberComponentsItemData> row_data(
      std::size_t i) const override;

  // Operator Overloadings
  RenumberComponentsModel& operator=(const RenumberComponentsModel& rhs) =
      delete;

private:  // Methods
  void invalidate() noexcept;
  void update() const noexcept;

private:
  QPointer<ProjectEditor> mProjectEditor;
  mutable QMap<QPointer<ComponentInstance>, QString> mRenames;
  mutable std::vector<ui::RenumberComponentsItemData> mItems;
  mutable bool mUpToDate;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
