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
#include "workspacesettingsitem.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceSettingsItem::WorkspaceSettingsItem(const QString& key,
                                             QObject* parent) noexcept
  : QObject(parent), mKey(key), mIsDefault(true), mEdited(false) {
}

WorkspaceSettingsItem::~WorkspaceSettingsItem() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void WorkspaceSettingsItem::valueModified() noexcept {
  mIsDefault = false;
  mEdited = true;
  emit edited();
}

void WorkspaceSettingsItem::restoreDefault() noexcept {
  restoreDefaultImpl();
  mIsDefault = true;
  mEdited = true;
}

void WorkspaceSettingsItem::load(const SExpression& root,
                                 const Version& fileFormat) {
  loadImpl(root, fileFormat);  // can throw
  mIsDefault = false;
  mEdited = false;
}

void WorkspaceSettingsItem::serialize(SExpression& root) const {
  serializeImpl(root);  // can throw
  mEdited = false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
