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
#include "newelementwizardpage_choosetype.h"

#include "ui_newelementwizardpage_choosetype.h"

#include <librepcb/library/elements.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardPage_ChooseType::NewElementWizardPage_ChooseType(
    NewElementWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::NewElementWizardPage_ChooseType) {
  mUi->setupUi(this);
  setPixmap(QWizard::WatermarkPixmap, QPixmap(":/img/wizards/watermark.jpg"));
  connect(mUi->btnComponentCategory, &QToolButton::clicked, this,
          &NewElementWizardPage_ChooseType::btnComponentCategoryClicked);
  connect(mUi->btnPackageCategory, &QToolButton::clicked, this,
          &NewElementWizardPage_ChooseType::btnPackageCategoryClicked);
  connect(mUi->btnSymbol, &QToolButton::clicked, this,
          &NewElementWizardPage_ChooseType::btnSymbolClicked);
  connect(mUi->btnPackage, &QToolButton::clicked, this,
          &NewElementWizardPage_ChooseType::btnPackageClicked);
  connect(mUi->btnComponent, &QToolButton::clicked, this,
          &NewElementWizardPage_ChooseType::btnComponentClicked);
  connect(mUi->btnDevice, &QToolButton::clicked, this,
          &NewElementWizardPage_ChooseType::btnDeviceClicked);
}

NewElementWizardPage_ChooseType::~NewElementWizardPage_ChooseType() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool NewElementWizardPage_ChooseType::isComplete() const noexcept {
  return (mContext.mElementType != NewElementWizardContext::ElementType::None);
}

int NewElementWizardPage_ChooseType::nextId() const noexcept {
  if (mUi->rbtnCopyExistingElement->isChecked()) {
    return NewElementWizardContext::ID_CopyFrom;
  } else {
    return NewElementWizardContext::ID_EnterMetadata;
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardPage_ChooseType::btnComponentCategoryClicked() noexcept {
  setElementType(NewElementWizardContext::ElementType::ComponentCategory);
}

void NewElementWizardPage_ChooseType::btnPackageCategoryClicked() noexcept {
  setElementType(NewElementWizardContext::ElementType::PackageCategory);
}

void NewElementWizardPage_ChooseType::btnSymbolClicked() noexcept {
  setElementType(NewElementWizardContext::ElementType::Symbol);
}

void NewElementWizardPage_ChooseType::btnPackageClicked() noexcept {
  setElementType(NewElementWizardContext::ElementType::Package);
}

void NewElementWizardPage_ChooseType::btnComponentClicked() noexcept {
  setElementType(NewElementWizardContext::ElementType::Component);
}

void NewElementWizardPage_ChooseType::btnDeviceClicked() noexcept {
  setElementType(NewElementWizardContext::ElementType::Device);
}

void NewElementWizardPage_ChooseType::initializePage() noexcept {
  QWizardPage::initializePage();
  setElementType(NewElementWizardContext::ElementType::None);
}

void NewElementWizardPage_ChooseType::cleanupPage() noexcept {
  QWizardPage::cleanupPage();
  setElementType(NewElementWizardContext::ElementType::None);
}

void NewElementWizardPage_ChooseType::setElementType(
    NewElementWizardContext::ElementType type) noexcept {
  mContext.reset(type);
  emit completeChanged();
  if (isComplete()) {
    wizard()->next();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
