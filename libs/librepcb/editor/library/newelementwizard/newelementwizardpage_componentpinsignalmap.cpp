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
#include "newelementwizardpage_componentpinsignalmap.h"

#include "ui_newelementwizardpage_componentpinsignalmap.h"

#include <librepcb/library/cmp/component.h>
#include <librepcb/libraryeditor/libraryelementcache.h>
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

NewElementWizardPage_ComponentPinSignalMap::
    NewElementWizardPage_ComponentPinSignalMap(NewElementWizardContext& context,
                                               QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::NewElementWizardPage_ComponentPinSignalMap) {
  mUi->setupUi(this);
}

NewElementWizardPage_ComponentPinSignalMap::
    ~NewElementWizardPage_ComponentPinSignalMap() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool NewElementWizardPage_ComponentPinSignalMap::validatePage() noexcept {
  return true;
}

bool NewElementWizardPage_ComponentPinSignalMap::isComplete() const noexcept {
  return true;
}

int NewElementWizardPage_ComponentPinSignalMap::nextId() const noexcept {
  return NewElementWizardContext::ID_None;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardPage_ComponentPinSignalMap::initializePage() noexcept {
  QWizardPage::initializePage();
  mUi->pinSignalMapEditorWidget->setReferences(
      mContext.mComponentSymbolVariants.value(0).get(),
      std::make_shared<LibraryElementCache>(
          mContext.getWorkspace().getLibraryDb()),
      &mContext.mComponentSignals, nullptr);
}

void NewElementWizardPage_ComponentPinSignalMap::cleanupPage() noexcept {
  QWizardPage::cleanupPage();

  // References might become invalid, thus resetting them.
  mUi->pinSignalMapEditorWidget->resetReferences();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
