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

#ifndef LIBREPCB_CORE_WORKSPACESETTINGSITEM_COLORSCHEMES_H
#define LIBREPCB_CORE_WORKSPACESETTINGSITEM_COLORSCHEMES_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "usercolorscheme.h"
#include "workspacesettingsitem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BaseColorScheme;

/*******************************************************************************
 *  Class WorkspaceSettingsItem_ColorSchemes
 ******************************************************************************/

/**
 * @brief Implementation of ::librepcb::WorkspaceSettingsItem to store
 *        color scheme configurations
 */
class WorkspaceSettingsItem_ColorSchemes final : public WorkspaceSettingsItem {
  Q_OBJECT

public:
  struct Kind {
    static constexpr const char* sSchematic = "schematic_color_schemes";
    static constexpr const char* sBoard = "board_color_schemes";
    static constexpr const char* s3d = "3d_color_schemes";
  };

  // Constructors / Destructor
  WorkspaceSettingsItem_ColorSchemes() = delete;
  WorkspaceSettingsItem_ColorSchemes(
      const WorkspaceSettingsItem_ColorSchemes& other) = delete;
  explicit WorkspaceSettingsItem_ColorSchemes(
      const char* kind, QObject* parent = nullptr) noexcept;
  ~WorkspaceSettingsItem_ColorSchemes() noexcept;

  // Getters
  const QVector<const BaseColorScheme*>& getBaseSchemes() const noexcept {
    return mBaseSchemes;
  }
  QMap<Uuid, UserColorScheme> getUserSchemes() const noexcept;
  std::shared_ptr<UserColorScheme> getUserScheme(const Uuid& uuid) noexcept;
  const Uuid& getActiveUuid() const noexcept { return mActiveUuid; }
  const ColorScheme& getActive() const noexcept { return *mActiveScheme; }

  // Setters
  void setUserSchemes(const QMap<Uuid, UserColorScheme>& schemes) noexcept;
  void setActiveUuid(const Uuid& uuid) noexcept;

  // Operator Overloadings
  WorkspaceSettingsItem_ColorSchemes& operator=(
      const WorkspaceSettingsItem_ColorSchemes& rhs) = delete;

signals:
  void colorsModified();

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

  void updateActive() noexcept;

private:
  QVector<const BaseColorScheme*> mBaseSchemes;
  QMap<Uuid, std::shared_ptr<UserColorScheme>> mUserSchemes;
  Uuid mActiveUuid;
  const ColorScheme* mActiveScheme;
  QVector<QMetaObject::Connection> mActiveConnections;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
