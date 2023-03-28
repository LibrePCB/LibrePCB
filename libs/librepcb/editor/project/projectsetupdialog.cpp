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
#include "projectsetupdialog.h"

#include "../editorcommandset.h"
#include "../undostack.h"
#include "cmd/cmdnetclassadd.h"
#include "cmd/cmdnetclassedit.h"
#include "cmd/cmdnetclassremove.h"
#include "cmd/cmdprojectedit.h"
#include "ui_projectsetupdialog.h"

#include <librepcb/core/application.h>
#include <librepcb/core/norms.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/netclass.h>
#include <librepcb/core/project/project.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectSetupDialog::ProjectSetupDialog(Project& project, UndoStack& undoStack,
                                       const QString& settingsPrefix,
                                       QWidget* parent) noexcept
  : QDialog(parent),
    mProject(project),
    mUndoStack(undoStack),
    mSettingsPrefix(settingsPrefix % "/project_setup_dialog"),
    mAttributes(mProject.getAttributes()),
    mUi(new Ui::ProjectSetupDialog) {
  mUi->setupUi(this);
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &ProjectSetupDialog::buttonBoxClicked);

  const EditorCommandSet& cmd = EditorCommandSet::instance();

  // Tab: Attributes
  mUi->edtProjectAttributes->setFrameStyle(QFrame::NoFrame);
  mUi->edtProjectAttributes->setReferences(nullptr, &mAttributes);

  // Tab: Locales & Norms
  QMap<QString, QString> locales;  // Map will be sorted by key.
  foreach (const QString& locale, Application::getTranslationLocales()) {
    locales.insert(Toolbox::prettyPrintLocale(locale), locale);
  }
  for (auto it = locales.begin(); it != locales.end(); it++) {
    mUi->cbxLocales->addItem(it.key(), it.value());
  }
  mUi->cbxLocales->setCurrentIndex(-1);
  connect(mUi->btnLocaleAdd, &QToolButton::clicked, this, [this]() {
    const QString locale = mUi->cbxLocales->currentData().toString();
    if (!locale.isEmpty()) {
      QListWidgetItem* item = new QListWidgetItem(
          Toolbox::prettyPrintLocale(locale), mUi->lstLocaleOrder);
      item->setData(Qt::UserRole, locale);
    }
  });
  connect(mUi->btnLocaleRemove, &QToolButton::clicked, this, [this]() {
    foreach (auto item, mUi->lstLocaleOrder->selectedItems()) { delete item; }
  });
  connect(mUi->btnLocaleUp, &QToolButton::clicked, this, [this]() {
    int row = mUi->lstLocaleOrder->currentRow();
    if (row > 0) {
      mUi->lstLocaleOrder->insertItem(row - 1,
                                      mUi->lstLocaleOrder->takeItem(row));
      mUi->lstLocaleOrder->setCurrentRow(row - 1);
    }
  });
  connect(mUi->btnLocaleDown, &QToolButton::clicked, this, [this]() {
    int row = mUi->lstLocaleOrder->currentRow();
    if ((row >= 0) && (row < mUi->lstLocaleOrder->count() - 1)) {
      mUi->lstLocaleOrder->insertItem(row + 1,
                                      mUi->lstLocaleOrder->takeItem(row));
      mUi->lstLocaleOrder->setCurrentRow(row + 1);
    }
  });
  mUi->cbxNorms->addItems(getAvailableNorms());
  mUi->cbxNorms->clearEditText();
  connect(mUi->btnNormAdd, &QToolButton::clicked, this, [this]() {
    const QString norm = mUi->cbxNorms->currentText().trimmed();
    if (!norm.isEmpty()) {
      mUi->lstNormOrder->addItem(norm);
    }
  });
  connect(mUi->btnNormRemove, &QToolButton::clicked, this, [this]() {
    foreach (auto item, mUi->lstNormOrder->selectedItems()) { delete item; }
  });
  connect(mUi->btnNormUp, &QToolButton::clicked, this, [this]() {
    int row = mUi->lstNormOrder->currentRow();
    if (row > 0) {
      mUi->lstNormOrder->insertItem(row - 1, mUi->lstNormOrder->takeItem(row));
      mUi->lstNormOrder->setCurrentRow(row - 1);
    }
  });
  connect(mUi->btnNormDown, &QToolButton::clicked, this, [this]() {
    int row = mUi->lstNormOrder->currentRow();
    if ((row >= 0) && (row < mUi->lstNormOrder->count() - 1)) {
      mUi->lstNormOrder->insertItem(row + 1, mUi->lstNormOrder->takeItem(row));
      mUi->lstNormOrder->setCurrentRow(row + 1);
    }
  });

  // Tab: Net Classes
  mUi->lstNetClasses->addAction(cmd.remove.createAction(
      mUi->lstNetClasses, this,
      [this]() {
        foreach (auto item, mUi->lstNetClasses->selectedItems()) {
          if (item->checkState() != Qt::Checked) {
            delete item;
          }
        }
      },
      EditorCommand::ActionFlag::QueuedConnection |
          EditorCommand::ActionFlag::WidgetShortcut));
  mUi->edtNetClassName->addAction(
      cmd.inputAcceptAdd.createAction(
          mUi->edtNetClassName, this,
          [this]() {
            const QString name = mUi->edtNetClassName->text().trimmed();
            if (!name.isEmpty()) {
              QListWidgetItem* item =
                  new QListWidgetItem(name, mUi->lstNetClasses);
              item->setCheckState(Qt::Unchecked);
              auto flags = item->flags();
              flags |= Qt::ItemIsEditable;
              flags &= ~Qt::ItemIsUserCheckable;
              item->setFlags(flags);
            }
          },
          EditorCommand::ActionFlag::WidgetShortcut),
      QLineEdit::TrailingPosition);

  // Load all properties.
  load();

  // Load the window geometry.
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value(mSettingsPrefix % "/window_geometry").toByteArray());

  // Always open first tab.
  mUi->tabWidget->setCurrentIndex(0);

  // Set focus to name so the user can immediately start typing to change it-
  mUi->edtProjectName->setFocus();
}

