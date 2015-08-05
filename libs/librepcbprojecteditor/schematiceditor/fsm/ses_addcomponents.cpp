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
#include "ses_addcomponents.h"
#include "../schematiceditor.h"
#include "ui_schematiceditor.h"
#include <librepcbproject/project.h>
#include <librepcbproject/library/projectlibrary.h>
#include <librepcblibrary/gencmp/genericcomponent.h>
#include <librepcbproject/circuit/cmd/cmdgencompinstadd.h>
#include <librepcbcommon/undostack.h>
#include <librepcbproject/schematics/cmd/cmdsymbolinstanceadd.h>
#include <librepcbproject/circuit/gencompinstance.h>
#include <librepcbproject/schematics/cmd/cmdsymbolinstanceedit.h>
#include <librepcbproject/schematics/items/si_symbol.h>
#include <librepcbproject/schematics/schematic.h>
#include "../../dialogs/addgencompdialog.h"
#include <librepcbcommon/gridproperties.h>
#include "librepcbproject/library/cmd/cmdprojectlibraryaddelement.h"
#include <librepcbworkspace/workspace.h>
#include <librepcblibrary/library.h>
#include <librepcblibrary/sym/symbol.h>

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SES_AddComponents::SES_AddComponents(SchematicEditor& editor, Ui::SchematicEditor& editorUi,
                                     GraphicsView& editorGraphicsView, UndoStack& undoStack) :
    SES_Base(editor, editorUi, editorGraphicsView, undoStack),
    mIsUndoCmdActive(false), mAddGenCompDialog(nullptr), mLastAngle(0),
    mGenComp(nullptr), mGenCompSymbVar(nullptr), mCurrentSymbVarItem(nullptr),
    mCurrentSymbolToPlace(nullptr), mCurrentSymbolEditCommand(nullptr)
{
}

SES_AddComponents::~SES_AddComponents()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

SES_Base::ProcRetVal SES_AddComponents::process(SEE_Base* event) noexcept
{
    switch (event->getType())
    {
        case SEE_Base::AbortCommand:
        {
            if (mAddGenCompDialog)
            {
                try
                {
                    if (!abortCommand(true)) return PassToParentState;
                    mLastAngle.setAngleMicroDeg(0); // reset the angle
                    startAddingComponent();
                    return ForceStayInState;
                }
                catch (UserCanceled& exc)
                {
                }
                catch (Exception& exc)
                {
                    QMessageBox::critical(&mEditor, tr("Error"), exc.getUserMsg());
                }
            }
            return PassToParentState;
        }
        case SEE_Base::StartAddComponent:
        {
            try
            {
                // start adding (another) component
                SEE_StartAddComponent* e = dynamic_cast<SEE_StartAddComponent*>(event);
                Q_ASSERT(e);
                if (!abortCommand(true)) return PassToParentState;
                mLastAngle.setAngleMicroDeg(0); // reset the angle
                startAddingComponent(e->getGenCompUuid(), e->getSymbVarUuid());
                return ForceStayInState;
            }
            catch (UserCanceled& exc)
            {
            }
            catch (Exception& exc)
            {
                QMessageBox::critical(&mEditor, tr("Error"), exc.getUserMsg());
            }
            return PassToParentState;
        }
        case SEE_Base::Edit_RotateCW:
            mCurrentSymbolEditCommand->rotate(-Angle::deg90(), mCurrentSymbolToPlace->getPosition(), true);
            return ForceStayInState;
        case SEE_Base::Edit_RotateCCW:
            mCurrentSymbolEditCommand->rotate(Angle::deg90(), mCurrentSymbolToPlace->getPosition(), true);
            return ForceStayInState;
        case SEE_Base::GraphicsViewEvent:
            return processSceneEvent(event);
        default:
            return PassToParentState;
    }
}

