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
#include "schematiceditor.h"
#include "ui_schematiceditor.h"
#include "../project.h"
#include "../../workspace/workspace.h"
#include "../../workspace/settings/workspacesettings.h"
#include "../../common/undostack.h"
#include "schematic.h"
#include "schematicpagesdock.h"
#include "../erc/ercmsgdock.h"
#include "fsm/ses_fsm.h"
#include "../circuit/circuit.h"
#include "../../common/dialogs/gridsettingsdialog.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicEditor::SchematicEditor(Project& project, bool readOnly) :
    QMainWindow(0), mProject(project), mUi(new Ui::SchematicEditor),
    mActiveSchematicIndex(-1), mPagesDock(0), mErcMsgDock(0), mFsm(0)
{
    mUi->setupUi(this);
    mUi->actionSave_Project->setEnabled(!readOnly);

    // set window title
    QString filenameStr = mProject.getFilepath().getFilename();
    if (readOnly) filenameStr.append(QStringLiteral(" [Read-Only]"));
    setWindowTitle(QString("%1 - Schematic Editor - EDA4U %2.%3")
                   .arg(filenameStr).arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR));

    // Add Dock Widgets
    mPagesDock = new SchematicPagesDock(mProject, *this);
    addDockWidget(Qt::LeftDockWidgetArea, mPagesDock, Qt::Vertical);
    mErcMsgDock = new ErcMsgDock(mProject);
    addDockWidget(Qt::RightDockWidgetArea, mErcMsgDock, Qt::Vertical);

    // connect some actions which are created with the Qt Designer
    connect(mUi->actionSave_Project, SIGNAL(triggered()), &mProject, SLOT(save()));
    connect(mUi->actionQuit, SIGNAL(triggered()), this, SLOT(close()));
    connect(mUi->actionAbout_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(mUi->actionZoom_In, SIGNAL(triggered()), mUi->graphicsView, SLOT(zoomIn()));
    connect(mUi->actionZoom_Out, SIGNAL(triggered()), mUi->graphicsView, SLOT(zoomOut()));
    connect(mUi->actionZoom_All, SIGNAL(triggered()), mUi->graphicsView, SLOT(zoomAll()));
    connect(mUi->actionEditNetclasses, &QAction::triggered,
            [this](){mProject.getCircuit().execEditNetClassesDialog(this);});

    // connect the undo/redo actions with the UndoStack of the project
    connect(&mProject.getUndoStack(), &UndoStack::undoTextChanged,
            [this](const QString& text){mUi->actionUndo->setText(text);});
    mUi->actionUndo->setText(mProject.getUndoStack().getUndoText());
    connect(&mProject.getUndoStack(), SIGNAL(canUndoChanged(bool)),
            mUi->actionUndo, SLOT(setEnabled(bool)));
    mUi->actionUndo->setEnabled(mProject.getUndoStack().canUndo());
    connect(&mProject.getUndoStack(), &UndoStack::redoTextChanged,
            [this](const QString& text){mUi->actionRedo->setText(text);});
    mUi->actionRedo->setText(mProject.getUndoStack().getRedoText());
    connect(&mProject.getUndoStack(), SIGNAL(canRedoChanged(bool)),
            mUi->actionRedo, SLOT(setEnabled(bool)));
    mUi->actionRedo->setEnabled(mProject.getUndoStack().canRedo());

    // build the whole schematic editor finite state machine with all its substate objects
    mFsm = new SES_FSM(*this, *mUi);

    // connect the "tools" toolbar with the state machine (the second line of the lambda
    // functions is a workaround to set the checked attribute of the QActions properly)
    connect(mUi->actionToolSelect, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartSelect), true);
                     mUi->actionToolSelect->setChecked(mUi->actionToolSelect->isCheckable());});
    connect(mUi->actionToolMove, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartMove), true);
                     mUi->actionToolMove->setChecked(mUi->actionToolMove->isCheckable());});
    connect(mUi->actionToolDrawText, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartDrawText), true);
                     mUi->actionToolDrawText->setChecked(mUi->actionToolDrawText->isCheckable());});
    connect(mUi->actionToolDrawRectangle, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartDrawRect), true);
                     mUi->actionToolDrawRectangle->setChecked(mUi->actionToolDrawRectangle->isCheckable());});
    connect(mUi->actionToolDrawPolygon, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartDrawPolygon), true);
                     mUi->actionToolDrawPolygon->setChecked(mUi->actionToolDrawPolygon->isCheckable());});
    connect(mUi->actionToolDrawCircle, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartDrawCircle), true);
                     mUi->actionToolDrawCircle->setChecked(mUi->actionToolDrawCircle->isCheckable());});
    connect(mUi->actionToolDrawEllipse, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartDrawEllipse), true);
                     mUi->actionToolDrawEllipse->setChecked(mUi->actionToolDrawEllipse->isCheckable());});
    connect(mUi->actionToolDrawWire, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartDrawWire), true);
                     mUi->actionToolDrawWire->setChecked(mUi->actionToolDrawWire->isCheckable());});

    // connect the "command" toolbar with the state machine
    connect(mUi->actionCommandAbort, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::AbortCommand), true);});

    // connect the "edit" toolbar with the state machine
    connect(mUi->actionCopy, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Copy), true);});
    connect(mUi->actionCut, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Cut), true);});
    connect(mUi->actionPaste, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Paste), true);});
    connect(mUi->actionRotate_CW, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::Edit_RotateCW), true);});
    connect(mUi->actionRotate_CCW, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::Edit_RotateCCW), true);});
    connect(mUi->actionRemove, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::Edit_Remove), true);});

    // Restore Window Geometry
    QSettings clientSettings;
    restoreGeometry(clientSettings.value("schematic_editor/window_geometry").toByteArray());
    restoreState(clientSettings.value("schematic_editor/window_state").toByteArray());

    // Load first schematic page
    mUi->graphicsView->setGridType(CADView::GridType_t::Lines);
    if (mProject.getSchematicCount() > 0)
        setActiveSchematicIndex(0);

    // mUi->graphicsView->zoomAll(); does not work here, must be executed in the event loop
    QTimer::singleShot(0, mUi->graphicsView, SLOT(zoomAll()));
}

