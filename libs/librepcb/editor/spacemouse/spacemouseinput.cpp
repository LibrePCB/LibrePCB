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

// These run on the Rust-owned background thread, *not* this object's own
// thread (see `hid.rs`). `userData` is the SpaceMouseInput* passed to
// rs::ffi_spacemouse_backend_new(), cast through `void*` since a plain C
// function pointer can't capture a `this`. These functions do exactly one
// thing: post the actual work onto `self`'s own thread via a queued
// QMetaObject::invokeMethod() call, matching the idiom found elsewhere in
// the editor for cross-thread event delivery.

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

// Following the same pattern as e.g. ZipArchive's `construct()` helper.
RustHandle<rs::FfiSpaceMouseBackend> SpaceMouseInput::construct(
    SpaceMouseInput* self) noexcept {
  rs::FfiSpaceMouseBackend* obj = rs::ffi_spacemouse_backend_new(
      self, &SpaceMouseInput::onMotion, &SpaceMouseInput::onConnectedChanged);
  // Per ffi_spacemouse_backend_new()'s contract, this can't fail - only
  // finding/opening a device can fail, and that's reported later via
  // on_connected_changed(), not a null return here.
  Q_ASSERT(obj);
  return RustHandle<rs::FfiSpaceMouseBackend>(*obj,
                                              &rs::ffi_spacemouse_backend_free);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
