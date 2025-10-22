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

#ifndef LIBREPCB_EDITOR_PROJECTEDITOR_H
#define LIBREPCB_EDITOR_PROJECTEDITOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../utils/uiobjectlist.h"
#include "appwindow.h"

#include <librepcb/core/project/projectloader.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class NetSignal;
class Project;
class RuleCheckMessage;
class Workspace;

namespace editor {

class BoardEditor;
class GuiApplication;
class RuleCheckMessagesModel;
class SchematicEditor;
class SchematicTab;
class UndoStack;

/*******************************************************************************
 *  Class ProjectEditor
 ******************************************************************************/

/**
 * @brief The ProjectEditor class
 */
class ProjectEditor final : public QObject {
  Q_OBJECT

public:
  // Signals
  Signal<ProjectEditor> onUiDataChanged;

  // Constructors / Destructor
  ProjectEditor() = delete;
  ProjectEditor(const ProjectEditor& other) = delete;
  explicit ProjectEditor(
      GuiApplication& app, std::unique_ptr<Project> project, int uiIndex,
      const std::optional<ProjectLoader::MigrationLog>& migrationLog,
      QObject* parent = nullptr) noexcept;
  ~ProjectEditor() noexcept;

  // General Methods
  GuiApplication& getApp() noexcept { return mApp; }
  Workspace& getWorkspace() noexcept { return mWorkspace; }
  Project& getProject() noexcept { return *mProject; }
  UndoStack& getUndoStack() noexcept { return *mUndoStack; }
  const QVector<std::shared_ptr<SchematicEditor>>& getSchematics() noexcept {
    return mSchematics->values();
  }
  const QVector<std::shared_ptr<BoardEditor>>& getBoards() noexcept {
    return mBoards->values();
  }
  int getUiIndex() const noexcept { return mUiIndex; }
  void setUiIndex(int index) noexcept;
  ui::ProjectData getUiData() const noexcept;
  void setUiData(const ui::ProjectData& data) noexcept;
  void trigger(ui::ProjectAction a) noexcept;
  bool getUseIeee315Symbols() const noexcept { return mUseIeee315Symbols; }
  std::shared_ptr<const QSet<const NetSignal*>> getHighlightedNetSignals()
      const noexcept {
    return mHighlightedNetSignals;
  }
  void setHighlightedNetSignals(
      const QSet<const NetSignal*>& netSignals) noexcept;

  bool hasUnsavedChanges() const noexcept;

  void undo() noexcept;
  void redo() noexcept;

  /**
   * @brief Request to close the project
   *
   * If there are unsaved changes to the project, this method will ask the user
   * whether the changes should be saved or not. If the user clicks on "cancel"
   * or the project could not be saved successfully, this method will return
   * false. If there were no unsaved changes or they were successfully saved,
   * the method returns true.
   *
   * @retval true   Project is safe to be closed.
   * @retval false  Project still has unsaved changes.
   */
  bool requestClose() noexcept;

  /**
   * @brief Save the whole project to the harddisc
   *
   * @note The whole save procedere is described in @ref doc_project_save.
   *
   * @return true on success, false on failure
   */
  bool saveProject() noexcept;

  /**
   * @brief Make a automatic backup of the project (save to temporary files)
   *
   * @note The whole save procedere is described in @ref doc_project_save.
   *
   * @return true on success, false on failure
   */
  bool autosaveProject() noexcept;

  /**
   * @brief Set the flag that manual modifications (no undo stack) are made
   */
  void setManualModificationsMade() noexcept;

  void execSetupDialog() noexcept;

  void execOutputJobsDialog(const QString& typeName = QString()) noexcept;

  void execBomReviewDialog(const Board* board) noexcept;

  /**
   * @brief Execute the *.lppz export dialog (blocking)
   *
   * @param parent    parent widget of the dialog
   */
  void execLppzExportDialog(QWidget* parent) noexcept;

  std::shared_ptr<SchematicEditor> execNewSheetDialog() noexcept;
  void execRenameSheetDialog(int index) noexcept;
  void execDeleteSheetDialog(int index) noexcept;

  std::shared_ptr<BoardEditor> execNewBoardDialog(
      std::optional<int> copyFromIndex) noexcept;
  void execDeleteBoardDialog(int index) noexcept;

  void registerActiveSchematicTab(SchematicTab* tab) noexcept;
  void unregisterActiveSchematicTab(SchematicTab* tab) noexcept;

  // Operator Overloadings
  ProjectEditor& operator=(const ProjectEditor& rhs) = delete;

signals:
  void uiIndexChanged();
  void manualModificationsMade();
  void projectAboutToBeSaved();
  void projectSavedToDisk();
  void ercUnapprovedCountChanged();
  void ercMessageHighlightRequested(std::shared_ptr<const RuleCheckMessage> msg,
                                    bool zoomTo, int windowId);
  void ercMarkersInvalidated();
  void highlightedNetSignalsChanged();
  void projectLibraryUpdaterRequested(const FilePath& fp);
  void statusBarMessageChanged(const QString& message, int timeoutMs);

  /**
   * @brief Abort any active (blocking) tools in other editors
   *
   * If an undo command group is already active while starting a new tool, try
   * to abort any active tool in other editors since it is annoying to block
   * one editor by another editor (an error message would appear). However, do
   * NOT abort tools in the own editor since this could lead to
   * unexpected/wrong behavior (e.g. recursion)!
   *
   * @param source  The calling editor (any kind of type), which will not be
   *                aborted. Typically, a ::librepcb::editor::WindowTab pointer
   *                is passed. Pass `nullptr` to abort in all editors.
   */
  void abortBlockingToolsInOtherEditors(const void* source);

private:
  /**
   * @brief Open the HTML with all project file format upgrade messages
   */
  void openMigrationLog() noexcept;
  void scheduleErcRun() noexcept;
  void runErc() noexcept;
  void projectSettingsChanged() noexcept;

private:
  GuiApplication& mApp;
  Workspace& mWorkspace;
  std::unique_ptr<Project> mProject;
  int mUiIndex;
  bool mUseIeee315Symbols;
  std::optional<ProjectLoader::MigrationLog> mMigrationLog;
  std::shared_ptr<UiObjectList<SchematicEditor, ui::SchematicData>> mSchematics;
  std::shared_ptr<UiObjectList<BoardEditor, ui::BoardData>> mBoards;
  std::unique_ptr<UndoStack> mUndoStack;

  std::shared_ptr<QSet<const NetSignal*>> mHighlightedNetSignals;
  QVector<QPointer<SchematicTab>> mActiveSchematicTabs;

  // ERC
  std::shared_ptr<RuleCheckMessagesModel> mErcMessages;  // Lazy initialized
  QSet<SExpression> mSupportedErcApprovals;
  QSet<SExpression> mDisappearedErcApprovals;
  QString mErcExecutionError;
  QTimer mErcTimer;

  /// Modifications bypassing the undo stack
  bool mManualModificationsMade;

  /// The UndoStack state ID of the last successful project (auto)save
  uint mLastAutosaveStateId;

  /// The timer for the periodically automatic saving
  /// functionality (see also @ref doc_project_save)
  QTimer mAutoSaveTimer;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
