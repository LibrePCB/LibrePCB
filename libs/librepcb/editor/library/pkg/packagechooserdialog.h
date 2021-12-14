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

#ifndef LIBREPCB_EDITOR_PACKAGECHOOSERDIALOG_H
#define LIBREPCB_EDITOR_PACKAGECHOOSERDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsScene;
class IF_GraphicsLayerProvider;
class Package;
class Workspace;

namespace editor {

class FootprintPreviewGraphicsItem;

namespace Ui {
class PackageChooserDialog;
}

/*******************************************************************************
 *  Class PackageChooserDialog
 ******************************************************************************/

/**
 * @brief The PackageChooserDialog class
 */
class PackageChooserDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageChooserDialog() = delete;
  PackageChooserDialog(const PackageChooserDialog& other) = delete;
  PackageChooserDialog(const Workspace& ws,
                       const IF_GraphicsLayerProvider* layerProvider,
                       QWidget* parent = nullptr) noexcept;
  ~PackageChooserDialog() noexcept;

  // Getters
  const tl::optional<Uuid>& getSelectedPackageUuid() const noexcept {
    return mSelectedPackageUuid;
  }

  // Operator Overloadings
  PackageChooserDialog& operator=(const PackageChooserDialog& rhs) = delete;

private:  // Methods
  void searchEditTextChanged(const QString& text) noexcept;
  void treeCategories_currentItemChanged(const QModelIndex& current,
                                         const QModelIndex& previous) noexcept;
  void listPackages_currentItemChanged(QListWidgetItem* current,
                                       QListWidgetItem* previous) noexcept;
  void listPackages_itemDoubleClicked(QListWidgetItem* item) noexcept;
  void searchPackages(const QString& input);
  void setSelectedCategory(const tl::optional<Uuid>& uuid) noexcept;
  void setSelectedPackage(const tl::optional<Uuid>& uuid) noexcept;
  void updatePreview(const FilePath& fp) noexcept;
  void accept() noexcept override;
  const QStringList& localeOrder() const noexcept;

private:  // Data
  const Workspace& mWorkspace;
  const IF_GraphicsLayerProvider* mLayerProvider;
  QScopedPointer<Ui::PackageChooserDialog> mUi;
  QScopedPointer<QAbstractItemModel> mCategoryTreeModel;
  tl::optional<Uuid> mSelectedCategoryUuid;
  tl::optional<Uuid> mSelectedPackageUuid;

  // preview
  QScopedPointer<Package> mPackage;
  QScopedPointer<GraphicsScene> mGraphicsScene;
  QScopedPointer<FootprintPreviewGraphicsItem> mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
