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
#include "ses_addnetlabel.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include "../../../common/undostack.h"
#include "../../project.h"
#include "../../circuit/circuit.h"
#include "../items/si_netlabel.h"
#include "../cmd/cmdschematicnetlabeladd.h"
#include "../cmd/cmdschematicnetlabeledit.h"
#include "../schematic.h"
#include "../items/si_netline.h"
#include "../../../common/gridproperties.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_AddNetLabel::SES_AddNetLabel(SchematicEditor& editor, Ui::SchematicEditor& editorUi) :
    SES_Base(editor, editorUi), mUndoCmdActive(false), mCurrentNetLabel(nullptr),
    mEditCmd(nullptr)
{
}

SES_AddNetLabel::~SES_AddNetLabel()
{
    Q_ASSERT(mUndoCmdActive == false);
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SES_Base::ProcRetVal SES_AddNetLabel::process(SEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case SEE_Base::SchematicSceneEvent:
            return processSceneEvent(event);
        default:
            return PassToParentState;
    }
}

bool SES_AddNetLabel::entry(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
    Schematic* schematic = mEditor.getActiveSchematic();
    if (!schematic) return false;

    if (!addLabel(*schematic)) return false;

    // Check this state in the "tools" toolbar
    mEditorUi.actionToolAddNetLabel->setCheckable(true);
    mEditorUi.actionToolAddNetLabel->setChecked(true);
    return true;
}

bool SES_AddNetLabel::exit(SEE_Base* event) noexcept
{
    Q_UNUSED(event);

    if (mUndoCmdActive)
    {
        try
        {
            mProject.getUndoStack().abortCommand();
            mUndoCmdActive = false;
        }
        catch (Exception& e)
        {
            QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
            return false;
        }
    }

    // Uncheck this state in the "tools" toolbar
    mEditorUi.actionToolAddNetLabel->setCheckable(false);
    mEditorUi.actionToolAddNetLabel->setChecked(false);
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SES_Base::ProcRetVal SES_AddNetLabel::processSceneEvent(SEE_Base* event) noexcept
{
    QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditor.getGridProperties().getInterval());
            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                {
                    fixLabel(pos);
                    addLabel(*schematic);
                    updateLabel(*schematic, pos);
                    return ForceStayInState;
                }
                case Qt::RightButton:
                {
                    mEditCmd->rotate(-Angle::deg90(), pos, true);
                    return ForceStayInState;
                }
                default:
                    break;
            }
            break;
        }

        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditor.getGridProperties().getInterval());
            updateLabel(*schematic, pos);
            return ForceStayInState;
        }

        default:
            break;
    }
    return PassToParentState;
}

bool SES_AddNetLabel::addLabel(Schematic& schematic) noexcept
{
    Q_ASSERT(mUndoCmdActive == false);

    try
    {
        NetSignal* signal = mCircuit.getNetSignals().values().first();
        if (!signal)
            throw RuntimeError(__FILE__, __LINE__, QString(), tr("No net signal found."));

        mProject.getUndoStack().beginCommand(tr("Add net label to schematic"));
        mUndoCmdActive = true;
        CmdSchematicNetLabelAdd* cmdAdd = new CmdSchematicNetLabelAdd(schematic, *signal, Point());
        mProject.getUndoStack().appendToCommand(cmdAdd);
        mCurrentNetLabel = cmdAdd->getNetLabel();
        mEditCmd = new CmdSchematicNetLabelEdit(*mCurrentNetLabel);
        return true;
    }
    catch (Exception& e)
    {
        if (mUndoCmdActive)
        {
            try {mProject.getUndoStack().abortCommand();} catch (...) {}
            mUndoCmdActive = false;
        }
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

bool SES_AddNetLabel::updateLabel(Schematic& schematic, const Point& pos) noexcept
{
    Q_ASSERT(mUndoCmdActive == true);

    try
    {
        // get netline under cursor
        QList<SI_NetLine*> lines;
        uint count = schematic.getNetLinesAtScenePos(lines, pos);
        if (count > 0) mEditCmd->setNetSignal(*lines.first()->getNetSignal(), true);
        mEditCmd->setPosition(pos, true);
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

bool SES_AddNetLabel::fixLabel(const Point& pos) noexcept
{
    Q_ASSERT(mUndoCmdActive == true);

    try
    {
        mEditCmd->setPosition(pos, false);
        mProject.getUndoStack().appendToCommand(mEditCmd);
        mProject.getUndoStack().endCommand();
        mUndoCmdActive = false;
        return true;
    }
    catch (Exception& e)
    {
        if (mUndoCmdActive)
        {
            try {mProject.getUndoStack().abortCommand();} catch (...) {}
            mUndoCmdActive = false;
        }
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
