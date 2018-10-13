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

#ifndef LIBREPCB_LIBRARY_EDITOR_SYMBOLCHOOSERDIALOG_H
#define LIBREPCB_LIBRARY_EDITOR_SYMBOLCHOOSERDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;
class GraphicsScene;

namespace workspace {
class Workspace;
}

namespace library {

class Symbol;
class SymbolGraphicsItem;

namespace editor {

namespace Ui {
class SymbolChooserDialog;
}

/*******************************************************************************
 *  Class SymbolChooserDialog
 ******************************************************************************/

/**
 * @brief The SymbolChooserDialog class
 *
 * @author ubruhin
 * @date 2017-03-19
 */
class SymbolChooserDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolChooserDialog()                                 = delete;
  SymbolChooserDialog(const SymbolChooserDialog& other) = delete;
  SymbolChooserDialog(const workspace::Workspace&     ws,
                      const IF_GraphicsLayerProvider& layerProvider,
                      QWidget* parent = nullptr) noexcept;
  ~SymbolChooserDialog() noexcept;

  // Getters
  tl::optional<Uuid> getSelectedSymbolUuid() const noexcept;
  QString            getSelectedSymbolNameTr() const noexcept;
  QString            getSelectedSymbolDescriptionTr() const noexcept;

  // Operator Overloadings
  SymbolChooserDialog& operator=(const SymbolChooserDialog& rhs) = delete;

private:  // Methods
  void treeCategories_currentItemChanged(const QModelIndex& current,
                                         const QModelIndex& previous) noexcept;
  void listSymbols_currentItemChanged(QListWidgetItem* current,
                                      QListWidgetItem* previous) noexcept;
  void listSymbols_itemDoubleClicked(QListWidgetItem* item) noexcept;
  void setSelectedCategory(const tl::optional<Uuid>& uuid) noexcept;
  void setSelectedSymbol(const FilePath& fp) noexcept;
  void accept() noexcept override;
  const QStringList& localeOrder() const noexcept;

private:  // Data
  const workspace::Workspace&             mWorkspace;
  const IF_GraphicsLayerProvider&         mLayerProvider;
  QScopedPointer<Ui::SymbolChooserDialog> mUi;
  QScopedPointer<QAbstractItemModel>      mCategoryTreeModel;
  QScopedPointer<GraphicsScene>           mPreviewScene;
  tl::optional<Uuid>                      mSelectedCategoryUuid;
  QScopedPointer<Symbol>                  mSelectedSymbol;
  QScopedPointer<SymbolGraphicsItem>      mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_SYMBOLCHOOSERDIALOG_H
