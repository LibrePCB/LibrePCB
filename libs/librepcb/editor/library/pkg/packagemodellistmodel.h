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

#ifndef LIBREPCB_EDITOR_PACKAGEMODELLISTMODEL_H
#define LIBREPCB_EDITOR_PACKAGEMODELLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

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
 *  Class PackageModelListModel
 ******************************************************************************/

/**
 * @brief The PackageModelListModel class
 */
class PackageModelListModel final : public QObject,
                                    public slint::Model<ui::PackageModelData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // PackageModelListModel() = delete;
  PackageModelListModel(const PackageModelListModel& other) = delete;
  explicit PackageModelListModel(QObject* parent = nullptr) noexcept;
  ~PackageModelListModel() noexcept;

  // General Methods
  void setReferences(Package* pkg, UndoStack* stack) noexcept;
  std::optional<int> add() noexcept;

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::PackageModelData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::PackageModelData& data) noexcept override;

  // Operator Overloadings
  PackageModelListModel& operator=(const PackageModelListModel& rhs) = delete;

private:
  ui::PackageModelData createItem(const PackageModel& obj) noexcept;
  void trigger(int index, std::shared_ptr<PackageModel> obj,
               ui::PackageModelAction a) noexcept;
  void listEdited(const PackageModelList& list, int index,
                  const std::shared_ptr<const PackageModel>& item,
                  PackageModelList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  ElementName validateNameOrThrow(const QString& name) const;
  bool chooseStepFile(QByteArray& content, FilePath* selectedFile = nullptr);

private:
  QPointer<Package> mPackage;
  QPointer<UndoStack> mUndoStack;

  QList<ui::PackageModelData> mItems;

  // Slots
  PackageModelList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
