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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "editorwidgetbase.h"

#include "../dialogs/directorylockhandlerdialog.h"
#include "../undostack.h"
#include "../utils/exclusiveactiongroup.h"
#include "../utils/toolbarproxy.h"
#include "../utils/undostackactiongroup.h"
#include "../widgets/statusbar.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

EditorWidgetBase::EditorWidgetBase(const Context& context, const FilePath& fp,
                                   QWidget* parent)
  : QWidget(parent),
    mContext(context),
    mFilePath(fp),
    mFileSystem(TransactionalFileSystem::open(
        fp, !context.readOnly, &askForRestoringBackup,
        DirectoryLockHandlerDialog::createDirectoryLockCallback())),  // can
                                                                      // throw
    mUndoStackActionGroup(nullptr),
    mToolsActionGroup(nullptr),
    mStatusBar(nullptr),
    mIsInterfaceBroken(false),
    mStatusBarMessage() {
  mUndoStack.reset(new UndoStack());
  connect(mUndoStack.data(), &UndoStack::cleanChanged, this,
          &EditorWidgetBase::undoStackCleanChanged);
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &EditorWidgetBase::undoStackStateModified);

  mCommandToolBarProxy.reset(new ToolBarProxy());

  // Run checks, but delay it because the subclass is not loaded yet!
  scheduleLibraryElementChecks();
}

EditorWidgetBase::~EditorWidgetBase() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void EditorWidgetBase::connectEditor(UndoStackActionGroup& undoStackActionGroup,
                                     ExclusiveActionGroup& toolsActionGroup,
                                     QToolBar& commandToolBar,
                                     StatusBar& statusBar) noexcept {
  mUndoStackActionGroup = &undoStackActionGroup;
  mUndoStackActionGroup->setUndoStack(mUndoStack.data());

  mToolsActionGroup = &toolsActionGroup;
  mToolsActionGroup->reset();
  connect(mToolsActionGroup, &ExclusiveActionGroup::changeRequestTriggered,
          this, &EditorWidgetBase::toolActionGroupChangeTriggered);

  mCommandToolBarProxy->setToolBar(&commandToolBar);

  mStatusBar = &statusBar;
  mStatusBar->setPermanentMessage(mStatusBarMessage);
}

