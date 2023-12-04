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

#ifndef LIBREPCB_EDITOR_KEYBOARDSHORTCUTSMODEL_H
#define LIBREPCB_EDITOR_KEYBOARDSHORTCUTSMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class EditorCommand;
class EditorCommandCategory;

/*******************************************************************************
 *  Class KeyboardShortcutsModel
 ******************************************************************************/

/**
 * @brief A QAbstractItemModel to represent keyboard shortcut workspace settings
 */
class KeyboardShortcutsModel final : public QAbstractItemModel {
  struct Category {
    const EditorCommandCategory* category;
    QList<const EditorCommand*> commands;
  };

public:
  // Constructors / Destructor
  KeyboardShortcutsModel() = delete;
  KeyboardShortcutsModel(const KeyboardShortcutsModel& other) = delete;
  explicit KeyboardShortcutsModel(QObject* parent = nullptr) noexcept;
  ~KeyboardShortcutsModel() noexcept;

  // General Methods
  const QMap<QString, QList<QKeySequence>>& getOverrides() const noexcept {
    return mOverrides;
  }
  void setOverrides(
      const QMap<QString, QList<QKeySequence>>& overrides) noexcept;

  // Inherited Methods
  int columnCount(
      const QModelIndex& parent = QModelIndex()) const noexcept override;
  int rowCount(
      const QModelIndex& parent = QModelIndex()) const noexcept override;
  QModelIndex index(
      int row, int column,
      const QModelIndex& parent = QModelIndex()) const noexcept override;
  QModelIndex parent(const QModelIndex& index) const noexcept override;
  Qt::ItemFlags flags(const QModelIndex& index) const noexcept override;
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const noexcept override;
  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) noexcept override;

  // Operator Overloadings
  KeyboardShortcutsModel& operator=(const KeyboardShortcutsModel& rhs) = delete;

private:  // Methods
  Category* categoryFromIndex(const QModelIndex& index) const noexcept;
  const EditorCommand* commandFromIndex(
      const QModelIndex& index) const noexcept;
  static QString format(const QList<QKeySequence>& sequences,
                        bool showNone) noexcept;

private:  // Data
  QList<Category*> mCategories;
  QMap<QString, QList<QKeySequence>> mOverrides;
};

}  // namespace editor
}  // namespace librepcb

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
