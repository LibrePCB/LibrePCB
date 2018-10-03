/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#include "../common/categorychooserdialog.h"
#include "../common/categorytreelabeltextbuilder.h"
#include "ui_newelementwizardpage_entermetadata.h"

#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
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
    case NewElementWizardContext::ElementType::ComponentCategory:
    case NewElementWizardContext::ElementType::PackageCategory:
    case NewElementWizardContext::ElementType::Symbol:
      return NewElementWizardContext::ID_None;
    case NewElementWizardContext::ElementType::Package:
      return NewElementWizardContext::ID_PackagePads;
    case NewElementWizardContext::ElementType::Component:
      return NewElementWizardContext::ID_ComponentProperties;
    case NewElementWizardContext::ElementType::Device:
      return NewElementWizardContext::ID_DeviceProperties;
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
  tl::optional<Uuid> categoryUuid;
  switch (mContext.mElementType) {
    case NewElementWizardContext::ElementType::ComponentCategory:
    case NewElementWizardContext::ElementType::Symbol:
    case NewElementWizardContext::ElementType::Component:
    case NewElementWizardContext::ElementType::Device: {
      ComponentCategoryChooserDialog dialog(mContext.getWorkspace(), this);
      if (dialog.exec() != QDialog::Accepted) return;
      categoryUuid = dialog.getSelectedCategoryUuid();
      break;
    }
    case NewElementWizardContext::ElementType::PackageCategory:
    case NewElementWizardContext::ElementType::Package: {
      PackageCategoryChooserDialog dialog(mContext.getWorkspace(), this);
      if (dialog.exec() != QDialog::Accepted) return;
      categoryUuid = dialog.getSelectedCategoryUuid();
      break;
    }
    default: {
      qCritical() << "Unknown enum value:"
                  << static_cast<int>(mContext.mElementType);
      return;
    }
  }
  mContext.mElementCategoryUuid = categoryUuid;
  updateCategoryTreeLabel();
}

void NewElementWizardPage_EnterMetadata::btnResetCategoryClicked() noexcept {
  mContext.mElementCategoryUuid = tl::nullopt;
  updateCategoryTreeLabel();
}

void NewElementWizardPage_EnterMetadata::updateCategoryTreeLabel() noexcept {
  switch (mContext.mElementType) {
    case NewElementWizardContext::ElementType::ComponentCategory:
    case NewElementWizardContext::ElementType::Symbol:
    case NewElementWizardContext::ElementType::Component:
    case NewElementWizardContext::ElementType::Device: {
      ComponentCategoryTreeLabelTextBuilder builder(
          mContext.getWorkspace().getLibraryDb(),
          mContext.getWorkspace()
              .getSettings()
              .getLibLocaleOrder()
              .getLocaleOrder(),
          *mUi->lblCategoryTree);
      builder.setHighlightLastLine(true);
      builder.setOneLine(true);
      builder.updateText(mContext.mElementCategoryUuid);
      break;
    }
    case NewElementWizardContext::ElementType::PackageCategory:
    case NewElementWizardContext::ElementType::Package: {
      PackageCategoryTreeLabelTextBuilder builder(
          mContext.getWorkspace().getLibraryDb(),
          mContext.getWorkspace()
              .getSettings()
              .getLibLocaleOrder()
              .getLocaleOrder(),
          *mUi->lblCategoryTree);
      builder.setHighlightLastLine(true);
      builder.setOneLine(true);
      builder.updateText(mContext.mElementCategoryUuid);
      break;
    }
    default: {
      mUi->lblCategoryTree->setText(tr("Root category"));
      break;
    }
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
}  // namespace library
}  // namespace librepcb
