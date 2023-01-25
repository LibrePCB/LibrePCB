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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "symboleditorstate.h"

#include "../../../widgets/graphicsview.h"
#include "../../editorwidgetbase.h"

#include <librepcb/core/graphics/graphicslayer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SymbolEditorState::SymbolEditorState(const Context& context) noexcept
  : QObject(nullptr), mContext(context) {
}

SymbolEditorState::~SymbolEditorState() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

const PositiveLength& SymbolEditorState::getGridInterval() const noexcept {
  return mContext.graphicsView.getGridInterval();
}

const LengthUnit& SymbolEditorState::getLengthUnit() const noexcept {
  return mContext.lengthUnit;
}

QList<GraphicsLayer*> SymbolEditorState::getAllowedTextLayers() const noexcept {
  return mContext.editorContext.layerProvider.getLayers({
      GraphicsLayer::sSymbolOutlines,
      // GraphicsLayer::sSymbolHiddenGrabAreas, -> makes no sense for texts
      GraphicsLayer::sSymbolNames,
      GraphicsLayer::sSymbolValues,
      GraphicsLayer::sSchematicSheetFrames,
      GraphicsLayer::sSchematicDocumentation,
      GraphicsLayer::sSchematicComments,
      GraphicsLayer::sSchematicGuide,
  });
}

QList<GraphicsLayer*> SymbolEditorState::getAllowedCircleAndPolygonLayers()
    const noexcept {
  return mContext.editorContext.layerProvider.getLayers({
      GraphicsLayer::sSymbolOutlines,
      GraphicsLayer::sSymbolHiddenGrabAreas,
      GraphicsLayer::sSymbolNames,
      GraphicsLayer::sSymbolValues,
      GraphicsLayer::sSchematicSheetFrames,
      GraphicsLayer::sSchematicDocumentation,
      GraphicsLayer::sSchematicComments,
      GraphicsLayer::sSchematicGuide,
  });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
