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
#include "schematiceditorevent.h"
#include "../../project.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include "../schematic.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicEditorEvent::SchematicEditorEvent(EventType type) :
    mType(type), mAccepted(false)
{
}

SchematicEditorEvent::~SchematicEditorEvent()
{
}

/*****************************************************************************************
 *  Class SEE_SwitchToSchematicPage
 ****************************************************************************************/

void SEE_SwitchToSchematicPage::changeActiveSchematicIndex(Project& project,
                                                           SchematicEditor& editor,
                                                           Ui::SchematicEditor& editorUi,
                                                           unsigned int newIndex) noexcept
{
    // get the currently displayed schematic scene
    Schematic* schematic = editor.getActiveSchematic();

    if (schematic)
    {
        // save current view scene rect
        schematic->saveViewSceneRect(editorUi.graphicsView->getVisibleSceneRect());
        // unregister event handler object
        schematic->setEventHandlerObject(0);
    }

    // change scene
    schematic = project.getSchematicByIndex(newIndex);
    editorUi.graphicsView->setCadScene(schematic);

    if (schematic)
    {
        // register event handler object
        schematic->setEventHandlerObject(&editor);
        // restore view scene rect
        editorUi.graphicsView->setVisibleSceneRect(schematic->restoreViewSceneRect());
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
