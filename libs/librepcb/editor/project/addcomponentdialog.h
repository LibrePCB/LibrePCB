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

#ifndef LIBREPCB_EDITOR_ADDCOMPONENTDIALOG_H
#define LIBREPCB_EDITOR_ADDCOMPONENTDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../workspace/categorytreemodel.h"

#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/library/dev/part.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/types/uuid.h>
#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class ComponentSymbolVariant;
class Device;
class Part;
class Symbol;
class Theme;
class WorkspaceLibraryDb;

namespace editor {

class DefaultGraphicsLayerProvider;
class FootprintGraphicsItem;
class GraphicsScene;
class SymbolGraphicsItem;

namespace Ui {
class AddComponentDialog;
}

/*******************************************************************************
 *  Class AddComponentDialog
 ******************************************************************************/

/**
 * @brief The AddComponentDialog class
 */
class AddComponentDialog final : public QDialog {
  Q_OBJECT

  // Types
  struct SearchResultDevice {
    tl::optional<Uuid> uuid;
    QString name;
    bool deprecated = false;
    FilePath pkgFp;
    QString pkgName;
    PartList parts;
    bool match = false;
  };

  struct SearchResultComponent {
    QString name;
    bool deprecated = false;
    QHash<FilePath, SearchResultDevice> devices;
    bool match = false;
  };

  struct SearchResult {
    QHash<FilePath, SearchResultComponent> components;
    int deviceCount = 0;
    int partsCount = 0;
  };

public:
  // Constructors / Destructor
  explicit AddComponentDialog(const WorkspaceLibraryDb& db,
                              const QStringList& localeOrder,
                              const QStringList& normOrder, const Theme& theme,
                              QWidget* parent = nullptr);
  ~AddComponentDialog() noexcept;

  // Getters
  std::shared_ptr<const Component> getSelectedComponent() const noexcept {
    return mSelectedComponent;
  }
  std::shared_ptr<const ComponentSymbolVariant> getSelectedSymbolVariant() const
      noexcept {
    return mSelectedSymbVar;
  }
  std::shared_ptr<const Device> getSelectedDevice() const noexcept {
    return mSelectedDevice;
  }
  std::shared_ptr<const Part> getSelectedPart() const noexcept {
    return mSelectedPart;
  }
  tl::optional<Package::AssemblyType> getSelectedPackageAssemblyType() const
      noexcept;

  /**
   * @brief Check if dialog shall be opened again after the current component
   *
   * Returns the checked state of the "Add More" checkbox, i.e. whether the
   * caller should open this dialog again after finishing placement of the
   * component.
   *
   * @retval true   Must open this dialog again after placing the component.
   * @retval false  Shall not open this dialog again, exit placement tool.
   */
  bool getAutoOpenAgain() const noexcept;

  // Setters
  void setLocaleOrder(const QStringList& order) noexcept;
  void setNormOrder(const QStringList& order) noexcept { mNormOrder = order; }

  // General Methods
  void selectComponentByKeyword(
      const QString keyword,
      const tl::optional<Uuid>& selectedDevice = tl::nullopt) noexcept;

private slots:
  void searchEditTextChanged(const QString& text) noexcept;
  void treeCategories_currentItemChanged(const QModelIndex& current,
                                         const QModelIndex& previous) noexcept;
  void treeComponents_currentItemChanged(QTreeWidgetItem* current,
                                         QTreeWidgetItem* previous) noexcept;
  void treeComponents_itemDoubleClicked(QTreeWidgetItem* item,
                                        int column) noexcept;
  void cbxSymbVar_currentIndexChanged(int index) noexcept;

private:
  // Private Methods
  void searchComponents(const QString& input,
                        const tl::optional<Uuid>& selectedDevice = tl::nullopt,
                        bool selectFirstDevice = false);
  SearchResult search(const QString& input);
  void setSelectedCategory(const tl::optional<Uuid>& categoryUuid);
  void setSelectedComponent(std::shared_ptr<const Component> cmp);
  void setSelectedSymbVar(
      std::shared_ptr<const ComponentSymbolVariant> symbVar);
  void setSelectedDevice(std::shared_ptr<const Device> dev);
  void setSelectedPart(std::shared_ptr<const Part> part);
  void addPartItem(std::shared_ptr<Part> part, QTreeWidgetItem* parent);
  void accept() noexcept;

  // General
  const WorkspaceLibraryDb& mDb;
  QStringList mLocaleOrder;
  QStringList mNormOrder;
  QScopedPointer<Ui::AddComponentDialog> mUi;
  QScopedPointer<GraphicsScene> mComponentPreviewScene;
  QScopedPointer<GraphicsScene> mDevicePreviewScene;
  QScopedPointer<DefaultGraphicsLayerProvider> mGraphicsLayerProvider;
  QScopedPointer<CategoryTreeModel> mCategoryTreeModel;
  QString mCurrentSearchTerm;

  // Attributes
  tl::optional<Uuid> mSelectedCategoryUuid;
  std::shared_ptr<const Component> mSelectedComponent;
  std::shared_ptr<const ComponentSymbolVariant> mSelectedSymbVar;
  std::shared_ptr<const Device> mSelectedDevice;
  std::unique_ptr<Package> mSelectedPackage;
  std::shared_ptr<const Part> mSelectedPart;
  QList<std::shared_ptr<Symbol>> mPreviewSymbols;
  QList<std::shared_ptr<SymbolGraphicsItem>> mPreviewSymbolGraphicsItems;
  QScopedPointer<FootprintGraphicsItem> mPreviewFootprintGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
