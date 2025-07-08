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

#ifndef LIBREPCB_EDITOR_PACKAGEPADLISTMODEL2_H
#define LIBREPCB_EDITOR_PACKAGEPADLISTMODEL2_H

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
 *  Class PackagePadListModel2
 ******************************************************************************/

/**
 * @brief The PackagePadListModel2 class
 */
class PackagePadListModel2 : public QObject,
                             public slint::Model<ui::PackagePadData> {
  Q_OBJECT

public:
  // Constructors / Destructor
  // PackagePadListModel2() = delete;
  PackagePadListModel2(const PackagePadListModel2& other) = delete;
  explicit PackagePadListModel2(QObject* parent = nullptr) noexcept;
  virtual ~PackagePadListModel2() noexcept;

  // General Methods
  void setList(PackagePadList* list) noexcept;
  void setUndoStack(UndoStack* stack) noexcept;
  bool add(const QStringList& names) noexcept;
  void apply();

  // Implementations
  std::size_t row_count() const override;
  std::optional<ui::PackagePadData> row_data(std::size_t i) const override;
  void set_row_data(std::size_t i,
                    const ui::PackagePadData& data) noexcept override;

  // Operator Overloadings
  PackagePadListModel2& operator=(const PackagePadListModel2& rhs) = delete;

private:
  ui::PackagePadData createItem(const PackagePad& sig) noexcept;
  void listEdited(const PackagePadList& list, int index,
                  const std::shared_ptr<const PackagePad>& item,
                  PackagePadList::Event event) noexcept;
  void execCmd(UndoCommand* cmd);
  static void throwDuplicateNameError(const QString& name);
  static QString cleanForcedNetName(const QString& name) noexcept;

private:
  PackagePadList* mList;
  UndoStack* mUndoStack;

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
