/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

#ifndef LIBREPCB_PROJECT_BOARDEDITOR_H
#define LIBREPCB_PROJECT_BOARDEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/graphics/if_graphicsvieweventhandler.h>
#include <librepcb/common/uuid.h>
#include <librepcb/project/boards/board.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsView;
class GridProperties;
class UndoStackActionGroup;
class ExclusiveActionGroup;

namespace project {

class Project;
class ComponentInstance;

namespace editor {

class ProjectEditor;
class ErcMsgDock;
class UnplacedComponentsDock;
class BoardLayersDock;
class BES_FSM;

namespace Ui {
class BoardEditor;
}

/*******************************************************************************
 *  Class BoardEditor
 ******************************************************************************/

/**
 * @brief The BoardEditor class
 */
class BoardEditor final : public QMainWindow,
                          public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit BoardEditor(ProjectEditor& projectEditor, Project& project);
  ~BoardEditor();

  // Getters
  ProjectEditor& getProjectEditor() const noexcept { return mProjectEditor; }
  Project&       getProject() const noexcept { return mProject; }
  Board*         getActiveBoard() const noexcept { return mActiveBoard.data(); }

  // Setters
  void setActiveBoardIndex(int index) noexcept;

  // General Methods
  void abortAllCommands() noexcept;

protected:
  void closeEvent(QCloseEvent* event);

public slots:

  void boardAdded(int newIndex);
  void boardRemoved(int oldIndex);

private slots:

  // Actions
  void on_actionProjectClose_triggered();
  void on_actionNewBoard_triggered();
  void on_actionCopyBoard_triggered();
  void on_actionRemoveBoard_triggered();
  void on_actionGrid_triggered();
  void on_actionExportAsPdf_triggered();
  void on_actionGenerateFabricationData_triggered();
  void on_actionProjectProperties_triggered();
  void on_actionUpdateLibrary_triggered();
  void on_actionLayerStackSetup_triggered();
  void on_actionModifyDesignRules_triggered();
  void on_actionRebuildPlanes_triggered();
  void on_tabBar_currentChanged(int index);
  void on_lblUnplacedComponentsNote_linkActivated();
  void boardListActionGroupTriggered(QAction* action);

private:
  // make some methods inaccessible...
  BoardEditor()                         = delete;
  BoardEditor(const BoardEditor& other) = delete;
  BoardEditor& operator=(const BoardEditor& rhs) = delete;

  // Private Methods
  bool graphicsViewEventHandler(QEvent* event);
  void toolActionGroupChangeTriggered(const QVariant& newTool) noexcept;
  void unplacedComponentsCountChanged(int count) noexcept;

  // General Attributes
  ProjectEditor&                       mProjectEditor;
  Project&                             mProject;
  Ui::BoardEditor*                     mUi;
  GraphicsView*                        mGraphicsView;
  QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
  QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;

  // Misc
  QPointer<Board> mActiveBoard;
  QList<QAction*> mBoardListActions;
  QActionGroup    mBoardListActionGroup;

  // Docks
  ErcMsgDock*             mErcMsgDock;
  UnplacedComponentsDock* mUnplacedComponentsDock;
  BoardLayersDock*        mBoardLayersDock;

  // Finite State Machine
  BES_FSM* mFsm;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BOARDEDITOR_H
