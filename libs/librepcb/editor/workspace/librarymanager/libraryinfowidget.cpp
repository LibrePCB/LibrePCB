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
#include "libraryinfowidget.h"

#include "ui_libraryinfowidget.h"

#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/library.h>
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

LibraryInfoWidget::LibraryInfoWidget(Workspace& ws, const FilePath& libDir)
  : QWidget(nullptr),
    mUi(new Ui::LibraryInfoWidget),
    mWorkspace(ws),
    mLibDir(libDir) {
  mUi->setupUi(this);
  connect(mUi->btnOpenLibraryEditor, &QPushButton::clicked, this,
          &LibraryInfoWidget::btnOpenLibraryEditorClicked);
  connect(mUi->btnRemove, &QPushButton::clicked, this,
          &LibraryInfoWidget::btnRemoveLibraryClicked);

  // try to load the library
  Library lib(
      std::unique_ptr<TransactionalDirectory>(new TransactionalDirectory(
          TransactionalFileSystem::openRO(mLibDir))));  // can throw

  const QStringList& localeOrder = ws.getSettings().libraryLocaleOrder.get();

  // image
  if (!lib.getIconAsPixmap().isNull()) {
    mUi->lblIcon->setPixmap(lib.getIconAsPixmap().scaled(
        mUi->lblIcon->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  } else {
    mUi->lblIcon->setVisible(false);
    mUi->line->setVisible(false);
  }

  // general attributes
  mUi->lblName->setText(*lib.getNames().value(localeOrder));
  mUi->lblDescription->setText(lib.getDescriptions().value(localeOrder));
  mUi->lblVersion->setText(lib.getVersion().toStr());
  mUi->lblAuthor->setText(lib.getAuthor());
  mUi->lblUrl->setText(
      QString("<a href='%1'>%2</a>")
          .arg(lib.getUrl().toEncoded(), lib.getUrl().toDisplayString()));
  mUi->lblCreated->setText(lib.getCreated().toString(Qt::TextDate));
  mUi->lblDeprecated->setText(
      lib.isDeprecated() ? tr("Yes - Consider switching to another library.")
                         : tr("No"));

  // extended attributes
  mUi->lblLibType->setText(isRemoteLibrary() ? tr("Remote") : tr("Local"));
  QString dependencies;
  foreach (const Uuid& uuid, lib.getDependencies()) {
    QString line = dependencies.isEmpty() ? "" : "<br>";
    FilePath fp = ws.getLibraryDb().getLatestLibrary(uuid);  // can throw
    if (fp.isValid()) {
      QString name;
      ws.getLibraryDb().getElementTranslations<Library>(fp, localeOrder,
                                                        &name);  // can throw
      line += QString(" <font color=\"green\">%1 ✔</font>").arg(name);
    } else {
      line += QString(" <font color=\"red\">%1 ✖</font>").arg(uuid.toStr());
    }
    dependencies.append(line);
  }
  mUi->lblDependencies->setText(dependencies);
  mUi->lblDirectory->setText(
      QString("<a href='%1'>%2</a>")
          .arg(mLibDir.toQUrl().toLocalFile(),
               mLibDir.toRelative(ws.getLibrariesPath())));
  mUi->lblDirectory->setToolTip(mLibDir.toNative());
}

LibraryInfoWidget::~LibraryInfoWidget() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void LibraryInfoWidget::btnOpenLibraryEditorClicked() noexcept {
  emit openLibraryEditorTriggered(mLibDir);
}

void LibraryInfoWidget::btnRemoveLibraryClicked() noexcept {
  QString title = tr("Remove Library");
  QString text = tr("Attention! This will remove the whole library directory:"
                    "\n\n%1\n\nAre you really sure to remove \"%2\"?")
                     .arg(mLibDir.toNative(), mUi->lblName->text());

  int res = QMessageBox::question(this, title, text,
                                  QMessageBox::Yes | QMessageBox::No);

  if (res == QMessageBox::Yes) {
    try {
      FileUtils::removeDirRecursively(mLibDir);  // can throw
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Error"), e.getMsg());
    }
    mWorkspace.getLibraryDb().startLibraryRescan();
  }
}

bool LibraryInfoWidget::isRemoteLibrary() const noexcept {
  return mLibDir.isLocatedInDir(mWorkspace.getRemoteLibrariesPath());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
