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
#include "newprojectwizardpage_metadata.h"

#include "ui_newprojectwizardpage_metadata.h"

#include <librepcb/common/application.h>
#include <librepcb/common/dialogs/filedialog.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

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

NewProjectWizardPage_Metadata::NewProjectWizardPage_Metadata(
    const workspace::Workspace& ws, QWidget* parent) noexcept
  : QWizardPage(parent), mUi(new Ui::NewProjectWizardPage_Metadata) {
  mUi->setupUi(this);
  setPixmap(QWizard::LogoPixmap, QPixmap(":/img/actions/plus_2.png"));
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/img/wizards/watermark.jpg"));

  // signal/slot connections
  connect(mUi->edtName, &QLineEdit::textChanged, this,
          &NewProjectWizardPage_Metadata::nameChanged);
  connect(mUi->edtLocation, &QLineEdit::textChanged, this,
          &NewProjectWizardPage_Metadata::locationChanged);
  connect(mUi->btnLocation, &QPushButton::clicked, this,
          &NewProjectWizardPage_Metadata::chooseLocationClicked);

  // insert values
  mUi->edtAuthor->setText(ws.getSettings().userName.get());
  mUi->cbxLicense->addItem(QString("No License (not recommended)"), QString());
  mUi->cbxLicense->addItem(
      tr("CC0-1.0 (no restrictions, recommended for open hardware projects)"),
      QString("licenses/cc0-1.0.txt"));
  mUi->cbxLicense->addItem(tr("CC-BY-4.0 (requires attribution)"),
                           QString("licenses/cc-by-4.0.txt"));
  mUi->cbxLicense->addItem(
      tr("CC-BY-SA-4.0 (requires attribution + share alike)"),
      QString("licenses/cc-by-sa-4.0.txt"));
  mUi->cbxLicense->setCurrentIndex(0);  // no license
}

NewProjectWizardPage_Metadata::~NewProjectWizardPage_Metadata() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void NewProjectWizardPage_Metadata::setDefaultLocation(
    const FilePath& dir) noexcept {
  mUi->edtLocation->setText(dir.toNative());
  updateProjectFilePath();
  emit completeChanged();
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString NewProjectWizardPage_Metadata::getProjectName() const noexcept {
  return mUi->edtName->text();
}

QString NewProjectWizardPage_Metadata::getProjectAuthor() const noexcept {
  return mUi->edtAuthor->text();
}

bool NewProjectWizardPage_Metadata::isLicenseSet() const noexcept {
  return !mUi->cbxLicense->currentData(Qt::UserRole).toString().isEmpty();
}

FilePath NewProjectWizardPage_Metadata::getProjectLicenseFilePath() const
    noexcept {
  QString licenseFileName =
      mUi->cbxLicense->currentData(Qt::UserRole).toString();
  if (!licenseFileName.isEmpty()) {
    return qApp->getResourcesDir().getPathTo(licenseFileName);
  } else {
    return FilePath();
  }
}

FilePath NewProjectWizardPage_Metadata::getFullFilePath() const noexcept {
  return mFullFilePath;
}

/*******************************************************************************
 *  GUI Action Handlers
 ******************************************************************************/

void NewProjectWizardPage_Metadata::nameChanged(const QString& name) noexcept {
  Q_UNUSED(name);
  updateProjectFilePath();
  emit completeChanged();
}

void NewProjectWizardPage_Metadata::locationChanged(
    const QString& dir) noexcept {
  Q_UNUSED(dir);
  updateProjectFilePath();
  emit completeChanged();
}

void NewProjectWizardPage_Metadata::chooseLocationClicked() noexcept {
  QString dir = FileDialog::getExistingDirectory(
      this, tr("Project's parent directory"), mUi->edtLocation->text());
  if (!dir.isEmpty()) {
    mUi->edtLocation->setText(FilePath(dir).toNative());
    updateProjectFilePath();
    emit completeChanged();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewProjectWizardPage_Metadata::updateProjectFilePath() noexcept {
  // invalidate filepath
  mFullFilePath = FilePath();

  // check filename
  QString name     = mUi->edtName->text();
  QString filename = FilePath::cleanFileName(name, FilePath::ReplaceSpaces);
  if (filename.isEmpty()) {
    mUi->lblFullFilePath->setText(tr("Please enter a valid project name."));
    return;
  }

  // check location
  FilePath location(mUi->edtLocation->text());
  if ((!location.isValid()) || (!location.isExistingDir())) {
    mUi->lblFullFilePath->setText(
        tr("The location must be an existing directory."));
    return;
  }

  // determine project directory and filepath
  FilePath projDir      = location.getPathTo(filename);
  FilePath fullFilePath = projDir.getPathTo(filename % ".lpp");
  if ((!projDir.isValid()) || (!fullFilePath.isValid())) {
    mUi->lblFullFilePath->setText(
        tr("Oops, could not determine a valid filepath."));
    return;
  }

  // the filepath is valid
  mFullFilePath = fullFilePath;
  mUi->lblFullFilePath->setText(fullFilePath.toNative());
}

bool NewProjectWizardPage_Metadata::isComplete() const noexcept {
  // check base class
  if (!QWizardPage::isComplete()) return false;

  // check filepath
  if (!mFullFilePath.isValid()) return false;

  return true;
}

bool NewProjectWizardPage_Metadata::validatePage() noexcept {
  // check base class
  if (!QWizardPage::validatePage()) return false;

  // check if the project's directory does not exists
  FilePath projDir = mFullFilePath.getParentDir();
  if ((projDir.isExistingDir() && !projDir.isEmptyDir()) ||
      (projDir.isExistingFile())) {
    QMessageBox::critical(
        this, tr("Invalid filepath"),
        tr("The project's directory exists already and is not empty."));
    return false;
  }

  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
