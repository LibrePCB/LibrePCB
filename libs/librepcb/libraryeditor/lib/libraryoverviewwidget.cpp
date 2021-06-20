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

#include "librarylisteditorwidget.h"
#include "ui_libraryoverviewwidget.h"

#include <librepcb/common/dialogs/filedialog.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/library/cmd/cmdlibraryedit.h>
#include <librepcb/library/elements.h>
#include <librepcb/library/msg/msgmissingauthor.h>
#include <librepcb/library/msg/msgnamenottitlecase.h>
#include <librepcb/workspace/library/workspacelibrarydb.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryOverviewWidget::LibraryOverviewWidget(const Context& context,
                                             const FilePath& fp,
                                             QWidget* parent)
  : EditorWidgetBase(context, fp, parent),
    mUi(new Ui::LibraryOverviewWidget),
    mCurrentFilter() {
  mUi->setupUi(this);
  mUi->lstMessages->setHandler(this);
  connect(mUi->btnIcon, &QPushButton::clicked, this,
          &LibraryOverviewWidget::btnIconClicked);
  connect(mUi->lstCmpCat, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstDoubleClicked);
  connect(mUi->lstPkgCat, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstDoubleClicked);
  connect(mUi->lstSym, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstDoubleClicked);
  connect(mUi->lstPkg, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstDoubleClicked);
  connect(mUi->lstCmp, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstDoubleClicked);
  connect(mUi->lstDev, &QListWidget::doubleClicked, this,
          &LibraryOverviewWidget::lstDoubleClicked);

  // Insert dependencies editor widget.
  mDependenciesEditorWidget.reset(
      new LibraryListEditorWidget(mContext.workspace, this));
  int row;
  QFormLayout::ItemRole role;
  mUi->formLayout->getWidgetPosition(mUi->lblDependencies, &row, &role);
  mUi->formLayout->setWidget(row, QFormLayout::FieldRole,
                             mDependenciesEditorWidget.data());

  // Load library.
  mLibrary.reset(new Library(std::unique_ptr<TransactionalDirectory>(
      new TransactionalDirectory(mFileSystem))));
  updateMetadata();

  // Reload metadata on undo stack state changes.
  connect(mUndoStack.data(), &UndoStack::stateModified, this,
          &LibraryOverviewWidget::updateMetadata);

  // Handle changes of metadata.
  connect(mUi->edtName, &QLineEdit::editingFinished, this,
          &LibraryOverviewWidget::commitMetadata);
  connect(mUi->edtDescription, &PlainTextEdit::editingFinished, this,
          &LibraryOverviewWidget::commitMetadata);
  connect(mUi->edtKeywords, &QLineEdit::editingFinished, this,
          &LibraryOverviewWidget::commitMetadata);
  connect(mUi->edtAuthor, &QLineEdit::editingFinished, this,
          &LibraryOverviewWidget::commitMetadata);
  connect(mUi->edtVersion, &QLineEdit::editingFinished, this,
          &LibraryOverviewWidget::commitMetadata);
  connect(mUi->cbxDeprecated, &QCheckBox::clicked, this,
          &LibraryOverviewWidget::commitMetadata);
  connect(mUi->edtUrl, &QLineEdit::editingFinished, this,
          &LibraryOverviewWidget::commitMetadata);
  connect(mDependenciesEditorWidget.data(), &LibraryListEditorWidget::edited,
          this, &LibraryOverviewWidget::commitMetadata);

  // Set up context menu triggers
  connect(mUi->lstCmpCat, &QListWidget::customContextMenuRequested, this,
          &LibraryOverviewWidget::openContextMenuAtPos);
  connect(mUi->lstSym, &QListWidget::customContextMenuRequested, this,
          &LibraryOverviewWidget::openContextMenuAtPos);
  connect(mUi->lstCmp, &QListWidget::customContextMenuRequested, this,
          &LibraryOverviewWidget::openContextMenuAtPos);
  connect(mUi->lstPkgCat, &QListWidget::customContextMenuRequested, this,
          &LibraryOverviewWidget::openContextMenuAtPos);
  connect(mUi->lstPkg, &QListWidget::customContextMenuRequested, this,
          &LibraryOverviewWidget::openContextMenuAtPos);
  connect(mUi->lstDev, &QListWidget::customContextMenuRequested, this,
          &LibraryOverviewWidget::openContextMenuAtPos);

  // Load all library elements.
  updateElementLists();
  connect(&mContext.workspace.getLibraryDb(),
          &workspace::WorkspaceLibraryDb::scanFinished, this,
          &LibraryOverviewWidget::updateElementLists);
}

