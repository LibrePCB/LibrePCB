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
#include "ses_select.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include <librepcbproject/project.h>
#include <librepcbproject/schematics/items/si_netpoint.h>
#include <librepcbproject/schematics/schematic.h>
#include <librepcbproject/schematics/cmd/cmdsymbolinstanceedit.h>
#include <librepcbcommon/undostack.h>
#include <librepcbproject/schematics/items/si_netline.h>
#include <librepcbproject/schematics/items/si_symbol.h>
#include <librepcbproject/schematics/cmd/cmdsymbolinstanceremove.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlineremove.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointremove.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointdetach.h>
#include <librepcbproject/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcbproject/schematics/items/si_symbolpin.h>
#include "../symbolinstancepropertiesdialog.h"
#include <librepcbproject/circuit/componentinstance.h>
#include <librepcbproject/circuit/cmd/cmdcomponentinstanceremove.h>
#include <librepcbproject/schematics/items/si_netlabel.h>
#include <librepcbproject/circuit/netsignal.h>
#include <librepcbproject/circuit/circuit.h>
#include <librepcbproject/circuit/cmd/cmdnetsignaledit.h>
#include <librepcbproject/circuit/cmd/cmdnetsignaladd.h>
#include <librepcbproject/circuit/cmd/cmdcompsiginstsetnetsignal.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetpointedit.h>
#include <librepcbproject/circuit/cmd/cmdnetsignalremove.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlabeledit.h>
#include "../schematicclipboard.h"
#include <librepcbproject/schematics/cmd/cmdsymbolinstanceadd.h>
#include <librepcbproject/schematics/cmd/cmdschematicnetlabelremove.h>
#include <librepcbcommon/gridproperties.h>
#include <librepcbproject/boards/cmd/cmddeviceinstanceremove.h>
#include <librepcbproject/boards/board.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_Select::SES_Select(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                       GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    SES_Base(editor, editorUi, editorGraphicsView, undoStack), mSubState(SubState_Idle),
    mParentCommand(nullptr)
{
}

SES_Select::~SES_Select()
{
    try
    {
        qDeleteAll(mSymbolEditCmds);    mSymbolEditCmds.clear();
        qDeleteAll(mNetPointEditCmds);  mNetPointEditCmds.clear();
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
        case SEE_Base::Edit_Cut:
            cutSelectedItems();
            return ForceStayInState;
        case SEE_Base::Edit_Copy:
            copySelectedItems();
            return ForceStayInState;
        case SEE_Base::Edit_Paste:
            pasteItems();
            return ForceStayInState;
        case SEE_Base::Edit_RotateCW:
            rotateSelectedItems(-Angle::deg90(), Point(), true);
            return ForceStayInState;
        case SEE_Base::Edit_RotateCCW:
            rotateSelectedItems(Angle::deg90(), Point(), true);
            return ForceStayInState;
        case SEE_Base::Edit_Remove:
            removeSelectedItems();
            return ForceStayInState;
        case SEE_Base::GraphicsViewEvent:
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
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            if (mouseEvent->button() == Qt::LeftButton)
            {
                // remove selection rectangle and keep the selection state of all items
                schematic->setSelectionRect(Point(), Point(), false);
                return ForceStayInState;
            }
            break;
        }
        case QEvent::GraphicsSceneMouseDoubleClick:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            return proccessIdleSceneDoubleClick(mouseEvent, schematic);
        }
        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            if (mouseEvent->buttons().testFlag(Qt::LeftButton))
            {
                // draw selection rectangle
                Point p1 = Point::fromPx(mouseEvent->buttonDownScenePos(Qt::LeftButton));
                Point p2 = Point::fromPx(mouseEvent->scenePos());
                schematic->setSelectionRect(p1, p2, true);
                return ForceStayInState;
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
    QList<SI_Base*> items = schematic->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty())
    {
        // no items under mouse --> start drawing a selection rectangle
        schematic->clearSelection();
        return ForceStayInState;
    }
    if (!items.first()->isSelected())
    {
        if (!(mouseEvent->modifiers() & Qt::ControlModifier)) // CTRL pressed
            schematic->clearSelection(); // select only the top most item under the mouse
        items.first()->setSelected(true);
    }

    if (startMovingSelectedItems(schematic))
        return ForceStayInState;
    else
        return PassToParentState;
}

