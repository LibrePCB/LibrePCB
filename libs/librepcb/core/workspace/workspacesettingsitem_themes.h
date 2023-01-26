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

#ifndef LIBREPCB_CORE_WORKSPACESETTINGSITEM_THEMES_H
#define LIBREPCB_CORE_WORKSPACESETTINGSITEM_THEMES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "theme.h"
#include "workspacesettingsitem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class WorkspaceSettingsItem_Themes
 ******************************************************************************/

/**
 * @brief Implementation of ::librepcb::WorkspaceSettingsItem to store
 *        theme configurations
 */
class WorkspaceSettingsItem_Themes final : public WorkspaceSettingsItem {
  Q_OBJECT

public:
  // Constructors / Destructor
  WorkspaceSettingsItem_Themes() = delete;
  WorkspaceSettingsItem_Themes(const WorkspaceSettingsItem_Themes& other) =
      delete;
  explicit WorkspaceSettingsItem_Themes(QObject* parent = nullptr) noexcept;
  ~WorkspaceSettingsItem_Themes() noexcept;

  // Getters
  const QMap<Uuid, Theme>& getAll() const noexcept { return mThemes; }
  const Uuid& getActiveUuid() const noexcept { return mActiveUuid; }
  const Theme& getActive() const noexcept { return mActiveTheme; }

  // Setters
  void setAll(const QMap<Uuid, Theme>& themes) noexcept;
  void setActiveUuid(const Uuid& uuid) noexcept;

  // Operator Overloadings
  WorkspaceSettingsItem_Themes& operator=(
      const WorkspaceSettingsItem_Themes& rhs) = delete;

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

  void addTheme(const Theme& theme) noexcept;
  void updateActiveTheme() noexcept;

private:
  QMap<Uuid, Theme> mThemes;
  Uuid mActiveUuid;
  Theme mActiveTheme;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
