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
#include <librepcb/common/fileio/smartsexprfile.h>
#include <librepcb/common/fileio/sexpression.h>
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
    mFilepath = mBoard.getProject().getPath().getPathTo(relpath);

    if (create || (!mFilepath.isExistingFile())) {
        mFile.reset(SmartSExprFile::create(mFilepath));

        mLayerSettings.reset(new GraphicsLayerStackAppearanceSettings(mBoard.getLayerStack()));
    } else {
        mFile.reset(new SmartSExprFile(mFilepath, restore, readOnly));

        try {
            SExpression root = mFile->parseFileAndBuildDomTree();

            mLayerSettings.reset(new GraphicsLayerStackAppearanceSettings(
                mBoard.getLayerStack(), root));
        } catch (const Exception&) {
            // Project user settings are normally not put under version control and thus
            // the likelyhood of parse errors is higher (e.g. when switching to an older,
            // now incompatible revision). To avoid frustration, we just ignore these
            // errors and load the default settings instead...
            qCritical() << "Could not open board user settings, defaults will be used instead!";
            mLayerSettings.reset(new GraphicsLayerStackAppearanceSettings(mBoard.getLayerStack()));
        }
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
        SExpression doc(serializeToDomElement("librepcb_board_user_settings"));
        mFile->save(doc, toOriginal);
    } catch (Exception& e) {
        success = false;
        errors.append(e.getMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void BoardUserSettings::serialize(SExpression& root) const
{
    mLayerSettings->serialize(root);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
