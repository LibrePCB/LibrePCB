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
#include "ses_select.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include "../../project.h"
#include "../schematicnetpoint.h"
#include "../schematic.h"
#include "../../../library/symbolgraphicsitem.h"
#include "../cmd/cmdsymbolinstancemove.h"
#include "../../../common/undostack.h"
#include "../cmd/cmdschematicnetpointmove.h"
#include "../schematicnetline.h"
#include "../symbolinstance.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_Select::SES_Select(SchematicEditor& editor, Ui::SchematicEditor& editorUi) :
    SES_Base(editor, editorUi), mSubState(SubState_Idle),
    mParentCommand(nullptr)
{
}

SES_Select::~SES_Select()
{
    try
    {
        qDeleteAll(mSymbolMoveCmds);    mSymbolMoveCmds.clear();
        qDeleteAll(mNetPointMoveCmds);  mNetPointMoveCmds.clear();
        delete mParentCommand;          mParentCommand = nullptr;
    }
    catch (Exception& e)
    {
        qWarning() << "Exception catched in the SES_SELECT destructor:" << e.getDebugMsg();
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SES_Base::ProcRetVal SES_Select::process(SEE_Base* event) noexcept
{
    switch (mSubState)
    {
        case SubState_Idle:
            return processSubStateIdle(event);
        case SubState_Moving:
            return processSubStateMoving(event);
        default:
            return PassToParentState;
    }
}

bool SES_Select::entry(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
    mEditorUi.actionToolSelect->setCheckable(true);
    mEditorUi.actionToolSelect->setChecked(true);
    return true;
}

bool SES_Select::exit(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
    mEditorUi.actionToolSelect->setCheckable(false);
    mEditorUi.actionToolSelect->setChecked(false);
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SES_Base::ProcRetVal SES_Select::processSubStateIdle(SEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case SEE_Base::Edit_RotateCW:
            rotateSelectedItems(Angle(90000000), Point(), true);
            return ForceStayInState;
        case SEE_Base::Edit_RotateCCW:
            rotateSelectedItems(Angle(-90000000), Point(), true);
            return ForceStayInState;
        case SEE_Base::SchematicSceneEvent:
            return processSubStateIdleSceneEvent(event);
        default:
            return PassToParentState;
    }
}

SES_Base::ProcRetVal SES_Select::processSubStateIdleSceneEvent(SEE_Base* event) noexcept
{
    QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            switch (mouseEvent->button())
            {
                case Qt::LeftButton:
                    return proccessIdleSceneLeftClick(mouseEvent, schematic);
                //case Qt::RightButton:
                //    // switch back to the last command (previous state)
                //    return PassToParentState;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
    return PassToParentState;
}

SES_Base::ProcRetVal SES_Select::proccessIdleSceneLeftClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                            Schematic* schematic) noexcept
{
    // handle items selection
    QList<QGraphicsItem*> items = schematic->items(mouseEvent->scenePos());
    if (items.isEmpty()) return PassToParentState; // no items under mouse --> abort
    if (!items.first()->isSelected())
    {
        if (!(mouseEvent->modifiers() & Qt::ControlModifier)) // CTRL pressed
            schematic->clearSelection(); // select only the top most item under the mouse
        items.first()->setSelected(true);
    }

    // get all selected items
    QList<SymbolInstance*> symbols;
    QList<SchematicNetPoint*> netpoints;
    uint count = extractGraphicsItems(schematic->selectedItems(), symbols, netpoints);

    // abort if no items are selected
    if (count == 0) return ForceStayInState;

    // create move commands for all selected items
    Q_ASSERT(!mParentCommand);
    Q_ASSERT(mSymbolMoveCmds.isEmpty());
    Q_ASSERT(mNetPointMoveCmds.isEmpty());
    mParentCommand = new UndoCommand(tr("Move Schematic Items"));
    foreach (SymbolInstance* instance, symbols)
        mSymbolMoveCmds.append(new CmdSymbolInstanceMove(*instance, mParentCommand));
    foreach (SchematicNetPoint* point, netpoints)
        mNetPointMoveCmds.append(new CmdSchematicNetPointMove(*point, mParentCommand));

    // switch to substate SubState_Moving
    mSubState = SubState_Moving;
    mMoveStartPos = Point::fromPx(mouseEvent->scenePos()); // not mapped to grid!
    return ForceStayInState;
}

SES_Base::ProcRetVal SES_Select::processSubStateMoving(SEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case SEE_Base::SchematicSceneEvent:
            return processSubStateMovingSceneEvent(event);
        default:
            return PassToParentState;
    }
}

SES_Base::ProcRetVal SES_Select::processSubStateMovingSceneEvent(SEE_Base* event) noexcept
{
    QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Schematic* schematic = mEditor.getActiveSchematic();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(schematic); if (!schematic) break;
            Point delta = Point::fromPx(sceneEvent->scenePos()) - mMoveStartPos;
            delta.mapToGrid(mEditorUi.graphicsView->getGridInterval());

            switch (sceneEvent->button())
            {
                case Qt::LeftButton: // stop moving items
                {
                    Q_CHECK_PTR(mParentCommand);

                    // move selected elements
                    foreach (CmdSymbolInstanceMove* cmd, mSymbolMoveCmds)
                        cmd->setDeltaToStartPosTemporary(delta);
                    foreach (CmdSchematicNetPointMove* cmd, mNetPointMoveCmds)
                        cmd->setDeltaToStartPosTemporary(delta);

                    // set position of all selected elements permanent
                    try
                    {
                        if (delta.isOrigin())
                        {
                            // items were not moved, do not execute the commands
                            delete mParentCommand; // this will also delete all objects in mSymbolInstanceMoveCommands
                        }
                        else
                        {
                            // items were moved, add commands to the project's undo stack
                            mProject.getUndoStack().execCmd(mParentCommand); // can throw an exception
                        }
                    }
                    catch (Exception& e)
                    {
                        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                    }
                    mSymbolMoveCmds.clear();
                    mNetPointMoveCmds.clear();
                    mParentCommand = nullptr;
                    mSubState = SubState_Idle;
                    break;
                } // case Qt::LeftButton

                default:
                    break;
            } // switch (sceneEvent->button())
            break;
        } // case QEvent::GraphicsSceneMouseRelease

        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Schematic* schematic = mEditor.getActiveSchematic();
            Q_CHECK_PTR(sceneEvent); if (!sceneEvent) break;
            Q_CHECK_PTR(schematic); if (!schematic) break;
            Q_CHECK_PTR(mParentCommand);

            // get delta position
            Point delta = Point::fromPx(sceneEvent->scenePos()) - mMoveStartPos;
            delta.mapToGrid(mEditorUi.graphicsView->getGridInterval());
            if (delta == mLastMouseMoveDeltaPos) break; // do not move any items

            // move selected elements
            foreach (CmdSymbolInstanceMove* cmd, mSymbolMoveCmds)
                cmd->setDeltaToStartPosTemporary(delta);
            foreach (CmdSchematicNetPointMove* cmd, mNetPointMoveCmds)
                cmd->setDeltaToStartPosTemporary(delta);

            mLastMouseMoveDeltaPos = delta;
            break;
        } // case QEvent::GraphicsSceneMouseMove

        default:
        {
            // Always accept graphics scene events, even if we do not react on some of the events!
            // This will give us the full control over the graphics scene. Otherwise, the graphics
            // scene can react on some events and disturb our state machine. Only the wheel event
            // is ignored because otherwise the view will not allow to zoom with the mouse wheel.
            if (qevent->type() != QEvent::GraphicsSceneWheel)
                return ForceStayInState;
            else
                return PassToParentState;
        }
    } // switch (qevent->type())
    return PassToParentState;
}

