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
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class ComponentSymbolVariant;
class DefaultGraphicsLayerProvider;
class Device;
class GraphicsScene;
class Package;
class Symbol;
class WorkspaceLibraryDb;

namespace editor {

class FootprintGraphicsItem;
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
    QString name;
    FilePath pkgFp;
    QString pkgName;
    bool match = false;
  };

  struct SearchResultComponent {
    QString name;
    QHash<FilePath, SearchResultDevice> devices;
    bool match = false;
  };

  typedef QHash<FilePath, SearchResultComponent> SearchResult;

public:
  // Constructors / Destructor
  explicit AddComponentDialog(const WorkspaceLibraryDb& db,
                              const QStringList& localeOrder,
                              const QStringList& normOrder,
                              QWidget* parent = nullptr);
  ~AddComponentDialog() noexcept;

  // Getters
  tl::optional<Uuid> getSelectedComponentUuid() const noexcept;
  tl::optional<Uuid> getSelectedSymbVarUuid() const noexcept;
  tl::optional<Uuid> getSelectedDeviceUuid() const noexcept;

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
  void searchComponents(const QString& input);
  SearchResult searchComponentsAndDevices(const QString& input);
  void setSelectedCategory(const tl::optional<Uuid>& categoryUuid);
  void setSelectedComponent(const Component* cmp);
  void setSelectedSymbVar(
      std::shared_ptr<const ComponentSymbolVariant> symbVar);
  void setSelectedDevice(const Device* dev);
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

  // Attributes
  tl::optional<Uuid> mSelectedCategoryUuid;
  std::shared_ptr<const Component> mSelectedComponent;
  std::shared_ptr<const ComponentSymbolVariant> mSelectedSymbVar;
  QScopedPointer<const Device> mSelectedDevice;
  QScopedPointer<Package> mSelectedPackage;
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
