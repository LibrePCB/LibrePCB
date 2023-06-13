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

#ifndef LIBREPCB_EDITOR_EDITORWIDGETBASE_H
#define LIBREPCB_EDITOR_EDITORWIDGETBASE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../dialogs/graphicsexportdialog.h"
#include "../undostack.h"
#include "../widgets/rulechecklistwidget.h"

#include <librepcb/core/fileio/transactionalfilesystem.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class LibraryBaseElement;
class Point;
class Point;
class Workspace;

namespace editor {

class ExclusiveActionGroup;
class IF_GraphicsLayerProvider;
class StatusBar;
class ToolBarProxy;
class UndoStackActionGroup;

/*******************************************************************************
 *  Class EditorWidgetBase
 ******************************************************************************/

/**
 * @brief The EditorWidgetBase class
 */
class EditorWidgetBase : public QWidget, protected IF_RuleCheckHandler {
  Q_OBJECT

public:
  // Types

  struct Context {
    Workspace& workspace;
    const IF_GraphicsLayerProvider& layerProvider;
    bool elementIsNewlyCreated;
    bool readOnly;
  };

  enum Tool {
    NONE,
    SELECT,
    DRAW_LINE,
    DRAW_RECT,
    DRAW_POLYGON,
    DRAW_CIRCLE,
    DRAW_ARC,
    DRAW_TEXT,
    DRAW_ZONE,
    ADD_NAMES,
    ADD_VALUES,
    ADD_PINS,
    ADD_THT_PADS,
    ADD_SMT_PADS,
    ADD_HOLES,
    MEASURE,
  };

  enum class Feature {
    // Handled by editor widgets (constant).
    Close,
    Filter,
    GraphicsView,
    OpenGlView,
    ExportGraphics,

    // Handled by FSM states (dynamic).
    SelectGraphics,
    ImportGraphics,
    Abort,
    Cut,
    Copy,
    Paste,
    Remove,
    Move,
    Rotate,
    Mirror,
    Flip,
    SnapToGrid,
    Properties,
  };

  // Constructors / Destructor
  EditorWidgetBase() = delete;
  EditorWidgetBase(const EditorWidgetBase& other) = delete;
  explicit EditorWidgetBase(const Context& context, const FilePath& fp,
                            QWidget* parent = nullptr);
  virtual ~EditorWidgetBase() noexcept;

  // Getters
  const FilePath& getFilePath() const noexcept { return mFilePath; }
  bool isDirty() const noexcept {
    return mManualModificationsMade || (!mUndoStack->isClean());
  }
  virtual QSet<Feature> getAvailableFeatures() const noexcept = 0;

  // Setters
  virtual void connectEditor(UndoStackActionGroup& undoStackActionGroup,
                             ExclusiveActionGroup& toolsActionGroup,
                             QToolBar& commandToolBar,
                             StatusBar& statusBar) noexcept;
  virtual void disconnectEditor() noexcept;

