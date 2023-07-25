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

#ifndef LIBREPCB_EDITOR_CHECKABLEITEMSEDITORWIDGET_H
#define LIBREPCB_EDITOR_CHECKABLEITEMSEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class CheckableItemsEditorWidget
 ******************************************************************************/

/**
 * @brief A widget to modify a list of checkable string items
 *
 * Used for ::librepcb::editor::CheckableItemsDelegate.
 */
class CheckableItemsEditorWidget final : public QFrame {
  Q_OBJECT

public:
  /// Key, text, check state
  typedef std::tuple<QVariant, QString, Qt::CheckState> Item;
  typedef QVector<Item> ItemList;

  // Constructors / Destructor
  CheckableItemsEditorWidget() = delete;
  explicit CheckableItemsEditorWidget(QWidget* parent = nullptr) noexcept;
  CheckableItemsEditorWidget(const CheckableItemsEditorWidget& other) = delete;
  ~CheckableItemsEditorWidget() noexcept;

  // General Methods
  const ItemList& getItems() const noexcept { return mItems; }
  void setItems(const ItemList& items) noexcept;

  // Operator Overloadings
  CheckableItemsEditorWidget& operator=(const CheckableItemsEditorWidget& rhs) =
      delete;

private:  // Methods
  void updateWidgets() noexcept;

private:  // Data
  QPointer<QVBoxLayout> mLayout;
  ItemList mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::CheckableItemsEditorWidget::Item)
Q_DECLARE_METATYPE(librepcb::editor::CheckableItemsEditorWidget::ItemList)

#endif
