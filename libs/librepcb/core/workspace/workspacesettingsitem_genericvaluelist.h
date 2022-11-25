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

#ifndef LIBREPCB_CORE_WORKSPACESETTINGSITEM_GENERICVALUELIST_H
#define LIBREPCB_CORE_WORKSPACESETTINGSITEM_GENERICVALUELIST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "workspacesettingsitem.h"

#include <librepcb/core/utils/toolbox.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class WorkspaceSettingsItem_GenericValueList
 ******************************************************************************/

/**
 * @brief Generic implementation of ::librepcb::WorkspaceSettingsItem
 *        for simple, value-in-list-type settings
 */
template <typename T>
class WorkspaceSettingsItem_GenericValueList final
  : public WorkspaceSettingsItem {
public:
  // Constructors / Destructor
  WorkspaceSettingsItem_GenericValueList() = delete;
  WorkspaceSettingsItem_GenericValueList(
      const WorkspaceSettingsItem_GenericValueList& other) = delete;
  explicit WorkspaceSettingsItem_GenericValueList(
      const QString& listKey, const QString& itemKey, const T& defaultValue,
      QObject* parent = nullptr) noexcept
    : WorkspaceSettingsItem(listKey, parent),
      mItemKey(itemKey),
      mDefaultValue(defaultValue),
      mCurrentValue(defaultValue) {}
  ~WorkspaceSettingsItem_GenericValueList() noexcept {}

  /**
   * @brief Get the current value
   *
   * @return Current value
   */
  const T& get() const noexcept { return mCurrentValue; }

  /**
   * @brief Check if the current value contains a praticular item
   *
   * @param item    Item to check.
   *
   * @return Whether the item is contained in the current value or not.
   */
  bool contains(const typename T::value_type& item) const noexcept {
    return mCurrentValue.contains(item);
  }

  /**
   * @brief Set the value
   *
   * @param value   The new value
   */
  void set(const T& value) noexcept {
    if (value != mCurrentValue) {
      mCurrentValue = value;
      valueModified();
    }
  }

  /**
   * @brief Add a single item to the value list
   *
   * @param item    The item to append (list) or insert (unordered set)
   */
  void add(const typename T::value_type& item) noexcept {
    auto newValue = mCurrentValue;
    newValue << item;
    set(newValue);
  }

  /**
   * @brief Get the default value
   *
   * @return Default value
   */
  const T& getDefault() const noexcept { return mDefaultValue; }

  // Operator Overloadings
  WorkspaceSettingsItem_GenericValueList& operator=(
      const WorkspaceSettingsItem_GenericValueList& rhs) = delete;

private:  // Methods
  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::restoreDefaultImpl()
   */
  virtual void restoreDefaultImpl() noexcept override { set(mDefaultValue); }

  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::loadImpl()
   */
  void loadImpl(const SExpression& root, const Version& fileFormat) override {
    T values;  // temporary object to make this method atomic
    foreach (const SExpression& child, root.getChildren(mItemKey)) {
      values << deserialize<typename T::value_type>(child.getChild("@0"),
                                                    fileFormat);
    }
    set(values);
  }

  /**
   * @brief Helper for serialization of QList
   *
   * @param value   QList to serialize.
   *
   * @return Unmodified value.
   */
  T makeCanonical(const QList<typename T::value_type>& value) const noexcept {
    return value;  // Do not sort QList since the order is relevant!
  }

  /**
   * @brief Helper for serialization of QSet
   *
   * @param value   QSet to serialize.
   *
   * @return QSet as a sorted QList.
   */
  QList<typename T::value_type> makeCanonical(
      const QSet<typename T::value_type>& value) const noexcept {
    return Toolbox::sortedQSet(value);  // Make file format canonical.
  }

  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::serializeImpl()
   */
  void serializeImpl(SExpression& root) const override {
    foreach (const auto& item, makeCanonical(mCurrentValue)) {
      root.ensureLineBreak();
      root.appendChild(mItemKey, item);
    }
    root.ensureLineBreak();
  }

private:
  QString mItemKey;  ///< Inner key used for serialization
  T mDefaultValue;  ///< Initial, default value
  T mCurrentValue;  ///< Current value
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
