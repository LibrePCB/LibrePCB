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
#include "rpcobjectregistry.h"

#include "api/rpcerrors.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using namespace rpc;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

uint32_t RpcObjectRegistry::sNextHandle = 0;

RpcObjectRegistry::RpcObjectRegistry(QObject* parent) noexcept
  : QObject(parent) {
}

RpcObjectRegistry::~RpcObjectRegistry() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

uint32_t RpcObjectRegistry::getHandle(QObject& obj) noexcept {
  const auto existing = mHandles.constFind(&obj);
  if (existing != mHandles.constEnd()) {
    return existing.value();
  }
  const uint32_t handle = ++sNextHandle;  // Skip handle 0!
  mObjects.insert(handle, &obj);
  mHandles.insert(&obj, handle);
  connect(&obj, &QObject::destroyed, this,
          [this](QObject* obj) { removeObject(obj); });
  return handle;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void RpcObjectRegistry::throwInvalidHandle() {
  throw RpcInvalidHandle();
}

void RpcObjectRegistry::removeObject(QObject* obj) noexcept {
  const auto handle = mHandles.constFind(obj);
  if (handle != mHandles.constEnd()) {
    mObjects.remove(handle.value());
    mHandles.erase(handle);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
