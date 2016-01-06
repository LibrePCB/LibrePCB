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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "wsi_debugtools.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_DebugTools::WSI_DebugTools(WorkspaceSettings& settings) :
    WSI_Base(settings), mWidget(nullptr)
{
    // create a QWidget
    mWidget = new QWidget();
    QGridLayout* layout = new QGridLayout(mWidget);
#ifndef QT_DEBUG
    layout->addWidget(new QLabel(tr("Warning: Some of these settings may only work in DEBUG mode!")), 0, 0);
#endif

    // stretch the last row
    layout->setRowStretch(layout->rowCount(), 1);

    // load from settings
    revert();
}

WSI_DebugTools::~WSI_DebugTools()
{
    delete mWidget;         mWidget = nullptr;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_DebugTools::restoreDefault()
{
}

void WSI_DebugTools::apply()
{
}

void WSI_DebugTools::revert()
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
