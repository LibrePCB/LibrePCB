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

#include "../project.h"
#include "board.h"
#include "boardlayerstack.h"

#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/common/utils/graphicslayerstackappearancesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardUserSettings::BoardUserSettings(Board& board) noexcept
  : QObject(&board),
    mBoard(board),
    mLayerSettings(
        new GraphicsLayerStackAppearanceSettings(board.getLayerStack())) {
}

BoardUserSettings::BoardUserSettings(Board&                   board,
                                     const BoardUserSettings& other) noexcept
  : BoardUserSettings(board) {
  *mLayerSettings = *other.mLayerSettings;
}

BoardUserSettings::BoardUserSettings(Board& board, const SExpression& node)
  : QObject(&board),
    mBoard(board),
    mLayerSettings(
        new GraphicsLayerStackAppearanceSettings(board.getLayerStack(), node)) {
}

BoardUserSettings::~BoardUserSettings() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardUserSettings::serialize(SExpression& root) const {
  mLayerSettings->serialize(root);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
