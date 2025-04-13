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

#ifndef LIBREPCB_EDITOR_PROJECTEDITOR2_H
#define LIBREPCB_EDITOR_PROJECTEDITOR2_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/serialization/fileformatmigration.h>
#include <librepcb/core/serialization/sexpression.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetSignal;
class Project;
class Workspace;

namespace editor {

class GuiApplication;
class RuleCheckMessagesModel;
class SchematicTab;
class UndoStack;

/*******************************************************************************
 *  Class ProjectEditor2
 ******************************************************************************/

/**
 * @brief The ProjectEditor2 class
 */
class ProjectEditor2 : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectEditor2() = delete;
  ProjectEditor2(const ProjectEditor2& other) = delete;
  explicit ProjectEditor2(
      GuiApplication& app, std::unique_ptr<Project> project,
      const std::optional<QList<FileFormatMigration::Message>>& upgradeMessages,
      QObject* parent = nullptr) noexcept;
  virtual ~ProjectEditor2() noexcept;

  // Getters
  Project& getProject() noexcept { return *mProject; }
  UndoStack& getUndoStack() noexcept { return *mUndoStack; }
  auto getErcMessages() noexcept { return mErcMessages; }
  const QString& getErcExecutionError() noexcept { return mErcExecutionError; }

  void registerActiveSchematicTab(SchematicTab* tab) noexcept;

  void unregisterActiveSchematicTab(SchematicTab* tab) noexcept;

  /**
   * @brief Show a dialog with all project file format upgrade messages
   */
  void showUpgradeMessages() noexcept;

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

  std::shared_ptr<const QSet<const NetSignal*>> getHighlightedNetSignals()
      const noexcept {
    return mHighlightedNetSignals;
  }
  void setHighlightedNetSignals(
      const QSet<const NetSignal*>& netSignals) noexcept;

  // Operator Overloadings
  ProjectEditor2& operator=(const ProjectEditor2& rhs) = delete;

signals:
  void manualModificationsMade();
  void projectAboutToBeSaved();
  void projectSavedToDisk();
  void ercFinished();
  void highlightedNetSignalsChanged();

private:
  void runErc() noexcept;

private:
  GuiApplication& mApp;
  Workspace& mWorkspace;
  std::unique_ptr<Project> mProject;
  QList<FileFormatMigration::Message> mUpgradeMessages;
  QVector<QPointer<SchematicTab>> mActiveSchematicTabs;
  std::unique_ptr<UndoStack> mUndoStack;

  QSet<SExpression> mSupportedErcApprovals;
  QSet<SExpression> mDisappearedErcApprovals;
  std::shared_ptr<RuleCheckMessagesModel> mErcMessages;
  QString mErcExecutionError;

  /// Modifications bypassing the undo stack
  bool mManualModificationsMade;

  /// The UndoStack state ID of the last successful project (auto)save
  uint mLastAutosaveStateId;

  /// The timer for the periodically automatic saving
  /// functionality (see also @ref doc_project_save)
  QTimer mAutoSaveTimer;

  std::shared_ptr<QSet<const NetSignal*>> mHighlightedNetSignals;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
