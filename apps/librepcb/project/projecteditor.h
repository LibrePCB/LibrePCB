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

#ifndef LIBREPCB_PROJECT_PROJECTEDITOR_H
#define LIBREPCB_PROJECT_PROJECTEDITOR_H

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

class Project;
class Workspace;

namespace editor {

class UndoStack;

namespace app {

class GuiApplication;

/*******************************************************************************
 *  Class ProjectEditor
 ******************************************************************************/

/**
 * @brief The ProjectEditor class
 */
class ProjectEditor : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectEditor() = delete;
  ProjectEditor(const ProjectEditor& other) = delete;
  explicit ProjectEditor(
      GuiApplication& app, std::unique_ptr<Project> project,
      const std::optional<QList<FileFormatMigration::Message>>& upgradeMessages,
      QObject* parent = nullptr) noexcept;
  virtual ~ProjectEditor() noexcept;

  // Getters
  Project& getProject() noexcept { return *mProject; }
  UndoStack& getUndoStack() noexcept { return *mUndoStack; }
  auto getErcMessages() noexcept { return mErcMessages; }

  bool canSave() const noexcept;

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

  // Operator Overloadings
  ProjectEditor& operator=(const ProjectEditor& rhs) = delete;

signals:
  void manualModificationsMade();
  void projectAboutToBeSaved();
  void projectSavedToDisk();

private:
  void runErc() noexcept;
  void saveErcMessageApprovals(const QSet<SExpression>& approvals) noexcept;

private:
  GuiApplication& mApp;
  Workspace& mWorkspace;
  std::unique_ptr<Project> mProject;
  QList<FileFormatMigration::Message> mUpgradeMessages;
  std::unique_ptr<UndoStack> mUndoStack;

  QSet<SExpression> mSupportedErcApprovals;
  QSet<SExpression> mDisappearedErcApprovals;
  std::shared_ptr<slint::VectorModel<ui::RuleCheckMessageData>> mErcMessages;

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

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
