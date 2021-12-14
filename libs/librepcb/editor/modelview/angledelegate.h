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

#ifndef LIBREPCB_EDITOR_ANGLEDELEGATE_H
#define LIBREPCB_EDITOR_ANGLEDELEGATE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/lengthunit.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class AngleDelegate
 ******************************************************************************/

/**
 * @brief Subclass of QStyledItemDelegate to display/edit librepcb::Angle values
 */
class AngleDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit AngleDelegate(QObject* parent = nullptr) noexcept;
  AngleDelegate(const AngleDelegate& other) = delete;
  ~AngleDelegate() noexcept;

  // Inherited from QStyledItemDelegate
  QString displayText(const QVariant& value,
                      const QLocale& locale) const override;
  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const override;
  void setEditorData(QWidget* editor, const QModelIndex& index) const override;
  void setModelData(QWidget* editor, QAbstractItemModel* model,
                    const QModelIndex& index) const override;
  void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                            const QModelIndex& index) const override;

  // Operator Overloadings
  AngleDelegate& operator=(const AngleDelegate& rhs) = delete;

private:  // Methods
  void editingFinished() noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
