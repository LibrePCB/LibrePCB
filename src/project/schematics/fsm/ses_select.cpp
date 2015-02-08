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
#include "../cmd/cmdsymbolinstanceremove.h"
#include "../cmd/cmdschematicnetlineremove.h"
#include "../cmd/cmdschematicnetpointremove.h"
#include "../cmd/cmdschematicnetpointdetach.h"
#include "../../circuit/cmd/cmdgencompsiginstsetnetsignal.h"
#include "../symbolpininstance.h"
#include "../symbolinstancepropertiesdialog.h"
#include "../../circuit/gencompinstance.h"

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
            rotateSelectedItems(Angle::deg90(), Point(), true);
            return ForceStayInState;
        case SEE_Base::Edit_RotateCCW:
            rotateSelectedItems(-Angle::deg90(), Point(), true);
            return ForceStayInState;
        case SEE_Base::Edit_Remove:
            removeSelectedItems();
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
                case Qt::RightButton:
                    return proccessIdleSceneRightClick(mouseEvent, schematic);
                default:
                    break;
            }
            break;
        } 
        case QEvent::GraphicsSceneMouseDoubleClick:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            return proccessIdleSceneDoubleClick(mouseEvent, schematic);
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
    items = schematic->selectedItems();
    uint count = 0;
    QList<SymbolInstance*> symbols;
    QList<SchematicNetPoint*> netpoints;
    count += SymbolInstance::extractFromGraphicsItems(items, symbols);
    count += SchematicNetPoint::extractFromGraphicsItems(items, netpoints, true, false,
                                                         true, false, false, false);

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

SES_Base::ProcRetVal SES_Select::proccessIdleSceneRightClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                             Schematic* schematic) noexcept
{
    // handle item selection
    QList<QGraphicsItem*> items = schematic->items(mouseEvent->scenePos());
    if (items.isEmpty()) return PassToParentState;
    schematic->clearSelection();
    items.first()->setSelected(true);

    // build and execute the context menu
    QMenu menu;
    switch (items.first()->type())
    {
        case CADScene::Type_Symbol:
        {
            // get symbol and component instances
            library::SymbolGraphicsItem* i = qgraphicsitem_cast<library::SymbolGraphicsItem*>(items.first());
            Q_ASSERT(i); if (!i) return PassToParentState;
            SymbolInstance* symbol = i->getSymbolInstance();
            Q_ASSERT(symbol); if (!symbol) return PassToParentState;
            GenCompInstance& genComp = symbol->getGenCompInstance();

            // build the context menu
            QAction* aCopy = menu.addAction(QIcon(":/img/actions/copy.png"), tr("Copy"));
            QAction* aRotate = menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
            QAction* aMirror = menu.addAction(QIcon(":/img/actions/flip_horizontal.png"), tr("Mirror"));
            menu.addSeparator();
            QAction* aPlaceUnplacedSymbols = menu.addAction(QString(tr("Place unplaced symbols of %1 (%2)")).arg(genComp.getName()).arg(genComp.getUnplacedSymbolsCount()));
            aPlaceUnplacedSymbols->setEnabled(genComp.getUnplacedSymbolsCount() > 0);
            QAction* aRemoveSymbol = menu.addAction(QIcon(":/img/actions/delete.png"), QString(tr("Remove Symbol %1")).arg(symbol->getName()));
            aRemoveSymbol->setEnabled(genComp.getPlacedSymbolsCount() > 1);
            QAction* aRemoveGenComp = menu.addAction(QIcon(":/img/actions/cancel.png"), QString(tr("Remove Component %1")).arg(genComp.getName()));
            menu.addSeparator();
            QAction* aProperties = menu.addAction(tr("Properties"));

            // execute the context menu
            QAction* action = menu.exec(mouseEvent->screenPos());
            if (action == aCopy)
            {
                // TODO
            }
            else if (action == aRotate)
            {
                rotateSelectedItems(-Angle::deg90(), symbol->getPosition());
            }
            else if (action == aMirror)
            {
                // TODO
            }
            else if (action == aPlaceUnplacedSymbols)
            {
                // TODO
            }
            else if (action == aRemoveSymbol)
            {
                // TODO
            }
            else if (action == aRemoveGenComp)
            {
                // TODO
            }
            else if (action == aProperties)
            {
                // open the properties editor dialog of the selected item
                SymbolInstancePropertiesDialog dialog(mProject, genComp, *symbol, &mEditor);
                dialog.exec();
            }
            return ForceStayInState;
        }
        default:
            break;
    }
    return PassToParentState;
}

