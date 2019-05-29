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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTCHOOSERDIALOG_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTCHOOSERDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsScene;
class IF_GraphicsLayerProvider;

namespace workspace {
class Workspace;
}

namespace library {

class Component;
class Symbol;
class SymbolPreviewGraphicsItem;

namespace editor {

namespace Ui {
class ComponentChooserDialog;
}

/*******************************************************************************
 *  Class ComponentChooserDialog
 ******************************************************************************/

/**
 * @brief The ComponentChooserDialog class
 */
class ComponentChooserDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentChooserDialog()                                    = delete;
  ComponentChooserDialog(const ComponentChooserDialog& other) = delete;
  ComponentChooserDialog(const workspace::Workspace&     ws,
                         const IF_GraphicsLayerProvider* layerProvider,
                         QWidget* parent = nullptr) noexcept;
  ~ComponentChooserDialog() noexcept;

  // Getters
  const tl::optional<Uuid>& getSelectedComponentUuid() const noexcept {
    return mSelectedComponentUuid;
  }

  // Operator Overloadings
  ComponentChooserDialog& operator=(const ComponentChooserDialog& rhs) = delete;

private:  // Methods
  void searchEditTextChanged(const QString& text) noexcept;
  void treeCategories_currentItemChanged(const QModelIndex& current,
                                         const QModelIndex& previous) noexcept;
  void listComponents_currentItemChanged(QListWidgetItem* current,
                                         QListWidgetItem* previous) noexcept;
  void listComponents_itemDoubleClicked(QListWidgetItem* item) noexcept;
  void searchComponents(const QString& input);
  void setSelectedCategory(const tl::optional<Uuid>& uuid) noexcept;
  void setSelectedComponent(const tl::optional<Uuid>& uuid) noexcept;
  void updatePreview(const FilePath& fp) noexcept;
  void accept() noexcept override;
  const QStringList& localeOrder() const noexcept;

private:  // Data
  const workspace::Workspace&                mWorkspace;
  const IF_GraphicsLayerProvider*            mLayerProvider;
  QScopedPointer<Ui::ComponentChooserDialog> mUi;
  QScopedPointer<QAbstractItemModel>         mCategoryTreeModel;
  tl::optional<Uuid>                         mSelectedCategoryUuid;
  tl::optional<Uuid>                         mSelectedComponentUuid;

  // preview
  QScopedPointer<Component>                         mComponent;
  QScopedPointer<GraphicsScene>                     mGraphicsScene;
  QList<std::shared_ptr<Symbol>>                    mSymbols;
  QList<std::shared_ptr<SymbolPreviewGraphicsItem>> mSymbolGraphicsItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPONENTCHOOSERDIALOG_H
