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
#include <QtWidgets>
#include "boardeditor.h"
#include "ui_boardeditor.h"
#include <librepcb/project/project.h>
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/common/undostack.h>
#include <librepcb/common/utils/undostackactiongroup.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/circuit/circuit.h>
#include <librepcb/common/dialogs/gridsettingsdialog.h>
#include <librepcb/common/dialogs/boarddesignrulesdialog.h>
#include "../dialogs/projectpropertieseditordialog.h"
#include <librepcb/project/settings/projectsettings.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/project/boards/cmd/cmdboardadd.h>
#include <librepcb/project/boards/cmd/cmdboarddesignrulesmodify.h>
#include "../docks/ercmsgdock.h"
#include "unplacedcomponentsdock.h"
#include "fsm/bes_fsm.h"
#include "../projecteditor.h"
#include "boardlayersdock.h"
#include "fabricationoutputdialog.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardEditor::BoardEditor(ProjectEditor& projectEditor, Project& project) :
    QMainWindow(0), mProjectEditor(projectEditor), mProject(project),
    mUi(new Ui::BoardEditor),
    mGraphicsView(nullptr), mActiveBoardIndex(-1), mBoardListActionGroup(this),
    mErcMsgDock(nullptr), mUnplacedComponentsDock(nullptr), mBoardLayersDock(nullptr),
    mFsm(nullptr)
{
    mUi->setupUi(this);
    mUi->actionProjectSave->setEnabled(!mProject.isReadOnly());

    // set window title
    QString filenameStr = mProject.getFilepath().getFilename();
    if (mProject.isReadOnly()) filenameStr.append(QStringLiteral(" [Read-Only]"));
    setWindowTitle(QString("%1 - LibrePCB Board Editor").arg(filenameStr));

    // Add Dock Widgets
    mErcMsgDock = new ErcMsgDock(mProject);
    addDockWidget(Qt::RightDockWidgetArea, mErcMsgDock, Qt::Vertical);
    mUnplacedComponentsDock = new UnplacedComponentsDock(mProjectEditor);
    connect(mUnplacedComponentsDock, &UnplacedComponentsDock::addDeviceTriggered,
            [this](ComponentInstance& cmp, const Uuid& dev, const Uuid& fpt)
            {mFsm->processEvent(new BEE_StartAddDevice(cmp, dev, fpt), true);});
    addDockWidget(Qt::RightDockWidgetArea, mUnplacedComponentsDock, Qt::Vertical);
    mBoardLayersDock = new BoardLayersDock(*this);
    addDockWidget(Qt::RightDockWidgetArea, mBoardLayersDock, Qt::Vertical);

    // add graphics view as central widget
    mGraphicsView = new GraphicsView(nullptr, this);
    mGraphicsView->setUseOpenGl(mProjectEditor.getWorkspace().getSettings().getAppearance().getUseOpenGl());
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
    connect(mUi->actionProjectSave, &QAction::triggered, &mProjectEditor, &ProjectEditor::saveProject);
    connect(mUi->actionQuit, &QAction::triggered, this, &BoardEditor::close);
    connect(mUi->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(mUi->actionZoomIn, &QAction::triggered, mGraphicsView, &GraphicsView::zoomIn);
    connect(mUi->actionZoomOut, &QAction::triggered, mGraphicsView, &GraphicsView::zoomOut);
    connect(mUi->actionZoomAll, &QAction::triggered, mGraphicsView, &GraphicsView::zoomAll);
    connect(mUi->actionShowControlPanel, &QAction::triggered,
            &mProjectEditor, &ProjectEditor::showControlPanelClicked);
    connect(mUi->actionShowSchematicEditor, &QAction::triggered,
            &mProjectEditor, &ProjectEditor::showSchematicEditor);
    connect(mUi->actionEditNetClasses, &QAction::triggered,
            [this](){mProjectEditor.execNetClassesEditorDialog(this);});
    connect(mUi->actionProjectSettings, &QAction::triggered,
            [this](){mProjectEditor.execProjectSettingsDialog(this);});

    // connect the undo/redo actions with the UndoStack of the project
    mUndoStackActionGroup.reset(new UndoStackActionGroup(
        *mUi->actionUndo, *mUi->actionRedo, nullptr, &mProjectEditor.getUndoStack(), this));

    // build the whole board editor finite state machine with all its substate objects
    mFsm = new BES_FSM(*this, *mUi, *mGraphicsView, mProjectEditor.getUndoStack());

    // connect the "tools" toolbar with the state machine (the second line of the lambda
    // functions is a workaround to set the checked attribute of the QActions properly)
    connect(mUi->actionToolSelect, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::StartSelect), true);
                     mUi->actionToolSelect->setChecked(mUi->actionToolSelect->isCheckable());});
    connect(mUi->actionToolDrawTrace, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::StartDrawTrace), true);
                     mUi->actionToolDrawTrace->setChecked(mUi->actionToolDrawTrace->isCheckable());});
    connect(mUi->actionToolAddVia, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::StartAddVia), true);
                     mUi->actionToolAddVia->setChecked(mUi->actionToolAddVia->isCheckable());});

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
    connect(mUi->actionFlipHorizontal, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_FlipHorizontal), true);});
    connect(mUi->actionFlipVertical, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_FlipVertical), true);});
    connect(mUi->actionRemove, &QAction::triggered,
            [this](){mFsm->processEvent(new BEE_Base(BEE_Base::Edit_Remove), true);});

    // Restore Window Geometry
    QSettings clientSettings;
    restoreGeometry(clientSettings.value("board_editor/window_geometry").toByteArray());
    restoreState(clientSettings.value("board_editor/window_state").toByteArray());

    // Load first board
    if (mProject.getBoards().count() > 0)
        setActiveBoardIndex(0);

    // mGraphicsView->zoomAll(); does not work properly here, should be executed later
    // in the event loop (ugly, but seems to work...)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 4, 0))
    QTimer::singleShot(200, mGraphicsView, &GraphicsView::zoomAll);
