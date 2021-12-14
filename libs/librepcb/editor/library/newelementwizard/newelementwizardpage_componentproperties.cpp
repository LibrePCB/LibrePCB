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
#include "newelementwizardpage_componentproperties.h"

#include "../sym/symbolchooserdialog.h"
#include "ui_newelementwizardpage_componentproperties.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardPage_ComponentProperties::
    NewElementWizardPage_ComponentProperties(NewElementWizardContext& context,
                                             QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::NewElementWizardPage_ComponentProperties) {
  mUi->setupUi(this);
}

NewElementWizardPage_ComponentProperties::
    ~NewElementWizardPage_ComponentProperties() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool NewElementWizardPage_ComponentProperties::validatePage() noexcept {
  try {
    mContext.mComponentSchematicOnly = mUi->cbxSchematicOnly->isChecked();
    mContext.mComponentDefaultValue =
        mUi->edtDefaultValue->toPlainText().trimmed();
    mContext.mComponentPrefixes.setDefaultValue(ComponentPrefix(
        mUi->edtPrefix->text().toUpper().trimmed()));  // can throw
    return true;
  } catch (const Exception& e) {
    return false;
  }
}

bool NewElementWizardPage_ComponentProperties::isComplete() const noexcept {
  return true;
}

int NewElementWizardPage_ComponentProperties::nextId() const noexcept {
  return NewElementWizardContext::ID_ComponentSymbols;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardPage_ComponentProperties::initializePage() noexcept {
  QWizardPage::initializePage();
  mUi->cbxSchematicOnly->setChecked(mContext.mComponentSchematicOnly);
  mUi->edtDefaultValue->setPlainText(mContext.mComponentDefaultValue);
  mUi->edtPrefix->setText(*mContext.mComponentPrefixes.getDefaultValue());
}

void NewElementWizardPage_ComponentProperties::cleanupPage() noexcept {
  QWizardPage::cleanupPage();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
