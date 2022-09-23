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

#ifndef LIBREPCB_EDITOR_BOARDEDITOR_H
#define LIBREPCB_EDITOR_BOARDEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../dialogs/graphicsexportdialog.h"
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "ui_boardeditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentInstance;
class Project;

namespace editor {

class BoardDesignRuleCheckMessagesDock;
class BoardEditorFsm;
class BoardLayersDock;
class ErcMsgDock;
class ExclusiveActionGroup;
class GraphicsView;
class ProjectEditor;
class UndoStackActionGroup;
class UnplacedComponentsDock;

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
  BoardEditor() = delete;
  BoardEditor(const BoardEditor& other) = delete;
  explicit BoardEditor(ProjectEditor& projectEditor, Project& project);
  ~BoardEditor();

  // Getters
  ProjectEditor& getProjectEditor() const noexcept { return mProjectEditor; }
  Project& getProject() const noexcept { return mProject; }
  Board* getActiveBoard() const noexcept { return mActiveBoard.data(); }

  // Setters
  bool setActiveBoardIndex(int index) noexcept;

  // General Methods
  void abortAllCommands() noexcept;

  // Operator Overloadings
  BoardEditor& operator=(const BoardEditor& rhs) = delete;

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
  void on_actionGenerateFabricationData_triggered();
  void on_actionGenerateBom_triggered();
  void on_actionGeneratePickPlace_triggered();
  void on_actionProjectProperties_triggered();
  void on_actionUpdateLibrary_triggered();
  void on_actionLayerStackSetup_triggered();
  void on_actionModifyDesignRules_triggered();
  void on_actionDesignRuleCheck_triggered();
  void on_actionRebuildPlanes_triggered();
  void on_actionShowAllPlanes_triggered();
  void on_actionHideAllPlanes_triggered();
  void on_tabBar_currentChanged(int index);
  void on_lblUnplacedComponentsNote_linkActivated();
  void boardListActionGroupTriggered(QAction* action);

private:
  // Private Methods
  bool graphicsViewEventHandler(QEvent* event);
  void toolActionGroupChangeTriggered(const QVariant& newTool) noexcept;
  void unplacedComponentsCountChanged(int count) noexcept;
  void runDrcNonInteractive() noexcept;
  void updateBoardDrcMessages(
      const Board& board,
      const QList<BoardDesignRuleCheckMessage>& messages) noexcept;
  void highlightDrcMessage(const BoardDesignRuleCheckMessage& msg,
                           bool zoomTo) noexcept;
  void clearDrcMarker() noexcept;
  QList<BI_Device*> getSearchCandidates() noexcept;
  QStringList getSearchToolBarCompleterList() noexcept;
  void goToDevice(const QString& name, int index) noexcept;
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;

  // General Attributes
  ProjectEditor& mProjectEditor;
  Project& mProject;
  Ui::BoardEditor* mUi;
  GraphicsView* mGraphicsView;
  QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
  QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;

  // DRC
  BoardDesignRuleCheck::Options mDrcOptions;
  QHash<Uuid, QList<BoardDesignRuleCheckMessage>>
      mDrcMessages;  ///< Key: Board UUID
  QScopedPointer<QGraphicsPathItem> mDrcLocationGraphicsItem;

  // Misc
  QPointer<Board> mActiveBoard;
  QList<QAction*> mBoardListActions;
  QActionGroup mBoardListActionGroup;

  // Docks
  ErcMsgDock* mErcMsgDock;
  UnplacedComponentsDock* mUnplacedComponentsDock;
  BoardLayersDock* mBoardLayersDock;
  QScopedPointer<BoardDesignRuleCheckMessagesDock> mDrcMessagesDock;

  // Finite State Machine
  QScopedPointer<BoardEditorFsm> mFsm;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
