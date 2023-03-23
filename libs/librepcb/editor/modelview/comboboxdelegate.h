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

#ifndef LIBREPCB_EDITOR_COMBOBOXDELEGATE_H
#define LIBREPCB_EDITOR_COMBOBOXDELEGATE_H

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
 *  Class ComboBoxDelegate
 ******************************************************************************/

/**
 * @brief Subclass of QStyledItemDelegate which uses QComboBox as item editor
 */
class ComboBoxDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  // Types
  struct Item final {
    QString text;
    QIcon icon;
    QVariant data;
  };
  struct Items final : public QVector<Item> {
    void sort() noexcept;
  };

  // Constructors / Destructor
  explicit ComboBoxDelegate(bool editable = false,
                            QObject* parent = nullptr) noexcept;
  ComboBoxDelegate(const ComboBoxDelegate& other) = delete;
  ~ComboBoxDelegate() noexcept;

  // Inherited from QStyledItemDelegate
  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
  void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const override;

  // Operator Overloadings
  ComboBoxDelegate& operator=(const ComboBoxDelegate& rhs) = delete;

private:  // Data
  bool mEditable;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::ComboBoxDelegate::Items)

#endif
