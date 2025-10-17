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

#ifndef LIBREPCB_EDITOR_LIBRARYEDITORTAB_H
#define LIBREPCB_EDITOR_LIBRARYEDITORTAB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../windowtab.h"

#include <librepcb/core/rulecheck/rulecheckmessage.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;
class TransactionalDirectory;

namespace editor {

class LibraryEditor;
class RuleCheckMessagesModel;
class UndoStack;

/*******************************************************************************
 *  Class LibraryEditorTab
 ******************************************************************************/

/**
 * @brief Specialized base class for all library editor tabs
 */
class LibraryEditorTab : public WindowTab {
  Q_OBJECT

public:
  // Constructors / Destructor
  LibraryEditorTab() = delete;
  LibraryEditorTab(const LibraryEditorTab& other) = delete;
  explicit LibraryEditorTab(LibraryEditor& editor,
                            QObject* parent = nullptr) noexcept;
  virtual ~LibraryEditorTab() noexcept;

  // General Methods
  virtual FilePath getDirectoryPath() const noexcept = 0;

  // Operator Overloadings
  LibraryEditorTab& operator=(const LibraryEditorTab& rhs) = delete;

protected:
  bool isPathOutsideLibDir() const noexcept;
  bool hasUnsavedChanges() const noexcept;
  void setWatchedFiles(const TransactionalDirectory& dir,
                       const QSet<QString>& filenames) noexcept;
  virtual void watchedFilesModifiedChanged() noexcept {}
  virtual void reloadFromDisk() {}
  void scheduleChecks() noexcept;
  void runChecks() noexcept;
  virtual std::optional<std::pair<RuleCheckMessageList, QSet<SExpression>>>
      runChecksImpl() = 0;
  virtual bool autoFixImpl(const std::shared_ptr<const RuleCheckMessage>& msg,
                           bool checkOnly) = 0;
  virtual void messageApprovalChanged(const SExpression& approval,
                                      bool approved) noexcept = 0;
  virtual void notifyDerivedUiDataChanged() noexcept = 0;
  QString getWorkspaceSettingsUserName() const noexcept;

private:
  bool autoFixHandler(const std::shared_ptr<const RuleCheckMessage>& msg,
                      bool checkOnly) noexcept;
  void watchedFileChanged(const QString& path) noexcept;
  void watchedFilesModifiedTimerElapsed() noexcept;

protected:
  LibraryEditor& mEditor;
  std::unique_ptr<UndoStack> mUndoStack;
  bool mManualModificationsMade;

  // Rule check
  QSet<SExpression> mSupportedApprovals;
  QSet<SExpression> mDisappearedApprovals;
  std::shared_ptr<RuleCheckMessagesModel> mCheckMessages;
  slint::SharedString mCheckError;
  QTimer mRuleCheckDelayTimer;

  // Monitoring of file modifications
  QFileSystemWatcher mFileSystemWatcher;
  QHash<FilePath, QByteArray> mWatchedFileHashes;  ///< To detect modifications
  QSet<FilePath> mModifiedWatchedFiles;  ///< Modified, but not reloaded yet
  bool mAutoReloadOnFileModifications;  ///< Set by derived classes
  QTimer mWatchedFilesTimer;  ///< To delay/aggregate the notification & reload
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
