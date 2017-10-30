/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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

#ifndef LIBREPCB_WSI_DEBUGTOOLS_H
#define LIBREPCB_WSI_DEBUGTOOLS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "wsi_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Class WSI_DebugTools
 ****************************************************************************************/

/**
 * @brief The WSI_DebugTools class contains some tools/settings which are useful for debugging
 */
class WSI_DebugTools final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WSI_DebugTools() = delete;
        WSI_DebugTools(const WSI_DebugTools& other) = delete;
        explicit WSI_DebugTools(const SExpression& node);
        ~WSI_DebugTools() noexcept;

        // Getters: Widgets
        QWidget* getWidget() const noexcept {return mWidget.data();}

        // General Methods
        void restoreDefault() noexcept override;
        void apply() noexcept override;
        void revert() noexcept override;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        WSI_DebugTools& operator=(const WSI_DebugTools& rhs) = delete;


    private: // Data

        // Widgets
        QScopedPointer<QWidget> mWidget;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WSI_DEBUGTOOLS_H
