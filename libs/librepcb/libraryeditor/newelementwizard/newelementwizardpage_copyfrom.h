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

#ifndef LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COPYFROM_H
#define LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COPYFROM_H

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

class LibraryBaseElement;

namespace editor {

namespace Ui {
class NewElementWizardPage_CopyFrom;
}

/*****************************************************************************************
 *  Class NewElementWizardPage_CopyFrom
 ****************************************************************************************/

/**
 * @brief The NewElementWizardPage_CopyFrom class
 *
 * @author ubruhin
 * @date 2017-03-23
 *
 * @todo All names/descriptions/keywords other than en_US are not yet copied.
 * @todo All categories other than the first one are not yet copied.
 */
class NewElementWizardPage_CopyFrom final : public QWizardPage
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        NewElementWizardPage_CopyFrom() = delete;
        NewElementWizardPage_CopyFrom(const NewElementWizardPage_CopyFrom& other) = delete;
        explicit NewElementWizardPage_CopyFrom(NewElementWizardContext& context,
                                               QWidget* parent = 0) noexcept;
        ~NewElementWizardPage_CopyFrom() noexcept;


        // Getters
        bool validatePage() noexcept override;
        bool isComplete() const noexcept override;
        int nextId() const noexcept override;


        // Operator Overloadings
        NewElementWizardPage_CopyFrom& operator=(const NewElementWizardPage_CopyFrom& rhs) = delete;


    private: // Methods
        void treeView_currentItemChanged(const QModelIndex& current,
                                         const QModelIndex& previous) noexcept;
        void treeView_doubleClicked(const QModelIndex& item) noexcept;
        void listWidget_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous) noexcept;
        void listWidget_itemDoubleClicked(QListWidgetItem* item) noexcept;
        void setSelectedCategory(const Uuid& uuid) noexcept;
        void setSelectedElement(const FilePath& fp) noexcept;
        void setCategoryTreeModel(QAbstractItemModel* model) noexcept;
        FilePath getCategoryFilePath(const Uuid& category) const;
        QSet<Uuid> getElementsByCategory(const Uuid& category) const;
        void getElementMetadata(const Uuid& uuid, FilePath& fp, QString& name) const;
        void initializePage() noexcept override;
        void cleanupPage() noexcept override;


    private: // Data
        NewElementWizardContext& mContext;
        QScopedPointer<Ui::NewElementWizardPage_CopyFrom> mUi;
        QScopedPointer<QAbstractItemModel> mCategoryTreeModel;
        bool mIsCategoryElement;
        Uuid mSelectedCategoryUuid;
        QScopedPointer<LibraryBaseElement> mSelectedElement;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_NEWELEMENTWIZARDPAGE_COPYFROM_H
