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

#ifndef LIBREPCB_EDITOR_FOOTPRINTLISTMODEL_H
#define LIBREPCB_EDITOR_FOOTPRINTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/pkg/footprint.h>
#include <librepcb/core/library/pkg/packagemodel.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Package;

namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class FootprintListModel
 ******************************************************************************/

/**
 * @brief The FootprintListModel class
 */
class FootprintListModel final : public QObject,
                                 public slint::Model<ui::FootprintData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // FootprintListModel() = delete;
  FootprintListModel(const FootprintListModel& other) = delete;
  explicit FootprintListModel(QObject* parent = nullptr) noexcept;
  ~FootprintListModel() noexcept;

  // General Methods
  void setReferences(Package* pkg, UndoStack* stack) noexcept;
  void add(const QString& name) noexcept;
  void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::FootprintData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::FootprintData& data) noexcept override;

  // Operator Overloadings
  FootprintListModel& operator=(const FootprintListModel& rhs) = delete;

signals:
  void footprintAdded(int index);

private:
  ui::FootprintData createItem(const Footprint& obj) noexcept;
  void updateModels(const Footprint& obj, ui::FootprintData& item) noexcept;
  void trigger(int index, std::shared_ptr<Footprint> obj,
               ui::FootprintAction a) noexcept;
  void listEdited(const FootprintList& list, int index,
                  const std::shared_ptr<const Footprint>& item,
                  FootprintList::Event event) noexcept;
  void modelListEdited(const PackageModelList& list, int index,
                       const std::shared_ptr<const PackageModel>& item,
                       PackageModelList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  ElementName validateNameOrThrow(const QString& name) const;

private:
  QPointer<Package> mPackage;
  QPointer<UndoStack> mUndoStack;

  QList<ui::FootprintData> mItems;

  // Slots
  FootprintList::OnEditedSlot mOnEditedSlot;
  PackageModelList::OnEditedSlot mOnModelsEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