void EditorWidgetBase::disconnectEditor() noexcept {
  mUndoStackActionGroup->setUndoStack(nullptr);
  mUndoStackActionGroup = nullptr;

  disconnect(mToolsActionGroup, &ExclusiveActionGroup::changeRequestTriggered,
             this, &EditorWidgetBase::toolActionGroupChangeTriggered);
  mToolsActionGroup->reset();

  mCommandToolBarProxy->setToolBar(nullptr);

  mStatusBar->clearMessage();
  mStatusBar->clearPermanentMessage();
  mStatusBar = nullptr;
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

bool EditorWidgetBase::save() noexcept {
  mIsInterfaceBroken = false;
  mUndoStack->setClean();
  emit dirtyChanged(false);
  emit interfaceBrokenChanged(false);
  emit elementEdited(mFilePath);
  return true;
}

bool EditorWidgetBase::exportImage() noexcept {
  return execGraphicsExportDialog(GraphicsExportDialog::Output::Image,
                                  "image_export");
}
bool EditorWidgetBase::exportPdf() noexcept {
  return execGraphicsExportDialog(GraphicsExportDialog::Output::Pdf,
                                  "pdf_export");
}
bool EditorWidgetBase::print() noexcept {
  return execGraphicsExportDialog(GraphicsExportDialog::Output::Print, "print");
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void EditorWidgetBase::setupInterfaceBrokenWarningWidget(
    QWidget& widget) noexcept {
  widget.setVisible(false);
  widget.setStyleSheet(
      "background-color: rgb(255, 255, 127); "
      "color: rgb(170, 0, 0);");
  QLabel* label = new QLabel(&widget);
  QFont font = label->font();
  font.setBold(true);
  label->setFont(font);
  label->setWordWrap(true);
  label->setText(
      tr("WARNING: You have changed some important properties of this "
         "library element. This breaks all other elements which depend on "
         "this one! Maybe you want to create a new library element instead "
         "of modifying this one?"));
  QHBoxLayout* layout = new QHBoxLayout(&widget);
  layout->addWidget(label);
  connect(this, &EditorWidgetBase::interfaceBrokenChanged, &widget,
          &QWidget::setVisible);
}

void EditorWidgetBase::setupErrorNotificationWidget(QWidget& widget) noexcept {
  widget.setVisible(false);
  widget.setStyleSheet(
      "background-color: rgb(255, 255, 127); "
      "color: rgb(170, 0, 0);");
  QLabel* label = new QLabel(&widget);
  QFont font = label->font();
  font.setBold(true);
  label->setFont(font);
  label->setWordWrap(true);
  label->setText(
      tr("WARNING: This library element contains errors, see exact messages "
         "below. You should fix these errors before saving it, otherwise the "
         "library element may not work as expected."));
  QHBoxLayout* layout = new QHBoxLayout(&widget);
  layout->addWidget(label);
  connect(this, &EditorWidgetBase::errorsAvailableChanged, &widget,
          &QWidget::setVisible);
}

void EditorWidgetBase::undoStackStateModified() noexcept {
  if (!mContext.elementIsNewlyCreated) {
    bool broken = isInterfaceBroken();
    if (broken != mIsInterfaceBroken) {
      mIsInterfaceBroken = broken;
      emit interfaceBrokenChanged(mIsInterfaceBroken);
    }
  }
  scheduleLibraryElementChecks();
}

void EditorWidgetBase::setStatusBarMessage(const QString& message,
                                           int timeoutMs) noexcept {
  if (mStatusBar) {
    if (timeoutMs < 0) {
      mStatusBar->setPermanentMessage(message);
    } else {
      mStatusBar->showMessage(message, timeoutMs);
    }
  }
  if (timeoutMs < 0) {
    mStatusBarMessage = message;
  }
}

const QStringList& EditorWidgetBase::getLibLocaleOrder() const noexcept {
  return mContext.workspace.getSettings().libraryLocaleOrder.get();
}

QString EditorWidgetBase::getWorkspaceSettingsUserName() noexcept {
  QString u = mContext.workspace.getSettings().userName.get();
  if (u.isEmpty()) {
    QMessageBox::warning(
        this, tr("User name not set"),
        tr("No user name defined in workspace settings. Please open "
           "workspace settings to set the default user name."));
  }
  return u;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool EditorWidgetBase::askForRestoringBackup(const FilePath& dir) {
  Q_UNUSED(dir);
  QMessageBox::StandardButton btn = QMessageBox::question(
      0, tr("Restore autosave backup?"),
      tr("It seems that the application crashed the last time you opened this "
         "library element. Do you want to restore the last autosave backup?"),
      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
      QMessageBox::Cancel);
  switch (btn) {
    case QMessageBox::Yes:
      return true;
    case QMessageBox::No:
      return false;
    default:
      throw UserCanceled(__FILE__, __LINE__);
  }
}

void EditorWidgetBase::toolActionGroupChangeTriggered(
    const QVariant& newTool) noexcept {
  toolChangeRequested(static_cast<Tool>(newTool.toInt()));
}

void EditorWidgetBase::undoStackCleanChanged(bool clean) noexcept {
  Q_UNUSED(clean);
  emit dirtyChanged(isDirty());
}

void EditorWidgetBase::scheduleLibraryElementChecks() noexcept {
  // Don't run check immediately when requested. Sometimes when the undo stack
  // reports changes, it's just in the middle of a bigger change, so the whole
  // change is not done yet. In that case, running checks would lead to wrong
  // results. Instead, just delay checks for some time to get more stable
  // messages. But also don't wait too long, otherwise it would feel like a
  // lagging user interface.
  QTimer::singleShot(50, this, &EditorWidgetBase::updateCheckMessages);
}

void EditorWidgetBase::updateCheckMessages() noexcept {
  try {
    LibraryElementCheckMessageList msgs;
    if (runChecks(msgs)) {  // can throw
      int errors = 0;
      foreach (const auto& msg, msgs) {
        if (msg->getSeverity() == LibraryElementCheckMessage::Severity::Error) {
          ++errors;
        }
      }
      emit errorsAvailableChanged(errors > 0);
    } else {
      // Failed to run checks (for example because a command is active), try it
      // later again.
      scheduleLibraryElementChecks();
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to run checks:" << e.getMsg();
  }
}

bool EditorWidgetBase::libraryElementCheckFixAvailable(
    std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept {
  try {
    return processCheckMessage(msg, false);  // can throw, but should really not
  } catch (const Exception&) {
    return false;
  }
}

void EditorWidgetBase::libraryElementCheckFixRequested(
    std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept {
  try {
    processCheckMessage(msg, true);  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void EditorWidgetBase::libraryElementCheckDescriptionRequested(
    std::shared_ptr<const LibraryElementCheckMessage> msg) noexcept {
  if (msg) {
    QMessageBox::information(this, msg->getMessage(), msg->getDescription());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
