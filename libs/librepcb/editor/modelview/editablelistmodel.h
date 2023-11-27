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

#ifndef LIBREPCB_EDITOR_EDITABLELISTMODEL_H
#define LIBREPCB_EDITOR_EDITABLELISTMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "comboboxdelegate.h"

#include <librepcb/core/types/uuid.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>
#include <QtWidgets>

#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Types
 ******************************************************************************/

enum class EditableListModelType { DEFAULT, LOCALE };

/*******************************************************************************
 *  Class EditableListModel
 ******************************************************************************/

/**
 * @brief A generic QAbstractTableModel subclass to view and edit list of
 *        various data types
 *
 * This class is similar to QStringListModel, but supports various other item
 * types than QString.
 *
 * Special Features:
 *   - Adds an additional column and row to allow modifying the model with
 *     ::librepcb::editor::EditableTableWidget.
 *   - Automatic pretty printing and input validation for various data types:
 *     - Locales, e.g. "de_DE" -> "Deutsch (Deutschland)"
 *     - URLs
 *   - Optionally providing a QComboBox to choose from a predefined set of
 *     values (#setChoices()). Values are automatically sorted by their text.
 *   - Support custom display text (#setDisplayText()).
 *   - Support icons (#setIcon()).
 *   - Duplicate values are revoked.
 */
template <typename T,
          EditableListModelType TYPE = EditableListModelType::DEFAULT>
class EditableListModel final : public QAbstractTableModel {
  typedef typename T::value_type ValueType;

public:
  enum Column { COLUMN_TEXT, COLUMN_ACTIONS, _COLUMN_COUNT };

  // Constructors / Destructor
  EditableListModel(const EditableListModel& other) noexcept = delete;
  explicit EditableListModel(QObject* parent = nullptr) noexcept
    : QAbstractTableModel(parent),
      mChoices(),
      mValues(),
      mDefaultValue(),
      mNewValue(),
      mPlaceholderText(),
      mComboBoxItems() {}
  ~EditableListModel() noexcept {}

  // Getters
  const T& getValues() const noexcept { return mValues; }

  // Setters
  void setDefaultValue(const ValueType& value) noexcept {
    mDefaultValue = value;
    mNewValue = value;
  }

  void setPlaceholderText(const QString& text) noexcept {
    mPlaceholderText = text;
    emit dataChanged(index(mValues.count(), COLUMN_TEXT),
                     index(mValues.count(), COLUMN_TEXT));
  }

  void setValues(const T& values) noexcept {
    emit beginResetModel();
    mValues = values;
    emit endResetModel();
  }

  void setChoices(const T& choices) noexcept {
    emit beginResetModel();
    mChoices = choices;
    updateComboBoxItems();
    emit endResetModel();
  }

  void setDisplayText(const ValueType& value, const QString& text) noexcept {
    emit beginResetModel();
    mDisplayTexts[value] = text;
    emit endResetModel();
  }

  void setIcon(const ValueType& value, const QIcon& icon) noexcept {
    emit beginResetModel();
    mIcons[value] = icon;
    emit endResetModel();
  }

  // Slots
  void add(const QPersistentModelIndex& itemIndex) noexcept {
    Q_UNUSED(itemIndex);

    if (!mNewValue) {
      QMessageBox::critical(nullptr, tr("Error"), tr("Invalid value."));
      return;
    }

    if (mValues.contains(*mNewValue)) {
      QMessageBox::critical(nullptr, tr("Error"),
                            tr("Value already contained in list."));
      return;
    }

    beginInsertRows(QModelIndex(), mValues.count(), mValues.count());
    mValues.append(*mNewValue);
    endInsertRows();

    mNewValue = mDefaultValue;
    emit dataChanged(index(mValues.count(), 0),
                     index(mValues.count(), _COLUMN_COUNT - 1));
  }

  void remove(const QPersistentModelIndex& itemIndex) noexcept {
    int row = itemIndex.data(Qt::EditRole).toInt();
    if ((row >= 0) && (row < mValues.count())) {
      beginRemoveRows(QModelIndex(), row, row);
      mValues.removeAt(row);
      endRemoveRows();
    }
  }

  void moveUp(const QPersistentModelIndex& itemIndex) noexcept {
    int row = itemIndex.data(Qt::EditRole).toInt();
    if (row >= 1) {
      mValues.move(row, row - 1);
      emit dataChanged(index(row - 1, 0), index(row, _COLUMN_COUNT - 1));
    }
  }

  void moveDown(const QPersistentModelIndex& itemIndex) noexcept {
    int row = itemIndex.data(Qt::EditRole).toInt();
    if (row < (mValues.count() - 1)) {
      mValues.move(row, row + 1);
      emit dataChanged(index(row, 0), index(row + 1, _COLUMN_COUNT - 1));
    }
  }

  // Inherited from QAbstractItemModel
  int rowCount(const QModelIndex& parent = QModelIndex()) const override {
    if (!parent.isValid()) {
      return mValues.count() + 1;
    }
    return 0;
  }