SES_Base::ProcRetVal SES_Select::proccessIdleSceneRightClick(QGraphicsSceneMouseEvent* mouseEvent,
                                                             Schematic* schematic) noexcept
{
    // handle item selection
    QList<SI_Base*> items = schematic->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty()) return PassToParentState;
    schematic->clearSelection();
    items.first()->setSelected(true);

    // build and execute the context menu
    QMenu menu;
    switch (items.first()->getType())
    {
        case SI_Base::Type_t::Symbol:
        {
            SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(items.first()); Q_ASSERT(symbol);
            ComponentInstance& cmpInstance = symbol->getComponentInstance();

            // build the context menu
            QAction* aCopy = menu.addAction(QIcon(":/img/actions/copy.png"), tr("Copy"));
            QAction* aRotateCCW = menu.addAction(QIcon(":/img/actions/rotate_left.png"), tr("Rotate"));
            QAction* aMirror = menu.addAction(QIcon(":/img/actions/flip_horizontal.png"), tr("Mirror"));
            menu.addSeparator();
            QAction* aPlaceUnplacedSymbols = menu.addAction(QString(tr("Place unplaced symbols of %1 (%2)")).arg(cmpInstance.getName()).arg(cmpInstance.getUnplacedSymbolsCount()));
            aPlaceUnplacedSymbols->setEnabled(cmpInstance.getUnplacedSymbolsCount() > 0);
            QAction* aRemoveSymbol = menu.addAction(QIcon(":/img/actions/delete.png"), QString(tr("Remove Symbol %1")).arg(symbol->getName()));
            aRemoveSymbol->setEnabled(cmpInstance.getPlacedSymbolsCount() > 1);
            QAction* aRemoveCmp = menu.addAction(QIcon(":/img/actions/cancel.png"), QString(tr("Remove Component %1")).arg(cmpInstance.getName()));
            menu.addSeparator();
            QAction* aProperties = menu.addAction(tr("Properties"));

            // execute the context menu
            QAction* action = menu.exec(mouseEvent->screenPos());
            if (action == aCopy)
            {
                // TODO
            }
            else if (action == aRotateCCW)
            {
                rotateSelectedItems(Angle::deg90(), symbol->getPosition());
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
            else if (action == aRemoveCmp)
            {
                // TODO
            }
            else if (action == aProperties)
            {
                // open the properties editor dialog of the selected item
                SymbolInstancePropertiesDialog dialog(mProject, cmpInstance, *symbol, mUndoStack, &mEditor);
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
        QList<SI_Base*> items = schematic->getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
        if (items.isEmpty()) return PassToParentState;
        // open the properties editor dialog of the top most item
        switch (items.first()->getType())
        {
            case SI_Base::Type_t::Symbol:
            {
                SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(items.first()); Q_ASSERT(symbol);
                ComponentInstance& cmpInstance = symbol->getComponentInstance();
                SymbolInstancePropertiesDialog dialog(mProject, cmpInstance, *symbol, mUndoStack, &mEditor);
                dialog.exec();
                return ForceStayInState;
            }
            case SI_Base::Type_t::NetLabel:
            {
                SI_NetLabel* label = dynamic_cast<SI_NetLabel*>(items.first()); Q_ASSERT(label);
                NetSignal& netsignal = label->getNetSignal();
                QString name = QInputDialog::getText(&mEditor, tr("Change Net Name"),
                                                     tr("New Net Name:"),QLineEdit::Normal,
                                                     netsignal.getName());
                if (!name.isNull())
                {
                    try
                    {
                        // change name
                        NetSignal* newSignal = mCircuit.getNetSignalByName(name);
                        if (newSignal)
                        {
                            mUndoStack.beginCommand(tr("Combine Net Signals"));
                            foreach (ComponentSignalInstance* signal, netsignal.getComponentSignals())
                            {
                                auto cmd = new CmdCompSigInstSetNetSignal(*signal, newSignal);
                                mUndoStack.appendToCommand(cmd);
                            }
                            foreach (SI_NetPoint* point, netsignal.getNetPoints())
                            {
                                auto cmd = new CmdSchematicNetPointEdit(*point);
                                cmd->setNetSignal(*newSignal);
                                mUndoStack.appendToCommand(cmd);
                            }
                            foreach (SI_NetLabel* label, netsignal.getNetLabels())
                            {
                                auto cmd = new CmdSchematicNetLabelEdit(*label);
                                cmd->setNetSignal(*newSignal, false);
                                mUndoStack.appendToCommand(cmd);
                            }
                            auto cmd = new CmdNetSignalRemove(mProject.getCircuit(), netsignal);
                            mUndoStack.appendToCommand(cmd);

                            mUndoStack.endCommand();
                        }
                        else
                        {
                            auto cmd = new CmdNetSignalEdit(mCircuit, netsignal);
                            cmd->setName(name, false);
                            mUndoStack.execCmd(cmd);
                        }
                    }
                    catch (Exception& e)
                    {
                        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                    }
                }
                break;
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
        case SEE_Base::GraphicsViewEvent:
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
            Point delta = Point::fromPx(sceneEvent->scenePos() - sceneEvent->buttonDownScenePos(Qt::LeftButton));
            delta.mapToGrid(mEditor.getGridProperties().getInterval());

            switch (sceneEvent->button())
            {
                case Qt::LeftButton: // stop moving items
                {
                    Q_CHECK_PTR(mParentCommand);

                    // move selected elements
                    foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds)
                        cmd->setDeltaToStartPos(delta, false);
                    foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds)
                        cmd->setDeltaToStartPos(delta, false);
                    foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds)
                        cmd->setDeltaToStartPos(delta, false);

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
                            mUndoStack.execCmd(mParentCommand); // can throw an exception
                        }
                    }
                    catch (Exception& e)
                    {
                        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                    }
                    mSymbolEditCmds.clear();
                    mNetPointEditCmds.clear();
                    mNetLabelEditCmds.clear();
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
            Point delta = Point::fromPx(sceneEvent->scenePos() - sceneEvent->buttonDownScenePos(Qt::LeftButton));
            delta.mapToGrid(mEditor.getGridProperties().getInterval());
            if (delta == mLastMouseMoveDeltaPos) break; // do not move any items

            // move selected elements
            foreach (CmdSymbolInstanceEdit* cmd, mSymbolEditCmds)
                cmd->setDeltaToStartPos(delta, true);
            foreach (CmdSchematicNetPointEdit* cmd, mNetPointEditCmds)
                cmd->setDeltaToStartPos(delta, true);
            foreach (CmdSchematicNetLabelEdit* cmd, mNetLabelEditCmds)
                cmd->setDeltaToStartPos(delta, true);

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

bool SES_Select::startMovingSelectedItems(Schematic* schematic) noexcept
{
    // get all selected items
    QList<SI_Base*> items = schematic->getSelectedItems(false, true, false, true, false, false,
                                                        false, false, false, false, false);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    // create move commands for all selected items
    Q_ASSERT(!mParentCommand);
    Q_ASSERT(mSymbolEditCmds.isEmpty());
    Q_ASSERT(mNetPointEditCmds.isEmpty());
    Q_ASSERT(mNetLabelEditCmds.isEmpty());
    mParentCommand = new UndoCommand(tr("Move Schematic Items"));
    foreach (SI_Base* item, items)
    {
        switch (item->getType())
        {
            case SI_Base::Type_t::Symbol:
            {
                SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(item); Q_ASSERT(symbol);
                mSymbolEditCmds.append(new CmdSymbolInstanceEdit(*symbol, mParentCommand));
                break;
            }
            case SI_Base::Type_t::NetPoint:
            {
                SI_NetPoint* point = dynamic_cast<SI_NetPoint*>(item); Q_ASSERT(point);
                mNetPointEditCmds.append(new CmdSchematicNetPointEdit(*point, mParentCommand));
                break;
            }
            case SI_Base::Type_t::NetLabel:
            {
                SI_NetLabel* label = dynamic_cast<SI_NetLabel*>(item); Q_ASSERT(label);
                mNetLabelEditCmds.append(new CmdSchematicNetLabelEdit(*label, mParentCommand));
                break;
            }
            default:
                break;
        }
    }

    // switch to substate SubState_Moving
    mSubState = SubState_Moving;
    return true;
}

bool SES_Select::rotateSelectedItems(const Angle& angle, Point center, bool centerOfElements) noexcept
{
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return false;

    // get all selected items
    QList<SI_Base*> items = schematic->getSelectedItems(false, true, false, true, false, false,
                                                        false, false, false, false, false);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    // find the center of all elements
    if (centerOfElements)
    {
        center = Point(0, 0);
        foreach (SI_Base* item, items)
            center += item->getPosition();
        center /= items.count();
        center.mapToGrid(mEditor.getGridProperties().getInterval());
    }

    bool commandActive = false;
    try
    {
        mUndoStack.beginCommand(tr("Rotate Schematic Elements"));
        commandActive = true;

        // rotate all elements
        foreach (SI_Base* item, items)
        {
            switch (item->getType())
            {
                case SI_Base::Type_t::Symbol:
                {
                    SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(item); Q_ASSERT(symbol);
                    CmdSymbolInstanceEdit* cmd = new CmdSymbolInstanceEdit(*symbol);
                    cmd->rotate(angle, center, false);
                    mUndoStack.appendToCommand(cmd);
                    break;
                }
                case SI_Base::Type_t::NetPoint:
                {
                    SI_NetPoint* netpoint = dynamic_cast<SI_NetPoint*>(item); Q_ASSERT(netpoint);
                    CmdSchematicNetPointEdit* cmd = new CmdSchematicNetPointEdit(*netpoint);
                    cmd->setPosition(netpoint->getPosition().rotated(angle, center), false);
                    mUndoStack.appendToCommand(cmd);
                    break;
                }
                case SI_Base::Type_t::NetLabel:
                {
                    SI_NetLabel* netlabel = dynamic_cast<SI_NetLabel*>(item); Q_ASSERT(netlabel);
                    CmdSchematicNetLabelEdit* cmd = new CmdSchematicNetLabelEdit(*netlabel);
                    cmd->rotate(angle, center, false);
                    mUndoStack.appendToCommand(cmd);
                    break;
                }
                default:
                    break;
            }
        }

        mUndoStack.endCommand();
        commandActive = false;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mUndoStack.abortCommand();} catch (...) {}
        return false;
    }

    return true;
}

bool SES_Select::removeSelectedItems() noexcept
{
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return false;

    // get all selected items
    QList<SI_Base*> items = schematic->getSelectedItems(false, true, true, true, true, true, true,
                                                        true, true, true, false);

    // abort if no items are selected
    if (items.isEmpty()) return false;

    // get all involved component instances
    QList<ComponentInstance*> componentInstances;
    foreach (SI_Base* item, items)
    {
        if (item->getType() == SI_Base::Type_t::Symbol)
        {
            SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(item); Q_ASSERT(symbol);
            if (!componentInstances.contains(&symbol->getComponentInstance()))
                componentInstances.append(&symbol->getComponentInstance());
        }
    }

    bool commandActive = false;
    try
    {
        mUndoStack.beginCommand(tr("Remove Schematic Elements"));
        commandActive = true;
        schematic->clearSelection();

        // remove all netlabels
        foreach (SI_Base* item, items)
        {
            if (item->getType() == SI_Base::Type_t::NetLabel)
            {
                SI_NetLabel* netlabel = dynamic_cast<SI_NetLabel*>(item); Q_ASSERT(netlabel);
                auto cmd = new CmdSchematicNetLabelRemove(*schematic, *netlabel);
                mUndoStack.appendToCommand(cmd);
            }
        }

        // remove all netlines
        foreach (SI_Base* item, items)
        {
            if (item->getType() == SI_Base::Type_t::NetLine)
            {
                SI_NetLine* netline = dynamic_cast<SI_NetLine*>(item); Q_ASSERT(netline);
                auto cmd = new CmdSchematicNetLineRemove(*schematic, *netline);
                mUndoStack.appendToCommand(cmd);
            }
        }

        // remove all netpoints
        foreach (SI_Base* item, items)
        {
            if (item->getType() == SI_Base::Type_t::NetPoint)
            {
                SI_NetPoint* netpoint = dynamic_cast<SI_NetPoint*>(item); Q_ASSERT(netpoint);
                // TODO: this code does not work correctly in all possible cases
                if (netpoint->getLines().count() == 0)
                {
                    CmdSchematicNetPointRemove* cmd = new CmdSchematicNetPointRemove(*schematic, *netpoint);
                    mUndoStack.appendToCommand(cmd);
                    if (netpoint->isAttached())
                    {
                        ComponentSignalInstance* signal = netpoint->getSymbolPin()->getComponentSignalInstance();
                        Q_ASSERT(signal); if (!signal) throw LogicError(__FILE__, __LINE__);
                        CmdCompSigInstSetNetSignal* cmd = new CmdCompSigInstSetNetSignal(*signal, nullptr);
                        mUndoStack.appendToCommand(cmd);
                    }
                }
                else if (netpoint->isAttached())
                {
                    ComponentSignalInstance* signal = netpoint->getSymbolPin()->getComponentSignalInstance();
                    Q_ASSERT(signal); if (!signal) throw LogicError(__FILE__, __LINE__);
                    CmdSchematicNetPointDetach* cmd1 = new CmdSchematicNetPointDetach(*netpoint);
                    mUndoStack.appendToCommand(cmd1);
                    CmdCompSigInstSetNetSignal* cmd2 = new CmdCompSigInstSetNetSignal(*signal, nullptr);
                    mUndoStack.appendToCommand(cmd2);
                }
            }
        }

        // remove all symbols
        foreach (SI_Base* item, items)
        {
            if (item->getType() == SI_Base::Type_t::Symbol)
            {
                SI_Symbol* symbol = dynamic_cast<SI_Symbol*>(item); Q_ASSERT(symbol);
                auto cmd = new CmdSymbolInstanceRemove(*schematic, *symbol);
                mUndoStack.appendToCommand(cmd);
            }
        }

        // remove devices and components
        foreach (auto component, componentInstances)
        {
            if (component->getPlacedSymbolsCount() == 0)
            {
                foreach (Board* board, mProject.getBoards()) {
                    DeviceInstance* dev = board->getDeviceInstanceByComponentUuid(component->getUuid());
                    if (dev) {
                        CmdDeviceInstanceRemove* cmd = new CmdDeviceInstanceRemove(*board, *dev);
                        mUndoStack.appendToCommand(cmd);
                    }
                }
                CmdComponentInstanceRemove* cmd = new CmdComponentInstanceRemove(mCircuit, *component);
                mUndoStack.appendToCommand(cmd);
            }
        }

        mUndoStack.endCommand();
        commandActive = false;
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mUndoStack.abortCommand();} catch (...) {}
        return false;
    }
}

bool SES_Select::cutSelectedItems() noexcept
{
    /*Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return false;

    // get all selected items
    QList<QGraphicsItem*> items = schematic->selectedItems();
    int count = 0;
    QList<SymbolInstance*> symbols;
    QList<SchematicNetPoint*> netpoints;
    QList<SchematicNetLine*> netlines;
    QList<SchematicNetLabel*> netlabels;
    count += SymbolInstance::extractFromGraphicsItems(items, symbols);
    count += SchematicNetPoint::extractFromGraphicsItems(items, netpoints, true, true,
                                                         true, true, true, true, true);
    count += SchematicNetLine::extractFromGraphicsItems(items, netlines, true, true, false);
    count += SchematicNetLabel::extractFromGraphicsItems(items, netlabels);

    // abort if no items are selected
    if (count == 0) return false;

    // get all involved component instances
    QList<ComponentInstance*> componentInstances;
    foreach (SymbolInstance* symbol, symbols)
    {
        if (!componentInstances.contains(&symbol->getComponentInstance()))
            componentInstances.append(&symbol->getComponentInstance());
    }

    bool commandActive = false;
    try
    {
        mEditor.getProject().getUndoStack().beginCommand(tr("Cut Schematic Elements"));
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
                    ComponentSignalInstance* signal = point->getPinInstance()->getComponentSignalInstance();
                    Q_ASSERT(signal); if (!signal) throw LogicError(__FILE__, __LINE__);
                    CmdCmpSigInstSetNetSignal* cmd = new CmdCmpSigInstSetNetSignal(*signal, nullptr);
                    mEditor.getProject().getUndoStack().appendToCommand(cmd);
                }
            }
            else if (point->isAttached())
            {
                ComponentSignalInstance* signal = point->getPinInstance()->getComponentSignalInstance();
                Q_ASSERT(signal); if (!signal) throw LogicError(__FILE__, __LINE__);
                CmdSchematicNetPointDetach* cmd1 = new CmdSchematicNetPointDetach(*point);
                mEditor.getProject().getUndoStack().appendToCommand(cmd1);
                CmdCmpSigInstSetNetSignal* cmd2 = new CmdCmpSigInstSetNetSignal(*signal, nullptr);
                mEditor.getProject().getUndoStack().appendToCommand(cmd2);
            }
        }

        // remove all symbols
        foreach (SymbolInstance* symbol, symbols)
        {
            CmdSymbolInstanceRemove* cmd = new CmdSymbolInstanceRemove(*schematic, *symbol);
            mEditor.getProject().getUndoStack().appendToCommand(cmd);
        }

        // remove components
        foreach (ComponentInstance* cmpInstance, componentInstances)
        {
            if (cmpInstance->getPlacedSymbolsCount() == 0)
            {
                CmdComponentInstanceRemove* cmd = new CmdComponentInstanceRemove(mCircuit, *cmpInstance);
                mEditor.getProject().getUndoStack().appendToCommand(cmd);
            }
        }

        mEditor.getProject().getUndoStack().endCommand();
        commandActive = false;

        // copy elements to clipboard
        SchematicClipboard::instance().cut(symbols);

        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mEditor.getProject().getUndoStack().abortCommand();} catch (...) {}
        return false;
    }*/
    return false; // TODO
}

bool SES_Select::copySelectedItems() noexcept
{
    return false; // TODO
}

bool SES_Select::pasteItems() noexcept
{
    /*Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return false;

    bool commandActive = false;
    try
    {
        // get clipboard content
        QList<SymbolInstance*> symbols;
        SchematicClipboard::instance().paste(*schematic, symbols);
        int countOfElements = symbols.count();
        Point centerOfElements;

        if (countOfElements == 0) return false;

        schematic->clearSelection();
        mEditor.getProject().getUndoStack().beginCommand(tr("Paste Schematic Elements"));
        commandActive = true;

        // add all symbols
        foreach (SymbolInstance* symbol, symbols)
        {
            CmdSymbolInstanceAdd* cmd = new CmdSymbolInstanceAdd(*symbol);
            mEditor.getProject().getUndoStack().appendToCommand(cmd);
            symbol->setSelected(true);
            centerOfElements += symbol->getPosition();
        }

        mEditor.getProject().getUndoStack().endCommand();
        commandActive = false;

        // move elements to cursor
        centerOfElements /= countOfElements;
        Point cursorPos = Point::fromPx(mEditorUi.graphicsView->mapToScene(
            mEditorUi.graphicsView->mapFromGlobal(QCursor::pos())),
            mEditorUi.graphicsView->getGridInterval());
        Point deltaPos = cursorPos - centerOfElements;
        foreach (SymbolInstance* symbol, symbols)
            symbol->setPosition(symbol->getPosition() + deltaPos);

        // start moving the items
        startMovingSelectedItems(schematic);

        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        if (commandActive)
            try {mEditor.getProject().getUndoStack().abortCommand();} catch (...) {}
        return false;
    }*/
    return false; // TODO
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
