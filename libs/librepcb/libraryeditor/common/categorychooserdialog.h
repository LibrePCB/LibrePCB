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

#ifndef LIBREPCB_LIBRARY_EDITOR_CATEGORYCHOOSERDIALOG_H
#define LIBREPCB_LIBRARY_EDITOR_CATEGORYCHOOSERDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/uuid.h>
#include <librepcb/workspace/library/cat/categorytreemodel.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace workspace {
class Workspace;
}

namespace library {
namespace editor {

namespace Ui {
class CategoryChooserDialog;
}

/*****************************************************************************************
 *  Class CategoryChooserDialog
 ****************************************************************************************/

/**
 * @brief The CategoryChooserDialog class
 *
 * @author  ubruhin
 * @date    2016-10-25
 */
template <typename ElementType>
class CategoryChooserDialog final : public QDialog
{
    public:

        // Constructors / Destructor
        CategoryChooserDialog() = delete;
        CategoryChooserDialog(const CategoryChooserDialog& other) = delete;
        explicit CategoryChooserDialog(const workspace::Workspace& ws, QWidget* parent = 0) noexcept;
        ~CategoryChooserDialog() noexcept;

        // Getters
        Uuid getSelectedCategoryUuid() const noexcept;

        // Operator Overloadings
        CategoryChooserDialog& operator=(const CategoryChooserDialog& rhs) = delete;

    private: // Data
        QScopedPointer<Ui::CategoryChooserDialog> mUi;
        QScopedPointer<workspace::CategoryTreeModel<ElementType>> mModel;
};

typedef CategoryChooserDialog<library::ComponentCategory> ComponentCategoryChooserDialog;
typedef CategoryChooserDialog<library::PackageCategory> PackageCategoryChooserDialog;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_CATEGORYCHOOSERDIALOG_H