#else
    QTimer::singleShot(200, mGraphicsView, SLOT(zoomAll()));
#endif
}

BoardEditor::~BoardEditor()
{
    // Save Window Geometry
    QSettings clientSettings;
    clientSettings.setValue("board_editor/window_geometry", saveGeometry());
    clientSettings.setValue("board_editor/window_state", saveState());

    delete mFsm;                    mFsm = nullptr;
    qDeleteAll(mBoardListActions);  mBoardListActions.clear();
    delete mBoardLayersDock;        mBoardLayersDock = nullptr;
    delete mUnplacedComponentsDock; mUnplacedComponentsDock = nullptr;
    delete mErcMsgDock;             mErcMsgDock = nullptr;
    delete mGraphicsView;           mGraphicsView = nullptr;
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

    // active board has changed!
    int oldIndex = mActiveBoardIndex;
    mActiveBoardIndex = index;
    mUnplacedComponentsDock->setBoard(board);
    mBoardLayersDock->setActiveBoard(board);
    mUi->tabBar->setCurrentIndex(index);
    emit activeBoardChanged(oldIndex, index);
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
    if (!mProjectEditor.windowIsAboutToClose(*this))
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

    mUi->tabBar->insertTab(newIndex, board->getName());
}

void BoardEditor::boardRemoved(int oldIndex)
{
    mUi->tabBar->removeTab(oldIndex);
    QAction* action = mBoardListActions.takeAt(oldIndex); Q_ASSERT(action);
    mBoardListActionGroup.removeAction(action);
    delete action;

    if (oldIndex == mActiveBoardIndex) {
        setActiveBoardIndex(0);
    } else if (oldIndex < mActiveBoardIndex) {
        mActiveBoardIndex--;
    }
}

/*****************************************************************************************
 *  Actions
 ****************************************************************************************/

void BoardEditor::on_actionProjectClose_triggered()
{
    mProjectEditor.closeAndDestroy(true, this);
}

void BoardEditor::on_actionNewBoard_triggered()
{
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Add New Board"),
                       tr("Choose a name:"), QLineEdit::Normal, tr("new_board"), &ok);
    if (!ok) return;

    try
    {
        CmdBoardAdd* cmd = new CmdBoardAdd(mProject, name);
        mProjectEditor.getUndoStack().execCmd(cmd);
        setActiveBoardIndex(mProject.getBoardIndex(*cmd->getBoard()));
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

void BoardEditor::on_actionCopyBoard_triggered()
{
    Board* board = getActiveBoard();
    if (!board) return;

    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Copy Board"),
                       tr("Choose a name:"), QLineEdit::Normal,
                       QString(tr("copy_of_%1")).arg(board->getName()), &ok);
    if (!ok) return;

    try
    {
        CmdBoardAdd* cmd = new CmdBoardAdd(mProject, *board, name);
        mProjectEditor.getUndoStack().execCmd(cmd);
        setActiveBoardIndex(mProject.getBoardIndex(*cmd->getBoard()));
    }
    catch (Exception& e)
    {
        QMessageBox::critical(this, tr("Error"), e.getUserMsg());
    }
}

void BoardEditor::on_actionGrid_triggered()
{
    Board* board = getActiveBoard();
    if (board) {
        GridSettingsDialog dialog(board->getGridProperties(), this);
        connect(&dialog, &GridSettingsDialog::gridPropertiesChanged,
                [this](const GridProperties& grid)
                {mGraphicsView->setGridProperties(grid);});
        if (dialog.exec())
        {
            foreach (Board* board, mProject.getBoards())
                board->setGridProperties(dialog.getGrid());
            mGraphicsView->setGridProperties(board->getGridProperties());
            //mProjectEditor.setModifiedFlag(); TODO
        }
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

void BoardEditor::on_actionGenerateFabricationData_triggered()
{
    Board* board = getActiveBoard();
    if (!board) return;

    FabricationOutputDialog dialog(*board, this);
    dialog.exec();
}

void BoardEditor::on_actionProjectProperties_triggered()
{
    ProjectPropertiesEditorDialog dialog(mProject, mProjectEditor.getUndoStack(), this);
    dialog.exec();
}

void BoardEditor::on_actionModifyDesignRules_triggered()
{
    Board* board = getActiveBoard();
    if (!board) return;

    try {
        BoardDesignRules originalRules = board->getDesignRules();
        BoardDesignRulesDialog dialog(board->getDesignRules(), this);
        connect(&dialog, &BoardDesignRulesDialog::rulesChanged,
                [&](const BoardDesignRules& rules){
                board->getDesignRules() = rules;
                emit board->attributesChanged();});
        int result = dialog.exec();
        board->getDesignRules() = originalRules; // important hack ;)
        if (result == QDialog::Accepted) {
            CmdBoardDesignRulesModify* cmd = new CmdBoardDesignRulesModify(*board, dialog.getDesignRules());
            mProjectEditor.getUndoStack().execCmd(cmd);
        }
    } catch (Exception& e) {
        QMessageBox::warning(this, tr("Error"), e.getUserMsg());
    }
}

void BoardEditor::on_tabBar_currentChanged(int index)
{
    setActiveBoardIndex(index);
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

} // namespace editor
} // namespace project
} // namespace librepcb
