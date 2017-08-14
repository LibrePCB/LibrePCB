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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include "boardusersettings.h"
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/domdocument.h>
#include <librepcb/common/utils/graphicslayerstackappearancesettings.h>
#include "board.h"
#include "boardlayerstack.h"
#include "../project.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

BoardUserSettings::BoardUserSettings(Board& board, const BoardUserSettings& other) noexcept :
    BoardUserSettings(board, false, false, true)
{
    *mLayerSettings = *other.mLayerSettings;
}

BoardUserSettings::BoardUserSettings(Board& board, bool restore, bool readOnly, bool create) :
    QObject(&board), mBoard(board)
{
    QString relpath = QString("user/boards/%1").arg(mBoard.getFilePath().getFilename());
    mXmlFilepath = mBoard.getProject().getPath().getPathTo(relpath);

    if (create || (!mXmlFilepath.isExistingFile())) {
        mXmlFile.reset(SmartXmlFile::create(mXmlFilepath));

        mLayerSettings.reset(new GraphicsLayerStackAppearanceSettings(mBoard.getLayerStack()));
    } else {
        mXmlFile.reset(new SmartXmlFile(mXmlFilepath, restore, readOnly));
        std::unique_ptr<DomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
        const DomElement& root = doc->getRoot();

        mLayerSettings.reset(new GraphicsLayerStackAppearanceSettings(
            mBoard.getLayerStack(), *root.getFirstChild("layers", true)));
    }
}

BoardUserSettings::~BoardUserSettings() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool BoardUserSettings::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    try {
        DomDocument doc(*serializeToDomElement("board_user_settings"));
        mXmlFile->save(doc, toOriginal);
    } catch (Exception& e) {
        success = false;
        errors.append(e.getMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardUserSettings::serialize(DomElement& root) const
{
    root.appendChild(mLayerSettings->serializeToDomElement("layers"));
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
