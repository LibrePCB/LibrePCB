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
#include "newelementwizardpage_packagepads.h"

#include "ui_newelementwizardpage_packagepads.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardPage_PackagePads::NewElementWizardPage_PackagePads(
    NewElementWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::NewElementWizardPage_PackagePads) {
  mUi->setupUi(this);
}

NewElementWizardPage_PackagePads::~NewElementWizardPage_PackagePads() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool NewElementWizardPage_PackagePads::validatePage() noexcept {
  return true;
}

bool NewElementWizardPage_PackagePads::isComplete() const noexcept {
  return true;
}

int NewElementWizardPage_PackagePads::nextId() const noexcept {
  return NewElementWizardContext::ID_None;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardPage_PackagePads::initializePage() noexcept {
  QWizardPage::initializePage();
  mUi->padListEditorWidget->setReferences(mContext.mPackagePads, nullptr);
}

void NewElementWizardPage_PackagePads::cleanupPage() noexcept {
  QWizardPage::cleanupPage();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
