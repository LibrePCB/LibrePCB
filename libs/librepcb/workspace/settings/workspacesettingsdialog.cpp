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
#include "workspacesettingsdialog.h"

#include "../workspace.h"
#include "ui_workspacesettingsdialog.h"
#include "workspacesettings.h"

#include <librepcb/common/application.h>
#include <librepcb/common/model/comboboxdelegate.h>
#include <librepcb/common/model/editablelistmodel.h>
#include <librepcb/common/norms.h>
#include <librepcb/common/toolbox.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

WorkspaceSettingsDialog::WorkspaceSettingsDialog(WorkspaceSettings& settings,
                                                 QWidget*           parent)
  : QDialog(parent),
    mSettings(settings),
    mLibLocaleOrderModel(new LibraryLocaleOrderModel()),
    mLibNormOrderModel(new LibraryNormOrderModel()),
    mRepositoryUrlsModel(new RepositoryUrlModel()),
    mUi(new Ui::WorkspaceSettingsDialog) {
  mUi->setupUi(this);

  // Initialize application locale widgets
  {
    mUi->cbxAppLocale->addItem(tr("System Language"), QString(""));
    QMap<QString, QString> map;  // map will be sorted by key
    foreach (const QString& locale, qApp->getAvailableTranslationLocales()) {
      map.insert(Toolbox::prettyPrintLocale(locale), locale);
    }
    QMap<QString, QString>::const_iterator i = map.constBegin();
    while (i != map.constEnd()) {
      mUi->cbxAppLocale->addItem(i.key(), i.value());
      ++i;
    }
  }

  // Initialize library locale order widgets
  {
    QList<QLocale> locales = QLocale::matchingLocales(
        QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    QStringList localesStr;
    foreach (const QLocale& l, locales) { localesStr.append(l.name()); }
    mLibLocaleOrderModel->setPlaceholderText(tr("Click here to add a locale"));
    mLibLocaleOrderModel->setDefaultValue(QString(""));
    mLibLocaleOrderModel->setChoices(localesStr);
    mUi->tblLibLocaleOrder->setShowMoveButtons(true);
    mUi->tblLibLocaleOrder->setModel(mLibLocaleOrderModel.data());
    mUi->tblLibLocaleOrder->setItemDelegateForColumn(
        LibraryLocaleOrderModel::COLUMN_TEXT,
        new ComboBoxDelegate(false, this));
    mUi->tblLibLocaleOrder->horizontalHeader()->setSectionResizeMode(
        LibraryLocaleOrderModel::COLUMN_TEXT, QHeaderView::Stretch);
    mUi->tblLibLocaleOrder->horizontalHeader()->setSectionResizeMode(
        LibraryLocaleOrderModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
    connect(mUi->tblLibLocaleOrder, &EditableTableWidget::btnAddClicked,
            mLibLocaleOrderModel.data(), &LibraryLocaleOrderModel::addItem);
    connect(mUi->tblLibLocaleOrder, &EditableTableWidget::btnRemoveClicked,
            mLibLocaleOrderModel.data(), &LibraryLocaleOrderModel::removeItem);
    connect(mUi->tblLibLocaleOrder, &EditableTableWidget::btnMoveUpClicked,
            mLibLocaleOrderModel.data(), &LibraryLocaleOrderModel::moveItemUp);
    connect(mUi->tblLibLocaleOrder, &EditableTableWidget::btnMoveDownClicked,
            mLibLocaleOrderModel.data(),
            &LibraryLocaleOrderModel::moveItemDown);
  }

  // Initialize library norm order widgets
  {
    mLibNormOrderModel->setPlaceholderText(tr("Click here to add a norm"));
    mLibNormOrderModel->setDefaultValue(QString(""));
    mLibNormOrderModel->setChoices(getAvailableNorms());
    mUi->tblLibNormOrder->setShowMoveButtons(true);
    mUi->tblLibNormOrder->setModel(mLibNormOrderModel.data());
    mUi->tblLibNormOrder->setItemDelegateForColumn(
        LibraryNormOrderModel::COLUMN_TEXT, new ComboBoxDelegate(true, this));
    mUi->tblLibNormOrder->horizontalHeader()->setSectionResizeMode(
        LibraryNormOrderModel::COLUMN_TEXT, QHeaderView::Stretch);
    mUi->tblLibNormOrder->horizontalHeader()->setSectionResizeMode(
        LibraryNormOrderModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
    connect(mUi->tblLibNormOrder, &EditableTableWidget::btnAddClicked,
            mLibNormOrderModel.data(), &LibraryNormOrderModel::addItem);
    connect(mUi->tblLibNormOrder, &EditableTableWidget::btnRemoveClicked,
            mLibNormOrderModel.data(), &LibraryNormOrderModel::removeItem);
    connect(mUi->tblLibNormOrder, &EditableTableWidget::btnMoveUpClicked,
            mLibNormOrderModel.data(), &LibraryNormOrderModel::moveItemUp);
    connect(mUi->tblLibNormOrder, &EditableTableWidget::btnMoveDownClicked,
            mLibNormOrderModel.data(), &LibraryNormOrderModel::moveItemDown);
  }

  // Initialize repository URL widgets
  {
    mRepositoryUrlsModel->setPlaceholderText(tr("Click here a add an URL"));
    mUi->tblRepositoryUrls->setShowMoveButtons(true);
    mUi->tblRepositoryUrls->setModel(mRepositoryUrlsModel.data());
    mUi->tblRepositoryUrls->horizontalHeader()->setSectionResizeMode(
        RepositoryUrlModel::COLUMN_TEXT, QHeaderView::Stretch);
    mUi->tblRepositoryUrls->horizontalHeader()->setSectionResizeMode(
        RepositoryUrlModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
    connect(mUi->tblRepositoryUrls, &EditableTableWidget::btnAddClicked,
            mRepositoryUrlsModel.data(), &RepositoryUrlModel::addItem);
    connect(mUi->tblRepositoryUrls, &EditableTableWidget::btnRemoveClicked,
            mRepositoryUrlsModel.data(), &RepositoryUrlModel::removeItem);
    connect(mUi->tblRepositoryUrls, &EditableTableWidget::btnMoveUpClicked,
            mRepositoryUrlsModel.data(), &RepositoryUrlModel::moveItemUp);
    connect(mUi->tblRepositoryUrls, &EditableTableWidget::btnMoveDownClicked,
            mRepositoryUrlsModel.data(), &RepositoryUrlModel::moveItemDown);
  }
  // Initialize external applications widgets
  {
    connect(mUi->pdfCustomRadioBtn, &QRadioButton::toggled,
            mUi->pdfDefaultCombo, &QComboBox::setDisabled);

    connect(mUi->pdfDefaultRadioBtn, &QRadioButton::toggled,
            mUi->pdfCustomCmdEdit, &QTextEdit::setDisabled);
  }

  // Now load all current settings
  loadSettings();

  // Load the window geometry
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("workspace_settings_dialog/window_geometry")
          .toByteArray());

  // Just in case that the wrong tab is selected in the UI designer:
  mUi->tabWidget->setCurrentIndex(0);

  // Connect event handlers
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &WorkspaceSettingsDialog::buttonBoxClicked);
}

WorkspaceSettingsDialog::~WorkspaceSettingsDialog() {
  // Save the window geometry
  QSettings clientSettings;
  clientSettings.setValue("workspace_settings_dialog/window_geometry",
                          saveGeometry());
}

/*******************************************************************************
 *  Private Slots for the GUI elements
 ******************************************************************************/

void WorkspaceSettingsDialog::buttonBoxClicked(
    QAbstractButton* button) noexcept {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::RejectRole: {
      reject();
      break;
    }

    case QDialogButtonBox::AcceptRole: {
      saveSettings();
      accept();
      break;
    }

    case QDialogButtonBox::ApplyRole: {
      saveSettings();
      break;
    }

    case QDialogButtonBox::ResetRole: {
      int answer = QMessageBox::question(
          this, tr("Restore default settings"),
          tr("Are you sure to reset all settings to their default values?\n\n"
             "Attention: This will be applied immediately and cannot be "
             "undone!"));
      if (answer == QMessageBox::Yes) {
        mSettings.restoreDefaults();
        loadSettings();  // updating all widgets with the new values
        saveSettings();  // save now since "cancel" does not revert!
      }
      break;
    }

    default:
      Q_ASSERT(false);
      break;
  }
}

void WorkspaceSettingsDialog::loadSettings() noexcept {
  // User Name
  mUi->edtUserName->setText(mSettings.userName.get());

  // Application Locale
  mUi->cbxAppLocale->setCurrentIndex(
      mUi->cbxAppLocale->findData(mSettings.applicationLocale.get()));

  // Default Length Unit
  mUi->cbxDefaultLengthUnit->clear();
  foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
    mUi->cbxDefaultLengthUnit->addItem(unit.toStringTr(), unit.getIndex());
  }
  mUi->cbxDefaultLengthUnit->setCurrentIndex(
      mSettings.defaultLengthUnit.get().getIndex());

  // Autosave Interval
  mUi->spbAutosaveInterval->setValue(
      mSettings.projectAutosaveIntervalSeconds.get());

  // Use OpenGL
  mUi->cbxUseOpenGl->setChecked(mSettings.useOpenGl.get());

  // Library Locale Order
  mLibLocaleOrderModel->setValues(mSettings.libraryLocaleOrder.get());

  // Library Norm Order
  mLibNormOrderModel->setValues(mSettings.libraryNormOrder.get());

  // Repository URLs
  mRepositoryUrlsModel->setValues(mSettings.repositoryUrls.get());
}

void WorkspaceSettingsDialog::saveSettings() noexcept {
  try {
    // User Name
    mSettings.userName.set(mUi->edtUserName->text().trimmed());

    // Application Locale
    if (mUi->cbxAppLocale->currentIndex() >= 0) {
      mSettings.applicationLocale.set(
          mUi->cbxAppLocale->currentData().toString());
    }

    // Default Length Unit
    if (mUi->cbxDefaultLengthUnit->currentIndex() >= 0) {
      mSettings.defaultLengthUnit.set(LengthUnit::fromIndex(
          mUi->cbxDefaultLengthUnit->currentIndex()));  // can throw
    }

    // Autosave Interval
    mSettings.projectAutosaveIntervalSeconds.set(
        mUi->spbAutosaveInterval->value());

    // Use OpenGL
    mSettings.useOpenGl.set(mUi->cbxUseOpenGl->isChecked());

    // Library Locale Order
    mSettings.libraryLocaleOrder.set(mLibLocaleOrderModel->getValues());

    // Library Norm Order
    mSettings.libraryNormOrder.set(mLibNormOrderModel->getValues());

    // Repository URLs
    mSettings.repositoryUrls.set(mRepositoryUrlsModel->getValues());

    mSettings.saveToFile();  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb
