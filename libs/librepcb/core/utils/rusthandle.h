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

#ifndef LIBREPCB_CORE_RUSTHANDLE_H
#define LIBREPCB_CORE_RUSTHANDLE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class RustHandle
 ******************************************************************************/

/**
 * @brief Scoped pointer for Rust objects
 */
template <typename T>
struct RustHandle final {
  typedef void (*Deleter)(T*);

  RustHandle() = delete;
  RustHandle(const RustHandle& other) = delete;
  RustHandle& operator=(const RustHandle& rhs) = delete;

  RustHandle(T& obj, Deleter deleter) noexcept
    : mObj(&obj), mDeleter(deleter) {}
  RustHandle(RustHandle&& other) noexcept
    : mObj(other.mObj), mDeleter(other.mDeleter) {
    other.mObj = nullptr;
  }
  ~RustHandle() noexcept {
    if (mObj) mDeleter(mObj);
  }

  const T* operator*() const noexcept {
    Q_ASSERT(mObj);
    return mObj;
  }
  T* operator*() noexcept {
    Q_ASSERT(mObj);
    return mObj;
  }

  T* mObj;
  Deleter mDeleter;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
