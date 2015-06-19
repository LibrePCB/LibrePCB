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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include "wsi_appearance.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_Appearance::WSI_Appearance(WorkspaceSettings& settings) :
    WSI_Base(settings), mUseOpenGlWidget(nullptr), mUseOpenGlCheckBox(nullptr)
{
    mUseOpenGlWidget = new QWidget();
    QGridLayout* openGlLayout = new QGridLayout(mUseOpenGlWidget);
    openGlLayout->setContentsMargins(0, 0, 0, 0);
    mUseOpenGlCheckBox = new QCheckBox(tr("Use OpenGL Hardware Acceleration"));
    openGlLayout->addWidget(mUseOpenGlCheckBox, openGlLayout->rowCount(), 0);
    openGlLayout->addWidget(new QLabel(tr("This setting will be applied only to newly "
                            "opened windows.")), openGlLayout->rowCount(), 0);

    // load from settings
    revert();
}

WSI_Appearance::~WSI_Appearance()
{
    delete mUseOpenGlWidget;        mUseOpenGlWidget = nullptr;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_Appearance::restoreDefault()
{
    mUseOpenGlCheckBox->setChecked(false);
}

void WSI_Appearance::apply()
{
    saveValue("appearance_use_opengl", mUseOpenGlCheckBox->isChecked());
}

void WSI_Appearance::revert()
{
    mUseOpenGlCheckBox->setChecked(loadValue("appearance_use_opengl", false).toBool());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
