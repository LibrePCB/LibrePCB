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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "newelementwizardpage_componentsymbols.h"
#include "ui_newelementwizardpage_componentsymbols.h"
#include <librepcb/library/cmp/component.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NewElementWizardPage_ComponentSymbols::NewElementWizardPage_ComponentSymbols(NewElementWizardContext& context, QWidget *parent) noexcept :
    QWizardPage(parent), mContext(context), mUi(new Ui::NewElementWizardPage_ComponentSymbols)
{
    mUi->setupUi(this);
    connect(mUi->symbolListEditorWidget, &ComponentSymbolVariantItemListEditorWidget::edited,
            this, &NewElementWizardPage_ComponentSymbols::completeChanged);
}

NewElementWizardPage_ComponentSymbols::~NewElementWizardPage_ComponentSymbols() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool NewElementWizardPage_ComponentSymbols::validatePage() noexcept
{
    mContext.mComponentSymbolVariants = mSymbolVariantList;
    return true;
}

bool NewElementWizardPage_ComponentSymbols::isComplete() const noexcept
{
    return mSymbolVariantList.value(0)->getSymbolItems().count() > 0;
}

int NewElementWizardPage_ComponentSymbols::nextId() const noexcept
{
    return NewElementWizardContext::ID_ComponentSignals;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void NewElementWizardPage_ComponentSymbols::initializePage() noexcept
{
    QWizardPage::initializePage();
    mSymbolVariantList = mContext.mComponentSymbolVariants;
    if (mSymbolVariantList.count() < 1) {
        mSymbolVariantList.append(
            std::make_shared<ComponentSymbolVariant>(Uuid::createRandom(), "",
                                                     ElementName("default"), ""));
    }
    mUi->symbolListEditorWidget->setVariant(mContext.getWorkspace(),
        mContext.getLayerProvider(), mSymbolVariantList.value(0)->getSymbolItems());
}

void NewElementWizardPage_ComponentSymbols::cleanupPage() noexcept
{
    QWizardPage::cleanupPage();
    mContext.mComponentSymbolVariants = mSymbolVariantList;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
