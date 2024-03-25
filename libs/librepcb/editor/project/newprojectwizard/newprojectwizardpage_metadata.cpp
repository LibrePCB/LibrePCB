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

#include "../../dialogs/filedialog.h"
#include "../../editorcommandset.h"
#include "../../workspace/desktopservices.h"
#include "ui_newprojectwizardpage_metadata.h"

#include <librepcb/core/application.h>
#include <librepcb/core/types/elementname.h>
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

NewProjectWizardPage_Metadata::NewProjectWizardPage_Metadata(
    const Workspace& ws, QWidget* parent) noexcept
  : QWizardPage(parent),
    mWorkspace(ws),
    mUi(new Ui::NewProjectWizardPage_Metadata),
    mLocation(ws.getProjectsPath()),
    mLocationOverridden(false) {
  mUi->setupUi(this);
  setPixmap(QWizard::LogoPixmap, QPixmap(":/img/actions/plus_2.png"));
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/img/wizards/watermark.jpg"));

  // Add browse action.
  const EditorCommandSet& cmd = EditorCommandSet::instance();
  mUi->edtPath->addAction(
      cmd.inputBrowse.createAction(
          mUi->edtPath, this,
          &NewProjectWizardPage_Metadata::chooseLocationClicked,
          EditorCommand::ActionFlag::WidgetShortcut),
      QLineEdit::TrailingPosition);

  // signal/slot connections
  connect(mUi->edtName, &QLineEdit::textChanged, this,
          &NewProjectWizardPage_Metadata::nameChanged);
  connect(mUi->edtName, &QLineEdit::editingFinished, this, [this]() {
    mUi->edtName->setText(cleanElementName(mUi->edtName->text()));
  });
  connect(mUi->edtPath, &QLineEdit::textChanged, this,
          &NewProjectWizardPage_Metadata::pathChanged);
  connect(mUi->lblLicenseLink, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mWorkspace.getSettings(), this);
            ds.openWebUrl(QUrl(url));
          });

  // insert values
  mUi->edtAuthor->setText(ws.getSettings().userName.get());
  mUi->cbxLicense->addItem(tr("None"), QString());
  // add SPDX license identifiers
  mUi->cbxLicense->addItem(tr("CC0-1.0 (no restrictions)"),
                           QString("licenses/cc0-1.0.txt"));
  mUi->cbxLicense->addItem(tr("CC-BY-4.0 (requires attribution)"),
                           QString("licenses/cc-by-4.0.txt"));
  mUi->cbxLicense->addItem(
      tr("CC-BY-SA-4.0 (requires attribution + share alike)"),
      QString("licenses/cc-by-sa-4.0.txt"));
  mUi->cbxLicense->addItem(
      tr("CC-BY-NC-4.0 (requires attribution + non commercial)"),
      QString("licenses/cc-by-nc-4.0.txt"));
  mUi->cbxLicense->addItem(tr("CC-BY-NC-SA-4.0 (requires attribution + non "
                              "commercial + share alike)"),
                           QString("licenses/cc-by-nc-sa-4.0.txt"));
  mUi->cbxLicense->addItem(tr("CC-BY-NC-ND-4.0 (requires attribution + non "
                              "commercial + no derivatives)"),
                           QString("licenses/cc-by-nc-nd-4.0.txt"));
  mUi->cbxLicense->addItem(
      tr("CC-BY-ND-4.0 (requires attribution + no derivatives)"),
      QString("licenses/cc-by-nd-4.0.txt"));
  mUi->cbxLicense->addItem(tr("TAPR-OHL-1.0"),
                           QString("licenses/tapr-ohl-1.0.txt"));
  mUi->cbxLicense->addItem(tr("CERN-OHL-P-2.0 (permissive)"),
                           QString("licenses/cern-ohl-p-2.0.txt"));
  mUi->cbxLicense->addItem(tr("CERN-OHL-W-2.0 (weakly reciprocal)"),
                           QString("licenses/cern-ohl-w-2.0.txt"));
  mUi->cbxLicense->addItem(tr("CERN-OHL-S-2.0 (strongly reciprocal)"),
                           QString("licenses/cern-ohl-s-2.0.txt"));
  mUi->cbxLicense->setCurrentIndex(0);  // no license

  // Restore client settings.
  QSettings cs;
  const QString loc = cs.value(QString("new_project_wizard/location/%1")
                                   .arg(mWorkspace.getPath().toStr()))
                          .toString();
  if (QDir::isAbsolutePath(loc)) {
    mLocation.setPath(loc);
  } else if (!loc.isEmpty()) {
    mLocation = mWorkspace.getPath().getPathTo(loc);
  }

  // Update UI state.
  nameChanged(mUi->edtName->text());
}