  int columnCount(const QModelIndex& parent = QModelIndex()) const override {
    if (!parent.isValid()) {
      return _COLUMN_COUNT;
    }
    return 0;
  }

  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override {
    if (!index.isValid()) {
      return QVariant();
    }

    tl::optional<ValueType> value = mNewValue;
    if ((index.row() >= 0) && (index.row() < mValues.count())) {
      value = mValues.at(index.row());
    }
    bool showPlaceholder = (index.row() == mValues.count()) &&
        ((!value) || getDisplayText(*value).isEmpty());
    switch (index.column()) {
      case COLUMN_TEXT: {
        switch (role) {
          case Qt::DisplayRole:
            return showPlaceholder
                ? mPlaceholderText
                : (value ? getDisplayText(*value) : QString());
          case Qt::DecorationRole:
            return value ? mIcons.value(*value) : QIcon();
          case Qt::EditRole:
            return value ? getDataForValue(*value) : QVariant();
          case Qt::ForegroundRole:
            if (showPlaceholder) {
              QColor color = qApp->palette().text().color();
              color.setAlpha(128);
              return QBrush(color);
            } else {
              return QVariant();
            }
          case Qt::UserRole:
            return QVariant::fromValue(mComboBoxItems);
          default:
            return QVariant();
        }
      }
      case COLUMN_ACTIONS: {
        switch (role) {
          case Qt::EditRole:
            return index.row();
          default:
            return QVariant();
        }
      }
      default:
        return QVariant();
    }

    return QVariant();
  }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override {
    if (orientation == Qt::Horizontal) {
      if (role == Qt::DisplayRole) {
        switch (section) {
          case COLUMN_TEXT:
            return tr("Item");
          default:
            return QVariant();
        }
      }
    } else if (orientation == Qt::Vertical) {
      bool isLastRow = section >= mValues.count();
      if (role == Qt::DisplayRole) {
        return isLastRow ? tr("New:") : QString::number(section + 1);
      } else if (role == Qt::TextAlignmentRole) {
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
      }
    }
    return QVariant();
  }

  Qt::ItemFlags flags(const QModelIndex& index) const override {
    Qt::ItemFlags f = QAbstractTableModel::flags(index);
    if (index.isValid() && (index.row() == mValues.count())) {
      f |= Qt::ItemIsEditable;
    }
    return f;
  }

  bool setData(const QModelIndex& index, const QVariant& value,
               int role = Qt::EditRole) override {
    bool isLastRow = index.row() >= mValues.count();

    if ((index.column() == COLUMN_TEXT) && role == Qt::EditRole) {
      if (isLastRow) {
        mNewValue = convertInputValue(value, mDefaultValue);
      }
      emit dataChanged(index, index);
      return true;
    }

    return false;
  }

  // Operator Overloadings
  EditableListModel& operator=(const EditableListModel& rhs) noexcept;

private:  // Methods
  QString getDisplayText(const Uuid& value) const noexcept {
    return mDisplayTexts.value(value, value.toStr());
  }

  QString getDisplayText(const QUrl& value) const noexcept {
    return value.toDisplayString();
  }

  QString getDisplayText(const QString& value) const noexcept {
    switch (TYPE) {
      case EditableListModelType::LOCALE:
        return Toolbox::prettyPrintLocale(value);
      default:
        return mDisplayTexts.value(value, value);
    }
  }

  QVariant getDataForValue(const Uuid& value) const noexcept {
    return value.toStr();
  }

  QVariant getDataForValue(const QUrl& value) const noexcept { return value; }

  QVariant getDataForValue(const QString& value) const noexcept {
    return value;
  }

  tl::optional<Uuid> convertInputValue(
      const QVariant& input, const tl::optional<Uuid>& tag) const noexcept {
    Q_UNUSED(tag);  // used only for template tag dispatching
    return Uuid::tryFromString(input.toString());
  }

  tl::optional<QString> convertInputValue(
      const QVariant& input, const tl::optional<QString>& tag) const noexcept {
    Q_UNUSED(tag);  // used only for template tag dispatching
    QString str = input.toString().trimmed();
    return str.isEmpty() ? tl::nullopt : tl::make_optional(str);
  }

  tl::optional<QUrl> convertInputValue(
      const QVariant& input, const tl::optional<QUrl>& tag) const noexcept {
    Q_UNUSED(tag);  // used only for template tag dispatching
    QUrl url = QUrl::fromUserInput(input.toString());
    return url.isValid() ? tl::make_optional(url) : tl::nullopt;
  }

  void updateComboBoxItems() noexcept {
    mComboBoxItems.clear();
    foreach (const ValueType& choice, mChoices) {
      ComboBoxDelegate::Item item{getDisplayText(choice), mIcons[choice],
                                  getDataForValue(choice)};
      mComboBoxItems.append(item);
    }
    mComboBoxItems.sort();
  }

private:  // Data
  T mChoices;
  T mValues;
  tl::optional<ValueType> mDefaultValue;
  tl::optional<ValueType> mNewValue;
  QString mPlaceholderText;
  QHash<ValueType, QString> mDisplayTexts;
  QHash<ValueType, QIcon> mIcons;
  ComboBoxDelegate::Items mComboBoxItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
