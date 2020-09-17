/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2016 The LibrePCB developers
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

#ifndef LIBREPCB_SCOPEGUARD_H
#define LIBREPCB_SCOPEGUARD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <utility>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ScopeGuardBase
 ******************************************************************************/
/**
 * Base class for ScopGuard mainly providing dismiss()
 */
class ScopeGuardBase {
public:
  ScopeGuardBase() noexcept : mActive(true) {}

  ScopeGuardBase(ScopeGuardBase&& rhs) noexcept : mActive(rhs.mActive) {
    rhs.dismiss();
  }

  /**
   * Do not execute cleanup code
   */
  void dismiss() noexcept { mActive = false; }

protected:
  /**
   * Do not allow to delete a ScopeGuardBase directly. So we can avoid a
   * virtual destructor.
   */
  ~ScopeGuardBase() = default;
  bool mActive;
};

/*******************************************************************************
 *  Class ScopeGuard
 ******************************************************************************/
/**
 * Implementation of a ScopeGuard based on
 * https://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Andrei-Alexandrescu-Systematic-Error-Handling-in-C
 */
template <class Fun>
class ScopeGuard final : public ScopeGuardBase {
public:
  ScopeGuard() = delete;
  ScopeGuard(const ScopeGuard&) = delete;

  ScopeGuard(Fun f) noexcept : mF(std::move(f)) {}

  ScopeGuard(ScopeGuard&& rhs) noexcept
    : ScopeGuardBase(std::move(rhs)), mF(std::move(rhs.mF)) {}

  /**
   * Calls the attached cleanup function
   */
  ~ScopeGuard() noexcept {
    if (mActive) {
      try {
        mF();
      } catch (const std::exception& e) {
        qFatal("Cleanup function threw an exception: %s", e.what());
      }
    }
  }

  ScopeGuard& operator=(const ScopeGuard&) = delete;

private:
  Fun mF;
};

/**
 * Create a ScopeGuard using argument deduction.
 */
template <class Fun>
ScopeGuard<Fun> scopeGuard(Fun f) {
  return ScopeGuard<Fun>(std::move(f));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_SCOPEGUARD_H
