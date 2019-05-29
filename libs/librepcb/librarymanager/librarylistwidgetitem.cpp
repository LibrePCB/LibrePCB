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
#include "librarylistwidgetitem.h"

#include "ui_librarylistwidgetitem.h"

#include <librepcb/workspace/workspace.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace manager {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

LibraryListWidgetItem::LibraryListWidgetItem(workspace::Workspace& ws,
                                             const FilePath&       libDir,
                                             const QString&        name,
                                             const QString&        description,
                                             const QPixmap& icon) noexcept
  : QWidget(nullptr),
    mUi(new Ui::LibraryListWidgetItem),
    mLibDir(libDir),
    mIsRemoteLibrary(libDir.isLocatedInDir(ws.getRemoteLibrariesPath())) {
  mUi->setupUi(this);

  if (mLibDir.isValid()) {
    if (!icon.isNull()) {
      mUi->lblIcon->setPixmap(icon);
    }
    mUi->lblLibraryName->setText(name);
    mUi->lblLibraryDescription->setText(description);
    QString path = libDir.toRelative(ws.getLibrariesPath());
    path.replace("local/", "<font color=\"blue\">local</font>/");
    path.replace("remote/", "<font color=\"red\">remote</font>/");
    mUi->lblLibraryUrl->setText(path);
  } else {
    QPixmap image(":/img/actions/add.png");
    mUi->lblIcon->setPixmap(image.scaled(
        mUi->lblIcon->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    mUi->lblLibraryName->setText(tr("Add a new library"));
    mUi->lblLibraryDescription->setText(tr("Click here to add a new library."));
    mUi->lblLibraryUrl->setText("");
  }

  if (mUi->lblLibraryDescription->text().isEmpty()) {
    mUi->lblLibraryDescription->setVisible(false);
  }
}

LibraryListWidgetItem::~LibraryListWidgetItem() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString LibraryListWidgetItem::getName() const noexcept {
  return mUi->lblLibraryName->text();
}

/*******************************************************************************
 *  Inherited from QWidget
 ******************************************************************************/

void LibraryListWidgetItem::mouseDoubleClickEvent(QMouseEvent* e) noexcept {
  if (mLibDir.isValid()) {
    emit openLibraryEditorTriggered(mLibDir);
    e->accept();
  } else {
    QWidget::mouseDoubleClickEvent(e);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace manager
}  // namespace library
}  // namespace librepcb
