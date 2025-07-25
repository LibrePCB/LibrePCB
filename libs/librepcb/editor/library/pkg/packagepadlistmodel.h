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

#ifndef LIBREPCB_EDITOR_PACKAGEPADLISTMODEL_H
#define LIBREPCB_EDITOR_PACKAGEPADLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/pkg/packagepad.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class PackagePadListModel
 ******************************************************************************/

/**
 * @brief The PackagePadListModel class
 */
class PackagePadListModel final : public QObject,
                                  public slint::Model<ui::PackagePadData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // PackagePadListModel() = delete;
  PackagePadListModel(const PackagePadListModel& other) = delete;
  explicit PackagePadListModel(QObject* parent = nullptr) noexcept;
  ~PackagePadListModel() noexcept;

  // General Methods
  void setReferences(PackagePadList* list, UndoStack* stack) noexcept;
  bool add(QString names) noexcept;
  void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::PackagePadData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::PackagePadData& data) noexcept override;

  // Operator Overloadings
  PackagePadListModel& operator=(const PackagePadListModel& rhs) = delete;

private:
  ui::PackagePadData createItem(const PackagePad& obj, int sortIndex) noexcept;
  void updateSortOrder(bool notify) noexcept;
  void listEdited(const PackagePadList& list, int index,
                  const std::shared_ptr<const PackagePad>& item,
                  PackagePadList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  CircuitIdentifier validateNameOrThrow(const QString& name) const;
  QString getNextPadNameProposal() const noexcept;

private:
  PackagePadList* mList;
  QPointer<UndoStack> mUndoStack;

  QList<ui::PackagePadData> mItems;

  // Slots
  PackagePadList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
