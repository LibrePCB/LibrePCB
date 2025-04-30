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

#ifndef LIBREPCB_EDITOR_COMPONENTSYMBOLVARIANTEDITDIALOG_H
#define LIBREPCB_EDITOR_COMPONENTSYMBOLVARIANTEDITDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/cmp/componentsymbolvariant.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class Symbol;
class Workspace;

namespace editor {

class GraphicsLayerList;
class GraphicsScene;
class LibraryElementCache;
class SymbolGraphicsItem;

namespace Ui {
class ComponentSymbolVariantEditDialog;
}

/*******************************************************************************
 *  Class ComponentSymbolVariantEditDialog
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantEditDialog class
 */
class ComponentSymbolVariantEditDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentSymbolVariantEditDialog() = delete;
  ComponentSymbolVariantEditDialog(
      const ComponentSymbolVariantEditDialog& other) = delete;
  ComponentSymbolVariantEditDialog(
      const Workspace& ws, std::shared_ptr<const Component> cmp,
      std::shared_ptr<ComponentSymbolVariant> symbVar,
      QWidget* parent = nullptr) noexcept;
  ~ComponentSymbolVariantEditDialog() noexcept;

  // Setters
  void setReadOnly(bool readOnly) noexcept;

  // Operator Overloadings
  ComponentSymbolVariantEditDialog& operator=(
      const ComponentSymbolVariantEditDialog& rhs) = delete;

private:  // Methods
  void accept() noexcept override;
  void schedulePreviewUpdate() noexcept;
  void schedulePreviewTextsUpdate() noexcept;
  void updatePreview() noexcept;

private:  // Data
  const Workspace& mWorkspace;
  std::shared_ptr<const Component> mComponent;
  std::shared_ptr<ComponentSymbolVariant> mOriginalSymbVar;
  ComponentSymbolVariant mSymbVar;
  QScopedPointer<GraphicsScene> mGraphicsScene;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::shared_ptr<LibraryElementCache> mLibraryElementCache;
  QScopedPointer<Ui::ComponentSymbolVariantEditDialog> mUi;

  bool mPreviewUpdateScheduled;
  bool mPreviewTextsUpdateScheduled;
  QList<std::shared_ptr<Symbol>> mSymbols;
  QList<std::shared_ptr<SymbolGraphicsItem>> mGraphicsItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
