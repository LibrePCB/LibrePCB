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
#include "packageeditorstate.h"

#include <librepcb/common/graphics/graphicslayer.h>
#include <librepcb/common/graphics/graphicsview.h>
#include <librepcb/common/gridproperties.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState::PackageEditorState(Context& context) noexcept
  : QObject(nullptr), mContext(context) {
}

PackageEditorState::~PackageEditorState() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

const PositiveLength& PackageEditorState::getGridInterval() const noexcept {
  return mContext.graphicsView.getGridProperties().getInterval();
}

const LengthUnit& PackageEditorState::getDefaultLengthUnit() const noexcept {
  return mContext.workspace.getSettings().defaultLengthUnit.get();
}

QList<GraphicsLayer*> PackageEditorState::getAllowedTextLayers() const
    noexcept {
  return mContext.layerProvider.getLayers({
      GraphicsLayer::sBoardSheetFrames,
      GraphicsLayer::sBoardOutlines,
      GraphicsLayer::sBoardMillingPth,
      GraphicsLayer::sBoardMeasures,
      GraphicsLayer::sBoardAlignment,
      GraphicsLayer::sBoardDocumentation,
      GraphicsLayer::sBoardComments,
      GraphicsLayer::sBoardGuide,
      GraphicsLayer::sTopPlacement,
      // GraphicsLayer::sTopHiddenGrabAreas, -> makes no sense for texts
      GraphicsLayer::sTopDocumentation,
      GraphicsLayer::sTopNames,
      GraphicsLayer::sTopValues,
      GraphicsLayer::sTopCopper,
      GraphicsLayer::sTopCourtyard,
      GraphicsLayer::sTopGlue,
      GraphicsLayer::sTopSolderPaste,
      GraphicsLayer::sTopStopMask,
      GraphicsLayer::sBotPlacement,
      // GraphicsLayer::sBotHiddenGrabAreas, -> makes no sense for texts
      GraphicsLayer::sBotDocumentation,
      GraphicsLayer::sBotNames,
      GraphicsLayer::sBotValues,
      GraphicsLayer::sBotCopper,
      GraphicsLayer::sBotCourtyard,
      GraphicsLayer::sBotGlue,
      GraphicsLayer::sBotSolderPaste,
      GraphicsLayer::sBotStopMask,
  });
}

QList<GraphicsLayer*> PackageEditorState::getAllowedCircleAndPolygonLayers()
    const noexcept {
  return mContext.layerProvider.getLayers({
      GraphicsLayer::sBoardSheetFrames, GraphicsLayer::sBoardOutlines,
      GraphicsLayer::sBoardMillingPth,  GraphicsLayer::sBoardMeasures,
      GraphicsLayer::sBoardAlignment,   GraphicsLayer::sBoardDocumentation,
      GraphicsLayer::sBoardComments,    GraphicsLayer::sBoardGuide,
      GraphicsLayer::sTopPlacement,     GraphicsLayer::sTopHiddenGrabAreas,
      GraphicsLayer::sTopDocumentation, GraphicsLayer::sTopNames,
      GraphicsLayer::sTopValues,        GraphicsLayer::sTopCopper,
      GraphicsLayer::sTopCourtyard,     GraphicsLayer::sTopGlue,
      GraphicsLayer::sTopSolderPaste,   GraphicsLayer::sTopStopMask,
      GraphicsLayer::sBotPlacement,     GraphicsLayer::sBotHiddenGrabAreas,
      GraphicsLayer::sBotDocumentation, GraphicsLayer::sBotNames,
      GraphicsLayer::sBotValues,        GraphicsLayer::sBotCopper,
      GraphicsLayer::sBotCourtyard,     GraphicsLayer::sBotGlue,
      GraphicsLayer::sBotSolderPaste,   GraphicsLayer::sBotStopMask,
  });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
