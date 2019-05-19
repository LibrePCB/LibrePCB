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

#ifndef LIBREPCB_ATTRIBUTELISTEDITORWIDGET_H
#define LIBREPCB_ATTRIBUTELISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../attributes/attribute.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class AttributeListEditorWidget
 ******************************************************************************/

/**
 * @brief The AttributeListEditorWidget class
 */
class AttributeListEditorWidget final : public QWidget {
  Q_OBJECT

private:  // Types
  enum Column {
    COLUMN_KEY = 0,
    COLUMN_TYPE,
    COLUMN_VALUE,
    COLUMN_UNIT,
    COLUMN_BUTTONS,
    _COLUMN_COUNT
  };

public:
  // Constructors / Destructor
  explicit AttributeListEditorWidget(QWidget* parent = nullptr) noexcept;
  AttributeListEditorWidget(const AttributeListEditorWidget& other) = delete;
  ~AttributeListEditorWidget() noexcept;

  // Getters
  const AttributeList& getAttributeList() const noexcept {
    return mAttributeList;
  }

  // Setters
  void setAttributeList(const AttributeList& list) noexcept;

  // Operator Overloadings
  AttributeListEditorWidget& operator=(const AttributeListEditorWidget& rhs) =
      delete;

signals:
  void edited(const AttributeList& attributes);

private:  // Slots
  void currentCellChanged(int currentRow, int currentColumn, int previousRow,
                          int previousColumn) noexcept;
  void tableCellChanged(int row, int column) noexcept;
  void attributeTypeChanged(const AttributeType* type) noexcept;
  void attributeUnitChanged(const AttributeUnit* unit) noexcept;
  void btnAddRemoveClicked() noexcept;
  void btnUpClicked() noexcept;
  void btnDownClicked() noexcept;

private:  // Methods
  void updateTable(const Attribute* selected = nullptr) noexcept;
  void setTableRowContent(int row, const QString& key,
                          const AttributeType& type, const QString& value,
                          const AttributeUnit* unit) noexcept;
  void getTableRowContent(int row, QString& key, const AttributeType*& type,
                          QString& value, const AttributeUnit*& unit) const
      noexcept;
  void         addAttribute(const QString& key, const AttributeType& type,
                            const QString& value, const AttributeUnit* unit) noexcept;
  void         removeAttribute(int index) noexcept;
  void         moveAttributeUp(int index) noexcept;
  void         moveAttributeDown(int index) noexcept;
  AttributeKey setKey(int index, const QString& key) noexcept;
  void         setType(int index, const AttributeType& type) noexcept;
  QString      setValue(int index, const QString& value) noexcept;
  void         setUnit(int index, const AttributeUnit* unit) noexcept;
  int          getRowOfTableCellWidget(QObject* obj) const noexcept;
  AttributeKey convertStringToKeyOrThrow(const QString& key) const;
  void         throwIfValueInvalid(const AttributeType& type,
                                   const QString&       value) const;

  // row index <-> attribute index conversion methods
  int  newAttributeRow() const noexcept { return mAttributeList.count(); }
  int  indexToRow(int index) const noexcept { return index; }
  int  rowToIndex(int row) const noexcept { return row; }
  bool isExistingAttributeRow(int row) const noexcept {
    return row >= 0 && row < mAttributeList.count();
  }
  bool isNewAttributeRow(int row) const noexcept {
    return row == newAttributeRow();
  }

private:  // Data
  QTableWidget* mTable;
  AttributeList mAttributeList;
  const Attribute*
      mSelectedAttribute;  ///< do NOT dereference it (could be dangling)!
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_ATTRIBUTELISTEDITORWIDGET_H
