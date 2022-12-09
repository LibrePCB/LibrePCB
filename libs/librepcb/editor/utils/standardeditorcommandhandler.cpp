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
#include "standardeditorcommandhandler.h"

#include "../dialogs/aboutdialog.h"
#include "../editorcommandset.h"
#include "../workspace/desktopservices.h"
#include "shortcutsreferencegenerator.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/utils/scopeguard.h>

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

StandardEditorCommandHandler::StandardEditorCommandHandler(
    const WorkspaceSettings& settings, QWidget* parent) noexcept
  : QObject(parent), mSettings(settings), mParent(parent) {
}

StandardEditorCommandHandler::~StandardEditorCommandHandler() noexcept {
}

/*******************************************************************************
 *  Action Handlers
 ******************************************************************************/

void StandardEditorCommandHandler::aboutLibrePcb() const noexcept {
  AboutDialog aboutDialog(mParent);
  aboutDialog.exec();
}

void StandardEditorCommandHandler::onlineDocumentation() const noexcept {
  DesktopServices ds(mSettings, mParent);
  ds.openWebUrl(QUrl("https://docs.librepcb.org"));
}

void StandardEditorCommandHandler::website() const noexcept {
  DesktopServices ds(mSettings, mParent);
  ds.openWebUrl(QUrl("https://librepcb.org"));
}

void StandardEditorCommandHandler::fileManager(const FilePath& fp) const
    noexcept {
  DesktopServices ds(mSettings, mParent);
  ds.openLocalPath(fp);
}

void StandardEditorCommandHandler::shortcutsReference() const noexcept {
  try {
    // This can take some time, use wait cursor to provide UI feedback
    // (keeping it a bit longer since opening the PDF reader takes some time).
    mParent->setCursor(Qt::WaitCursor);
    auto cursorScopeGuard = scopeGuard(
        [this]() { QTimer::singleShot(1000, mParent, &QWidget::unsetCursor); });

    FilePath fp = FilePath::getApplicationTempPath().getPathTo(
        "librepcb-shortcuts-reference.pdf");
    ShortcutsReferenceGenerator generator(EditorCommandSet::instance());
    generator.generatePdf(fp);

    DesktopServices ds(mSettings, mParent);
    ds.openLocalPath(fp);
  } catch (const Exception& e) {
    QMessageBox::critical(mParent, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
