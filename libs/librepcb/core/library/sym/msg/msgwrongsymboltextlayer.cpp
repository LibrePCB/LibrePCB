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
#include "msgwrongsymboltextlayer.h"

#include "../../../geometry/text.h"
#include "../../../graphics/graphicslayer.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MsgWrongSymbolTextLayer::MsgWrongSymbolTextLayer(
    std::shared_ptr<const Text> text, const QString& expectedLayerName) noexcept
  : LibraryElementCheckMessage(
        Severity::Warning,
        tr("Layer of '%1' is not '%2'")
            .arg(text->getText(),
                 GraphicsLayer::getTranslation(expectedLayerName)),
        tr("The text element '%1' should normally be on layer '%2'.")
            .arg(text->getText(),
                 GraphicsLayer::getTranslation(expectedLayerName))),
    mText(text),
    mExpectedLayerName(expectedLayerName) {
}

MsgWrongSymbolTextLayer::~MsgWrongSymbolTextLayer() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