bool SES_Select::rotateSelectedItems(const Angle& angle, Point center, bool centerOfElements) noexcept
{
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return false;

    // get all selected symbols and netpoints
    QList<SymbolInstance*> symbols;
    QList<SchematicNetPoint*> netpoints;
    uint count = extractGraphicsItems(schematic->selectedItems(), symbols, netpoints);
    if (count == 0) return false;

    // find the center of all elements
    if (centerOfElements)
    {
        center = Point(0, 0);
        foreach (SymbolInstance* symbol, symbols)
            center += symbol->getPosition();
        foreach (SchematicNetPoint* point, netpoints)
            center += point->getPosition();
        center /= count;
        center.mapToGrid(mEditorUi.graphicsView->getGridInterval());
    }

    bool commandActive = false;
    try
    {
        mEditor.getProject().getUndoStack().beginCommand(tr("Rotate Schematic Elements"));
        commandActive = true;

        // rotate all symbols
        foreach (SymbolInstance* symbol, symbols)
        {
            CmdSymbolInstanceMove* cmd = new CmdSymbolInstanceMove(*symbol);
            cmd->rotate(angle, center);
            mEditor.getProject().getUndoStack().appendToCommand(cmd);
        }

        // rotate all netpoints
        foreach (SchematicNetPoint* point, netpoints)
        {
            // TODO: use undo command
            point->setPosition(point->getPosition().rotated(angle, center));
        }

        mEditor.getProject().getUndoStack().endCommand();
        commandActive = false;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mEditor.getProject().getUndoStack().abortCommand();} catch (...) {}
        return false;
    }

    return true;
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

uint SES_Select::extractGraphicsItems(const QList<QGraphicsItem*>& graphicsItems,
                                      QList<SymbolInstance*>& symbolInstances,
                                      QList<SchematicNetPoint*>& netpoints) noexcept
{
    foreach (QGraphicsItem* item, graphicsItems)
    {
        switch (item->type())
        {
            case CADScene::Type_Symbol:
            {
                library::SymbolGraphicsItem* i = qgraphicsitem_cast<library::SymbolGraphicsItem*>(item);
                Q_ASSERT(i); if (!i) break;
                SymbolInstance* s = i->getSymbolInstance();
                Q_ASSERT(s); if (!s) break;
                symbolInstances.append(s);
                break;
            }
            case CADScene::Type_SchematicNetPoint:
            {
                SchematicNetPointGraphicsItem* i = qgraphicsitem_cast<SchematicNetPointGraphicsItem*>(item);
                Q_ASSERT(i); if (!i) break;
                if (i->getNetPoint().isAttached()) break;
                if (netpoints.contains(&i->getNetPoint())) break;
                netpoints.append(&i->getNetPoint());
                break;
            }
            case CADScene::Type_SchematicNetLine:
            {
                SchematicNetLineGraphicsItem* i = qgraphicsitem_cast<SchematicNetLineGraphicsItem*>(item);
                Q_ASSERT(i); if (!i) break;
                if (!i->getNetLine().isAttachedToSymbol())
                {
                    // add start & end points to list
                    if (!netpoints.contains(&i->getNetLine().getStartPoint()))
                        netpoints.append(&i->getNetLine().getStartPoint());
                    if (!netpoints.contains(&i->getNetLine().getEndPoint()))
                        netpoints.append(&i->getNetLine().getEndPoint());
                }
                break;
            }
            default:
                break;
        }
    }
    return (symbolInstances.count() + netpoints.count());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