NewProjectWizardPage_Metadata::~NewProjectWizardPage_Metadata() noexcept {
  // Save client settings.
  QSettings cs;
  if (!mLocationOverridden) {
    cs.setValue(QString("new_project_wizard/location/%1")
                    .arg(mWorkspace.getPath().toStr()),
                mLocation.isLocatedInDir(mWorkspace.getPath())
                    ? mLocation.toRelative(mWorkspace.getPath())
                    : mLocation.toStr());
  }
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void NewProjectWizardPage_Metadata::setProjectName(
    const QString& name) noexcept {
  mUi->edtName->setText(name);
}

void NewProjectWizardPage_Metadata::setLocationOverride(
    const FilePath& dir) noexcept {
  mLocationOverridden = true;
  mLocation = dir;
  pathChanged(mUi->edtName->text());
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

FilePath NewProjectWizardPage_Metadata::getProjectLicenseFilePath()
    const noexcept {
  QString licenseFileName =
      mUi->cbxLicense->currentData(Qt::UserRole).toString();
  if (!licenseFileName.isEmpty()) {
    return Application::getResourcesDir().getPathTo(licenseFileName);
  } else {
    return FilePath();
  }
}

/*******************************************************************************
 *  GUI Action Handlers
 ******************************************************************************/

void NewProjectWizardPage_Metadata::nameChanged(const QString& name) noexcept {
  if (cleanElementName(name).isEmpty()) {
    mUi->edtPath->clear();
    mUi->edtPath->setPlaceholderText(tr("Please enter a project name"));
    mUi->edtPath->setEnabled(false);
  } else {
    QString fname = FilePath::cleanFileName(name, FilePath::ReplaceSpaces);
    if (fname.isEmpty()) {
      fname = "project";
    }
    const FilePath fp = mLocation.getPathTo(fname % "/" % fname % ".lpp");
    mUi->edtPath->setText(fp.toNative());
    mUi->edtPath->setPlaceholderText(QString());
    mUi->edtPath->setEnabled(true);
  }
  emit completeChanged();
}

void NewProjectWizardPage_Metadata::pathChanged(const QString& fp) noexcept {
  mFullFilePath.setPath(fp);
  if (mFullFilePath.isValid() && (mFullFilePath.getSuffix() == "lpp")) {
    mLocation = mFullFilePath.getParentDir().getParentDir();
  }
  emit completeChanged();
}

void NewProjectWizardPage_Metadata::chooseLocationClicked() noexcept {
  const FilePath fp(FileDialog::getExistingDirectory(
      this, tr("Project's parent directory"), mLocation.toStr()));
  if (fp.isValid()) {
    mLocation = fp;
    nameChanged(mUi->edtName->text());
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool NewProjectWizardPage_Metadata::isComplete() const noexcept {
  // Check project name.
  if (cleanElementName(mUi->edtName->text()).isEmpty()) {
    setStatusMessage(QString());
    return false;
  }

  // Check file path.
  if ((!mFullFilePath.isValid()) || (mFullFilePath.getSuffix() != "lpp") ||
      (mFullFilePath.getBasename().isEmpty())) {
    setStatusMessage(
        "<font color=\"red\">⚠ " %
        tr("Please enter a valid project path with '%1' file extension.")
            .arg(".lpp")
            .toHtmlEscaped() %
        "</font>");
    return false;
  }

  // Check parent directory.
  const FilePath parent = mFullFilePath.getParentDir();
  if ((parent.isExistingDir() && (!parent.isEmptyDir()))) {
    setStatusMessage(
        "<font color=\"red\">⚠ " %
        tr("The selected directory is not empty.").toHtmlEscaped() % "</font>");
    return false;
  }

  // Check base class.
  if (!QWizardPage::isComplete()) {
    setStatusMessage(
        "<font color=\"red\">⚠ Please fill out all fields.</font>");
  }

  setStatusMessage(QString());
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

void NewProjectWizardPage_Metadata::setStatusMessage(
    const QString& msg) const noexcept {
  mUi->lblStatus->setText(msg);
  mUi->lblStatus->setVisible(!msg.isEmpty());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
