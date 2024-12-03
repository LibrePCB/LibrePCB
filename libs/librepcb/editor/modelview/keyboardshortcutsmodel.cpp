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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "keyboardshortcutsmodel.h"

#include "../editorcommandset.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

KeyboardShortcutsModel::KeyboardShortcutsModel(QObject* parent) noexcept
  : QAbstractItemModel(parent), mCategories(), mOverrides() {
  foreach (const EditorCommandCategory* category,
           EditorCommandSet::instance().getCategories()) {
    if (!category->isConfigurable()) {
      continue;
    }
    std::unique_ptr<Category> cat(new Category{category, {}});
    foreach (const EditorCommand* command,
             EditorCommandSet::instance().getCommands(category)) {
      cat->commands.append(command);
    }
    if (!cat->commands.isEmpty()) {
      mCategories.append(cat.release());
    }
  }
}

KeyboardShortcutsModel::~KeyboardShortcutsModel() noexcept {
  beginResetModel();
  qDeleteAll(mCategories);
  endResetModel();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KeyboardShortcutsModel::setOverrides(
    const QMap<QString, QList<QKeySequence>>& overrides) noexcept {
  beginResetModel();
  mOverrides = overrides;
  endResetModel();
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

int KeyboardShortcutsModel::columnCount(
    const QModelIndex& parent) const noexcept {
  Q_UNUSED(parent);
  return 3;
}

int KeyboardShortcutsModel::rowCount(const QModelIndex& parent) const noexcept {
  if (!parent.isValid()) {
    return mCategories.count();
  } else if (const Category* category = categoryFromIndex(parent)) {
    return category->commands.count();
  } else {
    return 0;
  }
}

QModelIndex KeyboardShortcutsModel::index(
    int row, int column, const QModelIndex& parent) const noexcept {
  if (!parent.isValid()) {
    return createIndex(row, column, nullptr);
  } else if (Category* category = categoryFromIndex(parent)) {
    return createIndex(row, column, category);
  } else {
    return QModelIndex();
  }
}

QModelIndex KeyboardShortcutsModel::parent(
    const QModelIndex& index) const noexcept {
  if (index.isValid()) {
    if (Category* category = static_cast<Category*>(index.internalPointer())) {
      int index = mCategories.indexOf(category);
      return createIndex(index, 0, nullptr);
    }
  }

  return QModelIndex();
}

Qt::ItemFlags KeyboardShortcutsModel::flags(
    const QModelIndex& index) const noexcept {
  Qt::ItemFlags flags = QAbstractItemModel::flags(index);
  if ((index.column() == 2) && (index.internalPointer())) {
    flags |= Qt::ItemIsEditable;
  }
  return flags;
}

QVariant KeyboardShortcutsModel::data(const QModelIndex& index,
                                      int role) const noexcept {
  if (const Category* category = categoryFromIndex(index)) {
    if (index.column() == 0) {
      switch (role) {
        case Qt::DisplayRole: {
          return category->category->getText();
        }
        case Qt::FontRole: {
          QFont font;
          font.setBold(true);
          return font;
        }
        default:
          break;
      }
    }
  } else if (const EditorCommand* command = commandFromIndex(index)) {
    switch (index.column()) {
      case 0: {
        switch (role) {
          case Qt::DisplayRole:
            return QString(command->getDisplayText());
          case Qt::DecorationRole: {
            QIcon icon = command->getIcon();
            if (icon.isNull()) {
              icon = QIcon(":/img/empty.png");  // Placeholder to align texts.
            }
            return icon;
          }
          default:
            break;
        }
        break;
      }
      case 1: {
        switch (role) {
          case Qt::DisplayRole:
            return command->getDescription();
          default:
            break;
        }
        break;
      }
      case 2: {
        const bool isOverridden = mOverrides.contains(command->getIdentifier());
        const QList<QKeySequence> overrideSequences =
            mOverrides.value(command->getIdentifier());
        const QList<QKeySequence>& defaultSequences =
            command->getDefaultKeySequences();
        switch (role) {
          case Qt::DisplayRole: {
            return format(isOverridden ? overrideSequences : defaultSequences,
                          isOverridden);
          }
          case Qt::ToolTipRole:
            return tr("Default") + ": " + format(defaultSequences, true);
          case Qt::FontRole: {
            QFont font;
            font.setBold(isOverridden);
            font.setItalic(!isOverridden);
            return font;
          }
          case Qt::EditRole:
            return isOverridden ? QVariant::fromValue(overrideSequences)
                                : QVariant();
          case Qt::UserRole:
            return QVariant::fromValue(defaultSequences);
          default:
            break;
        }
        break;
      }
      default: {
        break;
      }
    }
  }
  return QVariant();
}

bool KeyboardShortcutsModel::setData(const QModelIndex& index,
                                     const QVariant& value, int role) noexcept {
  if (const EditorCommand* command = commandFromIndex(index)) {
    if ((index.column() == 2) && (role == Qt::EditRole)) {
      if (value.isNull()) {
        mOverrides.remove(command->getIdentifier());  // Restore default.
      } else {
        mOverrides[command->getIdentifier()] =
            value.value<QList<QKeySequence>>();
      }
      emit dataChanged(index, index);
      return true;
    }
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

KeyboardShortcutsModel::Category* KeyboardShortcutsModel::categoryFromIndex(
    const QModelIndex& index) const noexcept {
  if ((index.model() == this) && (index.isValid()) &&
      (!index.internalPointer())) {
    return mCategories.value(index.row());
  } else {
    return nullptr;
  }
}

const EditorCommand* KeyboardShortcutsModel::commandFromIndex(
    const QModelIndex& index) const noexcept {
  if ((index.model() == this) && (index.isValid()) &&
      (index.internalPointer())) {
    Category* category = static_cast<Category*>(index.internalPointer());
    return category->commands.value(index.row());
  } else {
    return nullptr;
  }
}

QString KeyboardShortcutsModel::format(const QList<QKeySequence>& sequences,
                                       bool showNone) noexcept {
  if (sequences.isEmpty() && showNone) {
    return tr("None");
  } else {
    QStringList str;
    foreach (const QKeySequence& sequence, sequences) {
      str.append(sequence.toString(QKeySequence::NativeText));
    }
    return str.join(" | ");
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
