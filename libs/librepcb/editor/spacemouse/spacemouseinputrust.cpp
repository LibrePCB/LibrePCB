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

// AI DISCLAIMER: Claude AI assisted in the writing of this file.
// It was reviewed and edited by a human.

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "spacemouseinputrust.h"

#include <librepcb/rust-spacemouse/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace {

/*******************************************************************************
 *  Rust -> C++ callback trampolines
 ******************************************************************************/

// These run on the Rust-owned background thread, *not* this object's own 
// thread (see `hid.rs`). `userData` is the SpaceMouseInputRust* passed to 
// rs::spacemouse::ffi_spacemouse_backend_new(), cast through `void*` since 
// a plain C function pointer can't capture a `this`. This function does 
// exactly one thing: post the actual work onto `self`'s own thread via a 
// queued QMetaObject::invokeMethod() call, matching the idiom already used
// elsewhere in the editor for cross-thread event delivery.

extern "C" void spaceMouseInputRustOnMotion(
    void* userData, rs::spacemouse::SpaceMouseMotionFfi motion) noexcept {
  auto* self = reinterpret_cast<SpaceMouseInputRust*>(userData);
  SpaceMouseMotionEvent event;
  event.translationX = motion.translation_x;
  event.translationY = motion.translation_y;
  event.translationZ = motion.translation_z;
  event.rotationX = motion.rotation_x;
  event.rotationY = motion.rotation_y;
  event.rotationZ = motion.rotation_z;
  QMetaObject::invokeMethod(
      self, [self, event]() { self->handleMotion(event); },
      Qt::QueuedConnection);
}

extern "C" void spaceMouseInputRustOnConnectedChanged(
    void* userData, bool connected) noexcept {
  auto* self = reinterpret_cast<SpaceMouseInputRust*>(userData);
  QMetaObject::invokeMethod(
      self, [self, connected]() { self->handleConnectedChanged(connected); },
      Qt::QueuedConnection);
}

// Constructs the Rust-side backend for use in SpaceMouseInputRust's member
// initializer list (following the same pattern as e.g. ZipArchive's
// `construct()` helper). Kept as a free function (rather than inline in the
// initializer list) because it needs `self` to be usable as a `QObject*`.
RustHandle<rs::spacemouse::FfiSpaceMouseBackend> construct(
    SpaceMouseInputRust* self) noexcept {
  rs::spacemouse::FfiSpaceMouseBackend* obj =
      rs::spacemouse::ffi_spacemouse_backend_new(
          self, &spaceMouseInputRustOnMotion,
          &spaceMouseInputRustOnConnectedChanged);
  // Per ffi_spacemouse_backend_new()'s contract, this can't fail - only
  // finding/opening a device can fail, and that's reported later via
  // on_connected_changed(), not a null return here.
  Q_ASSERT(obj);
  return RustHandle<rs::spacemouse::FfiSpaceMouseBackend>(
      *obj, &rs::spacemouse::ffi_spacemouse_backend_free);
}

}  // namespace

/*******************************************************************************
 *  Class SpaceMouseInputRust
 ******************************************************************************/

SpaceMouseInputRust::SpaceMouseInputRust(QObject* parent) noexcept
  : IF_SpaceMouseInputBackend(parent),
    mHandle(construct(this)),
    mConnected(false) {
}

SpaceMouseInputRust::~SpaceMouseInputRust() noexcept {
}

bool SpaceMouseInputRust::isDeviceConnected() const noexcept {
  return mConnected.load(std::memory_order_relaxed);
}

void SpaceMouseInputRust::setLedEnabled(bool enabled) noexcept {
  rs::spacemouse::ffi_spacemouse_backend_set_led(*mHandle, enabled);
}

void SpaceMouseInputRust::handleMotion(
    const SpaceMouseMotionEvent& event) noexcept {
  emit motionEvent(event);
}

void SpaceMouseInputRust::handleConnectedChanged(bool connected) noexcept {
  mConnected.store(connected, std::memory_order_relaxed);
  emit deviceConnectedChanged(connected);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
