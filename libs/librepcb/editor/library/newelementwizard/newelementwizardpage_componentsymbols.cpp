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
#include "newelementwizardpage_componentsymbols.h"

#include "../../library/libraryelementcache.h"
#include "ui_newelementwizardpage_componentsymbols.h"

#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/workspace/workspace.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardPage_ComponentSymbols::NewElementWizardPage_ComponentSymbols(
    NewElementWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::NewElementWizardPage_ComponentSymbols) {
  mUi->setupUi(this);
  connect(mUi->symbolListEditorWidget,
          &ComponentSymbolVariantItemListEditorWidget::edited, this,
          &NewElementWizardPage_ComponentSymbols::completeChanged);
}

NewElementWizardPage_ComponentSymbols::
    ~NewElementWizardPage_ComponentSymbols() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool NewElementWizardPage_ComponentSymbols::validatePage() noexcept {
  return true;
}

bool NewElementWizardPage_ComponentSymbols::isComplete() const noexcept {
  return (mContext.mComponentSymbolVariants.count() > 0) &&
      (mContext.mComponentSymbolVariants.first()->getSymbolItems().count() > 0);
}

int NewElementWizardPage_ComponentSymbols::nextId() const noexcept {
  return NewElementWizardContext::ID_ComponentSignals;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardPage_ComponentSymbols::initializePage() noexcept {
  QWizardPage::initializePage();
  if (mContext.mComponentSymbolVariants.count() < 1) {
    mContext.mComponentSymbolVariants.append(
        std::make_shared<ComponentSymbolVariant>(Uuid::createRandom(), "",
                                                 ElementName("default"), ""));
  }
  mUi->symbolListEditorWidget->setReferences(
      mContext.getWorkspace(), mContext.getLayerProvider(),
      mContext.mComponentSymbolVariants.value(0)->getSymbolItems(),
      std::make_shared<LibraryElementCache>(
          mContext.getWorkspace().getLibraryDb()),
      nullptr);
}

void NewElementWizardPage_ComponentSymbols::cleanupPage() noexcept {
  QWizardPage::cleanupPage();

  // References might become invalid, thus resetting them.
  mUi->symbolListEditorWidget->resetReferences();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
