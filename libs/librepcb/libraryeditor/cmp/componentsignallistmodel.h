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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTSIGNALLISTMODEL_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTSIGNALLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/library/cmp/componentsignal.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

namespace library {
namespace editor {

/*******************************************************************************
 *  Class ComponentSignalListModel
 ******************************************************************************/

/**
 * @brief The ComponentSignalListModel class
 */
class ComponentSignalListModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    COLUMN_NAME,
    COLUMN_ISREQUIRED,
    COLUMN_FORCEDNETNAME,
    COLUMN_ACTIONS,
    _COLUMN_COUNT
  };

  // Constructors / Destructor
  ComponentSignalListModel() = delete;
  ComponentSignalListModel(const ComponentSignalListModel& other) noexcept;
  explicit ComponentSignalListModel(QObject* parent = nullptr) noexcept;
  ~ComponentSignalListModel() noexcept;

  // Setters
  void setSignalList(ComponentSignalList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;

  // Slots
  void addSignal(const QVariant& editData) noexcept;
  void removeSignal(const QVariant& editData) noexcept;

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
  ComponentSignalListModel& operator=(
      const ComponentSignalListModel& rhs) noexcept;

private:
  void signalListEdited(const ComponentSignalList& list, int index,
                        const std::shared_ptr<const ComponentSignal>& signal,
                        ComponentSignalList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  CircuitIdentifier validateNameOrThrow(const QString& name) const;
  static QString cleanForcedNetName(const QString& name) noexcept;

private:  // Data
  ComponentSignalList* mSignalList;
  UndoStack* mUndoStack;
  QString mNewName;
  bool mNewIsRequired;
  QString mNewForcedNetName;

  // Slots
  ComponentSignalList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
