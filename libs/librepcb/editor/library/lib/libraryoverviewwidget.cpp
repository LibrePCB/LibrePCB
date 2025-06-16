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
#include "libraryoverviewwidget.h"

#include "../../dialogs/filedialog.h"
#include "../../editorcommandset.h"
#include "../../library/cmd/cmdlibraryedit.h"
#include "../../utils/menubuilder.h"
#include "../../widgets/waitingspinnerwidget.h"
#include "librarylisteditorwidget.h"
#include "ui_libraryoverviewwidget.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/librarybaseelementcheckmessages.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
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

LibraryOverviewWidget::LibraryOverviewWidget(const Context& context,
                                             const FilePath& fp,
                                             QWidget* parent)
  : EditorWidgetBase(context, fp, parent), mUi(new Ui::LibraryOverviewWidget) {
  mUi->setupUi(this);
}

LibraryOverviewWidget::~LibraryOverviewWidget() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QSet<EditorWidgetBase::Feature> LibraryOverviewWidget::getAvailableFeatures()
    const noexcept {
  return {};
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool LibraryOverviewWidget::save() noexcept {
  return false;
}

bool LibraryOverviewWidget::remove() noexcept {
  if (QListWidget* list = dynamic_cast<QListWidget*>(focusWidget())) {
    QHash<QListWidgetItem*, FilePath> selectedItemPaths /*=
        getElementListItemFilePaths(list->selectedItems())*/
        ;
    if (!selectedItemPaths.empty()) {
      removeItems(selectedItemPaths);
      return true;
    }
  }

  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool LibraryOverviewWidget::runChecks(RuleCheckMessageList& msgs) const {
  return true;
}

bool LibraryOverviewWidget::processRuleCheckMessage(
    std::shared_ptr<const RuleCheckMessage> msg, bool applyFix) {
  return false;
}

void LibraryOverviewWidget::ruleCheckApproveRequested(
    std::shared_ptr<const RuleCheckMessage> msg, bool approve) noexcept {
}

void LibraryOverviewWidget::removeItems(
    const QHash<QListWidgetItem*, FilePath>& selectedItemPaths) noexcept {
  // Build message (list only the first few elements to avoid a huge message
  // box)
  QString msg = tr("WARNING: Library elements must normally NOT be removed "
                   "because this will break other elements which depend on "
                   "this one! They should be just marked as deprecated "
                   "instead.\n\nAre you still sure to delete the following "
                   "library elements?") %
      "\n\n";
  QList<QListWidgetItem*> listedItems = selectedItemPaths.keys().mid(0, 10);
  foreach (QListWidgetItem* item, listedItems) {
    msg.append(" - " % item->text() % "\n");
  }
  if (selectedItemPaths.count() > listedItems.count()) {
    msg.append(" - ...\n");
  }
  msg.append("\n" % tr("This cannot be undone!"));

  // Show message box
  int ret = QMessageBox::warning(
      this, tr("Remove %1 elements").arg(selectedItemPaths.count()), msg,
      QMessageBox::Yes, QMessageBox::Cancel);
  if (ret == QMessageBox::Yes) {
    foreach (QListWidgetItem* item, selectedItemPaths.keys()) {
      FilePath itemPath = selectedItemPaths.value(item);
      try {
        // Emit signal so that the library editor can close any tabs that have
        // opened this item
        emit removeElementTriggered(itemPath);
        FileUtils::removeDirRecursively(itemPath);
        delete item;  // Remove from list
      } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
      }
    }
    mContext.workspace.getLibraryDb().startLibraryRescan();
  }
}

void LibraryOverviewWidget::copyElementsToOtherLibrary(
    const QHash<QListWidgetItem*, FilePath>& selectedItemPaths,
    const FilePath& libFp, const QString& libName) noexcept {
  // Build message (list only the first few elements to avoid a huge message
  // box)
  QString msg =
      tr("Are you sure to move the following elements into the library '%1'?")
          .arg(libName) %
      "\n\n";
  QList<QListWidgetItem*> listedItems = selectedItemPaths.keys().mid(0, 10);
  foreach (QListWidgetItem* item, listedItems) {
    msg.append(" - " % item->text() % "\n");
  }
  if (selectedItemPaths.count() > listedItems.count()) {
    msg.append(" - ...\n");
  }
  msg.append("\n" % tr("Note: This operation cannot be easily undone!") % "\n");

  // Show confirmation dialog.
  QDialog dialog(this);
  dialog.setWindowTitle(tr("Move %1 elements").arg(selectedItemPaths.count()));
  QVBoxLayout* vLayoutOuter = new QVBoxLayout(&dialog);
  QHBoxLayout* hLayoutTop = new QHBoxLayout();
  vLayoutOuter->addItem(hLayoutTop);
  hLayoutTop->setSpacing(9);
  QVBoxLayout* vLayoutLeft = new QVBoxLayout();
  hLayoutTop->addItem(vLayoutLeft);
  QLabel* lblIcon = new QLabel(&dialog);
  lblIcon->setPixmap(QPixmap(":/img/status/dialog_warning.png"));
  lblIcon->setScaledContents(true);
  lblIcon->setFixedSize(48, 48);
  vLayoutLeft->addWidget(lblIcon);
  vLayoutLeft->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum,
                                       QSizePolicy::MinimumExpanding));
  QVBoxLayout* vLayoutRight = new QVBoxLayout();
  hLayoutTop->addItem(vLayoutRight);
  QLabel* lblMsg = new QLabel(msg, &dialog);
  lblMsg->setMinimumWidth(300);
  lblMsg->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  lblMsg->setWordWrap(true);
  vLayoutRight->addWidget(lblMsg);
  QHBoxLayout* hLayoutBot = new QHBoxLayout();
  hLayoutBot->setSpacing(9);
  vLayoutOuter->addItem(hLayoutBot);
  QCheckBox* cbxKeep = new QCheckBox(
      tr("Keep elements in current library (make a copy)"), &dialog);
  cbxKeep->setChecked(mContext.readOnly);
  cbxKeep->setEnabled(!mContext.readOnly);
  hLayoutBot->addWidget(cbxKeep);
  hLayoutBot->setStretch(0, 1);
  QDialogButtonBox* btnBox =
      new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::Cancel,
                           Qt::Horizontal, &dialog);
  connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  hLayoutBot->addWidget(btnBox);
  const int ret = dialog.exec();

  if (ret == QDialog::Accepted) {
    foreach (QListWidgetItem* item, selectedItemPaths.keys()) {
      FilePath itemPath = selectedItemPaths.value(item);
      QString relativePath =
          itemPath.toRelative(itemPath.getParentDir().getParentDir());
      FilePath destination = libFp.getPathTo(relativePath);
      try {
        if (!cbxKeep->isChecked()) {
          qInfo().nospace()
              << "Move library element from " << itemPath.toNative() << " to "
              << destination.toNative() << "...";
          // Emit signal so that the library editor can close any tabs that have
          // opened this item
          emit removeElementTriggered(itemPath);
          FileUtils::move(itemPath, destination);
          delete item;  // Remove from list
        } else {
          qInfo().nospace()
              << "Copy library element from " << itemPath.toNative() << " to "
              << destination.toNative() << "...";
          FileUtils::copyDirRecursively(itemPath, destination);
        }
      } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
      }
    }
    mContext.workspace.getLibraryDb().startLibraryRescan();
  }
}

QList<LibraryOverviewWidget::LibraryMenuItem>
    LibraryOverviewWidget::getLocalLibraries() const noexcept {
  QList<LibraryMenuItem> libs;
  try {
    QMultiMap<Version, FilePath> libraries =
        mContext.workspace.getLibraryDb().getAll<Library>();  // can throw
    foreach (const FilePath& libDir, libraries) {
      // Don't list remote libraries since they are read-only!
      if (libDir.isLocatedInDir(mContext.workspace.getLocalLibrariesPath())) {
        QString name;
        mContext.workspace.getLibraryDb().getTranslations<Library>(
            libDir, getLibLocaleOrder(), &name);  // can throw
        QPixmap icon;
        mContext.workspace.getLibraryDb().getLibraryMetadata(
            libDir,
            &icon);  // can throw
        libs.append(LibraryMenuItem{name, icon, libDir});
      }
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to list local libraries:" << e.getMsg();
  }
  // sort by name
  std::sort(libs.begin(), libs.end(),
            [](const LibraryMenuItem& lhs, const LibraryMenuItem& rhs) {
              return lhs.name < rhs.name;
            });
  return libs;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