  // Operator Overloadings
  EditorWidgetBase& operator=(const EditorWidgetBase& rhs) = delete;

public slots:
  virtual bool save() noexcept;
  virtual bool selectAll() noexcept { return false; }
  virtual bool cut() noexcept { return false; }
  virtual bool copy() noexcept { return false; }
  virtual bool paste() noexcept { return false; }
  virtual bool move(Qt::ArrowType direction) noexcept {
    Q_UNUSED(direction);
    return false;
  }
  virtual bool rotate(const librepcb::Angle& rotation) noexcept {
    Q_UNUSED(rotation);
    return false;
  }
  virtual bool mirror(Qt::Orientation orientation) noexcept {
    Q_UNUSED(orientation);
    return false;
  }
  virtual bool flip(Qt::Orientation orientation) noexcept {
    Q_UNUSED(orientation);
    return false;
  }
  virtual bool snapToGrid() noexcept { return false; }
  virtual bool remove() noexcept { return false; }
  virtual bool editProperties() noexcept { return false; }
  virtual bool zoomIn() noexcept { return false; }
  virtual bool zoomOut() noexcept { return false; }
  virtual bool zoomAll() noexcept { return false; }
  virtual bool toggle3D() noexcept { return false; }
  virtual bool abortCommand() noexcept { return false; }
  virtual bool importDxf() noexcept { return false; }
  virtual bool exportImage() noexcept;
  virtual bool exportPdf() noexcept;
  virtual bool print() noexcept;
  virtual bool editGridProperties() noexcept { return false; }
  virtual bool increaseGridInterval() noexcept { return false; }
  virtual bool decreaseGridInterval() noexcept { return false; }

protected:  // Methods
  void setupInterfaceBrokenWarningWidget(QWidget& widget) noexcept;
  void setupErrorNotificationWidget(QWidget& widget) noexcept;
  virtual bool isInterfaceBroken() const noexcept = 0;
  virtual bool toolChangeRequested(Tool newTool,
                                   const QVariant& mode) noexcept {
    Q_UNUSED(newTool);
    Q_UNUSED(mode);
    return false;
  }
  virtual bool runChecks(RuleCheckMessageList& msgs) const = 0;
  void setMessageApproved(LibraryBaseElement& element,
                          std::shared_ptr<const RuleCheckMessage> msg,
                          bool approve) noexcept;
  virtual bool execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                        const QString& settingsKey) noexcept {
    Q_UNUSED(output);
    Q_UNUSED(settingsKey);
    return false;
  }
  void undoStackStateModified() noexcept;
  void setStatusBarMessage(const QString& message, int timeoutMs = -1) noexcept;
  const QStringList& getLibLocaleOrder() const noexcept;
  QString getWorkspaceSettingsUserName() noexcept;

private slots:
  void updateCheckMessages() noexcept;

private:  // Methods
  /**
   * @brief Ask the user whether to restore a backup of a library element
   *
   * @param dir   The library element directory to be restored.
   *
   * @retval true   Restore backup.
   * @retval false  Do not restore backup.
   *
   * @throw Exception to abort opening the library element.
   */
  static bool askForRestoringBackup(const FilePath& dir);
  void toolRequested(int tool, const QVariant& mode) noexcept;
  void undoStackCleanChanged(bool clean) noexcept;
  void scheduleLibraryElementChecks() noexcept;
  virtual bool processRuleCheckMessage(
      std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) = 0;
  bool ruleCheckFixAvailable(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  void ruleCheckFixRequested(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  void ruleCheckDescriptionRequested(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  void ruleCheckMessageSelected(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;
  void ruleCheckMessageDoubleClicked(
      std::shared_ptr<const RuleCheckMessage> msg) noexcept override;

signals:
  void dirtyChanged(bool dirty);
  void elementEdited(const librepcb::FilePath& fp);
  void interfaceBrokenChanged(bool broken);
  void errorsAvailableChanged(bool hasErrors);
  void availableFeaturesChanged(
      const QSet<librepcb::editor::EditorWidgetBase::Feature>& features);

protected:  // Data
  Context mContext;
  FilePath mFilePath;
  std::shared_ptr<TransactionalFileSystem> mFileSystem;
  QScopedPointer<UndoStack> mUndoStack;
  UndoStackActionGroup* mUndoStackActionGroup;
  ExclusiveActionGroup* mToolsActionGroup;
  StatusBar* mStatusBar;
  QScopedPointer<ToolBarProxy> mCommandToolBarProxy;
  bool mManualModificationsMade;  ///< Modifications bypassing the undo stack.
  bool mIsInterfaceBroken;
  QString mStatusBarMessage;

  // Memorized message approvals
  QSet<SExpression> mSupportedApprovals;
  QSet<SExpression> mDisappearedApprovals;
};

inline uint qHash(const EditorWidgetBase::Feature& feature,
                  uint seed = 0) noexcept {
  return ::qHash(static_cast<int>(feature), seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
