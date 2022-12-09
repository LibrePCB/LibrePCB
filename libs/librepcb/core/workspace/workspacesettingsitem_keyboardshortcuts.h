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

#ifndef LIBREPCB_CORE_WORKSPACESETTINGSITEM_KEYBOARDSHORTCUTS_H
#define LIBREPCB_CORE_WORKSPACESETTINGSITEM_KEYBOARDSHORTCUTS_H

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
 *  Class WorkspaceSettingsItem_KeyboardShortcuts
 ******************************************************************************/

/**
 * @brief Implementation of ::librepcb::WorkspaceSettingsItem to store
 *        keyboard shortcuts settings
 */
class WorkspaceSettingsItem_KeyboardShortcuts final
  : public WorkspaceSettingsItem {
public:
  // Constructors / Destructor
  WorkspaceSettingsItem_KeyboardShortcuts() = delete;
  WorkspaceSettingsItem_KeyboardShortcuts(
      const WorkspaceSettingsItem_KeyboardShortcuts& other) = delete;
  explicit WorkspaceSettingsItem_KeyboardShortcuts(
      QObject* parent = nullptr) noexcept;
  ~WorkspaceSettingsItem_KeyboardShortcuts() noexcept;

  // Getters
  const QMap<QString, QList<QKeySequence>>& get() const noexcept {
    return mOverrides;
  }

  // Setters
  void set(const QMap<QString, QList<QKeySequence>>& overrides) noexcept;

  // Operator Overloadings
  WorkspaceSettingsItem_KeyboardShortcuts& operator=(
      const WorkspaceSettingsItem_KeyboardShortcuts& rhs) = delete;

private:  // Methods
  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::restoreDefaultImpl()
   */
  virtual void restoreDefaultImpl() noexcept override;

  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::loadImpl()
   */
  void loadImpl(const SExpression& root) override;

  /**
   * @copydoc ::librepcb::WorkspaceSettingsItem::serializeImpl()
   */
  void serializeImpl(SExpression& root) const override;

private:
  QMap<QString, SExpression> mNodes;
  QMap<QString, QList<QKeySequence>> mOverrides;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
