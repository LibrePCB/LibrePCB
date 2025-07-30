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

#include <librepcb/core/types/layer.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PackageEditorState::PackageEditorState(Context& context) noexcept
  : QObject(nullptr), mContext(context), mAdapter(mContext.adapter) {
}

PackageEditorState::~PackageEditorState() noexcept {
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

GraphicsScene* PackageEditorState::getGraphicsScene() noexcept {
  return mAdapter.fsmGetGraphicsScene();
}

PositiveLength PackageEditorState::getGridInterval() const noexcept {
  return mAdapter.fsmGetGridInterval();
}

const LengthUnit& PackageEditorState::getLengthUnit() const noexcept {
  return mContext.lengthUnit;
}

QWidget* PackageEditorState::parentWidget() noexcept {
  return qApp->activeWindow();
}

const QSet<const Layer*>& PackageEditorState::getAllowedTextLayers() noexcept {
  static const QSet<const Layer*> layers = {
      &Layer::boardSheetFrames(),
      &Layer::boardOutlines(),
      &Layer::boardCutouts(),
      &Layer::boardPlatedCutouts(),
      &Layer::boardMeasures(),
      &Layer::boardAlignment(),
      &Layer::boardDocumentation(),
      &Layer::boardComments(),
      &Layer::boardGuide(),
      &Layer::topLegend(),
      // &Layer::topHiddenGrabAreas(), -> makes no sense for texts
      &Layer::topDocumentation(),
      &Layer::topNames(),
      &Layer::topValues(),
      &Layer::topCopper(),
      &Layer::topCourtyard(),
      &Layer::topGlue(),
      &Layer::topSolderPaste(),
      &Layer::topStopMask(),
      &Layer::botLegend(),
      // &Layer::botHiddenGrabAreas(), -> makes no sense for texts
      &Layer::botDocumentation(),
      &Layer::botNames(),
      &Layer::botValues(),
      &Layer::botCopper(),
      &Layer::botCourtyard(),
      &Layer::botGlue(),
      &Layer::botSolderPaste(),
      &Layer::botStopMask(),
  };
  return layers;
}

const QSet<const Layer*>&
    PackageEditorState::getAllowedCircleAndPolygonLayers() noexcept {
  static const QSet<const Layer*> layers = {
      &Layer::boardSheetFrames(),
      &Layer::boardOutlines(),
      &Layer::boardCutouts(),
      &Layer::boardPlatedCutouts(),
      &Layer::boardMeasures(),
      &Layer::boardAlignment(),
      &Layer::boardDocumentation(),
      &Layer::boardComments(),
      &Layer::boardGuide(),
      &Layer::topLegend(),
      &Layer::topHiddenGrabAreas(),
      &Layer::topDocumentation(),
      &Layer::topPackageOutlines(),
      &Layer::topNames(),
      &Layer::topValues(),
      &Layer::topCopper(),
      &Layer::topCourtyard(),
      &Layer::topGlue(),
      &Layer::topSolderPaste(),
      &Layer::topStopMask(),
      &Layer::botLegend(),
      &Layer::botHiddenGrabAreas(),
      &Layer::botDocumentation(),
      &Layer::botPackageOutlines(),
      &Layer::botNames(),
      &Layer::botValues(),
      &Layer::botCopper(),
      &Layer::botCourtyard(),
      &Layer::botGlue(),
      &Layer::botSolderPaste(),
      &Layer::botStopMask(),
  };
  return layers;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
