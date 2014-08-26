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
#include "../../workspace/workspacesettings.h"
#include "../../common/undostack.h"
#include "schematic.h"
#include "schematicpagesdock.h"
#include "unplacedsymbolsdock.h"
#include "fsm/schematiceditorfsm.h"
#include "fsm/schematiceditorevent.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

SchematicEditor::SchematicEditor(Workspace& workspace, Project& project) :
    QMainWindow(0), mWorkspace(workspace), mProject(project), mUi(new Ui::SchematicEditor),
    mPagesDock(0), mUnplacedSymbolsDock(0), mFsm(0)
{
    mUi->setupUi(this);

    // Add Dock Widgets
    mPagesDock = new SchematicPagesDock(mProject);
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

    // connect the "tools" toolbar with the state machine
    connect(mUi->actionToolSelect, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartSelect));});
    connect(mUi->actionToolMove, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartMove));});
    connect(mUi->actionToolDrawWire, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartDrawWires));});
    connect(mUi->actionToolAddComponent, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::StartAddComponents));});

    // connect the "command" toolbar with the state machine
    connect(mUi->actionCommandAbort, &QAction::triggered,
            [this](){mFsm->processEvent(new SchematicEditorEvent(SchematicEditorEvent::AbortCommand));});

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

    // Restore Window Geometry
    QSettings s(mWorkspace.getMetadataPath().getPathTo("settings.ini").toStr(), QSettings::IniFormat);
    restoreGeometry(s.value("schematic_editor/window_geometry").toByteArray());
    restoreState(s.value("schematic_editor/window_state").toByteArray());

    // Load first schematic page
    mUi->graphicsView->setGridType(CADView::gridLines);
    if (mProject.getSchematicCount() > 0)
        mUi->graphicsView->setScene(mProject.getSchematicByIndex(0));

    mFsm = new SchematicEditorFsm(*this);
}

SchematicEditor::~SchematicEditor()
{
    // Save Window Geometry
    QSettings s(mWorkspace.getMetadataPath().getPathTo("settings.ini").toStr(), QSettings::IniFormat);
    s.setValue("schematic_editor/window_geometry", saveGeometry());
    s.setValue("schematic_editor/window_state", saveState());

    delete mFsm;                    mFsm = 0;
    delete mUnplacedSymbolsDock;    mUnplacedSymbolsDock = 0;
    delete mPagesDock;              mPagesDock = 0;
    delete mUi;                     mUi = 0;
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

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
