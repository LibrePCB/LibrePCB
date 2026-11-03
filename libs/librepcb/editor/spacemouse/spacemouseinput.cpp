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
#include "spacemouseinput.h"

#include <librepcb/rust-core/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class SpaceMouseInput
 ******************************************************************************/

SpaceMouseInput::SpaceMouseInput(QObject* parent) noexcept
  : QObject(parent), mHandle(construct(this)), mConnected(false) {
}

SpaceMouseInput::~SpaceMouseInput() noexcept {
}

bool SpaceMouseInput::isDeviceConnected() const noexcept {
  return mConnected.load(std::memory_order_relaxed);
}

void SpaceMouseInput::setLedEnabled(bool enabled) noexcept {
  rs::ffi_spacemouse_backend_set_led(*mHandle, enabled);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SpaceMouseInput::handleConnectedChanged(bool connected) noexcept {
  mConnected.store(connected, std::memory_order_relaxed);
  emit deviceConnectedChanged(connected);
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

// Called on the Rust thread. Forwards to this object's thread.
void SpaceMouseInput::onMotion(void* userData,
                               rs::SpaceMouseMotionFfi motion) noexcept {
  auto* self = reinterpret_cast<SpaceMouseInput*>(userData);
  SpaceMouseMotionEvent event;
  event.translationX = motion.translation_x;
  event.translationY = motion.translation_y;
  event.translationZ = motion.translation_z;
  event.rotationX = motion.rotation_x;
  event.rotationY = motion.rotation_y;
  event.rotationZ = motion.rotation_z;
  QMetaObject::invokeMethod(
      self, [self, event]() { emit self->motionEvent(event); },
      Qt::QueuedConnection);
}

void SpaceMouseInput::onConnectedChanged(void* userData,
                                         bool connected) noexcept {
  auto* self = reinterpret_cast<SpaceMouseInput*>(userData);
  QMetaObject::invokeMethod(
      self, [self, connected]() { self->handleConnectedChanged(connected); },
      Qt::QueuedConnection);
}

RustHandle<rs::FfiSpaceMouseBackend> SpaceMouseInput::construct(
    SpaceMouseInput* self) noexcept {
  rs::FfiSpaceMouseBackend* obj = rs::ffi_spacemouse_backend_new(
      self, &SpaceMouseInput::onMotion, &SpaceMouseInput::onConnectedChanged);
  // Cannot fail. Device errors are reported via onConnectedChanged().
  Q_ASSERT(obj);
  return RustHandle<rs::FfiSpaceMouseBackend>(*obj,
                                              &rs::ffi_spacemouse_backend_free);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
