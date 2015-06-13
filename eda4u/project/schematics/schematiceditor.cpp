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
#include <eda4ucommon/undostack.h>
#include "schematic.h"
#include "schematicpagesdock.h"
#include "../erc/ercmsgdock.h"
#include "fsm/ses_fsm.h"
#include "../circuit/circuit.h"
#include <eda4ucommon/dialogs/gridsettingsdialog.h>
#include "../dialogs/projectpropertieseditordialog.h"
#include "../settings/projectsettings.h"
#include <eda4ucommon/graphics/graphicsview.h>
#include <eda4ucommon/gridproperties.h>
#include "cmd/cmdschematicadd.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicEditor::SchematicEditor(Project& project, bool readOnly) :
    QMainWindow(0), mProject(project), mUi(new Ui::SchematicEditor),
    mGraphicsView(nullptr), mGridProperties(nullptr), mActiveSchematicIndex(-1),
    mPagesDock(nullptr), mErcMsgDock(nullptr), mFsm(nullptr)
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

    // create default grid properties
    mGridProperties = new GridProperties();

    // add graphics view as central widget
    mGraphicsView = new GraphicsView(nullptr, this);
    mGraphicsView->setGridProperties(*mGridProperties);
    setCentralWidget(mGraphicsView);

    // connect some actions which are created with the Qt Designer
    connect(mUi->actionSave_Project, &QAction::triggered, &mProject, &Project::saveProject);
    connect(mUi->actionQuit, &QAction::triggered, this, &SchematicEditor::close);
    connect(mUi->actionAbout_Qt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(mUi->actionZoom_In, &QAction::triggered, mGraphicsView, &GraphicsView::zoomIn);
    connect(mUi->actionZoom_Out, &QAction::triggered, mGraphicsView, &GraphicsView::zoomOut);
    connect(mUi->actionZoom_All, &QAction::triggered, mGraphicsView, &GraphicsView::zoomAll);
    connect(mUi->actionShow_Control_Panel, &QAction::triggered,
            &Workspace::instance(), &Workspace::showControlPanel);
    connect(mUi->actionShow_Board_Editor, &QAction::triggered,
            &mProject, &Project::showBoardEditor);
    connect(mUi->actionEditNetclasses, &QAction::triggered,
            [this](){mProject.getCircuit().execEditNetClassesDialog(this);});
    connect(mUi->actionProjectSettings, &QAction::triggered,
            [this](){mProject.getSettings().showSettingsDialog(this);});

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
    mFsm = new SES_FSM(*this, *mUi, *mGraphicsView);

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
    connect(mUi->actionToolAddNetLabel, &QAction::triggered,
            [this](){mFsm->processEvent(new SEE_Base(SEE_Base::StartAddNetLabel), true);
                     mUi->actionToolAddNetLabel->setChecked(mUi->actionToolAddNetLabel->isCheckable());});

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
    if (mProject.getSchematics().count() > 0)
        setActiveSchematicIndex(0);

    // mUi->graphicsView->zoomAll(); does not work properly here, should be executed later...
    QTimer::singleShot(500, mGraphicsView, &GraphicsView::zoomAll); // ...in the event loop
}

