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
#include "newelementwizardpage_entermetadata.h"

#include "../cat/categorychooserdialog.h"
#include "../cat/categorytreelabeltextbuilder.h"
#include "ui_newelementwizardpage_entermetadata.h"

#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardPage_EnterMetadata::NewElementWizardPage_EnterMetadata(
    NewElementWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::NewElementWizardPage_EnterMetadata) {
  mUi->setupUi(this);
  connect(mUi->edtName, &QLineEdit::textChanged, this,
          &NewElementWizardPage_EnterMetadata::edtNameTextChanged);
  connect(mUi->edtDescription, &QPlainTextEdit::textChanged, this,
          &NewElementWizardPage_EnterMetadata::edtDescriptionTextChanged);
  connect(mUi->edtKeywords, &QLineEdit::textChanged, this,
          &NewElementWizardPage_EnterMetadata::edtKeywordsTextChanged);
  connect(mUi->edtAuthor, &QLineEdit::textChanged, this,
          &NewElementWizardPage_EnterMetadata::edtAuthorTextChanged);
  connect(mUi->edtVersion, &QLineEdit::textChanged, this,
          &NewElementWizardPage_EnterMetadata::edtVersionTextChanged);
  connect(mUi->btnChooseCategory, &QToolButton::clicked, this,
          &NewElementWizardPage_EnterMetadata::btnChooseCategoryClicked);
  connect(mUi->btnResetCategory, &QToolButton::clicked, this,
          &NewElementWizardPage_EnterMetadata::btnResetCategoryClicked);
}

NewElementWizardPage_EnterMetadata::
    ~NewElementWizardPage_EnterMetadata() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool NewElementWizardPage_EnterMetadata::isComplete() const noexcept {
  if (!mContext.mElementName) return false;
  if (!mContext.mElementVersion) return false;
  return true;
}

int NewElementWizardPage_EnterMetadata::nextId() const noexcept {
  switch (mContext.mElementType) {
    default:
      return NewElementWizardContext::ID_None;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardPage_EnterMetadata::edtNameTextChanged(
    const QString& text) noexcept {
  try {
    mContext.mElementName = ElementName(text.trimmed());  // can throw
    emit completeChanged();
  } catch (const Exception& e) {
    // invalid name
  }
}

void NewElementWizardPage_EnterMetadata::edtDescriptionTextChanged() noexcept {
  mContext.mElementDescription = mUi->edtDescription->toPlainText().trimmed();
}

void NewElementWizardPage_EnterMetadata::edtKeywordsTextChanged(
    const QString& text) noexcept {
  mContext.mElementKeywords = text.trimmed();
}

void NewElementWizardPage_EnterMetadata::edtAuthorTextChanged(
    const QString& text) noexcept {
  mContext.mElementAuthor = text.trimmed();
}

void NewElementWizardPage_EnterMetadata::edtVersionTextChanged(
    const QString& text) noexcept {
  mContext.mElementVersion = Version::tryFromString(text.trimmed());
  emit completeChanged();
}

void NewElementWizardPage_EnterMetadata::btnChooseCategoryClicked() noexcept {
  std::optional<Uuid> categoryUuid;
  switch (mContext.mElementType) {
    default: {
      qCritical()
          << "Unhandled switch-case in "
             "NewElementWizardPage_EnterMetadata::btnChooseCategoryClicked():"
          << static_cast<int>(mContext.mElementType);
      return;
    }
  }
  mContext.mElementCategoryUuids.clear();
  if (categoryUuid.has_value()) {
    mContext.mElementCategoryUuids.insert(*categoryUuid);
  }
  updateCategoryTreeLabel();
}

void NewElementWizardPage_EnterMetadata::btnResetCategoryClicked() noexcept {
  mContext.mElementCategoryUuids.clear();
  updateCategoryTreeLabel();
}

void NewElementWizardPage_EnterMetadata::updateCategoryTreeLabel() noexcept {
  std::optional<Uuid> rootCategoryUuid = std::nullopt;
  if (mContext.mElementCategoryUuids.count()) {
    rootCategoryUuid = mContext.mElementCategoryUuids.values().first();
  }

  switch (mContext.mElementType) {
    default: {
      qCritical()
          << "NewElementWizardPage_EnterMetadata: Unhandled switch-case value:"
          << static_cast<int>(mContext.mElementType);
      break;
    }
  }
  if (mContext.mElementCategoryUuids.count() > 1) {
    mUi->lblMoreCategories->setText(
        tr("... and %1 more.").arg(mContext.mElementCategoryUuids.count() - 1));
  } else {
    mUi->lblMoreCategories->setText(QString());
  }
}

void NewElementWizardPage_EnterMetadata::initializePage() noexcept {
  QWizardPage::initializePage();
  mUi->edtName->setText(mContext.mElementName ? **mContext.mElementName
                                              : QString());
  mUi->edtDescription->setPlainText(mContext.mElementDescription);
  mUi->edtKeywords->setText(mContext.mElementKeywords);
  mUi->edtAuthor->setText(mContext.mElementAuthor);
  mUi->edtVersion->setText(
      mContext.mElementVersion ? mContext.mElementVersion->toStr() : QString());
  updateCategoryTreeLabel();
}

void NewElementWizardPage_EnterMetadata::cleanupPage() noexcept {
  QWizardPage::cleanupPage();
  updateCategoryTreeLabel();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
