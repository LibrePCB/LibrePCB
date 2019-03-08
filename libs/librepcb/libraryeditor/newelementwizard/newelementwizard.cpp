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
#include "newelementwizard.h"

#include "newelementwizardpage_choosetype.h"
#include "newelementwizardpage_componentpinsignalmap.h"
#include "newelementwizardpage_componentproperties.h"
#include "newelementwizardpage_componentsignals.h"
#include "newelementwizardpage_componentsymbols.h"
#include "newelementwizardpage_copyfrom.h"
#include "newelementwizardpage_deviceproperties.h"
#include "newelementwizardpage_entermetadata.h"
#include "newelementwizardpage_packagepads.h"
#include "ui_newelementwizard.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizard::NewElementWizard(const workspace::Workspace& ws, Library& lib,
                                   const IF_GraphicsLayerProvider& lp,
                                   QWidget* parent) noexcept
  : QWizard(parent), mUi(new Ui::NewElementWizard) {
  mUi->setupUi(this);
  setPixmap(WizardPixmap::LogoPixmap, QPixmap(":/img/logo/48x48.png"));
  mContext.reset(new NewElementWizardContext(ws, lib, lp));

  // add pages
  insertPage(NewElementWizardContext::ID_ChooseType,
             new NewElementWizardPage_ChooseType(*mContext, this));
  insertPage(NewElementWizardContext::ID_CopyFrom,
             new NewElementWizardPage_CopyFrom(*mContext, this));
  insertPage(NewElementWizardContext::ID_EnterMetadata,
             new NewElementWizardPage_EnterMetadata(*mContext, this));
  insertPage(NewElementWizardContext::ID_PackagePads,
             new NewElementWizardPage_PackagePads(*mContext, this));
  insertPage(NewElementWizardContext::ID_ComponentProperties,
             new NewElementWizardPage_ComponentProperties(*mContext, this));
  insertPage(NewElementWizardContext::ID_ComponentSymbols,
             new NewElementWizardPage_ComponentSymbols(*mContext, this));
  insertPage(NewElementWizardContext::ID_ComponentSignals,
             new NewElementWizardPage_ComponentSignals(*mContext, this));
  insertPage(NewElementWizardContext::ID_ComponentPinSignalMap,
             new NewElementWizardPage_ComponentPinSignalMap(*mContext, this));
  insertPage(NewElementWizardContext::ID_DeviceProperties,
             new NewElementWizardPage_DeviceProperties(*mContext, this));

  setStartId(NewElementWizardContext::ID_ChooseType);
}

NewElementWizard::~NewElementWizard() noexcept {
  // ensure that mContext lives longer than all pages!
  qDeleteAll(mPages);
  mPages.clear();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void NewElementWizard::setElementToCopy(
    NewElementWizardContext::ElementType type, const FilePath& fp) noexcept {
  try {
    mContext->reset(type);
    mContext->copyElement(type, fp);  // can throw
    setStartId(NewElementWizardContext::ID_EnterMetadata);
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Could not copy element"), e.getMsg());
    setStartId(NewElementWizardContext::ID_ChooseType);
  }
}

bool NewElementWizard::validateCurrentPage() noexcept {
  if (currentPage() && (!currentPage()->validatePage())) {
    return false;
  }
  if (nextId() == NewElementWizardContext::ID_None) {
    // last page --> create the library element!
    try {
      mContext->createLibraryElement();  // can throw
    } catch (const Exception& e) {
      QMessageBox::critical(this, tr("Failed to create element"), e.getMsg());
      return false;
    }
  }
  return true;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizard::insertPage(int index, QWizardPage* page) noexcept {
  setPage(index, page);
  mPages.append(page);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
