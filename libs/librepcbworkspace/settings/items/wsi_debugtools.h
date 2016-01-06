/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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
        explicit WSI_DebugTools(WorkspaceSettings& settings);
        ~WSI_DebugTools();

        // Getters: Widgets
        QWidget* getWidget() const {return mWidget;}

        // General Methods
        void restoreDefault();
        void apply();
        void revert();


    private:

        // make some methods inaccessible...
        WSI_DebugTools();
        WSI_DebugTools(const WSI_DebugTools& other);
        WSI_DebugTools& operator=(const WSI_DebugTools& rhs);


        // Widgets
        QWidget* mWidget;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_WSI_DEBUGTOOLS_H
