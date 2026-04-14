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
#include "workspacesettingsitem_colorschemes.h"

#include "basecolorscheme.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceSettingsItem_ColorSchemes::WorkspaceSettingsItem_ColorSchemes(
    const char* kind, QObject* parent) noexcept
  : WorkspaceSettingsItem(kind, parent),
    mActiveUuid(Uuid::createRandom()),
    mActiveScheme(nullptr) {
  if (kind == Kind::sSchematic) {
    mBaseSchemes.append(&BaseColorScheme::schematicLibrePcbLight());  // Default
  } else if (kind == Kind::sBoard) {
    mBaseSchemes.append(&BaseColorScheme::boardLibrePcbDark());  // Default
  } else if (kind == Kind::s3d) {
    mBaseSchemes.append(&BaseColorScheme::view3dLibrePcbDefault());  // Default
  } else {
    qFatal("Unknown color scheme kind: %s", kind);
  }
  mActiveUuid = mBaseSchemes.first()->getUuid();
  updateActive();
}

WorkspaceSettingsItem_ColorSchemes::
    ~WorkspaceSettingsItem_ColorSchemes() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QMap<Uuid, UserColorScheme> WorkspaceSettingsItem_ColorSchemes::getUserSchemes()
    const noexcept {
  QMap<Uuid, UserColorScheme> schemes;
  for (auto scheme : mUserSchemes) {
    schemes.insert(scheme->getUuid(), *scheme);
  }
  return schemes;
}

std::shared_ptr<UserColorScheme>
    WorkspaceSettingsItem_ColorSchemes::getUserScheme(
        const Uuid& uuid) noexcept {
  return mUserSchemes.value(uuid);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void WorkspaceSettingsItem_ColorSchemes::setUserSchemes(
    const QMap<Uuid, UserColorScheme>& schemes) noexcept {
  if (schemes != getUserSchemes()) {
    mUserSchemes.clear();
    for (const auto& scheme : schemes) {
      mUserSchemes.insert(scheme.getUuid(),
                          std::make_shared<UserColorScheme>(scheme));
    }
    updateActive();
    valueModified();
  }
}

void WorkspaceSettingsItem_ColorSchemes::setActiveUuid(
    const Uuid& uuid) noexcept {
  if (uuid != mActiveUuid) {
    mActiveUuid = uuid;
    updateActive();
    valueModified();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void WorkspaceSettingsItem_ColorSchemes::restoreDefaultImpl() noexcept {
  mUserSchemes.clear();
  mActiveUuid = mBaseSchemes.first()->getUuid();
  updateActive();
  valueModified();
}

void WorkspaceSettingsItem_ColorSchemes::loadImpl(const SExpression& root) {
  // Temporary objects to make this method atomic.
  QMap<Uuid, UserColorScheme> schemes;
  foreach (const SExpression* child, root.getChildren("scheme")) {
    UserColorScheme scheme(*child, mBaseSchemes);  // can throw
    schemes.insert(scheme.getUuid(), scheme);
  }
  const Uuid active = deserialize<Uuid>(root.getChild("active/@0"));

  setUserSchemes(schemes);
  setActiveUuid(active);
}

void WorkspaceSettingsItem_ColorSchemes::serializeImpl(
    SExpression& root) const {
  root.ensureLineBreak();
  root.appendChild("active", mActiveUuid).appendChild(mActiveScheme->getName());
  foreach (auto scheme, mUserSchemes) {
    root.ensureLineBreak();
    scheme->serialize(root.appendList("scheme"));
  }
  root.ensureLineBreak();
}

void WorkspaceSettingsItem_ColorSchemes::updateActive() noexcept {
  auto findActiveScheme = [this]() {
    if (auto scheme = mUserSchemes.value(mActiveUuid)) {
      return static_cast<const ColorScheme*>(scheme.get());
    }
    for (const BaseColorScheme* scheme : mBaseSchemes) {
      if (scheme->getUuid() == mActiveUuid) {
        return static_cast<const ColorScheme*>(scheme);
      }
    }
    return static_cast<const ColorScheme*>(mBaseSchemes.first());
  };

  while (!mActiveConnections.isEmpty()) {
    disconnect(mActiveConnections.takeLast());
  }

  mActiveScheme = findActiveScheme();

  if (auto s = dynamic_cast<const UserColorScheme*>(mActiveScheme)) {
    mActiveConnections.append(
        connect(s, &UserColorScheme::modified, this,
                &WorkspaceSettingsItem_ColorSchemes::valueModified));
    mActiveConnections.append(
        connect(s, &UserColorScheme::colorsModified, this,
                &WorkspaceSettingsItem_ColorSchemes::colorsModified));
  }

  emit colorsModified();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
