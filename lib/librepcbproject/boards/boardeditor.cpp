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
#include <QtWidgets>
#include "boardeditor.h"
#include "ui_boardeditor.h"
#include "../project.h"
#include <librepcbworkspace/workspace.h>
#include <librepcbworkspace/settings/workspacesettings.h>
#include <librepcbcommon/undostack.h>
#include "board.h"
#include "../circuit/circuit.h"
#include <librepcbcommon/dialogs/gridsettingsdialog.h>
#include "../dialogs/projectpropertieseditordialog.h"
#include "../settings/projectsettings.h"
#include <librepcbcommon/graphics/graphicsview.h>
#include <librepcbcommon/gridproperties.h>
#include "cmd/cmdboardadd.h"
#include "../erc/ercmsgdock.h"
#include "unplacedcomponentsdock.h"
#include "fsm/bes_fsm.h"

namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardEditor::BoardEditor(Project& project, bool readOnly) :
    QMainWindow(0), mProject(project), mUi(new Ui::BoardEditor),
    mGraphicsView(nullptr), mGridProperties(nullptr), mActiveBoardIndex(-1),
    mBoardListActionGroup(this), mErcMsgDock(nullptr), mUnplacedComponentsDock(nullptr),
    mFsm(nullptr)
{
    mUi->setupUi(this);
    mUi->actionProjectSave->setEnabled(!readOnly);

    // set window title
    QString filenameStr = mProject.getFilepath().getFilename();
    if (readOnly) filenameStr.append(QStringLiteral(" [Read-Only]"));
    setWindowTitle(QString("%1 - LibrePCB Board Editor").arg(filenameStr));

    // Add Dock Widgets
    mErcMsgDock = new ErcMsgDock(mProject);
    addDockWidget(Qt::RightDockWidgetArea, mErcMsgDock, Qt::Vertical);
    mUnplacedComponentsDock = new UnplacedComponentsDock(mProject);
    addDockWidget(Qt::RightDockWidgetArea, mUnplacedComponentsDock, Qt::Vertical);

    // create default grid properties
    mGridProperties = new GridProperties();

    // add graphics view as central widget
    mGraphicsView = new GraphicsView(nullptr, this);
    mGraphicsView->setGridProperties(*mGridProperties);
    mGraphicsView->setBackgroundBrush(Qt::black);
    mGraphicsView->setForegroundBrush(Qt::white);
    //setCentralWidget(mGraphicsView);
    mUi->centralwidget->layout()->addWidget(mGraphicsView);

    // add all boards to the menu and connect to project signals
    for (int i=0; i<mProject.getBoards().count(); i++)
        boardAdded(i);
    connect(&mProject, &Project::boardAdded, this, &BoardEditor::boardAdded);
    connect(&mProject, &Project::boardRemoved, this, &BoardEditor::boardRemoved);
    connect(&mBoardListActionGroup, &QActionGroup::triggered,
            this, &BoardEditor::boardListActionGroupTriggered);

    // connect some actions which are created with the Qt Designer
    connect(mUi->actionProjectSave, &QAction::triggered, &mProject, &Project::saveProject);
    connect(mUi->actionQuit, &QAction::triggered, this, &BoardEditor::close);
    connect(mUi->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(mUi->actionZoomIn, &QAction::triggered, mGraphicsView, &GraphicsView::zoomIn);
    connect(mUi->actionZoomOut, &QAction::triggered, mGraphicsView, &GraphicsView::zoomOut);
    connect(mUi->actionZoomAll, &QAction::triggered, mGraphicsView, &GraphicsView::zoomAll);
    connect(mUi->actionShowControlPanel, &QAction::triggered,
            &Workspace::instance(), &Workspace::showControlPanel);
    connect(mUi->actionShowSchematicEditor, &QAction::triggered,
            &mProject, &Project::showSchematicEditor);
    connect(mUi->actionProjectSettings, &QAction::triggered,
            [this](){mProject.getSettings().showSettingsDialog(this);});

    // connect the undo/redo actions with the UndoStack of the project
    connect(&mProject.getUndoStack(), &UndoStack::undoTextChanged,
            [this](const QString& text){mUi->actionUndo->setText(text);});
    mUi->actionUndo->setText(mProject.getUndoStack().getUndoText());
    connect(&mProject.getUndoStack(), &UndoStack::canUndoChanged,
            mUi->actionUndo, &QAction::setEnabled);
    mUi->actionUndo->setEnabled(mProject.getUndoStack().canUndo());
    connect(&mProject.getUndoStack(), &UndoStack::redoTextChanged,
            [this](const QString& text){mUi->actionRedo->setText(text);});
    mUi->actionRedo->setText(mProject.getUndoStack().getRedoText());
    connect(&mProject.getUndoStack(), &UndoStack::canRedoChanged,
            mUi->actionRedo, &QAction::setEnabled);
    mUi->actionRedo->setEnabled(mProject.getUndoStack().canRedo());

    // build the whole board editor finite state machine with all its substate objects
    mFsm = new BES_FSM(*this, *mUi, *mGraphicsView);

    // connect the "tools" toolbar with the state machine (the second line of the lambda
    // functions is a workaround to set the checked attribute of the QActions properly)
    connect(mUi->actionToolSelect, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::StartSelect), true);
                     mUi->actionToolSelect->setChecked(mUi->actionToolSelect->isCheckable());});
    /*connect(mUi->actionToolMove, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(SEE_Base::StartMove), true);
                     mUi->actionToolMove->setChecked(mUi->actionToolMove->isCheckable());});
    connect(mUi->actionToolDrawText, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(SEE_Base::StartDrawText), true);
                     mUi->actionToolDrawText->setChecked(mUi->actionToolDrawText->isCheckable());});
    connect(mUi->actionToolDrawRectangle, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(SEE_Base::StartDrawRect), true);
                     mUi->actionToolDrawRectangle->setChecked(mUi->actionToolDrawRectangle->isCheckable());});
    connect(mUi->actionToolDrawPolygon, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(SEE_Base::StartDrawPolygon), true);
                     mUi->actionToolDrawPolygon->setChecked(mUi->actionToolDrawPolygon->isCheckable());});
    connect(mUi->actionToolDrawCircle, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(SEE_Base::StartDrawCircle), true);
                     mUi->actionToolDrawCircle->setChecked(mUi->actionToolDrawCircle->isCheckable());});
    connect(mUi->actionToolDrawEllipse, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(SEE_Base::StartDrawEllipse), true);
                     mUi->actionToolDrawEllipse->setChecked(mUi->actionToolDrawEllipse->isCheckable());});
    connect(mUi->actionToolDrawWire, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(SEE_Base::StartDrawWire), true);
                     mUi->actionToolDrawWire->setChecked(mUi->actionToolDrawWire->isCheckable());});
    connect(mUi->actionToolAddNetLabel, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(SEE_Base::StartAddNetLabel), true);
                     mUi->actionToolAddNetLabel->setChecked(mUi->actionToolAddNetLabel->isCheckable());});*/

    // connect the "command" toolbar with the state machine
    connect(mUi->actionCommandAbort, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::AbortCommand), true);});

    // connect the "edit" toolbar with the state machine
    connect(mUi->actionCopy, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_Copy), true);});
    connect(mUi->actionCut, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_Cut), true);});
    connect(mUi->actionPaste, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_Paste), true);});
    connect(mUi->actionRotate_CW, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_RotateCW), true);});
    connect(mUi->actionRotate_CCW, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_RotateCCW), true);});
    connect(mUi->actionRemove, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_Remove), true);});

    // Restore Window Geometry
    QSettings clientSettings;
    restoreGeometry(clientSettings.value("board_editor/window_geometry").toByteArray());
    restoreState(clientSettings.value("board_editor/window_state").toByteArray());

    // Load first board
    if (mProject.getBoards().count() > 0)
        setActiveBoardIndex(0);

    // mUi->graphicsView->zoomAll(); does not work properly here, should be executed later...
    QTimer::singleShot(200, mGraphicsView, &GraphicsView::zoomAll); // ...in the event loop
}