LibraryOverviewWidget::~LibraryOverviewWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void LibraryOverviewWidget::setFilter(const QString& filter) noexcept {
  mCurrentFilter = filter.toLower().trimmed();
  updateElementListFilter(*mUi->lstCmpCat);
  updateElementListFilter(*mUi->lstPkgCat);
  updateElementListFilter(*mUi->lstSym);
  updateElementListFilter(*mUi->lstPkg);
  updateElementListFilter(*mUi->lstCmp);
  updateElementListFilter(*mUi->lstDev);
}

/*******************************************************************************
 *  Public Slots
 ******************************************************************************/

bool LibraryOverviewWidget::save() noexcept {
  // Commit metadata.
  QString errorMsg = commitMetadata();
  if (!errorMsg.isEmpty()) {
    QMessageBox::critical(this, tr("Invalid metadata"), errorMsg);
    return false;
  }

  // Save element.
  try {
    mLibrary->save();  // can throw
    mFileSystem->save();  // can throw
    return EditorWidgetBase::save();
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Save failed"), e.getMsg());
    return false;
  }
}

bool LibraryOverviewWidget::remove() noexcept {
  if (QListWidget* list = dynamic_cast<QListWidget*>(focusWidget())) {
    QHash<QListWidgetItem*, FilePath> selectedItemPaths =
        getElementListItemFilePaths(list->selectedItems());
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

void LibraryOverviewWidget::updateMetadata() noexcept {
  setWindowTitle(*mLibrary->getNames().getDefaultValue());
  setWindowIcon(mLibrary->getIconAsPixmap());
  mUi->btnIcon->setIcon(mLibrary->getIconAsPixmap());
  if (mLibrary->getIconAsPixmap().isNull()) {
    mUi->btnIcon->setText(mUi->btnIcon->toolTip());
  } else {
    mUi->btnIcon->setText(QString());
  }
  mUi->edtName->setText(*mLibrary->getNames().getDefaultValue());
  mUi->edtDescription->setPlainText(
      mLibrary->getDescriptions().getDefaultValue());
  mUi->edtKeywords->setText(mLibrary->getKeywords().getDefaultValue());
  mUi->edtAuthor->setText(mLibrary->getAuthor());
  mUi->edtVersion->setText(mLibrary->getVersion().toStr());
  mUi->cbxDeprecated->setChecked(mLibrary->isDeprecated());
  mUi->edtUrl->setText(mLibrary->getUrl().toString());
  mDependenciesEditorWidget->setUuids(mLibrary->getDependencies());
  mIcon = mLibrary->getIcon();
}

QString LibraryOverviewWidget::commitMetadata() noexcept {
  try {
    QScopedPointer<CmdLibraryEdit> cmd(new CmdLibraryEdit(*mLibrary));
    try {
      // throws on invalid name
      cmd->setName("", ElementName(mUi->edtName->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setDescription("", mUi->edtDescription->toPlainText().trimmed());
    cmd->setKeywords("", mUi->edtKeywords->text().trimmed());
    try {
      // throws on invalid version
      cmd->setVersion(Version::fromString(mUi->edtVersion->text().trimmed()));
    } catch (const Exception& e) {
    }
    cmd->setAuthor(mUi->edtAuthor->text().trimmed());
    cmd->setDeprecated(mUi->cbxDeprecated->isChecked());
    cmd->setUrl(QUrl::fromUserInput(mUi->edtUrl->text().trimmed()));
    cmd->setDependencies(mDependenciesEditorWidget->getUuids());
    cmd->setIcon(mIcon);

    // Commit all changes.
    mUndoStack->execCmd(cmd.take());  // can throw

    // Reload metadata into widgets to discard invalid input.
    updateMetadata();
  } catch (const Exception& e) {
    return e.getMsg();
  }
  return QString();
}

bool LibraryOverviewWidget::runChecks(
    LibraryElementCheckMessageList& msgs) const {
  msgs = mLibrary->runChecks();  // can throw
  mUi->lstMessages->setMessages(msgs);
  return true;
}

template <>
void LibraryOverviewWidget::fixMsg(const MsgNameNotTitleCase& msg) {
  mUi->edtName->setText(*msg.getFixedName());
  commitMetadata();
}

template <>
void LibraryOverviewWidget::fixMsg(const MsgMissingAuthor& msg) {
  Q_UNUSED(msg);
  mUi->edtAuthor->setText(getWorkspaceSettingsUserName());
  commitMetadata();
}

template <typename MessageType>
bool LibraryOverviewWidget::fixMsgHelper(
    std::shared_ptr<const LibraryElementCheckMessage> msg, bool applyFix) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (applyFix) fixMsg(*m);  // can throw
      return true;
    }
  }
  return false;
}

bool LibraryOverviewWidget::processCheckMessage(
    std::shared_ptr<const LibraryElementCheckMessage> msg, bool applyFix) {
  if (fixMsgHelper<MsgNameNotTitleCase>(msg, applyFix)) return true;
  if (fixMsgHelper<MsgMissingAuthor>(msg, applyFix)) return true;
  return false;
}

void LibraryOverviewWidget::updateElementLists() noexcept {
  updateElementList<ComponentCategory>(*mUi->lstCmpCat,
                                       QIcon(":/img/places/folder.png"));
  updateElementList<PackageCategory>(*mUi->lstPkgCat,
                                     QIcon(":/img/places/folder_green.png"));
  updateElementList<Symbol>(*mUi->lstSym, QIcon(":/img/library/symbol.png"));
  updateElementList<Package>(*mUi->lstPkg, QIcon(":/img/library/package.png"));
  updateElementList<Component>(*mUi->lstCmp,
                               QIcon(":/img/library/component.png"));
  updateElementList<Device>(*mUi->lstDev, QIcon(":/img/library/device.png"));
}

template <typename ElementType>
void LibraryOverviewWidget::updateElementList(QListWidget& listWidget,
                                              const QIcon& icon) noexcept {
  QHash<FilePath, QString> elementNames;

  try {
    // get all library element names
    QList<FilePath> elements =
        mContext.workspace.getLibraryDb().getLibraryElements<ElementType>(
            mLibrary->getDirectory().getAbsPath());  // can throw
    foreach (const FilePath& filepath, elements) {
      QString name;
      mContext.workspace.getLibraryDb().getElementTranslations<ElementType>(
          filepath, getLibLocaleOrder(), &name);  // can throw
      elementNames.insert(filepath, name);
    }
  } catch (const Exception& e) {
    listWidget.clear();
    QListWidgetItem* item = new QListWidgetItem(&listWidget);
    item->setText(e.getMsg());
    item->setToolTip(e.getMsg());
    item->setIcon(QIcon(":/img/status/dialog_error.png"));
    item->setBackground(Qt::red);
    item->setForeground(Qt::white);
    return;
  }

  // update/remove existing list widget items
  for (int i = listWidget.count() - 1; i >= 0; --i) {
    QListWidgetItem* item = listWidget.item(i);
    Q_ASSERT(item);
    FilePath filePath(item->data(Qt::UserRole).toString());
    if (elementNames.contains(filePath)) {
      item->setText(elementNames.take(filePath));
    } else {
      delete item;
    }
  }

  // add new list widget items
  foreach (const FilePath& fp, elementNames.keys()) {
    QString name = elementNames.value(fp);
    QListWidgetItem* item = new QListWidgetItem(&listWidget);
    item->setText(name);
    item->setToolTip(name);
    item->setData(Qt::UserRole, fp.toStr());
    item->setIcon(icon);
  }

  // apply filter
  updateElementListFilter(listWidget);
}

QHash<QListWidgetItem*, FilePath>
    LibraryOverviewWidget::getElementListItemFilePaths(
        const QList<QListWidgetItem*>& items) const noexcept {
  QHash<QListWidgetItem*, FilePath> itemPaths;
  foreach (QListWidgetItem* item, items) {
    FilePath fp = FilePath(item->data(Qt::UserRole).toString());
    if (fp.isValid()) {
      itemPaths.insert(item, fp);
    } else {
      qWarning() << "File path for item is not valid";
    }
  }
  return itemPaths;
}

void LibraryOverviewWidget::updateElementListFilter(
    QListWidget& listWidget) noexcept {
  for (int i = 0; i < listWidget.count(); ++i) {
    QListWidgetItem* item = listWidget.item(i);
    Q_ASSERT(item);
    item->setHidden((!mCurrentFilter.isEmpty()) &&
                    (!item->text().toLower().contains(mCurrentFilter)));
  }
}

void LibraryOverviewWidget::openContextMenuAtPos(const QPoint& pos) noexcept {
  Q_UNUSED(pos);

  // Get list widget item file paths
  QListWidget* list = dynamic_cast<QListWidget*>(sender());
  Q_ASSERT(list);
  QHash<QListWidgetItem*, FilePath> selectedItemPaths =
      getElementListItemFilePaths(list->selectedItems());
  QHash<QAction*, FilePath> aMoveToLibChildren;

  // Build the context menu
  QMenu menu;
  QAction* aEdit = menu.addAction(QIcon(":/img/actions/edit.png"), tr("Edit"));
  aEdit->setVisible(!selectedItemPaths.isEmpty());
  QAction* aDuplicate =
      menu.addAction(QIcon(":/img/actions/copy.png"), tr("Duplicate"));
  aDuplicate->setVisible(selectedItemPaths.count() == 1);
  QAction* aRemove =
      menu.addAction(QIcon(":/img/actions/delete.png"), tr("Remove"));
  aRemove->setVisible(!selectedItemPaths.isEmpty());
  if (!selectedItemPaths.isEmpty()) {
    QMenu* menuMoveToLib = menu.addMenu(QIcon(":/img/actions/move_to.png"),
                                        tr("Move to other library"));
    foreach (const LibraryMenuItem& item, getLocalLibraries()) {
      if (item.filepath != mLibrary->getDirectory().getAbsPath()) {
        QAction* action = menuMoveToLib->addAction(item.pixmap, item.name);
        aMoveToLibChildren.insert(action, item.filepath);
      }
    }
    // Disable menu item if it doesn't contain children.
    menuMoveToLib->setEnabled(!aMoveToLibChildren.isEmpty());
  }
  QAction* aNew = menu.addAction(QIcon(":/img/actions/new.png"), tr("New"));
  aNew->setVisible(selectedItemPaths.count() <= 1);

  // Set default action
  if (selectedItemPaths.isEmpty()) {
    menu.setDefaultAction(aNew);
  } else {
    menu.setDefaultAction(aEdit);
  }

  // Show context menu, handle action
  QAction* action = menu.exec(QCursor::pos());
  if (action == aEdit) {
    Q_ASSERT(selectedItemPaths.count() > 0);
    foreach (const FilePath& fp, selectedItemPaths) { editItem(list, fp); }
  } else if (action == aDuplicate) {
    Q_ASSERT(selectedItemPaths.count() == 1);
    duplicateItem(list, selectedItemPaths.values().first());
  } else if (action == aRemove) {
    Q_ASSERT(selectedItemPaths.count() > 0);
    removeItems(selectedItemPaths);
  } else if (action == aNew) {
    newItem(list);
  } else if (aMoveToLibChildren.contains(action)) {
    Q_ASSERT(selectedItemPaths.count() > 0);
    moveElementsToOtherLibrary(selectedItemPaths, aMoveToLibChildren[action],
                               action->text());
  }
}

void LibraryOverviewWidget::newItem(QListWidget* list) noexcept {
  if (list == mUi->lstCmpCat) {
    emit newComponentCategoryTriggered();
  } else if (list == mUi->lstPkgCat) {
    emit newPackageCategoryTriggered();
  } else if (list == mUi->lstSym) {
    emit newSymbolTriggered();
  } else if (list == mUi->lstPkg) {
    emit newPackageTriggered();
  } else if (list == mUi->lstCmp) {
    emit newComponentTriggered();
  } else if (list == mUi->lstDev) {
    emit newDeviceTriggered();
  } else if (list) {
    qCritical() << "Unknown list widget!";
  }
}

void LibraryOverviewWidget::duplicateItem(QListWidget* list,
                                          const FilePath& fp) noexcept {
  if (list == mUi->lstCmpCat) {
    emit duplicateComponentCategoryTriggered(fp);
  } else if (list == mUi->lstPkgCat) {
    emit duplicatePackageCategoryTriggered(fp);
  } else if (list == mUi->lstSym) {
    emit duplicateSymbolTriggered(fp);
  } else if (list == mUi->lstPkg) {
    emit duplicatePackageTriggered(fp);
  } else if (list == mUi->lstCmp) {
    emit duplicateComponentTriggered(fp);
  } else if (list == mUi->lstDev) {
    emit duplicateDeviceTriggered(fp);
  } else if (list) {
    qCritical() << "Unknown list widget!";
  }
}

void LibraryOverviewWidget::editItem(QListWidget* list,
                                     const FilePath& fp) noexcept {
  if (list == mUi->lstCmpCat) {
    emit editComponentCategoryTriggered(fp);
  } else if (list == mUi->lstPkgCat) {
    emit editPackageCategoryTriggered(fp);
  } else if (list == mUi->lstSym) {
    emit editSymbolTriggered(fp);
  } else if (list == mUi->lstPkg) {
    emit editPackageTriggered(fp);
  } else if (list == mUi->lstCmp) {
    emit editComponentTriggered(fp);
  } else if (list == mUi->lstDev) {
    emit editDeviceTriggered(fp);
  } else if (list) {
    qCritical() << "Unknown list widget!";
  }
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

void LibraryOverviewWidget::moveElementsToOtherLibrary(
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
  msg.append("\n" % tr("Note: This cannot be easily undone!"));

  // Show message box
  int ret = QMessageBox::warning(
      this, tr("Move %1 elements").arg(selectedItemPaths.count()), msg,
      QMessageBox::Yes, QMessageBox::Cancel);
  if (ret == QMessageBox::Yes) {
    foreach (QListWidgetItem* item, selectedItemPaths.keys()) {
      FilePath itemPath = selectedItemPaths.value(item);
      QString relativePath =
          itemPath.toRelative(itemPath.getParentDir().getParentDir());
      try {
        // Emit signal so that the library editor can close any tabs that have
        // opened this item
        emit removeElementTriggered(itemPath);
        FileUtils::move(itemPath, libFp.getPathTo(relativePath));
        delete item;  // Remove from list
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
        mContext.workspace.getLibraryDb().getLibraries();  // can throw
    foreach (const FilePath& libDir, libraries) {
      // Don't list remote libraries since they are read-only!
      if (libDir.isLocatedInDir(mContext.workspace.getLocalLibrariesPath())) {
        QString name;
        mContext.workspace.getLibraryDb().getElementTranslations<Library>(
            libDir, getLibLocaleOrder(), &name);  // can throw
        QPixmap icon;
        mContext.workspace.getLibraryDb().getLibraryMetadata(
            libDir,
            &icon);  // can throw
        libs.append(LibraryMenuItem{name, icon, libDir});
      }
    }
  } catch (const Exception& e) {
    qCritical() << "Could not list local libraries:" << e.getMsg();
  }
  // sort by name
  std::sort(libs.begin(), libs.end(),
            [](const LibraryMenuItem& lhs, const LibraryMenuItem& rhs) {
              return lhs.name < rhs.name;
            });
  return libs;
}

/*******************************************************************************
 *  Event Handlers
 ******************************************************************************/

void LibraryOverviewWidget::btnIconClicked() noexcept {
  QString fp = FileDialog::getOpenFileName(
      this, tr("Choose library icon"),
      mLibrary->getDirectory().getAbsPath().toNative(),
      tr("Portable Network Graphics (*.png)"));
  if (!fp.isEmpty()) {
    try {
      mIcon = FileUtils::readFile(FilePath(fp));  // can throw
      commitMetadata();
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Could not open file"), e.getMsg());
    }
  }
}

void LibraryOverviewWidget::lstDoubleClicked(
    const QModelIndex& index) noexcept {
  // Get list widget
  QListWidget* list = dynamic_cast<QListWidget*>(sender());
  Q_ASSERT(list);
  QListWidgetItem* item = list->item(index.row());
  FilePath fp =
      item ? FilePath(item->data(Qt::UserRole).toString()) : FilePath();
  if (fp.isValid()) {
    editItem(list, fp);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
