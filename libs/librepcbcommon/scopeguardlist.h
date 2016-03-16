/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2016 The LibrePCB developers
 * http://librepcb.org/
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


#ifndef LIBREPCB_SCOPEGUARDLIST_H
#define LIBREPCB_SCOPEGUARDLIST_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include "scopeguard.h"
#include <functional>
#include <vector>
#include <algorithm>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class ScopeGuardList
 ****************************************************************************************/
/**
 * @brief Keeps a list of functions to call.
 *
 * @see ScopeGuard
 */
class ScopeGuardList final : public ScopeGuardBase
{
    public:
        ScopeGuardList() = default;

        ScopeGuardList(size_t size) noexcept :
            ScopeGuardBase(),
            mScopeGuards()
        {
            mScopeGuards.reserve(size);
        }

        ScopeGuardList(ScopeGuardList&& rhs) noexcept :
            ScopeGuardBase(std::move(rhs)),
            mScopeGuards(std::move(rhs.mScopeGuards))
        { }

        ScopeGuardList(const ScopeGuardList&) = delete;
        ScopeGuardList& operator=(const ScopeGuardList&) = delete;

        /**
         * Calls the added functions in reverse order
         */
        ~ScopeGuardList() noexcept
        {
            if (mActive) {
                for (auto scopeGuard = mScopeGuards.rbegin(); scopeGuard != mScopeGuards.rend(); ++scopeGuard) {
                    // skip empty functions
                    if (! *scopeGuard) continue;
                    try{ (*scopeGuard)(); } catch(...) {
                        qFatal("Cleanup function threw an exception!");
                    }
                }
            }
        }

        /**
         * Add a function
         */
        template<class Fun>
        void add(Fun f)
        {
            mScopeGuards.emplace_back(std::move(f));
        }


    private:

        std::vector<std::function<void()>> mScopeGuards;
};

} // namespace librepcb

#endif // LIBREPCB_SCOPEGUARDLIST_H
