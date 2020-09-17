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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEPADLISTMODEL_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEPADLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/pkg/packagepad.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace library {
namespace editor {

/*******************************************************************************
 *  Class PackagePadListModel
 ******************************************************************************/

/**
 * @brief The PackagePadListModel class
 */
class PackagePadListModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column { COLUMN_NAME, COLUMN_ACTIONS, _COLUMN_COUNT };

  // Constructors / Destructor
  PackagePadListModel() = delete;
  PackagePadListModel(const PackagePadListModel& other) noexcept;
  explicit PackagePadListModel(QObject* parent = nullptr) noexcept;
  ~PackagePadListModel() noexcept;

  // Setters
  void setPadList(PackagePadList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;

  // Slots
  void addPad(const QVariant& editData) noexcept;
  void removePad(const QVariant& editData) noexcept;

  // Inherited from QAbstractItemModel
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override;

  // Operator Overloadings
  PackagePadListModel& operator=(const PackagePadListModel& rhs) noexcept;

private:
  void padListEdited(const PackagePadList& list, int index,
                     const std::shared_ptr<const PackagePad>& pad,
                     PackagePadList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  CircuitIdentifier validateNameOrThrow(const QString& name) const;
  QString getNextPadNameProposal() const noexcept;

private:  // Data
  PackagePadList* mPadList;
  UndoStack* mUndoStack;
  QString mNewName;

  // Slots
  PackagePadList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
