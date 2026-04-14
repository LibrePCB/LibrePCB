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

#ifndef LIBREPCB_CORE_USERCOLORSCHEME_H
#define LIBREPCB_CORE_USERCOLORSCHEME_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/sexpression.h"
#include "basecolorscheme.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class UserColorScheme
 ******************************************************************************/

/**
 * @brief User-defined color schemes as required by
 *        ::librepcb::WorkspaceSettingsItem_ColorSchemes
 */
class UserColorScheme final : public QObject, public ColorScheme {
  Q_OBJECT

public:
  // Constructors / Destructor
  UserColorScheme(const Uuid& uuid, const QString& name,
                  const BaseColorScheme& base) noexcept;
  UserColorScheme(const Uuid& uuid, const QString& name,
                  const UserColorScheme& copyFrom) noexcept;
  UserColorScheme(const SExpression& root,
                  const QVector<const BaseColorScheme*>& bases);
  UserColorScheme(const UserColorScheme& other) noexcept;
  ~UserColorScheme() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept override { return mUuid; }
  const QString& getName() const noexcept override { return mName; }
  const BaseColorScheme& getBase() const noexcept { return *mBase; }
  QColor getPrimaryOverride(const QString& role) const noexcept {
    return mPrimaryColorOverrides.value(role);
  }
  QColor getSecondaryOverride(const QString& role) const noexcept {
    return mSecondaryColorOverrides.value(role);
  }
  std::optional<Colors> tryGetColors(
      const QString& role) const noexcept override;

  // Setters
  void setName(const QString& name) noexcept;
  void setPrimary(const QString& role, const QColor& color) noexcept;
  void setSecondary(const QString& role, const QColor& color) noexcept;

  // General Methods
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const UserColorScheme& rhs) const noexcept;
  bool operator!=(const UserColorScheme& rhs) const noexcept {
    return !(*this == rhs);
  }
  UserColorScheme& operator=(const UserColorScheme& rhs) noexcept;

signals:
  void modified();
  void colorsModified(const QString& role);

private:
  QVector<SExpression> mLoadedNodes;
  QSet<QString> mModifiedColors;

  Uuid mUuid;
  QString mName;
  Uuid mBaseUuid;
  const BaseColorScheme* mBase;
  QHash<QString, QColor> mPrimaryColorOverrides;
  QHash<QString, QColor> mSecondaryColorOverrides;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
