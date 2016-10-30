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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGECHOOSERDIALOG_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGECHOOSERDIALOG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/uuid.h>
#include <librepcb/common/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsScene;
class IF_GraphicsLayerProvider;

namespace workspace {
class Workspace;
}

namespace library {

class Package;
class FootprintPreviewGraphicsItem;

namespace editor {

namespace Ui {
class PackageChooserDialog;
}

/*****************************************************************************************
 *  Class PackageChooserDialog
 ****************************************************************************************/

/**
 * @brief The PackageChooserDialog class
 *
 * @author ubruhin
 * @date 2017-03-25
 */
class PackageChooserDialog final : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        PackageChooserDialog() = delete;
        PackageChooserDialog(const PackageChooserDialog& other) = delete;
        PackageChooserDialog(const workspace::Workspace& ws,
                             const IF_GraphicsLayerProvider* layerProvider,
                             QWidget* parent = nullptr) noexcept;
        ~PackageChooserDialog() noexcept;

        // Getters
        const Uuid& getSelectedPackageUuid() const noexcept {return mSelectedPackageUuid;}

        // Operator Overloadings
        PackageChooserDialog& operator=(const PackageChooserDialog& rhs) = delete;


    private: // Methods
        void treeCategories_currentItemChanged(const QModelIndex& current,
                                               const QModelIndex& previous) noexcept;
        void listPackages_currentItemChanged(QListWidgetItem* current,
                                             QListWidgetItem* previous) noexcept;
        void listPackages_itemDoubleClicked(QListWidgetItem* item) noexcept;
        void setSelectedCategory(const Uuid& uuid) noexcept;
        void setSelectedPackage(const Uuid& uuid) noexcept;
        void updatePreview() noexcept;
        void accept() noexcept override;
        const QStringList& localeOrder() const noexcept;


    private: // Data
        const workspace::Workspace& mWorkspace;
        const IF_GraphicsLayerProvider* mLayerProvider;
        QScopedPointer<Ui::PackageChooserDialog> mUi;
        QScopedPointer<QAbstractItemModel> mCategoryTreeModel;
        Uuid mSelectedCategoryUuid;
        Uuid mSelectedPackageUuid;

        // preview
        FilePath mPackageFilePath;
        QScopedPointer<Package> mPackage;
        QScopedPointer<GraphicsScene> mGraphicsScene;
        QScopedPointer<FootprintPreviewGraphicsItem> mGraphicsItem;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_PACKAGECHOOSERDIALOG_H
