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

#ifndef LIBREPCB_EDITOR_ASSEMBLYVARIANTLISTMODEL_H
#define LIBREPCB_EDITOR_ASSEMBLYVARIANTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/circuit/assemblyvariant.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circuit;

namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class AssemblyVariantListModel
 ******************************************************************************/

/**
 * @brief The AssemblyVariantListModel class
 */
class AssemblyVariantListModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    COLUMN_NAME,
    COLUMN_DESCRIPTION,
    COLUMN_ACTIONS,
    _COLUMN_COUNT,
  };

  // Constructors / Destructor
  AssemblyVariantListModel() = delete;
  AssemblyVariantListModel(const AssemblyVariantListModel& other) = delete;
  explicit AssemblyVariantListModel(QObject* parent = nullptr) noexcept;
  ~AssemblyVariantListModel() noexcept;

  // Setters
  void setCircuit(Circuit* circuit) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;
  void setParentWidget(QWidget* widget) noexcept;

  // Slots
  void copy(const QPersistentModelIndex& itemIndex) noexcept;
  void remove(const QPersistentModelIndex& itemIndex) noexcept;
  void moveUp(const QPersistentModelIndex& itemIndex) noexcept;
  void moveDown(const QPersistentModelIndex& itemIndex) noexcept;

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
  AssemblyVariantListModel& operator=(
      const AssemblyVariantListModel& rhs) noexcept;

private:
  void listEdited(const AssemblyVariantList& list, int index,
                  const std::shared_ptr<const AssemblyVariant>& obj,
                  AssemblyVariantList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);

private:  // Data
  QPointer<QWidget> mParentWidget;
  QPointer<Circuit> mCircuit;
  UndoStack* mUndoStack;

  // Slots
  AssemblyVariantList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