bool SES_AddComponents::entry(SEE_Base* event) noexcept
{
    // only accept events of type SEE_StartAddComponent
    if (!event) return false;
    if (event->getType() != SEE_Base::StartAddComponent) return false;
    SEE_StartAddComponent* e = dynamic_cast<SEE_StartAddComponent*>(event);
    Q_ASSERT(e); if (!e) return false;
    Q_ASSERT(mIsUndoCmdActive == false);
    mLastAngle.setAngleMicroDeg(0);

    // start adding the specified component
    try
    {
        startAddingComponent(e->getGenCompUuid(), e->getSymbVarUuid());
    }
    catch (UserCanceled& exc)
    {
        if (mIsUndoCmdActive) abortCommand(false);
        delete mAddGenCompDialog; mAddGenCompDialog = nullptr;
        return false;
    }
    catch (Exception& exc)
    {
        QMessageBox::critical(&mEditor, tr("Error"), QString(tr("Could not add component:\n\n%1")).arg(exc.getUserMsg()));
        if (mIsUndoCmdActive) abortCommand(false);
        delete mAddGenCompDialog; mAddGenCompDialog = nullptr;
        return false;
    }

    // update the command toolbar action
    mEditorUi.actionToolAddComponent->setCheckable(true);
    mEditorUi.actionToolAddComponent->setChecked(true);
    return true;
}

bool SES_AddComponents::exit(SEE_Base* event) noexcept
{
    Q_UNUSED(event);
    if (!abortCommand(true)) return false;
    Q_ASSERT(mIsUndoCmdActive == false);
    delete mAddGenCompDialog;   mAddGenCompDialog = nullptr;
    mEditorUi.actionToolAddComponent->setCheckable(false);
    mEditorUi.actionToolAddComponent->setChecked(false);
    return true;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

SES_Base::ProcRetVal SES_AddComponents::processSceneEvent(SEE_Base* event) noexcept
{
    QEvent* qevent = SEE_RedirectedQEvent::getQEventFromSEE(event);
    Q_ASSERT(qevent); if (!qevent) return PassToParentState;
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) return PassToParentState;
    if (!mIsUndoCmdActive) return PassToParentState; // temporary

    switch (qevent->type())
    {
        case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditor.getGridProperties().getInterval());
            // set temporary position of the current symbol
            Q_ASSERT(mCurrentSymbolEditCommand);
            mCurrentSymbolEditCommand->setPosition(pos, true);
            break;
        }

        case QEvent::GraphicsSceneMouseDoubleClick:
        case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* sceneEvent = dynamic_cast<QGraphicsSceneMouseEvent*>(qevent);
            Q_ASSERT(sceneEvent);
            Point pos = Point::fromPx(sceneEvent->scenePos(), mEditor.getGridProperties().getInterval());
            switch (sceneEvent->button())
            {
                case Qt::LeftButton:
                {
                    try
                    {
                        // place the current symbol finally
                        mCurrentSymbolEditCommand->setPosition(pos, false);
                        mUndoStack.appendToCommand(mCurrentSymbolEditCommand);
                        mCurrentSymbolEditCommand = nullptr;
                        mUndoStack.endCommand();
                        mIsUndoCmdActive = false;
                        mUndoStack.beginCommand(tr("Add Symbol to Schematic"));
                        mIsUndoCmdActive = true;

                        // check if there is a next symbol to add
                        mCurrentSymbVarItem = mGenCompSymbVar->getNextItem(mCurrentSymbVarItem);

                        if (mCurrentSymbVarItem)
                        {
                            // create the next symbol instance and add it to the schematic
                            CmdSymbolInstanceAdd* cmd = new CmdSymbolInstanceAdd(*schematic,
                                mCurrentSymbolToPlace->getGenCompInstance(),
                                mCurrentSymbVarItem->getUuid(), pos);
                            mUndoStack.appendToCommand(cmd);
                            mCurrentSymbolToPlace = cmd->getSymbol();
                            Q_ASSERT(mCurrentSymbolToPlace);

                            // add command to move the current symbol
                            Q_ASSERT(mCurrentSymbolEditCommand == nullptr);
                            mCurrentSymbolEditCommand = new CmdSymbolInstanceEdit(*mCurrentSymbolToPlace);
                            mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
                            return ForceStayInState;
                        }
                        else
                        {
                            // all symbols placed, start adding the next component
                            QUuid genCompUuid = mGenComp->getUuid();
                            QUuid symbVarUuid = mGenCompSymbVar->getUuid();
                            mUndoStack.endCommand();
                            mIsUndoCmdActive = false;
                            abortCommand(false); // reset attributes
                            startAddingComponent(genCompUuid, symbVarUuid);
                            return ForceStayInState;
                        }
                    }
                    catch (Exception& e)
                    {
                        QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
                        abortCommand(false);
                        return ForceLeaveState;
                    }
                    break;
                }

                case Qt::RightButton:
                    // rotate symbol
                    mLastAngle += Angle::deg90();
                    mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
                    return ForceStayInState;

                default:
                    break;
            }
            break;
        }

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
    }
    return PassToParentState;
}

