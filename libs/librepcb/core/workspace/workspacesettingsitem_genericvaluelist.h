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

#ifndef LIBREPCB_WORKSPACE_WORKSPACESETTINGSITEM_GENERICVALUELIST_H
#define LIBREPCB_WORKSPACE_WORKSPACESETTINGSITEM_GENERICVALUELIST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "workspacesettingsitem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Class WorkspaceSettingsItem_GenericValueList
 ******************************************************************************/

/**
 * @brief Generic implementation of ::librepcb::workspace::WorkspaceSettingsItem
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
    : WorkspaceSettingsItem(parent),
      mListKey(listKey),
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
   * @brief Set the value
   *
   * @param value   The new value
   */
  void set(const T& value) noexcept {
    mCurrentValue = value;
    emit edited();
  }

  /**
   * @brief Get the default value
   *
   * @return Default value
   */
  const T& getDefault() const noexcept { return mDefaultValue; }

  /**
   * @copydoc ::librepcb::workspace::WorkspaceSettingsItem::restoreDefault()
   */
  virtual void restoreDefault() noexcept override { set(mDefaultValue); }

  /**
   * @copydoc ::librepcb::workspace::WorkspaceSettingsItem::load()
   */
  void load(const SExpression& root, const Version& fileFormat) override {
    T values;  // temporary object to make this method atomic
    foreach (const SExpression& child,
             root.getChild(mListKey).getChildren(mItemKey)) {
      values.append(deserialize<typename T::value_type>(child.getChild("@0"),
                                                        fileFormat));
    }
    set(values);
  }

  /**
   * @copydoc ::librepcb::workspace::WorkspaceSettingsItem::serialize()
   */
  void serialize(SExpression& root) const override {
    SExpression& child = root.appendList(mListKey, true);
    foreach (const auto& item, mCurrentValue) {
      child.appendChild(mItemKey, item, true);
    }
  }

  // Operator Overloadings
  WorkspaceSettingsItem_GenericValueList& operator=(
      const WorkspaceSettingsItem_GenericValueList& rhs) = delete;

private:
  QString mListKey;  ///< Outer key used for serialization
  QString mItemKey;  ///< Inner key used for serialization
  T mDefaultValue;  ///< Initial, default value
  T mCurrentValue;  ///< Current value
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

#endif
