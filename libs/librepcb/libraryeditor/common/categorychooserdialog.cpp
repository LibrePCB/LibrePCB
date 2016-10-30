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
#include <QtCore>
#include <QtWidgets>
#include "categorychooserdialog.h"
#include "ui_categorychooserdialog.h"
#include <librepcb/workspace/workspace.h>
#include <librepcb/workspace/settings/workspacesettings.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

template <typename ElementType>
CategoryChooserDialog<ElementType>::CategoryChooserDialog(const workspace::Workspace& ws,
                                                          QWidget* parent) noexcept :
    QDialog(parent), mUi(new Ui::CategoryChooserDialog)
{
    mUi->setupUi(this);
    connect(mUi->treeView, &QTreeView::doubleClicked,
            this, &CategoryChooserDialog<ElementType>::accept);

    mModel.reset(new workspace::CategoryTreeModel<ElementType>(ws.getLibraryDb(),
                 ws.getSettings().getLibLocaleOrder().getLocaleOrder()));
    mUi->treeView->setModel(mModel.data());
    mUi->treeView->setRootIndex(QModelIndex());
}

template <typename ElementType>
CategoryChooserDialog<ElementType>::~CategoryChooserDialog() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

template <typename ElementType>
Uuid CategoryChooserDialog<ElementType>::getSelectedCategoryUuid() const noexcept
{
    QModelIndex index = mUi->treeView->currentIndex();
    if (index.isValid() && index.internalPointer()) {
        workspace::CategoryTreeItem<ElementType>* item = mModel->getItem(index);
        return item ? item->getUuid() : Uuid();
    } else {
        return Uuid();
    }
}

/*****************************************************************************************
 *  Explicit template instantiations
 ****************************************************************************************/
template class CategoryChooserDialog<library::ComponentCategory>;
template class CategoryChooserDialog<library::PackageCategory>;

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb
