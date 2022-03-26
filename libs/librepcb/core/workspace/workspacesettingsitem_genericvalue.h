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

#ifndef LIBREPCB_CORE_WORKSPACESETTINGSITEM_GENERICVALUE_H
#define LIBREPCB_CORE_WORKSPACESETTINGSITEM_GENERICVALUE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "workspacesettingsitem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class WorkspaceSettingsItem_GenericValue
 ******************************************************************************/

/**
 * @brief Generic implementation of ::librepcb::WorkspaceSettingsItem
 *        for simple, value-type settings
 */
template <typename T>
class WorkspaceSettingsItem_GenericValue final : public WorkspaceSettingsItem {
public:
  // Constructors / Destructor
  WorkspaceSettingsItem_GenericValue() = delete;
  WorkspaceSettingsItem_GenericValue(
      const WorkspaceSettingsItem_GenericValue& other) = delete;
  explicit WorkspaceSettingsItem_GenericValue(
      const QString& key, const T& defaultValue,
      QObject* parent = nullptr) noexcept
    : WorkspaceSettingsItem(key, parent),
      mDefaultValue(defaultValue),
      mCurrentValue(defaultValue) {}
  ~WorkspaceSettingsItem_GenericValue() noexcept {}

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
    if (value != mCurrentValue) {
      mCurrentValue = value;
      valueModified();
    }
  }

  /**
   * @brief Get the default value
   *
   * @return Default value
   */
  const T& getDefault() const noexcept { return mDefaultValue; }

  // Operator Overloadings
  WorkspaceSettingsItem_GenericValue& operator=(
      const WorkspaceSettingsItem_GenericValue& rhs) = delete;

private:  // Methods
  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::restoreDefaultImpl()
   */
  virtual void restoreDefaultImpl() noexcept override { set(mDefaultValue); }

  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::loadImpl()
   */
  void loadImpl(const SExpression& root, const Version& fileFormat) override {
    set(deserialize<T>(root.getChild("@0"), fileFormat));  // can throw
  }

  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::serializeImpl()
   */
  void serializeImpl(SExpression& root) const override {
    root.appendChild(mCurrentValue);
  }

private:
  T mDefaultValue;  ///< Initial, default value
  T mCurrentValue;  ///< Current value
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
