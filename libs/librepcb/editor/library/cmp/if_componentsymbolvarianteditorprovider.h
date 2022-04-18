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

#ifndef LIBREPCB_EDITOR_IF_COMPONENTSYMBOLVARIANTEDITORPROVIDER_H
#define LIBREPCB_EDITOR_IF_COMPONENTSYMBOLVARIANTEDITORPROVIDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentSymbolVariant;

namespace editor {

/*******************************************************************************
 *  Interface IF_ComponentSymbolVariantEditorProvider
 ******************************************************************************/

/**
 * @brief The IF_ComponentSymbolVariantEditorProvider interface
 */
class IF_ComponentSymbolVariantEditorProvider {
public:
  // Constructors / Destructor
  IF_ComponentSymbolVariantEditorProvider() {}
  IF_ComponentSymbolVariantEditorProvider(
      const IF_ComponentSymbolVariantEditorProvider& other) = delete;
  virtual ~IF_ComponentSymbolVariantEditorProvider() {}

  // General Methods
  virtual bool openComponentSymbolVariantEditor(
      std::shared_ptr<ComponentSymbolVariant> variant) noexcept = 0;

  // Operator Overloadings
  IF_ComponentSymbolVariantEditorProvider& operator=(
      const IF_ComponentSymbolVariantEditorProvider& rhs) = delete;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
