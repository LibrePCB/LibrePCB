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

#ifndef LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_CHOOSETYPE_H
#define LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_CHOOSETYPE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "newelementwizardcontext.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

namespace Ui {
class NewElementWizardPage_ChooseType;
}

/*****************************************************************************************
 *  Class NewElementWizardPage_ChooseType
 ****************************************************************************************/

/**
 * @brief The NewElementWizardPage_ChooseType class
 *
 * @author ubruhin
 * @date 2017-03-21
 */
class NewElementWizardPage_ChooseType final : public QWizardPage
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        NewElementWizardPage_ChooseType() = delete;
        NewElementWizardPage_ChooseType(const NewElementWizardPage_ChooseType& other) = delete;
        explicit NewElementWizardPage_ChooseType(NewElementWizardContext& context,
                                                 QWidget* parent = 0) noexcept;
        ~NewElementWizardPage_ChooseType() noexcept;


        // Getters
        bool isComplete() const noexcept override;
        int nextId() const noexcept override;


        // Operator Overloadings
        NewElementWizardPage_ChooseType& operator=(const NewElementWizardPage_ChooseType& rhs) = delete;


    private: // Methods
        void btnComponentCategoryClicked() noexcept;
        void btnPackageCategoryClicked() noexcept;
        void btnSymbolClicked() noexcept;
        void btnPackageClicked() noexcept;
        void btnComponentClicked() noexcept;
        void btnDeviceClicked() noexcept;
        void initializePage() noexcept override;
        void cleanupPage() noexcept override;
        void setElementType(NewElementWizardContext::ElementType type) noexcept;


    private: // Data
        NewElementWizardContext& mContext;
        QScopedPointer<Ui::NewElementWizardPage_ChooseType> mUi;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_CHOOSETYPE_H
