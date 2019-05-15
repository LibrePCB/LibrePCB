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
#include "projectpropertieseditordialog.h"

#include "ui_projectpropertieseditordialog.h"

#include <librepcb/common/undostack.h>
#include <librepcb/project/metadata/cmd/cmdprojectmetadataedit.h>
#include <librepcb/project/metadata/projectmetadata.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectPropertiesEditorDialog::ProjectPropertiesEditorDialog(
    ProjectMetadata& metadata, UndoStack& undoStack, QWidget* parent) noexcept
  : QDialog(parent),
    mMetadata(metadata),
    mUndoStack(undoStack),
    mAttributes(mMetadata.getAttributes()),
    mUi(new Ui::ProjectPropertiesEditorDialog) {
  mUi->setupUi(this);

  mUi->edtName->setText(*mMetadata.getName());
  mUi->edtAuthor->setText(mMetadata.getAuthor());
  mUi->edtVersion->setText(mMetadata.getVersion());
  mUi->lblCreatedDateTime->setText(
      mMetadata.getCreated().toString(Qt::DefaultLocaleLongDate));
  mUi->lblLastModifiedDateTime->setText(
      mMetadata.getLastModified().toString(Qt::DefaultLocaleLongDate));
  mUi->attributeListEditorWidget->setReferences(nullptr, &mAttributes);
}

ProjectPropertiesEditorDialog::~ProjectPropertiesEditorDialog() noexcept {
  mUi->attributeListEditorWidget->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectPropertiesEditorDialog::keyPressEvent(QKeyEvent* e) {
  switch (e->key()) {
    case Qt::Key_Return:
      accept();
      break;
    case Qt::Key_Escape:
      reject();
      break;
    default:
      QDialog::keyPressEvent(e);
      break;
  }
}

void ProjectPropertiesEditorDialog::accept() {
  if (applyChanges()) {
    QDialog::accept();
  }
}

bool ProjectPropertiesEditorDialog::applyChanges() noexcept {
  try {
    CmdProjectMetadataEdit* cmd = new CmdProjectMetadataEdit(mMetadata);
    cmd->setName(ElementName(mUi->edtName->text().trimmed()));  // can throw
    cmd->setAuthor(mUi->edtAuthor->text().trimmed());
    cmd->setVersion(mUi->edtVersion->text().trimmed());
    cmd->setAttributes(mAttributes);
    mUndoStack.execCmd(cmd);
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
