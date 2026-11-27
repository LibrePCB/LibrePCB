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

#include <cstdlib>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

using namespace rpc;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

static uint32_t getNextHandle() noexcept {
  // Start with a random handle, to reduce the risk of accidentally considering
  // stale handles from RPC clients as valid after LibrePCB has been restarted.
  static uint32_t sHandle = static_cast<uint32_t>(std::rand());
  ++sHandle;
  if (sHandle == 0) {
    ++sHandle;  // Skip handle 0 to safely allow using QVariant::toInt().
  }
  return sHandle;
}

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
  const uint32_t handle = getNextHandle();
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