void SES_AddComponents::startAddingComponent(const QUuid& genComp, const QUuid& symbVar) throw (Exception)
{
    Schematic* schematic = mEditor.getActiveSchematic();
    Q_ASSERT(schematic); if (!schematic) throw LogicError(__FILE__, __LINE__);

    try
    {
        // start a new command
        Q_ASSERT(!mIsUndoCmdActive);
        mUndoStack.beginCommand(tr("Add Generic Component to Schematic"));
        mIsUndoCmdActive = true;

        if (genComp.isNull() || symbVar.isNull())
        {
            // show generic component chooser dialog
            if (!mAddGenCompDialog)
                mAddGenCompDialog = new AddGenCompDialog(mWorkspace, mProject, &mEditor);
            if (mAddGenCompDialog->exec() != QDialog::Accepted)
                throw UserCanceled(__FILE__, __LINE__); // abort

            // open the XML file
            library::GenericComponent* genComp = new library::GenericComponent(mAddGenCompDialog->getSelectedGenCompFilePath());
            QUuid genCompUuid = genComp->getUuid();
            Version genCompVersion = genComp->getVersion();
            delete genComp;

            // search the generic component in the library
            mGenComp = mProject.getLibrary().getGenComp(genCompUuid);
            if (mGenComp)
            {
                if (mGenComp->getVersion() != genCompVersion)
                {
                    QMessageBox::information(&mEditor, tr("Different Version"),
                        QString(tr("The same generic component exists already in this "
                        "project's library but the version is different. The version %1 from "
                        "the project's library will be used instead of the version %2."))
                        .arg(mGenComp->getVersion().toStr(), genCompVersion.toStr()));
                }
            }
            else
            {
                // copy the generic component to the project's library
                FilePath genCmpFp = mWorkspace.getLibrary().getLatestGenericComponent(genCompUuid);
                if (!genCmpFp.isValid())
                {
                    throw RuntimeError(__FILE__, __LINE__, QString(),
                        QString(tr("Generic Component not found in library: %1"))
                        .arg(genCompUuid.toString()));
                }
                mGenComp = new library::GenericComponent(genCmpFp);
                auto cmd = new CmdProjectLibraryAddElement<library::GenericComponent>(
                    mProject.getLibrary(), *mGenComp);
                mUndoStack.appendToCommand(cmd);
            }
            mGenCompSymbVar = mGenComp->getSymbolVariantByUuid(mAddGenCompDialog->getSelectedSymbVarUuid());
        }
        else
        {
            // search the generic component in the library
            mGenComp = mProject.getLibrary().getGenComp(genComp);
            if (mGenComp) mGenCompSymbVar = mGenComp->getSymbolVariantByUuid(symbVar);
        }

        // check pointers
        if (!mGenComp)
        {
            throw LogicError(__FILE__, __LINE__, QString(),
                QString(tr("The generic component \"%1\" was not found in the "
                "project's library.")).arg(genComp.toString()));
        }
        if (!mGenCompSymbVar)
        {
            throw LogicError(__FILE__, __LINE__, QString(), QString(
                tr("Invalid symbol variant: \"%1\"")).arg(symbVar.toString()));
        }

        // copy all required symbols to the project's library
        foreach (const library::GenCompSymbVarItem* item, mGenCompSymbVar->getItems())
        {
            QUuid uuid = item->getSymbolUuid();
            if (!mProject.getLibrary().getSymbol(uuid))
            {
                FilePath symbolFp = mWorkspace.getLibrary().getLatestSymbol(uuid);
                if (!symbolFp.isValid())
                {
                    throw RuntimeError(__FILE__, __LINE__, QString(),
                        QString(tr("Symbol not found in library: %1")).arg(uuid.toString()));
                }
                library::Symbol* symbol = new library::Symbol(symbolFp);
                auto cmd = new CmdProjectLibraryAddElement<library::Symbol>(
                    mProject.getLibrary(), *symbol);
                mUndoStack.appendToCommand(cmd);
            }
        }

        // get the scene position where the new symbol should be placed
        QPoint cursorPos = mEditorGraphicsView.mapFromGlobal(QCursor::pos());
        QPoint boundedCursorPos = QPoint(qBound(0, cursorPos.x(), mEditorGraphicsView.width()),
                                         qBound(0, cursorPos.y(), mEditorGraphicsView.height()));
        Point pos = Point::fromPx(mEditorGraphicsView.mapToScene(boundedCursorPos),
                                  mEditor.getGridProperties().getInterval());

        // create a new generic component instance and add it to the circuit
        CmdGenCompInstAdd* cmd = new CmdGenCompInstAdd(mCircuit, *mGenComp,
                                                               *mGenCompSymbVar);
        mUndoStack.appendToCommand(cmd);

        // create the first symbol instance and add it to the schematic
        mCurrentSymbVarItem = mGenCompSymbVar->getItems().first();
        if (!mCurrentSymbVarItem)
        {
            throw RuntimeError(__FILE__, __LINE__, symbVar.toString(),
                QString(tr("The generic component with the UUID \"%1\" does not have "
                           "any symbol.")).arg(genComp.toString()));
        }
        CmdSymbolInstanceAdd* cmd2 = new CmdSymbolInstanceAdd(*schematic,
            *(cmd->getGenCompInstance()), mCurrentSymbVarItem->getUuid(), pos);
        mUndoStack.appendToCommand(cmd2);
        mCurrentSymbolToPlace = cmd2->getSymbol();
        Q_ASSERT(mCurrentSymbolToPlace);

        // add command to move the current symbol
        Q_ASSERT(mCurrentSymbolEditCommand == nullptr);
        mCurrentSymbolEditCommand = new CmdSymbolInstanceEdit(*mCurrentSymbolToPlace);
        mCurrentSymbolEditCommand->setRotation(mLastAngle, true);
    }
    catch (Exception& e)
    {
        if (mIsUndoCmdActive) {try {mUndoStack.abortCommand(); mIsUndoCmdActive = false;} catch (...) {}}
        throw;
    }
}

bool SES_AddComponents::abortCommand(bool showErrMsgBox) noexcept
{
    try
    {
        // delete the current move command
        delete mCurrentSymbolEditCommand;
        mCurrentSymbolEditCommand = nullptr;

        // abort the undo command
        if (mIsUndoCmdActive)
        {
            mUndoStack.abortCommand();
            mIsUndoCmdActive = false;
        }

        // reset attributes, go back to idle state
        mGenComp = nullptr;
        mGenCompSymbVar = nullptr;
        mCurrentSymbVarItem = nullptr;
        mCurrentSymbolToPlace = nullptr;
        return true;
    }
    catch (Exception& e)
    {
        if (showErrMsgBox) QMessageBox::critical(&mEditor, tr("Error"), e.getUserMsg());
        return false;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
