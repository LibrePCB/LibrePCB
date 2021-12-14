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

#ifndef LIBREPCB_EDITOR_COMPONENTSYMBOLVARIANTLISTMODEL_H
#define LIBREPCB_EDITOR_COMPONENTSYMBOLVARIANTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/cmp/componentsymbolvariant.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class ComponentSymbolVariantListModel
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantListModel class
 */
class ComponentSymbolVariantListModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    COLUMN_NAME,
    COLUMN_DESCRIPTION,
    COLUMN_NORM,
    COLUMN_SYMBOLCOUNT,
    COLUMN_ACTIONS,
    _COLUMN_COUNT
  };

  // Constructors / Destructor
  ComponentSymbolVariantListModel() = delete;
  ComponentSymbolVariantListModel(
      const ComponentSymbolVariantListModel& other) noexcept;
  explicit ComponentSymbolVariantListModel(QObject* parent = nullptr) noexcept;
  ~ComponentSymbolVariantListModel() noexcept;

  // Setters
  void setSymbolVariantList(ComponentSymbolVariantList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;

  // Slots
  void addSymbolVariant(const QVariant& editData) noexcept;
  void removeSymbolVariant(const QVariant& editData) noexcept;
  void moveSymbolVariantUp(const QVariant& editData) noexcept;
  void moveSymbolVariantDown(const QVariant& editData) noexcept;

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
  ComponentSymbolVariantListModel& operator=(
      const ComponentSymbolVariantListModel& rhs) noexcept;

private:
  void symbolVariantListEdited(
      const ComponentSymbolVariantList& list, int index,
      const std::shared_ptr<const ComponentSymbolVariant>& variant,
      ComponentSymbolVariantList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  ElementName validateNameOrThrow(const QString& name) const;

private:  // Data
  ComponentSymbolVariantList* mSymbolVariantList;
  UndoStack* mUndoStack;
  QString mNewName;
  QString mNewDescription;
  QString mNewNorm;

  // Slots
  ComponentSymbolVariantList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
