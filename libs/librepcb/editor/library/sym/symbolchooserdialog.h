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

#ifndef LIBREPCB_EDITOR_SYMBOLCHOOSERDIALOG_H
#define LIBREPCB_EDITOR_SYMBOLCHOOSERDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Symbol;
class Workspace;

namespace editor {

class GraphicsLayerList;
class GraphicsScene;
class SymbolGraphicsItem;

namespace Ui {
class SymbolChooserDialog;
}

/*******************************************************************************
 *  Class SymbolChooserDialog
 ******************************************************************************/

/**
 * @brief The SymbolChooserDialog class
 */
class SymbolChooserDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolChooserDialog() = delete;
  SymbolChooserDialog(const SymbolChooserDialog& other) = delete;
  SymbolChooserDialog(const Workspace& ws, const GraphicsLayerList& layers,
                      QWidget* parent = nullptr) noexcept;
  ~SymbolChooserDialog() noexcept;

  // Getters
  std::optional<Uuid> getSelectedSymbolUuid() const noexcept;
  QString getSelectedSymbolNameTr() const noexcept;
  QString getSelectedSymbolDescriptionTr() const noexcept;

  // Operator Overloadings
  SymbolChooserDialog& operator=(const SymbolChooserDialog& rhs) = delete;

private:  // Methods
  void searchEditTextChanged(const QString& text) noexcept;
  void treeCategories_currentItemChanged(const QModelIndex& current,
                                         const QModelIndex& previous) noexcept;
  void listSymbols_currentItemChanged(QListWidgetItem* current,
                                      QListWidgetItem* previous) noexcept;
  void listSymbols_itemDoubleClicked(QListWidgetItem* item) noexcept;
  void searchSymbols(const QString& input);
  void setSelectedCategory(const std::optional<Uuid>& uuid) noexcept;
  void setSelectedSymbol(const FilePath& fp) noexcept;
  void accept() noexcept override;
  const QStringList& localeOrder() const noexcept;

private:  // Data
  const Workspace& mWorkspace;
  const GraphicsLayerList& mLayers;
  QScopedPointer<Ui::SymbolChooserDialog> mUi;
  QScopedPointer<QAbstractItemModel> mCategoryTreeModel;
  QScopedPointer<GraphicsScene> mPreviewScene;
  bool mCategorySelected;
  std::optional<Uuid> mSelectedCategoryUuid;
  std::unique_ptr<Symbol> mSelectedSymbol;
  QScopedPointer<SymbolGraphicsItem> mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
