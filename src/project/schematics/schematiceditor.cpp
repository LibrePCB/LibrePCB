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
#include "unplacedsymbolsdock.h"
#include "fsm/schematiceditorfsm.h"
#include "../circuit/circuit.h"
#include "../../common/dialogs/gridsettingsdialog.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicEditor::SchematicEditor(Project& project) :
    QMainWindow(0), mProject(project), mUi(new Ui::SchematicEditor),
    mActiveSchematicIndex(-1), mPagesDock(0), mUnplacedSymbolsDock(0), mFsm(0)
{
    mUi->setupUi(this);
    setWindowTitle(QString("%1 - Schematic Editor - EDA4U %2.%3").arg(mProject.getFilepath().getFilename())
                   .arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR));

    // Add Dock Widgets
    mPagesDock = new SchematicPagesDock(mProject, *this);
    addDockWidget(Qt::LeftDockWidgetArea, mPagesDock, Qt::Vertical);
    mUnplacedSymbolsDock = new UnplacedSymbolsDock(mProject);
    addDockWidget(Qt::RightDockWidgetArea, mUnplacedSymbolsDock, Qt::Vertical);

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

    mFsm = new SchematicEditorFsm(*this);

    // connect the "tools" toolbar with the state machine (the second line of the lambda
    // functions is a workaround to set the checked attribute of the QActions properly)
    connect(mUi->actionToolSelect, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartSelect), true);
                     mUi->actionToolSelect->setChecked(mUi->actionToolSelect->isCheckable());});
    connect(mUi->actionToolMove, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartMove), true);
                     mUi->actionToolMove->setChecked(mUi->actionToolMove->isCheckable());});
    connect(mUi->actionToolDrawText, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartDrawText), true);
                     mUi->actionToolDrawText->setChecked(mUi->actionToolDrawText->isCheckable());});
    connect(mUi->actionToolDrawRectangle, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartDrawRect), true);
                     mUi->actionToolDrawRectangle->setChecked(mUi->actionToolDrawRectangle->isCheckable());});
    connect(mUi->actionToolDrawPolygon, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartDrawPolygon), true);
                     mUi->actionToolDrawPolygon->setChecked(mUi->actionToolDrawPolygon->isCheckable());});
    connect(mUi->actionToolDrawCircle, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartDrawCircle), true);
                     mUi->actionToolDrawCircle->setChecked(mUi->actionToolDrawCircle->isCheckable());});
    connect(mUi->actionToolDrawEllipse, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartDrawEllipse), true);
                     mUi->actionToolDrawEllipse->setChecked(mUi->actionToolDrawEllipse->isCheckable());});
    connect(mUi->actionToolDrawWire, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartDrawWire), true);
                     mUi->actionToolDrawWire->setChecked(mUi->actionToolDrawWire->isCheckable());});
    connect(mUi->actionToolAddComponent, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartAddComponent), true);
                     mFsm->processEvent(new SEE_SetAddComponentParams("{60000002-3c94-4689-be29-92235ba993c5}",
                                                                      "{a3a3db3e-c03e-4b3a-b916-638f75e11d9a}"), true);
                     mUi->actionToolAddComponent->setChecked(mUi->actionToolAddComponent->isCheckable());});

    // connect the "command" toolbar with the state machine
    connect(mUi->actionCommandAbort, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::AbortCommand), true);});

    // Restore Window Geometry
    QSettings clientSettings;
    restoreGeometry(clientSettings.value("schematic_editor/window_geometry").toByteArray());
    restoreState(clientSettings.value("schematic_editor/window_state").toByteArray());

    // Load first schematic page
    mUi->graphicsView->setGridType(CADView::gridLines);
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
    delete mUnplacedSymbolsDock;    mUnplacedSymbolsDock = 0;
    delete mPagesDock;              mPagesDock = 0;
    delete mUi;                     mUi = 0;
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void SchematicEditor::setActiveSchematicIndex(int index)
{
    if (index == mActiveSchematicIndex)
        return;

    // get the currently displayed schematic scene
    Schematic* schematic = dynamic_cast<Schematic*>(mUi->graphicsView->getCadScene());

    if (schematic)
    {
        // save current view scene rect
        schematic->saveViewSceneRect(mUi->graphicsView->getVisibleSceneRect());
        // unregister event handler object
        schematic->setEventHandlerObject(0);
    }

    // change scene
    schematic = mProject.getSchematicByIndex(index);
    mUi->graphicsView->setCadScene(schematic);

    if (schematic)
    {
        // register event handler object
        schematic->setEventHandlerObject(this);
        // restore view scene rect
        mUi->graphicsView->setVisibleSceneRect(schematic->restoreViewSceneRect());
    }

    emit activeSchematicChanged(mActiveSchematicIndex, index);
    mActiveSchematicIndex = index;
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
            [this](CADView::GridType type){mUi->graphicsView->setGridType(type);});
    connect(&dialog, &GridSettingsDialog::gridIntervalChanged,
            [this](const Length& interval){mUi->graphicsView->setGridInterval(interval);});
    connect(&dialog, &GridSettingsDialog::gridIntervalUnitChanged,
            [this](const LengthUnit& unit){mUi->graphicsView->setGridIntervalUnit(unit);});

    dialog.exec();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool SchematicEditor::cadSceneEventHandler(QEvent* event)
{
    SEE_RedirectedQEvent* e = new SEE_RedirectedQEvent(SchematicEditorEvent::SchematicSceneEvent, event);
    return mFsm->processEvent(e, true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
