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

#ifndef LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTEDITDIALOG_H
#define LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTEDITDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>
#include <librepcb/library/cmp/componentsymbolvariant.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class DefaultGraphicsLayerProvider;
class GraphicsScene;

namespace workspace {
class Workspace;
}

namespace library {

class Component;
class Symbol;
class SymbolGraphicsItem;

namespace editor {

namespace Ui {
class ComponentSymbolVariantEditDialog;
}

/*******************************************************************************
 *  Class ComponentSymbolVariantEditDialog
 ******************************************************************************/

/**
 * @brief The ComponentSymbolVariantEditDialog class
 *
 * @author ubruhin
 * @date 2017-03-18
 */
class ComponentSymbolVariantEditDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentSymbolVariantEditDialog() = delete;
  ComponentSymbolVariantEditDialog(
      const ComponentSymbolVariantEditDialog& other) = delete;
  ComponentSymbolVariantEditDialog(const workspace::Workspace& ws,
                                   const Component&            cmp,
                                   ComponentSymbolVariant&     symbVar,
                                   QWidget* parent = nullptr) noexcept;
  ~ComponentSymbolVariantEditDialog() noexcept;

  // Operator Overloadings
  ComponentSymbolVariantEditDialog& operator       =(
      const ComponentSymbolVariantEditDialog& rhs) = delete;

private:  // Methods
  void accept() noexcept override;
  void updateGraphicsItems() noexcept;

private:  // Data
  const workspace::Workspace&                          mWorkspace;
  const Component&                                     mComponent;
  ComponentSymbolVariant&                              mOriginalSymbVar;
  ComponentSymbolVariant                               mSymbVar;
  QScopedPointer<Ui::ComponentSymbolVariantEditDialog> mUi;
  QScopedPointer<GraphicsScene>                        mGraphicsScene;
  QScopedPointer<DefaultGraphicsLayerProvider>         mGraphicsLayerProvider;

  QList<std::shared_ptr<Symbol>>             mSymbols;
  QList<std::shared_ptr<SymbolGraphicsItem>> mGraphicsItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_COMPONENTSYMBOLVARIANTEDITDIALOG_H
