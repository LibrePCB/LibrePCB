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

#ifndef LIBREPCB_EDITOR_ATTRIBUTELISTMODEL_H
#define LIBREPCB_EDITOR_ATTRIBUTELISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/attribute/attribute.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class UndoCommand;
class UndoStack;

/*******************************************************************************
 *  Class AttributeListModel
 ******************************************************************************/

/**
 * @brief The AttributeListModel class
 */
class AttributeListModel final : public QObject,
                                 public slint::Model<ui::AttributeData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // AttributeListModel() = delete;
  AttributeListModel(const AttributeListModel& other) = delete;
  explicit AttributeListModel(QObject* parent = nullptr) noexcept;
  ~AttributeListModel() noexcept;

  // General Methods
  void setReferences(AttributeList* list, UndoStack* stack) noexcept;
  void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::AttributeData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::AttributeData& data) noexcept override;

  // Operator Overloadings
  AttributeListModel& operator=(const AttributeListModel& rhs) = delete;

private:
  ui::AttributeData createItem(const Attribute& obj) noexcept;
  static ui::AttributeData createLastItem() noexcept;
  void trigger(int index, std::shared_ptr<Attribute> obj,
               ui::AttributeAction a) noexcept;
  void listEdited(const AttributeList& list, int index,
                  const std::shared_ptr<const Attribute>& item,
                  AttributeList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  AttributeKey validateKeyOrThrow(const QString& name) const;

private:
  AttributeList* mList;
  QPointer<UndoStack> mUndoStack;

  QList<ui::AttributeData> mItems;

  // Slots
  AttributeList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
