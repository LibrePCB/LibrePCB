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

#ifndef LIBREPCB_EDITOR_COMPONENTVARIANTLISTMODEL_H
#define LIBREPCB_EDITOR_COMPONENTVARIANTLISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

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
 *  Class ComponentVariantListModel
 ******************************************************************************/

/**
 * @brief The ComponentVariantListModel class
 */
class ComponentVariantListModel
  : public QObject,
    public slint::Model<ui::ComponentVariantData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // ComponentVariantListModel() = delete;
  ComponentVariantListModel(const ComponentVariantListModel& other) = delete;
  explicit ComponentVariantListModel(QObject* parent = nullptr) noexcept;
  virtual ~ComponentVariantListModel() noexcept;

  // General Methods
  void setVariantList(ComponentSymbolVariantList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;
  // bool add(const QStringList& names) noexcept;
  void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::ComponentVariantData> row_data(
      std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::ComponentVariantData& data) noexcept override;

  // Operator Overloadings
  ComponentVariantListModel& operator=(const ComponentVariantListModel& rhs) =
      delete;

private:
  ui::ComponentVariantData createItem(ComponentSymbolVariant& variant) noexcept;
  void variantListEdited(
      const ComponentSymbolVariantList& list, int index,
      const std::shared_ptr<const ComponentSymbolVariant>& variant,
      ComponentSymbolVariantList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  static void throwDuplicateNameError(const QString& name);
  static QString cleanForcedNetName(const QString& name) noexcept;

private:
  ComponentSymbolVariantList* mVariantList;
  UndoStack* mUndoStack;

  QList<ui::ComponentVariantData> mItems;

  // Slots
  ComponentSymbolVariantList::OnEditedSlot mOnEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
