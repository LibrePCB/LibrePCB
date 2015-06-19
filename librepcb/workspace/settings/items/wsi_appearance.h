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

#ifndef WSI_APPEARANCE_H
#define WSI_APPEARANCE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include "wsi_base.h"

/*****************************************************************************************
 *  Class WSI_Appearance
 ****************************************************************************************/

/**
 * @brief The WSI_Appearance class
 *
 * @author ubruhin
 * @date 2015-02-08
 */
class WSI_Appearance final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WSI_Appearance(WorkspaceSettings& settings);
        ~WSI_Appearance();

        // Getters
        bool getUseOpenGl() const noexcept {return mUseOpenGlCheckBox->isChecked();}

        // Getters: Widgets
        QString getUseOpenGlLabelText() const {return tr("Rendering Method:");}
        QWidget* getUseOpenGlWidget() const {return mUseOpenGlWidget;}

        // General Methods
        void restoreDefault();
        void apply();
        void revert();

    private:

        // make some methods inaccessible...
        WSI_Appearance();
        WSI_Appearance(const WSI_Appearance& other);
        WSI_Appearance& operator=(const WSI_Appearance& rhs);


        // Widgets
        QWidget* mUseOpenGlWidget;
        QCheckBox* mUseOpenGlCheckBox;
};

#endif // WSI_APPEARANCE_H