SchematicEditor::~SchematicEditor()
{
    // Save Window Geometry
    QSettings clientSettings;
    clientSettings.setValue("schematic_editor/window_geometry", saveGeometry());
    clientSettings.setValue("schematic_editor/window_state", saveState());

    delete mFsm;                    mFsm = 0;
    delete mErcMsgDock;             mErcMsgDock = 0;
    delete mPagesDock;              mPagesDock = 0;
    delete mUi;                     mUi = 0;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Schematic* SchematicEditor::getActiveSchematic() const noexcept
{
    return dynamic_cast<Schematic*>(mUi->graphicsView->scene());
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

bool SchematicEditor::setActiveSchematicIndex(int index) noexcept
{
    if (index == mActiveSchematicIndex)
        return true;

    // "Ask" the FSM if changing the scene is allowed at the moment.
    // If the FSM accepts the event, we can switch to the specified schematic page.
    SEE_SwitchToSchematicPage* event = new SEE_SwitchToSchematicPage(index);
    mFsm->processEvent(event);
    bool accepted = event->isAccepted();
    delete event;
    if (!accepted) return false; // changing the schematic page is not allowed!

    // event accepted --> change the schematic page
    Schematic* schematic = getActiveSchematic();
    if (schematic)
    {
        // save current view scene rect
        schematic->saveViewSceneRect(mUi->graphicsView->getVisibleSceneRect());
        // unregister event handler object
        schematic->setEventHandlerObject(0);
    }
    schematic = mProject.getSchematicByIndex(index);
    mUi->graphicsView->setCadScene(schematic);
    if (schematic)
    {
        // register event handler object
        schematic->setEventHandlerObject(this);
        // restore view scene rect
        mUi->graphicsView->setVisibleSceneRect(schematic->restoreViewSceneRect());
    }

    // schematic page has changed!
    emit activeSchematicChanged(mActiveSchematicIndex, index);
    mActiveSchematicIndex = index;
    return true;
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

void SchematicEditor::closeEvent(QCloseEvent* event)
{
    if (!mProject.windowIsAboutToClose(this))
        event->ignore();
    else
        QMainWindow::closeEvent(event);
}

/*****************************************************************************************
 *  Actions
 ****************************************************************************************/

void SchematicEditor::on_actionClose_Project_triggered()
{
    mProject.close(this);
}

void SchematicEditor::on_actionUndo_triggered()
{
    try
    {
        mProject.getUndoStack().undo();
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Undo failed"), e.getUserMsg());
    }
}

void SchematicEditor::on_actionRedo_triggered()
{
    try
    {
        mProject.getUndoStack().redo();
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Redo failed"), e.getUserMsg());
    }
}

void SchematicEditor::on_actionGrid_triggered()
{
    GridSettingsDialog dialog(mUi->graphicsView->getGridType(),
                              mUi->graphicsView->getGridInterval(),
                              mUi->graphicsView->getGridIntervalUnit(), this);

    connect(&dialog, &GridSettingsDialog::gridTypeChanged,
            [this](CADView::GridType_t type){mUi->graphicsView->setGridType(type);});
    connect(&dialog, &GridSettingsDialog::gridIntervalChanged,
            [this](const Length& interval){mUi->graphicsView->setGridInterval(interval);});
    connect(&dialog, &GridSettingsDialog::gridIntervalUnitChanged,
            [this](const LengthUnit& unit){mUi->graphicsView->setGridIntervalUnit(unit);});

    if (dialog.exec())
        mProject.setModifiedFlag();
}

void SchematicEditor::on_actionPDF_Export_triggered()
{
    try
    {
        QString filename = QFileDialog::getSaveFileName(this, tr("PDF Export"),
                                                        QDir::homePath(), "*.pdf");
        if (filename.isEmpty()) return;
        if (!filename.endsWith(".pdf")) filename.append(".pdf");
        FilePath filepath(filename);
        mProject.exportSchematicsAsPdf(filepath); // this method can throw an exception
    }
    catch (Exception& e)
    {
        QMessageBox::warning(this, tr("Error"), e.getUserMsg());
    }
}

void SchematicEditor::on_actionToolAddComponent_triggered()
{
    // TODO: use the component chooser dialog
    QUuid genCompUuid = "{60000002-3c94-4689-be29-92235ba993c5}";
    QUuid symbVarUuid = "{a3a3db3e-c03e-4b3a-b916-638f75e11d9a}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_Resistor_triggered()
{
    QUuid genCompUuid = "{2262ba69-33b0-414d-b03b-e1924a3fb3c8}";
    QUuid symbVarUuid = "{e8e00b88-f086-4edc-b3e1-742f2dcd2058}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_Capacitor_triggered()
{
    QUuid genCompUuid = "{e139c1ba-9d18-4f64-8a43-9a008ada7118}";
    QUuid symbVarUuid = "{94df66a0-a1f5-4b7d-9c57-22531a38e1f1}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_Inductor_triggered()
{
    QUuid genCompUuid = "{b39b86e1-2bcc-44fa-ae41-39d2f4b9f71f}";
    QUuid symbVarUuid = "{f9c64190-8dc9-4e3a-ab93-9437b0b3aae8}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_gnd_triggered()
{
    //QUuid genCompUuid = "{60000002-3c94-4689-be29-92235ba993c5}";
    //QUuid symbVarUuid = "{a3a3db3e-c03e-4b3a-b916-638f75e11d9a}";
    //SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    //mFsm->processEvent(addEvent, true);
    //mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_vcc_triggered()
{
    //QUuid genCompUuid = "{60000002-3c94-4689-be29-92235ba993c5}";
    //QUuid symbVarUuid = "{a3a3db3e-c03e-4b3a-b916-638f75e11d9a}";
    //SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    //mFsm->processEvent(addEvent, true);
    //mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SchematicEditor::cadSceneEventHandler(QEvent* event)
{
    SEE_RedirectedQEvent* e = new SEE_RedirectedQEvent(SEE_Base::SchematicSceneEvent, event);
    return mFsm->processEvent(e, true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