SchematicEditor::~SchematicEditor()
{
    // Save Window Geometry
    QSettings clientSettings;
    clientSettings.setValue("schematic_editor/window_geometry", saveGeometry());
    clientSettings.setValue("schematic_editor/window_state", saveState());

    delete mFsm;                    mFsm = nullptr;
    delete mErcMsgDock;             mErcMsgDock = nullptr;
    delete mPagesDock;              mPagesDock = nullptr;
    delete mGraphicsView;           mGraphicsView = nullptr;
    delete mGridProperties;         mGridProperties = nullptr;
    delete mUi;                     mUi = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Schematic* SchematicEditor::getActiveSchematic() const noexcept
{
    return mProject.getSchematicByIndex(mActiveSchematicIndex);
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
        schematic->saveViewSceneRect(mGraphicsView->getVisibleSceneRect());
    }
    schematic = mProject.getSchematicByIndex(index);
    if (schematic)
    {
        // show scene, restore view scene rect, set grid properties
        schematic->showInView(*mGraphicsView);
        mGraphicsView->setVisibleSceneRect(schematic->restoreViewSceneRect());
        mGraphicsView->setGridProperties(schematic->getGridProperties());
    }
    else
    {
        mGraphicsView->setScene(nullptr);
    }

    // schematic page has changed!
    emit activeSchematicChanged(mActiveSchematicIndex, index);
    mActiveSchematicIndex = index;
    return true;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void SchematicEditor::abortAllCommands() noexcept
{
    // ugly... ;-)
    mFsm->processEvent(new SEE_Base(SEE_Base::AbortCommand), true);
    mFsm->processEvent(new SEE_Base(SEE_Base::AbortCommand), true);
    mFsm->processEvent(new SEE_Base(SEE_Base::AbortCommand), true);
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

void SchematicEditor::on_actionNew_Schematic_Page_triggered()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Add schematic page"),
                       tr("Choose a name:"), QLineEdit::Normal, tr("New Page"), &ok);
    if (!ok) return;

    try
    {
        CmdSchematicAdd* cmd = new CmdSchematicAdd(mProject, name);
        mProject.getUndoStack().execCmd(cmd);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
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
    GridSettingsDialog dialog(*mGridProperties, this);
    connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
            [this](const GridProperties& grid)
            {   *mGridProperties = grid;
                mGraphicsView->setGridProperties(grid);
            });
    if (dialog.exec())
    {
        foreach (Schematic* schematic, mProject.getSchematics())
            schematic->setGridProperties(*mGridProperties);
        mProject.setModifiedFlag();
    }
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
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent();
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_Resistor_triggered()
{
    QUuid genCompUuid = "{ef80cd5e-2689-47ee-8888-31d04fc99174}";
    QUuid symbVarUuid = "{a5995314-f535-45d4-8bd8-2d0b8a0dc42a}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_BipolarCapacitor_triggered()
{
    QUuid genCompUuid = "{d167e0e3-6a92-4b76-b013-77b9c230e5f1}";
    QUuid symbVarUuid = "{8cd7b37f-e5fa-4af5-a8dd-d78830bba3af}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_UnipolarCapacitor_triggered()
{
    QUuid genCompUuid = "{c54375c5-7149-4ded-95c5-7462f7301ee7}";
    QUuid symbVarUuid = "{5412add2-af9c-44b8-876d-a0fb7c201897}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_Inductor_triggered()
{
    QUuid genCompUuid = "{506bd124-6062-400e-9078-b38bd7e1aaee}";
    QUuid symbVarUuid = "{62a7598c-17fe-41cf-8fa1-4ed274c3adc2}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_gnd_triggered()
{
    QUuid genCompUuid = "{8076f6be-bfab-4fc1-9772-5d54465dd7e1}";
    QUuid symbVarUuid = "{f09ad258-595b-4ee9-a1fc-910804a203ae}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionAddGenCmp_vcc_triggered()
{
    QUuid genCompUuid = "{58c3c6cd-11eb-4557-aa3f-d3e05874afde}";
    QUuid symbVarUuid = "{afb86b45-68ec-47b6-8d96-153d73567228}";
    SEE_StartAddComponent* addEvent = new SEE_StartAddComponent(genCompUuid, symbVarUuid);
    mFsm->processEvent(addEvent, true);
    mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());
}

void SchematicEditor::on_actionProjectProperties_triggered()
{
    ProjectPropertiesEditorDialog dialog(mProject, this);
    dialog.exec();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SchematicEditor::graphicsViewEventHandler(QEvent* event)
{
    SEE_RedirectedQEvent* e = new SEE_RedirectedQEvent(SEE_Base::GraphicsViewEvent, event);
    return mFsm->processEvent(e, true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project

