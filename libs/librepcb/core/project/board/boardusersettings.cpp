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
#include "boardusersettings.h"

#include "../../graphics/graphicslayerstackappearancesettings.h"
#include "../../serialization/sexpression.h"
#include "../project.h"
#include "board.h"
#include "boardlayerstack.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardUserSettings::BoardUserSettings(Board& board) noexcept
  : QObject(&board),
    mBoard(board),
    mLayerSettings(
        new GraphicsLayerStackAppearanceSettings(board.getLayerStack())),
    mPlanesVisibility() {
}

BoardUserSettings::BoardUserSettings(Board& board,
                                     const BoardUserSettings& other) noexcept
  : BoardUserSettings(board) {
  *mLayerSettings = *other.mLayerSettings;
  mPlanesVisibility = other.mPlanesVisibility;
}

BoardUserSettings::BoardUserSettings(Board& board, const SExpression& node,
                                     const Version& fileFormat)
  : QObject(&board),
    mBoard(board),
    mLayerSettings(new GraphicsLayerStackAppearanceSettings(
        board.getLayerStack(), node, fileFormat)),
    mPlanesVisibility() {
  // Load planes visibility.
  // Note: Support for this was added in file format v0.2. However, there is
  // no need to check the file format here - v0.1 simply doesn't contain these
  // nodes.
  foreach (const SExpression& node, node.getChildren("plane")) {
    Uuid uuid = deserialize<Uuid>(node.getChild("@0"), fileFormat);
    bool visible = deserialize<bool>(node.getChild("visible/@0"), fileFormat);
    mPlanesVisibility[uuid] = visible;
  }
}

BoardUserSettings::~BoardUserSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardUserSettings::serialize(SExpression& root) const {
  mLayerSettings->serialize(root);

  for (auto i = mPlanesVisibility.constBegin();
       i != mPlanesVisibility.constEnd(); i++) {
    SExpression node = SExpression::createList("plane");
    node.appendChild(i.key());
    node.appendChild("visible", i.value(), false);
    root.appendChild(node, true);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
