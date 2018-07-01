/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include <librepcb/project/project.h>
#include <librepcb/project/schematics/items/si_netpoint.h>
#include <librepcb/project/schematics/schematic.h>
#include <librepcb/common/undostack.h>
#include <librepcb/project/schematics/items/si_netline.h>
#include <librepcb/project/schematics/items/si_symbol.h>
#include <librepcb/project/schematics/items/si_symbolpin.h>
#include "../symbolinstancepropertiesdialog.h"
#include <librepcb/project/circuit/componentinstance.h>
#include <librepcb/project/schematics/items/si_netlabel.h>
#include <librepcb/project/circuit/netsignal.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaledit.h>
#include <librepcb/project/circuit/cmd/cmdnetsignaladd.h>
#include "../../cmd/cmdcombinenetsignals.h"
#include "../../cmd/cmdremoveselectedschematicitems.h"
#include "../../cmd/cmdrotateselectedschematicitems.h"
#include "../../cmd/cmdmoveselectedschematicitems.h"
#include "../../cmd/cmdchangenetsignalofschematicnetsegment.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_Select::SES_Select(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                       GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    SES_Base(editor, editorUi, editorGraphicsView, undoStack), mSubState(SubState_Idle)
{
}

SES_Select::~SES_Select()
{
    Q_ASSERT(mSelectedItemsMoveCommand.isNull());
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
    return true;
}

bool SES_Select::exit(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
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
            rotateSelectedItems(-Angle::deg90());
            return ForceStayInState;
        case SEE_Base::Edit_RotateCCW:
            rotateSelectedItems(Angle::deg90());
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
                    return proccessIdleSceneLeftClick(mouseEvent, *schematic);
                default:
                    break;
            }
            break;
        }
        case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            switch (mouseEvent->button())
            {
                case Qt::LeftButton:
                    // remove selection rectangle and keep the selection state of all items
                    schematic->setSelectionRect(Point(), Point(), false);
                    return ForceStayInState;
                case Qt::RightButton:
                    return proccessIdleSceneRightMouseButtonReleased(mouseEvent, schematic);
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
                                                            Schematic& schematic) noexcept
{
    // handle items selection
    QList<SI_Base*> items = schematic.getItemsAtScenePos(Point::fromPx(mouseEvent->scenePos()));
    if (items.isEmpty()) {
        // no items under mouse --> start drawing a selection rectangle
        schematic.clearSelection();
        return ForceStayInState;
    }
    if (!items.first()->isSelected()) {
        if (!(mouseEvent->modifiers() & Qt::ControlModifier)) // CTRL pressed
            schematic.clearSelection(); // select only the top most item under the mouse
        items.first()->setSelected(true);
    }

    if (startMovingSelectedItems(schematic, Point::fromPx(mouseEvent->scenePos())))
        return ForceStayInState;
    else
        return PassToParentState;
}

SES_Base::ProcRetVal SES_Select::proccessIdleSceneRightMouseButtonReleased(
        QGraphicsSceneMouseEvent* mouseEvent, Schematic* schematic) noexcept
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
                rotateSelectedItems(Angle::deg90());
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
                NetSignal& netsignal = label->getNetSignalOfNetSegment();
                QString name = QInputDialog::getText(&mEditor, tr("Change net of segment"),
                                                     tr("New net name:"), QLineEdit::Normal,
                                                     netsignal.getName());
                if (!name.isNull()) {
                    try {
                        // change netsignal of netsegment
                        mUndoStack.beginCmdGroup(tr("Change netsignal of netsegment"));
                        NetSignal* newSignal = mCircuit.getNetSignalByName(name);
                        if (!newSignal) {
                            CmdNetSignalAdd* cmd = new CmdNetSignalAdd(mProject.getCircuit(),
                                                                       netsignal.getNetClass(),
                                                                       name);
                            mUndoStack.appendToCmdGroup(cmd);
                            newSignal = cmd->getNetSignal();
                            Q_ASSERT(newSignal);
                        }
                        mUndoStack.appendToCmdGroup(new CmdChangeNetSignalOfSchematicNetSegment(
                                                    label->getNetSegment(), *newSignal));
                        mUndoStack.commitCmdGroup();
                    } catch (const Exception& e) {
                        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
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
        case QEvent::GraphicsSceneMouseRelease: {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent); if (!sceneEvent) break;
            if (sceneEvent->button() == Qt::LeftButton) {
                // stop moving items (set position of all selected elements permanent)
                Q_ASSERT(!mSelectedItemsMoveCommand.isNull());
                Point pos = Point::fromPx(sceneEvent->scenePos());
                mSelectedItemsMoveCommand->setCurrentPosition(pos);
                try {
                    mUndoStack.execCmd(mSelectedItemsMoveCommand.take()); // can throw
                } catch (Exception& e) {
                    QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
                }
                mSelectedItemsMoveCommand.reset();
                mSubState = SubState_Idle;
            }
            break;
        } // case QEvent::GraphicsSceneMouseRelease

        case QEvent::GraphicsSceneMouseMove: {
            // move selected elements to cursor position
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent); if (!sceneEvent) break;
            Q_ASSERT(!mSelectedItemsMoveCommand.isNull());
            Point pos = Point::fromPx(sceneEvent->scenePos());
            mSelectedItemsMoveCommand->setCurrentPosition(pos);
            break;
        } // case QEvent::GraphicsSceneMouseMove

#if (QT_VERSION < QT_VERSION_CHECK(5, 3, 0))
        case QEvent::GraphicsSceneMouseDoubleClick: {
            QGraphicsSceneMouseEvent* mouseEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(mouseEvent); if (!mouseEvent) break;
            Schematic* schematic = mEditor.getActiveSchematic();
            Q_ASSERT(schematic); if (!schematic) break;
            // abort moving and handle double click
            mSelectedItemsMoveCommand.reset();
            mSubState = SubState_Idle;
            return proccessIdleSceneDoubleClick(mouseEvent, schematic);
        }
#endif

        default: {
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

bool SES_Select::startMovingSelectedItems(Schematic& schematic, const Point& startPos) noexcept
{
    Q_ASSERT(mSelectedItemsMoveCommand.isNull());
    mSelectedItemsMoveCommand.reset(new CmdMoveSelectedSchematicItems(schematic, startPos));
    mSubState = SubState_Moving;
    return true;
}

bool SES_Select::rotateSelectedItems(const Angle& angle) noexcept
{
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return false;

    try
    {
        CmdRotateSelectedSchematicItems* cmd = new CmdRotateSelectedSchematicItems(*schematic, angle);
        mUndoStack.execCmd(cmd);
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        return false;
    }
}

bool SES_Select::removeSelectedItems() noexcept
{
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return false;

    try
    {
        CmdRemoveSelectedSchematicItems* cmd = new CmdRemoveSelectedSchematicItems(*schematic);
        mUndoStack.execCmd(cmd);
        return true;
    }
    catch (Exception& e)
    {
        QMessageBox::critical(&mEditor, tr("Error"), e.getMsg());
        return false;
    }
}

bool SES_Select::cutSelectedItems() noexcept
{

    return false; // TODO
}

bool SES_Select::copySelectedItems() noexcept
{
    return false; // TODO
}

bool SES_Select::pasteItems() noexcept
{
    return false; // TODO
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