SES_Base::ProcRetVal SES_Select::proccessIdleSceneDoubleClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                              Schematic* schematic) noexcept
{
    if (mouseEvent->buttons() == Qt::LeftButton)
    {
        // check if there is an element under the mouse
        QList<QGraphicsItem*> items = schematic->items(mouseEvent->scenePos());
        if (items.isEmpty()) return PassToParentState;
        // open the properties editor dialog of the top most item
        switch (items.first()->type())
        {
            case CADScene::Type_Symbol:
            {
                library::SymbolGraphicsItem* i = qgraphicsitem_cast<library::SymbolGraphicsItem*>(items.first());
                Q_ASSERT(i); if (!i) return PassToParentState;
                SymbolInstance* symbol = i->getSymbolInstance();
                Q_ASSERT(symbol); if (!symbol) return PassToParentState;
                GenCompInstance& genComp = symbol->getGenCompInstance();
                SymbolInstancePropertiesDialog dialog(mProject, genComp, *symbol, &mEditor);
                dialog.exec();
                return ForceStayInState;
            }
            default:
                break;
        }
    }
    return PassToParentState;
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

    // get all selected items
    QList<QGraphicsItem*> items = schematic->selectedItems();
    uint count = 0;
    QList<SymbolInstance*> symbols;
    QList<SchematicNetPoint*> netpoints;
    count += SymbolInstance::extractFromGraphicsItems(items, symbols);
    count += SchematicNetPoint::extractFromGraphicsItems(items, netpoints, true, false,
                                                         true, false, false, false);

    // abort if no items are selected
    if (count == 0) return ForceStayInState;

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
            CmdSchematicNetPointMove* cmd = new CmdSchematicNetPointMove(*point);
            cmd->setAbsolutePosTemporary(point->getPosition().rotated(angle, center));
            mEditor.getProject().getUndoStack().appendToCommand(cmd);
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

bool SES_Select::removeSelectedItems() noexcept
{
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return false;

    // get all selected items
    QList<QGraphicsItem*> items = schematic->selectedItems();
    uint count = 0;
    QList<SymbolInstance*> symbols;
    QList<SchematicNetPoint*> netpoints;
    QList<SchematicNetLine*> netlines;
    count += SymbolInstance::extractFromGraphicsItems(items, symbols);
    count += SchematicNetPoint::extractFromGraphicsItems(items, netpoints, true, true,
                                                         true, true, true, true, true);
    count += SchematicNetLine::extractFromGraphicsItems(items, netlines, true, true, false);

    // abort if no items are selected
    if (count == 0) return false;

    bool commandActive = false;
    try
    {
        mEditor.getProject().getUndoStack().beginCommand(tr("Remove Schematic Elements"));
        commandActive = true;
        schematic->clearSelection();

        // remove all netlines
        foreach (SchematicNetLine* line, netlines)
        {
            CmdSchematicNetLineRemove* cmd = new CmdSchematicNetLineRemove(*schematic, *line);
            mEditor.getProject().getUndoStack().appendToCommand(cmd);
        }

        // remove all netpoints
        foreach (SchematicNetPoint* point, netpoints)
        {
            // TODO: this code does not work correctly in all possible cases
            if (point->getLines().count() == 0)
            {
                CmdSchematicNetPointRemove* cmd = new CmdSchematicNetPointRemove(*schematic, *point);
                mEditor.getProject().getUndoStack().appendToCommand(cmd);
                if (point->isAttached())
                {
                    GenCompSignalInstance* signal = point->getPinInstance()->getGenCompSignalInstance();
                    Q_ASSERT(signal); if (!signal) throw LogicError(__FILE__, __LINE__);
                    CmdGenCompSigInstSetNetSignal* cmd = new CmdGenCompSigInstSetNetSignal(*signal, nullptr);
                    mEditor.getProject().getUndoStack().appendToCommand(cmd);
                }
            }
            else if (point->isAttached())
            {
                GenCompSignalInstance* signal = point->getPinInstance()->getGenCompSignalInstance();
                Q_ASSERT(signal); if (!signal) throw LogicError(__FILE__, __LINE__);
                CmdSchematicNetPointDetach* cmd1 = new CmdSchematicNetPointDetach(*point);
                mEditor.getProject().getUndoStack().appendToCommand(cmd1);
                CmdGenCompSigInstSetNetSignal* cmd2 = new CmdGenCompSigInstSetNetSignal(*signal, nullptr);
                mEditor.getProject().getUndoStack().appendToCommand(cmd2);
            }
        }

        // remove all symbols
        foreach (SymbolInstance* symbol, symbols)
        {
            CmdSymbolInstanceRemove* cmd = new CmdSymbolInstanceRemove(*schematic, *symbol);
            mEditor.getProject().getUndoStack().appendToCommand(cmd);
        }

        mEditor.getProject().getUndoStack().endCommand();
        commandActive = false;
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mEditor.getProject().getUndoStack().abortCommand();} catch (...) {}
        return false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