ProjectSetupDialog::~ProjectSetupDialog() {
  // Save the window geometry.
  QSettings clientSettings;
  clientSettings.setValue(mSettingsPrefix % "/window_geometry", saveGeometry());

  mUi->edtProjectAttributes->setReferences(nullptr, nullptr);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectSetupDialog::buttonBoxClicked(QAbstractButton* button) {
  Q_ASSERT(button);
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::ApplyRole:
      apply();
      break;
    case QDialogButtonBox::AcceptRole:
      if (apply()) {
        accept();
      }
      break;
    case QDialogButtonBox::RejectRole:
      reject();
      break;
    default:
      break;
  }
}

void ProjectSetupDialog::load() noexcept {
  // Tab: Metadata
  mUi->edtProjectName->setText(*mProject.getName());
  mUi->edtProjectAuthor->setText(mProject.getAuthor());
  mUi->edtProjectVersion->setText(mProject.getVersion());
  mUi->lblProjectCreated->setText(
      mProject.getCreated().toString(Qt::DefaultLocaleLongDate));
  mUi->lblProjectLastModified->setText(
      mProject.getLastModified().toString(Qt::DefaultLocaleLongDate));

  // Tab: Locales & Norms
  mUi->lstLocaleOrder->clear();
  foreach (const QString& locale, mProject.getLocaleOrder()) {
    QListWidgetItem* item = new QListWidgetItem(
        Toolbox::prettyPrintLocale(locale), mUi->lstLocaleOrder);
    item->setData(Qt::UserRole, locale);
  }
  mUi->lstNormOrder->clear();
  mUi->lstNormOrder->addItems(mProject.getNormOrder());

  // Tab: Net Classes
  mUi->lstNetClasses->clear();
  foreach (NetClass* netClass, mProject.getCircuit().getNetClasses()) {
    QListWidgetItem* item =
        new QListWidgetItem(*netClass->getName(), mUi->lstNetClasses);
    item->setData(Qt::UserRole, netClass->getUuid().toStr());
    item->setCheckState(netClass->isUsed() ? Qt::Checked : Qt::Unchecked);
    auto flags = item->flags();
    flags |= Qt::ItemIsEditable;
    flags &= ~Qt::ItemIsUserCheckable;
    item->setFlags(flags);
  }
}

bool ProjectSetupDialog::apply() noexcept {
  try {
    UndoStackTransaction transaction(mUndoStack, tr("Modify Project Setup"));

    QScopedPointer<CmdProjectEdit> cmd(new CmdProjectEdit(mProject));

    // Tab: Metadata
    cmd->setName(
        ElementName(mUi->edtProjectName->text().trimmed()));  // can throw
    cmd->setAuthor(mUi->edtProjectAuthor->text().trimmed());
    cmd->setVersion(mUi->edtProjectVersion->text().trimmed());

    // Tab: Attributes
    cmd->setAttributes(mAttributes);

    // Tab: Locales & Norms
    {
      QStringList locales;
      for (int i = 0; i < mUi->lstLocaleOrder->count(); i++) {
        locales.append(
            mUi->lstLocaleOrder->item(i)->data(Qt::UserRole).toString());
      }
      cmd->setLocaleOrder(locales);
    }
    {
      QStringList norms;
      for (int i = 0; i < mUi->lstNormOrder->count(); i++) {
        norms.append(mUi->lstNormOrder->item(i)->text());
      }
      cmd->setNormOrder(norms);
    }

    transaction.append(cmd.take());  // can throw

    // Tab: Net Classes
    {
      // Collect net classes.
      QHash<QListWidgetItem*, NetClass*> items;
      for (int i = 0; i < mUi->lstNetClasses->count(); ++i) {
        if (QListWidgetItem* item = mUi->lstNetClasses->item(i)) {
          const QString uuid = item->data(Qt::UserRole).toString();
          NetClass* netClass = (!uuid.isEmpty())
              ? mProject.getCircuit().getNetClasses().value(
                    Uuid::fromString(uuid))
              : nullptr;
          items.insert(item, netClass);
        }
      }

      // Remove no longer existing net classes.
      foreach (NetClass* netClass,
               mProject.getCircuit().getNetClasses().values().toSet() -
                   items.values().toSet()) {
        transaction.append(new CmdNetClassRemove(*netClass));
      }

      // Add new net classes.
      for (auto it = items.begin(); it != items.end(); it++) {
        if (!it.value()) {
          const ElementName name(it.key()->text().trimmed());  // can throw
          transaction.append(new CmdNetClassAdd(mProject.getCircuit(), name));
          if (NetClass* obj = mProject.getCircuit().getNetClassByName(name)) {
            it.key()->setData(Qt::UserRole, obj->getUuid().toStr());
          }
        }
      }

      // Apply renames.
      for (auto it = items.begin(); it != items.end(); it++) {
        const QString name = it.key()->text().trimmed();
        if (it.value() && (name != *it.value()->getName())) {
          QScopedPointer<CmdNetClassEdit> cmd(new CmdNetClassEdit(*it.value()));
          cmd->setName(ElementName(name));  // can throw
          transaction.append(cmd.take());
        }
      }
    }

    transaction.commit();  // can throw
    return true;
  } catch (const Exception& e) {
    QMessageBox::warning(this, tr("Could not apply settings"), e.getMsg());
    return false;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
