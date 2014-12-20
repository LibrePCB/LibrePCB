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
#include "wsi_debugtools.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_DebugTools::WSI_DebugTools(WorkspaceSettings& settings) :
    WSI_Base(settings), mWidget(nullptr), mCbxShowAllSchematicNetpoints(nullptr),
    mCbxShowSchematicNetlinesNetsignals(nullptr)
{
    // create a QWidget
    mWidget = new QWidget();
    QGridLayout* layout = new QGridLayout(mWidget);
#ifndef QT_DEBUG
    layout->addWidget(new QLabel(tr("Warning: Some of these settings may only work in DEBUG mode!")), 0, 0);
#endif

    // create checkboxes
    mCbxShowAllSchematicNetpoints = new QCheckBox(tr("Show All Schematic Netpoints"));
    layout->addWidget(mCbxShowAllSchematicNetpoints, layout->rowCount(), 0);
    mCbxShowSchematicNetlinesNetsignals = new QCheckBox(tr("Show Netsignals of Schematic Netlines"));
    layout->addWidget(mCbxShowSchematicNetlinesNetsignals, layout->rowCount(), 0);
    mCbxShowSymbolPinNetsignals = new QCheckBox(tr("Show Netsignal of Symbol Pins"));
    layout->addWidget(mCbxShowSymbolPinNetsignals, layout->rowCount(), 0);
    mCbxShowGenCompSymbolCount = new QCheckBox(tr("Show Count of Placed Symbols"));
    layout->addWidget(mCbxShowGenCompSymbolCount, layout->rowCount(), 0);

    // stretch the last row
    layout->setRowStretch(layout->rowCount(), 1);

    // load from settings
    revert();
}

WSI_DebugTools::~WSI_DebugTools()
{
    delete mWidget;         mWidget = nullptr; // this deletes also all checkboxes
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_DebugTools::restoreDefault()
{
    mCbxShowAllSchematicNetpoints->setChecked(false);
    mCbxShowSchematicNetlinesNetsignals->setChecked(false);
    mCbxShowSymbolPinNetsignals->setChecked(false);
    mCbxShowGenCompSymbolCount->setChecked(false);
}

void WSI_DebugTools::apply()
{
    saveValue("dbg_show_all_schematic_netpoints", mCbxShowAllSchematicNetpoints->isChecked());
    saveValue("dbg_show_schematic_netlines_netsignals", mCbxShowSchematicNetlinesNetsignals->isChecked());
    saveValue("dbg_show_symbol_pin_netsignals", mCbxShowSymbolPinNetsignals->isChecked());
    saveValue("dbg_show_gen_comp_symbol_count", mCbxShowGenCompSymbolCount->isChecked());
}

void WSI_DebugTools::revert()
{
    mCbxShowAllSchematicNetpoints->setChecked(loadValue("dbg_show_all_schematic_netpoints", false).toBool());
    mCbxShowSchematicNetlinesNetsignals->setChecked(loadValue("dbg_show_schematic_netlines_netsignals", false).toBool());
    mCbxShowSymbolPinNetsignals->setChecked(loadValue("dbg_show_symbol_pin_netsignals", false).toBool());
    mCbxShowGenCompSymbolCount->setChecked(loadValue("dbg_show_gen_comp_symbol_count", false).toBool());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