BoardEditor::~BoardEditor()
{
    // Save Window Geometry
    QSettings clientSettings;
    clientSettings.setValue("board_editor/window_geometry", saveGeometry());
    clientSettings.setValue("board_editor/window_state", saveState());

    delete mFsm;                    mFsm = nullptr;
    qDeleteAll(mBoardListActions);  mBoardListActions.clear();
    delete mUnplacedComponentsDock; mUnplacedComponentsDock = nullptr;
    delete mErcMsgDock;             mErcMsgDock = nullptr;
    delete mGraphicsView;           mGraphicsView = nullptr;
    delete mGridProperties;         mGridProperties = nullptr;
    delete mUi;                     mUi = nullptr;
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

Board* BoardEditor::getActiveBoard() const noexcept
{
    return mProject.getBoardByIndex(mActiveBoardIndex);
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

bool BoardEditor::setActiveBoardIndex(int index) noexcept
{
    if (index == mActiveBoardIndex)
        return true;

    Board* board = getActiveBoard();
    if (board)
    {
        // save current view scene rect
        board->saveViewSceneRect(mGraphicsView->getVisibleSceneRect());
        // uncheck QAction
        QAction* action = mBoardListActions.value(mActiveBoardIndex); Q_ASSERT(action);
        if (action) action->setChecked(false);
    }
    board = mProject.getBoardByIndex(index);
    if (board)
    {
        // show scene, restore view scene rect, set grid properties
        board->showInView(*mGraphicsView);
        mGraphicsView->setVisibleSceneRect(board->restoreViewSceneRect());
        mGraphicsView->setGridProperties(board->getGridProperties());
        // check QAction
        QAction* action = mBoardListActions.value(index); Q_ASSERT(action);
        if (action) action->setChecked(true);
    }
    else
    {
        mGraphicsView->setScene(nullptr);
    }
    mUnplacedComponentsDock->setBoard(board);

    // active board has changed!
    emit activeBoardChanged(mActiveBoardIndex, index);
    mActiveBoardIndex = index;
    return true;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void BoardEditor::abortAllCommands() noexcept
{
    // ugly... ;-)
    mFsm->processEvent(new BEE_Base(BEE_Base::AbortCommand), true);
    mFsm->processEvent(new BEE_Base(BEE_Base::AbortCommand), true);
    mFsm->processEvent(new BEE_Base(BEE_Base::AbortCommand), true);
}

/*****************************************************************************************
 *  Inherited Methods
 ****************************************************************************************/

void BoardEditor::closeEvent(QCloseEvent* event)
{
    if (!mProject.windowIsAboutToClose(this))
        event->ignore();
    else
        QMainWindow::closeEvent(event);
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void BoardEditor::boardAdded(int newIndex)
{
    Board* board = mProject.getBoardByIndex(newIndex);
    Q_ASSERT(board); if (!board) return;

    QAction* actionBefore = mBoardListActions.value(newIndex-1);
    //if (!actionBefore) actionBefore = TODO
    QAction* newAction = new QAction(board->getName(), this);
    newAction->setCheckable(true);
    mUi->menuBoard->insertAction(actionBefore, newAction);
    mBoardListActions.insert(newIndex, newAction);
    mBoardListActionGroup.addAction(newAction);
}

void BoardEditor::boardRemoved(int oldIndex)
{
    QAction* action = mBoardListActions.takeAt(oldIndex); Q_ASSERT(action);
    mBoardListActionGroup.removeAction(action);
    delete action;
}

/*****************************************************************************************
 *  Actions
 ****************************************************************************************/

void BoardEditor::on_actionProjectClose_triggered()
{
    mProject.close(this);
}

void BoardEditor::on_actionNewBoard_triggered()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Add board"),
                       tr("Choose a name:"), QLineEdit::Normal, tr("default"), &ok);
    if (!ok) return;

    try
    {
        CmdBoardAdd* cmd = new CmdBoardAdd(mProject, name);
        mProject.getUndoStack().execCmd(cmd);
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

void BoardEditor::on_actionUndo_triggered()
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

void BoardEditor::on_actionRedo_triggered()
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

void BoardEditor::on_actionGrid_triggered()
{
    GridSettingsDialog dialog(*mGridProperties, this);
    connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
            [this](const GridProperties& grid)
            {   *mGridProperties = grid;
                mGraphicsView->setGridProperties(grid);
            });
    if (dialog.exec())
    {
        foreach (Board* board, mProject.getBoards())
            board->setGridProperties(*mGridProperties);
        mProject.setModifiedFlag();
    }
}

void BoardEditor::on_actionExportAsPdf_triggered()
{
    try
    {
        QString filename = QFileDialog::getSaveFileName(this, tr("PDF Export"),
                                                        QDir::homePath(), "*.pdf");
        if (filename.isEmpty()) return;
        if (!filename.endsWith(".pdf")) filename.append(".pdf");
        //FilePath filepath(filename);
        //mProject.exportSchematicsAsPdf(filepath); // this method can throw an exception
    }
    catch (Exception& e)
    {
        QMessageBox::warning(this, tr("Error"), e.getUserMsg());
    }
}

void BoardEditor::on_actionProjectProperties_triggered()
{
    ProjectPropertiesEditorDialog dialog(mProject, this);
    dialog.exec();
}

void BoardEditor::boardListActionGroupTriggered(QAction* action)
{
    setActiveBoardIndex(mBoardListActions.indexOf(action));
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool BoardEditor::graphicsViewEventHandler(QEvent* event)
{
    BEE_RedirectedQEvent* e = new BEE_RedirectedQEvent(BEE_Base::GraphicsViewEvent, event);
    return mFsm->processEvent(e, true);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
