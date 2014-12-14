/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef WSI_DEBUGTOOLS_H
#define WSI_DEBUGTOOLS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include "wsi_base.h"

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

        // Getters
        bool getShowAllSchematicNetpoints() const noexcept {return mCbxShowAllSchematicNetpoints->isChecked();}
        bool getShowSchematicNetlinesNetsignals() const noexcept {return mCbxShowSchematicNetlinesNetsignals->isChecked();}

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
        QCheckBox* mCbxShowAllSchematicNetpoints;
        QCheckBox* mCbxShowSchematicNetlinesNetsignals;
};

#endif // WSI_DEBUGTOOLS_H
