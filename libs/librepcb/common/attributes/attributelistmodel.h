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

#ifndef LIBREPCB_ATTRIBUTELISTMODEL_H
#define LIBREPCB_ATTRIBUTELISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../model/comboboxdelegate.h"
#include "attribute.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class UndoStack;

/*******************************************************************************
 *  Class AttributeListModel
 ******************************************************************************/

/**
 * @brief The AttributeListModel class
 */
class AttributeListModel final : public QAbstractTableModel {
  Q_OBJECT

public:
  enum Column {
    COLUMN_KEY,
    COLUMN_TYPE,
    COLUMN_VALUE,
    COLUMN_UNIT,
    COLUMN_ACTIONS,
    _COLUMN_COUNT
  };

  // Constructors / Destructor
  AttributeListModel() = delete;
  AttributeListModel(const AttributeListModel& other) noexcept;
  explicit AttributeListModel(QObject* parent = nullptr) noexcept;
  ~AttributeListModel() noexcept;

  // Setters
  void setAttributeList(AttributeList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;

  // Slots
  void addAttribute(const QVariant& editData) noexcept;
  void removeAttribute(const QVariant& editData) noexcept;
  void moveAttributeUp(const QVariant& editData) noexcept;
  void moveAttributeDown(const QVariant& editData) noexcept;

  // Inherited from QAbstractItemModel
  int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  QVariant      data(const QModelIndex& index,
                     int                role = Qt::DisplayRole) const override;
  QVariant      headerData(int section, Qt::Orientation orientation,
                           int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;
  bool          setData(const QModelIndex& index, const QVariant& value,
                        int role = Qt::EditRole) override;

  // Operator Overloadings
  AttributeListModel& operator=(const AttributeListModel& rhs) noexcept;

private:
  void         attributeListEdited(const AttributeList& list, int index,
                                   const std::shared_ptr<const Attribute>& attribute,
                                   AttributeList::Event event) noexcept;
  void         execCmd(UndoCommand* cmd);
  AttributeKey validateKeyOrThrow(const QString& key) const;
  static ComboBoxDelegate::Items buildUnitComboBoxData(
      const AttributeType& type) noexcept;

private:  // Data
  AttributeList*          mAttributeList;
  UndoStack*              mUndoStack;
  ComboBoxDelegate::Items mTypeComboBoxItems;
  QString                 mNewKey;
  const AttributeType*    mNewType;
  QString                 mNewValue;
  const AttributeUnit*    mNewUnit;

  // Slots
  AttributeList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_ATTRIBUTELISTMODEL_H
