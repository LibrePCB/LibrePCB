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
#include "fsm/boardeditorfsmadapter.h"
#include "ui_boardeditor.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/rulecheck/rulecheckmessage.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BoardPlaneFragmentsBuilder;
class ComponentInstance;
class Project;
class Theme;

namespace editor {

class BoardEditorFsm;
class BoardGraphicsScene;
class BoardLayersDock;
class ExclusiveActionGroup;
class GraphicsLayerList;
class OpenGlSceneBuilder;
class OpenGlSceneBuilder;
class OpenGlView;
class ProjectEditor;
class RuleCheckDock;
class SearchToolBar;
class StandardEditorCommandHandler;
class ToolBarProxy;
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
                          public IF_GraphicsViewEventHandler,
                          public BoardEditorFsmAdapter {
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
  GraphicsLayerList& getLayers() const noexcept { return *mLayers; }
  Board* getActiveBoard() const noexcept { return mActiveBoard.data(); }
  BoardGraphicsScene* getActiveBoardScene() noexcept {
    return mGraphicsScene.data();
  }

  // Setters
  bool setActiveBoardIndex(int index) noexcept;

  // General Methods
  void abortAllCommands() noexcept;
  void abortBlockingToolsInOtherEditors() noexcept;

  // BoardEditorFsmAdapter
  Board* fsmGetActiveBoard() noexcept override;
  BoardGraphicsScene* fsmGetGraphicsScene() noexcept override;
  bool fsmGetIgnoreLocks() const noexcept override;
  void fsmSetViewCursor(
      const std::optional<Qt::CursorShape>& shape) noexcept override;
  void fsmSetViewGrayOut(bool grayOut) noexcept override;
  void fsmSetViewInfoBoxText(const QString& text) noexcept override;
  void fsmSetViewRuler(
      const std::optional<std::pair<Point, Point>>& pos) noexcept override;
  void fsmSetSceneCursor(const Point& pos, bool cross,
                         bool circle) noexcept override;
  QPainterPath fsmCalcPosWithTolerance(
      const Point& pos, qreal multiplier) const noexcept override;
  Point fsmMapGlobalPosToScenePos(const QPoint& pos) const noexcept override;
  void fsmSetHighlightedNetSignals(
      const QSet<const NetSignal*>& sigs) noexcept override;
  void fsmAbortBlockingToolsInOtherEditors() noexcept override;
  void fsmSetStatusBarMessage(const QString& message,
                              int timeoutMs = -1) noexcept override;
  void fsmToolLeave() noexcept override;
  void fsmToolEnter(BoardEditorState_Select& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawTrace& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddVia& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawPolygon& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddStrokeText& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawPlane& state) noexcept override;
  void fsmToolEnter(BoardEditorState_DrawZone& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddHole& state) noexcept override;
  void fsmToolEnter(BoardEditorState_AddDevice& state) noexcept override;
  void fsmToolEnter(BoardEditorState_Measure& state) noexcept override;

  // Operator Overloadings
  BoardEditor& operator=(const BoardEditor& rhs) = delete;

protected:
  virtual void closeEvent(QCloseEvent* event) noexcept override;

public slots:

  void boardAdded(int newIndex);
  void boardRemoved(int oldIndex);

private slots:

  // Actions
  void on_tabBar_currentChanged(int index);
  void on_lblUnplacedComponentsNote_linkActivated();

private:
  // Private Methods
  void updateEnabledCopperLayers() noexcept;
  void loadLayersVisibility() noexcept;
  void storeLayersVisibility() noexcept;
  void createActions() noexcept;
  void createToolBars() noexcept;
  void createDockWidgets() noexcept;
  void createMenus() noexcept;
  void updateBoardActionGroup() noexcept;
  bool graphicsSceneKeyPressed(
      const GraphicsSceneKeyEvent& e) noexcept override;
  bool graphicsSceneKeyReleased(
      const GraphicsSceneKeyEvent& e) noexcept override;
  bool graphicsSceneMouseMoved(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonPressed(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneLeftMouseButtonDoubleClicked(
      const GraphicsSceneMouseEvent& e) noexcept override;
  bool graphicsSceneRightMouseButtonReleased(
      const GraphicsSceneMouseEvent& e) noexcept override;
  void toolRequested(const QVariant& newTool) noexcept;
  void unplacedComponentsCountChanged(int count) noexcept;
  void runDrc(bool quick) noexcept;
  void highlightDrcMessage(const RuleCheckMessage& msg, bool zoomTo) noexcept;
  void setDrcMessageApproved(const RuleCheckMessage& msg,
                             bool approved) noexcept;
  void clearDrcMarker() noexcept;
  QList<BI_Device*> getSearchCandidates() noexcept;
  QStringList getSearchToolBarCompleterList() noexcept;
  void goToDevice(const QString& name, int index) noexcept;
  void scheduleOpenGlSceneUpdate() noexcept;
  void performScheduledTasks() noexcept;
  void startPlaneRebuild(bool full = false) noexcept;
  bool isActiveTopLevelWindow() const noexcept;
  void newBoard() noexcept;
  void copyBoard() noexcept;
  void removeBoard() noexcept;
  void setGridProperties(const PositiveLength& interval, const LengthUnit& unit,
                         Theme::GridStyle style, bool applyToBoard) noexcept;
  void execGridPropertiesDialog() noexcept;
  void execBoardSetupDialog(bool switchToDrcSettings = false) noexcept;
  void execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept;
  void execStepExportDialog() noexcept;
  void execD356NetlistExportDialog() noexcept;
  void execSpecctraExportDialog() noexcept;
  void execSpecctraImportDialog() noexcept;
  bool show3DView() noexcept;
  void hide3DView() noexcept;

  // General Attributes
  ProjectEditor& mProjectEditor;
  Project& mProject;
  QScopedPointer<Ui::BoardEditor> mUi;
  QScopedPointer<OpenGlView> mOpenGlView;
  QScopedPointer<ToolBarProxy> mCommandToolBarProxy;
  QScopedPointer<StandardEditorCommandHandler> mStandardCommandHandler;

  // Misc
  QPointer<Board> mActiveBoard;
  std::unique_ptr<GraphicsLayerList> mLayers;
  QScopedPointer<BoardGraphicsScene> mGraphicsScene;
  QScopedPointer<OpenGlSceneBuilder> mOpenGlSceneBuilder;
  bool mOpenGlSceneBuildScheduled;
  qint64 mTimestampOfLastOpenGlSceneRebuild;
  QHash<Uuid, QRectF> mVisibleSceneRect;
  QScopedPointer<BoardEditorFsm> mFsm;

  // Plane Fragments Builder
  QScopedPointer<BoardPlaneFragmentsBuilder> mPlaneFragmentsBuilder;
  qint64 mTimestampOfLastPlaneRebuild;

  // DRC
  QHash<Uuid, std::optional<RuleCheckMessageList>>
      mDrcMessages;  ///< UUID=Board
  QScopedPointer<QGraphicsPathItem> mDrcLocationGraphicsItem;

  // Actions
  QScopedPointer<QAction> mActionAboutLibrePcb;
  QScopedPointer<QAction> mActionAboutQt;
  QScopedPointer<QAction> mActionOnlineDocumentation;
  QScopedPointer<QAction> mActionKeyboardShortcutsReference;
  QScopedPointer<QAction> mActionWebsite;
  QScopedPointer<QAction> mActionSaveProject;
  QScopedPointer<QAction> mActionCloseProject;
  QScopedPointer<QAction> mActionCloseWindow;
  QScopedPointer<QAction> mActionQuit;
  QScopedPointer<QAction> mActionFileManager;
  QScopedPointer<QAction> mActionSchematicEditor;
  QScopedPointer<QAction> mActionControlPanel;
  QScopedPointer<QAction> mActionProjectSetup;
  QScopedPointer<QAction> mActionUpdateLibrary;
  QScopedPointer<QAction> mActionBoardSetup;
  QScopedPointer<QAction> mActionRunQuickCheck;
  QScopedPointer<QAction> mActionRunDesignRuleCheck;
  QScopedPointer<QAction> mActionImportDxf;
  QScopedPointer<QAction> mActionImportSpecctra;
  QScopedPointer<QAction> mActionExportLppz;
  QScopedPointer<QAction> mActionExportImage;
  QScopedPointer<QAction> mActionExportPdf;
  QScopedPointer<QAction> mActionExportStep;
  QScopedPointer<QAction> mActionExportSpecctra;
  QScopedPointer<QAction> mActionPrint;
  QScopedPointer<QAction> mActionGenerateBom;
  QScopedPointer<QAction> mActionGenerateFabricationData;
  QScopedPointer<QAction> mActionGeneratePickPlace;
  QScopedPointer<QAction> mActionGenerateD356Netlist;
  QScopedPointer<QAction> mActionOutputJobs;
  QScopedPointer<QAction> mActionOrderPcb;
  QScopedPointer<QAction> mActionNewBoard;
  QScopedPointer<QAction> mActionCopyBoard;
  QScopedPointer<QAction> mActionRemoveBoard;
  QScopedPointer<QAction> mActionNextPage;
  QScopedPointer<QAction> mActionPreviousPage;
  QScopedPointer<QAction> mActionFind;
  QScopedPointer<QAction> mActionFindNext;
  QScopedPointer<QAction> mActionFindPrevious;
  QScopedPointer<QAction> mActionSelectAll;
  QScopedPointer<QAction> mActionGridProperties;
  QScopedPointer<QAction> mActionGridIncrease;
  QScopedPointer<QAction> mActionGridDecrease;
  QScopedPointer<QAction> mActionIgnoreLocks;
  QScopedPointer<QAction> mActionZoomFit;
  QScopedPointer<QAction> mActionZoomIn;
  QScopedPointer<QAction> mActionZoomOut;
  QScopedPointer<QAction> mActionToggle3D;
  QScopedPointer<QAction> mActionUndo;
  QScopedPointer<QAction> mActionRedo;
  QScopedPointer<QAction> mActionCut;
  QScopedPointer<QAction> mActionCopy;
  QScopedPointer<QAction> mActionPaste;
  QScopedPointer<QAction> mActionMoveLeft;
  QScopedPointer<QAction> mActionMoveRight;
  QScopedPointer<QAction> mActionMoveUp;
  QScopedPointer<QAction> mActionMoveDown;
  QScopedPointer<QAction> mActionRotateCcw;
  QScopedPointer<QAction> mActionRotateCw;
  QScopedPointer<QAction> mActionFlipHorizontal;
  QScopedPointer<QAction> mActionFlipVertical;
  QScopedPointer<QAction> mActionSnapToGrid;
  QScopedPointer<QAction> mActionLock;
  QScopedPointer<QAction> mActionUnlock;
  QScopedPointer<QAction> mActionResetAllTexts;
  QScopedPointer<QAction> mActionIncreaseLineWidth;
  QScopedPointer<QAction> mActionDecreaseLineWidth;
  QScopedPointer<QAction> mActionChangeLineWidth;
  QScopedPointer<QAction> mActionProperties;
  QScopedPointer<QAction> mActionRemove;
  QScopedPointer<QAction> mActionShowPlanes;
  QScopedPointer<QAction> mActionHidePlanes;
  QScopedPointer<QAction> mActionRebuildPlanes;
  QScopedPointer<QAction> mActionAbort;
  QScopedPointer<QAction> mActionToolSelect;
  QScopedPointer<QAction> mActionToolTrace;
  QScopedPointer<QAction> mActionToolVia;
  QScopedPointer<QAction> mActionToolPolygon;
  QScopedPointer<QAction> mActionToolText;
  QScopedPointer<QAction> mActionToolPlane;
  QScopedPointer<QAction> mActionToolZone;
  QScopedPointer<QAction> mActionToolHole;
  QScopedPointer<QAction> mActionToolMeasure;
  QScopedPointer<QAction> mActionDockErc;
  QScopedPointer<QAction> mActionDockDrc;
  QScopedPointer<QAction> mActionDockLayers;
  QScopedPointer<QAction> mActionDockPlaceDevices;

  // Action groups
  QScopedPointer<UndoStackActionGroup> mUndoStackActionGroup;
  QScopedPointer<ExclusiveActionGroup> mToolsActionGroup;
  QScopedPointer<QActionGroup> mBoardActionGroup;

  // Toolbars
  QScopedPointer<QToolBar> mToolBarFile;
  QScopedPointer<QToolBar> mToolBarEdit;
  QScopedPointer<QToolBar> mToolBarView;
  QScopedPointer<SearchToolBar> mToolBarSearch;
  QScopedPointer<QToolBar> mToolBarCommand;
  QScopedPointer<QToolBar> mToolBarTools;

  // Docks
  QScopedPointer<UnplacedComponentsDock> mDockUnplacedComponents;
  QScopedPointer<BoardLayersDock> mDockLayers;
  QScopedPointer<RuleCheckDock> mDockErc;
  QScopedPointer<RuleCheckDock> mDockDrc;

  // Menus
  QPointer<QMenu> mMenuBoard;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
